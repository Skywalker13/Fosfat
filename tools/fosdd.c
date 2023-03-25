/*
 * FOS fosdd: bidirectional tool to convert floppy (.DI) img to harddisk img
 * Copyright (C) 2023 Mathieu Schroeter <mathieu@schroetersa.ch>
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
#include "fosfat_internal.h"

#define HELP_TEXT \
"Tool to convert floppy image to disk image and vice-versa. fosfat-" LIBFOSFAT_VERSION_STR "\n\n" \
"Usage: fosdd [options] if of\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -l --fos-logger       that will turn on the FOS logger\n" \
"\n" \
"\nPlease, report bugs to <mathieu@schroetersa.ch>.\n"

#define VERSION_TEXT "fosdd-" LIBFOSFAT_VERSION_STR "\n"

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

/* Add blocks at the beginning according to the disk type. */
static int
dd (fosfat_t *fosfat, fosfat_disk_t type, const char *output_file)
{
  FILE *f_out = NULL;
  f_out = fopen (output_file, "wb");
  if (!f_out)
    return -1;

  /* The source is a floppy and the destination a disk */
  if (type == FOSFAT_FD)
  {
    /* Add padding of 0x10 */
    char buffer[FOSFAT_BLK] = {0};
    size_t read = 0;
    while (read < (0x10 * FOSFAT_BLK))
    {
      size_t to_write = (0x10 * FOSFAT_BLK) - read;
      if (to_write > FOSFAT_BLK)
        to_write = FOSFAT_BLK;
      fwrite (buffer, 1, to_write, f_out);
      read += to_write;
    }
  }

  for (int blk = -0x10;; ++blk)
  {
    fosfat_data_t *read_buffer = fosfat_read_d (fosfat, blk);
    if (!read_buffer)
      break;

    fwrite (read_buffer->data, 1, FOSFAT_BLK, f_out);
    free (read_buffer);
  }

  fclose (f_out);
  return 0;
}

int
main (int argc, char **argv)
{
  fosfat_t *fosfat; 
  int res = 0, next_option;
  fosfat_disk_t type = FOSFAT_AD;
  const char *input_file;
  const char *output_file;

  const char *const short_options = "hlv";

  const struct option long_options[] = {
    { "help",       no_argument, NULL, 'h' },
    { "fos-logger", no_argument, NULL, 'l' },
    { "version",    no_argument, NULL, 'v' },
    { NULL,         0,           NULL,  0  }
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
    case 'l':           /* -l or --fos-logger */
      fosfat_logger (1);
      break;
    case -1:            /* end */
      break ;
    }
  } while (next_option != -1);

  if (argc < optind + 2)
  {
    print_info ();
    return -1;
  }

  input_file  = argv[optind + 0];
  output_file = argv[optind + 1];

  fosfat = fosfat_open (input_file, FOSFAT_AD, 0);
  if (!fosfat)
  {
    fprintf (stderr, "Could not open %s!\n", input_file);
    return -1;
  }

  type = fosfat_type (fosfat);
  switch (type)
  {
  case FOSFAT_FD:
    printf ("Convert floppy \"%s\" into disk image at \"%s\"\n",
            input_file, output_file);
    res = dd (fosfat, type, output_file);
    break;

  case FOSFAT_HD:
    printf ("Convert disk \"%s\" into floppy image at \"%s\"\n",
            input_file, output_file);
    res = dd (fosfat, type, output_file);
    break;

  default:
    fprintf (stderr, "Input device / image not supported!\n");
    return -2;
  }

  fosfat_close (fosfat);

  if (res)
    fprintf (stderr, "Unexpected error when converting the device / image!\n");

  return res;
}
