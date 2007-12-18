/*
 * wrapper: C wrapper for Disk C++ Class (Window$ DLL)
 * Copyright (C) 2007 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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

#include <stdlib.h>

/* C++ Class for drive low level access (Win9x/NT) */
#include "Disk.hxx"

#include "w32disk.h"


/** Structure for the C */
struct win32disk_s {
  Disk *wdisk;
};


/**
 * \brief Create a new device object.
 *
 * The devices are numbers like:
 *  0 for a:\
 *  1 for b:\
 *  2 for c:\
 *  etc,..
 *
 * \param driver_index to open.
 * \return the new device object.
 */
EXPORT win32disk_t *new_w32disk(unsigned int drive_index) {
  win32disk_t *disk = NULL;

  if ((disk = (win32disk_t *)malloc(sizeof(win32disk_t)))) {
    disk->wdisk = new Disk(drive_index);
  }

  return disk;
}

/**
 * \brief Free the device object.
 *
 * \param disk structure for the wrapper.
 */
EXPORT void free_w32disk(win32disk_t *disk) {
  Disk *wdisk;

  if (!disk)
    return;

  wdisk = disk->wdisk;

  if (wdisk)
    delete wdisk;

  free(disk);
}

/**
 * \brief Get the sector's size.
 *
 * The common value is 512 bytes.
 *
 * \param disk structure for the wrapper.
 * \return the size.
 */
EXPORT size_t w32disk_sectorsize(win32disk_t *disk) {
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (size_t)wdisk->SectorSize();
}

/**
 * \brief Read some sectors on the drive.
 *
 * The common value for csectors is 512.
 *
 * \param disk structure for the wrapper.
 * \param buffer where the data are put.
 * \param sector_index where the data are got.
 * \param csectors the number of sectors (length).
 * \return 0 if error.
 */
EXPORT int w32disk_readsectors(win32disk_t *disk, void *buffer,
                               unsigned long sector_index, size_t csectors)
{
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (int)wdisk->ReadSectors(buffer, sector_index, (std::size_t)csectors);
}

/**
 * \brief Get the drive index number.
 *
 * The number must be the same as for create the object with new_w32disk().
 *
 * \param disk structure for the wrapper.
 * \return the index.
 */
EXPORT unsigned int w32disk_getdriveindex(win32disk_t *disk) {
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (int)wdisk->GetDriveIndex();
}

/**
 * \brief Test if the device is valid.
 *
 * \param disk structure for the wrapper.
 * \return 0 if invalid.
 */
EXPORT int w32disk_valid(win32disk_t *disk) {
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (int)wdisk->Valid();
}
