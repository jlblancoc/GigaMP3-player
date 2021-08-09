
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
#include <interrupt.h>
#include <progmem.h>

#include "types.h"
#include "delay.h"
#include "ata_if.h"
#include "uart.h"


//#define DEBUG_ATA	1

 
//----------------------------------------------------------------------------
// Select address and CS signals
//
// addressing bits
// 35 DA0	A0	0x01	Address Line 0
// 33 DA1	A1	0x02	Address Line 1
// 36 DA2	A2	0x04	Address Line 2
//
// chip selects
// 37 CS0	A3 	0x08	Command Block Select
// 38 CS1	A4	0x10	Control Block Select
//
//
//----------------------------------------------------------------------------
u08 SetAddress(u08 cs, u08 adr) 
{ 
	u16 i;
	
  	if (cs==CTRL)  
		i = adr+0x08;		// select A4 low -> CS1 -> CTRL
	else 
		i = adr+0x10;		// select A3 low -> CS0 -> CMD

	return *(u08 *) (i+0xE000);
	// dummy return to avoid optimization problems
}



//----------------------------------------------------------------------------
// Read data BYTE from Drive
//----------------------------------------------------------------------------
u08 ReadBYTE(u08 cs, u08 adr) 
{ 
  	u08 tmp; 

  	SetAddress(cs,adr); 

	cbi(MCUCR, SRE);	// disable RAM

	outp(0x00, DDRA);	// port A as input
	outp(0x00, DDRC);	// port C as input

	cbi(PORTB, 1);		// set DIOR lo
	asm volatile ("nop");	// allow pin change
	tmp = inp(PINA);	// read byte
	sbi(PORTB, 1);		// set DIOR hi
	sbi(MCUCR, SRE);	// enable RAM

  	return tmp;
}
 
 
//----------------------------------------------------------------------------
// Write data BYTE to Drive
//----------------------------------------------------------------------------
void WriteBYTE(u08 cs, u08 adr, u08 dat) 
{ 
  	SetAddress(cs,adr); 

	outp(0xff, DDRA);	// port A as output
	outp(0xff, DDRC);	// port C as output

	cbi(MCUCR, SRE);	// disable RAM
	asm volatile ("nop");	// allow pin change
	cbi(PORTB, 0);		// set DIOW lo
	asm volatile ("nop");	// allow pin change
	outp(dat, PORTA);	// write byte
	sbi(PORTB, 0);		// set DIOW hi
	sbi(MCUCR, SRE);	// enable RAM
}
 


void DiskErr(void)
{
	u08 b;

	sei();
	b = ReadBYTE(CMD,1);	
	PRINT("Error : "); 
	UART_Printfu08(b); 
	EOL();

/*	
	EOL();
	b = ReadBYTE(CMD,1);	
	PRINT("Error Reg : "); UART_Printfu08(b); EOL();
	b = ReadBYTE(CMD,2);
	PRINT("Sector Cnt: "); UART_Printfu08(b); EOL();
	b = ReadBYTE(CMD,3);
	PRINT("Sector Nbr: "); UART_Printfu08(b); EOL();
	b = ReadBYTE(CMD,4);
	PRINT("Cylindr Lo: "); UART_Printfu08(b); EOL();
	b = ReadBYTE(CMD,5);
	PRINT("Cylindr Hi: "); UART_Printfu08(b); EOL();
	b = ReadBYTE(CMD,6);
	PRINT("Device/Hd : "); UART_Printfu08(b); EOL();
	b = ReadBYTE(CMD,7);
	PRINT("Status    : "); UART_Printfu08(b); EOL();
*/
}


 
#define MINIBUFFERSIZE	16
u16 minibuffer[MINIBUFFERSIZE]; 

//----------------------------------------------------------------------------
// Read one or more sectors, identified by drive, head, track and sector
// Returns 0 if no error detected
//---------------------------------------------------------------------------- 
u08 ATA_Read_Sectors(	u08 Drive, 
                		u08 Head, 
                		u16 Track, 
                		u08 Sector,
                		u16 numsectors,
                		u08 *Buffer) 
{
  	u16 i,j;
  	u08 hi,lo,err = 0;
	u16 *pB = (u16*) Buffer;

  	// Prepare parameters...
  	WriteBYTE(CMD,6, 0xA0+(Drive ? 0x10:00)+Head); // CHS mode/Drive/Head

	while ( ( ReadBYTE(CMD,7) & (SR_BUSY|SR_DRDY) ) == SR_BUSY ); 	// Wait for ! BUSY

  	WriteBYTE(CMD,5, Track>>8);  	// MSB of track
  	WriteBYTE(CMD,4, Track);     	// LSB of track
  	WriteBYTE(CMD,3, Sector);    	// sector
  	WriteBYTE(CMD,2, numsectors); // # of sectors

  	// Issue read sector command...
  	WriteBYTE(CMD,7, 0x21);      // Read sector(s) command

	delay(5);					// wait 5 uS


	// for each sector requested 
	for (i=0; i<(512/MINIBUFFERSIZE)*numsectors; i+=2) 	
	{

		// loop reading ALT STATUS until BUSY is cleared
		while ( ( ReadBYTE(CTRL,6) & SR_BUSY) == SR_BUSY ); 	// Wait for ! BUSY
	
	
		// then read STATUS register to reset interrupt
		err = ReadBYTE(CMD,7);
	
		if ( ! (err & SR_DRQ) )	// failure if DRQ not set
		{
			sei();
			PRINT("NO DATA");
			return 1;
		}
	
		if ( err & SR_ERR )		//fail on error
		{
			sei();
			PRINT("WR ERR");
			return 1;
		}
	
	
		outp(0x00, DDRA);	// port A as input
		outp(0x00, DDRC);	// port C as input

		// setup addressing and chip selects
		SetAddress(CMD, 0); 
		
		cbi(MCUCR, SRE);	// disable RAM

		for (j=0;j<MINIBUFFERSIZE;j++)
		{	
			cbi(PORTB, 1);		// set DIOR lo
	
			asm volatile ("nop");	// allow pin change. This is absolutely needed !
		
			lo = inp(PINA);	// read lo byte
			hi = inp(PINC);	// read hi byte
		
			sbi(PORTB, 1);		// set DIOR hi
			
			minibuffer[j] = (hi<<8)+lo;
		}

		sbi(MCUCR, SRE);	// enable RAM

		for (j=0;j<MINIBUFFERSIZE;j++)
			*pB++ = minibuffer[j];
		
	}

  // Return the error status

	return (err & SR_ERR) ? 1:0;
}


//----------------------------------------------------------------------------
// Read one sector or more sectors, identified by LBA (Logical Block Address)
// Returns 0 if no error detected
//---------------------------------------------------------------------------- 
u08 ATA_Read_Sectors_LBA(	u08 Drive, 
							u32 lba,
							u16 numsectors,
                       		u08 *Buffer)
{
   	u16 cyl, head, sect;
   	u08 r;

#ifdef DEBUG_ATA
	sei();
	PRINT("ATA LBA read ");
	UART_Printfu32(lba); UART_SendByte(' ');
	UART_Printfu16(numsectors); UART_SendByte(' ');
	UART_Printfu16((u16)Buffer); 
	EOL();
#endif

   	sect = (u16) ( lba & 0x000000ffL );
   	lba = lba >> 8;
   	cyl = (u16) ( lba & 0x0000ffff );
   	lba = lba >> 16;
   	head = ( (u16) ( lba & 0x0fL ) ) | 0x40;

   	r = ATA_Read_Sectors( Drive, head, cyl, sect, numsectors, Buffer );

	if (r)
		DiskErr();
   	return r;
}                            		


u08 ATA_SW_Reset(void)
{
//	PRINT("ATA SW RESET\r\n");
	WriteBYTE(CTRL, 6, 0x06);	// SRST and nIEN bits
    delay(10);	// 10uS delay
	WriteBYTE(CTRL, 6, 0x02);	// nIEN bit
    delay(10);	// 10 uS delay
    while ( (ReadBYTE(CMD,7) & (SR_BUSY|SR_DRDY)) == SR_BUSY ); // Wait for DRDY and not BUSY
  	return ReadBYTE(CMD, 7) + ReadBYTE(CMD, 1);
}
