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

#ifndef FOSGRA_BMP_H
#define FOSGRA_BMP_H

/**
 * \file bmp.h
 *
 * bmp internal header.
 */

size_t fosgra_bmp1_sizes (int width, int height,
                          int *bpr, int *pbpr, int *header_size);
size_t fosgra_bmp4_sizes (int width, int height,
                          int *bpr, int *image_size, int *header_size);

void fosgra_bmp1_buffer (const uint8_t *input, int width, int height,
                         uint8_t **output, size_t *output_size);
void fosgra_bmp4_buffer (const uint8_t *input, const uint32_t *pal,
                         int width, int height,
                         uint8_t **output, size_t *output_size);

#endif /* FOSGRA_BMP_H */
