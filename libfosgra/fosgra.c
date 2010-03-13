/*
 * FOS libfosgra: Smaky [.IMAGE|.COLOR] decoder
 * Copyright (C) 2009-2010 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "fosgra.h"

#define FOSGRA_IMAGE_HEADER_LENGTH      32
#define FOSGRA_IMAGE_HEADER_LENGTH_BIN  256
#define FOSGRA_COLOR_HEADER_LENGTH_MAP  (6 * 32)
#define FOSGRA_IMAGE_HEADER_TYP         0x81 /* .IMAGE header         */
#define FOSGRA_COLOR_HEADER_TYP         0x82 /* .COLOR header         */
#define FOSGRA_IMAGE_HEADER_BIN         0x82 /* binary header         */
#define FOSGRA_IMAGE_HEADER_BIN_TYP     0x01 /* .IMAGE header for bin */
#define FOSGRA_IMAGE_HEADER_COD_N       0x00 /* image uncoded         */
#define FOSGRA_IMAGE_HEADER_COD_C       0x04 /* image coded           */
#define FOSGRA_IMAGE_HEADER_BIT         0x01 /* 1 bit per pixel       */
#define FOSGRA_COLOR_HEADER_BIT         0x04 /* 4 bit per pixel       */
#define FOSGRA_IMAGE_HEADER_DIR         0x02 /* coord X-Y like screen */

/*
 * Header size : 32 bytes
 *
 * :  8  :  8  :  8  :  8  :  8  :  8  :  8  :  8  :
 * :_____:_____:_____:_____:_____:_____:_____:_____:
 * |_TYP_|_COD_|_BIP_|_DIR_|____DLX____|____DLY____|
 * |__________NBB__________| _ _ _ _ _ _ _ _ _ _ _ |
 * | _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ |
 * |_______________________________________________|
 *
 * Please, note that NBB concerns only the raw data and the color map with
 * .COLOR files is not considered.
 */
typedef struct fosgra_image_h_s {
  uint8_t  typ;       /* .IMAGE, .COLOR or BIN           */
  uint8_t  cod;       /* Pixel coded or not              */
  uint8_t  bip;       /* Bit per pixel                   */
  uint8_t  dir;       /* Coord.                          */
  uint16_t dlx;       /* Width in pixel (multiple of 8)  */
  uint16_t dly;       /* Height in pixel                 */
  uint32_t nbb;       /* Data length                     */
  uint8_t  res[160];  /* Unused                          */
} fosgra_image_h_t;

/*
 * Color map size : 192 bytes
 *  this map is only available with .COLOR files
 *
 * :  8  :  8  :  8  :  8  :  8  :  8  :  8  :  8  :
 * :_____:_____:_____:_____:_____:_____:_____:_____:
 * |__a__| _ _ _ _ _ _ _ _ _ _ _ |__b__| _ _ |__c__|
 * | _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ |
 * | _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  _____|
 * |_________________________________________|__d__|
 *
 * The informations provided in this header are not known. Common values
 * seem to be a:0x90, b:0xa0, c:0x10 and d:0x24|0x1a. The next 160 bytes
 * is the color map.
 *  _______________________________________________
 * |__________IDX__________|____RED____|___GREEN___|
 * |___BLUE____| _ _ _ _ _IDX_ _
 *
 * The first index is 0 and the last is 15. Each color is stored with 16
 * bits. But in all samples, only 8 bits are used.
 */
typedef struct fosgra_color_map_s {
  uint8_t undef[32];
  struct {
    uint16_t idx[2];
    uint8_t  red[2];
    uint8_t  green[2];
    uint8_t  blue[2];
  } __attribute__ ((__packed__)) map[16];
} __attribute__ ((__packed__)) fosgra_color_map_t;


#define FOSGRA_IMAGE_GET_PIXEL                               \
  do                                                         \
  {                                                          \
    if (offset + len >= offset && ucod_size <= offset + len) \
    {                                                        \
      *it_dec = buf[size];                                   \
      len--;                                                 \
      it_dec--;                                              \
    }                                                        \
  }                                                          \
  while (0)


/*
 * .IMAGE|.COLOR decoding
 * ~~~~~~~~~~~~~~~~~~~~~~
 * The images are decoded from the bottom to the top. In order to limit
 * the size of an image, when a pattern of pixels is repeated consecutively,
 * this group is coded into 16 bits where the first byte is the pattern and
 * the next byte is the value to repeat.
 * For each eight objects, a byte describes where are the coded objects and
 * where are the objects to read "as is" (normal).
 * (the first is always at the bottom|right)
 *
 * Example : 0x11 0x3A 0xF2 0x00 0x00 0xB3 0xAA 0x50 0x00 0x02 0xBB 0x00 0x17
 *
 *  0x17      : index byte -> 0b00010111
 *              '1' is coded and '0' is normal
 *  0xBB 0x00 : first coded object where 0xBB is repeated 187 times      (1)
 *  0x00 0x02 : coded where 0x02 is repeated 256 times                   (1)
 *              0x00 is equal to 256
 *  0xAA 0x00 : coded where 0x00 is repeated 170 times                   (1)
 *  0xB3      : normal, this object is single                            (0)
 *  0x00 0x00 : coded where 0x00 is repeated 256 times                   (1)
 *  0xF2      : normal                                                   (0)
 *  0x3A      : normal                                                   (0)
 *  0x11      : normal                                                   (0)
 *                                                                        ^
 *                                                                        |
 *           you can see on this column that the value is equal to 0x17 -'
 *
 * There is an exception for the last byte of the IMAGE. The first index is
 * located just before the last byte. The last byte is a value corresponding
 * to the number of bits LSB to ignore in the first index byte. After this
 * first index, there are always eight objects between each index.
 *
 * For example, if the last bytes are 0xA0 0x05; there are only 3 objects.
 *
 *
 * The image is decoded from the bottom to the top because an image is coded
 * from the top to the bottom.
 */

static uint8_t *
fosgra_image_decod (uint8_t *buf, int size, int offset, int len, int ucod_size)
{
  uint8_t idx, cnt;
  uint8_t *dec, *it_dec;

  dec = calloc (1, len);
  if (!dec)
    return NULL;

  idx = buf[size - 2] >> buf[size - 1]; /* first index */
  cnt = 8 - buf[size - 1]; /* index counter */
  size -= 3; /* go on first item */
  it_dec = dec + len - 1; /* decoded frame */

  for (; size >= 0 && len > 0; size--)
  {
    /* index ? */
    if (!cnt)
    {
      idx = buf[size];
      cnt = 8;
      continue;
    }

    /* pixels coded ? */
    if (idx & 0x01)
    {
      int l = buf[size - 1] ? buf[size - 1] : 256; /* 0x00 == 256 */
      for (; l && len > 0; l--, ucod_size--)
        FOSGRA_IMAGE_GET_PIXEL;
      size--;
    }
    /* pixels normal */
    else
    {
      FOSGRA_IMAGE_GET_PIXEL;
      ucod_size--;
    }

    /* next object */
    idx >>= 1;
    cnt--;
  }

  return dec;
}

#define SWAP_16(val)         \
  {                          \
    typeof (val) p = (val);  \
    val = (p >> 8 | p << 8); \
  }
#define SWAP_32(val)                                                           \
  {                                                                            \
    typeof (val) p = (val);                                                    \
    val =                                                                      \
      (p >> 24 | ((p >> 8) & 0x0000FF00) | ((p << 8) & 0x00FF0000) | p << 24); \
  }

static int
fosgra_header_open (uint8_t *buffer, fosgra_image_h_t *header)
{
  memcpy (header, buffer, FOSGRA_IMAGE_HEADER_LENGTH);

  /* fix endian */
  SWAP_16 (header->dlx);
  SWAP_16 (header->dly);
  SWAP_32 (header->nbb);

  if (   header->typ != FOSGRA_IMAGE_HEADER_TYP
      && header->typ != FOSGRA_COLOR_HEADER_TYP
      && header->typ != FOSGRA_IMAGE_HEADER_BIN_TYP)
    return -1;

  if (   header->cod != FOSGRA_IMAGE_HEADER_COD_N
      && header->cod != FOSGRA_IMAGE_HEADER_COD_C)
    return -1;

  if (   header->bip != FOSGRA_IMAGE_HEADER_BIT
      && header->bip != FOSGRA_COLOR_HEADER_BIT)
    return -1;

  if (header->dir != FOSGRA_IMAGE_HEADER_DIR)
    return -1;

  if (header->dlx % 8)
    return -1;

  return 0;
}

static int
fosgra_get_header (fosfat_t *fosfat, const char *path, fosgra_image_h_t *header)
{
  uint8_t *buffer;
  int res, jump = 0;

  buffer = fosfat_get_buffer (fosfat, path, 0, 1);
  if (!buffer)
    return -1;

  /* ignore BIN header if available */
  if (strstr (path, ".image\0") && *buffer == FOSGRA_IMAGE_HEADER_BIN)
    jump = FOSGRA_IMAGE_HEADER_LENGTH_BIN;
  free (buffer);

  buffer = fosfat_get_buffer (fosfat, path, jump, FOSGRA_IMAGE_HEADER_LENGTH);
  if (!buffer)
    return -1;

  res = fosgra_header_open (buffer, header);
  free (buffer);
  return res;
}

uint32_t
fosgra_color_get (fosfat_t *fosfat, const char *path, uint8_t idx)
{
  fosgra_image_h_t header;
  fosgra_color_map_t *map;
  uint8_t *buffer;
  int res;
  uint32_t color;

  if (!fosfat || !path || idx >= 16)
    return 0;

  res = fosgra_get_header (fosfat, path, &header);
  if (res)
    return 0;

  if (header.bip != FOSGRA_COLOR_HEADER_BIT)
    return 0;

  buffer = fosfat_get_buffer (fosfat, path,
                              FOSGRA_IMAGE_HEADER_LENGTH,
                              FOSGRA_COLOR_HEADER_LENGTH_MAP);
  if (!buffer)
    return 0;

  map = (fosgra_color_map_t *) buffer;
  color =   map->map[idx].red[0]   << 16
          | map->map[idx].green[0] << 8
          | map->map[idx].blue[0]  << 0;

  free (buffer);
  return color;
}

uint8_t *
fosgra_get_buffer (fosfat_t *fosfat,
                   const char *path, int offset, int size)
{
  fosgra_image_h_t header;
  uint8_t *buffer;
  uint8_t *dec;
  size_t map = 0;
  int res;
  int jump = FOSGRA_IMAGE_HEADER_LENGTH;
  int ucod_size;

  if (!fosfat || !path)
    return NULL;

  res = fosgra_get_header (fosfat, path, &header);
  if (res)
    return NULL;

  /* ignore BIN header if available */
  if (   header.bip == FOSGRA_IMAGE_HEADER_BIT
      && header.typ == FOSGRA_IMAGE_HEADER_BIN_TYP)
    jump += FOSGRA_IMAGE_HEADER_LENGTH_BIN;
  else if (header.bip == FOSGRA_COLOR_HEADER_BIT)
    map = FOSGRA_COLOR_HEADER_LENGTH_MAP;

  if (header.cod != FOSGRA_IMAGE_HEADER_COD_C)
    return fosfat_get_buffer (fosfat, path, jump + map + offset, size);

  buffer = fosfat_get_buffer (fosfat, path, jump + map, header.nbb);
  if (!buffer)
    return NULL;

  ucod_size = header.bip == 4
              ? header.dlx / 2 * header.dly  /* 2 pixels / byte */
              : header.dlx / 8 * header.dly; /* 8 pixels / byte */

  /* fix max size */
  if (offset + size > ucod_size)
    size = ucod_size - offset;

  dec = fosgra_image_decod (buffer, header.nbb,
                            offset, size, ucod_size);
  free (buffer);
  return dec;
}

void
fosgra_get_info (fosfat_t *fosfat,
                 const char *path, uint16_t *x, uint16_t *y, uint8_t *bpp)
{
  fosgra_image_h_t header;
  int res;

  if (!fosfat || !path)
    return;

  res = fosgra_get_header (fosfat, path, &header);
  if (res)
    return;

  if (x)
    *x = header.dlx;
  if (y)
    *y = header.dly;
  if (bpp)
    *bpp = header.bip;
}

int
fosgra_is_image (fosfat_t *fosfat, const char *path)
{
  fosgra_image_h_t header;

  if (!fosfat || !path)
    return 0;

  return !fosgra_get_header (fosfat, path, &header);
}
