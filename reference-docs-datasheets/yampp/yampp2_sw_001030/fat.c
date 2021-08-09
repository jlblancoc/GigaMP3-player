
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


#include <io.h>
#include <string.h>

#include "ata_if.h"
#include "fat.h"
#include "mem.h"
#include "progmem.h"
#include "uart.h"

unsigned char *SectorBuffer  = (unsigned char *) SECTOR_BUFFER;
unsigned char *long_name_buffer = (unsigned char *) LONGNAME_BUF;
unsigned char *dir_name_buffer = (unsigned char *) DIRNAME_BUF;

unsigned long FirstDataSector;
unsigned int  Bytes_Per_Sector;
unsigned int  Sectors_Per_Cluster;
unsigned long FirstFATSector;
unsigned long FirstDirSector;
unsigned long FileSize;
unsigned long FatInCache = 0;

/*************************************************************************/
/*************************************************************************/


unsigned long clust2sect(unsigned long clust)
{
	return ((clust-2) * Sectors_Per_Cluster) + FirstDataSector;
}

unsigned char init_fat( unsigned char device)
{
	struct partrecord *pr;
	struct bpb710 *bpb;
    unsigned long first_sec;


	// read partition table
    ATA_Read_Sectors_LBA( 	DRIVE0,				// TODO.... error checking
    						0,
							1,
                            SectorBuffer);
	
	pr = (struct partrecord *) ((struct partsector *) SectorBuffer)->psPart;

/*	
	switch (pr->prPartType)
	{

//		case DOS16:
//				lcd_print("DOS 16");
//				break;
//		case DOS16_32:
//				lcd_print("DOS 16/32");
//				break;
//		case FAT16_LBA:
//				lcd_print("FAT16 LBA");
//				break;

		case PART_TYPE_FAT32LBA:
				PRINT("FAT32 LBA");
				break;

		case PART_TYPE_FAT32:
                PRINT("FAT32");
//                return 1;	
				break;
		
		default:
				PRINT("Part.!");
				return 1;
				break;
	}

	EOL();
*/
	first_sec = pr->prStartLBA;
/*
    PRINT("First sector: ");
    UART_Printfu32(pr->prStartLBA);
	EOL();
    PRINT("Size: ");
    UART_Printfu32(pr->prSize);
	EOL();
*/  

	// Read the Partition BootSector

    ATA_Read_Sectors_LBA( 	DRIVE0,
    						first_sec,
							1,
                            SectorBuffer);

	bpb = (struct bpb710 *) ((struct bootsector710 *) SectorBuffer)->bsBPB;

/*
	PRINT("bytes/Sec. : ");
	UART_Printfu16(bpb->bpbBytesPerSec);
	EOL();
	
	PRINT("sec/clu : ");
	UART_Printfu08(bpb->bpbSecPerClust);
	EOL();

	PRINT("reserved : ");
	UART_Printfu16(bpb->bpbResSectors);
	EOL();
	
	PRINT("BigFatSects : ");
	UART_Printfu32(bpb->bpbBigFATsecs);
	EOL();

	PRINT("# Fats : ");
	UART_Printfu08(bpb->bpbFATs);
	EOL();

*/
	// setup some constants
	FirstDataSector 	= bpb->bpbResSectors + bpb->bpbFATs * bpb->bpbBigFATsecs;
	FirstDataSector    += first_sec;
	Sectors_Per_Cluster = bpb->bpbSecPerClust;
	Bytes_Per_Sector 	= bpb->bpbBytesPerSec;

	FirstFATSector = bpb->bpbResSectors + first_sec;
	FirstDirSector = bpb->bpbRootClust;

	return 0;	
}

//////////////////////////////////////////////////////////////


unsigned int baseentry = 0;
unsigned int entrycount = 0;

unsigned long get_dir_entry(unsigned int entry, unsigned int count)
{
    unsigned long sector;
	struct direntry *de = 0;	// avoid compiler warning by initializing
	struct winentry *we;
	unsigned int hasBuffer;
	unsigned int b;
    int i,index;
    char *p;
	

	if (count == 0)
	{
		entrycount = 0;
		*dir_name_buffer = 0;
	}
	
	// read dir data
	
	sector = clust2sect(FirstDirSector);

	hasBuffer = 0;

	index = 16;	// crank it up
	do 
	{
		if (index == 16)	// time for next sector ?
		{
    		ATA_Read_Sectors_LBA( 	DRIVE0,
    								sector++,
									1,
                    	        	SectorBuffer);
			de = (struct direntry *) SectorBuffer;
			index = 0;
		}	
	
		if (*de->deName != 0xE5)	// if not a deleted entry
		{
			// long name entry
			if (de->deAttributes == ATTR_LONG_FILENAME)
			{
				we = (struct winentry *) de;
				b = 13 *( (we->weCnt-1) & 0x0f);				// index into string
				p = &long_name_buffer[b];
				for (i=0;i<5;i++)	*p++ = we->wePart1[i*2];	// copy first part			
				for (i=0;i<6;i++)	*p++ = we->wePart2[i*2];	// second part
				for (i=0;i<2;i++)	*p++ = we->wePart3[i*2];	// and third part
				if (we->weCnt & 0x40) *p = 0;					// in case dirnamelength is multiple of 13
				if ((we->weCnt & 0x0f) == 1) hasBuffer = 1;		// mark that we have a long entry
			}
			else // short name entry
			{
				if (hasBuffer)		// a long entry name has been collected
				{
					if (de->deAttributes == ATTR_DIRECTORY)	// is it a directory ?
					{
						unsigned long save = FirstDirSector;
						unsigned int save2 = baseentry;
						unsigned long rval;
						
						strcpy(dir_name_buffer,long_name_buffer);
						strcat(dir_name_buffer,"/");
						
//						UART_Puts(long_name_buffer); UART_SendByte('/'); //EOL();

						// call recursively
						FirstDirSector = ((unsigned long)de->deHighClust << 16) + de->deStartCluster;
						rval = get_dir_entry(entry,1);
						FirstDirSector = save;
						baseentry = save2;
						if (rval)
							return rval;
						else	
						{
			    		  	ATA_Read_Sectors_LBA( 	DRIVE0,			// reload original sector
    												sector-1,
													1,
                    		    	    			SectorBuffer);
                    		
							entrycount--;			// decrement entry counter		
							*dir_name_buffer = 0;
                    	}
					}
					else // normal file entry
						if (entrycount == entry)		
							break;
					hasBuffer = 0;	// clear buffer	
					entrycount++;			// increment entry counter		
				}
				// else ignore short_name_only entries
			}
		}
		de++;
		index++;
	}	while (*de->deName || index == 16);	// 0 in de->deName[0] if no more entries

	if (hasBuffer == 0)		// end of entries
		return 0;
	
	FileSize = de->deFileSize;
	return (unsigned long) ((unsigned long)de->deHighClust << 16) + de->deStartCluster;
}


//
// return the size of the last directory entry
//
unsigned long get_filesize(void)
{
	return FileSize;
}


//
// return the long name of the last directory entry
//
char *get_filename(void)
{	
	return long_name_buffer;
}

//
// return the directory of the last directory entry
//
char *get_dirname(void)
{	
	return dir_name_buffer;
}


//
// load a clusterfull of data
//
void load_sectors(unsigned long cluster, unsigned char *buffer)
{
	// read cluster
	while ( ATA_Read_Sectors_LBA( 	DRIVE0,
   							clust2sect(cluster),
							Sectors_Per_Cluster,
           	                buffer) != 0);
}

//
// find next cluster in the FAT chain
//
unsigned long next_cluster(unsigned long cluster)
{
	unsigned long FatOffset = cluster << 2;	
	unsigned long sector = FirstFATSector + (FatOffset / Bytes_Per_Sector);
	unsigned int offset = FatOffset % Bytes_Per_Sector;

	if (sector != FatInCache)
	{
		// read sector
	    while (ATA_Read_Sectors_LBA(	DRIVE0,
	   									sector,
										1,
	           	                		(unsigned char*)FATCACHE) != 0);
		FatInCache = sector;
    }
           	                
	FatOffset =  (*((unsigned long*) &((char*)FATCACHE)[offset])) & FAT32_MASK;

	if (FatOffset == (CLUST_EOFE & FAT32_MASK))
		FatOffset = 0;

/*	UART_SendByte('>');
	UART_Printfu32(FatOffset);
	EOL();
*/	
	return FatOffset;
}
