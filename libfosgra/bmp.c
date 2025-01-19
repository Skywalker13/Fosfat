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
get_bmp1_size (int width, int height, int *bpr, int *pbpr)
{
  *bpr  = (width + 7) / 8;    /* Number of bytes per row */
  *pbpr = (*bpr + 3) / 4 * 4; /* Alignment on 4 bytes */
  return sizeof(bmp_file_header_t)
       + sizeof(bmp_info_header_t) + 2 * sizeof(rgb_quad_t) + *pbpr * height;
}

void
create_bmp1_buffer(const uint8_t *input,
                   int width, int height, uint8_t **output, size_t *output_size)
{
  int bpr;
  int pbpr;

  size_t bmp_size = get_bmp1_size (width, height, &bpr, &pbpr);

  // Allocation du buffer BMP
  *output = (uint8_t *)malloc(bmp_size);
  *output_size = bmp_size;

  // Remplissage de l'en-tête du fichier BMP
  bmp_file_header_t *bfh = (bmp_file_header_t *)(*output);
  bfh->type = 0x4D42; // 'BM'
  bfh->size = bmp_size;
  bfh->reserved1 = 0;
  bfh->reserved2 = 0;
  bfh->off_bits = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + 2 * sizeof(rgb_quad_t);

  // Remplissage de l'en-tête DIB
  bmp_info_header_t *bih = (bmp_info_header_t *)(*output + sizeof(bmp_file_header_t));
  bih->size = sizeof(bmp_info_header_t);
  bih->width = width;
  bih->height = -height;
  bih->planes = 1;
  bih->bit_count = 1;
  bih->compression = 0;
  bih->size_image = 0;
  bih->x_pels_per_meter = 0;
  bih->y_pels_per_meter = 0;
  bih->clr_used = 0;
  bih->clr_important = 0;

  // Remplissage de la palette de couleurs
  rgb_quad_t *palette = (rgb_quad_t *)(*output + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));
  palette[0].blue = 255;
  palette[0].green = 255;
  palette[0].red = 255;
  palette[0].reserved = 0;
  palette[1].blue = 0;
  palette[1].green = 0;
  palette[1].red = 0;
  palette[1].reserved = 0;

  // Copie des données de l'image
  uint8_t *image_data = *output + sizeof(bmp_file_header_t)
                      + sizeof(bmp_info_header_t) + 2 * sizeof(rgb_quad_t);
  for (int y = 0; y < height; y++) {
    memcpy(image_data + y * pbpr, input + y * bpr, bpr);
    memset(image_data + y * pbpr + bpr, 0, pbpr - bpr);
  }
}

size_t
get_bmp4_size (int width, int height, int *bpr, int *image_size, int *header_size)
{
  *bpr        = (width + 1) / 2; /* Number of bytes per row */
  *image_size = *bpr * height; /* Image size in bytes */
  int palette_size = 16 * sizeof(rgb_quad_t); /* Palette size */
  *header_size = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + palette_size;
  return *header_size + *image_size;
}

void
create_bmp4_buffer(const uint8_t *input, const uint32_t *pal,
                   int width, int height, uint8_t **output, size_t *output_size)
{
  int bpr;
  int image_size;
  int header_size;

  size_t total_size =
    get_bmp4_size (width, height, &bpr, &image_size, &header_size);

  // Allouer de la mémoire pour le buffer de sortie
  *output = (uint8_t *)malloc(total_size);
  *output_size = total_size;

  // Remplir l'en-tête de fichier BMP
  bmp_file_header_t *bfh = (bmp_file_header_t *)(*output);
  bfh->type = 0x4D42; // 'BM'
  bfh->size = total_size;
  bfh->reserved1 = 0;
  bfh->reserved2 = 0;
  bfh->off_bits = header_size;

  // Remplir l'en-tête DIB
  bmp_info_header_t *bih = (bmp_info_header_t *)(*output + sizeof(bmp_file_header_t));
  bih->size = sizeof(bmp_info_header_t);
  bih->width = width;
  bih->height = -height;
  bih->planes = 1;
  bih->bit_count = 4;
  bih->compression = 0;
  bih->size_image = image_size;
  bih->x_pels_per_meter = 0;
  bih->y_pels_per_meter = 0;
  bih->clr_used = 16;
  bih->clr_important = 0;

  // Remplir la palette
  rgb_quad_t *palette = (rgb_quad_t *)(*output + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));
  for (int i = 0; i < 16; i++) {
    palette[i].blue  = pal[i] >> 16 & 0xFF;
    palette[i].green = pal[i] >>  8 & 0xFF;
    palette[i].red   = pal[i] >>  0 & 0xFF;
    palette[i].reserved = 0;
  }

  // Copier les données de l'image
  uint8_t *image_data = *output + header_size;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x += 2) {
      uint8_t byte = input[y * bpr + x / 2];
      image_data[y * bpr + x / 2] = byte;
    }
  }
}
