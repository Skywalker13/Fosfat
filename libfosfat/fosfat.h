/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2006-2008 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
 *    And to Epsitec SA for the Smaky computers
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

#ifndef FOSFAT_H
#define FOSFAT_H

#define FOSFAT_NAMELGT  17

#define F_UNDELETE      (1 << 0)

/** Disk types */
typedef enum disk_type {
  FOSFAT_FD,                   /*!< Floppy Disk           */
  FOSFAT_HD,                   /*!< Hard Disk             */
  FOSFAT_AD,                   /*!< Auto Detection        */
  FOSFAT_ED                    /*!< Auto Detection fails  */
} fosfat_disk_t;

/** Time */
typedef struct time_s {
  short int year;
  short int month;
  short int day;
  short int hour;
  short int minute;
  short int second;
} fosfat_time_t;

/** Attributes for a file (or dir) */
typedef struct att_s {
  int isdir     : 1;
  int isvisible : 1;
  int isencoded : 1;
  int islink    : 1;
  int isdel     : 1;
} fosfat_att_t;

/** List of files in a directory */
typedef struct file_info_s {
  char name[FOSFAT_NAMELGT];  /*!< File name              */
  int size;                   /*!< File size              */
  fosfat_att_t att;           /*!< File attributes        */
  fosfat_time_t time_c;       /*!< Creation date          */
  fosfat_time_t time_w;       /*!< Writing date           */
  fosfat_time_t time_r;       /*!< Use date               */
  /* Linked list */
  struct file_info_s *next_file;
} fosfat_file_t;

/* fosfat opaque data types */
typedef struct fosfat_s fosfat_t;


/*
 * Load a device compatible Smaky FOS. The device can be a file or a device.
 * But with Window$ currently only device are supported. fosfat_close() must
 * always be called to freeing the memory and close the device.
 *
 * Linux   : specify the location on /dev/... or on a file
 * Window$ : specify the device with 'a' for diskette, 'c' for the first hard
 *           disk, etc,...
 *
 * param dev[in]        device or location
 * param disk[in]       type of disk, use FOSFAT_AD for auto-detection
 * param flag[in]       use F_UNDELETE for load deleted files or 0 for normal
 * return NULL if error or return the disk loaded
 */
fosfat_t *fosfat_open (const char *dev, fosfat_disk_t disk, unsigned int flag);

/*
 * Free the memory and close the device properly.
 *
 * param fosfat[in]     disk loaded
 */
void fosfat_close (fosfat_t *fosfat);

/*
 * By default, the internal logger is disabled. Use this function to enable
 * or disable the verbosity. The logger is enabled or disabled for all
 * devices loaded.
 *
 * param state[in]      boolean, 0 for disable the logger
 */
void fosfat_logger (int state);

/*
 * Get the diskname of a specific disk. The pointer must be freed when
 * no longer used.
 *
 * param fosfat[in]     disk loaded
 * return NULL if error/null-string or return the name
 */
char *fosfat_diskname (fosfat_t *fosfat);

/*
 * Get file/dir list of a directory in a linked list. When the list is no
 * longer used, fosfat_free_listdir() must always be called to freeing
 * the memory.
 *
 * param fosfat[in]     disk loaded
 * param location[in]   directory to list
 * return NULL if error or return the first file in the directory
 */
fosfat_file_t *fosfat_list_dir (fosfat_t *fosfat, const char *location);

/*
 * Free the memory for an linked list created by fosfat_list_dir().
 *
 * param var[in]        first file of the linked list
 */
void fosfat_free_listdir (fosfat_file_t *var);

/*
 * Get boolean information on a specific file or folder spedified with
 * a location.
 *
 * isdir     : test if the location is a directory
 * islink    : test if the location is a soft-link
 * isvisible : test if the location is not hidden
 * isencoded : test if the location is encoded
 * isopenexm : test if the location is 'open exclusif' and 'multiple'
 *
 * param fosfat[in]     disk loaded
 * param location[in]   file or directory to test
 * return a boolean, 0 for false
 */
int fosfat_isdir (fosfat_t *fosfat, const char *location);
int fosfat_islink (fosfat_t *fosfat, const char *location);
int fosfat_isvisible (fosfat_t *fosfat, const char *location);
int fosfat_isencoded (fosfat_t *fosfat, const char *location);
int fosfat_isopenexm (fosfat_t *fosfat, const char *location);

/*
 * Get some informations on a file or a directory. File size, attributes,
 * creation date, writing date and use date. Look fosfat_file_t structure
 * for more informations. The pointer must be freed when no longer used.
 *
 * param fosfat[in]     disk loaded
 * param location[in]   file or directory where get informations
 * return NULL if error or return the file structure
 */
fosfat_file_t *fosfat_get_stat (fosfat_t *fosfat, const char *location);

/*
 * If the location is a soft-link, then this function will return the
 * location of the target. The pointer must be freed when no longer used.
 *
 * param fosfat[in]     disk loaded
 * param location[in]   soft-link
 * return NULL if error/null-string or return the location
 */
char *fosfat_symlink (fosfat_t *fosfat, const char *location);

/*
 * Get a file from a location and put this on the user hard drive. You can't
 * get a directory with this function, you must recursively list directory
 * with fosfat_list_dir() and use fosfat_get_file with each file entry.
 *
 * param fosfat[in]     disk loaded
 * param src[in]        source location on the FOS disk
 * param dst[in]        destination location on the user hard drive
 * param output[in]     boolean for print copy progression in the console
 * return a boolean, 0 for error
 */
int fosfat_get_file (fosfat_t *fosfat, const char *src,
                     const char *dst, int output);

/*
 * Get a buffer of a file. Usefull for get only a part of a file without
 * save anything on the user hard drive. You can get all the data if you
 * prefer. The pointer must be freed when no longer used.
 *
 * param fosfat[in]     disk loaded
 * param path[in]       file where get data
 * param offset[in]     from where (in bytes) in the data
 * param size[in]       how many bytes
 * return NULL if error or return the buffer
 */
char *fosfat_get_buffer (fosfat_t *fosfat, const char *path,
                         int offset, int size);

#endif /* FOSFAT_H */
