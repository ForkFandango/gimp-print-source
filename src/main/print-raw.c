/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <string.h>
#include <stdio.h>

#ifdef __GNUC__
#define inline __inline__
#endif

typedef struct
{
  int color_model;
  int output_channels;
  const char *name;
} ink_t;

typedef struct raw_printer
{
  int output_bits;
} raw_printer_t;

static const raw_printer_t raw_model_capabilities[] =
{
  {
    16
  },
  {
    8
  },
};

static const ink_t inks[] =
{
  { COLOR_MODEL_RGB, 3, "RGB" },
  { COLOR_MODEL_CMY, 3, "CMY" },
  { COLOR_MODEL_RGB, 1, "RGBGray" },
  { COLOR_MODEL_CMY, 1, "CMYGray" },
};

static const int ink_count = sizeof(inks) / sizeof(ink_t);

static void
raw_parameters(const stp_vars_t v, const char *name,
	       stp_parameter_t *description)
{
  int		i;
  description->type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  stp_fill_parameter_settings(description, name);
  description->deflt.str = NULL;
  if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      description->bounds.str = stp_string_list_allocate();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (stp_papersize_get_width(pt) == 0 &&
	      stp_papersize_get_height(pt) == 0)
	    {
	      stp_string_list_add_param(description->bounds.str,
					stp_papersize_get_name(pt),
					stp_papersize_get_text(pt));
	      break;
	    }
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      description->bounds.str = stp_string_list_allocate();
      for (i = 0; i < ink_count; i++)
	stp_string_list_add_param(description->bounds.str,
				  inks[i].name, inks[i].name);
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }      
  else if ((strcmp(name, "Resolution") == 0) ||
	   (strcmp(name, "MediaType") == 0) ||
	   (strcmp(name, "InputSlot") == 0))
    {
      description->bounds.str = stp_string_list_allocate();
      stp_string_list_add_param(description->bounds.str,
				"Standard", "Standard");
    }
  else
    stp_describe_internal_parameter(v, name, description);
}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
raw_imageable_area(const stp_vars_t v,
		   int  *left,
		   int  *right,
		   int  *bottom,
		   int  *top)
{
  *left = 0;
  *top = 0;
  *right = stp_get_page_width(v);
  *bottom = stp_get_page_height(v);
}

static void
raw_limit(const stp_vars_t v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width = 65535;
  *height = 65535;
  *min_width = 1;
  *min_height =	1;
}

static void
raw_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  *x = 72;
  *y = 72;
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static int
raw_print(const stp_vars_t v, stp_image_t *image)
{
  int		model = stp_get_model(v);
  int width = stp_get_page_width(v);
  int height = stp_get_page_height(v);
  int		i, j;
  int		y;		/* Looping vars */
  stp_convert_t	colorfunc;	/* Color conversion function... */
  stp_vars_t	nv = stp_allocate_copy(v);
  int out_channels;
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned short *final_out = NULL;
  unsigned char	*in;		/* Input pixels */
  int		status = 1;
  int bytes_per_channel = raw_model_capabilities[model].output_bits / 8;
  int ink_channels = 1;

  if (!stp_verify(nv))
    {
      stp_eprintf(nv, _("Print options not verified; cannot print.\n"));
      stp_vars_free(nv);
      return 0;
    }
  if (width != image->width(image) || height != image->height(image))
    {
      stp_eprintf(nv, _("Image dimensions must match paper dimensions"));
      stp_vars_free(nv);
      return 0;
    }
  for (i = 0; i < ink_count; i++)
    if (strcmp(stp_get_string_parameter(nv, "InkType"), inks[i].name) == 0)
      {
	stp_set_output_color_model(nv, inks[i].color_model);
	ink_channels = inks[i].output_channels;
	break;
      }
  colorfunc = stp_choose_colorfunc(nv, image->bpp(image), &out_channels);
  if (out_channels != ink_channels && out_channels != 1 && ink_channels != 1)
    {
      stp_eprintf(nv, _("Internal error!  Output channels or input channels must be 1\n"));
      stp_vars_free(nv);
      return 0;
    }

  in  = stp_malloc(width * image->bpp(image));
  out = stp_malloc(width * out_channels * 2);
  if (out_channels != ink_channels)
    final_out = stp_malloc(width * ink_channels * 2);

  stp_set_float_parameter(nv, "Density", 1.0);
  stp_compute_lut(nv, 256, NULL, NULL, NULL);

  image->progress_init(image);

  for (y = 0; y < height; y++)
    {
      unsigned short *real_out = out;
      int zero_mask;
      if ((y & 63) == 0)
	image->note_progress(image, y, height);
      if (image->get_row(image, in, y) != STP_IMAGE_OK)
	{
	  status = 2;
	  break;
	}
      (*colorfunc)(nv, in, out, &zero_mask, width, image->bpp(image));
      if (out_channels != ink_channels)
	{
	  real_out = final_out;
	  if (out_channels < ink_channels)
	    {
	      for (i = 0; i < width; i++)
		{
		  for (j = 0; j < ink_channels; j++)
		    final_out[i * ink_channels + j] = out[i];
		}
	    }
	  else
	    {
	      for (i = 0; i < width; i++)
		{
		  int avg = 0;
		  for (j = 0; j < out_channels; j++)
		    avg += out[i * out_channels + j];
		  final_out[i] = avg / out_channels;
		}
	    }
	}
      if (bytes_per_channel == 1)
	{
	  unsigned char *char_out = (unsigned char *) real_out;
	  for (i = 0; i < width * ink_channels; i++)
	    char_out[i] = real_out[i] / 257;
	}
      stp_zfwrite((char *) real_out,
		  width * ink_channels * bytes_per_channel, 1, nv);
    }
  image->progress_conclude(image);
  if (final_out)
    stp_free(final_out);
  stp_free(out);
  stp_free(in);
  stp_vars_free(nv);
  return status;
}

const stp_printfuncs_t stp_raw_printfuncs =
{
  raw_parameters,
  stp_default_media_size,
  raw_imageable_area,
  raw_limit,
  raw_print,
  raw_describe_resolution,
  stp_verify_printer_params,
  NULL,
  NULL
};
