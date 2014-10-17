/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

#ifndef ATA_H
#define ATA_H

#include "parameters.h"

#ifdef SUPPORT_CDROM
# define LOWLEVEL_CDROM cdrom_interface
#endif

typedef enum _sense {
      SENSE_NONE = 0, SENSE_NOT_READY = 2, SENSE_ILLEGAL_REQUEST = 5,
      SENSE_UNIT_ATTENTION = 6
} sense_t;

typedef enum _asc {
      ASC_INV_FIELD_IN_CMD_PACKET = 0x24,
      ASC_MEDIUM_NOT_PRESENT = 0x3a,
      ASC_SAVING_PARAMETERS_NOT_SUPPORTED = 0x39,
      ASC_LOGICAL_BLOCK_OOR = 0x21
} asc_t;

class LOWLEVEL_CDROM;

class device_image_t
{
  public:
      // Open a image. Returns non-negative if successful.
      virtual int open (const char* pathname, bool readonly) = 0;

      // Close the image.
      virtual void close () = 0;

      // Position ourselves. Return the resulting offset from the
      // beginning of the file.
      virtual off_t lseek (off_t offset, int whence) = 0;

      // Read count bytes to the buffer buf. Return the number of
      // bytes read (count).
      virtual ssize_t read (void* buf, size_t count) = 0;

      // Write count bytes from buf. Return the number of bytes
      // written (count).
      virtual ssize_t write (const void* buf, size_t count) = 0;

      virtual ~device_image_t() { }

      unsigned cylinders;
      unsigned heads;
      unsigned sectors;
      bool byteswap;
};

class default_image_t : public device_image_t
{
  public:
      // Open a image. Returns non-negative if successful.
      int open (const char* pathname, bool readonly);

      // Close the image.
      void close ();

      // Position ourselves. Return the resulting offset from the
      // beginning of the file.
      off_t lseek (off_t offset, int whence);

      // Read count bytes to the buffer buf. Return the number of
      // bytes read (count).
      ssize_t read (void* buf, size_t count);

      // Write count bytes from buf. Return the number of bytes
      // written (count).
      ssize_t write (const void* buf, size_t count);

  private:
      int fd;

};

#ifdef BX_SPLIT_HD_SUPPORT
class concat_image_t : public device_image_t
{
  public:
      // Default constructor
      concat_image_t();
  
      // Open a image. Returns non-negative if successful.
      int open (const char* pathname, bool readonly);

      // Close the image.
      void close ();

      // Position ourselves. Return the resulting offset from the
      // beginning of the file.
      off_t lseek (off_t offset, int whence);

      // Read count bytes to the buffer buf. Return the number of
      // bytes read (count).
      ssize_t read (void* buf, size_t count);

      // Write count bytes from buf. Return the number of bytes
      // written (count).
      ssize_t write (const void* buf, size_t count);

  private:
#define BX_CONCAT_MAX_IMAGES 8
      int fd_table[BX_CONCAT_MAX_IMAGES];
      off_t start_offset_table[BX_CONCAT_MAX_IMAGES];
      off_t length_table[BX_CONCAT_MAX_IMAGES];
      void increment_string (char *str);
      int maxfd;  // number of entries in tables that are valid

      // notice if anyone does sequential read or write without seek in between.
      // This can be supported pretty easily, but needs additional checks.
      // 0=something other than seek was last operation
      // 1=seek was last operation
      int seek_was_last_op;

      // the following variables tell which partial image file to use for
      // the next read and write.
      int index;  // index into table
      int fd;     // fd to use for reads and writes
      off_t thismin, thismax; // byte offset boundary of this image
};
#endif /* BX_SPLIT_HD_SUPPORT */

#ifdef EXTERNAL_DISK_SIMULATOR
#include "external-disk-simulator.h"
#endif

#ifdef DLL_HD_SUPPORT
class dll_image_t : public device_image_t
{
  public:
      // Open a image. Returns non-negative if successful.
      int open (const char* pathname, bool readonly);

      // Close the image.
      void close ();

      // Position ourselves. Return the resulting offset from the
      // beginning of the file.
      off_t lseek (off_t offset, int whence);

      // Read count bytes to the buffer buf. Return the number of
      // bytes read (count).
      ssize_t read (void* buf, size_t count);

      // Write count bytes from buf. Return the number of bytes
      // written (count).
      ssize_t write (const void* buf, size_t count);

  private:
      int vunit,vblk;

};
#endif


typedef struct {
  struct {
    bool busy;
    bool drive_ready;
    bool write_fault;
    bool seek_complete;
    bool drq;
    bool corrected_data;
    bool index_pulse;
    unsigned index_pulse_count;
    bool err;
    } status;
  Bit8u    error_register;
  Bit8u    head_no;
  union {
    Bit8u    sector_count;
    struct {
#ifdef WORDS_BIGENDIAN
      unsigned tag : 5;
      unsigned rel : 1;
      unsigned i_o : 1;
      unsigned c_d : 1;
#else	    
      unsigned c_d : 1;
      unsigned i_o : 1;
      unsigned rel : 1;
      unsigned tag : 5;
#endif
    } interrupt_reason;
  };
  Bit8u    sector_no;
  union {
    Bit16u   cylinder_no;
    Bit16u   byte_count;
  };
  Bit8u    buffer[2048];
  Bit32u   buffer_index;
  Bit32u   drq_index;
  Bit8u    current_command;
  Bit8u    sectors_per_block;
  Bit8u    lba_mode;
  struct {
    bool reset;       // 0=normal, 1=reset controller
    bool disable_irq; // 0=allow irq, 1=disable irq
    } control;
  Bit8u    reset_in_progress;
  Bit8u    features;
  } controller_t;

struct sense_info_t {
  sense_t sense_key;
  struct {
    Bit8u arr[4];
  } information;
  struct {
    Bit8u arr[4];
  } specific_inf;
  struct {
    Bit8u arr[3];
  } key_spec;
  Bit8u fruc;
  Bit8u asc;
  Bit8u ascq;
};

struct error_recovery_t {
  unsigned char data[8];

  error_recovery_t ();
};

uint16 read_16bit(const uint8* buf);
uint32 read_32bit(const uint8* buf);


#ifdef LOWLEVEL_CDROM
#  include "cdrom.h"
#endif


struct cdrom_t
{
  bool ready;
  bool locked;
#ifdef LOWLEVEL_CDROM
  LOWLEVEL_CDROM* cd;
#endif
  uint32 capacity;
  int next_lba;
  int remaining_blocks;
  struct currentStruct {
    error_recovery_t error_recovery;
  } current;
};

struct atapi_t
{
  uint8 command;
  int drq_bytes;
  int total_bytes_remaining;
};

#ifdef BX_USE_HD_SMF
#  define BX_HD_SMF  static
#  define BX_HD_THIS bx_hard_drive.
#else
#  define BX_HD_SMF
#  define BX_HD_THIS this->
#endif

class bx_hard_drive_c {
public:

  bx_hard_drive_c(void);
  ~bx_hard_drive_c(void);
  BX_HD_SMF void   close_harddrive(void);
  BX_HD_SMF void   init();
  BX_HD_SMF void   reset(unsigned type);
  BX_HD_SMF Bit32u   get_device_handle(Bit8u channel, Bit8u device);
  BX_HD_SMF Bit32u   get_first_cd_handle(void);
  BX_HD_SMF unsigned get_cd_media_status(Bit32u handle);
  BX_HD_SMF unsigned set_cd_media_status(Bit32u handle, unsigned status);

  BX_HD_SMF bool  bmdma_read_sector(Bit8u channel, Bit8u *buffer);
  BX_HD_SMF bool  bmdma_write_sector(Bit8u channel, Bit8u *buffer);
  BX_HD_SMF void  bmdma_complete(Bit8u channel);


#ifndef BX_USE_HD_SMF
  Bit32u read(Bit32u address, unsigned io_len);
  void   write(Bit32u address, Bit32u value, unsigned io_len);
#endif

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);

private:

  BX_HD_SMF bool calculate_logical_address(Bit8u channel, off_t *sector);
  BX_HD_SMF void increment_address(Bit8u channel);
  BX_HD_SMF void identify_drive(Bit8u channel);
  BX_HD_SMF void identify_ATAPI_drive(Bit8u channel);
  BX_HD_SMF void command_aborted(Bit8u channel, unsigned command);

  BX_HD_SMF void init_send_atapi_command(Bit8u channel, Bit8u command, int req_length, int alloc_length, bool lazy = false);
  BX_HD_SMF void ready_to_send_atapi(Bit8u channel);
  BX_HD_SMF void raise_interrupt(Bit8u channel);
  BX_HD_SMF void atapi_cmd_error(Bit8u channel, sense_t sense_key, asc_t asc);
  BX_HD_SMF void init_mode_sense_single(Bit8u channel, const void* src, int size);
  BX_HD_SMF void atapi_cmd_nop(Bit8u channel);

  // FIXME:
  // For each ATA channel we should have one controller struct
  // and an array of two drive structs
  struct channel_t {
    struct drive_t {
      device_image_t* hard_drive;
      device_type_t device_type;
      // 512 byte buffer for ID drive command
      // These words are stored in native word endian format, as
      // they are fetched and returned via a return(), so
      // there's no need to keep them in x86 endian format.
      Bit16u id_drive[256];

      controller_t controller;
      cdrom_t cdrom;
      sense_info_t sense;
      atapi_t atapi;

      Bit8u model_no[41];
      } drives[2];
    unsigned drive_select;

#if 0
    Bit32u ioaddr1;
    Bit32u ioaddr2;
    Bit8u  irq;
#endif
    
    Bit32u (*addr2io)(Bit32u);

    } channels[BX_MAX_ATA_CHANNEL];

  };

extern bx_hard_drive_c bx_hard_drive;

#ifndef UNUSED
#  define UNUSED(x) ((void)x)
#endif

#endif /* ATA_H */
