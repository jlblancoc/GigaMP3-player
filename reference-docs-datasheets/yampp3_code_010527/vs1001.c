
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
#include <progmem.h>
#include "types.h"
#include "delay.h"
#include "vs1001.h"
#include "uart.h"

//
// VS1001 commands
//
#define VS1001_READ		0x03
#define VS1001_WRITE	0x02


//
// read one or more word(s) from the VS1001 Control registers
//
void vs1001_read(u08 address, u16 count, u16 *pData)
{
	cbi( MP3_PORT, MP3_PIN);	// xCS lo

	outp(VS1001_READ, SPDR);
	loop_until_bit_is_set(SPSR, SPIF);    

	outp(address, SPDR);
	loop_until_bit_is_set(SPSR, SPIF);    

	while (count--)
	{
		outp(0x00, SPDR);
		loop_until_bit_is_set(SPSR, SPIF);    
		*pData = ((u16)inp(SPDR)) << 8;

		outp(0x00, SPDR);
		loop_until_bit_is_set(SPSR, SPIF);    
		*pData |= inp(SPDR);

		pData++;
	}

	sbi( MP3_PORT, MP3_PIN);	// xCS hi

	//this is absolutely neccessary!
	delay(5); //wait 5 microseconds after sending data to control port
}

//
// write one or more word(s) to the VS1001 Control registers
//
void vs1001_write(u08 address, u16 count, u16 *pData)
{
	cbi( MP3_PORT, MP3_PIN);	// xCS lo

	outp(VS1001_WRITE, SPDR);
	loop_until_bit_is_set(SPSR, SPIF);    

	outp(address, SPDR);
	loop_until_bit_is_set(SPSR, SPIF);    

	while (count--)
	{
		outp((u08)((*pData) >> 8), SPDR);
		loop_until_bit_is_set(SPSR, SPIF);    
		
		outp((u08)(*pData), SPDR);
		loop_until_bit_is_set(SPSR, SPIF);    
	
		pData++;
	}

	sbi( MP3_PORT, MP3_PIN);	// xCS hi

	//this is absolutely neccessary!
	delay(6); //wait 5 microseconds after sending data to control port
}


/****************************************************************************
**
** MPEG Data Stream
**
****************************************************************************/

//
// send a byte to the VS1001 MPEG stream
//
inline void vs1001_send_data(u08 b)
{
	
	sbi( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync hi

	outp(b, SPDR);		// send data

	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	
	cbi( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync lo

	// wait for data to be sent
	loop_until_bit_is_set(SPSR, SPIF);    

}


//
// send a burst of 32 data bytes to the VS1001 MPEG stream
//
inline void vs1001_send_32(u08 *p)
{
	u16 j;

	sbi( BSYNC_PORT,   BSYNC_PIN ); 		// byte sync hi
	for (j=0;j<32;j++)
	{
//		sbi( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync hi
		outp(*p++, SPDR);		// send data
//		asm volatile("nop");
//		asm volatile("nop");
//		asm volatile("nop");
//		cbi( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync lo

		// wait for data to be sent
		loop_until_bit_is_set(SPSR, SPIF);    
	}
	cbi( BSYNC_PORT,   BSYNC_PIN ); 		// byte sync lo
}



/****************************************************************************
**
** Init and helper functions
**
****************************************************************************/


// setup I/O pins and directions for
// communicating with the VS1001
void vs1001_init_io(void)
{
	u08 dummy;
	// setup BSYNC
	sbi( BSYNC_PORT-1, BSYNC_PIN ); 	// pin is output for BSYNC
	cbi( BSYNC_PORT,   BSYNC_PIN ); 	// output low

	// set the MP3/ChipSelect pin hi
	sbi( MP3_PORT-1, MP3_PIN); 			// pin output for xCS
	sbi( MP3_PORT,   MP3_PIN); 			// output hi (select MP3)

	// set the /Reset pin hi
//	sbi( RESET_PORT-1, RESET_PIN); 		// pin output 
//	sbi( RESET_PORT,   RESET_PIN); 		// output hi

	// setup serial data interface :
	// clock = f/4
	// select clock phase positive going in middle of data
	// master mode
	// enable SPI

	// setup serial data I/O pins

	sbi(DDRB, PB5);	// set MOSI a output
	sbi(DDRB, PB4);	// SS must be output for Master mode to work

	sbi(DDRB, PB7);	// set SCK as output
	cbi(PORTB, PB7);// set SCK lo

	outp((1<<MSTR)|(1<<SPE) /*| (1<<SPR0)*/, SPCR );	// 2 MHz clock

	dummy = inp(SPSR);	// clear status
}


// setup the VS1001 chip for decoding
void vs1001_init_chip(void)
{

	//do a software reset

	delay(3000);
	vs1001_reset();

	// ande flush the buffers
	delay(3000);
	vs1001_nulls(32);
}

// reset the VS1001
void vs1001_reset(void)
{
	u16 buf[2];

	delayms(200);		// wait 200 mS
	
	// set SW reset bit	
	buf[0] = 0x04;
	vs1001_write(0,1,buf);	// set bit 2

	delayms(2);		// wait 2 mS

	loop_until_bit_is_set(DREQ_PORT - 2, DREQ_PIN); //wait for DREQ

	// set CLOCKF to compensate for a 24 MHz x-tal
	buf[0] = 12000;
	vs1001_write(3,1,buf);	

	vs1001_nulls(1024);
	    
}


//
// send a number of zero's to the VS1001
//
void vs1001_nulls(u16 nNulls)
{
	while (nNulls--)
		vs1001_send_data(0);
}


//
// Set the VS1001 volume
//
void vs_1001_setvolume(u08 left, u08 right)
{
	u16 buf[2];

	buf[0] = (((u16)left) << 8) | (u16)right;

	vs1001_write(11, 1, buf);
}



