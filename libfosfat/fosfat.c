/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2006-2008 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <ctype.h>      /* tolower */
#include <string.h>     /* strcasecmp strncasecmp strdup strlen strtok
                           memcmp memcpy strcasestr */

#ifdef _WIN32
#include <w32disk.h>
#endif

#include "fosfat.h"

#define MAX_SPLIT       64

/* Block size (256 bytes) */
#define FOSFAT_BLK      256

#define FOSFAT_NBL      4
#define FOSFAT_Y2K      70

#define FOSFAT_BLOCK0   0x00
#define FOSFAT_SYSLIST  0x01

#define FOSBOOT_FD      0x10
#define FOSBOOT_HD      0x20

#define FOSFAT_DEV      FILE

/* FOS attributes and type */
#define FOSFAT_ATT_OPENEX     0x1
#define FOSFAT_ATT_MULTIPLE   0x2
#define FOSFAT_ATT_DIR        0x1000
#define FOSFAT_ATT_VISIBLE    0x2000
#define FOSFAT_ATT_ENCODED    0x20000
#define FOSFAT_ATT_LINK       0x1000000
#define FOSFAT_TYPE_SYSTEM    0xF8

/** List of all block types */
typedef enum block_type {
  eB0,                         //!< Block 0
  eBL,                         //!< Block List
  eBD,                         //!< Block Description
  eDATA                        //!< Only DATA
} fosfat_type_t;

/** Search type */
typedef enum search_type {
  eSBD,                        //!< Search BD
  eSBLF                        //!< Search BL File
} fosfat_search_t;

/** foslog type */
typedef enum foslog {
  eERROR,                      //!< Error log
  eWARNING,                    //!< Warning log
  eNOTICE                      //!< Notice log
} foslog_t;

/** Data Block (256 bytes) */
typedef struct block_data_s {
  uint8_t data[256];           //!< Data
  /* Linked list */
  struct block_data_s *next_data;
} fosfat_data_t;

/** Block 0 (256 bytes) */
typedef struct block_0_s {
  uint8_t sys[44];             //!< SYSTEM folder
  int8_t nlo[16];              //!< Disk name
  uint8_t chk[4];              //!< Check control
  uint8_t mes[172];            //!< Message
  uint8_t change;              //!< Need of change the CHK number
  uint8_t bonchk;              //!< New format
  uint8_t oldchk[4];           //!< Old CHK value
  uint8_t newchk[4];           //!< New CHK value
  uint8_t reserve[10];         //!< Unused
} fosfat_b0_t;

/** File in a Block List (60 bytes) */
typedef struct block_listf_s {
  int8_t name[16];             //!< Filename
  uint8_t typ;                 //!< Filetype
  uint8_t ope;                 //!< Open counter
  uint8_t att[4];              //!< Attributes
  uint8_t lg[4];               //!< Length in blocks
  uint8_t lgb[2];              //!< Number of bytes in the last block
  uint8_t cd[3];               //!< Date of Creation
  uint8_t ch[3];               //!< Hour of Creation
  uint8_t wd[3];               //!< Date of the last Write
  uint8_t wh[3];               //!< Hour of the last Write
  uint8_t rd[3];               //!< Date of the last Read
  uint8_t rh[3];               //!< Hour of the last Read
  uint8_t secid[3];            //!< Security ID
  uint8_t clope;               //!< Open mode before CLOSE
  uint8_t pt[4];               //!< Pointer on the BD
  uint8_t lgf[4];              //!< File size in bytes
  uint8_t code[2];             //!< Code control
} fosfat_blf_t;

/** Block List (256 bytes) */
typedef struct block_list_s {
  fosfat_blf_t file[4];        //!< 4 BL files (240 bytes)
  uint8_t next[4];             //!< Next BL
  uint8_t chk[4];              //!< Check control
  uint8_t prev[4];             //!< Previous BL
  uint8_t reserve[4];          //!< Unused
  /* Not in the block */
  uint32_t pt;                 //!< Block's number of this BL
  /* Linked list */
  struct block_list_s *next_bl;
} fosfat_bl_t;

/** Block Description (256 bytes) */
typedef struct block_desc_s {
  uint8_t next[4];             //!< Next BD
  uint8_t prev[4];             //!< Previous BD
  uint8_t npt[2];              //!< Number of tranches in the BD
  uint8_t pts[42][4];          //!< Pointers on the tranches (max 42 tranches)
  int8_t name[16];             //!< Filename
  uint8_t nbs[42];             //!< Length (in blocks) of each tranches
  uint8_t reserve[4];          //!< Unused
  uint8_t lst[2];              //!< Number of byte in the last tranche
  uint8_t hac[2];              //!< Hashing function if LIST
  uint8_t nbl[2];              //!< Number of BL used if LIST
  uint8_t chk[4];              //!< Check control
  uint8_t off[4];              //!< Offset (in blocks) of all previous BD
  uint8_t free[2];             //!< Unused
  /* Linked list */
  struct block_desc_s *next_bd;
  struct block_list_s *first_bl;
} fosfat_bd_t;

/** Cache list for name, BD and BL blocks */
typedef struct cache_list_s {
  char *name;
  uint32_t bl;                 //!< BL Address
  uint32_t bd;                 //!< BD Address
  int isdir;                   //!< If is a directory
  int islink;                  //!< If is a soft link
  int isdel;                   //!< If is deleted
  /* Linked list */
  struct cache_list_s *sub;
  struct cache_list_s *next;
} cachelist_t;

/** Main fosfat structure */
struct fosfat_s {
#ifdef _WIN32
  win32disk_t *dev;            //!< device object
#else
  FOSFAT_DEV *dev;             //!< physical device
#endif
  int fosboot;                 //!< FOSBOOT address
  uint32_t foschk;             //!< CHK
  unsigned int cache;          //!< use cache system (search)
  int viewdel;                 //!< list deleted files
  cachelist_t *cachelist;      //!< cache data
};


/** Global variable for internal logger */
static int g_logger = 0;


#ifdef __linux__
/**
 * \brief Translate a block number to an address.
 *
 *  This function depend if the media is an HARD DISK or an 3"1/2 DISK.
 *  The FOSBOOT is the double in number of blocks for an hard disk.
 *  The real address is the address given by the disk + the size of the
 *  FOSBOOT part.
 *
 * \param block   the block's number given by the disk
 * \param fosboot offset in the FOS address
 * \return the address of this block on the disk
 */
static inline uint32_t
blk2add (uint32_t block, int fosboot)
{
  return ((block + fosboot) * FOSFAT_BLK);
}
#endif

#ifdef _WIN32
/**
 * \brief Translate a block number to a sector.
 *
 *  This function depend if the media is an HARD DISK or an 3"1/2 DISK.
 *  The FOSBOOT is the double in number of blocks for an hard disk.
 *  The real address is the sector given by the disk with the size of the
 *  FOSBOOT part.
 *
 * \param block   the block's number given by the disk
 * \param fosboot offset in the FOS address
 * \return the sector on the disk
 */
static inline uint32_t
blk2sector (uint32_t block, int fosboot)
{
  // FIXME: div by 2 only right if the sector size is 512
  return ((block + fosboot) / 2);
}

/**
 * \brief Return the offset in the block for get the right Smaky's block.
 *
 *  Smaky's blocks are of 256 bytes and Win32 uses 512. This function search
 *  if an offset of 256 is necessary for get the right Smaky's block, or not.
 *
 * \param block   the block's number given by the disk
 * \param fosboot offset in the FOS address
 * \return the offset in the Win32's block
 */
static inline uint32_t
sec_offset (uint32_t block, int fosboot)
{
  // FIXME: modulo by 2 only right if the sector size is 512
  return (((block + fosboot) % 2) ? FOSFAT_BLK : 0);
}
#endif

/**
 * \brief Convert char table to an integer.
 *
 *  This convertion will swapped all bytes and returned an int value.
 *
 * \param value pointer on the char table
 * \param size  size of the table (number of bytes)
 * \return the integer value
 */
static uint32_t
c2l (uint8_t *value, int size)
{
  int i, j;
  uint32_t res = 0;

  if (value) {
    for (i = size - 1, j = 0; i >= 0; i--, j++)
      res += (*(value + j) << i * 8);
  }
  return res;
}

/**
 * \brief Change a string in lower case.
 *
 * \param data the string
 * \return a pointer on this string
 */
static char *
lc (char *data)
{
  int i;

  if (data) {
    for (i = 0; data[i] != '\0'; i++)
      data[i] = tolower (data[i]);
  }

  return data;
}

/**
 * \brief Hex to dec convertion.
 *
 *  Convert an integer in base 10 with the value shown in base 16, in an
 *  integer with the value shown in base 10.
 *
 * \param val the value
 * \return the new integer
 */
static int
h2d (int val)
{
  char *conv;
  int res = 0;

  if ((conv = malloc (sizeof (val)))) {
    snprintf (conv, sizeof (conv), "%X", val);
    res = atoi (conv);
    free (conv);
  }

  return res;
}

/**
 * \brief Convert the year to y2k.
 *
 * \param y year on two digits
 * \return the year on four digits
 */
static inline int
y2k (int y)
{
  return ((y < FOSFAT_Y2K) ? 2000 : 1900) + y;
}

/**
 * \brief Like strchr but limited to a length.
 *
 * \param s     string
 * \param count number of chars
 * \param c     char searched
 * \return the pointer on the char or NULL if not found
 */
static char *
my_strnchr (const char *s, size_t count, int c)
{
  size_t i;

  for (i = 0; i < count; i++) {
    if (*(s + i) == (const char) c)
      return (char *) (s + i);
  }

  return NULL;
}

/**
 * \brief Enable or disable the internal FOS logger.
 *
 * \param state 1 or 0
 */
void
fosfat_logger (int state)
{
  if (state)
    g_logger = 1;
  else
    g_logger = 0;
}

/**
 * \brief Print function for the internal FOS logger.
 *
 * \param type of logging
 * \param msg  text shown
 * \param ...  variables
 */
static void
foslog (foslog_t type, const char *msg, ...)
{
  va_list va;
  char log[256] = "fosfat-";

  va_start (va, msg);

  if (msg) {
    switch (type) {
    case eERROR:
      strcat (log, "error");
      break;

    case eWARNING:
      strcat (log, "warning");
      break;

    case eNOTICE:
      strcat (log, "notice");
    }

    fprintf (stderr, "%s: ", log);
    vfprintf (stderr, msg, va);
    fprintf (stderr, "\n");
  }
  va_end (va);
}

/**
 * \brief Free a DATA file variable.
 *
 *  It will be used only when a data is loaded for copy a file on the PC,
 *  for freed each block after each write.
 *
 * \param var pointer on the data block
 */
static void
fosfat_free_data (fosfat_data_t *var)
{
  fosfat_data_t *d, *free_d;

  d = var;
  while (d) {
    free_d = d;
    d = d->next_data;
    free (free_d);
  }
}

/**
 * \brief Free a BD file variable.
 *
 *  It must be used only with a BD that is (or can be) use as the first
 *  member of a linked list.
 *  This function must be used after all fosfat_read_file()!
 *
 * \param var pointer on the description block
 */
static void
fosfat_free_file (fosfat_bd_t *var)
{
  fosfat_bd_t *bd, *free_bd;

  bd = var;
  while (bd) {
    free_bd = bd;
    bd = bd->next_bd;
    free (free_bd);
  }
}

/**
 * \brief Free a BD dir variable.
 *
 *  It must be used only with a BD that is (or can be) use as the first
 *  member of a linked list for a dir (with BL linked list into).
 *  This function must be used after all fosfat_read_dir()!
 *
 * \param var pointer on the description block
 */
static void
fosfat_free_dir (fosfat_bd_t *var)
{
  fosfat_bl_t *bl, *free_bl;
  fosfat_bd_t *bd, *free_bd;

  if (!var)
    return;
  bd = var;

  do {
    bl = bd->first_bl;

    /* Freed all BL */
    while (bl) {
      free_bl = bl;
      bl = bl->next_bl;
      free (free_bl);
    }

    /* And after, freed the BD */
    free_bd = bd;
    bd = bd->next_bd;
    if (free_bd)
      free (free_bd);
  } while (bd);
}

/**
 * \brief Free a List Dir variable.
 *
 *  This function must be used after all fosfat_list_dir()!
 *
 * \param var pointer on the description block
 */
void
fosfat_free_listdir (fosfat_file_t *var)
{
  fosfat_file_t *ld, *free_ld;

  ld = var;
  while (ld) {
    free_ld = ld;
    ld = ld->next_file;
    free (free_ld);
  }
}

/**
 * \brief Test if the file is a directory.
 *
 *  This function read the ATT and return the value of the 12th bit,
 *  mask 0x1000.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_isdir (fosfat_blf_t *file)
{
  return (file ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_DIR) : 0);
}

/**
 * \brief Test if the file is a soft-link.
 *
 *  This function read the ATT and return the value of the 24th bit,
 *  mask 0x1000000.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_islink (fosfat_blf_t *file)
{
  return (file ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_LINK) : 0);
}

/**
 * \brief Test if the file is visible.
 *
 *  This function read the ATT and return the value of the 13th bit,
 *  mask 0x2000.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_isvisible (fosfat_blf_t *file)
{
  return (file
          ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_VISIBLE)
          : 0);
}

/**
 * \brief Test if the file is 'open exclusif' and 'multiple'.
 *
 *  This function read the ATT and return the value of the 1st or 2th bit,
 *  mask 0x1 and 0x2.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_isopenexm (fosfat_blf_t *file)
{
  return (file
          ? (((c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_OPENEX))
             || ((c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_MULTIPLE)))
          : 0);
}

/**
 * \brief Test if the file is encoded.
 *
 *  This function read the ATT and return the value of the 17th bit,
 *  mask 0x20000.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_isencoded (fosfat_blf_t *file)
{
  return (file
          ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_ENCODED)
          : 0);
}

/**
 * \brief Test if the file is system (5 MSB bits).
 *
 *  This function read the TYP and return the value of the 3-7th bits,
 *  mask 0xF8.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_issystem (fosfat_blf_t *file)
{
  return (file ? ((file->typ & FOSFAT_TYPE_SYSTEM)) : 0);
}

/**
 * \brief Test if the file is not deleted.
 *
 * \param file pointer on the file (in the BL)
 * \return a boolean (true for success)
 */
static inline int
fosfat_in_isnotdel (fosfat_blf_t *file)
{
  return (file && strlen ((char *) file->name) > 0) ? 1 : 0;
}

/**
 * \brief Read a block defined by a type.
 *
 *  This function read a block on the disk and return the structure in
 *  function of the type chosen. Each type use 256 bytes, but the structures
 *  are always bigger. The order of the attributes in each structures is
 *  very important, because the informations from the device are just copied
 *  directly without parsing.
 *
 * \param fosfat the main structure
 * \param block  block position
 * \param type   type of this block (eB0, eBL, eBD or eDATA)
 * \return a pointer on the new block or NULL if broken
 */
static void *
fosfat_read_b (fosfat_t *fosfat, uint32_t block, fosfat_type_t type)
{
#ifdef _WIN32
  size_t ssize, csector;
  int8_t *buffer;

  /* sector seems to be always 512 with Window$ */
  ssize = w32disk_sectorsize (fosfat->dev);

  if ((csector = (size_t) FOSFAT_BLK / ssize) == 0)
    csector = 1;

  buffer = malloc (csector * ssize);

  if (fosfat && fosfat->dev && buffer) {
    memset (buffer, 0, csector * ssize);
#else
  /* Move the pointer on the block */
  if (fosfat && fosfat->dev
      && !fseek (fosfat->dev, blk2add (block, fosfat->fosboot), SEEK_SET))
  {
#endif
    switch (type) {
    case eB0: {
      fosfat_b0_t *blk;

      if ((blk = malloc (sizeof (fosfat_b0_t)))) {
#ifdef _WIN32
        if (w32disk_readsectors (fosfat->dev, buffer,
                                 blk2sector (block, fosfat->fosboot), csector))
        {
          memcpy (blk, buffer + sec_offset (block, fosfat->fosboot),
                  (size_t) FOSFAT_BLK);
          free (buffer);
#else
        if (fread ((fosfat_b0_t *) blk, 1, (size_t) FOSFAT_BLK, fosfat->dev)
            == (size_t) FOSFAT_BLK)
        {
#endif
          return (fosfat_b0_t *) blk;
        }
        else
          free (blk);
      }
      break;
    }

    case eBL: {
      fosfat_bl_t *blk;

      if ((blk = malloc (sizeof (fosfat_bl_t)))) {
#ifdef _WIN32
        if (w32disk_readsectors (fosfat->dev, buffer,
                                 blk2sector (block, fosfat->fosboot), csector))
        {
          memcpy (blk, buffer + sec_offset (block, fosfat->fosboot),
                  (size_t) FOSFAT_BLK);
          free (buffer);
#else
        if (fread ((fosfat_bl_t *) blk, 1, (size_t) FOSFAT_BLK, fosfat->dev)
            == (size_t)FOSFAT_BLK)
        {
#endif
          blk->next_bl = NULL;

          /* Check the CHK value */
          if (!fosfat->foschk)
            fosfat->foschk = c2l (blk->chk, sizeof (blk->chk));
          if (fosfat->foschk == c2l (blk->chk, sizeof (blk->chk)))
            return (fosfat_bl_t *) blk;

          if (g_logger)
            foslog (eERROR, "bad FOSCHK for this BL (block:%li)", block);
        }
        free (blk);
      }
      break;
    }

    case eBD: {
      fosfat_bd_t *blk;

      if ((blk = malloc (sizeof (fosfat_bd_t)))) {
#ifdef _WIN32
        if (w32disk_readsectors (fosfat->dev, buffer,
                                 blk2sector (block, fosfat->fosboot), csector))
        {
          memcpy (blk, buffer + sec_offset (block, fosfat->fosboot),
                  (size_t) FOSFAT_BLK);
          free (buffer);
#else
        if (fread ((fosfat_bd_t *) blk, 1, (size_t) FOSFAT_BLK, fosfat->dev)
            == (size_t) FOSFAT_BLK)
        {
#endif
          blk->next_bd = NULL;
          blk->first_bl = NULL;

          /* Check the CHK value */
          if (!fosfat->foschk)
            fosfat->foschk = c2l (blk->chk, sizeof (blk->chk));
          if (fosfat->foschk == c2l (blk->chk, sizeof (blk->chk)))
            return (fosfat_bd_t *) blk;

          if (g_logger)
            foslog (eERROR, "bad FOSCHK for this BD (block:%li)", block);
        }
        free (blk);
      }
      break;
    }

    case eDATA: {
      fosfat_data_t *blk;

      if ((blk = malloc (sizeof (fosfat_data_t)))) {
#ifdef _WIN32
        if (w32disk_readsectors (fosfat->dev, buffer,
                                 blk2sector (block, fosfat->fosboot), csector))
        {
          memcpy (blk, buffer + sec_offset (block, fosfat->fosboot),
                  (size_t) FOSFAT_BLK);
          free (buffer);
#else
        if (fread ((fosfat_data_t *) blk, 1, (size_t) FOSFAT_BLK, fosfat->dev)
            == (size_t) FOSFAT_BLK)
        {
#endif
          blk->next_data = NULL;
          return (fosfat_data_t *) blk;
        }
        else
          free (blk);
      }
    }
    }
  }

#ifdef _WIN32
  if (buffer)
    free (buffer);
#endif

  return NULL;
}

/**
 * \brief Read the first useful block (0).
 *
 *  This block contents some informations on the disk. But no information
 *  are critical for read the file list.
 *
 * \param fosfat the main structure
 * \param block  block position
 * \return the block0 or NULL if broken
 */
static inline fosfat_b0_t *
fosfat_read_b0 (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_b0_t *) fosfat_read_b (fosfat, block, eB0))
          : NULL);
}

/**
 * \brief Read data block.
 *
 *  This block contents only a char table of 256 bytes for the raw data.
 *
 * \param fosfat the main structure
 * \param block  block position
 * \return the data or NULL if broken
 */
static inline fosfat_data_t *
fosfat_read_d (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_data_t *) fosfat_read_b (fosfat, block, eDATA))
          : NULL);
}

/**
 * \brief Read a Description Block (BD).
 *
 * \param fosfat the main structure
 * \param block  block position
 * \return the BD or NULL if broken
 */
static inline fosfat_bd_t *
fosfat_read_bd (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_bd_t *) fosfat_read_b (fosfat, block, eBD))
          : NULL);
}

/**
 * \brief Read a Block List (BL).
 *
 * \param fosfat the main structure
 * \param block  block position
 * \return the BL or NULL if broken
 */
static inline fosfat_bl_t *
fosfat_read_bl (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_bl_t *) fosfat_read_b (fosfat, block, eBL))
          : NULL);
}

/**
 * \brief Read data of some blocks and create linked list if necessary.
 *
 *  When a directory or a file is read, the content is shown as data. But
 *  for a directory, really it is a BL. This function create the linked list
 *  for a DATA block or a BL for a number of consecutive blocks.
 *
 * \param fosfat the main structure
 * \param block  the first block (start) for the linked list
 * \param nbs    number of consecutive blocks
 * \param type   type of this block (eB0, eBL, eBD or eDATA)
 * \return the first block of the linked list created
 */
static void *
fosfat_read_data (fosfat_t *fosfat, uint32_t block,
                  uint8_t nbs, fosfat_type_t type)
{
  if (!fosfat)
    return NULL;

  switch (type) {
  case eBL: {
    int i;
    fosfat_bl_t *block_list, *first_bl;

    first_bl = fosfat_read_bl (fosfat, block);
    if (!first_bl)
      break;

    block_list = first_bl;
    block_list->pt = block;

    for (i = 1; block_list && i < nbs; i++) {
      block_list->next_bl = fosfat_read_bl (fosfat, block + (uint32_t) i);
      block_list = block_list->next_bl;

      if (block_list)
        block_list->pt = block + (uint32_t) i;
    }
    return (fosfat_bl_t *) first_bl;
  }

  case eDATA: {
    int i;
    fosfat_data_t *block_data, *first_data;

    first_data = fosfat_read_d (fosfat, block);
    if (!first_data)
      break;

    block_data = first_data;

    for (i = 1; block_data && i < nbs; i++) {
      block_data->next_data = fosfat_read_d (fosfat, block + (uint32_t) i);
      block_data = block_data->next_data;
    }
    return (fosfat_data_t *) first_data;
  }

  /* Only for no compilation warning because
   * all types are not in the switch
   */
  default:
    break;
  }

  return NULL;
}

/**
 * \brief Read a file description.
 *
 *  A linked list is created for found all BD. And each BD have a linked list
 *  for found all DATA.
 *
 * \param fosfat the main structure
 * \param block  the file BD position
 * \return the first BD of the linked list
 */
static fosfat_bd_t *
fosfat_read_file (fosfat_t *fosfat, uint32_t block)
{
  uint32_t next;
  fosfat_bd_t *file_desc, *first_bd;

  if (!fosfat)
    return NULL;

  file_desc = fosfat_read_bd (fosfat, block);
  if (!file_desc)
    return NULL;

  file_desc->next_bd = NULL;
  file_desc->first_bl = NULL;     // Useless in this case
  first_bd = file_desc;

  /* Go to the next BD if exists (create the linked list for BD) */
  while (file_desc && file_desc->next
         && (next = c2l (file_desc->next, sizeof (file_desc->next))))
  {
    file_desc->next_bd = fosfat_read_bd (fosfat, next);
    file_desc = file_desc->next_bd;
  }

  /* End of the BD linked list */
  if (file_desc)
    file_desc->next_bd = NULL;

  return first_bd;
}

/**
 * \brief Get a file and put this in a location on the PC or in a buffer.
 *
 *  This function read all BD->DATA of a file BD, and write the data in a
 *  new file on your disk. An output variable can be used for that the current
 *  size is printed for each PTS. The properties like "Creation Date" are not
 *  saved in the new file. All Linux file system are the same attributes for
 *  them files. And for example, ext2/3 have no "Creation Date".
 *  The data can be saved in a buffer if the flag is used with an offset,
 *  size and a buffer.
 *
 * \param fosfat the main structure
 * \param file   file description block
 * \param dst    destination on your PC
 * \param output TRUE for print the size
 * \param flag   use the optional arguments or not
 * \param ...    offset, size and buffer
 * \return a boolean (true for success)
 */
static int
fosfat_get(fosfat_t *fosfat, fosfat_bd_t *file,
           const char *dst, int output, int flag, ...)
{
  /* optional arguments */
  va_list pp;
  va_start (pp, flag);
  int op_offset = 0;
  int op_size = 0;
  char *op_buffer = NULL;
  int op_inoff = 0;
  if (flag) {
    op_offset = va_arg (pp, int);
    op_size = va_arg (pp, int);
    op_buffer = va_arg (pp, char *);
  }
  va_end(pp);

  unsigned int i;
  int res = 1;
  size_t check_last;
  size_t size = 0;
  FILE *f_dst = NULL;
  fosfat_data_t *file_d, *first_d;

  if (!fosfat || !file)
    return 0;

  if (!flag) {
    f_dst = fopen (dst, "w");
    if (!f_dst)
      return 0;
  }

  /* Loop for all BD */
  do {
    /* Loop for all pointers */
    for (i = 0; res && i < c2l (file->npt, sizeof (file->npt)); i++) {
      if ((file_d = fosfat_read_data (fosfat, c2l (file->pts[i],
          sizeof (file->pts[i])), file->nbs[i], eDATA)))
      {
        first_d = file_d;

        /* Loop for all data blocks */
        do {
          check_last = (i == c2l (file->npt, sizeof (file->npt)) - 1
                        && !file_d->next_data)
                        ? (size_t) c2l (file->lst, sizeof (file->lst))
                        : (size_t) FOSFAT_BLK;

          /* When the result is written in a file */
          if (!flag) {
            /* Write the block */
            if (fwrite ((uint8_t *) file_d->data, 1, check_last, f_dst)
                != check_last)
              res = 0;
          }
          /* When the result is written in RAM (offset and size) */
          else if (op_inoff || ((unsigned) op_offset >= size &&
                                (unsigned) op_offset < size + check_last))
          {
            int first_pts = op_offset + op_inoff - (signed) size;
            int cp = (op_size - op_inoff > (signed) check_last - first_pts)
                     ? ((signed) check_last - first_pts)
                     : (op_size - op_inoff);

            /* Copy the tranche */
            memcpy (op_buffer + op_inoff, file_d->data + first_pts, cp);
            op_inoff += check_last - first_pts;

            if (op_size <= op_inoff)
              res = 0;
          }
          size += check_last;
        } while (res && file_d->next_data && (file_d = file_d->next_data));

        /* Freed all data */
        fosfat_free_data (first_d);
        if (res && output)
          fprintf (stdout, " %i bytes\n", (int) size);
      }
      else
        res = 0;
    }
  } while (res && file->next_bd && (file = file->next_bd));

  if (!flag) {
    fclose (f_dst);

    /* If fails then remove the incomplete file */
    if (!res)
      remove (dst);
  }

  return res;
}

/**
 * \brief Read a complete .DIR (or SYS_LIST).
 *
 *  A linked list is created for found all files. The first BD is returned.
 *  You can use this function for read all SYS_LIST, even in each folder. But
 *  the result will be always the same (recursive). Warning for not to do an
 *  infinite loop /!\
 *
 * \param fosfat the main structure
 * \param block  DIR (or SYS_LIST) BD position
 * \return the first BD of the linked list
 */
static fosfat_bd_t *
fosfat_read_dir (fosfat_t *fosfat, uint32_t block)
{
  unsigned int i;
  uint32_t next;
  fosfat_bd_t *dir_desc, *first_bd;
  fosfat_bl_t *dir_list;

  if (!fosfat)
    return NULL;

  dir_desc = fosfat_read_bd (fosfat, block);
  if (!dir_desc) {
    if (g_logger)
      foslog (eERROR, "directory to block %i cannot be read", block);
    return NULL;
  }

  dir_desc->next_bd = NULL;
  dir_desc->first_bl = NULL;
  first_bd = dir_desc;

  do {
    /* Get the first pointer */
    dir_desc->first_bl = fosfat_read_data (fosfat, c2l(dir_desc->pts[0],
                         sizeof (dir_desc->pts[0])), dir_desc->nbs[0], eBL);
    dir_list = dir_desc->first_bl;

    /* Go to the last BL */
    while (dir_list && dir_list->next_bl)
      dir_list = dir_list->next_bl;

    /* Loop all others pointers */
    for (i = 1; dir_list && i < c2l (dir_desc->npt,
                                     sizeof (dir_desc->npt)); i++)
    {
      dir_list->next_bl = fosfat_read_data (fosfat, c2l (dir_desc->pts[i],
                          sizeof (dir_desc->pts[i])), dir_desc->nbs[i], eBL);
      dir_list = dir_list->next_bl;

      /* Go to the last BL */
      while (dir_list && dir_list->next_bl)
        dir_list = dir_list->next_bl;
    }

    /* End of the BL linked list */
    if (dir_list)
      dir_list->next_bl = NULL;
  /* Go to the next BD if exists (create the linked list for BD) */
  } while ((next = c2l (dir_desc->next, sizeof (dir_desc->next)))
           && (dir_desc->next_bd = fosfat_read_bd (fosfat, next))
           && (dir_desc = dir_desc->next_bd));

  /* End of the BD linked list */
  dir_desc->next_bd = NULL;

  return first_bd;
}

/**
 * \brief Test if two names are the same or not.
 *
 *  This function must be used only for particular purpose, because that will
 *  no test if it is a directory or not, but only the names. That is very
 *  useful for fosfat_search_bdlf() and fosfat_search_incache().
 *
 * \param realname   the name of the file in the FOS
 * \param searchname the name in the path
 * \return true if it seems to be a dir name
 */
static inline int
fosfat_isdirname (const char *realname, const char *searchname)
{
      /* Test with a name as foobar.dir */
  if ((
#ifdef _WIN32
        (strstr (realname, ".dir") || strstr (realname, ".DIR"))
#else
        strcasestr (realname, ".dir")
#endif
        && !strncasecmp (realname, searchname, strlen (realname) - 4)
        && strlen (searchname) == strlen (realname) - 4
      ) ||
      /* Test with a name as sys_list (without .dir) */
      (
        !strncasecmp (realname, searchname, strlen (realname))
        && strlen (searchname) == strlen (realname)
      ))
    return 1;

  return 0;
}

/**
 * \brief Search a BD or a BLF from a location.
 * \deprecated use the cache instead!
 *
 *  A good example for use this function, is the BL of the first SYS_LIST in
 *  the disk. It will search the file BD since this BL.
 *  The location must be not bigger of MAX_SPLIT /!\
 *
 * \param fosfat   the main structure
 * \param location path for found the BD/BLF (foo/bar/file)
 * \param files    first BL for start the search
 * \param type     eSBD or eSBLF
 * \return the BD, BLF or NULL is nothing found
 */
static void *
fosfat_search_bdlf (fosfat_t *fosfat, const char *location,
                    fosfat_bl_t *files, fosfat_search_t type)
{
  int i, j, nb = 0, ontop = 1;
  char *tmp, *path;
  char dir[MAX_SPLIT][FOSFAT_NAMELGT];
  fosfat_bl_t *loop;
  fosfat_bd_t *loop_bd = NULL;
  fosfat_blf_t *loop_blf = NULL;

  if (!fosfat || !files || !location)
    return NULL;

  if (type == eSBLF)
    loop_blf = malloc (sizeof (fosfat_blf_t));

  loop = files;
  path = strdup (location);

  /* Split the path into a table */
  if ((tmp = strtok ((char *) path, "/"))) {
    snprintf (dir[nb], sizeof (dir[nb]), "%s", tmp);
    while ((tmp = strtok (NULL, "/")) && nb < MAX_SPLIT - 1)
      snprintf (dir[++nb], sizeof (dir[nb]), "%s", tmp);
  }
  else
    snprintf (dir[nb], sizeof (dir[nb]), "%s", path);
  nb++;

  /* Loop for all directories in the path */
  for (i = 0; i < nb; i++) {
    ontop = 1;

    /* Loop for all BL */
    do {
      /* Loop for FOSFAT_NBL files in the BL */
      for (j = 0; j < FOSFAT_NBL && ontop && loop; j++) {
        if (fosfat_in_isopenexm (&loop->file[j])
            && (fosfat->viewdel
            || (!fosfat->viewdel && fosfat_in_isnotdel (&loop->file[j]))))
        {
          /* Test if it is a directory */
          if (fosfat_in_isdir (&loop->file[j])
              && fosfat_isdirname ((char *) loop->file[j].name, dir[i]))
          {
            if (type == eSBLF && loop_blf)
              memcpy (loop_blf, &loop->file[j], sizeof (*loop_blf));

            uint32_t pt = c2l (loop->file[j].pt, sizeof (loop->file[j].pt));

            if (loop_bd)
              fosfat_free_dir (loop_bd);

            loop_bd = fosfat_read_dir (fosfat, pt);

            if (loop_bd)
              loop = loop_bd->first_bl;

            ontop = 0;  // dir found
            break;
          }
          /* Test if it is a file or a soft-link */
          else if (!fosfat_in_isdir(&loop->file[j])
                   && (!strcasecmp ((char *) loop->file[j].name, dir[i])
                       || (fosfat_in_islink (&loop->file[j])
                           && fosfat_isdirname ((char *) loop->file[j].name,
                                                dir[i]))
                      )
                  )
          {
            if (type == eSBLF && loop_blf)
              memcpy (loop_blf, &loop->file[j], sizeof (*loop_blf));

            uint32_t pt = c2l (loop->file[j].pt, sizeof (loop->file[j].pt));

            if (loop_bd)
              fosfat_free_dir (loop_bd);

            loop_bd = fosfat_read_file (fosfat, pt);
            loop = NULL;
            ontop = 0;  // file (or soft-link) found
            break;
          }
          else
            ontop = 1;
        }
        else
          ontop = 1;
      }
    } while (ontop && loop && (loop = loop->next_bl));
  }

  free (path);

  if (!ontop) {
    if (type == eSBLF) {
      fosfat_free_dir (loop_bd);
      return (fosfat_blf_t *) loop_blf;
    }
    else
      return (fosfat_bd_t *) loop_bd;
  }

  return NULL;
}

/**
 * \brief Search a BD or a BLF from a location in the cache.
 *
 *  The location must be not bigger of MAX_SPLIT /!\
 *
 * \param fosfat   the main structure
 * \param location path for found the BD/BLF (foo/bar/file)
 * \param type     eSBD or eSBLF
 * \return the BD, BLF or NULL is nothing found
 */
static void *
fosfat_search_incache (fosfat_t *fosfat, const char *location,
                       fosfat_search_t type)
{
  int i, nb = 0, ontop = 1, isdir = 0;
  char *tmp, *path, *name = NULL;
  char dir[MAX_SPLIT][FOSFAT_NAMELGT];
  cachelist_t *list;
  uint32_t bd_block = 0, bl_block = 0;
  fosfat_bl_t *bl_found = NULL;
  fosfat_blf_t *blf_found = NULL;
  fosfat_bd_t *bd_found = NULL;

  if (!fosfat || !location)
    return NULL;

  list = fosfat->cachelist;
  path = strdup (location);

  /* Split the path into a table */
  if ((tmp = strtok ((char *) path, "/"))) {
    snprintf (dir[nb], sizeof (dir[nb]), "%s", tmp);
    while ((tmp = strtok (NULL, "/")) && nb < MAX_SPLIT - 1)
      snprintf (dir[++nb], sizeof (dir[nb]), "%s", tmp);
  }
  else
    snprintf (dir[nb], sizeof (dir[nb]), "%s", path);
  nb++;

  /* Loop for all directories in the path */
  for (i = 0; list && i < nb; i++) {
    ontop = 1;

    do {
      /* test if the file is deleted or not */
      if (fosfat->viewdel || (!fosfat->viewdel && !list->isdel)) {
        /* Test if it is a directory */
        if (list->isdir && fosfat_isdirname (list->name, dir[i])) {
          bd_block = list->bd;
          bl_block = list->bl;

          if (name)
            free (name);

          name = strdup (list->name);

          /* Go to the next level */
          list = list->sub;
          ontop = 0;
          isdir = 1;
        }
        /* Test if it is a file or a soft-link */
        else if (!list->isdir
                 && (!strcasecmp (list->name, dir[i])
                     || (list->islink
                         && fosfat_isdirname (list->name, dir[i]))
                    )
                )
        {
          bd_block = list->bd;
          bl_block = list->bl;

          if (name)
            free (name);

          name = strdup (list->name);
          ontop = 0;
          isdir = 0;
        }
        else
          ontop = 1;
      }
      else
        ontop = 1;
    } while (ontop && list && (list = list->next));
  }

  free(path);

  if (!ontop) {
    switch (type) {
    case eSBD: {
      if (name)
        free (name);

      if (isdir)
        bd_found = fosfat_read_dir (fosfat, bd_block);
      else
        bd_found = fosfat_read_file (fosfat, bd_block);

      return (fosfat_bd_t *) bd_found;
    }

    case eSBLF: {
      bl_found = fosfat_read_bl (fosfat, bl_block);

      for (i = 0; bl_found && i < FOSFAT_NBL; i++) {
        char name_r[FOSFAT_NAMELGT];
        snprintf (name_r, FOSFAT_NAMELGT, "%s",
                  (char) bl_found->file[i].name[0]
                  ? (char *) bl_found->file[i].name
                  : (char *) bl_found->file[i].name + 1);

        if (!strcasecmp (name_r, name)) {
          free (name);

          if ((blf_found = malloc (sizeof (fosfat_blf_t)))) {
            memcpy (blf_found, &bl_found->file[i], sizeof (*blf_found));
            free (bl_found);

            return (fosfat_blf_t *) blf_found;
          }
        }
      }

      if (bl_found)
        free (bl_found);
    }
    }
  }

  if (name)
    free (name);

  return NULL;
}

/**
 * \brief Search a BD or a BLF from a location in the first SYS_LIST.
 *
 *  That uses fosfat_search_bdlf() and fosfat_search_incache().
 *  But always use the cache is recommanded!
 *
 * \param fosfat   the main structure
 * \param location path for found the BD (foo/bar/file)
 * \param type     eSBD or eSBLF
 * \return the BD, BLF or NULL is nothing found
 */
static void *
fosfat_search_insys (fosfat_t *fosfat, const char *location,
                     fosfat_search_t type)
{
  fosfat_bd_t *syslist;
  fosfat_bl_t *files;
  void *search = NULL;

  if (!fosfat || !location)
    return NULL;

  if (type == eSBD && (*location == '\0' || !strcmp (location, "/"))) {
    syslist = fosfat_read_dir (fosfat, FOSFAT_SYSLIST);
    return (fosfat_bd_t *) syslist;
  }
  /* Without cache, slower but better if the files change
   * when the FOS is always mounted (normally useless) !
   */
  if (!fosfat->cache
      && (syslist = fosfat_read_dir (fosfat, FOSFAT_SYSLIST)))
  {
    files = syslist->first_bl;

    if ((search = fosfat_search_bdlf (fosfat, location, files, type))) {
      fosfat_free_dir (syslist);

      if (type == eSBLF)
        return (fosfat_blf_t *) search;
      else
        return (fosfat_bd_t *) search;
    }
    fosfat_free_dir (syslist);
  }
  /* With cache enable, faster */
  else if (fosfat->cache) {
    if ((search = fosfat_search_incache (fosfat, location, type))) {

      if (type == eSBLF)
        return (fosfat_blf_t *) search;
      else
        return (fosfat_bd_t *) search;
    }
  }

  if (g_logger)
    foslog (eWARNING, "file \"%s\" not found", location);

  return NULL;
}

/**
 * \brief Test if the file is a directory.
 *
 *  This function uses a string location.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return a boolean (true for success)
 */
int
fosfat_isdir (fosfat_t *fosfat, const char *location)
{
  int res = 0;
  fosfat_blf_t *entry;

  if (!fosfat || !location)
    return 0;

  if (strcmp (location, "/")) {
    if ((entry = fosfat_search_insys (fosfat, location, eSBLF))) {
      if (fosfat_in_isdir (entry))
        res = 1;

      free (entry);
    }
  }
  else
    res = 1;

  return res;
}

/**
 * \brief Test if the file is a link.
 *
 *  This function uses a string location.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return a boolean (true for success)
 */
int
fosfat_islink (fosfat_t *fosfat, const char *location)
{
  int res = 0;
  fosfat_blf_t *entry;

  if (!fosfat || !location)
    return 0;

  if ((entry = fosfat_search_insys (fosfat, location, eSBLF))) {
    if (fosfat_in_islink (entry))
      res = 1;

    free (entry);
  }

  return res;
}

/**
 * \brief Test if the file is visible.
 *
 *  This function uses a string location.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return a boolean (true for success)
 */
int
fosfat_isvisible (fosfat_t *fosfat, const char *location)
{
  int res = 0;
  fosfat_blf_t *entry;

  if (!fosfat || !location)
    return 0;

  if ((entry = fosfat_search_insys (fosfat, location, eSBLF))) {
    if (fosfat_in_isvisible (entry))
      res = 1;

    free (entry);
  }

  return res;
}

/**
 * \brief Test if the file is encoded.
 *
 *  This function uses a string location.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return a boolean (true for success)
 */
int
fosfat_isencoded (fosfat_t *fosfat, const char *location)
{
  int res = 0;
  fosfat_blf_t *entry;

  if (!fosfat || !location)
    return 0;

  if ((entry = fosfat_search_insys (fosfat, location, eSBLF))) {
    if (fosfat_in_isencoded (entry))
      res = 1;

    free (entry);
  }

  return res;
}

/**
 * \brief Test if the file is valid.
 *
 *  This function uses a string location.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return a boolean (true for success)
 */
int
fosfat_isopenexm (fosfat_t *fosfat, const char *location)
{
  int res = 0;
  fosfat_blf_t *entry;

  if (!fosfat || !location)
    return 0;

  if ((entry = fosfat_search_insys (fosfat, location, eSBLF))) {
    if (fosfat_in_isopenexm (entry))
      res = 1;

    free (entry);
  }

  return res;
}

/**
 * \brief Read the data for get the target path of a symlink.
 *
 *  A Smaky's symlink is a simple file with the target's path in the data.
 *  This function read the data and extract the right path.
 *
 * \param fosfat the main structure
 * \param file   BD of the symlink
 * \return the target path
 */
static char *
fosfat_get_link (fosfat_t *fosfat, fosfat_bd_t *file)
{
  fosfat_data_t *data;
  char *path = NULL;
  char *start, *it;

  data = fosfat_read_data (fosfat, c2l (file->pts[0], sizeof (file->pts[0])),
                           file->nbs[0], eDATA);
  if (!data)
    return NULL;

  start = (char *) data->data + 3;

  while ((it = my_strnchr (start, strlen (start), ':')))
    *it = '/';

  it = strrchr (start, '/');
  if (it)
    *it = '\0';

  path = strdup (start);
  if (path)
    lc (path);

  fosfat_free_data (data);

  return path;
}

/**
 * \brief Get the target of a symlink.
 *
 *  This function is high level.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return the target path
 */
char *
fosfat_symlink (fosfat_t *fosfat, const char *location)
{
  fosfat_bd_t *entry;
  char *link = NULL;

  if (!fosfat || !location)
    return NULL;

  entry = fosfat_search_insys (fosfat, location, eSBD);
  if (entry) {
    link = fosfat_get_link (fosfat, entry);
    free (entry);
  }

  if (g_logger && !link)
    foslog (eERROR, "target of symlink \"%s\" not found", location);

  return link;
}

/**
 * \brief Return all informations on one file.
 *
 *  This function uses the BLF and get only useful attributes.
 *
 * \param file BLF on the file
 * \return the stat
 */
static fosfat_file_t *
fosfat_stat (fosfat_blf_t *file)
{
  fosfat_file_t *stat = NULL;

  if (file && (stat = malloc (sizeof (fosfat_file_t)))) {
    /* Size (bytes) */
    stat->size = c2l (file->lgf, sizeof (file->lgf));

    /* Attributes (field bits) */
    stat->att.isdir     = fosfat_in_isdir (file)     ? 1 : 0;
    stat->att.isvisible = fosfat_in_isvisible (file) ? 1 : 0;
    stat->att.isencoded = fosfat_in_isencoded (file) ? 1 : 0;
    stat->att.islink    = fosfat_in_islink (file)    ? 1 : 0;
    stat->att.isdel     = fosfat_in_isnotdel (file)  ? 0 : 1;

    /* Creation date */
    stat->time_c.year   = y2k (h2d (file->cd[2]));
    stat->time_c.month  = h2d (file->cd[1]);
    stat->time_c.day    = h2d (file->cd[0]);
    stat->time_c.hour   = h2d (file->ch[0]);
    stat->time_c.minute = h2d (file->ch[1]);
    stat->time_c.second = h2d (file->ch[2]);
    /* Writing date */
    stat->time_w.year   = y2k (h2d (file->wd[2]));
    stat->time_w.month  = h2d (file->wd[1]);
    stat->time_w.day    = h2d (file->wd[0]);
    stat->time_w.hour   = h2d (file->wh[0]);
    stat->time_w.minute = h2d (file->wh[1]);
    stat->time_w.second = h2d (file->wh[2]);
    /* Use date */
    stat->time_r.year   = y2k (h2d (file->rd[2]));
    stat->time_r.month  = h2d (file->rd[1]);
    stat->time_r.day    = h2d (file->rd[0]);
    stat->time_r.hour   = h2d (file->rh[0]);
    stat->time_r.minute = h2d (file->rh[1]);
    stat->time_r.second = h2d (file->rh[2]);

    /* Name */
    if (stat->att.isdel) {
      strncpy (stat->name, (char *) file->name + 1, sizeof (stat->name));
      stat->name[15] = '\0';
    }
    else
      strncpy (stat->name, (char *) file->name, sizeof (stat->name));
    lc (stat->name);

    stat->next_file = NULL;
  }

  return stat;
}

/**
 * \brief Return all informations on one file.
 *
 *  This function is high level.
 *
 * \param fosfat   the main structure
 * \param location file in the path
 * \return the stat
 */
fosfat_file_t *
fosfat_get_stat (fosfat_t *fosfat, const char *location)
{
  fosfat_blf_t *entry;
  fosfat_file_t *stat = NULL;

  if (fosfat && location
      && (entry = fosfat_search_insys (fosfat, location, eSBLF)))
  {
    stat = fosfat_stat (entry);

    free (entry);
  }

  if (g_logger && !stat)
    foslog (eWARNING, "stat of \"%s\" not found", location);

  return stat;
}

/**
 * \brief Return a linked list with all files of a directory.
 *
 *  This function is high level.
 *
 * \param fosfat   the main structure
 * \param location directory in the path
 * \return the linked list
 */
fosfat_file_t *
fosfat_list_dir (fosfat_t *fosfat, const char *location)
{
  int i;
  fosfat_bd_t *dir;
  fosfat_bl_t *files;
  fosfat_file_t *sysdir = NULL;
  fosfat_file_t *listdir = NULL;
  fosfat_file_t *firstfile = NULL;
  fosfat_file_t *res = NULL;

  if (fosfat && location
      && (dir = fosfat_search_insys (fosfat, location, eSBD)))
  {
    /* Test if it is a directory */
    if (fosfat_isdir (fosfat, location)) {
      files = dir->first_bl;

      if (files) {
        do {
          /* Check all files in the BL */
          for (i = 0; i < FOSFAT_NBL; i++) {
            if (fosfat_in_isopenexm (&files->file[i])
                && (fosfat->viewdel
                    || (!fosfat->viewdel
                        && fosfat_in_isnotdel (&files->file[i]))
                   )
                && !fosfat_in_issystem (&files->file[i]))
            {
              /* Complete the linked list with all files */
              if (listdir) {
                listdir->next_file = fosfat_stat (&files->file[i]);
                listdir = listdir->next_file;
              }
              else {
                firstfile = fosfat_stat (&files->file[i]);
                listdir = firstfile;
              }
            }
            else if (fosfat_in_issystem (&files->file[i])
                     && !strcasecmp ((char *) files->file[i].name, "sys_list"))
            {
              sysdir = fosfat_stat (&files->file[i]);
              strcpy (sysdir->name, "..dir");
            }
          }
        } while ((files = files->next_bl));
      }
    }
    fosfat_free_dir (dir);
  }

  if (sysdir) {
    sysdir->next_file = firstfile;
    res = sysdir;
  }
  else
    res = firstfile;

  if (g_logger) {
    if (!res)
      foslog (eWARNING, "directory \"%s\" is unknown", location);
    else
      foslog (eNOTICE, "directory \"%s\" is read successfully", location);
  }

  return res;
}

/**
 * \brief Get a file and put this in a location on the PC.
 *
 *  This function create a copy from src to dst. An output variable can be
 *  used for that the current size is printed for each PTS.
 *
 * \param fosfat the main structure
 * \param src    source on the Smaky disk
 * \param dst    destination on your PC
 * \param output TRUE for print the size
 * \return a boolean (true for success)
 */
int
fosfat_get_file (fosfat_t *fosfat, const char *src,
                 const char *dst, int output)
{
  int res = 0;
  fosfat_blf_t *file;
  fosfat_bd_t *file2;

  if (fosfat && src && dst 
      && (file = fosfat_search_insys (fosfat, src, eSBLF)))
  {
    if (!fosfat_in_isdir (file)) {
      file2 = fosfat_read_file (fosfat, c2l (file->pt, sizeof (file->pt)));

      if (file2 && fosfat_get (fosfat, file2, dst, output, 0))
        res = 1;

      fosfat_free_file (file2);
    }

    free (file);
  }

  if (g_logger) {
    if (!res)
      foslog (eWARNING, "file \"%s\" cannot be copied", src);
    else
      foslog (eNOTICE, "get file \"%s\" and save to \"%s\"", src, dst);
  }

  return res;
}

/**
 * \brief Get a buffer from a file in the FOS.
 *
 *  The buffer can be selected with an offset in the file and with a size.
 *
 * \param fosfat the main structure
 * \param path   source on the Smaky disk
 * \param offset start address in the file
 * \param size   length of the buffer
 * \return the buffer with the data
 */
char *
fosfat_get_buffer (fosfat_t *fosfat, const char *path, int offset, int size)
{
  char *buffer = NULL;
  fosfat_blf_t *file;
  fosfat_bd_t *file2;

  if (fosfat && path && (file = fosfat_search_insys (fosfat, path, eSBLF))
      && !fosfat_in_isdir (file))
  {
    buffer = malloc (size);

    if (buffer) {
      memset (buffer, 0, size);
      file2 = fosfat_read_file (fosfat, c2l (file->pt, sizeof (file->pt)));

      if (file2)
        fosfat_get (fosfat, file2, NULL, 0, 1, offset, size, buffer);
      else {
        free (buffer);
        buffer = NULL;
      }

      free (file);

      if (file2)
        fosfat_free_file (file2);
    }
  }

  if (g_logger) {
    if (!buffer)
      foslog (eERROR, "data (offset:%i size:%i) of \"%s\" not read",
              offset, size, path);
    else
      foslog (eNOTICE, "data (offset:%i size:%i) of \"%s\" correctly read",
              offset, size, path);
  }

  return buffer;
}

/**
 * \brief Get the name of a disk.
 *
 * \param fosfat the main structure
 * \return the name
 */
char *
fosfat_diskname (fosfat_t *fosfat)
{
  fosfat_b0_t *block0;
  char *name = NULL;

  if (fosfat && (block0 = fosfat_read_b0 (fosfat, FOSFAT_BLOCK0))) {
    name = strdup ((char *) block0->nlo);
    free (block0);
  }

  if (g_logger) {
    if (!name)
      foslog (eERROR, "the disk name cannot be found");
    else
      foslog (eNOTICE, "disk name found (%s)", name);
  }

  return name;
}

/**
 * \brief Put file information in the cache list structure.
 *
 * \param file BLF element in the BL
 * \param bl   BL block's number
 * \return the cache for a file
 */
static cachelist_t *
fosfat_cache_file (fosfat_blf_t *file, uint32_t bl)
{
  cachelist_t *cachefile = NULL;

  if (file && (cachefile = malloc (sizeof (cachelist_t)))) {
    cachefile->next = NULL;
    cachefile->sub = NULL;
    cachefile->isdir = fosfat_in_isdir (file) ? 1 : 0;
    cachefile->islink = fosfat_in_islink (file) ? 1 : 0;

    /* if the first char is NULL, then the file is deleted */
    if ((char) file->name[0] == '\0') {
      cachefile->isdel = 1;
      cachefile->name = strdup ((char *) file->name + 1);
      if (strlen (cachefile->name) >= 16)
        cachefile->name[15] = '\0';
    }
    else {
      cachefile->isdel = 0;
      cachefile->name = strdup ((char *) file->name);
    }

    cachefile->bl = bl;
    cachefile->bd = c2l (file->pt, sizeof (file->pt));
  }

  return cachefile;
}

/**
 * \brief List all files on the disk for fill the global cache list.
 *
 *  This function is recursive!
 *
 * \param fosfat the main structure
 * \param pt     block's number of the BD
 * \return the first element of the cache list.
 */
static cachelist_t *
fosfat_cache_dir (fosfat_t *fosfat, uint32_t pt)
{
  int i;
  fosfat_bd_t *dir = NULL;
  fosfat_bl_t *files;
  cachelist_t *firstfile = NULL;
  cachelist_t *list = NULL;

  if (fosfat && (dir = fosfat_read_dir (fosfat, pt))) {
    files = dir->first_bl;

    if (files) {
      do {
        /* Check all files in the BL */
        for (i = 0; i < FOSFAT_NBL; i++) {
          if (fosfat_in_isopenexm (&files->file[i])
              && (fosfat->viewdel
                  || (!fosfat->viewdel && fosfat_in_isnotdel(&files->file[i]))
                 )
             )
          {
            /* Complete the linked list with all files */
            if (list) {
              list->next = fosfat_cache_file (&files->file[i], files->pt);
              list = list->next;
            }
            else {
              firstfile = fosfat_cache_file (&files->file[i], files->pt);
              list = firstfile;
            }

            /* If the file is a directory, then do a recursive cache */
            if (list && fosfat_in_isdir (&files->file[i])
                && !fosfat_in_issystem (&files->file[i]))
              list->sub = fosfat_cache_dir (fosfat, list->bd);
          }
        }
      } while ((files = files->next_bl));
    }
    fosfat_free_dir (dir);
  }

  if (g_logger && !firstfile)
    foslog (eERROR, "cache to block %i not correctly loaded", pt);

  return firstfile;
}

/**
 * \brief Unload the cache.
 *
 *  This function releases all the cache when the device is closed.
 *
 * \param cache the first element of the cache list
 */
static void
fosfat_cache_unloader (cachelist_t *cache)
{
  cachelist_t *it, *tofree;

  it = cache;
  while (it) {
    if (it->sub)
      fosfat_cache_unloader (it->sub);

    tofree = it;
    it = it->next;
    free (tofree->name);
    free (tofree);
  }
}

/**
 * \brief Auto detection of the FOSBOOT length.
 *
 *  This function detects if the device is an hard disk or a floppy disk.
 *  There is no explicit information for know that, then the CHK is tested
 *  between the SYS_LIST and the first BL and a test for know if the BL's
 *  pointer in the BD is right.
 *
 * \param fosfat the main structure
 * \return the type of disk or eFAILS
 */
static fosfat_disk_t
fosfat_diskauto (fosfat_t *fosfat)
{
  int i, loop = 1;
  int fboot = -1;
  fosfat_disk_t res = eFAILS;
  fosfat_bd_t *sys_list;
  fosfat_bl_t *first_bl;

  if (fosfat) {
    fosfat->fosboot = FOSBOOT_FD;

    /* for i = 0, test with FD and when i = 1, test for HD */
    for (i = 0; loop && i < 2; i++) {
      sys_list = fosfat_read_bd (fosfat, FOSFAT_SYSLIST);
      fosfat->foschk = 0;
      first_bl = fosfat_read_bl (fosfat, FOSFAT_SYSLIST + 1);
      fosfat->foschk = 0;

      if (sys_list && first_bl
          && !strncmp ((char *) sys_list->chk, (char *) first_bl->chk,
                       sizeof (sys_list->chk))
          && c2l (*sys_list->pts, sizeof (*sys_list->pts))
          == FOSFAT_SYSLIST + 1)
      {
        loop = 0;
        fboot = fosfat->fosboot;
      }

      if (sys_list)
        free (sys_list);
      if (first_bl)
        free (first_bl);

      if (loop && !i)
        fosfat->fosboot = FOSBOOT_HD;
    }
    /* Select the right fosboot */
    switch (fboot) {
    case FOSBOOT_FD:
      res = eFD;
      break;

    case FOSBOOT_HD:
      res = eHD;
      break;

    default:
      res = eFAILS;
    }
  }

  /* Restore the fosboot */
  fosfat->fosboot = -1;

  return res;
}

/**
 * \brief Open the device.
 *
 *  That hides the fopen processing. A device can be read like a file.
 *  But for Win32, the w32disk library is used for Win9x and WinNT low
 *  level access on the disk.
 *
 * \param dev  the device name
 * \param disk disk type
 * \param flag F_UNDELETE or 0 for nothing
 * \return the device handle
 */
fosfat_t *
fosfat_open (const char *dev, fosfat_disk_t disk, unsigned int flag)
{
  fosfat_disk_t fboot;
  fosfat_t *fosfat = NULL;

  if (dev && (fosfat = malloc (sizeof (fosfat_t)))) {
    fosfat->fosboot = -1;
    fosfat->foschk = 0;
    fosfat->cache = 1;
    fosfat->viewdel = (flag & F_UNDELETE) == F_UNDELETE ? 1 : 0;
    fosfat->cachelist = NULL;

    /* Open the device */
    if (g_logger)
      foslog (eNOTICE, "device is opening ...");

#ifdef _WIN32
    if ((fosfat->dev = new_w32disk (*dev - 'a'))) {
#else
    if ((fosfat->dev = fopen (dev, "r"))) {
#endif
      if (disk == eDAUTO) {
        if (g_logger)
          foslog (eNOTICE, "auto detection in progress ...");

        disk = fosfat_diskauto (fosfat);
        fboot = disk;
      }
      else
        fboot = fosfat_diskauto (fosfat);

      /* Test if the auto detection and the user param are the same */
      if (g_logger && fboot != disk)
        foslog (eWARNING, "disk type forced seems to be false");

      switch (disk) {
      case eFD:
        fosfat->fosboot = FOSBOOT_FD;
        if (g_logger)
          foslog (eNOTICE, "floppy disk selected");
        break;

      case eHD:
        fosfat->fosboot = FOSBOOT_HD;
        if (g_logger)
          foslog (eNOTICE, "hard disk selected");
        break;

      case eFAILS: {
        if (g_logger)
          foslog (eERROR, "disk auto detection for \"%s\" has failed", dev);
      }

      default:
#ifdef _WIN32
        free_w32disk (fosfat->dev);
#else
        fclose (fosfat->dev);
#endif
        free (fosfat);
        fosfat = NULL;
      }

      /* Load the cache if needed */
      if (fosfat && fosfat->cache) {
        if (g_logger)
          foslog (eNOTICE, "cache file is loading ...");

        if (!(fosfat->cachelist = fosfat_cache_dir (fosfat, FOSFAT_SYSLIST))) {
#ifdef _WIN32
          free_w32disk (fosfat->dev);
#else
          fclose (fosfat->dev);
#endif
          free (fosfat);
          fosfat = NULL;
        }
        else if (g_logger)
          foslog (eNOTICE, "fosfat is ready");
      }
    }
  }

  return fosfat;
}

/**
 * \brief Close the device.
 *
 *  That hides the fclose processing. And it uses the w32disk library with
 *  Win9x and WinNT.
 *
 * \param fosfat the main structure
 */
void
fosfat_close (fosfat_t *fosfat)
{
  if (fosfat) {
    /* Unload the cache if is loaded */
    if (fosfat->cachelist) {
      if (g_logger)
        foslog (eNOTICE, "cache file is unloading ...");

      fosfat_cache_unloader (fosfat->cachelist);
    }

    if (g_logger)
      foslog (eNOTICE, "device is closing ...");

    if (fosfat->dev)
#ifdef _WIN32
      free_w32disk (fosfat->dev);
#else
      fclose (fosfat->dev);
#endif

    free (fosfat);
  }
}
