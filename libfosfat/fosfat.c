/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2006-2009 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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

#define MAX_SPLIT             64

/* Block size (256 bytes) */
#define FOSFAT_BLK            256

#define FOSFAT_NBL            4
#define FOSFAT_Y2K            70

#define FOSFAT_BLOCK0         0x00
#define FOSFAT_SYSLIST        0x01

#define FOSBOOT_FD            0x10
#define FOSBOOT_HD            0x20

#define FOSFAT_DEV            FILE

/* FOS attributes and type */
#define FOSFAT_ATT_OPENEX     (1 <<  0)
#define FOSFAT_ATT_MULTIPLE   (1 <<  1)
#define FOSFAT_ATT_DIR        (1 << 12)
#define FOSFAT_ATT_VISIBLE    (1 << 13)
#define FOSFAT_ATT_ENCODED    (1 << 17)
#define FOSFAT_ATT_LINK       (1 << 24)
#define FOSFAT_TYPE_SYSTEM    0xF8

/* List of all block types */
typedef enum block_type {
  B_B0,                        /* Block 0                               */
  B_BL,                        /* Block List                            */
  B_BD,                        /* Block Description                     */
  B_DATA                       /* Only DATA                             */
} fosfat_type_t;

/* Search type */
typedef enum search_type {
  S_BD,                        /* Search BD                             */
  S_BLF                        /* Search BL File                        */
} fosfat_search_t;

/* foslog type */
typedef enum foslog {
  FOSLOG_ERROR,                /* Error log                             */
  FOSLOG_WARNING,              /* Warning log                           */
  FOSLOG_NOTICE                /* Notice log                            */
} foslog_t;

/* Data Block (256 bytes) */
typedef struct block_data_s {
  uint8_t data[256];           /* Data                                  */
  /* Linked list */
  struct  block_data_s *next_data;
} fosfat_data_t;

/* Block 0 (256 bytes) */
typedef struct block_0_s {
  uint8_t sys[44];             /* SYSTEM folder                         */
  int8_t  nlo[16];             /* Disk name                             */
  uint8_t chk[4];              /* Check control                         */
  uint8_t mes[172];            /* Message                               */
  uint8_t change;              /* Need of change the CHK number         */
  uint8_t bonchk;              /* New format                            */
  uint8_t oldchk[4];           /* Old CHK value                         */
  uint8_t newchk[4];           /* New CHK value                         */
  uint8_t reserve[10];         /* Unused                                */
} fosfat_b0_t;

/* File in a Block List (60 bytes) */
typedef struct block_listf_s {
  int8_t  name[16];            /* Filename                              */
  uint8_t typ;                 /* Filetype                              */
  uint8_t ope;                 /* Open counter                          */
  uint8_t att[4];              /* Attributes                            */
  uint8_t lg[4];               /* Length in blocks                      */
  uint8_t lgb[2];              /* Number of bytes in the last block     */
  uint8_t cd[3];               /* Date of Creation                      */
  uint8_t ch[3];               /* Hour of Creation                      */
  uint8_t wd[3];               /* Date of the last Write                */
  uint8_t wh[3];               /* Hour of the last Write                */
  uint8_t rd[3];               /* Date of the last Read                 */
  uint8_t rh[3];               /* Hour of the last Read                 */
  uint8_t secid[3];            /* Security ID                           */
  uint8_t clope;               /* Open mode before CLOSE                */
  uint8_t pt[4];               /* Pointer on the BD                     */
  uint8_t lgf[4];              /* File size in bytes                    */
  uint8_t code[2];             /* Code control                          */
} fosfat_blf_t;

/* Block List (256 bytes) */
typedef struct block_list_s {
  fosfat_blf_t file[4];        /* 4 BL files (240 bytes)                */
  uint8_t      next[4];        /* Next BL                               */
  uint8_t      chk[4];         /* Check control                         */
  uint8_t      prev[4];        /* Previous BL                           */
  uint8_t      reserve[4];     /* Unused                                */
  /* Not in the block */
  uint32_t     pt;             /* Block's number of this BL             */
  /* Linked list */
  struct       block_list_s *next_bl;
} fosfat_bl_t;

/* Block Description (256 bytes) */
typedef struct block_desc_s {
  uint8_t next[4];             /* Next BD                               */
  uint8_t prev[4];             /* Previous BD                           */
  uint8_t npt[2];              /* Number of tranches in the BD          */
  uint8_t pts[42][4];          /* Pointers on the tranches (max 42)     */
  int8_t  name[16];            /* Filename                              */
  uint8_t nbs[42];             /* Length (in blocks) of each tranches   */
  uint8_t reserve[4];          /* Unused                                */
  uint8_t lst[2];              /* Number of byte in the last tranche    */
  uint8_t hac[2];              /* Hashing function if LIST              */
  uint8_t nbl[2];              /* Number of BL used if LIST             */
  uint8_t chk[4];              /* Check control                         */
  uint8_t off[4];              /* Offset (in blocks) of all previous BD */
  uint8_t free[2];             /* Unused                                */
  /* Linked list */
  struct  block_desc_s *next_bd;
  struct  block_list_s *first_bl;
} fosfat_bd_t;

/* Cache list for name, BD and BL blocks */
typedef struct cache_list_s {
  char    *name;
  uint32_t bl;                 /* BL Address                            */
  uint32_t bd;                 /* BD Address                            */
  int      isdir;              /* If is a directory                     */
  int      islink;             /* If is a soft link                     */
  int      isdel;              /* If is deleted                         */
  /* Linked list */
  struct   cache_list_s *sub;
  struct   cache_list_s *next;
} cachelist_t;

/* Main fosfat structure */
struct fosfat_s {
#ifdef _WIN32
  win32disk_t *dev;            /* device object                         */
#else
  FOSFAT_DEV  *dev;            /* physical device                       */
#endif
  int          fosboot;        /* FOSBOOT address                       */
  uint32_t     foschk;         /* CHK                                   */
  int          viewdel;        /* list deleted files                    */
  cachelist_t *cachelist;      /* cache data                            */
};


/* Global variable for internal logger */
static int g_logger = 0;


#define FOSFAT_IS(handle, loc, att)                   \
  {                                                   \
    int res = 0;                                      \
    fosfat_blf_t *entry;                              \
                                                      \
    entry = fosfat_search_insys (handle, loc, S_BLF); \
    if (!entry)                                       \
      return 0;                                       \
                                                      \
    if (fosfat_in_is##att (entry))                    \
      res = 1;                                        \
                                                      \
    free (entry);                                     \
    return res;                                       \
  }
#define FOSFAT_IS_DIR(handle, loc)     FOSFAT_IS(handle, loc, dir)
#define FOSFAT_IS_LINK(handle, loc)    FOSFAT_IS(handle, loc, link)
#define FOSFAT_IS_VISIBLE(handle, loc) FOSFAT_IS(handle, loc, visible)
#define FOSFAT_IS_ENCODED(handle, loc) FOSFAT_IS(handle, loc, encoded)
#define FOSFAT_IS_OPENEXM(handle, loc) FOSFAT_IS(handle, loc, openexm)

#ifdef __linux__
/*
 * Translate a block number to an address.
 *
 * This function depend if the media is an HARD DISK or an 3"1/2 DISK.
 * The FOSBOOT is the double in number of blocks for an hard disk.
 * The real address is the address given by the disk + the size of the
 * FOSBOOT part.
 *
 * block        the block's number given by the disk
 * fosboot      offset in the FOS address
 * return the address of this block on the disk
 */
static inline uint32_t
blk2add (uint32_t block, int fosboot)
{
  return ((block + fosboot) * FOSFAT_BLK);
}
#endif

#ifdef _WIN32
/*
 * Translate a block number to a sector.
 *
 * This function depend if the media is an HARD DISK or an 3"1/2 DISK.
 * The FOSBOOT is the double in number of blocks for an hard disk.
 * The real address is the sector given by the disk with the size of the
 * FOSBOOT part.
 *
 * block        the block's number given by the disk
 * fosboot      offset in the FOS address
 * return the sector on the disk
 */
static inline uint32_t
blk2sector (uint32_t block, int fosboot)
{
  /* FIXME: div by 2 only right if the sector size is 512 */
  return ((block + fosboot) / 2);
}

/*
 * Return the offset in the block to get the right Smaky's block.
 *
 * Smaky's blocks are of 256 bytes and Win32 uses 512. This function search
 * if an offset of 256 is necessary to get the right Smaky's block, or not.
 *
 * block        the block's number given by the disk
 * fosboot      offset in the FOS address
 * return the offset in the Win32's block
 */
static inline uint32_t
sec_offset (uint32_t block, int fosboot)
{
  /* FIXME: modulo by 2 only right if the sector size is 512 */
  return (((block + fosboot) % 2) ? FOSFAT_BLK : 0);
}
#endif

/*
 * Convert char table to an integer.
 *
 * This convertion will swapped all bytes and returned an int value.
 *
 * value        pointer on the char table
 * size         size of the table (number of bytes)
 * return the integer value
 */
static uint32_t
c2l (uint8_t *value, int size)
{
  int i, j;
  uint32_t res = 0;

  if (value)
    for (i = size - 1, j = 0; i >= 0; i--, j++)
      res += (*(value + j) << i * 8);

  return res;
}

/*
 * Change a string in lower case.
 *
 * data         the string
 * return a pointer on this string
 */
static char *
lc (char *data)
{
  int i;

  if (data)
    for (i = 0; data[i] != '\0'; i++)
      data[i] = (char) tolower ((int) (unsigned char) data[i]);

  return data;
}

/*
 * Hex to dec convertion.
 *
 * Convert an integer in base 10 with the value shown in base 16, in an
 * integer with the value shown in base 10.
 *
 * val          the value
 * return the new integer
 */
static int
h2d (int val)
{
  char conv[16];
  int res = 0;

  snprintf (conv, sizeof (conv), "%X", val);
  res = atoi (conv);

  return res;
}

/*
 * Convert the year to y2k.
 *
 * y            year on two digits
 * return the year on four digits
 */
static inline int
y2k (int y)
{
  return ((y < FOSFAT_Y2K) ? 2000 : 1900) + y;
}

/*
 * Like strchr but limited to a length.
 */
static char *
my_strnchr (const char *s, size_t count, int c)
{
  size_t i;

  for (i = 0; i < count; i++)
    if (*(s + i) == (const char) c)
      return (char *) (s + i);

  return NULL;
}

static inline char *
my_strcasestr (const char *s1, const char *s2)
{
#ifdef _WIN32
  char *res, *_s1, *_s2;

  _s1 = strdup (s1);
  _s2 = strdup (s2);
  res = strstr (lc (_s1), lc (_s2));
  free (_s1);
  free (_s2);
  return res;
#else
  return strcasestr (s1, s2);
#endif
}

static inline char *
my_strndup (const char *src, size_t n)
{
#ifdef _WIN32
  char *dst;

  dst = malloc (n + 1);
  memcpy (dst, src, n);
  dst[n] = '\0';
  return dst;
#else
  return strndup (src, n);
#endif
}

/*
 * Enable or disable the internal FOS logger.
 */
void
fosfat_logger (int state)
{
  if (state)
    g_logger = 1;
  else
    g_logger = 0;
}

/*
 * Print function for the internal FOS logger.
 */
static void
foslog (foslog_t type, const char *msg, ...)
{
  va_list va;
  char log[32] = "[fosfat] ";

  if (!g_logger || !msg)
    return;

  va_start (va, msg);

  switch (type)
  {
  case FOSLOG_ERROR:
    strcat (log, "error");
    break;

  case FOSLOG_WARNING:
    strcat (log, "warning");
    break;

  case FOSLOG_NOTICE:
    strcat (log, "notice");
  }

  fprintf (stderr, "%s: ", log);
  vfprintf (stderr, msg, va);
  fprintf (stderr, "\n");

  va_end (va);
}

/*
 * Free a DATA file variable.
 *
 * It will be used only when a data is loaded to copy a file on the PC,
 * to free each block after each write.
 *
 * var          pointer on the data block
 */
static void
fosfat_free_data (fosfat_data_t *var)
{
  fosfat_data_t *d, *free_d;

  d = var;
  while (d)
  {
    free_d = d;
    d = d->next_data;
    free (free_d);
  }
}

/*
 * Free a BD file variable.
 *
 * It must be used only with a BD that is (or can be) use as the first
 * member of a linked list.
 * This function must be used after all fosfat_read_file()!
 *
 * var          pointer on the description block
 */
static void
fosfat_free_file (fosfat_bd_t *var)
{
  fosfat_bd_t *bd, *free_bd;

  bd = var;
  while (bd)
  {
    free_bd = bd;
    bd = bd->next_bd;
    free (free_bd);
  }
}

/*
 * Free a BD dir variable.
 *
 * It must be used only with a BD that is (or can be) use as the first
 * member of a linked list for a dir (with BL linked list into).
 * This function must be used after all fosfat_read_dir()!
 *
 * var          pointer on the description block
 */
static void
fosfat_free_dir (fosfat_bd_t *var)
{
  fosfat_bl_t *bl, *free_bl;
  fosfat_bd_t *bd, *free_bd;

  if (!var)
    return;
  bd = var;

  do
  {
    bl = bd->first_bl;

    /* Freed all BL */
    while (bl)
    {
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

/*
 * Free a List Dir variable.
 *
 * This function must be used after all fosfat_list_dir()!
 *
 * var          pointer on the description block
 */
void
fosfat_free_listdir (fosfat_file_t *var)
{
  fosfat_file_t *ld, *free_ld;

  ld = var;
  while (ld)
  {
    free_ld = ld;
    ld = ld->next_file;
    free (free_ld);
  }
}

/*
 * Test if the file is a directory.
 *
 * This function read the ATT and return the value of the 12th bit,
 * mask 0x1000.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_isdir (fosfat_blf_t *file)
{
  return (file ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_DIR) : 0);
}

/*
 * Test if the file is a soft-link.
 *
 * This function read the ATT and return the value of the 24th bit,
 * mask 0x1000000.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_islink (fosfat_blf_t *file)
{
  return (file ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_LINK) : 0);
}

/*
 * Test if the file is visible.
 *
 * This function read the ATT and return the value of the 13th bit,
 * mask 0x2000.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_isvisible (fosfat_blf_t *file)
{
  return (file
          ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_VISIBLE)
          : 0);
}

/*
 * Test if the file is 'open exclusif' and 'multiple'.
 *
 * This function read the ATT and return the value of the 1st or 2th bit,
 * mask 0x1 and 0x2.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_isopenexm (fosfat_blf_t *file)
{
  return (file
          ? (((c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_OPENEX))
             || ((c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_MULTIPLE)))
          : 0);
}

/*
 * Test if the file is encoded.
 *
 * This function read the ATT and return the value of the 17th bit,
 * mask 0x20000.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_isencoded (fosfat_blf_t *file)
{
  return (file
          ? (c2l (file->att, sizeof (file->att)) & FOSFAT_ATT_ENCODED)
          : 0);
}

/*
 * Test if the file is system (5 MSB bits).
 *
 * This function read the TYP and return the value of the 3-7th bits,
 * mask 0xF8.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_issystem (fosfat_blf_t *file)
{
  return (file ? ((file->typ & FOSFAT_TYPE_SYSTEM)) : 0);
}

/*
 * Test if the file is not deleted.
 *
 * file         pointer on the file (in the BL)
 * return a boolean (true for success)
 */
static inline int
fosfat_in_isnotdel (fosfat_blf_t *file)
{
  return (file && strlen ((char *) file->name) > 0) ? 1 : 0;
}

/*
 * Read a block defined by a type.
 *
 * This function read a block on the disk and return the structure in
 * function of the type chosen. Each type use 256 bytes, but the structures
 * are always bigger. The order of the attributes in each structures is
 * very important, because the informations from the device are just copied
 * directly without parsing.
 *
 * fosfat       handle
 * block        block position
 * type         type of this block (B_B0, B_BL, B_BD or B_DATA)
 * return a pointer on the new block or NULL if broken
 */
static void *
fosfat_read_b (fosfat_t *fosfat, uint32_t block, fosfat_type_t type)
{
#ifdef _WIN32
  size_t ssize, csector;
  int8_t *buffer;
#endif

  if (!fosfat || !fosfat->dev)
    return NULL;

#ifdef _WIN32
  /* sector seems to be always 512 with Window$ */
  ssize = w32disk_sectorsize (fosfat->dev);

  csector = (size_t) FOSFAT_BLK / ssize;
  if (!csector)
    csector = 1;

  buffer = calloc (1, csector * ssize);
  if (!buffer)
    return NULL;
#else
  /* Move the pointer on the block */
  if (fseek (fosfat->dev, blk2add (block, fosfat->fosboot), SEEK_SET))
    return NULL;
#endif

  switch (type)
  {
  case B_B0:
  {
    fosfat_b0_t *blk;

    blk = malloc (sizeof (fosfat_b0_t));
    if (!blk)
      break;

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
      return blk;
    }
    free (blk);
    break;
  }

  case B_BL:
  {
    fosfat_bl_t *blk;

    blk = malloc (sizeof (fosfat_bl_t));
    if (!blk)
      break;

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
        return blk;

      foslog (FOSLOG_ERROR, "bad FOSCHK for this BL (block:%li)", block);
    }
    free (blk);
    break;
  }

  case B_BD:
  {
    fosfat_bd_t *blk;

    blk = malloc (sizeof (fosfat_bd_t));
    if (!blk)
      break;

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
        return blk;

      foslog (FOSLOG_ERROR, "bad FOSCHK for this BD (block:%li)", block);
    }
    free (blk);
    break;
  }

  case B_DATA:
  {
    fosfat_data_t *blk;

    blk = malloc (sizeof (fosfat_data_t));
    if (!blk)
      break;

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
      return blk;
    }
    free (blk);
    break;
  }
  }

#ifdef _WIN32
  if (buffer)
    free (buffer);
#endif

  return NULL;
}

/*
 * Read the first useful block (0).
 *
 * This block contents some informations on the disk. But no information
 * are critical to read the file list.
 *
 * fosfat       handle
 * block        block position
 * return the block0 or NULL if broken
 */
static inline fosfat_b0_t *
fosfat_read_b0 (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_b0_t *) fosfat_read_b (fosfat, block, B_B0))
          : NULL);
}

/*
 * Read data block.
 *
 * This block contents only a char table of 256 bytes for the raw data.
 *
 * fosfat       handle
 * block        block position
 * return the data or NULL if broken
 */
static inline fosfat_data_t *
fosfat_read_d (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_data_t *) fosfat_read_b (fosfat, block, B_DATA))
          : NULL);
}

/*
 * Read a Description Block (BD).
 *
 * fosfat       handle
 * block        block position
 * return the BD or NULL if broken
 */
static inline fosfat_bd_t *
fosfat_read_bd (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_bd_t *) fosfat_read_b (fosfat, block, B_BD))
          : NULL);
}

/*
 * Read a Block List (BL).
 *
 * fosfat       handle
 * block        block position
 * return the BL or NULL if broken
 */
static inline fosfat_bl_t *
fosfat_read_bl (fosfat_t *fosfat, uint32_t block)
{
  return (fosfat
          ? ((fosfat_bl_t *) fosfat_read_b (fosfat, block, B_BL))
          : NULL);
}

/*
 * Read data of some blocks and create linked list if necessary.
 *
 * When a directory or a file is read, the content is shown as data. But
 * for a directory, really it is a BL. This function create the linked list
 * for a DATA block or a BL for a number of consecutive blocks.
 *
 * fosfat       handle
 * block        the first block (start) for the linked list
 * nbs          number of consecutive blocks
 * type         type of this block (B_B0, B_BL, B_BD or B_DATA)
 * return the first block of the linked list created
 */
static void *
fosfat_read_data (fosfat_t *fosfat, uint32_t block,
                  uint8_t nbs, fosfat_type_t type)
{
  if (!fosfat)
    return NULL;

  switch (type)
  {
  case B_BL:
  {
    int i;
    fosfat_bl_t *block_list, *first_bl;

    first_bl = fosfat_read_bl (fosfat, block);
    if (!first_bl)
      break;

    block_list = first_bl;
    block_list->pt = block;

    for (i = 1; block_list && i < nbs; i++)
    {
      block_list->next_bl = fosfat_read_bl (fosfat, block + (uint32_t) i);
      block_list = block_list->next_bl;

      if (block_list)
        block_list->pt = block + (uint32_t) i;
    }
    return first_bl;
  }

  case B_DATA:
  {
    int i;
    fosfat_data_t *block_data, *first_data;

    first_data = fosfat_read_d (fosfat, block);
    if (!first_data)
      break;

    block_data = first_data;

    for (i = 1; block_data && i < nbs; i++)
    {
      block_data->next_data = fosfat_read_d (fosfat, block + (uint32_t) i);
      block_data = block_data->next_data;
    }
    return first_data;
  }

  default:
    break;
  }

  return NULL;
}

/*
 * Read a file description.
 *
 * A linked list is created to found all BD. And each BD have a linked list
 * to found all DATA.
 *
 * fosfat       handle
 * block        the file BD position
 * return the first BD of the linked list
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
  file_desc->first_bl = NULL;     /* Useless in this case */
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

/*
 * Get a file and put this in a location on the PC or in a buffer.
 *
 * This function read all BD->DATA of a file BD, and write the data in a
 * new file on your disk. An output variable can be used to print the
 * current size for each PTS. The properties like "Creation Date" are not
 * saved in the new file. All Linux file system are the same attributes for
 * them files. And for example, ext2/3 have no "Creation Date".
 * The data can be saved in a buffer if the flag is used with an offset,
 * size and a buffer.
 *
 * fosfat       handle
 * file         file description block
 * dst          destination on your PC
 * output       TRUE to print the size
 * flag         use the optional arguments or not
 * ...          offset, size and buffer
 * return a boolean (true for success)
 */
static int
fosfat_get (fosfat_t *fosfat, fosfat_bd_t *file,
            const char *dst, int output, int flag, ...)
{
  /* optional arguments */
  va_list pp;
  va_start (pp, flag);
  int op_offset = 0;
  int op_size = 0;
  uint8_t *op_buffer = NULL;
  int op_inoff = 0;
  if (flag)
  {
    op_offset = va_arg (pp, int);
    op_size = va_arg (pp, int);
    op_buffer = va_arg (pp, uint8_t *);
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

  if (!flag)
  {
    f_dst = fopen (dst, "w");
    if (!f_dst)
      return 0;
  }

  /* Loop for all BD */
  do
  {
    /* Loop for all pointers */
    for (i = 0; res && i < c2l (file->npt, sizeof (file->npt)); i++)
    {
      file_d = fosfat_read_data (fosfat, c2l (file->pts[i],
                                 sizeof (file->pts[i])), file->nbs[i], B_DATA);
      if (!file_d)
      {
        res = 0;
        break;
      }

      first_d = file_d;

      /* Loop for all data blocks */
      do
      {
        check_last = (i == c2l (file->npt, sizeof (file->npt)) - 1
                      && !file_d->next_data)
                      ? (size_t) c2l (file->lst, sizeof (file->lst))
                      : (size_t) FOSFAT_BLK;

        /* When the result is written in a file */
        if (!flag)
        {
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
  } while (res && file->next_bd && (file = file->next_bd));

  if (!flag)
  {
    fclose (f_dst);

    /* If fails then remove the incomplete file */
    if (!res)
      remove (dst);
  }

  return res;
}

/*
 * Read a complete .DIR (or SYS_LIST).
 *
 * A linked list is created to found all files. The first BD is returned.
 * You can use this function to read all SYS_LIST, even in each folder. But
 * the result will be always the same (recursive). Warning to don't create an
 * infinite loop /!\
 *
 * fosfat       handle
 * block        DIR (or SYS_LIST) BD position
 * return the first BD of the linked list
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
  if (!dir_desc)
  {
    foslog (FOSLOG_ERROR, "directory to block %i cannot be read", block);
    return NULL;
  }

  dir_desc->next_bd = NULL;
  dir_desc->first_bl = NULL;
  first_bd = dir_desc;

  do
  {
    /* Get the first pointer */
    dir_desc->first_bl = fosfat_read_data (fosfat, c2l(dir_desc->pts[0],
                         sizeof (dir_desc->pts[0])), dir_desc->nbs[0], B_BL);
    dir_list = dir_desc->first_bl;

    /* Go to the last BL */
    while (dir_list && dir_list->next_bl)
      dir_list = dir_list->next_bl;

    /* Loop all others pointers */
    for (i = 1; dir_list && i < c2l (dir_desc->npt,
                                     sizeof (dir_desc->npt)); i++)
    {
      dir_list->next_bl = fosfat_read_data (fosfat, c2l (dir_desc->pts[i],
                          sizeof (dir_desc->pts[i])), dir_desc->nbs[i], B_BL);
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

/*
 * Test if two names are the same or not.
 *
 * This function must be used only for particular purpose, because that will
 * no test if it is a directory or not, but only the names. That is very
 * useful for fosfat_search_incache().
 *
 * realname     the name of the file in the FOS
 * searchname   the name in the path
 * return true if it seems to be a dir name
 */
static inline int
fosfat_isdirname (const char *realname, const char *searchname)
{
      /* Test with a name as foobar.dir */
  if ((
        my_strcasestr (realname, ".dir")
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

/*
 * Search a BD or a BLF from a location in the cache.
 *
 * The location must not be bigger than MAX_SPLIT /!\
 *
 * fosfat       handle
 * location     path to found the BD/BLF (foo/bar/file)
 * type         S_BD or S_BLF
 * return the BD, BLF or NULL is nothing found
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
  if ((tmp = strtok ((char *) path, "/")))
  {
    snprintf (dir[nb], sizeof (dir[nb]), "%s", tmp);
    while ((tmp = strtok (NULL, "/")) && nb < MAX_SPLIT - 1)
      snprintf (dir[++nb], sizeof (dir[nb]), "%s", tmp);
  }
  else
    snprintf (dir[nb], sizeof (dir[nb]), "%s", path);
  nb++;

  /* Loop for all directories in the path */
  for (i = 0; list && i < nb; i++)
  {
    ontop = 1;

    do
    {
      /* test if the file is deleted or not */
      if (fosfat->viewdel || !list->isdel)
      {
        /* Test if it is a directory */
        if (list->isdir && fosfat_isdirname (list->name, dir[i]))
        {
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

  free (path);

  if (ontop)
  {
    if (name)
      free (name);
    return NULL;
  }

  switch (type)
  {
  case S_BD:
  {
    if (name)
      free (name);

    if (isdir)
      bd_found = fosfat_read_dir (fosfat, bd_block);
    else
      bd_found = fosfat_read_file (fosfat, bd_block);

    return bd_found;
  }

  case S_BLF:
  {
    bl_found = fosfat_read_bl (fosfat, bl_block);

    for (i = 0; bl_found && i < FOSFAT_NBL; i++)
    {
      char name_r[FOSFAT_NAMELGT];
      snprintf (name_r, FOSFAT_NAMELGT, "%s",
                (char) bl_found->file[i].name[0]
                ? (char *) bl_found->file[i].name
                : (char *) bl_found->file[i].name + 1);

      if (!strcasecmp (name_r, name))
      {
        free (name);

        blf_found = malloc (sizeof (fosfat_blf_t));
        if (blf_found)
        {
          memcpy (blf_found, &bl_found->file[i], sizeof (*blf_found));
          free (bl_found);

          return blf_found;
        }
      }
    }

    if (bl_found)
      free (bl_found);
  }
  }

  if (name)
    free (name);

  return NULL;
}

/*
 * Search a BD or a BLF from a location in the first SYS_LIST.
 *
 * That uses fosfat_search_incache().
 *
 * fosfat       handle
 * location     path to found the BD (foo/bar/file)
 * type         S_BD or S_BLF
 * return the BD, BLF or NULL is nothing found
 */
static void *
fosfat_search_insys (fosfat_t *fosfat, const char *location,
                     fosfat_search_t type)
{
  fosfat_bd_t *syslist;
  void *search = NULL;

  if (!fosfat || !location)
    return NULL;

  if (type == S_BD && (*location == '\0' || !strcmp (location, "/")))
  {
    syslist = fosfat_read_dir (fosfat, FOSFAT_SYSLIST);
    return syslist;
  }

  search = fosfat_search_incache (fosfat, location, type);
  if (search)
    return search;

  foslog (FOSLOG_WARNING, "file or directory \"%s\" not found", location);

  return NULL;
}

/*
 * Test if the file is a directory.
 *
 * This function uses a string location.
 *
 * fosfat       handle
 * location     file in the path
 * return a boolean (true for success)
 */
int
fosfat_isdir (fosfat_t *fosfat, const char *location)
{
  if (!fosfat || !location)
    return 0;

  if (!strcmp (location, "/"))
    return 1;

  FOSFAT_IS_DIR (fosfat, location)
}

/*
 * Test if the file is a link.
 *
 * This function uses a string location.
 *
 * fosfat       handle
 * location     file in the path
 * return a boolean (true for success)
 */
int
fosfat_islink (fosfat_t *fosfat, const char *location)
{
  if (!fosfat || !location)
    return 0;

  FOSFAT_IS_LINK (fosfat, location)
}

/*
 * Test if the file is visible.
 *
 * This function uses a string location.
 *
 * fosfat       handle
 * location     file in the path
 * return a boolean (true for success)
 */
int
fosfat_isvisible (fosfat_t *fosfat, const char *location)
{
  if (!fosfat || !location)
    return 0;

  FOSFAT_IS_VISIBLE (fosfat, location)
}

/*
 * Test if the file is encoded.
 *
 * This function uses a string location.
 *
 * fosfat       handle
 * location     file in the path
 * return a boolean (true for success)
 */
int
fosfat_isencoded (fosfat_t *fosfat, const char *location)
{
  if (!fosfat || !location)
    return 0;

  FOSFAT_IS_ENCODED (fosfat, location)
}

/*
 * Test if the file is valid.
 *
 * This function uses a string location.
 *
 * fosfat       handle
 * location     file in the path
 * return a boolean (true for success)
 */
int
fosfat_isopenexm (fosfat_t *fosfat, const char *location)
{
  if (!fosfat || !location)
    return 0;

  FOSFAT_IS_OPENEXM (fosfat, location)
}

/*
 * Read the data for get the target path of a symlink.
 *
 * A Smaky's symlink is a simple file with the target's path in the data.
 * This function read the data and extract the right path.
 *
 * fosfat       handle
 * file         BD of the symlink
 * return the target path
 */
static char *
fosfat_get_link (fosfat_t *fosfat, fosfat_bd_t *file)
{
  fosfat_data_t *data;
  char *path = NULL;
  char *start, *it;

  data = fosfat_read_data (fosfat, c2l (file->pts[0], sizeof (file->pts[0])),
                           file->nbs[0], B_DATA);
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

/*
 * Get the target of a symlink.
 *
 * This function is high level.
 *
 * fosfat       handle
 * location     file in the path
 * return the target path
 */
char *
fosfat_symlink (fosfat_t *fosfat, const char *location)
{
  fosfat_bd_t *entry;
  char *link = NULL;

  if (!fosfat || !location)
    return NULL;

  entry = fosfat_search_insys (fosfat, location, S_BD);
  if (entry)
  {
    link = fosfat_get_link (fosfat, entry);
    free (entry);
  }

  if (!link)
    foslog (FOSLOG_ERROR, "target of symlink \"%s\" not found", location);

  return link;
}

/*
 * Return all informations on one file.
 *
 * This function uses the BLF and get only useful attributes.
 *
 * file         BLF on the file
 * return the stat
 */
static fosfat_file_t *
fosfat_stat (fosfat_blf_t *file)
{
  fosfat_file_t *stat = NULL;

  if (!file)
    return NULL;

  stat = malloc (sizeof (fosfat_file_t));
  if (!stat)
    return NULL;

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
  if (stat->att.isdel)
  {
    strncpy (stat->name, (char *) file->name + 1, sizeof (stat->name));
    stat->name[15] = '\0';
  }
  else
    strncpy (stat->name, (char *) file->name, sizeof (stat->name));
  lc (stat->name);

  stat->next_file = NULL;

  return stat;
}

/*
 * Return all informations on one file.
 *
 * This function is high level.
 *
 * fosfat       handle
 * location     file in the path
 * return the stat
 */
fosfat_file_t *
fosfat_get_stat (fosfat_t *fosfat, const char *location)
{
  fosfat_blf_t *entry;
  fosfat_file_t *stat = NULL;

  if (!fosfat || !location)
    return NULL;

  entry = fosfat_search_insys (fosfat, location, S_BLF);
  if (entry)
  {
    stat = fosfat_stat (entry);
    free (entry);
  }

  if (!stat)
    foslog (FOSLOG_WARNING, "stat of \"%s\" not found", location);

  return stat;
}

/*
 * Return a linked list with all files of a directory.
 *
 * This function is high level.
 *
 * fosfat       handle
 * location     directory in the path
 * return the linked list
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

  if (!fosfat || !location)
    return NULL;

  if (!fosfat_isdir (fosfat, location))
  {
    foslog (FOSLOG_WARNING, "directory \"%s\" is unknown", location);
    return NULL;
  }

  dir = fosfat_search_insys (fosfat, location, S_BD);
  if (!dir)
    return NULL;

  files = dir->first_bl;
  if (!files)
  {
    fosfat_free_dir (dir);
    return NULL;
  }

  do
  {
    /* Check all files in the BL */
    for (i = 0; i < FOSFAT_NBL; i++)
    {
      if (fosfat_in_issystem (&files->file[i]))
      {
        if (!strcasecmp ((char *) files->file[i].name, "sys_list"))
        {
          sysdir = fosfat_stat (&files->file[i]);
          strcpy (sysdir->name, "..dir");
        }
        continue;
      }

      if (!fosfat_in_isopenexm (&files->file[i]))
        continue;

      if (fosfat->viewdel || fosfat_in_isnotdel (&files->file[i]))
      {
        /* Complete the linked list with all files */
        if (listdir)
        {
          listdir->next_file = fosfat_stat (&files->file[i]);
          listdir = listdir->next_file;
        }
        else
        {
          firstfile = fosfat_stat (&files->file[i]);
          listdir = firstfile;
        }
      }
    }
  } while ((files = files->next_bl));

  fosfat_free_dir (dir);

  if (sysdir)
  {
    sysdir->next_file = firstfile;
    res = sysdir;
  }
  else
    res = firstfile;

  if (res)
    foslog (FOSLOG_NOTICE, "directory \"%s\" is read successfully", location);

  return res;
}

/*
 * Get a file and put this in a location on the PC.
 *
 * This function create a copy from src to dst. An output variable can be
 * used to print the current size for each PTS.
 *
 * fosfat       handle
 * src          source on the Smaky disk
 * dst          destination on your PC
 * output       TRUE to print the size
 * return a boolean (true for success)
 */
int
fosfat_get_file (fosfat_t *fosfat, const char *src,
                 const char *dst, int output)
{
  int res = 0;
  fosfat_blf_t *file;
  fosfat_bd_t *file2;

  if (!fosfat || !src || !dst)
    return 0;

  file = fosfat_search_insys (fosfat, src, S_BLF);
  if (file)
  {
    if (!fosfat_in_isdir (file))
    {
      file2 = fosfat_read_file (fosfat, c2l (file->pt, sizeof (file->pt)));

      if (file2 && fosfat_get (fosfat, file2, dst, output, 0))
        res = 1;

      fosfat_free_file (file2);
    }

    free (file);
  }

  if (!res)
    foslog (FOSLOG_WARNING, "file \"%s\" cannot be copied", src);
  else
    foslog (FOSLOG_NOTICE, "get file \"%s\" and save to \"%s\"", src, dst);

  return res;
}

/*
 * Get a buffer from a file in the FOS.
 *
 * The buffer can be selected with an offset in the file and with a size.
 *
 * fosfat       handle
 * path         source on the Smaky disk
 * offset       start byte in the file
 * size         length of the buffer
 * return the buffer with the data
 */
uint8_t *
fosfat_get_buffer (fosfat_t *fosfat, const char *path, int offset, int size)
{
  uint8_t *buffer = NULL;
  fosfat_blf_t *file;
  fosfat_bd_t *file2;

  if (!fosfat || !path)
    return NULL;

  file = fosfat_search_insys (fosfat, path, S_BLF);
  if (file && !fosfat_in_isdir (file))
  {
    buffer = calloc (1, size);

    if (buffer)
    {
      file2 = fosfat_read_file (fosfat, c2l (file->pt, sizeof (file->pt)));

      if (file2)
        fosfat_get (fosfat, file2, NULL, 0, 1, offset, size, buffer);
      else
      {
        free (buffer);
        buffer = NULL;
      }

      free (file);

      if (file2)
        fosfat_free_file (file2);
    }
  }

  if (!buffer)
    foslog (FOSLOG_ERROR, "data (offset:%i size:%i) of \"%s\" not read",
            offset, size, path);
  else
    foslog (FOSLOG_NOTICE, "data (offset:%i size:%i) of \"%s\" correctly read",
            offset, size, path);

  return buffer;
}

/*
 * Get the name of a disk.
 *
 * fosfat       handle
 * return the name
 */
char *
fosfat_diskname (fosfat_t *fosfat)
{
  fosfat_b0_t *block0;
  char *name = NULL;

  if (!fosfat)
    return NULL;

  block0 = fosfat_read_b0 (fosfat, FOSFAT_BLOCK0);
  if (block0)
  {
    name = strdup ((char *) block0->nlo);
    free (block0);
  }

  if (!name)
    foslog (FOSLOG_ERROR, "the disk name cannot be found");
  else
    foslog (FOSLOG_NOTICE, "disk name found (%s)", name);

  return name;
}

/*
 * Put file information in the cache list structure.
 *
 * file         BLF element in the BL
 * bl           BL block's number
 * return the cache for a file
 */
static cachelist_t *
fosfat_cache_file (fosfat_blf_t *file, uint32_t bl)
{
  cachelist_t *cachefile = NULL;

  if (!file)
    return NULL;

  cachefile = malloc (sizeof (cachelist_t));
  if (!cachefile)
    return NULL;

  cachefile->next = NULL;
  cachefile->sub = NULL;
  cachefile->isdir = fosfat_in_isdir (file) ? 1 : 0;
  cachefile->islink = fosfat_in_islink (file) ? 1 : 0;

  /* if the first char is NULL, then the file is deleted */
  if ((char) file->name[0] == '\0')
  {
    cachefile->isdel = 1;
    cachefile->name =
      my_strndup ((char *) file->name + 1, sizeof (file->name - 1));
  }
  else
  {
    cachefile->isdel = 0;
    cachefile->name = strdup ((char *) file->name);
  }

  cachefile->bl = bl;
  cachefile->bd = c2l (file->pt, sizeof (file->pt));

  return cachefile;
}

/*
 * List all files on the disk to fill the global cache list.
 *
 * This function is recursive!
 *
 * fosfat       handle
 * pt           block's number of the BD
 * return the first element of the cache list.
 */
static cachelist_t *
fosfat_cache_dir (fosfat_t *fosfat, uint32_t pt)
{
  int i;
  fosfat_bd_t *dir = NULL;
  fosfat_bl_t *files;
  cachelist_t *firstfile = NULL;
  cachelist_t *list = NULL;

  if (!fosfat)
    return NULL;

  dir = fosfat_read_dir (fosfat, pt);
  if (!dir)
    return NULL;

  files = dir->first_bl;
  if (!files)
  {
    fosfat_free_dir (dir);
    return NULL;
  }

  do
  {
    /* Check all files in the BL */
    for (i = 0; i < FOSFAT_NBL; i++)
    {
      if (!fosfat_in_isopenexm (&files->file[i]))
        continue;

      if (fosfat->viewdel || fosfat_in_isnotdel(&files->file[i]))
      {
        /* Complete the linked list with all files */
        if (list)
        {
          list->next = fosfat_cache_file (&files->file[i], files->pt);
          list = list->next;
        }
        else
        {
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

  fosfat_free_dir (dir);

  if (!firstfile)
    foslog (FOSLOG_ERROR, "cache to block %i not correctly loaded", pt);

  return firstfile;
}

/*
 * Unload the cache.
 *
 * This function releases all the cache when the device is closed.
 *
 * cache        the first element of the cache list
 */
static void
fosfat_cache_unloader (cachelist_t *cache)
{
  cachelist_t *it, *tofree;

  it = cache;
  while (it)
  {
    if (it->sub)
      fosfat_cache_unloader (it->sub);

    tofree = it;
    it = it->next;
    free (tofree->name);
    free (tofree);
  }
}

/*
 * Auto detection of the FOSBOOT length.
 *
 * This function detects if the device is an hard disk or a floppy disk.
 * There is no explicit information for know that, then the CHK is tested
 * between the SYS_LIST and the first BL and a test to know if the BL's
 * pointer in the BD is right.
 *
 * fosfat       handle
 * return the type of disk or FOSFAT_ED
 */
static fosfat_disk_t
fosfat_diskauto (fosfat_t *fosfat)
{
  int i, loop = 1;
  int fboot = -1;
  fosfat_disk_t res = FOSFAT_ED;
  fosfat_bd_t *sys_list;
  fosfat_bl_t *first_bl;

  if (!fosfat)
    return FOSFAT_ED;

  fosfat->fosboot = FOSBOOT_FD;

  /* for i = 0, test with FD and when i = 1, test for HD */
  for (i = 0; loop && i < 2; i++)
  {
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
  switch (fboot)
  {
  case FOSBOOT_FD:
    res = FOSFAT_FD;
    break;

  case FOSBOOT_HD:
    res = FOSFAT_HD;
    break;

  default:
    res = FOSFAT_ED;
  }

  /* Restore the fosboot */
  fosfat->fosboot = -1;

  return res;
}

/*
 * Open the device.
 *
 * That hides the fopen processing. A device can be read like a file.
 * But for Win32, the w32disk library is used for Win9x and WinNT low
 * level access on the disk.
 *
 * dev          the device name
 * disk         disk type
 * flag         F_UNDELETE or 0 for nothing
 * return the device handle
 */
fosfat_t *
fosfat_open (const char *dev, fosfat_disk_t disk, unsigned int flag)
{
  fosfat_disk_t fboot;
  fosfat_t *fosfat = NULL;

  if (!dev)
    return NULL;

  fosfat = malloc (sizeof (fosfat_t));
  if (!fosfat)
    return NULL;

  fosfat->fosboot = -1;
  fosfat->foschk = 0;
  fosfat->viewdel = (flag & F_UNDELETE) == F_UNDELETE ? 1 : 0;
  fosfat->cachelist = NULL;

#ifdef _WIN32
  fosfat->dev = new_w32disk (*dev - 'a');
#else
  fosfat->dev = fopen (dev, "r");
#endif
  if (!fosfat->dev)
    goto err_dev;

  /* Open the device */
  foslog (FOSLOG_NOTICE, "device is opening ...");

  if (disk == FOSFAT_AD)
  {
    foslog (FOSLOG_NOTICE, "auto detection in progress ...");

    disk = fosfat_diskauto (fosfat);
    fboot = disk;
  }
  else
    fboot = fosfat_diskauto (fosfat);

  /* Test if the auto detection and the user param are the same */
  if (fboot != disk)
    foslog (FOSLOG_WARNING, "disk type forced seems to be false");

  switch (disk)
  {
  case FOSFAT_FD:
    fosfat->fosboot = FOSBOOT_FD;
    foslog (FOSLOG_NOTICE, "floppy disk selected");
    break;

  case FOSFAT_HD:
    fosfat->fosboot = FOSBOOT_HD;
    foslog (FOSLOG_NOTICE, "hard disk selected");
    break;

  case FOSFAT_ED:
    foslog (FOSLOG_ERROR, "disk auto detection for \"%s\" has failed", dev);

  default:
    goto err;
  }

  foslog (FOSLOG_NOTICE, "cache file is loading ...");

  fosfat->cachelist = fosfat_cache_dir (fosfat, FOSFAT_SYSLIST);
  if (!fosfat->cachelist)
    goto err;

  foslog (FOSLOG_NOTICE, "fosfat is ready");

  return fosfat;

 err:
#ifdef _WIN32
  free_w32disk (fosfat->dev);
#else
  fclose (fosfat->dev);
#endif
 err_dev:
  free (fosfat);
  return NULL;
}

/*
 * Close the device.
 *
 * That hides the fclose processing. And it uses the w32disk library with
 * Win9x and WinNT.
 *
 * fosfat       handle
 */
void
fosfat_close (fosfat_t *fosfat)
{
  if (!fosfat)
    return;

  /* Unload the cache if is loaded */
  if (fosfat->cachelist)
  {
    foslog (FOSLOG_NOTICE, "cache file is unloading ...");
    fosfat_cache_unloader (fosfat->cachelist);
  }

  foslog (FOSLOG_NOTICE, "device is closing ...");

  if (fosfat->dev)
#ifdef _WIN32
    free_w32disk (fosfat->dev);
#else
    fclose (fosfat->dev);
#endif

  free (fosfat);
}
