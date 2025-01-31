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
#include <string.h>
#include <inttypes.h>

#include "fosfat.h"
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
typedef struct mosfat_f_s {
  int8_t   name[8];    /* ASCII file name               */
  uint8_t  ext[2];     /* ASCII file extension          */
  uint16_t bbloc;      /* Bloc where begin the file     */
  uint16_t ebloc;      /* End bloc of the file          */
  uint8_t  att;        /* File attributes               */
  uint8_t  open;       /* Open file counter             */
  uint8_t  valid;      /* Valid bytes in the last bloc  */
  uint16_t begin;      /* Memory position               */
  uint16_t start;      /* Start address                 */
  uint8_t  day;        /* Creation date (day)           */
  uint8_t  month;      /* Creation date (month)         */
  uint8_t  year;       /* Creation date (year)          */
} __attribute__ ((__packed__)) mosfat_f_t;

/*
 * Directory : 768 bytes
 */
typedef struct mosfat_dr_s {
  mosfat_f_t files[32];
} __attribute__ ((__packed__)) mosfat_dr_t;

/* Main mosfat structure */
struct mosfat_s {
  MOSFAT_DEV *dev;            /* file disk image or physical device    */
  int         isfile;         /* if it's a file                        */
};


/*
 * Translate a block number to an address.
 *
 * block        the block's number given by the disk
 * fosboot      offset in the MOS address
 * return the address of this block on the disk
 */
static inline uint16_t
blk2add (uint16_t block)
{
  return (block * MOSFAT_BLK);
}

static mosfat_dr_t *
mosfat_read_dr (mosfat_t *mosfat, uint16_t block)
{
  mosfat_dr_t *dr;
  int read = 0;

  if (!mosfat || !mosfat->dev)
    return NULL;

  /* Move the pointer on the block */
  if (fseek (mosfat->dev, blk2add (block), SEEK_SET))
    return NULL;

  dr = malloc (sizeof (mosfat_dr_t));
  if (!dr)
    return NULL;

  read = fread ((mosfat_dr_t *) dr,
                1, (size_t) sizeof (mosfat_dr_t), mosfat->dev)
         == (size_t) sizeof (mosfat_dr_t);
  if (read)
    return dr;

  free (dr);
  return NULL;
}

static inline int
hex2int (uint8_t hex)
{
  int high = (hex >> 4) * 10;
  int low = hex & 0x0F;
  return high + low;
}

mosfat_file_t *
mosfat_list_dr (mosfat_t *mosfat,
                 const char dir[MAX_SPLIT][MOSFAT_NAMELGT], int iterator, int nb, uint16_t block)
{
  const mosfat_dr_t *dr;
  mosfat_file_t *first = NULL;
  mosfat_file_t *file = NULL;

  if (!mosfat || !dir)
    return NULL;

  dr = mosfat_read_dr (mosfat, block);
  if (!dr)
    return NULL;

  /* Loop for all directories in the path */
  for (int i = iterator; dr && i < nb; i++)
    for (size_t j = 0; j < countof (dr->files); ++j)
    {
      char lname[9] = {0};
      char name[MOSFAT_NAMELGT] = {0};
      char *it;
      char ext[3] = {0};
      const mosfat_f_t *f = &dr->files[j];
      if (f->name[0] == '\0')
        continue;

      it = memchr (f->name, ' ', sizeof (f->name));
      if (!it)
        it = memchr (f->name, '\r', sizeof (f->name));
      if (!it)
        it = memchr (f->name, '\t', sizeof (f->name));
      if (!it)
        it = memchr (f->name, '/', sizeof (f->name));
      if (!it)
        it = memchr (f->name, '\0', sizeof (f->name));
      if (!it)
        it = memchr (f->name, '[', sizeof (f->name));
      if (!it)
        it = (char *) f->name + sizeof (f->name);
      memcpy (lname, f->name, it - (char *) f->name);

      ext[0] = f->ext[0];
      ext[1] = f->ext[1];
      snprintf (name, sizeof (name), "%s.%s", lname, ext);

      if (!strcasecmp (name, dir[i]))
        return mosfat_list_dr (mosfat, dir, i + 1, nb + 1, f->bbloc);

      if (i < nb - 1)
        continue;

      if (file)
      {
        file->next_file = calloc (1, sizeof (mosfat_file_t));
        file = file->next_file;
      }
      else
        file = calloc (1, sizeof (mosfat_file_t));
      // FIXME: file == NULL

      snprintf (file->name, sizeof (file->name), "%s", name);
      file->size = (f->ebloc - 1 - f->bbloc) * MOSFAT_BLK + f->valid;
      file->time.year  = hex2int (f->year);
      file->time.month = hex2int (f->month);
      file->time.day   = hex2int (f->day);
      file->att.isdir = ext[0] == 'D' && ext[1] == 'R';

      if (!first)
        first = file;
    }

  return first;
}

mosfat_file_t *
mosfat_list_dir (mosfat_t *mosfat, const char *location)
{
  int nb = 0;
  char *tmp, *path;
  char dir[MAX_SPLIT][MOSFAT_NAMELGT];

  if (!mosfat || !location)
    return NULL;

  /* If location is "" or "/" then it's the first block */
  //if (location[0] == '\0' || (location[0] == '/' && location[1] == '\0'))

  /* Split the path into a table */
  if ((tmp = strtok ((char *) location, "/")))
  {
    snprintf (dir[nb], sizeof (dir[nb]), "%s", tmp);
    while ((tmp = strtok (NULL, "/")) && nb < MAX_SPLIT - 1)
      snprintf (dir[++nb], sizeof (dir[nb]), "%s", tmp);
  }
  else
    snprintf (dir[nb], sizeof (dir[nb]), "%s", location);
  nb++;

  return mosfat_list_dr (mosfat, dir, 0, nb, 0);
}

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
