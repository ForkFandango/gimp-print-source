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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "print-escp2.h"

const res_t stpi_escp2_standard_reslist[] =
{
  { "360x90dpi",        N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   360,  90,   0,  0, 1, 1, 0, 1, 1, RES_120_M },
  { "360x90sw",         N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   360,  90,   1,  0, 1, 1, 0, 1, 1, RES_120 },

  { "360x120dpi",       N_("360 x 120 DPI Economy Draft"),
    360,  120,  360,  120,  0,  0, 1, 1, 0, 3, 1, RES_120_M },
  { "360x120sw",        N_("360 x 120 DPI Economy Draft"),
    360,  120,  360,  120,  1,  0, 1, 1, 0, 3, 1, RES_120 },

  { "180dpi",           N_("180 DPI Economy Draft"),
    180,  180,  180,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "180sw",            N_("180 DPI Economy Draft"),
    180,  180,  180,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360x240dpi",       N_("360 x 240 DPI Draft"),
    360,  240,  360,  240,  0,  0, 1, 1, 0, 3, 2, RES_180_M },
  { "360x240sw",        N_("360 x 240 DPI Draft"),
    360,  240,  360,  240,  1,  0, 1, 1, 0, 3, 2, RES_180 },

  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "360x180sw",        N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360sw",            N_("360 DPI"),
    360,  360,  360,  360,  1,  0, 1, 1, 0, 1, 1, RES_360 },
  { "360swuni",         N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  1,  0, 1, 1, 1, 1, 1, RES_360 },
  { "360mw",            N_("360 DPI Microweave"),
    360,  360,  360,  360,  0,  1, 1, 1, 0, 1, 1, RES_360_M },
  { "360mwuni",         N_("360 DPI Microweave Unidirectional"),
    360,  360,  360,  360,  0,  1, 1, 1, 1, 1, 1, RES_360_M },
  { "360dpi",           N_("360 DPI"),
    360,  360,  360,  360,  0,  0, 1, 1, 0, 1, 1, RES_360_M },
  { "360uni",           N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  0,  0, 1, 1, 1, 1, 1, RES_360_M },

  { "720x360sw",        N_("720 x 360 DPI"),
    720,  360,  720,  360,  1,  0, 1, 1, 0, 2, 1, RES_720_360 },
  { "720x360swuni",     N_("720 x 360 DPI Unidirectional"),
    720,  360,  720,  360,  1,  0, 1, 1, 1, 2, 1, RES_720_360 },

  { "720mw",            N_("720 DPI Microweave"),
    720,  720,  720,  720,  0,  1, 1, 1, 0, 1, 1, RES_720_M },
  { "720mwuni",         N_("720 DPI Microweave Unidirectional"),
    720,  720,  720,  720,  0,  1, 1, 1, 1, 1, 1, RES_720_M },
  { "720sw",            N_("720 DPI"),
    720,  720,  720,  720,  1,  0, 1, 1, 0, 1, 1, RES_720 },
  { "720swuni",         N_("720 DPI Unidirectional"),
    720,  720,  720,  720,  1,  0, 1, 1, 1, 1, 1, RES_720 },
  { "720hq",            N_("720 DPI High Quality"),
    720,  720,  720,  720,  1,  0, 2, 1, 0, 1, 1, RES_720 },
  { "720hquni",         N_("720 DPI High Quality Unidirectional"),
    720,  720,  720,  720,  1,  0, 2, 1, 1, 1, 1, RES_720 },
  { "720hq2",           N_("720 DPI Highest Quality"),
    720,  720,  720,  720,  1,  0, 4, 1, 1, 1, 1, RES_720 },

  { "1440x720mw",       N_("1440 x 720 DPI Microweave"),
    1440, 720,  1440, 720,  0,  1, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720mwuni",    N_("1440 x 720 DPI Microweave Unidirectional"),
    1440, 720,  1440, 720,  0,  1, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720sw",       N_("1440 x 720 DPI"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 0, 1, 1, RES_1440_720 },
  { "1440x720swuni",    N_("1440 x 720 DPI Unidirectional"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 1, 1, 1, RES_1440_720 },
  { "1440x720hq2",      N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1440, 720,  1,  0, 2, 1, 1, 1, 1, RES_1440_720 },

  {"2880x720sw",       N_("2880 x 720 DPI"),
    2880, 720,  2880, 720,  1,  0, 1, 1, 0, 1, 1, RES_2880_720},
  { "2880x720swuni",    N_("2880 x 720 DPI Unidirectional"),
    2880, 720,  2880, 720,  1,  0, 1, 1, 1, 1, 1, RES_2880_720},

  { "1440x1440sw",      N_("1440 x 1440 DPI"),
    1440, 1440, 1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_1440_1440},
  { "1440x1440hq2",     N_("1440 x 1440 DPI Highest Quality"),
    1440, 1440, 1440, 1440, 1,  0, 2, 1, 1, 1, 1, RES_1440_1440},

  { "2880x1440sw",      N_("2880 x 1440 DPI"),
    2880, 1440, 2880, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "", "", 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 }
};

const res_t stpi_escp2_sp5000_reslist[] =
{
  { "180sw",            N_("180 DPI Economy Draft"),
    180,  180,  180,  180,  1,  0, 1, 1, 0, 4, 1, RES_180 },

  { "360x180sw",        N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  1,  0, 1, 1, 0, 4, 1, RES_180 },

  { "360sw",            N_("360 DPI"),
    360,  360,  360,  360,  1,  0, 1, 1, 0, 2, 1, RES_360 },
  { "360swuni",         N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  1,  0, 1, 1, 1, 2, 1, RES_360 },

  { "720x360sw",        N_("720 x 360 DPI"),
    720,  360,  720,  360,  1,  0, 1, 1, 0, 2, 1, RES_720_360 },
  { "720x360swuni",     N_("720 x 360 DPI Unidirectional"),
    720,  360,  720,  360,  1,  0, 1, 1, 1, 2, 1, RES_720_360 },

  { "720sw",            N_("720 DPI"),
    720,  720,  720,  720,  1,  0, 1, 1, 0, 1, 1, RES_720 },
  { "720swuni",         N_("720 DPI Unidirectional"),
    720,  720,  720,  720,  1,  0, 1, 1, 1, 1, 1, RES_720 },
  { "720hq",            N_("720 DPI High Quality"),
    720,  720,  720,  720,  1,  0, 2, 1, 0, 1, 1, RES_720 },
  { "720hquni",         N_("720 DPI High Quality Unidirectional"),
    720,  720,  720,  720,  1,  0, 2, 1, 1, 1, 1, RES_720 },
  { "720hq2",           N_("720 DPI Highest Quality"),
    720,  720,  720,  720,  1,  0, 4, 1, 1, 1, 1, RES_720 },

  { "1440x720sw",       N_("1440 x 720 DPI"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 0, 1, 1, RES_1440_720 },
  { "1440x720swuni",    N_("1440 x 720 DPI Unidirectional"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 1, 1, 1, RES_1440_720 },
  { "1440x720hq2",      N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1440, 720,  1,  0, 2, 1, 1, 1, 1, RES_1440_720 },

  { "", "", 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 }
};

const res_t stpi_escp2_escp950_reslist[] =
{
  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "360x180sw",        N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360sw",            N_("360 DPI"),
    360,  360,  360,  360,  1,  0, 1, 1, 0, 1, 1, RES_360 },
  { "360swuni",         N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  1,  0, 1, 1, 1, 1, 1, RES_360 },
  { "360mw",            N_("360 DPI Microweave"),
    360,  360,  360,  360,  0,  1, 1, 1, 0, 1, 1, RES_360_M },
  { "360mwuni",         N_("360 DPI Microweave Unidirectional"),
    360,  360,  360,  360,  0,  1, 1, 1, 1, 1, 1, RES_360_M },
  { "360dpi",           N_("360 DPI"),
    360,  360,  360,  360,  0,  0, 1, 1, 0, 1, 1, RES_360_M },
  { "360uni",           N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  0,  0, 1, 1, 1, 1, 1, RES_360_M },

  { "720x360sw",        N_("720 x 360 DPI"),
    720,  360,  720,  360,  1,  0, 1, 1, 0, 2, 1, RES_720_360 },
  { "720x360swuni",     N_("720 x 360 DPI Unidirectional"),
    720,  360,  720,  360,  1,  0, 1, 1, 1, 2, 1, RES_720_360 },

  { "720mw",            N_("720 DPI Microweave"),
    720,  720,  720,  720,  0,  1, 1, 1, 0, 1, 1, RES_720_M },
  { "720mwuni",         N_("720 DPI Microweave Unidirectional"),
    720,  720,  720,  720,  0,  1, 1, 1, 1, 1, 1, RES_720_M },
  { "720sw",            N_("720 DPI"),
    720,  720,  720,  720,  1,  0, 1, 1, 0, 1, 1, RES_720 },
  { "720swuni",         N_("720 DPI Unidirectional"),
    720,  720,  720,  720,  1,  0, 1, 1, 1, 1, 1, RES_720 },
  { "720hq",            N_("720 DPI High Quality"),
    720,  720,  720,  720,  1,  0, 2, 1, 0, 1, 1, RES_720 },
  { "720hquni",         N_("720 DPI High Quality Unidirectional"),
    720,  720,  720,  720,  1,  0, 2, 1, 1, 1, 1, RES_720 },
  { "720hq2",           N_("720 DPI Highest Quality"),
    720,  720,  720,  720,  1,  0, 4, 1, 1, 1, 1, RES_720 },

  { "1440x720mw",       N_("1440 x 720 DPI Microweave"),
    1440, 720,  1440, 720,  0,  1, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720mwuni",    N_("1440 x 720 DPI Microweave Unidirectional"),
    1440, 720,  1440, 720,  0,  1, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720sw",       N_("1440 x 720 DPI"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 0, 1, 1, RES_1440_720 },
  { "1440x720swuni",    N_("1440 x 720 DPI Unidirectional"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 1, 1, 1, RES_1440_720 },
  { "1440x720hq2",      N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1440, 720,  1,  0, 2, 1, 1, 1, 1, RES_1440_720 },

  { "2880x720sw",       N_("2880 x 720 DPI"),
    2880, 1440, 2880, 720,  1,  0, 1, 1, 0, 1, 1, RES_2880_1440},
  { "2880x720swuni",    N_("2880 x 720 DPI Unidirectional"),
    2880, 1440, 2880, 720,  1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "1440x1440sw",      N_("1440 x 1440 DPI"),
    2880, 1440, 1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},
  { "1440x1440hq2",     N_("1440 x 1440 DPI Highest Quality"),
    2880, 1440, 1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "2880x1440sw",      N_("2880 x 1440 DPI"),
    2880, 1440, 2880, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "", "", 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 }
};

const res_t stpi_escp2_escp2200_reslist[] =
{
  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "360x180sw",        N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360sw",            N_("360 DPI"),
    360,  360,  360,  360,  1,  0, 1, 1, 0, 1, 1, RES_360 },
  { "360swuni",         N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  1,  0, 1, 1, 1, 1, 1, RES_360 },

  { "720x360sw",        N_("720 x 360 DPI"),
    720,  360,  720,  360,  1,  0, 1, 1, 0, 2, 1, RES_720_360 },
  { "720x360swuni",     N_("720 x 360 DPI Unidirectional"),
    720,  360,  720,  360,  1,  0, 1, 1, 1, 2, 1, RES_720_360 },

  { "720sw",            N_("720 DPI"),
    720,  720,  720,  720,  1,  0, 1, 1, 0, 1, 1, RES_720 },
  { "720swuni",         N_("720 DPI Unidirectional"),
    720,  720,  720,  720,  1,  0, 1, 1, 1, 1, 1, RES_720 },
  { "720hq",            N_("720 DPI High Quality"),
    720,  720,  720,  720,  1,  0, 2, 1, 0, 1, 1, RES_720 },
  { "720hquni",         N_("720 DPI High Quality Unidirectional"),
    720,  720,  720,  720,  1,  0, 2, 1, 1, 1, 1, RES_720 },
  { "720hq2",           N_("720 DPI Highest Quality"),
    720,  720,  720,  720,  1,  0, 4, 1, 1, 1, 1, RES_720 },

  { "1440x720sw",       N_("1440 x 720 DPI"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 0, 1, 1, RES_1440_720 },
  { "1440x720swuni",    N_("1440 x 720 DPI Unidirectional"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 1, 1, 1, RES_1440_720 },
  { "1440x720hq2",      N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1440, 720,  1,  0, 2, 1, 1, 1, 1, RES_1440_720 },

  { "2880x720sw",       N_("2880 x 720 DPI"),
    2880, 720,  2880, 720,  1,  0, 1, 1, 0, 1, 1, RES_2880_720},
  { "2880x720swuni",    N_("2880 x 720 DPI Unidirectional"),
    2880, 720,  2880, 720,  1,  0, 1, 1, 1, 1, 1, RES_2880_720},

  { "1440x1440sw",      N_("1440 x 1440 DPI"),
    2880, 1440, 1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},
  { "1440x1440hq2",     N_("1440 x 1440 DPI Highest Quality"),
    2880, 1440, 1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "2880x1440sw",      N_("2880 x 1440 DPI"),
    2880, 1440, 2880, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "", "", 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 }
};

const res_t stpi_escp2_no_microweave_reslist[] =
{
  { "360x90dpi",        N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   360,  90,   0,  0, 1, 1, 0, 1, 1, RES_120_M },
  { "360x90sw",         N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   360,  90,   1,  0, 1, 1, 0, 1, 1, RES_120 },

  { "360x120dpi",       N_("360 x 120 DPI Economy Draft"),
    360,  120,  360,  120,  0,  0, 1, 1, 0, 3, 1, RES_120_M },
  { "360x120sw",        N_("360 x 120 DPI Economy Draft"),
    360,  120,  360,  120,  1,  0, 1, 1, 0, 3, 1, RES_120 },

  { "180dpi",           N_("180 DPI Economy Draft"),
    180,  180,  180,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "180sw",            N_("180 DPI Economy Draft"),
    180,  180,  180,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360x240dpi",       N_("360 x 240 DPI Draft"),
    360,  240,  360,  240,  0,  0, 1, 1, 0, 3, 2, RES_180_M },
  { "360x240sw",        N_("360 x 240 DPI Draft"),
    360,  240,  360,  240,  1,  0, 1, 1, 0, 3, 2, RES_180 },

  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "360x180sw",        N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360sw",            N_("360 DPI"),
    360,  360,  360,  360,  1,  0, 1, 1, 0, 1, 1, RES_360 },
  { "360swuni",         N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  1,  0, 1, 1, 1, 1, 1, RES_360 },
  { "360dpi",           N_("360 DPI"),
    360,  360,  360,  360,  0,  0, 1, 1, 0, 1, 1, RES_360_M },
  { "360uni",           N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  0,  0, 1, 1, 1, 1, 1, RES_360_M },

  { "720x360sw",        N_("720 x 360 DPI"),
    720,  360,  720,  360,  1,  0, 1, 1, 0, 2, 1, RES_720_360 },
  { "720x360swuni",     N_("720 x 360 DPI Unidirectional"),
    720,  360,  720,  360,  1,  0, 1, 1, 1, 2, 1, RES_720_360 },

  { "720sw",            N_("720 DPI"),
    720,  720,  720,  720,  1,  0, 1, 1, 0, 1, 1, RES_720 },
  { "720swuni",         N_("720 DPI Unidirectional"),
    720,  720,  720,  720,  1,  0, 1, 1, 1, 1, 1, RES_720 },
  { "720hq",            N_("720 DPI High Quality"),
    720,  720,  720,  720,  1,  0, 2, 1, 0, 1, 1, RES_720 },
  { "720hquni",         N_("720 DPI High Quality Unidirectional"),
    720,  720,  720,  720,  1,  0, 2, 1, 1, 1, 1, RES_720 },
  { "720hq2",           N_("720 DPI Highest Quality"),
    720,  720,  720,  720,  1,  0, 4, 1, 1, 1, 1, RES_720 },

  { "1440x720sw",       N_("1440 x 720 DPI"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 0, 1, 1, RES_1440_720 },
  { "1440x720swuni",    N_("1440 x 720 DPI Unidirectional"),
    1440, 720,  1440, 720,  1,  0, 1, 1, 1, 1, 1, RES_1440_720 },
  { "1440x720hq2",      N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1440, 720,  1,  0, 2, 1, 1, 1, 1, RES_1440_720 },

  { "2880x720sw",       N_("2880 x 720 DPI"),
    2880, 720,  2880, 720,  1,  0, 1, 1, 0, 1, 1, RES_2880_720},
  { "2880x720swuni",    N_("2880 x 720 DPI Unidirectional"),
    2880, 720,  2880, 720,  1,  0, 1, 1, 1, 1, 1, RES_2880_720},

  { "1440x1440sw",      N_("1440 x 1440 DPI"),
    1440, 1440, 1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_1440_1440},
  { "1440x1440hq2",     N_("1440 x 1440 DPI Highest Quality"),
    1440, 1440, 1440, 1440, 1,  0, 2, 1, 1, 1, 1, RES_1440_1440},

  { "2880x1440sw",      N_("2880 x 1440 DPI"),
    2880, 1440, 2880, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "", "", 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 }
};

const res_t stpi_escp2_pro_reslist[] =
{
  { "360x90dpi",        N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   360,  90,   0,  0, 1, 1, 0, 1, 1, RES_120_M },

  { "360x120dpi",       N_("360 x 120 DPI Economy Draft"),
    360,  120,  360,  120,  0,  0, 1, 1, 0, 3, 1, RES_120_M },

  { "180dpi",           N_("180 DPI Economy Draft"),
    180,  180,  180,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },

  { "360x240dpi",       N_("360 x 240 DPI Draft"),
    360,  240,  360,  240,  0,  0, 1, 1, 0, 3, 2, RES_180_M },

  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },

  { "360mw",            N_("360 DPI Microweave"),
    360,  360,  360,  360,  0,  1, 1, 1, 0, 1, 1, RES_360_M },
  { "360mwuni",         N_("360 DPI Microweave Unidirectional"),
    360,  360,  360,  360,  0,  1, 1, 1, 1, 1, 1, RES_360_M },
  { "360dpi",           N_("360 DPI"),
    360,  360,  360,  360,  0,  0, 1, 1, 0, 1, 1, RES_360_M },
  { "360uni",           N_("360 DPI Unidirectional"),
    360,  360,  360,  360,  0,  0, 1, 1, 1, 1, 1, RES_360_M },
  { "360fol",           N_("360 DPI Full Overlap"),
    360,  360,  360,  360,  0,  2, 1, 1, 0, 1, 1, RES_360_M },
  { "360foluni",        N_("360 DPI Full Overlap Unidirectional"),
    360,  360,  360,  360,  0,  2, 1, 1, 1, 1, 1, RES_360_M },
  { "360fol2",          N_("360 DPI FOL2"),
    360,  360,  360,  360,  0,  4, 1, 1, 0, 1, 1, RES_360_M },
  { "360fol2uni",       N_("360 DPI FOL2 Unidirectional"),
    360,  360,  360,  360,  0,  4, 1, 1, 1, 1, 1, RES_360_M },
  { "360mw2",           N_("360 DPI MW2"),
    360,  360,  360,  360,  0,  5, 1, 1, 0, 1, 1, RES_360_M },
  { "360mw2uni",        N_("360 DPI MW2 Unidirectional"),
    360,  360,  360,  360,  0,  5, 1, 1, 1, 1, 1, RES_360_M },

  { "720x360dpi",       N_("720 x 360 DPI"),
    720,  360,  720,  360,  0,  0, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360uni",       N_("720 x 360 DPI Unidirectional"),
    720,  360,  720,  360,  0,  0, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360mw",        N_("720 x 360 DPI Microweave"),
    720,  360,  720,  360,  0,  1, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360mwuni",     N_("720 x 360 DPI Microweave Unidirectional"),
    720,  360,  720,  360,  0,  1, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360fol",       N_("720 x 360 DPI FOL"),
    720,  360,  720,  360,  0,  2, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360foluni",    N_("720 x 360 DPI FOL Unidirectional"),
    720,  360,  720,  360,  0,  2, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360fol2",      N_("720 x 360 DPI FOL2"),
    720,  360,  720,  360,  0,  4, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360fol2uni",   N_("720 x 360 DPI FOL2 Unidirectional"),
    720,  360,  720,  360,  0,  4, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360mw2",       N_("720 x 360 DPI MW2"),
    720,  360,  720,  360,  0,  5, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360mw2uni",    N_("720 x 360 DPI MW2 Unidirectional"),
    720,  360,  720,  360,  0,  5, 1, 1, 1, 2, 1, RES_720_360_M },

  { "720mw",            N_("720 DPI Microweave"),
    720,  720,  720,  720,  0,  1, 1, 1, 0, 1, 1, RES_720_M },
  { "720mwuni",         N_("720 DPI Microweave Unidirectional"),
    720,  720,  720,  720,  0,  1, 1, 1, 1, 1, 1, RES_720_M },
  { "720fol",           N_("720 DPI Full Overlap"),
    720,  720,  720,  720,  0,  2, 1, 1, 0, 1, 1, RES_720_M },
  { "720foluni",        N_("720 DPI Full Overlap Unidirectional"),
    720,  720,  720,  720,  0,  2, 1, 1, 1, 1, 1, RES_720_M },
  { "720fourp",         N_("720 DPI Four Pass"),
    720,  720,  720,  720,  0,  3, 1, 1, 0, 1, 1, RES_720_M },
  { "720fourpuni",      N_("720 DPI Four Pass Unidirectional"),
    720,  720,  720,  720,  0,  3, 1, 1, 1, 1, 1, RES_720_M },

  { "1440x720mw",       N_("1440 x 720 DPI Microweave"),
    1440, 720,  1440, 720,  0,  1, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720mwuni",    N_("1440 x 720 DPI Microweave Unidirectional"),
    1440, 720,  1440, 720,  0,  1, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720fol",      N_("1440 x 720 DPI FOL"),
    1440, 720,  1440, 720,  0,  2, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720foluni",   N_("1440 x 720 DPI FOL Unidirectional"),
    1440, 720,  1440, 720,  0,  2, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720fourp",    N_("1440 x 720 DPI Four Pass"),
    1440, 720,  1440, 720,  0,  3, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720fourpuni", N_("1440 x 720 DPI Four Pass Unidirectional"),
    1440, 720,  1440, 720,  0,  3, 1, 1, 1, 1, 1, RES_1440_720_M },

  { "2880x720mw",       N_("2880 x 720 DPI Microweave"),
    2880, 720,  2880, 720,  0,  1, 1, 1, 0, 1, 1, RES_2880_720_M },
  { "2880x720mwuni",    N_("2880 x 720 DPI Microweave Unidirectional"),
    2880, 720,  2880, 720,  0,  1, 1, 1, 1, 1, 1, RES_2880_720_M },
  { "2880x720fol",      N_("2880 x 720 DPI FOL"),
    2880, 720,  2880, 720,  0,  2, 1, 1, 0, 1, 1, RES_2880_720_M },
  { "2880x720foluni",   N_("2880 x 720 DPI FOL Unidirectional"),
    2880, 720,  2880, 720,  0,  2, 1, 1, 1, 1, 1, RES_2880_720_M },
  { "2880x720fourp",    N_("2880 x 720 DPI Four Pass"),
    2880, 720,  2880, 720,  0,  3, 1, 1, 0, 1, 1, RES_2880_720_M },
  { "2880x720fourpuni", N_("2880 x 720 DPI Four Pass Unidirectional"),
    2880, 720,  2880, 720,  0,  3, 1, 1, 1, 1, 1, RES_2880_720_M },

  { "1440x1440mw",       N_("1440 x 1440 DPI Microweave"),
    1440, 1440,  1440, 1440,  0,  1, 1, 1, 0, 1, 1, RES_1440_1440_M },
  { "1440x1440mwuni",    N_("1440 x 1440 DPI Microweave Unidirectional"),
    1440, 1440,  1440, 1440,  0,  1, 1, 1, 1, 1, 1, RES_1440_1440_M },
  { "1440x1440fol",      N_("1440 x 1440 DPI FOL"),
    1440, 1440,  1440, 1440,  0,  2, 1, 1, 0, 1, 1, RES_1440_1440_M },
  { "1440x1440foluni",   N_("1440 x 1440 DPI FOL Unidirectional"),
    1440, 1440,  1440, 1440,  0,  2, 1, 1, 1, 1, 1, RES_1440_1440_M },
  { "1440x1440fourp",    N_("1440 x 1440 DPI Four Pass"),
    1440, 1440,  1440, 1440,  0,  3, 1, 1, 0, 1, 1, RES_1440_1440_M },
  { "1440x1440fourpuni", N_("1440 x 1440 DPI Four Pass Unidirectional"),
    1440, 1440,  1440, 1440,  0,  3, 1, 1, 1, 1, 1, RES_1440_1440_M },

  { "2880x1440mw",       N_("2880 x 1440 DPI Microweave"),
    2880, 1440,  2880, 1440,  0,  1, 1, 1, 0, 1, 1, RES_2880_1440_M },
  { "2880x1440mwuni",    N_("2880 x 1440 DPI Microweave Unidirectional"),
    2880, 1440,  2880, 1440,  0,  1, 1, 1, 1, 1, 1, RES_2880_1440_M },
  { "2880x1440fol",      N_("2880 x 1440 DPI FOL"),
    2880, 1440,  2880, 1440,  0,  2, 1, 1, 0, 1, 1, RES_2880_1440_M },
  { "2880x1440foluni",   N_("2880 x 1440 DPI FOL Unidirectional"),
    2880, 1440,  2880, 1440,  0,  2, 1, 1, 1, 1, 1, RES_2880_1440_M },
  { "2880x1440fourp",    N_("2880 x 1440 DPI Four Pass"),
    2880, 1440,  2880, 1440,  0,  3, 1, 1, 0, 1, 1, RES_2880_1440_M },
  { "2880x1440fourpuni", N_("2880 x 1440 DPI Four Pass Unidirectional"),
    2880, 1440,  2880, 1440,  0,  3, 1, 1, 1, 1, 1, RES_2880_1440_M },

  { "", "", 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 }
};
