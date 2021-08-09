#ifndef FAT_H
  #define FAT_H

  typedef struct{
    u08 name[31];
    u08 attr;
    u32 clus;
    u32 size;
    u32 dtSec;
    u08 dtDE;
  } STRUCT_FILE_INFO;

  typedef struct{
    u32 sec;
    u08 de;
  } STRUCT_DIR_TABLE;

  u08 FAT_Init(void);
  u32 FAT_NextCls(u32 cluster);
  u32 FAT_cls2sec(u32 cluster);
  void FAT_FileInfo(STRUCT_FILE_INFO file);
  STRUCT_FILE_INFO FAT_ChangeDir(STRUCT_FILE_INFO dir);
  STRUCT_FILE_INFO FAT_NextFile(STRUCT_FILE_INFO prevFile);
  STRUCT_FILE_INFO FAT_PrevFile(STRUCT_FILE_INFO file);
  //u08 FAT_NextDir(void);

  #define FAT16               0x01
	#define FAT32								0x02

  #define FAT12_MASK      0x00000fff      // mask for 12 bit cluster numbers
	#define FAT16_MASK      0x0000ffff      // mask for 16 bit cluster numbers
	#define FAT32_MASK      0x0fffffff      // mask for FAT32 cluster numbers

	// Partition Type used in the partition record
	#define PART_TYPE_UNKNOWN	  0x00
	#define PART_TYPE_FAT12		  0x01
	#define PART_TYPE_XENIX		  0x02
	#define PART_TYPE_DOSFAT16	0x04
	#define PART_TYPE_EXTDOS	  0x05
	#define PART_TYPE_FAT16		  0x06
	#define PART_TYPE_NTFS		  0x07
	#define PART_TYPE_FAT32		  0x0B
	#define PART_TYPE_FAT32LBA	0x0C
	#define PART_TYPE_FAT16LBA	0x0E
	#define PART_TYPE_EXTDOSLBA	0x0F
	#define PART_TYPE_ONTRACK	  0x33
	#define PART_TYPE_NOVELL	  0x40
	#define PART_TYPE_PCIX		  0x4B
	#define PART_TYPE_PHOENIXSAVE	0xA0
	#define PART_TYPE_CPM		    0xDB
	#define PART_TYPE_DBFS		  0xE0
	#define PART_TYPE_BBT		    0xFF


  struct STRUCT_LFN_ENTRY{       // Structure of a Win95 long name directory entry
    u08  cnt;
      #define CNT_1stMASK     0x40
      #define CNT_LAST        0x01
      #define CNT_MASK        0x1f
    u08  name1[10];
    u08  attr;
      #define ATTR_LFN        0x0f
    u08  type;
      #define TYPE_IS         0x00
    u08  chksum;
    u08  name2[12];
    u16  clus;
      #define CLUS_IS         0x00
    u08  name3[4];
	};


  struct STRUCT_DIR_ENTRY{       // Structure of a dos directory entry.
    u08  name[8];           // filename, blank filled
      #define DE_END          0x00  // free & last de
      #define DE_CHRxE5       0x05  // the real value is 0xe5
      #define DE_FREE         0xe5  // file in this slot deleted
    u08  nameExt[3];        // extension, blank filled
    u08  attr;              // file attributes
      #define ATTR_FILE       0x00  // normal file
      #define ATTR_READ_ONLY  0x01  // file is readonly
      #define ATTR_HIDDEN     0x02  // file is hidden
      #define ATTR_SYSTEM     0x04  // file is a system file
      #define ATTR_VOLUME_ID  0x08  // entry is a volume label
      #define ATTR_DIRECTORY  0x10  // entry is a directory name
      #define ATTR_ARCHIVE    0x20  // file is new or modified
      #define ATTR_LONG_NAME  0x0f  // this is a s32 filename entry

      #define ATTR_MASK       0x0f  // normal file
      #define ATTR_HID_SYS    0x06  // file is hidden and system

    u08  ntRes;             // reserved used in NT VFAT (lower case flags)
      #define LCASE_BASE      0x08  // filename base in lower case
      #define LCASE_EXT       0x10  // filename extension in lower case
    u08  createTimeTenth;    // hundredth of seconds in CTime
    u08  createTime[2];      // create time
    u08  createDate[2];      // create date
    u08  lastAccDate[2];    // access date
    u16  firstClusHI;       // high bytes of cluster number
    u08  writeTime[2];      // last update time
    u08  writeDate[2];      // last update date
    u16  firstClusLO;       // starting cluster of file
    u32  fileSize;          // size of file in bytes
  };

  struct STRUCT_PART_REC{      // length 16 bytes
    u08 IsActive;         // 0x80 indicates active partition
    u08 StartHead;        // starting head for partition
    u16 StartCylSect;     // starting cylinder and sector
    u08 PartType;         // partition type (see above)
    u08 EndHead;          // ending head for this partition
    u16 EndCylSect;       // ending cylinder and sector
    u32 StartLBA;         // first LBA sector for this partition
    u32 Size;             // size of this partition (bytes or sectors ?)
  };

  struct STRUCT_PART_SEC{
    u08 PartCode[512-64-2];// pad so struct is 512b
    u08 Part[64];          // four partition records (64 bytes)
    u08 BootSectSig0;      // two signature bytes (2 bytes)
    u08 BootSectSig1;
        #define BOOTSIG0  0x55
        #define BOOTSIG1  0xaa
  };

  struct STRUCT_BOOT_SEC{       // 512bits
    u08 jmpBoot[3];        // jump inst E9xxxx or EBxx90
    u08 OEMName[8];        // OEM name and version
    u08 BPB[25];           // BIOS parameter block
    u08 BPBext[54];        // BIOS parameter block Extension
    u08 BootCode[420];     // pad so structure is 512b
    u08 BootSectSig0;      // two signature bytes (2 bytes)
    u08 BootSectSig1;
        #define BOOTSIG0  0x55
        #define BOOTSIG1  0xaa
  };

  // BPB same for FAT16&32
  struct STRUCT_BPB{               // 25bits
    u16 BytsPerSec;       // bytes per sector
    u08 SecPerClus;       // sectors per cluster
    u16 RsvdSecCnt;       // number of reserved sectors
    u08 NumFATs;          // number of FATs
    u16 RootEntCnt;       // number of root directory entries
    u16 TotSec16;         // total number of sectors
    u08 Media;            // media descriptor
    u16 FATSz16;          // number of sectors per FAT
    u16 SecPerTrk;        // sectors per track
    u16 NumHeads;         // number of heads
    u32 HiddSec;          // # of hidden sectors
    u32 TotSec32;         // # of sectors if bpbTotSec16 == 0
  };

  // extended BPB for FAT16
  struct STRUCT_BPB16{             // 26bits
    u08 DrvNum;
    u08 Reserved1;
    u08 BootSig;
    u32 VolID;
    u08 VolLab[11];
    u08 FilSysType[8];
  };

	// extended BPB for FAT32
  struct STRUCT_BPB32{             // 54bits
    u32 FATSz32;
    u16 ExtFlags;
        #define FATNUM    0xf // mask for numbering active FAT
        #define FATMIRROR 0x80// FAT is mirrored (like it always was)
    u16 FSVer;
        #define FSVERS    0   // currently only 0 is understood
    u32 RootClus;
    u16 FSInfo;
    u16 BkBootSec;
    u08 Reserved[12];
    u08 DrvNum;
    u08 Reserved1;
    u08 BootSig;
    u32 VolID;
    u08 VolLab[11];
    u08 FilSysType[8];
  };

#endif
