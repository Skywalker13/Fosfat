/*
 * FOS smascii: Smaky's ASCII to Extended ASCII (ISO-8859-1) converter
 * Copyright (C) 2007-2008 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
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
#include <string.h>   /* strdup */

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

/**
 * \brief Run the text conversion.
 *
 * \param input  Smaky text file
 * \param output ISO-8859-1 text file
 * \return 1 for success and 0 for error
 */
int
run_conv (const char *input, const char *output, e_newline newline)
{
  int res = 1;
  FILE *in = NULL, *out = NULL;
  char buffer[BUFFER_SIZE];
  size_t lng;

  if ((in = fopen (input, "r")) && (out = fopen (output, "w")))
  {
    while ((lng = fread ((char *) buffer, 1, (size_t) BUFFER_SIZE, in)))
    {
      if (sma2iso8859 (buffer, (unsigned int) lng, newline))
        fwrite ((char *) buffer, 1, lng, out);
      else
      {
        fprintf (stderr, "Conversion error!\n");
        res = 0;
      }
    }
    if (res)
      printf ("File %s successfully converted to %s!\n", input, output);
  }
  else
  {
    fprintf (stderr, "Reading or writing error!\n");
    res = 0;
  }

  if (in)
    fclose (in);
  if (out)
    fclose (out);

  return res;
}

/** Print help. */
void
print_help (void)
{
  printf (HELP_TEXT);
}

int
main (int argc, char **argv)
{
  int res = 0;
  char *input_file;
  char *output_file;
  e_newline newline = ASCII_CR;

  if (argc >= 3)
  {
    input_file = strdup (argv[1]);
    output_file = strdup (argv[2]);

    if (argc == 4 && !strcmp (argv[3], "--unix"))
      newline = ASCII_LF;

    if (!run_conv (input_file, output_file, newline))
      res = -1;

    if (input_file)
      free (input_file);
    if (output_file)
      free (output_file);
  }
  else
    print_help ();

  return res;
}
