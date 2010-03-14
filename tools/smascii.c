/*
 * FOS smascii: Smaky's ASCII to Extended ASCII (ISO-8859-1) converter
 * Copyright (C) 2007-2010 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ascii.h"

#define BUFFER_SIZE 256

#define HELP_TEXT \
"Tool for convert Smaky text file to " \
"Extended ASCII (ISO-8859-1).\n\n" \
"Usage: smascii smaky_file converted_file [--unix]\n\n" \
" smaky_file            the smaky text file\n" \
" converted_file        the file converted\n" \
" --unix                the Carriage Return (Old Mac) will be\n" \
"                       converted to Line Feed (unix)\n" \
"\nPlease, report bugs to <fosfat-devel@gamesover.ch>.\n"

/*
 * Run the text conversion.
 *
 * input        Smaky text file
 * output       ISO-8859-1 text file
 * return 1 for success and 0 for error
 */
static int
run_conv (const char *input, const char *output, newline_t newline)
{
  int res = -1;
  FILE *in = NULL, *out = NULL;
  char buffer[BUFFER_SIZE];
  size_t lng;

  in = fopen (input, "rb");
  if (!in)
  {
    fprintf (stderr, "Reading error!\n");
    goto out;
  }

  out = fopen (output, "wb");
  if (!out)
  {
    fprintf (stderr, "Writing error!\n");
    goto out;
  }

  while ((lng = fread (buffer, 1, sizeof (buffer), in)))
  {
    size_t size;
    if (fos_sma2iso8859 (buffer, (unsigned int) lng, newline))
    {
      size = fwrite (buffer, 1, lng, out);
      if (size != lng)
      {
        fprintf (stderr, "Conversion error, bad size!\n");
        goto out;
      }
    }
    else
    {
      fprintf (stderr, "Conversion error!\n");
      goto out;
    }
  }

  printf ("File %s successfully converted to %s!\n", input, output);

 out:
  if (in)
    fclose (in);
  if (out)
    fclose (out);

  return res;
}

static void
print_help (void)
{
  printf (HELP_TEXT);
}

int
main (int argc, char **argv)
{
  newline_t newline = ASCII_CR;

  if (argc < 3)
  {
    print_help ();
    return -1;
  }

  if (argc == 4 && !strcmp (argv[3], "--unix"))
    newline = ASCII_LF;

  return run_conv (argv[1], argv[2], newline);
}
