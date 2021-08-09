
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


// 32 k External RAM

#define EXTRAM_START	0x260
#define EXTRAM_END		0x7FFF	




#define SECTOR_BUFFER	0x1000	// length 512
#define SCROLL_LINE		0x1200
#define I2CDATA			0x1300
#define LONGNAME_BUF	0x1400	// length 256
#define MAS_BUFFER		0x1600  // variable, allow 512 
#define BUFFERLIST		0x1800	// allow up to 32 structures
#define DIRNAME_BUF		0x1900	// length 256

#define BUFFERSTART		0x1B00



#define FATCACHE		0x7E00		// last 512 bytes



// our simple allocator
/*
unsigned char *malloc(unsigned int nSize);
*/
