/* No license specified, original author unknown */

#include <windows.h>
#include <winioctl.h>

#include "Disk.h"

static inline void
Multiply512 (DWORD Input, DWORD& lowResult, DWORD& highResult)
{
  lowResult = Input << 9;
  highResult = Input >> (32 - 9);
}

static inline HANDLE
Get_Win95_vwin32 ()
{
  return CreateFile ("\\\\.\\vwin32", 0, 0, NULL, 0,
                                      FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

static HANDLE
Get_WinNT_Disk (unsigned DriveIndex)
{
  char DiskPath[] = "\\\\.\\A:";
  DiskPath[4] = 'A' + DriveIndex;
  return CreateFile (DiskPath, GENERIC_READ, FILE_SHARE_READ
                               | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);
}

static bool
Win95_ReadSectors (HANDLE vwin32Device, unsigned DriveIndex,
                   void *Buffer, unsigned long FirstSectorToRead,
                   std::size_t cSectorsToRead)
{
  static const DWORD VWIN32_DIOC_DOS_DRIVEINFO = 6;

  #pragma pack (1)
  struct ControlBlock {
    DWORD StartingSector;
    WORD NumberOfSectors;
    LPVOID pBuffer;
  };
  #pragma pack ()

  struct IoRegisters {
    DWORD EBX, EDX, ECX, EAX, EDI, ESI, Flags;
  };

  if (vwin32Device == INVALID_HANDLE_VALUE)
    return false;
  if (cSectorsToRead > 65536L)
    return false;

  ControlBlock control_block = {FirstSectorToRead, cSectorsToRead, Buffer};
  IoRegisters regs = {0, 0, 0, 0, 0, 0, 0};

  //-----------------------------------------------------------
  // SI contains read/write mode flags
  // SI=0h for read and SI=1h for write
  // CX must be equal to ffffh for
  // int 21h's 7305h extention
  // DS:BX -> base addr of the
  // control block structure
  // DL must contain the drive number
  // (01h=A:, 02h=B: etc)
  //-----------------------------------------------------------

  regs.EAX = 0x7305;
  regs.ECX = static_cast<DWORD> (-1);
  regs.EBX = reinterpret_cast<DWORD> (&control_block);
  regs.EDX = DriveIndex + 1;
  regs.ESI = 0;

  DWORD cbOutputBuffer;
  //  6 == VWIN32_DIOC_DOS_DRIVEINFO
  return
    DeviceIoControl (vwin32Device, VWIN32_DIOC_DOS_DRIVEINFO, &regs,
                     sizeof (regs), &regs, sizeof (regs),
                     &cbOutputBuffer, NULL) && !(regs.Flags & 1);
}

static bool
WinNT_ReadSectors (HANDLE DiskDevice, void *Buffer,
                   unsigned long FirstSectorToRead,
                   std::size_t cSectorsToRead)
{
  if (DiskDevice == INVALID_HANDLE_VALUE)
    return false;

  DWORD ByteIndex_low, ByteIndex_high;
  DWORD ByteCount, ByteCount_high;

  Multiply512 (FirstSectorToRead, ByteIndex_low, ByteIndex_high);
  Multiply512 (cSectorsToRead, ByteCount, ByteCount_high);

  if (ByteCount_high != 0)
    return false;

  DWORD cbRead;
  LONG lByteIndex_high = ByteIndex_high;

  SetLastError (ERROR_SUCCESS);

  if (static_cast<DWORD> (-1)
      == SetFilePointer (DiskDevice, ByteIndex_low,
                         &lByteIndex_high, FILE_BEGIN)
      && GetLastError () != ERROR_SUCCESS)
    return false;

  return ReadFile (DiskDevice, Buffer, ByteCount, &cbRead, NULL)
                   && cbRead == ByteCount;
}


unsigned
Disk::GetDriveIndex () const
{
  return DriveIndex;
}

Disk::Disk (unsigned driveIndex):Win95_vwin32 (Get_Win95_vwin32()),
                                 WinNT_Disk (NULL), DriveIndex (driveIndex)
{
  sectorSize = 512;

  if (!Win95_vwin32.Valid ())
    WinNT_Disk = Get_WinNT_Disk (DriveIndex);

  if (!Win95_vwin32.Valid () && WinNT_Disk.Valid ()) {
    DISK_GEOMETRY Geometry;
    DWORD cbOut;

    if (DeviceIoControl (WinNT_Disk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                         &Geometry, sizeof (Geometry), &cbOut, NULL))
      sectorSize = Geometry.BytesPerSector;
  }
}

Disk::~Disk ()
{
}

bool
Disk::Valid () const
{
  return Win95_vwin32.Valid () || WinNT_Disk.Valid ();
}

bool
Disk::ReadSectors (void *Buffer, unsigned long SectorIndex,
                   std::size_t cSectors) const
{
  if (Win95_vwin32.Valid ())
    return Win95_ReadSectors (Win95_vwin32, DriveIndex,
                              Buffer, SectorIndex, cSectors);
  else
    return WinNT_Disk.Valid ()
           && WinNT_ReadSectors (WinNT_Disk, Buffer, SectorIndex, cSectors);
}

std::size_t
Disk::SectorSize () const
{
  return sectorSize;
}
