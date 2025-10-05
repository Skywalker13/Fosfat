/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2023 Mathieu Schroeter <mathieu@schroetersa.ch>
 *
 * Thanks to Pierre Arnaud for his help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 * This file is part of Fosfat.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FOSFAT_INTERNAL_H
#define FOSFAT_INTERNAL_H

/**
 * \file fosfat_internal.h
 *
 * libfosfat internal API header.
 */

/* Block size (256 bytes) */
#define FOSFAT_BLK      256
#define MOSFAT_BLK      256

#define MAX_SPLIT             64

/* Data Block (256 bytes) */
typedef struct block_data_s {
  uint8_t data[256];           /* Data                                  */
  /* Linked list */
  struct  block_data_s *next_data;
} fosfat_data_t;

/* foslog type */
typedef enum foslog {
  FOSLOG_ERROR,                /* Error log                             */
  FOSLOG_WARNING,              /* Warning log                           */
  FOSLOG_NOTICE                /* Notice log                            */
} foslog_t;

#define countof(array) (sizeof (array) / sizeof (array[0]))


fosfat_data_t *fosfat_read_d (fosfat_t *fosfat, uint32_t block);
void foslog (foslog_t type, const char *msg, ...);

/*
 * Hex (BCD) to dec convertion.
 *
 * Convert an integer in base 10 with the value shown in base 16, in an
 * integer with the value shown in base 10.
 *
 * val          the value
 * return the new integer
 */
static inline int bcd2int (uint8_t hex)
{
  int high = (hex >> 4) * 10;
  int low = hex & 0x0F;
  int res = high + low;
  return res > 99 ? 0 : res;
}

static inline void remove_dup_slashes (char *s)
{
  char *r = s;
  char *w = s;
  while (*r)
  {
    *w++ = *r++;
    if (*(r-1) == '/' && *r == '/')
      while (*r == '/')
        r++;
  }
  *w = '\0';
}

#endif /* FOSFAT_H */
