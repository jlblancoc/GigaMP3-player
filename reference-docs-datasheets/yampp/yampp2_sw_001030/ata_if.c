
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
#include <progmem.h>
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
char SetAddress(unsigned char cs, unsigned char adr) 
{ 
	unsigned char/*int*/ i;
	
  	if (cs==CTRL)  
		i = adr+0x08;		// select A4 low -> CS1 -> CTRL
	else 
		i = adr+0x10;		// select A3 low -> CS0 -> CMD

// new handling
/*
	cbi(MCUCR, SRE);	// disable RAM interface

	outp(0xff, DDRA);	// port A as output

	// turn off ALE
	cbi(PORTD,2);
	// LE high
	asm volatile ("nop");	// allow pin change
	sbi(PORTD,3);
	
	asm volatile ("nop");	// allow pin change
	outp(i,PORTA);

	asm volatile ("nop");	// allow pin change
	// LE low
	cbi(PORTD,3);
	
	return 0;
*/
	return *(unsigned char *) (i+0xE000);

	// dummy return to avoid optimization problems
}

//----------------------------------------------------------------------------
// Read data WORD from Drive
//----------------------------------------------------------------------------
/*unsigned int ReadWORD(unsigned char cs, unsigned char adr) 
{ 
  	unsigned int tmp; 

  	SetAddress(cs,adr); 

	cbi(MCUCR, SRE);	// disable RAM
	
	outp(0x00, DDRA);	// port A as input
	outp(0x00, DDRC);	// port C as input

	cbi(PORTB, 1);		// set DIOR lo

	asm volatile ("nop");	// allow pin change
	
	tmp = inp(PINA);		// read lo byte
	tmp = (tmp<<8) + inp(PINC);	// read hi byte
	
	sbi(PORTB, 1);		// set DIOR hi

	sbi(MCUCR, SRE);	// enable RAM

  	return tmp;

}
*/ 
//----------------------------------------------------------------------------
// Read data BYTE from Drive
//----------------------------------------------------------------------------
unsigned char ReadBYTE(unsigned char cs, unsigned char adr) 
{ 
  	unsigned char tmp; 

  	SetAddress(cs,adr); 

	cbi(MCUCR, SRE);	// disable RAM

	outp(0x00, DDRA);	// port A as input
	outp(0x00, DDRC);	// port C as input

	cbi(PORTB, 1);		// set DIOR lo
	asm volatile ("nop");	// allow pin change
	tmp = inp(PINA);	// read byte
	sbi(PORTB, 1);		// set DIOR hi
	sbi(MCUCR, SRE);	// enable RAM
	// reenable ALE
//	sbi(PORTD,2);

  	return tmp;
}
 
//----------------------------------------------------------------------------
// Write data WORD to Drive
//----------------------------------------------------------------------------
/*void WriteWORD(unsigned char cs, unsigned char adr, unsigned int dat) 
{ 
  	SetAddress(cs,adr); 

	cbi(MCUCR, SRE);	// disable RAM
	
	outp(0xff, DDRA);	// port A as output
	outp(0xff, DDRC);	// port C as output

	// NOTE NOTE !!  maybe LSB and MSB should be swapped !! was so in old code
	
	cbi(PORTB, 0);		// set DIOW lo

	outp(dat, PORTA);		// write lo byte
	outp((dat>>8), PORTC);	// write hi byte

//	asm volatile ("nop");	// allow pin change
	
	sbi(PORTB, 0);		// set DIOW hi

//	asm volatile ("nop");	// allow pin change

	sbi(MCUCR, SRE);	// enable RAM

}
*/
 
//----------------------------------------------------------------------------
// Write data BYTE to Drive
//----------------------------------------------------------------------------
void WriteBYTE(unsigned char cs, unsigned char adr, unsigned char dat) 
{ 
  	SetAddress(cs,adr); 

	outp(0xff, DDRA);	// port A as output
	outp(0xff, DDRC);	// port C as output

	cbi(MCUCR, SRE);	// disable RAM
	cbi(PORTB, 0);		// set DIOW lo
	outp(dat, PORTA);	// write byte
	sbi(PORTB, 0);		// set DIOW hi
	sbi(MCUCR, SRE);	// enable RAM
	// reenable ALE
//	sbi(PORTD,2);
}
 


void DiskErr(void)
{
	unsigned char b;

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
unsigned int minibuffer[MINIBUFFERSIZE]; 
//----------------------------------------------------------------------------
// Read one sector, identified by drive, head, track and sector
// Returns contents of the Error Register (0x00 is no error detected)
//---------------------------------------------------------------------------- 
unsigned char ATA_Read_Sectors(	unsigned char Drive, 
                				unsigned char Head, 
                				unsigned int Track, 
                				unsigned char Sector,
                				unsigned int numsectors,
                				unsigned char *Buffer) 
{
  	unsigned int i,j;
  	unsigned char hi,lo;
	unsigned int *pB = (unsigned int*) Buffer;

  	// Prepare parameters...
  	WriteBYTE(CMD,6, 0xA0+(Drive ? 0x10:00)+Head); // CHS mode/Drive/Head
  	WriteBYTE(CMD,5, Track>>8);  	// MSB of track
  	WriteBYTE(CMD,4, Track);     	// LSB of track
  	WriteBYTE(CMD,3, Sector);    	// sector
  	WriteBYTE(CMD,2, numsectors); // # of sectors

  	// Issue read sector command...
  	WriteBYTE(CMD,7, 0x21);      // Read sector(s) command

	delay(100);

	while ( ((lo=ReadBYTE(CMD,7)) & SR_BSY) == SR_BSY ); 	// Wait for ! BSY

	if (lo & SR_ERR)
	{
		PRINT("WR ERR");
		return 1;
	}


	outp(0x00, DDRA);	// port A as input
	outp(0x00, DDRC);	// port C as input

	// Two bytes at a time
	// 22mS/cluster
	// 5 uS/read
	for (i=0; i<(512/MINIBUFFERSIZE)*numsectors; i+=2) 	
	{
		SetAddress(CMD, 0); 
		
		cbi(MCUCR, SRE);	// disable RAM

		// new stuff need this
//		outp(0x00, DDRA);	// port A as input

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
		// reenable ALE
//		sbi(PORTD,2);

		for (j=0;j<MINIBUFFERSIZE;j++)
			*pB++ = minibuffer[j];
		
	}


  // Return the error bit from the status register...

	lo = ReadBYTE(CMD,7);				// read status register

	return (lo & SR_ERR) ? 1:0;

}


unsigned char ATA_Read_Sectors_LBA(	unsigned char Drive, 
									unsigned long lba,
									unsigned int numsectors,
                            		unsigned char *Buffer)
{
   	unsigned int cyl, head, sect;
   	unsigned char r;

#ifdef DEBUG_ATA
	PRINT("ATA LBA read ");
	UART_Printfu32(lba); UART_SendByte(' ');
	UART_Printfu16(numsectors); UART_SendByte(' ');
	UART_Printfu16((unsigned int)Buffer); 
	EOL();
#endif

   	sect = (int) ( lba & 0x000000ffL );
   	lba = lba >> 8;
   	cyl = (int) ( lba & 0x0000ffff );
   	lba = lba >> 16;
   	head = ( (int) ( lba & 0x0fL ) ) | 0x40;

   	r = ATA_Read_Sectors( Drive, head, cyl, sect, numsectors, Buffer );

	if (r)
		DiskErr();
   	return r;
}                            		


 
//----------------------------------------------------------------------------
// Set drive mode (STANDBY, IDLE)
//----------------------------------------------------------------------------
/*#define STANDBY 0
#define IDLE    1
#define SLEEP   2 
*/ 

/*
unsigned char SetMode(unsigned char DriveNo, unsigned char Mode, unsigned char PwrDown) 
{
  WriteBYTE(CMD, 6, 0xA0 + (DriveNo ? 0x10:0x00)); // Select drive
  WriteBYTE(CMD, 2, (PwrDown ? 0x01:0x00)); // Enable automatic power down
  switch (Mode) 
  {
    case STANDBY: WriteBYTE(CMD,7, 0xE2); break;
    case IDLE:    WriteBYTE(CMD,7, 0xE3); break;
    // NOTE: To recover from sleep, either issue a soft or hardware reset !
    // (But not on all drives, f.ex seagate ST3655A it's not nessecary to reset
    // but only to go in Idle mode, But on a Conner CFA170A it's nessecary with
    // a reset)
    case SLEEP:   WriteBYTE(CMD,7, 0xE6); break;
  }
  Timer10mSec=10000;
  while ((ReadBYTE(CMD,7) & 0xC0)!=0x40 && Timer10mSec); // Wait for DRDY & NOT BUSY 
  if (Timer10mSec==0) return 0xFF;                       //   or timeout
 
  // Return the error register...
  return ReadBYTE(CMD, 1);
}

*/
 
//----------------------------------------------------------------------------
// Show all IDE registers
//---------------------------------------------------------------------------- 
/*void ShowRegisters(unsigned char DriveNo) 
{ 
	WriteBYTE(CMD,6, 0xA0 + (DriveNo ? 0x10:0x00)); // Select drive
	
	PRINT("Reg 0=");	UART_Printfu08(ReadBYTE(CMD, 0));	UART_SendByte(' ');
	PRINT("1=");		UART_Printfu08(ReadBYTE(CMD, 1));	UART_SendByte(' ');
	PRINT("2=");		UART_Printfu08(ReadBYTE(CMD, 2));	UART_SendByte(' ');
	PRINT("3=");		UART_Printfu08(ReadBYTE(CMD, 3));	UART_SendByte(' ');
	PRINT("4=");		UART_Printfu08(ReadBYTE(CMD, 4));	UART_SendByte(' ');
	PRINT("5=");		UART_Printfu08(ReadBYTE(CMD, 5));	UART_SendByte(' ');
	PRINT("6=");		UART_Printfu08(ReadBYTE(CMD, 6));	UART_SendByte(' ');
	PRINT("7=");		UART_Printfu08(ReadBYTE(CMD, 7));	UART_SendByte(' ');
	EOL();
} 
*/

unsigned char ATA_SW_Reset()
{
	WriteBYTE(CTRL, 6, 0x06);	// SRST and nIEN bits
    delay(10);	// 10uS delay
	WriteBYTE(CTRL, 6, 0x02);	// nIEN bit
    delay(10);	// 10 uS delay
   
    while ( (ReadBYTE(CMD,7) & 0xC0) != 0x40 ); // Wait for DRDY and not BSY
    
  	return ReadBYTE(CMD, 7) + ReadBYTE(CMD, 1);
}
/*
unsigned char ATA_Idle(unsigned char Drive)
{

  WriteBYTE(CMD, 6, 0xA0 + (Drive ? 0x10:0x00)); // Select drive
  WriteBYTE(CMD,7, 0xE1);

  while ((ReadBYTE(CMD,7) & 0xC0)!=0x40); // Wait for DRDY & NOT BUSY 

  // Return the error register...
  return ReadBYTE(CMD, 1);
}
*/