/*
 * "$Id$"
 *
 *   Gimp-Print color management module - traditional Gimp-Print algorithm.
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
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include "module.h"
#include "xml.h"
#include "curve-cache.h"
#include "color-conversion.h"

#ifdef __GNUC__
#define inline __inline__
#endif

static const color_correction_t color_corrections[] =
{
  { "None",        N_("Default"),       COLOR_CORRECTION_DEFAULT,     1 },
  { "Accurate",    N_("High Accuracy"), COLOR_CORRECTION_ACCURATE,    1 },
  { "Bright",      N_("Bright Colors"), COLOR_CORRECTION_BRIGHT,      1 },
  { "Uncorrected", N_("Uncorrected"),   COLOR_CORRECTION_UNCORRECTED, 0 },
  { "Desaturated", N_("Desaturated"),   COLOR_CORRECTION_DESATURATED, 0 },
  { "Threshold",   N_("Threshold"),     COLOR_CORRECTION_THRESHOLD,   0 },
  { "Density",     N_("Density"),       COLOR_CORRECTION_DENSITY,     0 },
  { "Raw",         N_("Raw"),           COLOR_CORRECTION_RAW,         0 },
};

static const int color_correction_count =
sizeof(color_corrections) / sizeof(color_correction_t);


static const channel_param_t channel_params[] =
{
  { CMASK_K, "BlackGamma",   "BlackCurve",   "WhiteGamma",   "WhiteCurve"   },
  { CMASK_C, "CyanGamma",    "CyanCurve",    "RedGamma",     "RedCurve"     },
  { CMASK_M, "MagentaGamma", "MagentaCurve", "GreenGamma",   "GreenCurve"   },
  { CMASK_Y, "YellowGamma",  "YellowCurve",  "BlueGamma",    "BlueCurve"    },
  { CMASK_W, "WhiteGamma",   "WhiteCurve",   "BlackGamma",   "BlackCurve"   },
  { CMASK_R, "RedGamma",     "RedCurve",     "CyanGamma",    "CyanCurve"    },
  { CMASK_G, "GreenGamma",   "GreenCurve",   "MagentaGamma", "MagentaCurve" },
  { CMASK_B, "BlueGamma",    "BlueCurve",    "YellowGamma",  "YellowCurve"  },
};

static const int channel_param_count =
sizeof(channel_params) / sizeof(channel_param_t);

static const channel_param_t raw_channel_params[] =
{
  { 0,  "GammaCh0",  "CurveCh0",  "GammaCh0",  "CurveCh0"  },
  { 1,  "GammaCh1",  "CurveCh1",  "GammaCh1",  "CurveCh1"  },
  { 2,  "GammaCh2",  "CurveCh2",  "GammaCh2",  "CurveCh2"  },
  { 3,  "GammaCh3",  "CurveCh3",  "GammaCh3",  "CurveCh3"  },
  { 4,  "GammaCh4",  "CurveCh4",  "GammaCh4",  "CurveCh4"  },
  { 5,  "GammaCh5",  "CurveCh5",  "GammaCh5",  "CurveCh5"  },
  { 6,  "GammaCh6",  "CurveCh6",  "GammaCh6",  "CurveCh6"  },
  { 7,  "GammaCh7",  "CurveCh7",  "GammaCh7",  "CurveCh7"  },
  { 8,  "GammaCh8",  "CurveCh8",  "GammaCh8",  "CurveCh8"  },
  { 9,  "GammaCh9",  "CurveCh9",  "GammaCh9",  "CurveCh9"  },
  { 10, "GammaCh10", "CurveCh10", "GammaCh10", "CurveCh10" },
  { 11, "GammaCh11", "CurveCh11", "GammaCh11", "CurveCh11" },
  { 12, "GammaCh12", "CurveCh12", "GammaCh12", "CurveCh12" },
  { 13, "GammaCh13", "CurveCh13", "GammaCh13", "CurveCh13" },
  { 14, "GammaCh14", "CurveCh14", "GammaCh14", "CurveCh14" },
  { 15, "GammaCh15", "CurveCh15", "GammaCh15", "CurveCh15" },
  { 16, "GammaCh16", "CurveCh16", "GammaCh16", "CurveCh16" },
  { 17, "GammaCh17", "CurveCh17", "GammaCh17", "CurveCh17" },
  { 18, "GammaCh18", "CurveCh18", "GammaCh18", "CurveCh18" },
  { 19, "GammaCh19", "CurveCh19", "GammaCh19", "CurveCh19" },
  { 20, "GammaCh20", "CurveCh20", "GammaCh20", "CurveCh20" },
  { 21, "GammaCh21", "CurveCh21", "GammaCh21", "CurveCh21" },
  { 22, "GammaCh22", "CurveCh22", "GammaCh22", "CurveCh22" },
  { 23, "GammaCh23", "CurveCh23", "GammaCh23", "CurveCh23" },
  { 24, "GammaCh24", "CurveCh24", "GammaCh24", "CurveCh24" },
  { 25, "GammaCh25", "CurveCh25", "GammaCh25", "CurveCh25" },
  { 26, "GammaCh26", "CurveCh26", "GammaCh26", "CurveCh26" },
  { 27, "GammaCh27", "CurveCh27", "GammaCh27", "CurveCh27" },
  { 28, "GammaCh28", "CurveCh28", "GammaCh28", "CurveCh28" },
  { 29, "GammaCh29", "CurveCh29", "GammaCh29", "CurveCh29" },
  { 30, "GammaCh30", "CurveCh30", "GammaCh30", "CurveCh30" },
  { 31, "GammaCh31", "CurveCh31", "GammaCh31", "CurveCh31" },
};

static const int raw_channel_param_count =
sizeof(raw_channel_params) / sizeof(channel_param_t);


static const color_description_t color_descriptions[] =
{
  { "Grayscale",  1, 1, COLOR_ID_GRAY,   COLOR_BLACK,   CMASK_K,      1,
    COLOR_CORRECTION_UNCORRECTED, &stpi_color_convert_to_gray   },
  { "Whitescale", 1, 1, COLOR_ID_WHITE,  COLOR_WHITE,   CMASK_K,      1,
    COLOR_CORRECTION_UNCORRECTED, &stpi_color_convert_to_gray   },
  { "RGB",        1, 1, COLOR_ID_RGB,    COLOR_WHITE,   CMASK_CMY,    3,
    COLOR_CORRECTION_ACCURATE,    &stpi_color_convert_to_color  },
  { "CMY",        1, 1, COLOR_ID_CMY,    COLOR_BLACK,   CMASK_CMY,    3,
    COLOR_CORRECTION_ACCURATE,    &stpi_color_convert_to_color  },
  { "CMYK",       1, 0, COLOR_ID_CMYK,   COLOR_BLACK,   CMASK_CMYK,   4,
    COLOR_CORRECTION_ACCURATE,    &stpi_color_convert_to_kcmy   },
  { "KCMY",       1, 1, COLOR_ID_KCMY,   COLOR_BLACK,   CMASK_CMYK,   4,
    COLOR_CORRECTION_ACCURATE,    &stpi_color_convert_to_kcmy   },
  { "CMYKRB",     0, 1, COLOR_ID_CMYKRB, COLOR_BLACK,   CMASK_CMYKRB, 6,
    COLOR_CORRECTION_ACCURATE,    &stpi_color_convert_to_cmykrb },
  { "Raw",        1, 1, COLOR_ID_RAW,    COLOR_UNKNOWN, 0,           -1,
    COLOR_CORRECTION_RAW,         &stpi_color_convert_raw       },
};

static const int color_description_count =
sizeof(color_descriptions) / sizeof(color_description_t);


static const channel_depth_t channel_depths[] =
{
  { "8",  8  },
  { "16", 16 }
};

static const int channel_depth_count =
sizeof(channel_depths) / sizeof(channel_depth_t);


typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  unsigned channel_mask;
  int color_only;
} float_param_t;

#define RAW_GAMMA_CHANNEL(channel)					 \
  {									 \
    {									 \
      "GammaCh" #channel, N_("Channel " #channel " Gamma"), N_("Gamma"), \
      N_("Gamma for raw channel " #channel),				 \
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,		 \
      STP_PARAMETER_LEVEL_INTERNAL, 0, 1, channel, 1, 0			 \
    }, 0.1, 4.0, 1.0, CMASK_RAW, 0					 \
  }

static const float_param_t float_parameters[] =
{
  {
    {
      "ColorCorrection", N_("Color Correction"), N_("Basic Image Adjustment"),
      N_("Color correction to be applied"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 1, 1, -1, 1, 0
    }, 0.0, 0.0, 0.0, CMASK_EVERY, 0
  },
  {
    {
      "ChannelBitDepth", N_("Channel Bit Depth"), N_("Core Parameter"),
      N_("Bit depth per channel"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
    }, 0.0, 0.0, 0.0, CMASK_EVERY, 0
  },
  {
    {
      "InputImageType", N_("Input Image Type"), N_("Core Parameter"),
      N_("Input image type"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
    }, 0.0, 0.0, 0.0, CMASK_EVERY, 0
  },
  {
    {
      "STPIOutputType", N_("Output Image Type"), N_("Core Parameter"),
      N_("Output image type"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
      STP_PARAMETER_LEVEL_INTERNAL, 1, 1, -1, 1, 0
    }, 0.0, 0.0, 0.0, CMASK_EVERY, 0
  },
  {
    {
      "STPIRawChannels", N_("Raw Channels"), N_("Core Parameter"),
      N_("Raw Channels"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_CORE,
      STP_PARAMETER_LEVEL_INTERNAL, 1, 1, -1, 1, 0
    }, 1.0, STP_CHANNEL_LIMIT, 1.0, CMASK_EVERY, 0
  },
  {
    {
      "Brightness", N_("Brightness"), N_("Basic Image Adjustment"),
      N_("Brightness of the print (0 is solid black, 2 is solid white)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
    }, 0.0, 2.0, 1.0, CMASK_ALL, 0
  },
  {
    {
      "Contrast", N_("Contrast"), N_("Basic Image Adjustment"),
      N_("Contrast of the print (0 is solid gray)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_ALL, 0
  },
  {
    {
      "LinearContrast", N_("Linear Contrast Adjustment"), N_("Advanced Image Control"),
      N_("Use linear vs. fixed end point contrast adjustment"),
      STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 1, 1, -1, 1, 0
    }, 0.0, 0.0, 0.0, CMASK_ALL, 0
  },
  {
    {
      "Gamma", N_("Gamma"), N_("Gamma"),
      N_("Adjust the gamma of the print. Larger values will "
	 "produce a generally brighter print, while smaller "
	 "values will produce a generally darker print. "
	 "Black and white will remain the same, unlike with "
	 "the brightness adjustment."),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
    }, 0.1, 4.0, 1.0, CMASK_EVERY, 0
  },
  {
    {
      "AppGamma", N_("AppGamma"), N_("Gamma"),
      N_("Gamma value assumed by application"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_INTERNAL, 0, 1, -1, 1, 0
    }, 0.1, 4.0, 1.0, CMASK_EVERY, 0
  },
  {
    {
      "CyanGamma", N_("Cyan"), N_("Gamma"),
      N_("Adjust the cyan gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 1, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_C, 1
  },
  {
    {
      "MagentaGamma", N_("Magenta"), N_("Gamma"),
      N_("Adjust the magenta gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 2, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_M, 1
  },
  {
    {
      "YellowGamma", N_("Yellow"), N_("Gamma"),
      N_("Adjust the yellow gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 3, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_Y, 1
  },
  {
    {
      "RedGamma", N_("Red"), N_("Gamma"),
      N_("Adjust the red gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 1, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_R, 1
  },
  {
    {
      "GreenGamma", N_("Green"), N_("Gamma"),
      N_("Adjust the green gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 2, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_G, 1
  },
  {
    {
      "BlueGamma", N_("Blue"), N_("Gamma"),
      N_("Adjust the blue gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 3, 1, 0
    }, 0.0, 4.0, 1.0, CMASK_B, 1
  },
  {
    {
      "Saturation", N_("Saturation"), N_("Basic Image Adjustment"),
      N_("Adjust the saturation (color balance) of the print\n"
	 "Use zero saturation to produce grayscale output "
	 "using color and black inks"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
    }, 0.0, 9.0, 1.0, CMASK_CMY | CMASK_RGB, 1
  },
  /* Need to think this through a bit more -- rlk 20030712 */
  {
    {
      "InkLimit", N_("Ink Limit"), N_("Advanced Output Control"),
      N_("Limit the total ink printed to the page"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 0, 0
    }, 0.0, STP_CHANNEL_LIMIT, STP_CHANNEL_LIMIT, CMASK_CMY, 0
  },
  {
    {
      "BlackGamma", N_("GCR Transition"), N_("Advanced Output Control"),
      N_("Adjust the gray component transition rate"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1, 0
    }, 0.0, 1.0, 1.0, CMASK_K, 1
  },
  {
    {
      "GCRLower", N_("GCR Lower Bound"), N_("Advanced Output Control"),
      N_("Lower bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1, 0
    }, 0.0, 1.0, 0.2, CMASK_K, 1
  },
  {
    {
      "GCRUpper", N_("GCR Upper Bound"), N_("Advanced Output Control"),
      N_("Upper bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1, 0
    }, 0.0, 5.0, 0.5, CMASK_K, 1
  },
  RAW_GAMMA_CHANNEL(0),
  RAW_GAMMA_CHANNEL(1),
  RAW_GAMMA_CHANNEL(2),
  RAW_GAMMA_CHANNEL(3),
  RAW_GAMMA_CHANNEL(4),
  RAW_GAMMA_CHANNEL(5),
  RAW_GAMMA_CHANNEL(6),
  RAW_GAMMA_CHANNEL(7),
  RAW_GAMMA_CHANNEL(8),
  RAW_GAMMA_CHANNEL(9),
  RAW_GAMMA_CHANNEL(10),
  RAW_GAMMA_CHANNEL(11),
  RAW_GAMMA_CHANNEL(12),
  RAW_GAMMA_CHANNEL(13),
  RAW_GAMMA_CHANNEL(14),
  RAW_GAMMA_CHANNEL(15),
  RAW_GAMMA_CHANNEL(16),
  RAW_GAMMA_CHANNEL(17),
  RAW_GAMMA_CHANNEL(18),
  RAW_GAMMA_CHANNEL(19),
  RAW_GAMMA_CHANNEL(20),
  RAW_GAMMA_CHANNEL(21),
  RAW_GAMMA_CHANNEL(22),
  RAW_GAMMA_CHANNEL(23),
  RAW_GAMMA_CHANNEL(24),
  RAW_GAMMA_CHANNEL(25),
  RAW_GAMMA_CHANNEL(26),
  RAW_GAMMA_CHANNEL(27),
  RAW_GAMMA_CHANNEL(28),
  RAW_GAMMA_CHANNEL(29),
  RAW_GAMMA_CHANNEL(30),
  RAW_GAMMA_CHANNEL(31),
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(float_param_t);

typedef struct
{
  stp_parameter_t param;
  stp_curve_t *defval;
  unsigned channel_mask;
  int hsl_only;
  int color_only;
} curve_param_t;

static int standard_curves_initialized = 0;

static stp_curve_t hue_map_bounds = NULL;
static stp_curve_t lum_map_bounds = NULL;
static stp_curve_t sat_map_bounds = NULL;
static stp_curve_t color_curve_bounds = NULL;
static stp_curve_t gcr_curve_bounds = NULL;


#define RAW_CURVE_CHANNEL(channel)					\
  {									\
    {									\
      "CurveCh" #channel, N_("Channel " #channel " Curve"),		\
      N_("Output Curves"), N_("Curve for raw channel " #channel),	\
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,		\
      STP_PARAMETER_LEVEL_INTERNAL, 0, 1, channel, 1, 0			\
    }, &color_curve_bounds, CMASK_RAW, 0, 0				\
  }

static curve_param_t curve_parameters[] =
{
  {
    {
      "CyanCurve", N_("Cyan Curve"), N_("Output Curves"),
      N_("Cyan curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1, 1, 0
    }, &color_curve_bounds, CMASK_C, 0, 1
  },
  {
    {
      "MagentaCurve", N_("Magenta Curve"), N_("Output Curves"),
      N_("Magenta curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 2, 1, 0
    }, &color_curve_bounds, CMASK_M, 0, 1
  },
  {
    {
      "YellowCurve", N_("Yellow Curve"), N_("Output Curves"),
      N_("Yellow curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 3, 1, 0
    }, &color_curve_bounds, CMASK_Y, 0, 1
  },
  {
    {
      "BlackCurve", N_("Black Curve"), N_("Output Curves"),
      N_("Black curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 0, 1, 0
    }, &color_curve_bounds, CMASK_K, 0, 0
  },
  {
    {
      "RedCurve", N_("Red Curve"), N_("Output Curves"),
      N_("Red curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1, 1, 0
    }, &color_curve_bounds, CMASK_R, 0, 1
  },
  {
    {
      "GreenCurve", N_("Green Curve"), N_("Output Curves"),
      N_("Green curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1, 1, 0
    }, &color_curve_bounds, CMASK_G, 0, 1
  },
  {
    {
      "BlueCurve", N_("Blue Curve"), N_("Output Curves"),
      N_("Blue curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1, 1, 0
    }, &color_curve_bounds, CMASK_B, 0, 1
  },
  {
    {
      "WhiteCurve", N_("White Curve"), N_("Output Curves"),
      N_("White curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1, 1, 0
    }, &color_curve_bounds, CMASK_W, 0, 0
  },
  {
    {
      "HueMap", N_("Hue Map"), N_("Advanced HSL Curves"),
      N_("Hue adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1, 1, 0
    }, &hue_map_bounds, CMASK_CMY | CMASK_RGB, 1, 1
  },
  {
    {
      "SatMap", N_("Saturation Map"), N_("Advanced HSL Curves"),
      N_("Saturation adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1, 1, 0
    }, &sat_map_bounds, CMASK_CMY | CMASK_RGB, 1, 1
  },
  {
    {
      "LumMap", N_("Luminosity Map"), N_("Advanced HSL Curves"),
      N_("Luminosity adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1, 1, 0
    }, &lum_map_bounds, CMASK_CMY | CMASK_RGB, 1, 1
  },
  {
    {
      "GCRCurve", N_("Gray Component Reduction"), N_("Advanced Output Control"),
      N_("Gray component reduction curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, 0, 1, 0
    }, &gcr_curve_bounds, CMASK_K, 0, 1
  },
  RAW_CURVE_CHANNEL(0),
  RAW_CURVE_CHANNEL(1),
  RAW_CURVE_CHANNEL(2),
  RAW_CURVE_CHANNEL(3),
  RAW_CURVE_CHANNEL(4),
  RAW_CURVE_CHANNEL(5),
  RAW_CURVE_CHANNEL(6),
  RAW_CURVE_CHANNEL(7),
  RAW_CURVE_CHANNEL(8),
  RAW_CURVE_CHANNEL(9),
  RAW_CURVE_CHANNEL(10),
  RAW_CURVE_CHANNEL(11),
  RAW_CURVE_CHANNEL(12),
  RAW_CURVE_CHANNEL(13),
  RAW_CURVE_CHANNEL(14),
  RAW_CURVE_CHANNEL(15),
  RAW_CURVE_CHANNEL(16),
  RAW_CURVE_CHANNEL(17),
  RAW_CURVE_CHANNEL(18),
  RAW_CURVE_CHANNEL(19),
  RAW_CURVE_CHANNEL(20),
  RAW_CURVE_CHANNEL(21),
  RAW_CURVE_CHANNEL(22),
  RAW_CURVE_CHANNEL(23),
  RAW_CURVE_CHANNEL(24),
  RAW_CURVE_CHANNEL(25),
  RAW_CURVE_CHANNEL(26),
  RAW_CURVE_CHANNEL(27),
  RAW_CURVE_CHANNEL(28),
  RAW_CURVE_CHANNEL(29),
  RAW_CURVE_CHANNEL(30),
  RAW_CURVE_CHANNEL(31),
};

static const int curve_parameter_count =
sizeof(curve_parameters) / sizeof(curve_param_t);


static const color_description_t *
get_color_description(const char *name)
{
  int i;
  if (name)
    for (i = 0; i < color_description_count; i++)
      {
	if (strcmp(name, color_descriptions[i].name) == 0)
	  return &(color_descriptions[i]);
      }
  return NULL;
}

static const channel_depth_t *
get_channel_depth(const char *name)
{
  int i;
  if (name)
    for (i = 0; i < channel_depth_count; i++)
      {
	if (strcmp(name, channel_depths[i].name) == 0)
	  return &(channel_depths[i]);
      }
  return NULL;
}

static const color_correction_t *
get_color_correction(const char *name)
{
  int i;
  if (name)
    for (i = 0; i < color_correction_count; i++)
      {
	if (strcmp(name, color_corrections[i].name) == 0)
	  return &(color_corrections[i]);
      }
  return NULL;
}

static const color_correction_t *
get_color_correction_by_tag(color_correction_enum_t correction)
{
  int i;
  for (i = 0; i < color_correction_count; i++)
    {
      if (correction == color_corrections[i].correction)
	return &(color_corrections[i]);
    }
  return NULL;
}


static void
initialize_channels(stp_vars_t v, stp_image_t *image)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(v, "Color"));
  if (stp_check_float_parameter(v, "InkLimit", STP_PARAMETER_ACTIVE))
    stpi_channel_set_ink_limit(v, stp_get_float_parameter(v, "InkLimit"));
  stpi_channel_initialize(v, image, lut->out_channels);
  lut->channels_are_initialized = 1;
}

static int
stpi_color_traditional_get_row(stp_vars_t v,
			       stp_image_t *image,
			       int row,
			       unsigned *zero_mask)
{
  const lut_t *lut = (const lut_t *)(stpi_get_component_data(v, "Color"));
  unsigned zero;
  if (stpi_image_get_row(image, lut->in_data,
			 lut->image_width * lut->in_channels, row)
      != STP_IMAGE_STATUS_OK)
    return 2;
  if (!lut->channels_are_initialized)
    initialize_channels(v, image);
  zero = (lut->output_color_description->conversion_function)
    (v, lut->in_data, stpi_channel_get_input(v));
  if (zero_mask)
    *zero_mask = zero;
  stpi_channel_convert(v, zero_mask);
  return 0;
}

static void
free_channels(lut_t *lut)
{
  int i;
  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    stpi_curve_free_curve_cache(&(lut->channel_curves[i]));
}

static lut_t *
allocate_lut(void)
{
  int i;
  lut_t *ret = stpi_zalloc(sizeof(lut_t));
  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    {
      ret->gamma_values[i] = 1.0;
    }
  ret->print_gamma = 1.0;
  ret->app_gamma = 1.0;
  ret->contrast = 1.0;
  ret->brightness = 1.0;
  return ret;
}

static void *
copy_lut(void *vlut)
{
  const lut_t *src = (const lut_t *)vlut;
  int i;
  lut_t *dest;
  if (!src)
    return NULL;
  dest = allocate_lut();
  free_channels(dest);

  dest->steps = src->steps;
  dest->channel_depth = src->channel_depth;
  dest->image_width = src->image_width;
  dest->in_channels = src->in_channels;
  dest->out_channels = src->out_channels;
  /* Don't copy channels_are_initialized */
  dest->invert_output = src->invert_output;
  dest->input_color_description = src->input_color_description;
  dest->output_color_description = src->output_color_description;
  dest->color_correction = src->color_correction;
  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    {
      stpi_curve_cache_copy(&(dest->channel_curves[i]), &(src->channel_curves[i]));
      dest->gamma_values[i] = src->gamma_values[i];
    }
  dest->print_gamma = src->print_gamma;
  dest->app_gamma = src->app_gamma;
  dest->screen_gamma = src->screen_gamma;
  dest->contrast = src->contrast;
  dest->brightness = src->brightness;
  dest->linear_contrast_adjustment = src->linear_contrast_adjustment;
  stpi_curve_cache_copy(&(dest->hue_map), &(src->hue_map));
  stpi_curve_cache_copy(&(dest->lum_map), &(src->lum_map));
  stpi_curve_cache_copy(&(dest->sat_map), &(src->sat_map));
  stpi_curve_cache_copy(&(dest->gcr_curve), &(src->gcr_curve));
  /* Don't copy gray_tmp */
  /* Don't copy cmy_tmp */
  /* Don't copy cmyk_tmp */
  if (src->in_data)
    {
      dest->in_data = stpi_malloc(src->image_width * src->in_channels);
      memset(dest->in_data, 0, src->image_width * src->in_channels);
    }
  return dest;
}

static void
free_lut(void *vlut)
{
  lut_t *lut = (lut_t *)vlut;
  free_channels(lut);
  stpi_curve_free_curve_cache(&(lut->hue_map));
  stpi_curve_free_curve_cache(&(lut->lum_map));
  stpi_curve_free_curve_cache(&(lut->sat_map));
  stpi_curve_free_curve_cache(&(lut->gcr_curve));
  SAFE_FREE(lut->gray_tmp);
  SAFE_FREE(lut->cmy_tmp);
  SAFE_FREE(lut->cmyk_tmp);
  SAFE_FREE(lut->in_data);
  memset(lut, 0, sizeof(lut_t));
  stpi_free(lut);
}

static stp_curve_t
compute_gcr_curve(stp_const_vars_t vars)
{
  stp_curve_t curve;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  double k_lower = 0.0;
  double k_upper = 1.0;
  double k_gamma = 1.0;
  double i_k_gamma = 1.0;
  double *tmp_data = stpi_malloc(sizeof(double) * lut->steps);
  int i;

  if (stp_check_float_parameter(vars, "GCRUpper", STP_PARAMETER_DEFAULTED))
    k_upper = stp_get_float_parameter(vars, "GCRUpper");
  if (stp_check_float_parameter(vars, "GCRLower", STP_PARAMETER_DEFAULTED))
    k_lower = stp_get_float_parameter(vars, "GCRLower");
  if (stp_check_float_parameter(vars, "BlackGamma", STP_PARAMETER_DEFAULTED))
    k_gamma = stp_get_float_parameter(vars, "BlackGamma");
  k_upper *= lut->steps;
  k_lower *= lut->steps;

  if (k_lower > lut->steps)
    k_lower = lut->steps;
  if (k_upper < k_lower)
    k_upper = k_lower + 1;
  i_k_gamma = 1.0 / k_gamma;

  for (i = 0; i < k_lower; i ++)
    tmp_data[i] = 0;
  if (k_upper < lut->steps)
    {
      for (i = ceil(k_lower); i < k_upper; i ++)
	{
	  double where = (i - k_lower) / (k_upper - k_lower);
	  double g1 = pow(where, i_k_gamma);
	  double g2 = 1.0 - pow(1.0 - where, k_gamma);
	  double value = (g1 > g2 ? g1 : g2);
	  tmp_data[i] = 65535.0 * k_upper * value / (double) (lut->steps - 1);
	  tmp_data[i] = floor(tmp_data[i] + .5);
	}
      for (i = ceil(k_upper); i < lut->steps; i ++)
	tmp_data[i] = 65535.0 * i / (double) (lut->steps - 1);
    }
  else if (k_lower < lut->steps)
    for (i = ceil(k_lower); i < lut->steps; i ++)
      {
	double where = (i - k_lower) / (k_upper - k_lower);
	double g1 = pow(where, i_k_gamma);
	double g2 = 1.0 - pow(1.0 - where, k_gamma);
	double value = (g1 > g2 ? g1 : g2);
	tmp_data[i] = 65535.0 * lut->steps * value / (double) (lut->steps - 1);
	tmp_data[i] = floor(tmp_data[i] + .5);
      }
  curve = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_set_bounds(curve, 0, 65535);
  if (! stp_curve_set_data(curve, lut->steps, tmp_data))
    {
      stpi_eprintf(vars, "set curve data failed!\n");
      stpi_abort();
    }
  stpi_free(tmp_data);
  return curve;
}

static void
initialize_gcr_curve(stp_const_vars_t vars)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  if (!stpi_curve_cache_get_curve(&(lut->gcr_curve)))
    {
      if (stp_check_curve_parameter(vars, "GCRCurve", STP_PARAMETER_DEFAULTED))
	{
	  double data;
	  size_t count;
	  int i;
	  stp_curve_t curve =
	    stp_curve_create_copy(stp_get_curve_parameter(vars, "GCRCurve"));
	  stp_curve_resample(curve, lut->steps);
	  count = stp_curve_count_points(curve);
	  stp_curve_set_bounds(curve, 0.0, 65535.0);
	  for (i = 0; i < count; i++)
	    {
	      stp_curve_get_point(curve, i, &data);
	      data = 65535.0 * data * (double) i / (count - 1);
	      stp_curve_set_point(curve, i, data);
	    }
	  stpi_curve_cache_set_curve(&(lut->gcr_curve), curve);
	}
      else
	stpi_curve_cache_set_curve(&(lut->gcr_curve), compute_gcr_curve(vars));
    }
}

/*
 * Channels that are synthesized (e. g. the black channel in CMY -> CMYK
 * conversion) need to be computed differently from channels that are
 * mapped 1-1 from input to output.  Channels that are simply mapped need
 * to be inverted if necessary, and in addition the contrast, brightness,
 * and other gamma factors are factored in.  Channels that are synthesized
 * are never inverted, since they're computed from the output of other
 * channels that have already been inverted.
 *
 * This isn't simply a matter of comparing the input channels to the output
 * channels, since some of the conversions (K -> CMYK) work by means
 * of a chain of conversions (K->CMY->CMYK).  In this case, we need to
 * fully compute the CMY channels, but the K channel in the output is
 * not inverted.
 *
 * The rules that are implemented by the logic below are:
 *
 * 1) If the input is raw, we always perform the normal computation (without
 *    synthesizing channels).
 *
 * 2) If the output is CMY or K only, we never synthesize channels.  We've
 *    now covered raw, black, and CMY/RGB outputs, leaving CMYK and CMYKRB.
 *
 * 3) Output channels above CMYK are synthesized (e. g. RB in CMYKRB).
 *
 * 4) If the input is CMYK, we do not synthesize channels.
 *
 * 5) The black channel (in CMYK) is synthesized.
 *
 * 6) All other channels (CMY/RGB) are not synthesized.
 */

static int
channel_is_synthesized(lut_t *lut, int channel)
{
  if (lut->output_color_description->color_id == COLOR_ID_RAW)
    return 1;			/* Case 1 */
  else if (lut->output_color_description->channels == CMASK_CMY ||
	   lut->output_color_description->channels == CMASK_K)
    return 0;			/* Case 2 */
  else if (channel >= CHANNEL_W)
    return 1;			/* Case 3 */
  else if (lut->input_color_description->channels == CMASK_CMYK)
    return 0;			/* Case 4 */
  else if (channel == CHANNEL_K)
    return 1;			/* Case 5 */
  else
    return 0;			/* Case 6 */
}

static void
compute_a_curve_full(lut_t *lut, int channel)
{
  double *tmp;
  double pivot = .25;
  double ipivot = 1.0 - pivot;
  double xcontrast = pow(lut->contrast, lut->contrast);
  double xgamma = pow(pivot, lut->screen_gamma);
  stp_curve_t curve = stpi_curve_cache_get_curve(&(lut->channel_curves[channel]));
  int i;
  int isteps = lut->steps;
  if (isteps > 256)
    isteps = 256;
  tmp = stpi_malloc(sizeof(double) * lut->steps);
  for (i = 0; i < isteps; i ++)
    {
      double temp_pixel, pixel;
      pixel = (double) i / (double) (isteps - 1);

      if (lut->input_color_description->color_model == COLOR_BLACK)
	pixel = 1.0 - pixel;

      /*
       * First, correct contrast
       */
      if (pixel >= .5)
	temp_pixel = 1.0 - pixel;
      else
	temp_pixel = pixel;
      if (lut->contrast > 3.99999)
	{
	  if (temp_pixel < .5)
	    temp_pixel = 0;
	  else
	    temp_pixel = 1;
	}
      if (temp_pixel <= .000001 && lut->contrast <= .0001)
	temp_pixel = .5;
      else if (temp_pixel > 1)
	temp_pixel = .5 * pow(2 * temp_pixel, xcontrast);
      else if (temp_pixel < 1)
	{
	  if (lut->linear_contrast_adjustment)
	    temp_pixel = 0.5 -
	      ((0.5 - .5 * pow(2 * temp_pixel, lut->contrast)) * lut->contrast);
	  else
	    temp_pixel = 0.5 -
	      ((0.5 - .5 * pow(2 * temp_pixel, lut->contrast)));
	}
      if (temp_pixel > .5)
	temp_pixel = .5;
      else if (temp_pixel < 0)
	temp_pixel = 0;
      if (pixel < .5)
	pixel = temp_pixel;
      else
	pixel = 1 - temp_pixel;

      /*
       * Second, do brightness
       */
      if (lut->brightness < 1)
	pixel = pixel * lut->brightness;
      else
	pixel = 1 - ((1 - pixel) * (2 - lut->brightness));

      /*
       * Third, correct for the screen gamma
       */

      pixel = 1.0 -
	(1.0 / (1.0 - xgamma)) *
	(pow(pivot + ipivot * pixel, lut->screen_gamma) - xgamma);

      /*
       * Third, fix up cyan, magenta, yellow values
       */
      if (pixel < 0.0)
	pixel = 0.0;
      else if (pixel > 1.0)
	pixel = 1.0;

      if (pixel > .9999 && lut->gamma_values[channel] < .00001)
	pixel = 0;
      else
	pixel = 1 - pow(1 - pixel, lut->gamma_values[channel]);

      /*
       * Finally, fix up print gamma and scale
       */

      pixel = 65535 * pow(pixel, lut->print_gamma);	/* was + 0.5 here */
      if (lut->output_color_description->color_model == COLOR_WHITE)
	pixel = 65535 - pixel;

      if (pixel <= 0.0)
	tmp[i] = 0;
      else if (pixel >= 65535.0)
	tmp[i] = 65535;
      else
	tmp[i] = (pixel);
      tmp[i] = floor(tmp[i] + 0.5);		/* rounding is done here */
    }
  stp_curve_set_data(curve, isteps, tmp);
  if (isteps != lut->steps)
    stp_curve_resample(curve, lut->steps);
  stpi_free(tmp);
}

static void
compute_a_curve_fast(lut_t *lut, int channel)
{
  double *tmp;
  stp_curve_t curve = stpi_curve_cache_get_curve(&(lut->channel_curves[channel]));
  int i;
  int isteps = lut->steps;
  if (isteps > 256)
    isteps = 256;
  tmp = stpi_malloc(sizeof(double) * lut->steps);
  for (i = 0; i < isteps; i++)
    {
      double pixel = (double) i / (double) (isteps - 1);
      pixel = 1 - pow(1 - pixel, lut->gamma_values[channel]);
      tmp[i] = floor((65535.0 * pixel) + 0.5);
    }
  stp_curve_set_data(curve, isteps, tmp);
  if (isteps != lut->steps)
    stp_curve_resample(curve, lut->steps);
  stpi_free(tmp);
}

/*
 * If the input and output color spaces both have a particular channel,
 * we want to use the general algorithm.  If not (i. e. we have to
 * synthesize the channel), use a simple gamma curve.
 */
static void
compute_a_curve(lut_t *lut, int channel)
{
  if (channel_is_synthesized(lut, channel))
    compute_a_curve_fast(lut, channel);
  else
    compute_a_curve_full(lut, channel);
}

static void
invert_curve(stp_curve_t curve, int invert_output)
{
  double lo, hi;
  int i;
  size_t count;
  const double *data = stp_curve_get_data(curve, &count);
  double f_gamma = stp_curve_get_gamma(curve);
  double *tmp_data;

  stp_curve_get_bounds(curve, &lo, &hi);

  if (f_gamma)
    stp_curve_set_gamma(curve, -f_gamma);
  else
    {
      tmp_data = stpi_malloc(sizeof(double) * count);
      for (i = 0; i < count; i++)
	tmp_data[i] = data[count - i - 1];
      stp_curve_set_data(curve, count, tmp_data);
      stpi_free(tmp_data);
    }
  if (!invert_output)
    {
      stp_curve_rescale(curve, -1, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(curve, lo + hi, STP_CURVE_COMPOSE_ADD,
			STP_CURVE_BOUNDS_RESCALE);
    }
}

static void
compute_one_lut(lut_t *lut, int i)
{
  stp_curve_t curve =
    stpi_curve_cache_get_curve(&(lut->channel_curves[i]));
  if (curve)
    {
      int invert_output =
	!channel_is_synthesized(lut, i) && lut->invert_output;
      stp_curve_rescale(curve, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      invert_curve(curve, invert_output);
      stp_curve_resample(curve, lut->steps);
    }
  else
    {
      curve = stp_curve_create_copy(color_curve_bounds);
      stp_curve_rescale(curve, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stpi_curve_cache_set_curve(&(lut->channel_curves[i]), curve);
      compute_a_curve(lut, i);
    }
}

static void
setup_channel(stp_vars_t v, int i, const channel_param_t *p)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(v, "Color"));
  const char *gamma_name =
    (lut->output_color_description->color_model == COLOR_BLACK ?
     p->gamma_name : p->rgb_gamma_name);
  const char *curve_name =
    (lut->output_color_description->color_model == COLOR_BLACK ?
     p->curve_name : p->rgb_curve_name);
  if (stp_check_float_parameter(v, p->gamma_name, STP_PARAMETER_DEFAULTED))
    lut->gamma_values[i] = stp_get_float_parameter(v, gamma_name);

  if (stp_get_curve_parameter_active(v, curve_name) > 0 &&
      stp_get_curve_parameter_active(v, curve_name) >=
      stp_get_float_parameter_active(v, gamma_name))
    stpi_curve_cache_set_curve_copy
      (&(lut->channel_curves[i]), stp_get_curve_parameter(v, curve_name));

  stpi_dprintf(STPI_DBG_LUT, " %s %.3f\n", gamma_name, lut->gamma_values[i]);
  compute_one_lut(lut, i);
}


static void
stpi_compute_lut(stp_vars_t v)
{
  int i;
  lut_t *lut = (lut_t *)(stpi_get_component_data(v, "Color"));
  stpi_dprintf(STPI_DBG_LUT, v, "stpi_compute_lut\n");

  if (lut->input_color_description->color_model == COLOR_UNKNOWN ||
      lut->output_color_description->color_model == COLOR_UNKNOWN ||
      lut->input_color_description->color_model ==
      lut->output_color_description->color_model)
    lut->invert_output = 0;
  else
    lut->invert_output = 1;

  lut->linear_contrast_adjustment = 0;
  lut->print_gamma = 1.0;
  lut->app_gamma = 1.0;
  lut->contrast = 1.0;
  lut->brightness = 1.0;

  if (stp_check_boolean_parameter(v, "LinearContrast", STP_PARAMETER_DEFAULTED))
    lut->linear_contrast_adjustment =
      stp_get_boolean_parameter(v, "LinearContrast");
  if (stp_check_float_parameter(v, "Gamma", STP_PARAMETER_DEFAULTED))
    lut->print_gamma = stp_get_float_parameter(v, "Gamma");
  if (stp_check_float_parameter(v, "Contrast", STP_PARAMETER_DEFAULTED))
    lut->contrast = stp_get_float_parameter(v, "Contrast");
  if (stp_check_float_parameter(v, "Brightness", STP_PARAMETER_DEFAULTED))
    lut->brightness = stp_get_float_parameter(v, "Brightness");

  if (stp_check_float_parameter(v, "AppGamma", STP_PARAMETER_ACTIVE))
    lut->app_gamma = stp_get_float_parameter(v, "AppGamma");
  lut->screen_gamma = lut->app_gamma / 4.0; /* "Empirical" */

  /*
   * TODO check that these are wraparound curves and all that
   */

  if (lut->color_correction->correct_hsl)
    {
      if (stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_DEFAULTED))
	lut->hue_map.curve =
	  stp_curve_create_copy(stp_get_curve_parameter(v, "HueMap"));
      if (stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_DEFAULTED))
	lut->lum_map.curve =
	  stp_curve_create_copy(stp_get_curve_parameter(v, "LumMap"));
      if (stp_check_curve_parameter(v, "SatMap", STP_PARAMETER_DEFAULTED))
	lut->sat_map.curve =
	  stp_curve_create_copy(stp_get_curve_parameter(v, "SatMap"));
    }

  stpi_dprintf(STPI_DBG_LUT, v, " print_gamma %.3f\n", lut->print_gamma);
  stpi_dprintf(STPI_DBG_LUT, v, " contrast %.3f\n", lut->contrast);
  stpi_dprintf(STPI_DBG_LUT, v, " brightness %.3f\n", lut->brightness);
  stpi_dprintf(STPI_DBG_LUT, v, " screen_gamma %.3f\n", lut->screen_gamma);

  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    {
      if (lut->output_color_description->channel_count < 1 &&
	  i < lut->out_channels)
	setup_channel(v, i, &(raw_channel_params[i]));
      else if (i < channel_param_count &&
	       lut->output_color_description->channels & (1 << i))
	setup_channel(v, i, &(channel_params[i]));
    }
  if (((lut->output_color_description->channels & CMASK_CMYK) == CMASK_CMYK) &&
      (lut->input_color_description->color_id == COLOR_ID_GRAY ||
       lut->input_color_description->color_id == COLOR_ID_WHITE ||
       lut->input_color_description->color_id == COLOR_ID_RGB ||
       lut->input_color_description->color_id == COLOR_ID_CMY))
    initialize_gcr_curve(v);
}

static int
stpi_color_traditional_init(stp_vars_t v,
			    stp_image_t *image,
			    size_t steps)
{
  lut_t *lut;
  const char *image_type = stp_get_string_parameter(v, "ImageType");
  const char *color_correction = stp_get_string_parameter(v, "ColorCorrection");
  const channel_depth_t *channel_depth =
    get_channel_depth(stp_get_string_parameter(v, "ChannelBitDepth"));
  size_t total_channel_bits;

  if (steps != 256 && steps != 65536)
    return -1;
  if (!channel_depth)
    return -1;

  lut = allocate_lut();
  lut->input_color_description =
    get_color_description(stp_get_string_parameter(v, "InputImageType"));
  lut->output_color_description =
    get_color_description(stp_get_string_parameter(v, "STPIOutputType"));

  if (!lut->input_color_description || !lut->output_color_description)
    {
      free_lut(lut);
      return -1;
    }

  if (lut->input_color_description->color_id == COLOR_ID_RAW)
    {
      if (stpi_verify_parameter(v, "STPIRawChannels", 1) != PARAMETER_OK)
	{
	  free_lut(lut);
	  return -1;
	}
      lut->out_channels = stp_get_int_parameter(v, "STPIRawChannels");
      lut->in_channels = lut->out_channels;
    }
  else
    {
      lut->out_channels = lut->output_color_description->channel_count;
      lut->in_channels = lut->input_color_description->channel_count;
    }

  stpi_allocate_component_data(v, "Color", copy_lut, free_lut, lut);
  lut->steps = steps;
  lut->channel_depth = channel_depth->bits;

  if (image_type && strcmp(image_type, "None") != 0)
    {
      if (strcmp(image_type, "Text") == 0)
	lut->color_correction = get_color_correction("Threshold");
      else
	lut->color_correction = get_color_correction("None");
    }
  else if (color_correction)
    lut->color_correction = get_color_correction(color_correction);
  else
    lut->color_correction = get_color_correction("None");
  if (lut->color_correction->correction == COLOR_CORRECTION_DEFAULT)
    lut->color_correction =
      (get_color_correction_by_tag
       (lut->output_color_description->default_correction));

  stpi_compute_lut(v);

  lut->image_width = stpi_image_width(image);
  total_channel_bits = lut->in_channels * lut->channel_depth;
  lut->in_data = stpi_malloc(((lut->image_width * total_channel_bits) + 7)/8);
  memset(lut->in_data, 0, ((lut->image_width * total_channel_bits) + 7) / 8);
  return lut->out_channels;
}

static void
initialize_standard_curves(void)
{
  if (!standard_curves_initialized)
    {
      int i;
      hue_map_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
	 "<sequence count=\"2\" lower-bound=\"-6\" upper-bound=\"6\">\n"
	 "0 0\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      lum_map_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
	 "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"4\">\n"
	 "1 1\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      sat_map_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
	 "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"4\">\n"
	 "1 1\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      color_curve_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"1.0\">\n"
	 "<sequence count=\"0\" lower-bound=\"0\" upper-bound=\"1\">\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      gcr_curve_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"0.0\">\n"
	 "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"1\">\n"
	 "1 1\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      for (i = 0; i < curve_parameter_count; i++)
	curve_parameters[i].param.deflt.curve =
	 *(curve_parameters[i].defval);
      standard_curves_initialized = 1;
    }
}

static stp_parameter_list_t
stpi_color_traditional_list_parameters(stp_const_vars_t v)
{
  stpi_list_t *ret = stp_parameter_list_create();
  int i;
  initialize_standard_curves();
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  for (i = 0; i < curve_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(curve_parameters[i].param));
  return ret;
}

static void
stpi_color_traditional_describe_parameter(stp_const_vars_t v,
					  const char *name,
					  stp_parameter_t *description)
{
  int i, j;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  initialize_standard_curves();
  if (name == NULL)
    return;

  for (i = 0; i < float_parameter_count; i++)
    {
      const float_param_t *param = &(float_parameters[i]);
      if (strcmp(name, param->param.name) == 0)
	{
	  stpi_fill_parameter_settings(description, &(param->param));
	  if (param->channel_mask != CMASK_EVERY)
	    {
	      const color_description_t *color_description =
		get_color_description(stpi_describe_output(v));
	      if (color_description &&
		  (param->channel_mask & color_description->channels) &&
		  param->channel_mask != CMASK_RAW)
		description->is_active = 1;
	      else
		description->is_active = 0;
	      if (param->color_only &&
		  !(color_description->channels & ~CMASK_K))
		description->is_active = 0;
	    }
	  if (stp_check_string_parameter(v, "ImageType", STP_PARAMETER_ACTIVE) &&
	      strcmp(stp_get_string_parameter(v, "ImageType"), "None") != 0 &&
	      description->p_level > STP_PARAMETER_LEVEL_BASIC)
	    description->is_active = 0;
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      description->deflt.boolean = (int) param->defval;
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      description->bounds.integer.upper = (int) param->max;
	      description->bounds.integer.lower = (int) param->min;
	      description->deflt.integer = (int) param->defval;
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      description->bounds.dbl.upper = param->max;
	      description->bounds.dbl.lower = param->min;
	      description->deflt.dbl = param->defval;
	      if (strcmp(name, "InkLimit") == 0)
		{
		  stp_parameter_t ink_limit_desc;
		  stp_describe_parameter(v, "InkChannels", &ink_limit_desc);
		  if (ink_limit_desc.p_type == STP_PARAMETER_TYPE_INT)
		    {
		      if (ink_limit_desc.deflt.integer > 1)
			{
			  description->bounds.dbl.upper =
			    ink_limit_desc.deflt.integer;
			  description->deflt.dbl =
			    ink_limit_desc.deflt.integer;
			}
		      else
			description->is_active = 0;
		    }
		  stp_parameter_description_free(&ink_limit_desc);
		}
	      break;
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      if (!strcmp(param->param.name, "ColorCorrection"))
		{
		  description->bounds.str = stp_string_list_create();
		  for (j = 0; j < color_correction_count; j++)
		    stp_string_list_add_string
		      (description->bounds.str, color_corrections[j].name,
		       _(color_corrections[j].text));
		  description->deflt.str =
		    stp_string_list_param(description->bounds.str, 0)->name;
		}
	      else if (strcmp(name, "ChannelBitDepth") == 0)
		{
		  description->bounds.str = stp_string_list_create();
		  for (j = 0; j < channel_depth_count; j++)
		    stp_string_list_add_string
		      (description->bounds.str, channel_depths[j].name,
		       channel_depths[j].name);
		  description->deflt.str =
		    stp_string_list_param(description->bounds.str, 0)->name;
		}
	      else if (strcmp(name, "InputImageType") == 0)
		{
		  description->bounds.str = stp_string_list_create();
		  for (j = 0; j < color_description_count; j++)
		    if (color_descriptions[j].input)
		      {
			if (color_descriptions[j].color_id == COLOR_ID_RAW)
			  {
			    stp_parameter_t desc;
			    stp_describe_parameter(v, "RawChannels", &desc);
			    if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
			      stp_string_list_add_string
				(description->bounds.str,
				 color_descriptions[j].name,
				 color_descriptions[j].name);
			    stp_parameter_description_free(&desc);
			  }
			else
			  stp_string_list_add_string
			    (description->bounds.str,
			     color_descriptions[j].name,
			     color_descriptions[j].name);
		      }
		  description->deflt.str =
		    stp_string_list_param(description->bounds.str, 0)->name;
		}
	      else if (strcmp(name, "OutputImageType") == 0)
		{
		  description->bounds.str = stp_string_list_create();
		  for (j = 0; j < color_description_count; j++)
		    if (color_descriptions[j].output)
		      stp_string_list_add_string
			(description->bounds.str, color_descriptions[j].name,
			 color_descriptions[j].name);
		  description->deflt.str =
		    stp_string_list_param(description->bounds.str, 0)->name;
		}
	      break;
	    default:
	      break;
	    }
	  return;
	}
    }
  for (i = 0; i < curve_parameter_count; i++)
    {
      curve_param_t *param = &(curve_parameters[i]);
      if (strcmp(name, param->param.name) == 0)
	{
	  description->is_active = 1;
	  stpi_fill_parameter_settings(description, &(param->param));
	  if (param->channel_mask != CMASK_EVERY)
	    {
	      const color_description_t *color_description =
		get_color_description(stpi_describe_output(v));
	      if (color_description &&
		  (param->channel_mask & color_description->channels))
		description->is_active = 1;
	      else
		description->is_active = 0;
	      if (param->color_only &&
		  !(color_description->channels & ~CMASK_K))
		description->is_active = 0;
	    }
	  if (stp_check_string_parameter(v, "ImageType", STP_PARAMETER_ACTIVE) &&
	      strcmp(stp_get_string_parameter(v, "ImageType"), "None") != 0 &&
	      description->p_level > STP_PARAMETER_LEVEL_BASIC)
	    description->is_active = 0;
	  else if (param->hsl_only)
	    {
	      const color_correction_t *correction =
		(get_color_correction
		 (stp_get_string_parameter (v, "ColorCorrection")));
	      if (correction && !correction->correct_hsl)
		description->is_active = 0;
	    }
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_CURVE:
	      description->deflt.curve = *(param->defval);
	      description->bounds.curve =
		stp_curve_create_copy(*(param->defval));
	      break;
	    default:
	      break;
	    }
	  return;
	}
    }
}


static const stpi_colorfuncs_t stpi_color_traditional_colorfuncs =
{
  &stpi_color_traditional_init,
  &stpi_color_traditional_get_row,
  &stpi_color_traditional_list_parameters,
  &stpi_color_traditional_describe_parameter
};

static const stpi_internal_color_t stpi_color_traditional_module_data =
  {
    COOKIE_COLOR,
    "traditional",
    N_("Traditional Gimp-Print color conversion"),
    &stpi_color_traditional_colorfuncs
  };


static int
color_traditional_module_init(void)
{
  return stpi_color_register(&stpi_color_traditional_module_data);
}


static int
color_traditional_module_exit(void)
{
  return stpi_color_unregister(&stpi_color_traditional_module_data);
}


/* Module header */
#define stpi_module_version color_traditional_LTX_stpi_module_version
#define stpi_module_data color_traditional_LTX_stpi_module_data

stpi_module_version_t stpi_module_version = {0, 0};

stpi_module_t stpi_module_data =
  {
    "traditional",
    VERSION,
    "Traditional Gimp-Print color conversion",
    STPI_MODULE_CLASS_COLOR,
    NULL,
    color_traditional_module_init,
    color_traditional_module_exit,
    (void *) &stpi_color_traditional_module_data
  };
