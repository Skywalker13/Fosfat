/*
 * FOS ascii: Smaky's ASCII to Extended ASCII (ISO-8859-1) and UTF-8 converter
 * Copyright (C) 2007-2010,2025 Mathieu Schroeter <mathieu@schroetersa.ch>
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

#include "fosfat.h"


/*
 * Smaky's ASCII table converter.
 * Index : Smaky
 * Value : ISO-8859-1
 */
static const unsigned char smaky2iso8859_charset[] = {
     0,    1,    2,    3,    4,    5,    6,    7,
     8,    9,   10,   11,   12,   13,   14, 0xFC,
  0xE0, 0xE2, 0xE9, 0xE8, 0xEB, 0xEA, 0xEF, 0xEE,
  0xF4, 0xF9, 0xFB, 0xE4, 0xF6, 0xE7, 0xAB, 0xBB,
    32,   33,   34,   35,   36,   37,   38,   39,
    40,   41,   42,   43,   44,   45,   46,   47,
    48,   49,   50,   51,   52,   53,   54,   55,
    56,   57,   58,   59,   60,   61,   62,   63,
    64,   65,   66,   67,   68,   69,   70,   71,
    72,   73,   74,   75,   76,   77,   78,   79,
    80,   81,   82,   83,   84,   85,   86,   87,
    88,   89,   90,   91,   92,   93,   94,   95,
    96,   97,   98,   99,  100,  101,  102,  103,
   104,  105,  106,  107,  108,  109,  110,  111,
   112,  113,  114,  115,  116,  117,  118,  119,
   120,  121,  122,  123,  124,  125,  126,  127
};


/*
 * Convert smaky char to ISO-8859-1 char.
 *
 * value        the char
 * newline      CR or LF
 * return the new char
 */
static inline unsigned char
char_sma2iso8859 (unsigned char value, fosfat_newline_t newline)
{
  return (value > 127 ? 0
          : (value == 13 ? newline : smaky2iso8859_charset[value]));
}

/*
 * Convert a buffer of chars to ISO-8859-1.
 *
 * buffer       pointer on the buffer
 * size         the length
 * newline      CR or LF
 * return the buffer
 */
char *
fosfat_sma2iso8859 (char *buffer, unsigned int size, fosfat_newline_t newline)
{
  unsigned int i = 0;

  if (buffer && size)
  {
    for (i = 0; i < size; i++)
      *(buffer + i) = char_sma2iso8859 (*(buffer + i), newline);
    return buffer;
  }

  return NULL;
}

/*
 * Convert ISO-8859-1 char to UTF-8.
 *
 * value        the ISO-8859-1 char
 * dest         destination buffer (must have at least 2 bytes available)
 * return number of bytes written (1 or 2)
 */
static inline int
char_iso8859_to_utf8 (unsigned char value, unsigned char *dest)
{
  if (value < 0x80) /* ASCII: 1 byte */
  {
    dest[0] = value;
    return 1;
  }

  /* Latin-1 extended: 2 bytes */
  dest[0] = 0xC0 | (value >> 6);
  dest[1] = 0x80 | (value & 0x3F);
  return 2;
}

/*
 * Convert a buffer of chars to UTF-8.
 * Maximum size needed: size * 2
 *
 * src          source buffer (Smaky encoding)
 * src_size     source buffer length
 * dest         destination buffer (UTF-8)
 * dest_size    destination buffer size
 * newline      CR or LF
 * return number of bytes written to dest, or 0 on error
 */
unsigned int
fosfat_sma2utf8 (const char *src, unsigned int src_size,
                 char *dest, unsigned int dest_size, fosfat_newline_t newline)
{
  unsigned int i;
  unsigned int dest_pos = 0;
  unsigned char iso_char;
  int bytes_written;

  if (!src || !dest || !src_size || !dest_size)
    return 0;

  for (i = 0; i < src_size; i++)
  {
    iso_char = char_sma2iso8859 ((unsigned char)src[i], newline);
    if (dest_pos + 2 > dest_size)
      return 0; /* Not enough space */

    bytes_written =
      char_iso8859_to_utf8 (iso_char, (unsigned char *) (dest + dest_pos));
    dest_pos += bytes_written;
  }

  return dest_pos;
}
