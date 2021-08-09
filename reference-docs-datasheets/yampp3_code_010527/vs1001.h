
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




#define MP3_PORT	PORTB
#define BSYNC_PORT	PORTB    
#define DREQ_PORT	PORTB

#define MP3_PIN		PB4				// MP3 control bit
#define	BSYNC_PIN	PB3				// BSYNC signal
#define DREQ_PIN	PB2				// DREQ signal


// setup I/O pins and directions for
// communicating with the VS1001
void vs1001_init_io(void);

// setup the VS1001 chip for decoding
void vs1001_init_chip(void);

// reset the VS1001
void vs1001_reset(void);

// send a number of zero's to the VS1001
void vs1001_nulls(u16 nNulls);

// read one or more word(s) from the VS1001 Control registers
void vs1001_read(u08 address, u16 count, u16 *pData);

// write one or more word(s) to the VS1001 Control registers
void vs1001_write(u08 address, u16 count, u16 *pData);

// set the VS1001 volume
void vs_1001_setvolume(u08 left, u08 right);

// send MP3 data
inline void vs1001_send_data(u08 b);

// send MP3 data in 32 byte blocks
inline void vs1001_send_32(u08 *p);

