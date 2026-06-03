/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2025 Mathieu Schroeter <mathieu@schroetersa.ch>
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

#ifndef FOSFAT_UTILS_H
#define FOSFAT_UTILS_H

/**
 * \file utils.h
 *
 * tools utils header.
 */

/* Replace //a//b by /a/b */
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

#endif /* FOSFAT_UTILS_H */
