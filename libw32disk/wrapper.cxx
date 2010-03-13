/*
 * wrapper: C wrapper for Disk C++ Class (Window$ DLL)
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

#include <stdlib.h>

/* C++ Class for drive low level access (Win9x/NT) */
#include "Disk.h"

#include "w32disk.h"


/* Structure for the C */
struct win32disk_s {
  Disk *wdisk;
};


/*
 * Create a new device object.
 *
 * The devices are numbers like:
 *  0 for a:\
 *  1 for b:\
 *  2 for c:\
 *  etc,..
 *
 * driver_index to open.
 * return the new device object.
 */
win32disk_t *
new_w32disk (unsigned int drive_index)
{
  win32disk_t *disk = NULL;

  if ((disk = (win32disk_t *) malloc (sizeof (win32disk_t)))) {
    disk->wdisk = new Disk (drive_index);
  }

  return disk;
}

/*
 * Free the device object.
 *
 * disk         structure for the wrapper.
 */
void
free_w32disk (win32disk_t *disk)
{
  Disk *wdisk;

  if (!disk)
    return;

  wdisk = disk->wdisk;

  if (wdisk)
    delete wdisk;

  free (disk);
}

/*
 * Get the sector's size.
 *
 * The common value is 512 bytes.
 *
 * disk         structure for the wrapper.
 * return the size.
 */
size_t
w32disk_sectorsize (win32disk_t *disk)
{
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (size_t) wdisk->SectorSize ();
}

/*
 * Read some sectors on the drive.
 *
 * The common value for csectors is 512.
 *
 * disk         structure for the wrapper.
 * buffer       where the data are put.
 * sector_index where the data are got.
 * csectors     the number of sectors (length).
 * return 0 if error.
 */
int
w32disk_readsectors (win32disk_t *disk, void *buffer,
                     unsigned long sector_index, size_t csectors)
{
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (int) wdisk->ReadSectors (buffer, sector_index,
                                   (std::size_t) csectors);
}

/*
 * Get the drive index number.
 *
 * The number must be the same as for create the object with new_w32disk().
 *
 * disk         structure for the wrapper.
 * return the index.
 */
unsigned int
w32disk_getdriveindex (win32disk_t *disk)
{
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (int) wdisk->GetDriveIndex ();
}

/*
 * Test if the device is valid.
 *
 * disk         structure for the wrapper.
 * return 0 if invalid.
 */
int
w32disk_valid (win32disk_t *disk)
{
  Disk *wdisk;

  if (!disk && !disk->wdisk)
    return 0;

  wdisk = disk->wdisk;

  return (int) wdisk->Valid ();
}
