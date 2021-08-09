
/*
  Copyright (C) 2001 Jesper Hansen <jesperh@telia.com>.

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

#define SR_BUSY		0x80
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
// Prototypes
//----------------------------------------------------------------------------


u08 SetAddress(u08 cs, u08 adr);

u08 ReadBYTE(u08 cs, u08 adr);

void WriteBYTE(u08 cs, u08 adr, u08 dat);

//u08 IdentifyDrive(u08 DriveNo,  u08 *Buffer, tdefDriveInfo *DriveInfo);

u08 ATA_Read_Sectors(	u08 Drive, 
                		u08 Head, 
                		u16 Track, 
                		u08 Sector,
						u16 numsectors,
                		u08 *Buffer);

u08 ATA_Read_Sectors_LBA(	u08 Drive, 
							u32 lba,
							u16 numsectors,
                            u08 *Buffer);

u08  ATA_SW_Reset(void);

