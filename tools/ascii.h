/*
 * FOS ascii: Smaky's ASCII to Extended ASCII (ISO-8859-1) converter
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

#ifndef ASCII_H
#define ASCII_H

/** New line */
typedef enum newline {
  ASCII_CR = 0x0D,       /*!< Carriage Return (Old Mac)   */
  ASCII_LF = 0x0A        /*!< Line Feed       (Unix)      */
} e_newline;

char *sma2iso8859 (char *buffer, unsigned int size, e_newline ret);

#endif /* ASCII_H */
