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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <winioctl.h>

#include "w32disk.h"

struct w32disk_s {
  HANDLE win95_vwin32;
  HANDLE winnt_disk;

  unsigned int drive_idx;
  size_t       sector_size;
};

#define ISHDL_VALID(h) ((h) != INVALID_HANDLE_VALUE)


static inline void
multiply_512 (DWORD input, DWORD *low_result, DWORD *high_result)
{
  *low_result  = input << 9;
  *high_result = input >> (32 - 9);
}

static HANDLE
win95_get_vwin32 (void)
{
  return CreateFile ("\\\\.\\vwin32", 0,
                     0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

static HANDLE
winnt_get_disk (unsigned int drive_idx)
{
  char path_disk[] = "\\\\.\\A:";

  path_disk[4] = 'A' + drive_idx;
  return CreateFile (path_disk, GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                     OPEN_EXISTING, 0, NULL);
}

static int
win95_read_sectors (HANDLE vwin32_device, unsigned int drive_idx, void *buffer,
                    unsigned long int first_sector2read, size_t c_sectors2read)
{
  const DWORD vwin32_dioc_dos_driveinfo = 6;
  DWORD cb_outputbuffer;
  BOOL io_ctl;

  struct ctl_block_s {
    DWORD  sec_start;
    WORD   sec_nb;
    LPVOID pbuffer;
  } __attribute__ ((__packed__)) ctl_block;

  struct io_reg_s {
    DWORD ebx, edx, ecx, eax, edi, esi, flags;
  } regs = {
    0, 0, 0, 0, 0, 0, 0
  };

  if (vwin32_device == INVALID_HANDLE_VALUE)
    return 0;

  if (c_sectors2read > 65536L)
    return 0;

  ctl_block.sec_start = first_sector2read;
  ctl_block.sec_nb    = c_sectors2read;
  ctl_block.pbuffer   = buffer;

  /*
   * SI contains read/write mode flags
   *   SI = 0h for read and SI = 1h for write
   * CX must be equal to ffffh for int 21h's 7305h extention
   * DS:BX -> base addr of the control block structure
   * DL must contain the drive number (01h=A:, 02h=B: etc)
   */

  regs.eax = 0x7305;
  regs.ebx = (DWORD) &ctl_block;
  regs.ecx = (DWORD) -1;
  regs.edx = drive_idx + 1;

  io_ctl = DeviceIoControl (vwin32_device, vwin32_dioc_dos_driveinfo, &regs,
                            sizeof (regs), &regs, sizeof (regs),
                            &cb_outputbuffer, NULL);
  return io_ctl && !(regs.flags & 1);
}

static int
winnt_read_sectors (HANDLE disk_device, void *buffer,
                    unsigned long int first_sector2read,
                    size_t c_sectors2read)
{
  DWORD byte_idx_low, byte_idx_high;
  DWORD byte_cnt,     byte_cnt_high;
  DWORD cb_read;
  DWORD setfileptr;
  LONG  lbyte_idx_high;

  if (disk_device == INVALID_HANDLE_VALUE)
    return 0;

  multiply_512 (first_sector2read, &byte_idx_low, &byte_idx_high);
  multiply_512 (c_sectors2read,    &byte_cnt,     &byte_cnt_high);

  if (byte_cnt_high)
    return 0;

  lbyte_idx_high = byte_idx_high;

  SetLastError (ERROR_SUCCESS);

  setfileptr =
    SetFilePointer (disk_device, byte_idx_low, &lbyte_idx_high, FILE_BEGIN);

  if (setfileptr == (DWORD) -1 && GetLastError () != ERROR_SUCCESS)
    return 0;

  return ReadFile (disk_device, buffer, byte_cnt, &cb_read, NULL)
         && cb_read == byte_cnt;
}


unsigned int
w32disk_getdriveindex (w32disk_t *disk)
{
  return disk ? disk->drive_idx : 0;
}

w32disk_t *
w32disk_new (unsigned int drive_idx)
{
  w32disk_t *disk;

  disk = calloc (1, sizeof (w32disk_t));
  if (!disk)
    return NULL;

  disk->sector_size  = 512;
  disk->drive_idx    = drive_idx;
  disk->win95_vwin32 = win95_get_vwin32 ();

  if (!ISHDL_VALID (disk->win95_vwin32))
    disk->winnt_disk = winnt_get_disk (drive_idx);

  if (!ISHDL_VALID (disk->win95_vwin32) && ISHDL_VALID (disk->winnt_disk))
  {
    DISK_GEOMETRY geometry;
    DWORD cb_out;
    BOOL ioctl;

    ioctl = DeviceIoControl (disk->winnt_disk,
                             IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                             &geometry, sizeof (geometry), &cb_out, NULL);
    if (ioctl)
      disk->sector_size = geometry.BytesPerSector;
  }

  return disk;
}

void
w32disk_free (w32disk_t *disk)
{
  if (!disk)
    return;

  if (ISHDL_VALID (disk->win95_vwin32))
    CloseHandle (disk->win95_vwin32);
  if (ISHDL_VALID (disk->winnt_disk))
    CloseHandle (disk->winnt_disk);

  free (disk);
}

int
w32disk_valid (w32disk_t *disk)
{
  if (!disk)
    return 0;

  return ISHDL_VALID (disk->win95_vwin32) || ISHDL_VALID (disk->winnt_disk);
}

int
w32disk_readsectors (w32disk_t *disk, void *buffer,
                     unsigned long int sector_idx, size_t c_sectors)
{
  if (!disk)
    return 0;

  if (ISHDL_VALID (disk->win95_vwin32))
    return win95_read_sectors (disk->win95_vwin32,
                               disk->drive_idx, buffer, sector_idx, c_sectors);

  if (ISHDL_VALID (disk->winnt_disk))
    return winnt_read_sectors (disk->winnt_disk, buffer, sector_idx, c_sectors);

  return 0;
}

size_t
w32disk_sectorsize (w32disk_t *disk)
{
  return disk ? disk->sector_size : 0;
}
