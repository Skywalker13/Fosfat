/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2025 Mathieu Schroeter <mathieu@schroetersa.ch>
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "mosfat.h"
#include "fosfat_internal.h"


#define MOSFAT_DEV            void

/*
 * File entry size : 24 bytes
 *
 * NAME uses space, \r, \t, /, \0 or [ as terminator.
 *
 * A bloc has 256 bytes.
 *
 * File attributes (bits): |_._.O.C.P.R.W|
 *  W = write protect
 *  R = read protect
 *  P = protected W and R argument (?)
 *  C = file is open for write
 *  O = file is open for read
 *
 * OPEN is 0 when the file is closed.
 *
 * :  8  :  8  :  8  :  8  :  8  :  8  :  8  :  8  :
 * :_____:_____:_____:_____:_____:_____:_____:_____:
 * |_____________________NAME______________________|
 * |____EXT____|___BBLOC___|___EBLOC___|_ATT_|_OPEN|
 * |VALID|___BEGIN___|___START___|_DAY_|MONTH|_YEAR|
 */
typedef struct mosfat_file_s {
  uint8_t  name[8];    /* ASCII file name               */
  uint8_t  ext[2];     /* ASCII file extension          */
  uint16_t bbloc;      /* Bloc where begin the file     */
  uint16_t ebloc;      /* End bloc of the file          */
  uint8_t  att;        /* File attributes               */
  uint8_t  open;       /* Open file counter             */
  uint8_t  valid;      /* Valid bytes in the last bloc  */
  uint16_t begin;      /* Memory position               */
  uint16_t start;      /* Start address                 */
  uint16_t day;        /* Date (day)                    */
  uint16_t month;      /* Date (month)                  */
  uint16_t year;       /* Date (year)                   */
} __attribute__ ((__packed__)) mosfat_file_t;

/*
 * Directory : 768 bytes
 */
typedef struct mosfat_directory_s {
  mosfat_file_t files[32];
} __attribute__ ((__packed__)) mosfat_directory_t;

/* Main mosfat structure */
struct mosfat_s {
  MOSFAT_DEV  *dev;            /* file disk image or physical device    */
  int          isfile;         /* if it's a file                        */
};


/*
 * Open the device.
 *
 * That hides the fopen processing.
 *
 * dev          the device name
 * return the device handle
 */
mosfat_t *
mosfat_open (const char *dev)
{
  mosfat_t *mosfat = NULL;

  if (!dev)
    return NULL;

  mosfat = calloc (1, sizeof (mosfat_t));
  if (!mosfat)
    return NULL;

  mosfat->isfile = 1;
  mosfat->dev = fopen (dev, "rb");
  if (!mosfat->dev)
    goto err_dev;

  foslog (FOSLOG_NOTICE, "mosfat is ready");

  return mosfat;

 err_dev:
  free (mosfat);
  return NULL;
}

/*
 * Close the device.
 *
 * That hides the fclose processing.
 *
 * mosfat       handle
 */
void
mosfat_close(mosfat_t *mosfat)
{
  if (!mosfat)
    return;

  foslog (FOSLOG_NOTICE, "device is closing ...");

  if (mosfat->dev)
    fclose (mosfat->dev);

  free (mosfat);
}
