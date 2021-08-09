
/*
  Copyright (C) 2000 Jesper Hansen <jesperh@telia.com>.

  This file is part of the yampp system.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


//----------------------------------------------------------------------------
// Constants...
//----------------------------------------------------------------------------

#define TRUE     1 
#define FALSE    0 
#define CTRL     0
#define CMD      1
#define DRIVE0   0


#define STANDBY	0
#define SLEEP	1
#define IDLE	2


// ATA status register bits

#define SR_BSY		0x80
#define SR_DRDY		0x40
#define SR_DF		0x20
#define SR_DSC		0x10
#define SR_DRQ		0x08
#define SR_CORR		0x04
#define SR_IDX		0x02
#define SR_ERR		0x01

// ATA error register bits

#define ER_UNC		0x40
#define ER_MC		0x20
#define ER_IDNF		0x10
#define ER_MCR		0x08
#define ER_ABRT		0x04
#define ER_TK0NF	0x02
#define ER_AMNF		0x01


//----------------------------------------------------------------------------
// Typedefs
//----------------------------------------------------------------------------

/*
typedef struct 
{
  unsigned char Heads; 
  unsigned int Tracks;
  unsigned int SectorsPerTrack;
  char Model[41];
} tdefDriveInfo;
*/

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------


char SetAddress(unsigned char cs, unsigned char adr);
//unsigned int ReadWORD(unsigned char cs, unsigned char adr);
unsigned char ReadBYTE(unsigned char cs, unsigned char adr);
 
//void WriteWORD(unsigned char cs, unsigned char adr, unsigned int dat);
void WriteBYTE(unsigned char cs, unsigned char adr, unsigned char dat);

//unsigned char IdentifyDrive(unsigned char DriveNo,  unsigned char *Buffer, tdefDriveInfo *DriveInfo);

unsigned char ATA_Read_Sectors(	unsigned char Drive, 
                				unsigned char Head, 
                				unsigned int Track, 
                				unsigned char Sector,
								unsigned int numsectors,
                				unsigned char *Buffer);

unsigned char ATA_Read_Sectors_LBA(	unsigned char Drive, 
									unsigned long lba,
									unsigned int numsectors,
                            		unsigned char *Buffer);

//unsigned char SetMode(unsigned char DriveNo, unsigned char Mode, unsigned char PwrDown);
//void ShowRegisters(unsigned char DriveNo);
unsigned char  ATA_SW_Reset(void);

//unsigned char ATA_Idle(unsigned char Drive);
