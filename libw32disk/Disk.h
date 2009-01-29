/* No license specified, original author unknown */

#ifndef DISK_HXX
#define DISK_HXX

#include <windows.h>
#include <cstddef>

struct AutoHandle {
  AutoHandle ():handle (NULL)
  {
  }

  AutoHandle (HANDLE h):handle (h)
  {
  }

  AutoHandle& operator = (HANDLE h)
  {
    Reset ();
    handle = h;
    return *this;
  }

  operator
  HANDLE() const
  {
    return handle;
  }

  bool
  Valid() const
  {
    return handle != INVALID_HANDLE_VALUE;
  }

  ~AutoHandle()
  {
    Reset ();
  }

  private:
    void
    Reset ()
    {
      if (Valid ()) CloseHandle (handle);
    }

    void operator = (const AutoHandle&);
    AutoHandle (const AutoHandle&);
    HANDLE handle;
};

/* represents an open disk device */
class Disk {
  public:
    Disk (unsigned DriveIndex);
    ~Disk ();
    std::size_t SectorSize () const;
    bool ReadSectors (void *Buffer, unsigned long SectorIndex,
                      std::size_t cSectors) const;
    unsigned GetDriveIndex () const;
    bool Valid() const;

  private:
    AutoHandle Win95_vwin32;
    AutoHandle WinNT_Disk;
    unsigned DriveIndex;
    std::size_t sectorSize;
};

#endif /* DISK_HXX */
