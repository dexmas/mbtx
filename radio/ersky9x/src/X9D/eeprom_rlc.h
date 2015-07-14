/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Romolo Manfredini <romolo.manfredini@gmail.com>
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef eeprom_rlc_h
#define eeprom_rlc_h

#include <inttypes.h>

#define FILE_TYPE_MODEL		0
#define FILE_TYPE_TEXT		1

#define blkid_t    uint8_t
#define blkidL_t    uint16_t

#if !defined(EESIZE)
    #define EESIZE    (32*1024)
#else
  #define EESIZE_SIMU (32*1024)
#endif
#define EEFS_VERS  1
#define MAXFILES   41
#define BS         128

#ifndef PACK
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

PACK(struct DirEnt
{
  blkid_t  startBlk ;
  uint16_t size:12 ;
  uint16_t typ:4 ;
}) ;

#define EEFS_EXTRA_FIELDS uint8_t  spare[1];

PACK(struct EeFs
{
  uint8_t  version ;				// 1
  blkid_t  mySize ;					// +1 = 2
  blkid_t  freeList ;				// +1 = 3
  uint8_t  bs ;							// +1 = 4
  EEFS_EXTRA_FIELDS					// +1 = 5
  DirEnt   files[MAXFILES] ;  // +123 = 128
});

extern EeFs eeFs;

#define FILE_TYP_GENERAL 1
#define FILE_TYP_MODEL   2

/// fileId of general file
#define FILE_GENERAL   0
/// convert model number 0..MAX_MODELS-1  int fileId
#define FILE_MODEL(n) (1+(n))
#define FILE_TMP      (1+MAX_MODELS)

#define RESV          128 // sizeof(EeFs)  //reserv for eeprom header with directory (eeFs)

#define FIRSTBLK      1
#define BLOCKS        (1+(EESIZE-RESV)/BS)
#define BLOCKS_OFFSET (RESV-BS)

int8_t EeFsck();
void EeFsFormat();
uint16_t EeFsGetFree();

class EFile
{
  public:

    ///remove contents of given file
    static void rm(uint8_t i_fileId);

    ///swap contents of file1 with them of file2
    static void swap(uint8_t i_fileId1, uint8_t i_fileId2);

    ///return true if the file with given fileid exists
    static bool exists(uint8_t i_fileId);

    ///open file for reading, no close necessary
    void openRd(uint8_t i_fileId);

    uint8_t read(uint8_t *buf, uint8_t len);

//  protected:

    uint8_t  m_fileId;    //index of file in directory = filename
    uint16_t m_pos;       //over all filepos
    blkid_t  m_currBlk;   //current block.id
    uint8_t  m_ofs;       //offset inside of the current block
};

#define eeFileSize(f)   eeFs.files[f].size
#define eeModelSize(id) eeFileSize(FILE_MODEL(id))

#define ERR_NONE 0
#define ERR_FULL 1
extern uint8_t  s_write_err;    // error reasons

extern uint8_t  s_sync_write;

#define ENABLE_SYNC_WRITE(val) s_sync_write = val;
#define IS_SYNC_WRITE_ENABLE() s_sync_write

///deliver current errno, this is reset in open
inline uint8_t write_errno() { return s_write_err; }

class RlcFile: public EFile
{
  uint8_t  m_bRlc;      // control byte for run length decoder
  uint8_t  m_zeroes;

  uint8_t m_flags;
#define WRITE_FIRST_LINK               0x01
#define WRITE_NEXT_LINK_1              0x02
#define WRITE_NEXT_LINK_2              0x03
#define WRITE_START_STEP               0x10
#define WRITE_FREE_UNUSED_BLOCKS_STEP1 0x20
#define WRITE_FREE_UNUSED_BLOCKS_STEP2 0x30
#define WRITE_FINAL_DIRENT_STEP        0x40
#define WRITE_TMP_DIRENT_STEP          0x50
  uint8_t m_write_step;
  uint16_t m_rlc_len;
  uint8_t * m_rlc_buf;
  uint8_t m_cur_rlc_len;
  uint8_t m_write1_byte;
  uint8_t m_write_len;
  uint8_t * m_write_buf;

//#if defined (EEPROM_PROGRESS_BAR)
//  uint8_t m_ratio;
//#endif

public:

  void openRlc(uint8_t i_fileId);

  void create(uint8_t i_fileId, uint8_t typ, uint8_t sync_write);

  /// copy contents of i_fileSrc to i_fileDst
  bool copy(uint8_t i_fileDst, uint8_t i_fileSrc);

  inline bool isWriting() { return m_write_step != 0; }
  void write(uint8_t *buf, uint8_t i_len);
  void write1(uint8_t b);
  void nextWriteStep();
  void nextRlcWriteStep();
  void writeRlc(uint8_t i_fileId, uint8_t typ, uint8_t *buf, uint16_t i_len, uint8_t sync_write);

  // flush the current write operation if any
  void flush();

  // read from opened file and decode rlc-coded data
  uint16_t readRlc(uint8_t *buf, uint16_t i_len);

//#if defined (EEPROM_PROGRESS_BAR)
//  void DisplayProgressBar(uint8_t x);
//#endif
};

extern RlcFile theFile;  //used for any file operation

inline void eeFlush() { theFile.flush(); }
extern void eePoll( void ) ;

//#if defined (EEPROM_PROGRESS_BAR)
//#define DISPLAY_PROGRESS_BAR(x) theFile.DisplayProgressBar(x)
//#else
//#define DISPLAY_PROGRESS_BAR(x)
//#endif

#define eeDeleteModel(x) EFile::rm(FILE_MODEL(x))
void ee32WaitLoadModel(uint8_t x) ;
bool eeModelExists(uint8_t id) ;
void eeLoadModel(uint8_t id) ;

bool eeDuplicateModel(uint8_t src);
bool eeCopyModel(uint8_t dst, uint8_t src);
void ee32SwapModels(uint8_t id1, uint8_t id2);
uint32_t ee32_check_finished( void ) ;
void ee32_delete_model( uint8_t x ) ;

#if defined(SDCARD)
const char *ee32BackupModel( uint8_t modelIndex ) ;
//const pm_char * eeBackupModel(uint8_t i_fileSrc);
const pm_char * eeRestoreModel(uint8_t i_fileDst, char *model_name);
#endif

extern uint8_t ModelImage[] ;
extern uint8_t ModelImageValid ;
uint32_t loadModelImage( void ) ;

#endif
