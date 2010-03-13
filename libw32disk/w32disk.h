/*
 * libw32disk: C wrapper for Disk C++ Class (Window$ DLL)
 * Copyright (C) 2007-2008 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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

#include <stdlib.h>

typedef struct win32disk_s win32disk_t;

#ifdef __cplusplus
extern "C" {
#endif

win32disk_t *new_w32disk (unsigned int drive_index);
void free_w32disk (win32disk_t *disk);

size_t w32disk_sectorsize (win32disk_t *disk);
int w32disk_readsectors (win32disk_t *disk, void *buffer,
                                unsigned long sector_index, size_t csectors);
unsigned int w32disk_getdriveindex (win32disk_t *disk);
int w32disk_valid (win32disk_t *disk);

#ifdef __cplusplus
}
#endif

#endif /* W32DISK_H */
