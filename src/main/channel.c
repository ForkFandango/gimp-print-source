/*
 *
 *   Dither routine entrypoints
 *
 *   Copyright 2003 Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <math.h>
#include <string.h>

#ifdef __GNUC__
#define inline __inline__
// No reason to inline functions outside of the inner loop.
#define NOINLINE __attribute__ ((noinline))
#else
$define NOINLINE
#endif

#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define FMIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
  double value;
  double lower;
  double upper;
  double cutoff;
  unsigned short s_density;
  stp_curve_t *curve;
} stpi_subchannel_t;

typedef struct
{
  unsigned subchannel_count;
  stpi_subchannel_t *sc;
  unsigned short *lut;
  const double *hue_map;
  size_t h_count;
  stp_curve_t *curve;
} stpi_channel_t;

typedef struct
{
  stpi_channel_t *c;
  stp_curve_t *gcr_curve;
  unsigned curve_count;
  unsigned gloss_limit;
  unsigned short *input_data;
  unsigned short *multi_tmp;
  unsigned short *gcr_data;
  unsigned short *split_input;
  unsigned short *output_data;
  unsigned short *alloc_data_1;
  unsigned short *alloc_data_2;
  unsigned short *alloc_data_3;
  unsigned char *output_data_8bit;
  size_t width;
  double cyan_balance;
  double magenta_balance;
  double yellow_balance;
  unsigned channel_count;
  unsigned total_channels;
  unsigned input_channels;
  unsigned gcr_channels;
  unsigned aux_output_channels;
  unsigned ink_limit;
  unsigned max_density;
  int black_channel;
  int gloss_channel;
  int gloss_physical_channel;
  int initialized;
  int valid_8bit;
} stpi_channel_group_t;


static stpi_channel_group_t *
get_channel_group(const stp_vars_t *v)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stp_get_component_data(v, "Channel"));
  return cg;
}

static void
clear_a_channel(stpi_channel_group_t *cg, int channel)
{
  if (channel < cg->channel_count)
    {
      int i;
      for (i = 0 ; i < cg->c[channel].subchannel_count ; i++)
        {
	  if (cg->c[channel].sc[i].curve)
	    {
	      stp_curve_destroy(cg->c[channel].sc[i].curve);
	      cg->c[channel].sc[i].curve = NULL;
	    }
        }

      STP_SAFE_FREE(cg->c[channel].sc);
      STP_SAFE_FREE(cg->c[channel].lut);
      if (cg->c[channel].curve)
	{
	  stp_curve_destroy(cg->c[channel].curve);
	  cg->c[channel].curve = NULL;
	}
      cg->c[channel].subchannel_count = 0;
    }
}

static void
stpi_channel_clear(void *vc)
{
  stpi_channel_group_t *cg = (stpi_channel_group_t *) vc;
  int i;
  if (cg->channel_count > 0)
    for (i = 0; i < cg->channel_count; i++)
      clear_a_channel(cg, i);

  STP_SAFE_FREE(cg->alloc_data_1);
  STP_SAFE_FREE(cg->alloc_data_2);
  STP_SAFE_FREE(cg->alloc_data_3);
  STP_SAFE_FREE(cg->c);
  if (cg->gcr_curve)
    {
      stp_curve_destroy(cg->gcr_curve);
      cg->gcr_curve = NULL;
    }
  cg->channel_count = 0;
  cg->curve_count = 0;
  cg->aux_output_channels = 0;
  cg->total_channels = 0;
  cg->input_channels = 0;
  cg->initialized = 0;
  cg->valid_8bit = 0;
}

void
stp_channel_reset(stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (cg)
    stpi_channel_clear(cg);
}

void
stp_channel_reset_channel(stp_vars_t *v, int channel)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (cg)
    clear_a_channel(cg, channel);
}

static void
stpi_channel_free(void *vc)
{
  stpi_channel_clear(vc);
  stp_free(vc);
}

static stpi_subchannel_t *
get_channel(stp_vars_t *v, unsigned channel, unsigned subchannel)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return NULL;
  if (channel >= cg->channel_count)
    return NULL;
  if (subchannel >= cg->c[channel].subchannel_count)
    return NULL;
  return &(cg->c[channel].sc[subchannel]);
}

void
stp_channel_add(stp_vars_t *v, unsigned channel, unsigned subchannel,
		double value)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  stpi_channel_t *chan;
  stp_dprintf(STP_DBG_INK, v, "Add channel %d, %d, %f\n",
	      channel, subchannel, value);
  if (!cg)
    {
      cg = stp_zalloc(sizeof(stpi_channel_group_t));
      cg->black_channel = -1;
      cg->gloss_channel = -1;
      stp_allocate_component_data(v, "Channel", NULL, stpi_channel_free, cg);
      stp_dprintf(STP_DBG_INK, v, "*** Set up channel data ***\n");
    }
  if (channel >= cg->channel_count)
    {
      unsigned oc = cg->channel_count;
      cg->c = stp_realloc(cg->c, sizeof(stpi_channel_t) * (channel + 1));
      memset(cg->c + oc, 0, sizeof(stpi_channel_t) * (channel + 1 - oc));
      stp_dprintf(STP_DBG_INK, v, "*** Increment channel count from %d to %d\n",
		  oc, channel + 1);
      if (channel >= cg->channel_count)
	cg->channel_count = channel + 1;
    }
  chan = cg->c + channel;
  if (subchannel >= chan->subchannel_count)
    {
      unsigned oc = chan->subchannel_count;
      chan->sc =
	stp_realloc(chan->sc, sizeof(stpi_subchannel_t) * (subchannel + 1));
      (void) memset
	(chan->sc + oc, 0, sizeof(stpi_subchannel_t) * (subchannel + 1 - oc));
      chan->sc[subchannel].value = value;
      stp_dprintf(STP_DBG_INK, v,
		  "*** Increment subchannel count for %d from %d to %d\n",
		  channel, oc, subchannel + 1);
      if (subchannel >= chan->subchannel_count)
	chan->subchannel_count = subchannel + 1;
    }
  chan->sc[subchannel].value = value;
  chan->sc[subchannel].s_density = 65535;
  chan->sc[subchannel].cutoff = 0.75;
  if (chan->sc[subchannel].curve)
    {
      stp_curve_destroy(chan->sc[subchannel].curve);
      chan->sc[subchannel].curve = NULL;
    }
}

void
stp_channel_set_subchannel_curve(stp_vars_t *v,
				 unsigned channel, unsigned subchannel,
				 const stp_curve_t *curve)
{
  stpi_subchannel_t *sch = get_channel(v, channel, subchannel);
  if (sch)
    {
      /* Destroy old curve */
      if (sch->curve)
	stp_curve_destroy(sch->curve);
      sch->curve = NULL;

      if (curve)
        {
	  double lo, hi;
	  /* Duplicate input curve */
	  sch->curve = stp_curve_create_copy(curve);
	  if (!sch->curve)
	    return;

	  /* Figure out the bounds */
	  stp_curve_get_bounds(sch->curve, &lo, &hi);
	  if (lo != 0)
	    { /* If low point is not 0, we need to fix that. */
	      lo = - lo;
	      stp_curve_rescale(sch->curve, lo, STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
	      stp_curve_get_bounds(sch->curve, &lo, &hi);
	    }
	  if (hi != 65535)
	    {
	      hi = 65535 / hi; /* Compute scaling factor so that the highest value is 65535 */
	      stp_curve_rescale(sch->curve, hi, STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
	    }
	  if (stp_curve_count_points(sch->curve) != 65536)
	    stp_curve_resample(sch->curve, 65536); /* Make sure there are 64K points */
	}
    }
}

double
stp_channel_get_value(stp_vars_t *v, unsigned color, unsigned subchannel)
{
  stpi_subchannel_t *sch = get_channel(v, color, subchannel);
  if (sch)
    return sch->value;
  else
    return -1;
}

void
stp_channel_set_density_adjustment(stp_vars_t *v, int color, int subchannel,
				   double adjustment)
{
  stpi_subchannel_t *sch = get_channel(v, color, subchannel);
  if ((strcmp(stp_get_string_parameter(v, "STPIOutputType"), "Raw") == 0 &&
       strcmp(stp_get_string_parameter(v, "ColorCorrection"), "None") == 0) ||
      strcmp(stp_get_string_parameter(v, "ColorCorrection"), "Raw") == 0 ||
      strcmp(stp_get_string_parameter(v, "ColorCorrection"), "Predithered") == 0)
    {
      stp_dprintf(STP_DBG_INK, v,
		  "Ignoring channel_density channel %d subchannel %d adjustment %f\n",
		  color, subchannel, adjustment);
    }
  else
    {
      stp_dprintf(STP_DBG_INK, v,
		  "channel_density channel %d subchannel %d adjustment %f\n",
		  color, subchannel, adjustment);
      if (sch && adjustment >= 0 && adjustment <= 1)
	sch->s_density = adjustment * 65535;
    }
}

double
stp_channel_get_density_adjustment(stp_vars_t *v, int color, int subchannel)
{
  stpi_subchannel_t *sch = get_channel(v, color, subchannel);
  if (sch)
    return sch->s_density / 65535.0;
  else
    return -1;
}

void
stp_channel_set_ink_limit(stp_vars_t *v, double limit)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  stp_dprintf(STP_DBG_INK, v, "ink_limit %f\n", limit);
  if (cg && limit > 0)
    cg->ink_limit = 65535 * limit;
}

double
stp_channel_get_ink_limit(stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return 0.0;
  return cg->ink_limit / 65535.0;
}

void
stp_channel_set_black_channel(stp_vars_t *v, int channel)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  stp_dprintf(STP_DBG_INK, v, "black_channel %d\n", channel);
  if (cg)
    cg->black_channel = channel;
}

int
stp_channel_get_black_channel(stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (cg)
    return cg->black_channel;
  else
    return -1;
}

void
stp_channel_set_gloss_channel(stp_vars_t *v, int channel)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  stp_dprintf(STP_DBG_INK, v, "gloss_channel %d\n", channel);
  if (cg)
    cg->gloss_channel = channel;
}

int
stp_channel_get_gloss_channel(stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (cg)
    return cg->gloss_channel;
  else
    return -1;
}

void
stp_channel_set_gloss_limit(stp_vars_t *v, double limit)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  stp_dprintf(STP_DBG_INK, v, "gloss_limit %f\n", limit);
  if (cg && limit > 0)
    cg->gloss_limit = 65535 * limit;
}

double
stp_channel_get_gloss_limit(stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (cg)
    return cg->gloss_limit / 65535.0;
  else
    return 0;
}

void
stp_channel_set_cutoff_adjustment(stp_vars_t *v, int color, int subchannel,
				  double adjustment)
{
  stpi_subchannel_t *sch = get_channel(v, color, subchannel);
  stp_dprintf(STP_DBG_INK, v,
	      "channel_cutoff channel %d subchannel %d adjustment %f\n",
	      color, subchannel, adjustment);
  if (sch && adjustment >= 0)
    sch->cutoff = adjustment;
}

double
stp_channel_get_cutoff_adjustment(stp_vars_t *v, int color, int subchannel)
{
  stpi_subchannel_t *sch = get_channel(v, color, subchannel);
  if (sch)
    return sch->cutoff;
  else
    return -1.0;
}

void
stp_channel_set_gcr_curve(stp_vars_t *v, const stp_curve_t *curve)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return;
  stp_dprintf(STP_DBG_INK, v, "set_gcr_curve\n");
  if (curve)
    cg->gcr_curve = stp_curve_create_copy(curve);
  else
    cg->gcr_curve = NULL;
}

const stp_curve_t *
stp_channel_get_gcr_curve(stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return NULL;
  stp_dprintf(STP_DBG_INK, v, "get_gcr_curve\n");
  return cg->gcr_curve;
}

void
stp_channel_set_curve(stp_vars_t *v, int color, const stp_curve_t *curve)
{
  stpi_channel_t *ch;
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg || color >= cg->channel_count)
    return;
  ch = &(cg->c[color]);
  stp_dprintf(STP_DBG_INK, v, "set_curve channel %d set curve\n", color);
  if (ch)
    {
      if (curve)
	ch->curve = stp_curve_create_copy(curve);
      else
	ch->curve = NULL;
    }
}

const stp_curve_t *
stp_channel_get_curve(stp_vars_t *v, int color)
{
  stpi_channel_t *ch;
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg || color >= cg->channel_count)
    return NULL;
  ch = &(cg->c[color]);
  if (ch)
    return ch->curve;
  else
    return NULL;
}

static int
input_has_special_channels(const stpi_channel_group_t *cg)
{
  return (cg->curve_count > 0);
}

static int
output_needs_gcr(const stpi_channel_group_t *cg)
{
  return (cg->gcr_curve && cg->black_channel == 0);
}

static int
output_has_gloss(const stpi_channel_group_t *cg)
{
  return (cg->gloss_channel >= 0);
}

static int
input_needs_splitting(const stpi_channel_group_t *cg)
{
#if 0
  return cg->total_channels != cg->aux_output_channels;
#else
  int i;
  if (!cg || cg->channel_count <= 0)
    return 0;
  for (i = 0; i < cg->channel_count; i++)
    {
      if (cg->c[i].subchannel_count > 1)
	return 1;
    }
  return 0;
#endif
}

static void
stp_dump_channels(const stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  int i, j;
  stp_erprintf("   channel_count  %d\n", cg->channel_count);
  stp_erprintf("   total_channels %d\n", cg->total_channels);
  stp_erprintf("   input_channels %d\n", cg->input_channels);
  stp_erprintf("   aux_channels   %d\n", cg->aux_output_channels);
  stp_erprintf("   gcr_channels   %d\n", cg->gcr_channels);
  stp_erprintf("   width          %ld\n", (long)cg->width);
  stp_erprintf("   ink_limit      %d\n", cg->ink_limit);
  stp_erprintf("   gloss_limit    %d\n", cg->gloss_limit);
  stp_erprintf("   max_density    %d\n", cg->max_density);
  stp_erprintf("   curve_count    %d\n", cg->curve_count);
  stp_erprintf("   black_channel  %d\n", cg->black_channel);
  stp_erprintf("   gloss_channel  %d\n", cg->gloss_channel);
  stp_erprintf("   gloss_physical %d\n", cg->gloss_physical_channel);
  stp_erprintf("   cyan           %.3f\n", cg->cyan_balance);
  stp_erprintf("   magenta        %.3f\n", cg->magenta_balance);
  stp_erprintf("   yellow         %.3f\n", cg->yellow_balance);
  stp_erprintf("   input_data     %p\n", (void *) cg->input_data);
  stp_erprintf("   multi_tmp      %p\n", (void *) cg->multi_tmp);
  stp_erprintf("   split_input    %p\n", (void *) cg->split_input);
  stp_erprintf("   output_data    %p\n", (void *) cg->output_data);
  stp_erprintf("   gcr_data       %p\n", (void *) cg->gcr_data);
  stp_erprintf("   alloc_data_1   %p\n", (void *) cg->alloc_data_1);
  stp_erprintf("   alloc_data_2   %p\n", (void *) cg->alloc_data_2);
  stp_erprintf("   alloc_data_3   %p\n", (void *) cg->alloc_data_3);
  stp_erprintf("   gcr_curve      %p\n", (void *) cg->gcr_curve);
  for (i = 0; i < cg->channel_count; i++)
    {
      stp_erprintf("   Channel %d:\n", i);
      for (j = 0; j < cg->c[i].subchannel_count; j++)
	{
	  stpi_subchannel_t *sch = &(cg->c[i].sc[j]);
	  stp_erprintf("      Subchannel %d:\n", j);
	  stp_erprintf("         value   %.3f:\n", sch->value);
	  stp_erprintf("         lower   %.3f:\n", sch->lower);
	  stp_erprintf("         upper   %.3f:\n", sch->upper);
	  stp_erprintf("         cutoff  %.3f:\n", sch->cutoff);
	  if (sch->curve)
	    {
	      char *curve = stp_curve_write_string(sch->curve);
	      stp_erprintf("         curve   %s\n", curve);
	      stp_free(curve);
	    }
	  stp_erprintf("         density %d:\n", sch->s_density);
	}
    }
}

void
stp_channel_initialize(stp_vars_t *v, stp_image_t *image,
		       int input_channel_count)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  int width = stp_image_width(image);
  int curve_count = 0;
  int i, j, k;
  if (!cg)
    {
      cg = stp_zalloc(sizeof(stpi_channel_group_t));
      cg->black_channel = -1;
      stp_allocate_component_data(v, "Channel", NULL, stpi_channel_free, cg);
    }
  if (cg->initialized)
    return;
  cg->initialized = 1;
  cg->max_density = 0;
  if (cg->black_channel < -1 || cg->black_channel >= cg->channel_count)
    cg->black_channel = -1;
  for (i = 0; i < cg->channel_count; i++)
    {
      stpi_channel_t *c = &(cg->c[i]);
      int sc = c->subchannel_count;
      if (c->curve)
	{
	  curve_count++;
	  stp_curve_resample(c->curve, 4096);
	  c->hue_map = stp_curve_get_data(c->curve, &(c->h_count));
	  cg->curve_count++;
	}
      if (sc > 1)
	{
	  int next_breakpoint = 0;
	  int val = 0;

	  c->lut = stp_zalloc(sizeof(unsigned short) * sc * 65536);

	  for (k = 0 ; k < sc ; k++)
	    {
	      for (j = 0 ; j < 65536; j++)
	        {
		  if (c->sc[k].curve)
		    {
		      double data = 0;
		      stp_curve_get_point(c->sc[k].curve, j, &data);
		      c->lut[j * sc + k] = data;
		    }
		  else
		    {
		      goto traditional;
		    }
		}
	    }
	  goto lut_done;

	traditional:
	  next_breakpoint = c->sc[0].value * 65535 * c->sc[0].cutoff;
	  if (next_breakpoint > 65535)
	    next_breakpoint = 65535;
	  while (val <= next_breakpoint)
	    {
	      int value = (int) ((double) val / c->sc[0].value);
	      c->lut[val * sc + sc - 1] = value;
	      val++;
	    }

	  for (k = 0; k < sc - 1; k++)
	    {
	      double this_val = c->sc[k].value;
	      double next_val = c->sc[k + 1].value;
	      double this_cutoff = c->sc[k].cutoff;
	      double next_cutoff = c->sc[k + 1].cutoff;
	      int range;
	      int base = val;
	      double cutoff = sqrt(this_cutoff * next_cutoff);
	      next_breakpoint = next_val * 65535 * cutoff;
	      if (next_breakpoint > 65535)
		next_breakpoint = 65535;
	      range = next_breakpoint - val;
	      while (val <= next_breakpoint)
		{
		  double where = ((double) val - base) / (double) range;
		  double lower_val = base * (1.0 - where);
		  double lower_amount = lower_val / this_val;
		  double upper_amount = (val - lower_val) / next_val;
		  if (lower_amount > 65535.0)
		    lower_amount = 65535.0;
		  c->lut[val * sc + sc - k - 2] = upper_amount;
		  c->lut[val * sc + sc - k - 1] = lower_amount;
		  val++;
		}
	    }
	  while (val <= 65535)
	    {
	      c->lut[val * sc] = val / c->sc[sc - 1].value;
	      val++;
	    }
	}

lut_done:
      if (cg->gloss_channel != i && c->subchannel_count > 0)
	cg->aux_output_channels++;
      cg->total_channels += c->subchannel_count;
      for (j = 0; j < c->subchannel_count; j++)
	cg->max_density += c->sc[j].s_density;
    }
  if (cg->gloss_channel >= 0)
    {
      for (i = 0; i < cg->channel_count; i++)
	{
	  if (cg->gloss_channel == i)
	    break;
	  cg->gloss_physical_channel += cg->c[i].subchannel_count;
	}
    }

  cg->input_channels = input_channel_count;
  cg->width = width;
  cg->alloc_data_1 =
    stp_malloc(sizeof(unsigned short) * cg->total_channels * width);
  cg->output_data = cg->alloc_data_1;
  if (curve_count == 0)
    {
      cg->gcr_channels = cg->input_channels;
      if (input_needs_splitting(cg))
	{
	  cg->alloc_data_2 =
	    stp_malloc(sizeof(unsigned short) * cg->input_channels * width);
	  cg->input_data = cg->alloc_data_2;
	  cg->split_input = cg->input_data;
	  cg->gcr_data = cg->split_input;
	}
      else if (cg->gloss_channel != -1)
	{
	  cg->alloc_data_2 =
	    stp_malloc(sizeof(unsigned short) * cg->input_channels * width);
	  cg->input_data = cg->alloc_data_2;
	  cg->gcr_data = cg->output_data;
	  cg->gcr_channels = cg->total_channels;
	}
      else
	{
	  cg->input_data = cg->output_data;
	  cg->gcr_data = cg->output_data;
	}
      cg->aux_output_channels = cg->gcr_channels;
    }
  else
    {
      cg->alloc_data_2 =
	stp_malloc(sizeof(unsigned short) * cg->input_channels * width);
      cg->input_data = cg->alloc_data_2;
      if (input_needs_splitting(cg))
	{
	  cg->alloc_data_3 =
	    stp_malloc(sizeof(unsigned short) * cg->aux_output_channels * width);
	  cg->multi_tmp = cg->alloc_data_3;
	  cg->split_input = cg->multi_tmp;
	  cg->gcr_data = cg->split_input;
	}
      else
	{
	  cg->multi_tmp = cg->alloc_data_1;
	  cg->gcr_data = cg->output_data;
	  cg->aux_output_channels = cg->total_channels;
	}
      cg->gcr_channels = cg->aux_output_channels;
    }
  cg->cyan_balance = stp_get_float_parameter(v, "CyanBalance");
  cg->magenta_balance = stp_get_float_parameter(v, "MagentaBalance");
  cg->yellow_balance = stp_get_float_parameter(v, "YellowBalance");
  stp_dprintf(STP_DBG_INK, v, "stp_channel_initialize:\n");
  if (stp_get_debug_level() & STP_DBG_INK)
    stp_dump_channels(v);
}

static void NOINLINE
clear_channel(unsigned short *data, unsigned width, unsigned depth)
{
  int i;
  width *= depth;
  for (i = 0; i < width; i += depth)
    data[i] = 0;
}

static int NOINLINE
scale_channel(unsigned short *data, unsigned width, unsigned depth,
	      unsigned short density)
{
  int i;
  int retval = 0;
  unsigned short previous_data = 0;
  unsigned short previous_value = 0;
  width *= depth;
  for (i = 0; i < width; i += depth)
    {
      if (data[i] == previous_data)
	data[i] = previous_value;
      else if (data[i] == (unsigned short) 65535)
	{
	  data[i] = density;
	  retval = 1;
	}
      else if (data[i] > 0)
	{
	  unsigned short tval = (32767u + data[i] * density) / 65535u;
	  previous_data = data[i];
	  if (tval)
	    retval = 1;
	  previous_value = (unsigned short) tval;
	  data[i] = (unsigned short) tval;
	}
    }
  return retval;
}

static int NOINLINE
scan_channel(unsigned short *data, unsigned width, unsigned depth)
{
  int i;
  width *= depth;
  for (i = 0; i < width; i += depth)
    {
      if (data[i])
	return 1;
    }
  return 0;
}

static inline unsigned
ink_sum(const unsigned short *data, int total_channels)
{
  int j;
  unsigned total_ink = 0;
  for (j = 0; j < total_channels; j++)
    total_ink += data[j];
  return total_ink;
}

static int NOINLINE
limit_ink(stpi_channel_group_t *cg)
{
  int i;
  int retval = 0;
  unsigned short *ptr;
  if (!cg || cg->ink_limit == 0 || cg->ink_limit >= cg->max_density)
    return 0;
  cg->valid_8bit = 0;
  ptr = cg->output_data;
  for (i = 0; i < cg->width; i++)
    {
      int total_ink = ink_sum(ptr, cg->total_channels);
      if (total_ink > cg->ink_limit) /* Need to limit ink? */
	{
	  int j;
	  /*
	   * FIXME we probably should first try to convert light ink to dark
	   */
	  double ratio = (double) cg->ink_limit / (double) total_ink;
	  for (j = 0; j < cg->total_channels; j++)
	    ptr[j] *= ratio;
	  retval = 1;
	}
      ptr += cg->total_channels;
   }
  return retval;
}

static inline int
short_eq(const unsigned short *i1, const unsigned short *i2, size_t count)
{
#if 1
  int i;
  for (i = 0; i < count; i++)
    if (i1[i] != i2[i])
      return 0;
  return 1;
#else
  return !memcmp(i1, i2, count * sizeof(unsigned short));
#endif
}

static inline void
short_copy(unsigned short *out, const unsigned short *in, size_t count)
{
#if 1
  int i;
  for (i = 0; i < count; i++)
    out[i] = in[i];
#else
  (void) memcpy(out, in, count * sizeof(unsigned short));
#endif
}

static void NOINLINE
copy_channels(stpi_channel_group_t *cg)
{
  int i, j, k;
  const unsigned short *input;
  unsigned short *output;
  if (!cg)
    return;
  input = cg->input_data;
  output = cg->output_data;
  for (i = 0; i < cg->width; i++)
    {
      for (j = 0; j < cg->channel_count; j++)
	{
	  stpi_channel_t *ch = &(cg->c[j]);
	  for (k = 0; k < ch->subchannel_count; k++)
	    {
	      if (cg->gloss_channel != j)
		{
		  *output = *input++;
		}
	      output++;
	    }
	}
    }
}

static inline double
compute_hue(int c, int m, int y, int max)
{
  double h;
  if (max == c)
    h = (m - y) / (double) max;
  else if (max == m)
    h = 2 + ((y - c) / (double) max);
  else
    h = 4 + ((c - m) / (double) max);
  if (h < 0)
    h += 6;
  else if (h >= 6)
    h -= 6;
  return h;
}

static inline double
interpolate_value(const double *vec, double val)
{
  double base = floor(val);
  double frac = val - base;
  int ibase = (int) base;
  double lval = vec[ibase];
  if (frac > 0)
    lval += (vec[ibase + 1] - lval) * frac;
  return lval;
}

static void NOINLINE
generate_special_channels(stpi_channel_group_t *cg)
{
  int i, j;
  const unsigned short *input_cache = NULL;
  const unsigned short *output_cache = NULL;
  const unsigned short *input;
  unsigned short *output;
  int offset;
  int outbytes;
  if (!cg)
    return;
  cg->valid_8bit = 0;
  input = cg->input_data;
  output = cg->multi_tmp;
  offset = (cg->black_channel >= 0 ? 0 : -1);
  outbytes = cg->aux_output_channels * sizeof(unsigned short);
  for (i = 0; i < cg->width;
       input += cg->input_channels, output += cg->aux_output_channels, i++)
    {
      if (input_cache && short_eq(input_cache, input, cg->input_channels))
	{
	  memcpy(output, output_cache, outbytes);
	}
      else
	{
	  int c = input[STP_ECOLOR_C + offset];
	  int m = input[STP_ECOLOR_M + offset];
	  int y = input[STP_ECOLOR_Y + offset];
	  int min = FMIN(c, FMIN(m, y));
	  int max = FMAX(c, FMAX(m, y));
	  if (max > min)	/* Otherwise it's gray, and we don't care */
	    {
	      double hue;
	      /*
	       * We're only interested in converting color components
	       * to special inks.  We want to compute the hue and
	       * luminosity to determine what we want to convert.
	       * Since we're eliminating all grayscale component, the
	       * computations become simpler.
	       */
	      c -= min;
	      m -= min;
	      y -= min;
	      max -= min;
	      if (offset == 0)
		output[STP_ECOLOR_K] = input[STP_ECOLOR_K];
	      hue = compute_hue(c, m, y, max);
	      for (j = 1; j < cg->aux_output_channels - offset; j++)
		{
		  stpi_channel_t *ch = &(cg->c[j]);
		  if (ch->hue_map)
		    output[j + offset] =
		      max * interpolate_value(ch->hue_map,
					      hue * ch->h_count / 6.0);
		  else
		    output[j + offset] = 0;
		}
	      output[STP_ECOLOR_C + offset] += min;
	      output[STP_ECOLOR_M + offset] += min;
	      output[STP_ECOLOR_Y + offset] += min;
	    }
	  else
	    {
	      for (j = 0; j < 4 + offset; j++)
		output[j] = input[j];
	      for (j = 4 + offset; j < cg->aux_output_channels; j++)
		output[j] = 0;
	    }
	}
      input_cache = input;
      output_cache = output;
    }
}

static void NOINLINE
split_channels(stpi_channel_group_t *cg, unsigned *zero_mask)
{
  int i, j, k;
  int nz[STP_CHANNEL_LIMIT];
  int outbytes;
  const unsigned short *input_cache = NULL;
  const unsigned short *output_cache = NULL;
  const unsigned short *input;
  unsigned short *output;
  if (!cg)
    return;
  cg->valid_8bit = 0;
  outbytes = cg->total_channels * sizeof(unsigned short);
  input = cg->split_input;
  output = cg->output_data;
  for (i = 0; i < cg->total_channels; i++)
    nz[i] = 0;
  for (i = 0; i < cg->width; i++)
    {
      int zero_ptr = 0;
      if (input_cache && short_eq(input_cache, input, cg->aux_output_channels))
	{
	  memcpy(output, output_cache, outbytes);
	  input += cg->aux_output_channels;
	  output += cg->total_channels;
	}
      else
	{
	  unsigned black_value = 0;
	  unsigned virtual_black = 65535;
	  input_cache = input;
	  output_cache = output;
	  if (cg->black_channel >= 0)
	    black_value = input[cg->black_channel];
	  for (j = 0; j < cg->aux_output_channels; j++)
	    {
	      if (input[j] < virtual_black && j != cg->black_channel)
		virtual_black = input[j];
	    }
	  black_value += virtual_black / 4;
	  for (j = 0; j < cg->channel_count; j++)
	    {
	      stpi_channel_t *c = &(cg->c[j]);

	      if (cg->gloss_channel == j) // FIXME workaround.
		continue;

	      int s_count = c->subchannel_count;
	      if (s_count >= 1)
		{
		  unsigned i_val = *input++;
		  if (i_val == 0)
		    {
		      for (k = 0; k < s_count; k++)
			*(output++) = 0;
		    }
		  else if (s_count == 1)
		    {
		      if (c->sc[0].s_density < 65535)
			i_val = i_val * c->sc[0].s_density / 65535;
		      nz[zero_ptr++] |= *(output++) = i_val;
		    }
		  else
		    {
		      unsigned l_val = i_val;
		      unsigned offset;
		      if (i_val > 0 && black_value && j != cg->black_channel)
			{
			  l_val += black_value;
			  if (l_val > 65535)
			    l_val = 65535;
			}
		      offset = l_val * s_count;
		      for (k = 0; k < s_count; k++)
			{
			  unsigned o_val;
			  if (c->sc[k].s_density > 0)
			    {
			      o_val = c->lut[offset + k];
			      if (i_val != l_val)
				o_val = o_val * i_val / l_val;
			      if (c->sc[k].s_density < 65535)
				o_val = o_val * c->sc[k].s_density / 65535;
			    }
			  else
			    o_val = 0;
			  *output++ = o_val;
			  nz[zero_ptr++] |= o_val;
			}
		    }
		}
	    }
	}
    }
  if (zero_mask)
    {
      *zero_mask = 0;
      for (i = 0; i < cg->total_channels; i++)
	if (!nz[i])
	  *zero_mask |= 1 << i;
    }
}

static void NOINLINE
scale_channels(stpi_channel_group_t *cg, unsigned *zero_mask,
	       int zero_mask_valid)
{
  int i, j;
  int physical_channel = 0;
  if (!cg)
    return;
  cg->valid_8bit = 0;
  if (zero_mask)
    *zero_mask = 0;
  for (i = 0; i < cg->channel_count; i++)
    {
      stpi_channel_t *ch = &(cg->c[i]);
      if (ch->subchannel_count > 0)
	for (j = 0; j < ch->subchannel_count; j++)
	  {
	    if (cg->gloss_channel != i)
	      {
		stpi_subchannel_t *sch = &(ch->sc[j]);
		unsigned density = sch->s_density;
		unsigned short *output = cg->output_data + physical_channel;
		if (density == 0)
		  {
		    clear_channel(output, cg->width, cg->total_channels);
		    if (zero_mask)
		      *zero_mask |= 1 << physical_channel;
		  }
		else if (density != 65535)
		  {
		    if (scale_channel(output, cg->width, cg->total_channels,
				      density) == 0)
		      if (zero_mask)
			*zero_mask |= 1 << physical_channel;
		  }
		else if (zero_mask && ! zero_mask_valid)
		  {
		    if (scan_channel(output, cg->width, cg->total_channels)==0)
		      *zero_mask |= 1 << physical_channel;
		  }
	      }
	    physical_channel++;
	  }
    }
}

static void NOINLINE
generate_gloss(stpi_channel_group_t *cg, unsigned *zero_mask)
{
  unsigned short *output;
  unsigned gloss_mask;
  int i, j, k;
  if (!cg || cg->gloss_channel == -1 || cg->gloss_limit <= 0)
    return;
  cg->valid_8bit = 0;
  output = cg->output_data;
  gloss_mask = ~(1 << cg->gloss_physical_channel);
  for (i = 0; i < cg->width; i++)
    {
      int physical_channel = 0;
      unsigned channel_sum = 0;
      output[cg->gloss_physical_channel] = 0;
      for (j = 0; j < cg->channel_count; j++)
	{
	  stpi_channel_t *ch = &(cg->c[j]);
	  for (k = 0; k < ch->subchannel_count; k++)
	    {
	      if (cg->gloss_channel != j)
		{
		  channel_sum += (unsigned) output[physical_channel];
		  if (channel_sum >= cg->gloss_limit)
		    goto next;
		}
	      physical_channel++;
	    }
	}
      if (channel_sum < cg->gloss_limit)
	{
	  unsigned gloss_required = cg->gloss_limit - channel_sum;
	  // FIXME IJM apply gloss LuT?
	  if (gloss_required > 65535)
	    gloss_required = 65535;
	  output[cg->gloss_physical_channel] = gloss_required;
	  if (zero_mask)
	    *zero_mask &= gloss_mask;
	}
    next:
      output += cg->total_channels;
    }
}

static void NOINLINE
do_gcr(stpi_channel_group_t *cg, unsigned *zero_mask)
{
  const unsigned short *gcr_lookup;
  unsigned short *output;
  size_t count;
  int i;
  union {
    unsigned short nz[4];
    unsigned long long nzl;
  } nzx;

  nzx.nzl = 0ull;
  if (!cg)
    return;
  cg->valid_8bit = 0;

  output = cg->gcr_data;
  stp_curve_resample(cg->gcr_curve, 65536);
  gcr_lookup = stp_curve_get_ushort_data(cg->gcr_curve, &count);
  for (i = 0; i < cg->width; i++)
    {
      unsigned k = output[0];
      if (k > 0)
	{
	  int kk = gcr_lookup[k];
	  int ck;
	  if (kk > k)
	    kk = k;
	  ck = k - kk;
	  output[0] = kk;
	  output[1] += ck * cg->cyan_balance;
	  output[2] += ck * cg->magenta_balance;
	  output[3] += ck * cg->yellow_balance;
	  nzx.nzl |= *(unsigned long long *) output;
	}
      output += cg->gcr_channels;
    }
  *zero_mask = 0;
  for (i = 0; i < 4; i++)
    if (nzx.nz[i] == 0)
      *zero_mask |= (1 << i);
}

void
stp_channel_convert(const stp_vars_t *v, unsigned *zero_mask)
{
  int zero_mask_valid = 1;
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stp_get_component_data(v, "Channel"));
  if (input_has_special_channels(cg))
    {
      generate_special_channels(cg);
      zero_mask_valid = 0;
    }
  else if (output_has_gloss(cg) && !input_needs_splitting(cg))
    {
      copy_channels(cg);
      zero_mask_valid = 0;
    }
  if (output_needs_gcr(cg))
    do_gcr(cg, zero_mask);
  if (input_needs_splitting(cg))
    split_channels(cg, zero_mask);
  else
    scale_channels(cg, zero_mask, zero_mask_valid);
  (void) limit_ink(cg);
  (void) generate_gloss(cg, zero_mask);
}

unsigned short *
stp_channel_get_input(const stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return NULL;
  return (unsigned short *) cg->input_data;
}

unsigned short *
stp_channel_get_output(const stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return NULL;
  return cg->output_data;
}

unsigned char *
stp_channel_get_output_8bit(const stp_vars_t *v)
{
  stpi_channel_group_t *cg = get_channel_group(v);
  if (!cg)
    return NULL;
  if (cg->valid_8bit)
    return cg->output_data_8bit;
  if (! cg->output_data_8bit)
    cg->output_data_8bit = stp_malloc(sizeof(unsigned char) *
				      cg->total_channels * cg->width);
  int i;
  (void) memset(cg->output_data_8bit, 0, sizeof(unsigned char) *
		cg->total_channels * cg->width);
  for (i = 0; i < cg->width * cg->total_channels; i++)
    cg->output_data_8bit[i] = cg->output_data[i] / (unsigned short) 257;
  cg->valid_8bit = 1;
  return cg->output_data_8bit;
}
