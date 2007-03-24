/*
 * FOS smascii: Smaky's ASCII to Extended ASCII (ISO-8859-1) converter
 * Copyright (C) 2007 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for his help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>   /* strdup */

#include "ascii.h"

#define BUFFER_SIZE 256

/** Run the text conversion.
 * @param input Smaky text file
 * @param output ISO-8859-1 text file
 * @return 1 for success and 0 for error
 */
int run_conv(const char *input, const char *output) {
  FILE *in = NULL, *out = NULL;
  char buffer[BUFFER_SIZE];
  size_t lng;

  if ((in = fopen(input, "r")) && (out = fopen(output, "w"))) {
    while ((lng = fread((char *)buffer, (size_t)sizeof(char),
           (size_t)BUFFER_SIZE, in)))
    {
      if (sma2iso8859(buffer, (unsigned int)lng))
        fwrite((char *)buffer, (size_t)sizeof(char), lng, out);
      else {
        printf("Conversion error!\n");
        return 0;
      }
    }
    printf("File %s successfully converted to %s!\n", input, output);
    fclose(in);
    fclose(out);
  }
  else {
    printf("Reading or writing error!\n");
    return 0;
  }
  return 1;
}

/** Print help. */
void print_help(void) {
  printf("Usage: smascii smaky_file converted_file\n");
  printf("Tool for convert Smaky text file to ");
  printf("Extended ASCII (ISO-8859-1).\n\n");
  printf("smaky_file        the smaky text file\n\n");
  printf("converted_file    the file converted\n\n");
  printf("\nPlease, report bugs to <fosfat-devel@gamesover.ch>\n");
}

int main(int argc, char **argv) {
  char *input_file;
  char *output_file;

  if (argc == 3) {
    input_file = strdup(argv[1]);
    output_file = strdup(argv[2]);
    if (run_conv(input_file, output_file))
      return 0;

    if (input_file)
      free(input_file);
    if (output_file)
      free(output_file);
  }
  else
    print_help();
  return -1;
}
