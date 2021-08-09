
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


#include <io.h>
#include <string.h>

#include "types.h"
#include "ata_if.h"
#include "fat.h"
#include "mem.h"
#include "progmem.h"
#include "uart.h"

u08 *SectorBuffer;
u08 *long_name_buffer;
u08 *dir_name_buffer;

u32 FirstDataSector;
u16 Bytes_Per_Sector;
u16 Sectors_Per_Cluster;
u32 FirstFATSector;
u32 FirstDirSector;
u32 FileSize;
u32 FatInCache;
u32 MyCluster;


/*************************************************************************/
/*************************************************************************/


u32 clust2sect(u32 clust)
{
	return ((clust-2) * Sectors_Per_Cluster) + FirstDataSector;
}

u08 init_fat( u08 device)
{
	struct partrecord *pr;
	struct bpb710 *bpb;
    u32 first_sec;

	// setup pointers
	long_name_buffer = (u08 *) LONGNAME_BUF;
	dir_name_buffer  = (u08 *) DIRNAME_BUF;

	// clear cache 
	FatInCache = 0;

	// read partition table
    ATA_Read_Sectors_LBA( 	DRIVE0,
    						0,
							1,
                            SectorBuffer);
	
	// get the partition record
	pr = (struct partrecord *) ((struct partsector *) SectorBuffer)->psPart;

	// and find the first valid sector
	first_sec = pr->prStartLBA;

	// Read the Partition BootSector
    ATA_Read_Sectors_LBA( 	DRIVE0,
    						first_sec,
							1,
                            SectorBuffer);

	// get BIOS parameter block
	bpb = (struct bpb710 *)  ((struct bootsector710 *) SectorBuffer)->bsBPB;

	// and setup some constants
	FirstDataSector 	= bpb->bpbResSectors + bpb->bpbFATs * bpb->bpbBigFATsecs;
	FirstDataSector    += first_sec;
	Sectors_Per_Cluster = bpb->bpbSecPerClust;
	Bytes_Per_Sector 	= bpb->bpbBytesPerSec;
	FirstFATSector 		= bpb->bpbResSectors + first_sec;
	FirstDirSector 		= bpb->bpbRootClust;

	return 0;	
}

//////////////////////////////////////////////////////////////




extern u08 *buffer1;

u16 entrycount;
u08 dirlisting = 0;


struct direntry * get_de(u16 entry, u32 basecluster)
{
    u32 prevcluster;
	struct direntry *de;	
    u16 i,index;
	
	// always clear the longname buffer
	*long_name_buffer = 0;

	//avoid warnings
	de = 0; 
	prevcluster = 0;

	index = 16*Sectors_Per_Cluster;	// crank it up
	do 
	{
		if (index == 16*Sectors_Per_Cluster)	// time for a new cluster ?
		{
			ATA_Read_Sectors_LBA( 	DRIVE0,
   									clust2sect(basecluster),
									Sectors_Per_Cluster,
           			                buffer1);
				
            prevcluster = basecluster;					// save cluster for directry backstep below
            basecluster = next_cluster(basecluster);
			de = (struct direntry *) buffer1;
			index = 0;
		}	
	
		if (*de->deName != 0xE5)	// if not a deleted entry
		{
			// if this is a long name entry
			if (de->deAttributes == ATTR_LONG_FILENAME)
			{
				struct winentry *we;
			    char *p;
				u16 b;

				we = (struct winentry *) de;
				b = 13 *( (we->weCnt-1) & 0x0f);				// index into string
				p = &long_name_buffer[b];
				for (i=0;i<5;i++)	*p++ = we->wePart1[i*2];	// copy first part			
				for (i=0;i<6;i++)	*p++ = we->wePart2[i*2];	// second part
				for (i=0;i<2;i++)	*p++ = we->wePart3[i*2];	// and third part
				if (we->weCnt & 0x40) *p = 0;					// in case dirnamelength is multiple of 13
			}
			else 
			if ((de->deAttributes & (ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME)) == 0) // normal file entry (including directories)
			{
				// if the longname buffer is empty, copy the short name in there
				
				// ignore the dot names/directories 
				if (*de->deName != '.')
				{
					if (*long_name_buffer == 0)
					{
						char *p = long_name_buffer;
						for (i=0;i<8;i++)	*p++ = de->deName[i];		// copy first part			
						*p++ = '.';
						for (i=0;i<3;i++)	*p++ = (de->deExtension[i] & 0x5F);	// copy second part	(in lower case)		
						*p = 0;
					}

					if (de->deAttributes == ATTR_DIRECTORY)	// is it a directory ?
					{
							struct direntry *subde;
							char *p;
							
							// copy name buffer to directory buffer
							strcpy(dir_name_buffer,long_name_buffer);

							// special handling for directory names that only have a short entry
							// they may have a dot and some spaces in the name from the copying above
							// fix this
							
							if ( (p = strrchr(dir_name_buffer,'.')) != 0 )
							{
								while (*p == ' ' || *p == '.')
										*p-- = 0;
							}
							
							// add a slash to the directory
							strcat(dir_name_buffer,"/");

							subde = get_de(entry,((u32)de->deHighClust << 16) + de->deStartCluster);
							if (subde)
							{
								return subde;
							}	
							else	
							{
				    		  	ATA_Read_Sectors_LBA( 	DRIVE0,			// reload original sector
	    												clust2sect(prevcluster),
														Sectors_Per_Cluster,
	                    		    	    			buffer1);
								*dir_name_buffer = 0;
	                    	}
					}
					else
					{
						if (dirlisting == DIRLIST_VERBOSE)	
						{
							// print the name
							UART_Puts(dir_name_buffer);
							UART_Putsln(long_name_buffer);
						}

						// if this is the entry we're looking for, exit
						if (entrycount == entry)
						{
							// check for ".mp3" extension, and if it exist,
							// then cut it off, else skip file
							char *p;
							if ( (p = strrchr(long_name_buffer,'.')) != 0 )
							{
								if ( *(p+1) == 'm' && *(p+2) == 'p' && *(p+3) == '3' )
								{
									*p = 0;	// cut it
									
									if (!dirlisting)
										return de;
								}	
							}
						}
						*long_name_buffer = 0;
						entrycount++;			// increment entry counter		
					}
				}	
			}				
		}
		de++;		// point to next directory entry
		index++;	// increment index count within this sector
		
	} while (index == (16*Sectors_Per_Cluster) || *de->deName);	// 0 in de->deName[0] if no more entries


	return 0;
}


//
// do a directory listing 
// either just scan to find # of files,
// or print dir/filename on each
//
u16 dirlist(u08 mode)
{
	*dir_name_buffer = 0;
	*long_name_buffer = 0;
	entrycount = 0;

	dirlisting = mode;
	get_de(-1,FirstDirSector);	// scan through all
	dirlisting = 0;
	return entrycount;
}


//
//
//
u32 get_dir_entry(u16 entry)
{
	struct direntry *de;

	*dir_name_buffer = 0;
	*long_name_buffer = 0;

	entrycount = 0;

	de = get_de(entry,FirstDirSector);	// check in root

	if (de)
	{
		FileSize = de->deFileSize;
		MyCluster = (u32) ((u32)de->deHighClust << 16) + de->deStartCluster;
		return MyCluster;
	}

	return 0;
}


//
// return the size of the last directory entry
//
u32 get_filesize(void)
{
	return FileSize;
}


//
// return the long name of the last directory entry
//
u08 *get_filename(void)
{	
	return long_name_buffer;
}

//
// return the directory of the last directory entry
//
u08 *get_dirname(void)
{	
	return dir_name_buffer;
}


//
// load a clusterfull of data
//
void load_sectors(u08 *buffer)
{

	if (!MyCluster)
		return;

	// read cluster
	while ( ATA_Read_Sectors_LBA( 	DRIVE0,
   									clust2sect(MyCluster),
									Sectors_Per_Cluster,
           	                		buffer) != 0)
    {
    	UART_SendByte('*');
    	ATA_SW_Reset();
    };

	MyCluster = next_cluster(MyCluster);

}

//
// find next cluster in the FAT chain
//
u32 next_cluster(u32 cluster)
{
	u32 FatOffset = cluster << 2;	
	u32 sector    = FirstFATSector + (FatOffset / Bytes_Per_Sector);
	u16 offset 	= FatOffset % Bytes_Per_Sector;

	if (sector != FatInCache)
	{
		// read sector
	    while (ATA_Read_Sectors_LBA(DRIVE0,
	   								sector,
									1,
	           	                	(u08*)FATCACHE) != 0)
	    {
	    	UART_SendByte('&');
	    	ATA_SW_Reset();
	    };

		FatInCache = sector;
    }
           	                
	FatOffset =  (*((u32*) &((char*)FATCACHE)[offset])) & FAT32_MASK;

	if ( FatOffset >= (CLUST_EOFS & FAT32_MASK) )
		FatOffset = 0;

	return FatOffset;
}
