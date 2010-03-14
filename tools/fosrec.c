/*
 * FOS fosrec: tool to undelete files on a Smaky file system
 * Copyright (C) 2008-2009 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud for his help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 * This file is part of Fosfat.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "fosfat.h"

#define HELP_TEXT \
"Tool to search deleted files on a Smaky disk. fosfat-" LIBFOSFAT_VERSION_STR "\n\n" \
"Usage: fosrec [options] device destination\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -a --harddisk         force an hard disk (default autodetect)\n" \
" -f --floppydisk       force a floppy disk (default autodetect)\n" \
" -l --fos-logger       that will turn on the FOS logger\n" \
"\n" \
"\nPlease, report bugs to <fosfat-devel@gamesover.ch>.\n"

#define VERSION_TEXT "fosrec-" LIBFOSFAT_VERSION_STR "\n"

static void
search_deleted (fosfat_t *fosfat, const char *location, const char *dest)
{
  fosfat_file_t *list, *list_first;
  char path[256], path2[256];

  if (!fosfat || !location || !dest)
    return;

  list = fosfat_list_dir (fosfat, location);
  if (strlen (location) == 1 && *location == '/')
    location++;

  list_first = list;

  for (; list; list = list->next_file)
  {
    if (strstr (list->name, ".dir"))
      *(list->name + strlen (list->name) - 4) = '\0';

    if (list->att.isdir)
    {
      snprintf (path, sizeof (path), "%s/%s", location, list->name);
      search_deleted (fosfat, path, dest);
    }
    else if (list->att.isdel)
    {
      int res;
      static unsigned int cnt = 1;

      snprintf (path, sizeof (path), "%s/%s", location, list->name);
      snprintf (path2, sizeof (path2), "%s/%08u_%s", dest, cnt, list->name);

      res = fosfat_get_file (fosfat, path, path2, 0);
      if (res)
      {
        printf ("File \"%s\" recovered...\n", path2);
        cnt++;
      }
    }
  }

  fosfat_free_listdir (list_first);
}

/* Print help. */
static void
print_info (void)
{
  printf (HELP_TEXT);
}

/* Print version. */
static void
print_version (void)
{
  printf (VERSION_TEXT);
}

int
main (int argc, char **argv)
{
  fosfat_t *fosfat;
  int res = 0, next_option;
  fosfat_disk_t type = FOSFAT_AD;

  const char *const short_options = "afhlv";

  const struct option long_options[] = {
    { "harddisk",     no_argument, NULL, 'a' },
    { "floppydisk",   no_argument, NULL, 'f' },
    { "help",         no_argument, NULL, 'h' },
    { "fos-logger",   no_argument, NULL, 'l' },
    { "version",      no_argument, NULL, 'v' },
    { NULL,           0,           NULL,  0  }
  };

  /* check options */
  do
  {
    next_option = getopt_long (argc, argv, short_options, long_options, NULL);
    switch (next_option)
    {
    default :           /* unknown */
    case '?':           /* invalid option */
    case 'h':           /* -h or --help */
      print_info ();
      return -1;
    case 'v':           /* -v or --version */
      print_version ();
      return -1;
    case 'a':           /* -a or --harddisk */
      type = FOSFAT_HD;
      break ;
    case 'f':           /* -f or --floppydisk */
      type = FOSFAT_FD;
      break ;
    case 'l':           /* -l or --fos-logger */
      fosfat_logger (1);
      break ;
    case -1:            /* end */
      break ;
    }
  } while (next_option != -1);

  if (argc < optind + 2)
  {
    print_info ();
    return -1;
  }

  fosfat = fosfat_open (argv[optind], type, F_UNDELETE);
  if (!fosfat)
  {
    fprintf (stderr, "Could not open %s to undelete files!\n", argv[optind]);
    return -1;
  }

  search_deleted (fosfat, "/", argv[optind + 1]);

  fosfat_close (fosfat);

  return res;
}
