/*
 * Extract translation strings
 *
 * Copyright 2008 by Robert Krawitz.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <gutenprint/gutenprint.h>
#include <gutenprint/mxml.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int
main(int argc, char **argv)
{
  int status = 0;
  argv++;
  printf("/*\n");
  printf(" * TRANSLATORS:\n");
  printf(" * All strings in this file must be no more than 80 bytes in length.\n");
  printf(" * Please take into account expansion due to multi-byte characters,\n");
  printf(" * as the limit is *bytes*, not characters!\n");
  printf(" */\n");
  while (*argv)
    {
      stp_mxml_node_t *top =
	stp_mxmlLoadFromFile(NULL, *argv, STP_MXML_NO_CALLBACK);
      if (top)
	{
	  stp_mxml_node_t *n = top;
	  stp_string_list_t *sl = stp_string_list_create();
	  do
	    {
	      const char *attr = stp_mxmlElementGetAttr(n, "translate");
	      if (attr)
		{
		  const char *str = stp_mxmlElementGetAttr(n, attr);
		  if (! stp_string_list_is_present(sl, str))
		    {
		      char *s;
		      stp_string_list_add_string_unsafe(sl, str, str);
		      stp_asprintf(&s, "N_(\"%s\");", str);
		      printf("%-40s /* %s */\n", s, *argv);
		      stp_free(s);
		    }
		}
	      n = stp_mxmlWalkNext(n, top, STP_MXML_DESCEND);
	    } while (n);
	  stp_string_list_destroy(sl);
	  stp_mxmlDelete(top);
	}
      else
	{
	  fprintf(stderr, "Cannot read %s: %s\n", *argv, strerror(errno));
	  status = 1;
	}
      argv++;
    }
  return status;
}
