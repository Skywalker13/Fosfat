/*
 * libw32disk: Win95 and WinNT low level disk access.
 * Copyright (C) 2007-2010 Mathieu Schroeter <mathieu@schroetersa.ch>
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

#ifndef W32DISK_H
#define W32DISK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct w32disk_s w32disk_t;

w32disk_t *w32disk_new (unsigned int drive_index);
void w32disk_free (w32disk_t *disk);

size_t w32disk_sectorsize (w32disk_t *disk);
int w32disk_readsectors (w32disk_t *disk, void *buffer,
                         unsigned long int sector_index, size_t csectors);
unsigned int w32disk_getdriveindex (w32disk_t *disk);
int w32disk_valid (w32disk_t *disk);

#ifdef __cplusplus
}
#endif

#endif /* W32DISK_H */
