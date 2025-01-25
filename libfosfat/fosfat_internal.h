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


fosfat_data_t *fosfat_read_d (fosfat_t *fosfat, uint32_t block);
void foslog (foslog_t type, const char *msg, ...);

#endif /* FOSFAT_H */
