/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2006-2009 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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

/**
 * \file fosfat.h
 *
 * libfosfat public API header.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

#define FOSFAT_NAMELGT  17

#define F_UNDELETE      (1 << 0)

/** Disk types. */
typedef enum disk_type {
  FOSFAT_FD,                   /*!< Floppy Disk.          */
  FOSFAT_HD,                   /*!< Hard Disk.            */
  FOSFAT_AD,                   /*!< Auto Detection.       */
  FOSFAT_ED                    /*!< Auto Detection fails. */
} fosfat_disk_t;

/** Time. */
typedef struct time_s {
  short int year;
  short int month;
  short int day;
  short int hour;
  short int minute;
  short int second;
} fosfat_time_t;

/** Attributes for a file (or dir).*/
typedef struct att_s {
  int isdir     : 1;
  int isvisible : 1;
  int isencoded : 1;
  int islink    : 1;
  int isdel     : 1;
} fosfat_att_t;

/** List of files in a directory. */
typedef struct file_info_s {
  char name[FOSFAT_NAMELGT];  /*!< File name.             */
  int size;                   /*!< File size.             */
  fosfat_att_t att;           /*!< File attributes.       */
  fosfat_time_t time_c;       /*!< Creation date.         */
  fosfat_time_t time_w;       /*!< Writing date.          */
  fosfat_time_t time_r;       /*!< Use date.              */
  /* Linked list */
  struct file_info_s *next_file;
} fosfat_file_t;

/** Fosfat handle on a disk. */
typedef struct fosfat_s fosfat_t;


/**
 * \brief Load a device compatible Smaky FOS.
 *
 * The device can be a file or a disk. But with Window$ currently only device
 * are supported. fosfat_close() must always be called to free the memory and
 * close the device.
 *
 * Linux   : specify the location on /dev/... or on a file
 * Window$ : specify the device with 'a' for diskette, 'c' for the first hard
 *           disk, etc,...
 *
 * \param[in] dev        device or location.
 * \param[in] disk       type of disk, use FOSFAT_AD for auto-detection.
 * \param[in] flag       use F_UNDELETE to load deleted files or 0 for normal.
 * \return NULL if error or return the disk handle.
 */
fosfat_t *fosfat_open (const char *dev, fosfat_disk_t disk, unsigned int flag);

/**
 * \brief Free the memory and close the device properly.
 *
 * \param[in] fosfat     disk handle.
 */
void fosfat_close (fosfat_t *fosfat);

/**
 * \brief Change log level (verbosity).
 *
 * By default, the internal logger is disabled. Use this function to enable
 * or disable the verbosity. The logger is enabled or disabled for all
 * devices loaded.
 *
 * \param[in] state      boolean, 0 to disable the logger.
 */
void fosfat_logger (int state);

/**
 * \brief Get the disk's name of a specific device.
 *
 * The pointer must be freed when no longer used.
 *
 * \param[in] fosfat     disk handle.
 * \return NULL if error/null-string or return the name.
 */
char *fosfat_diskname (fosfat_t *fosfat);

/**
 * \brief Get file/dir list of a directory in a linked list.
 *
 * When the list is no longer used, fosfat_free_listdir() must always be
 * called to free the memory.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   directory to list.
 * \return NULL if error or return the first file in the directory.
 */
fosfat_file_t *fosfat_list_dir (fosfat_t *fosfat, const char *location);

/**
 * \brief Free the memory for an linked list created by fosfat_list_dir().
 *
 * \param[in] var        first file of the linked list.
 */
void fosfat_free_listdir (fosfat_file_t *var);

/**
 * \brief Get boolean information on a specific file or folder.
 *
 * Test if the location is a directory.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   file or directory to test.
 * \return a boolean, 0 for false.
 */
int fosfat_isdir (fosfat_t *fosfat, const char *location);

/**
 * \brief Get boolean information on a specific file or folder.
 *
 * Test if the location is a soft-link.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   file or directory to test.
 * \return a boolean, 0 for false.
 */
int fosfat_islink (fosfat_t *fosfat, const char *location);

/**
 * \brief Get boolean information on a specific file or folder.
 *
 * Test if the location is not hidden.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   file or directory to test.
 * \return a boolean, 0 for false.
 */
int fosfat_isvisible (fosfat_t *fosfat, const char *location);

/**
 * \brief Get boolean information on a specific file or folder.
 *
 * Test if the location is encoded.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   file or directory to test.
 * \return a boolean, 0 for false.
 */
int fosfat_isencoded (fosfat_t *fosfat, const char *location);

/**
 * \brief Get boolean information on a specific file or folder.
 *
 * Test if the location is 'open exclusif' and 'multiple'.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   file or directory to test.
 * \return a boolean, 0 for false.
 */
int fosfat_isopenexm (fosfat_t *fosfat, const char *location);

/**
 * \brief Get some informations on a file or a directory.
 *
 * File size, attributes, creation date, writing date and use date. Look
 * fosfat_file_t structure for more informations. The pointer must be freed
 * when no longer used.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   file or directory where get informations.
 * \return NULL if error or return the file structure.
 */
fosfat_file_t *fosfat_get_stat (fosfat_t *fosfat, const char *location);

/**
 * \brief Get the target of a soft-link.
 *
 * If the location is a soft-link, then this function will return the
 * location of the target. The pointer must be freed when no longer used.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] location   soft-link.
 * \return NULL if error/null-string or return the location.
 */
char *fosfat_symlink (fosfat_t *fosfat, const char *location);

/**
 * \brief Get a file from a location and put this on the user hard drive.
 *
 * You can't get a directory with this function, you must recursively list
 * the directory with fosfat_list_dir() and use fosfat_get_file() with each
 * file entry.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] src        source location on the FOS disk.
 * \param[in] dst        destination location on the user hard drive.
 * \param[in] output     boolean to print copy progression in the terminal.
 * \return a boolean, 0 for error.
 */
int fosfat_get_file (fosfat_t *fosfat,
                     const char *src, const char *dst, int output);

/**
 * \brief Get a buffer of a file.
 *
 * Useful to get only a part of a file without save anything on the user
 * hard drive. You can get all the data if you prefer. The pointer must be
 * freed when no longer used.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] path       file where get data.
 * \param[in] offset     from where (in bytes) in the data.
 * \param[in] size       how many bytes.
 * \return NULL if error or return the buffer.
 */
uint8_t *fosfat_get_buffer (fosfat_t *fosfat,
                         const char *path, int offset, int size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FOSFAT_H */
