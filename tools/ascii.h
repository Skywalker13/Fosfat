/*
 * FOS ascii: Smaky's ASCII to Extended ASCII (ISO-8859-1) converter
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

#ifndef _ASCII_H_
#define _ASCII_H_

/** New line */
typedef enum newline {
  eCR = 0x0D,            //!< Carriage Return (Old Mac)
  eLF = 0x0A             //!< Line Feed       (Unix)
} e_newline;

char *sma2iso8859(char *buffer, unsigned int size, e_newline ret);

#endif /* _ASCII_H_ */
