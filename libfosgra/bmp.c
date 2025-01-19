/*
 * FOS libfosgra: Smaky [.IMAGE|.COLOR] decoder
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

// Structure de l'en-tête du fichier BMP
typedef struct {
  uint16_t type;
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t off_bits;
} __attribute__ ((packed)) bmp_file_header_t;

// Structure de l'en-tête DIB (Device Independent Bitmap)
typedef struct {
  uint32_t size;
  int32_t  width;
  int32_t  height;
  uint16_t planes;
  uint16_t bit_count;
  uint32_t compression;
  uint32_t size_image;
  int32_t  x_pels_per_meter;
  int32_t  y_pels_per_meter;
  uint32_t clr_used;
  uint32_t clr_important;
} __attribute__ ((packed)) bmp_info_header_t;

// Structure de la palette de couleurs pour le format 1 bit par pixel
typedef struct {
  uint8_t blue;
  uint8_t green;
  uint8_t red;
  uint8_t reserved;
} __attribute__ ((packed)) rgb_quad_t;

size_t
fosgra_bmp1_sizes (int width, int height, int *bpr, int *pbpr, int *header_size)
{
  *bpr  = (width + 7) / 8;    /* Number of bytes per row */
  *pbpr = (*bpr + 3) / 4 * 4; /* Alignment on 4 bytes */
  *header_size = sizeof(bmp_file_header_t)
               + sizeof(bmp_info_header_t) + 2 * sizeof(rgb_quad_t);
  return *header_size + *pbpr * height;
}

static void
bmp_fill_header (bmp_file_header_t *bfh, size_t bmp_size, int header_size)
{
  bfh->type      = 0x4D42; // 'BM'
  bfh->size      = bmp_size;
  bfh->reserved1 = 0;
  bfh->reserved2 = 0;
  bfh->off_bits  = header_size;
}

static void
bmp_fill_dib_header (bmp_info_header_t *bih,
                     int bpp, int width, int height, int image_size)
{
  bih->size             = sizeof (bmp_info_header_t);
  bih->width            = width;
  bih->height           = -height;
  bih->planes           = 1;
  bih->bit_count        = bpp;
  bih->compression      = 0;
  bih->size_image       = bpp == 1 ? 0 : image_size;
  bih->x_pels_per_meter = 0;
  bih->y_pels_per_meter = 0;
  bih->clr_used         = bpp == 1 ? 0 : 16;
  bih->clr_important    = 0;
}

uint8_t *
fosgra_bmp1_buffer (const uint8_t *input,
                    int width, int height, size_t *output_size)
{
  int bpr;
  int pbpr;
  int header_size;
  uint8_t *output = NULL;

  size_t bmp_size = fosgra_bmp1_sizes (width, height, &bpr, &pbpr, &header_size);

  output = malloc (bmp_size);
  if (!output)
    return NULL;

  *output_size = bmp_size;

  /* BMP file header */
  bmp_file_header_t *bfh = (bmp_file_header_t *) (output);
  bmp_fill_header (bfh, bmp_size, header_size);

  /* DIB header */
  bmp_info_header_t *bih =
    (bmp_info_header_t *) (output + sizeof(bmp_file_header_t));
  bmp_fill_dib_header (bih, 1, width, height, 0);

  /* Monochrome palette (black and white) */
  rgb_quad_t *palette = (rgb_quad_t *) (output + sizeof(bmp_file_header_t)
                                               + sizeof(bmp_info_header_t));
  for (int i = 0; i < 2; ++i)
  {
    palette[i].blue  = 0xFF * !i;
    palette[i].green = 0xFF * !i;
    palette[i].red   = 0xFF * !i;
    palette[i].reserved = 0;
  }

  uint8_t *image_data = output + sizeof (bmp_file_header_t)
                      + sizeof (bmp_info_header_t) + 2 * sizeof (rgb_quad_t);
  for (int y = 0; y < height; ++y)
  {
    memcpy (image_data + y * pbpr, input + y * bpr, bpr);
    memset (image_data + y * pbpr + bpr, 0, pbpr - bpr);
  }

  return output;
}

size_t
fosgra_bmp4_sizes (int width, int height, int *bpr, int *image_size, int *header_size)
{
  *bpr        = (width + 1) / 2; /* Number of bytes per row */
  *image_size = *bpr * height;   /* Image size in bytes */
  int palette_size = 16 * sizeof (rgb_quad_t); /* Palette size */
  *header_size = sizeof (bmp_file_header_t)
               + sizeof (bmp_info_header_t) + palette_size;
  return *header_size + *image_size;
}

uint8_t *
fosgra_bmp4_buffer (const uint8_t *input, const uint32_t *pal,
                    int width, int height, size_t *output_size)
{
  int bpr;
  int image_size;
  int header_size;
  uint8_t *output = NULL;

  size_t bmp_size =
    fosgra_bmp4_sizes (width, height, &bpr, &image_size, &header_size);

  output = malloc (bmp_size);
  if (!output)
    return NULL;

  *output_size = bmp_size;

  /* BMP file header */
  bmp_file_header_t *bfh = (bmp_file_header_t *) (output);
  bmp_fill_header (bfh, bmp_size, header_size);

  /* DIB header */
  bmp_info_header_t *bih =
    (bmp_info_header_t *) (output + sizeof (bmp_file_header_t));
  bmp_fill_dib_header (bih, 4, width, height, image_size);

  /* 16 colors palette */
  rgb_quad_t *palette = (rgb_quad_t *) (output + sizeof (bmp_file_header_t)
                                               + sizeof (bmp_info_header_t));
  for (int i = 0; i < 16; ++i)
  {
    palette[i].blue  = pal[i] >> 16 & 0xFF;
    palette[i].green = pal[i] >>  8 & 0xFF;
    palette[i].red   = pal[i] >>  0 & 0xFF;
    palette[i].reserved = 0;
  }

  uint8_t *image_data = output + header_size;
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; x += 2)
      image_data[y * bpr + x / 2] = input[y * bpr + x / 2];

  return output;
}
