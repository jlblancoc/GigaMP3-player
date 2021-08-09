
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


// 32 k External RAM

#define EXTRAM_START	0x260
#define EXTRAM_END		0x7FFF	

#define SECTOR_BUFFER	0x0300	// length 512
#define SCROLL_LINE		0x0500	// length 256
#define LONGNAME_BUF	0x0600	// length 256
#define DIRNAME_BUF		0x0700	// length 256
#define FATCACHE		0x0800	// length 512

//free from 0x0A00 - 0x1FFF


// define memory for the double buffers
// each buffer is 12k (3 clusters, 24 sectors) long
// 
// buffer_1	0x2000 - 0x4FFF
// buffer_2	0x5000 - 0x7FFF

#define BUFFERSTART		0x2000
#define BUFFERSIZE		0x3000

