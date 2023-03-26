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
#include <unistd.h>

#include "fosfat.h"
#include "fosfat_internal.h"

#define HELP_TEXT \
"Tool to convert floppy image to disk image and vice-versa. fosfat-" LIBFOSFAT_VERSION_STR "\n\n" \
"Usage: fosdd [options] device destination\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -f --force            overwrite the destination if exists\n" \
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

  size_t offset = 0x10;

  /* The source is a floppy and the destination a disk */
  if (type == FOSFAT_FD)
  {
    /* Add padding of 0x10 (offset) */
    static const char zero[FOSFAT_BLK] = {0};
    /* Static boot entry for hard disk */
    static const char boot[FOSFAT_BLK] = {
      0x28, 0x3c, 0x80, 0x00, 0x00, 0x10, 0x7a, 0x10,
      0x70, 0x10, 0x28, 0x7c, 0x00, 0x02, 0x00, 0x00,
      0x4e, 0xb9, 0x00, 0x01, 0xe0, 0x08, 0x4a, 0x47,
      0x66, 0x00, 0x00, 0x08, 0x4e, 0xf9, 0x00, 0x02,
      0x00, 0x00, 0xa0, 0x42, 0x0d, 0x2a, 0x2a, 0x2a,
      0x2a, 0x2a, 0x20, 0x45, 0x72, 0x72, 0x65, 0x75,
      0x72, 0x20, 0x64, 0x65, 0x20, 0x6c, 0x65, 0x63,
      0x74, 0x75, 0x72, 0x65, 0x00, 0x00, 0x60, 0x00,
      0x00, 0x24, 0xa0, 0x42, 0x0d, 0x2a, 0x2a, 0x2a,
      0x2a, 0x2a, 0x20, 0x50, 0x72, 0x6f, 0x67, 0x72,
      0x61, 0x6d, 0x6d, 0x65, 0x20, 0x53, 0x54, 0x41,
      0x52, 0x54, 0x55, 0x50, 0x20, 0x61, 0x62, 0x73,
      0x65, 0x6e, 0x74, 0x00, 0xa0, 0x42, 0x2c, 0x20,
      0x64, 0x12, 0x6d, 0x61, 0x72, 0x72, 0x61, 0x67,
      0x65, 0x20, 0x69, 0x6d, 0x70, 0x6f, 0x73, 0x73,
      0x69, 0x62, 0x6c, 0x65, 0x0d, 0x0d, 0x00, 0x00,
      0x4e, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    size_t read = 0;
    while (read < (offset * FOSFAT_BLK))
    {
      size_t to_write = (offset * FOSFAT_BLK) - read;
      if (to_write > FOSFAT_BLK)
        to_write = FOSFAT_BLK;
      fwrite (read == 0x400 ? boot : zero, 1, to_write, f_out);
      read += to_write;
    }
  }

  for (int blk = -offset;; ++blk)
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
  int res = 0, force = 0, next_option;
  fosfat_disk_t type = FOSFAT_AD;
  const char *input_file;
  const char *output_file;

  const char *const short_options = "hflv";

  const struct option long_options[] = {
    { "help",       no_argument, NULL, 'h' },
    { "force",      no_argument, NULL, 'f' },
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
    case 'f':
      force = 1;
      break;
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

  if (!force && !access (output_file, F_OK)) {
    fprintf (stderr,
             "The output file \"%s\" already exists, use --force to overwrite\n",
             output_file);
    return -1;
  }

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
