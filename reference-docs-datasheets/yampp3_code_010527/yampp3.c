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



/*
	PIN assignements on test board
	
	PA0-PA7		data bus 0..7 + Address 0..7
	PC0-=C7		address 8..15	
	
	PB0		T0		DIOW
	PB1		T1		DIOR
	PB2		AIN0	DEMAND
	PB3		AIN1	BSYNC		
	PB4		SS		MP3
	PB5		MOSI	SO
	PB6		MISO 	SI
	PB7		SCK		SCK
	

	PD0		RxD		RS-232
	PD1		TxD		RS-232
	PD2		INT0	IR_INPUT
	PD3		INT1	KEY_INPUT
	PD4		T0		
	PD5		T1		LCD_ENABLE
	PD6		WR		RD
	PD7		RD		WR	


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	At 8MHz clock :			100 mS divisor		8bit overflow
	tClk 		= 125 nS 	   200000				32 uS (31250 Hz)
	tClk/8 		=   1 uS		50000 			   128 uS (7812.5 Hz)
	tClk/64		=   8 uS	 	12500			   512 us (1953.125 Hz)
	tClk/256	=  32 uS		 3125	 		  2048 uS
	tClk/1024	= 128 uS	 	  781.25		  8192 uS
 
 
*/


#include <io.h>
#include <progmem.h>
#include <eeprom.h>
//#include <signal.h>
#include <sig-avr.h>
#include <interrupt.h>

#include "types.h"
#include "uart.h"
#include "lcd.h"
#include "ata_if.h"
#include "delay.h"
#include "fat.h"
#include "mem.h"

#include "rec80.h"


#include "vs1001.h"



// eeprom positions
#define EEPROM_VOLUME	0x0010
#define EEPROM_AUTOPLAY	0x0011
#define EEPROM_SONGPOS	0x0012


/* another way
static u08 volume __attribute__ ( ( section(".eeprom") ) ) = 12;	//-6dB
*/


#define HEXDUMP 1


extern u08 *SectorBuffer;

#ifdef HEXDUMP

void HexDump( u08 *buffer, u16 len ) 
{
    u16 i,j,k = 0;

    EOL();
    for(i=0;i<len/16;i++,k+=16)
    {
        UART_Printfu16(k);	
        UART_SendByte(' ');
        for(j=0;j<16;j++)
        {
            UART_Printfu08(buffer[i*16+j]);	
            UART_SendByte(' ');
        }
        PRINT("    ");
        for(j=0;j<16;j++)
        {
            if (buffer[i*16+j] >= 0x20)
                UART_SendByte(buffer[i*16+j]);
            else
                UART_SendByte('.');
        }
        EOL();
    }

}

void sectordump(u32 sector)
{
	ATA_Read_Sectors_LBA(DRIVE0,sector,1,SectorBuffer);
	UART_Printfu32(sector);
	HexDump(SectorBuffer, 512);
}

#endif




u08 scroll_length;
u08 scroll_pos = 0;
u08 scroll_flag = 0;
char *scroll_line;


u08 isPlaying = 0;
u08 *outptr;

u08 *buffer1;
u08 *buffer2;
u08 *buffer3;


u32 lbasector = 0;
u32 temp;
u08 *updbuf;


typedef enum
{	
	EV_IDLE,
	EV_PLAY,
	EV_STOP,
	EV_NEXT,
	EV_PREV,
	EV_VOLUP,
	EV_VOLDN,
	EV_NEUTRAL,
//	EV_MUTE,
	EV_LASTEVENT	
} event_e;	



// prescaler set to 32 uS
// 3125 ticks = 100 mS
#define TI1_H	(((u16)-3125) >> 8)
#define TI1_L	(((u16)-3125) & 0xff )

volatile u16  p_time;
volatile u08 time_flag = 0;

SIGNAL(SIG_OVERFLOW1)	//timer 1 overflow every 100 mS
{
	p_time++;
	time_flag++;
	scroll_flag++;
	outp(TI1_H, TCNT1H);					//reload timer 
	outp(TI1_L, TCNT1L);
}



event_e get_char_event(void)
{
	u08 c;
	
	switch ((c = UART_ReceiveByte())) 
    {

#ifdef HEXDUMP
		case '0' : case '1' : case '2' : case '3' : case '4' : 				
		case '5' : case '6' : case '7' : case '8' : case '9' :
			temp = temp*10 + c-'0';
			break;
			
		case 0x0d :
			lbasector = temp;
			temp = 0;
			break;

		case '+':
			sectordump(++lbasector);
			break;	      		
		case '-':
			sectordump(--lbasector);
			break;	      		
#endif
		
		case 'V' : return EV_VOLUP;
		case 'v' : return EV_VOLDN;
		case 'N' : return EV_NEUTRAL;
//		case 'm' : return EV_MUTE;
		

		case 'p' : return EV_PREV;
		case 'n' : return EV_NEXT;
		case 'g' : return EV_PLAY;
		case 'G' : return EV_STOP;
		default:   return EV_IDLE;
    }
    return EV_IDLE;
}


event_e ir_event(void)
{
	switch (get_rec80())
	{
		default:
		case 0: return EV_IDLE;
		case IR_PLAY: return EV_PLAY;
		case IR_STOP: return EV_STOP;
		case IR_PREV: return EV_PREV;
		case IR_NEXT: return EV_NEXT;
		case IR_VOLUP: return EV_VOLUP;
		case IR_VOLDN: return EV_VOLDN;
	}
}


//
// get a keyboard event
//
event_e get_key_event(void)
{
	static u08 keydown = 0;
	u08 i;
	event_e event = EV_IDLE;

	cbi(MCUCR, SRE);		// disable EXTRAM

	outp(0xff, DDRA);	// set port as output
	outp(0, PORTA);			// set all pins to 0
	asm volatile("nop");	// allow time for port to activate

	if (keydown)	// if a key was pressed earlier
	{
		// see if it has been released
		if (bit_is_set(PIND,PD4))	// check bit
		{
			// no key is down
			// return last key seen
			switch (keydown)
			{
				case 1 : event = EV_STOP; break;
				case 2 : event = EV_PLAY; break;
				case 3 : event = EV_PREV; break;
				case 4 : event = EV_NEXT; break;
			}
			keydown = 0;
		}
	}
	else
	{	
		if (bit_is_clear(PIND,PD4))	// check if a key is down
		{
			// a key is active, check which one
			for (i=0;i<8;i++)
			{
				outp(~(1<<i), PORTA);		// write bit mask
				asm volatile("nop");		// wait a while
				if (bit_is_clear(PIND,PD4))	// this bit ?
				{
					keydown = i+1;		// store the key
					break;				// and exit
				}
			}
		}
	}

	sbi(MCUCR, SRE);	// enable RAM
	return event;
}



void send_sinewave_beeps(void)
{
	u08 i,j,buf[8];
	
	// sine on
	buf[0] = 0x53;	buf[1] = 0xEF;	buf[2] = 0x6E;	buf[3] = 0x30;
	// sine off
	buf[4] = 0x45;	buf[5] = 0x78;	buf[6] = 0x69;	buf[7] = 0x74;

	for (j=0;j<3;j++)
	{
		for (i=0;i<4;i++)
			vs1001_send_data(buf[i]);
		vs1001_nulls(4);
		delayms(100);
		
		for (i=4;i<8;i++)
			vs1001_send_data(buf[i]);
		vs1001_nulls(4);
		delayms(100);
	}	
}





void userchars(void)
{
	u08 c;
	lcd_command(0x48);	// start at 2'nd definable character				
	for (c=0;c<8;c++)	lcd_data(0x10);	// 1 bar 
	for (c=0;c<8;c++)	lcd_data(0x18);	// 2 bars 
	for (c=0;c<8;c++)	lcd_data(0x1C);	// 3 bars 
	for (c=0;c<8;c++)	lcd_data(0x1E);	// 4 bars 
	for (c=0;c<8;c++)	lcd_data(0x1F);	// 5 bars 
	// 2 more user definable characters possible
}

void setvolume(u08 v)
{
	u08 c,a = 1;

	vs_1001_setvolume(v,v);
	eeprom_wb(EEPROM_VOLUME,v);

	lcd_command(0x40);
	for (c=0;c<8;c++)
	{
		if (v <= a)
			lcd_data(0xff);
		else
			lcd_data(0x00);
		a <<= 1;	
	}
	
}



void dispbar(u16 v)
{
	// 8 bars of 5 steps each = 40 steps
	u08 c,a;
	
	a = v / 5;	// # of full bars

	lcd_gotoxy(0,1);				

	for (c=0;c<a;c++)
		lcd_putchar(5);

	lcd_putchar(1+ v % 5);	
	
}



//
// random stuff
//

#define MY_RAND_MAX  	65535
static u32 seed = 1;

u16 do_rand(u32 *ctx)
{
	return ((*ctx = *ctx * 1103515245 + 12345) % ((u32)MY_RAND_MAX + 1));
}

u16 rand(void)
{
	return do_rand(&seed);
}

//
// end random
//






//----------------------------------------------------------------------------
// Main part
//----------------------------------------------------------------------------
int main(void) 
{

 	u16  i;
  	u08 c;
 	u08 *p, *q;
	u16 dentry = 0;
 	u32 filesize = 0;
	u08 volume = 12;
	event_e event = EV_IDLE;
//	u08 mute = 0;
	u08 autoplay;
	u16  barsize = 0;
	u16 barstep = 0;
	u16 filepos = 0;

	u16 random = 1;
	u16 last_song = 1;
	

 
 //------------------------------
 // Initialize 
 //------------------------------


//////////////////////////////////////////////////////////////////
// B - Port
//////////////////////////////////////////////////////////////////

	sbi(PORTB, 0);	// DIOW- hi
	sbi(DDRB, 0);	// pin PB0 is output, DIOW- 

	sbi(PORTB, 1);	// DIOR- hi
	sbi(DDRB, 1);	// pin PB1 is output, DIOR- 

	cbi(DDRB, 2);	// pin PB2 is input, DEMAND
	sbi(PORTB, 2);	// activate pullup

	// PB3 and PB4 is used for BSYNC and MP3 signals to VS1001

	// PB5 (MOSI) is used for SPI
	// PB6 (MISO) is used for SPI
	// PB7 (SCK) is used for SPI

//////////////////////////////////////////////////////////////////
// D - Port
//////////////////////////////////////////////////////////////////

	// PD0 and PD1 are TX and RX signals

	// PD2 is used for the IR receiver

	cbi(DDRD, 3);	// pin PD3 is keyboard input

	// PD4 is available
	
	// PD5 is LCD_ENABLE used by LCD module

	// PD6 and PD7 are RD and WR signals
	// but we need to set their default states used
	// when the external memory interface is disabled
    // they should then both be high to avoid disturbing
	// RAM contents
	
	sbi(DDRD, 6);		// as outputs
	sbi(DDRD, 7);

	sbi(PORTD, 6);		// and both hi
	sbi(PORTD, 7);


//////////////////////////////////////////////////////////////////
// Timer 1
//////////////////////////////////////////////////////////////////

	//
	// setup timer1 for a 100mS periodic interrupt
	//	
	
	outp(0, TCCR1A);
	outp(4, TCCR1B);		// prescaler /256  tPeriod = 32 uS
	outp(TI1_H, TCNT1H);	// load counter value hi
	outp(TI1_L, TCNT1L);	// load counter value lo


 //----------------------------------------------------------------------------
 // 
 //----------------------------------------------------------------------------

    lcd_init(0, LCD_FUNCTION_DEFAULT); 	// also sets up EXTRAM pins


	//
	// construct some user definable characters
	//
	userchars();
	
	//
	// show a '*' in all display positions, then clear
	// screen and display a welcome message
	//
	for (c=0;c<17;c++)
	{
		lcd_gotoxy(c,0);
		lcd_putchar('*');
		delayms(10);
	}
	for (c=16;c>0;c--)
	{
		lcd_gotoxy(c,1);
		lcd_putchar('*');
		delayms(10);
	}
	lcd_clrscr();
	lcd_puts(" yampp-3  Alive ");
	

	UART_Init();		// init RS-232 link

	vs1001_init_io();	// init

	init_rec80();		// set up IR receiver interface


	// setup some buffers
	SectorBuffer = (u08 *) SECTOR_BUFFER;	// 512 byte
	buffer1 = (u08 *) BUFFERSTART;			// 3 clusters, 12 k
	buffer2 = (u08 *) buffer1 + BUFFERSIZE;	// 3 clusters, 12 k
	buffer3 = (u08 *) buffer2 + BUFFERSIZE;	// end pointer


	// say hello
  	PRINT("yampp-3  ");
	UART_Puts(__DATE__);
	PRINT(" ");
	UART_Putsln(__TIME__);
	EOL();


//  	delayms(5);		// 5 mS

	// init VS1001
	vs1001_init_chip();

	// Init and wait for drive
  	WriteBYTE(CMD, 6, 0xA0); // Set drive/Head register, ie. select drive 0
    while ( (ReadBYTE(CMD,7) & (SR_BUSY|SR_DRDY)) == SR_BUSY ); // Wait for DRDY and not BUSY
  	PRINT("Ready!\r\n");

	// init randomizer from timer
	seed = 1 + inp(TCNT1L);

	// reset drive 
	ATA_SW_Reset();

	// read structures
	init_fat(DRIVE0);



	// load and set default volume
//	volume = eeprom_rb(EEPROM_VOLUME);
//	vs_1001_setvolume(volume,volume);

	volume = 12;		// -6 dB
	setvolume(volume);

	// Send three sinewave beeps to indicate we're starting up okay
	send_sinewave_beeps();


//
// TESTCODE to get number of songs on disk
//	
 	PRINT("Scanning... \r\n");

//	last_song = dirlist(DIRLIST_VERBOSE);
	last_song = dirlist(DIRLIST_SCAN);

 	PRINT("Found ");
	UART_Printfu16(last_song);
	PRINT(" Entries\r\n");
//
// END TESTCODE
//	


	// load autoplay and position settings
	autoplay = eeprom_rb(EEPROM_AUTOPLAY);
	dentry 	 = eeprom_rb(EEPROM_SONGPOS);	


	if (autoplay)
		event = EV_PLAY;




///////////////////////////////////////////////////////////////////////////////
// start things rolling
///////////////////////////////////////////////////////////////////////////////


	// enable timer1 interrupt
	sbi(TIMSK, TOIE1);	 	


	//
	// main loop
	//

  	while (1) 
  	{
		
		if (UART_HasChar())
			event = get_char_event();

		if (rec80_active)
			event = ir_event();

		if (event == EV_IDLE)
			event = get_key_event();

		switch (event)
		{
			default:
			case EV_IDLE: 
				break;

				
			case EV_PREV:
				dentry-=2;	
				if ((int)dentry <= -2)
					dentry = (u16)-1;

				// fall through		

			case EV_NEXT:
				dentry++;

				// fall through

			case EV_PLAY:

				lcd_clrscr();

				vs1001_reset();

				eeprom_wb(EEPROM_SONGPOS,dentry);

				// setup for play
				cli();
				filesize = get_dir_entry(dentry);
				sei();
				
				if (filesize == 0)	// no more
				{	
					// wrap
					dentry = 0;
					cli();
					get_dir_entry(dentry);
					sei();
				}	

				filesize = get_filesize() / 32;
				
				barsize = (u16) ((u32) filesize / 40);
				barstep = 0;
				filepos = 0;

				// start playing
				PRINT("\r\nPlaying #"); UART_Printfu08(dentry); UART_SendByte(' ');
				UART_Puts( get_dirname() ); 
				UART_Putsln( (p = get_filename()) );
								
				scroll_line = q = p;
				scroll_pos = 0;
				scroll_length = 0;

				while (*q++)
					scroll_length++;
				q--;
				*((u32*)q)++ = 0x207E7F20L;
				*q++ = 0;
				scroll_length += 4;

			    // preload buffers
			    
			    cli();
			    for (i=0;i<BUFFERSIZE*2;i+=0x1000)
					load_sectors(buffer1 + i);
				sei();

			    outptr = buffer1;
				isPlaying = 1;
				updbuf = 0;
				p_time = 0;
				scroll_flag = 0;
				time_flag = 0;


				// special MP3 header patch to strip
				// all invalid headers 
				{
					u16 i=0;
					
					while ( ( (*(u16 *)(outptr)) & 0xfeff) != 0xfaff ) 
					{
		    			*outptr++ = 0;
	    				if (i++ > 1000)
	    					break;
					}
				    outptr = buffer1;
				}
				/* end of special header patch */

				event = EV_IDLE;
				break;					

			case EV_STOP:
				isPlaying = 0;
				event = EV_IDLE;
				break;				

			case EV_VOLDN: 
				if (volume < 0xfe) volume++; 
				setvolume(volume);
				break;

			case EV_VOLUP: 
				if (volume > 0) volume--; 
				setvolume(volume);
				break;

			case EV_NEUTRAL: 
				volume = 12;		// -6 dB
				setvolume(volume);
				break;
/*
			case EV_MUTE:
				mute = 1-mute;
				break;
*/
		}
	
		// we always go back to IDLE
		// when exiting switch
		event = EV_IDLE;
	
	
		// time to update a buffer ?
		if (updbuf)
		{
			cli();
		    for (i=0;i<BUFFERSIZE;i+=0x1000)
				load_sectors(updbuf + i);
			sei();			    
			updbuf = (u08 *) 0;			// mark as done
		}
		
		if (isPlaying)
		{
			// pump some data to the decoder
			
			while ( bit_is_set(PINB, PB2) )			// while DREQ is hi 
			{
			
				//send data on SPI bus
				vs1001_send_32(outptr);
				outptr += 32;

				// check for buffer wrap
				if (outptr == buffer2)
					updbuf = buffer1;
				else if (outptr == buffer3)
				{
					updbuf = buffer2;
					outptr = buffer1;
				}

				// check if we're done
				filesize--;
				
				if (barstep++ == barsize)
				{
					filepos++;
					barstep = 0;
				}

				if (filesize <= 0)
				{
					isPlaying = 0;					// the end is near 
					// try autoplay next song
					
					if (random)
					{
						dentry = rand();
						dentry = dentry % last_song;
						event = EV_PLAY;
					}
					else
					{
						event = EV_NEXT;
					}
					PRINT("done\r\n");
					break;
				}
			}
			
			// scroll display every 300 ms
			if (scroll_flag >= 3)
			{
				lcd_gotoxy(0,0);
				
				for (c=0;c<16;c++)
					lcd_putchar( scroll_line[ (scroll_pos+c) % scroll_length ] );
				scroll_pos++;
				scroll_flag = 0;
			}    

			// update timer every 500mS
			if (time_flag >= 5)
			{

				dispbar(filepos);
			
				p = SectorBuffer;

				i = p_time / 10;	// seconds
			
				*p++ = '0' + (i / 3600);
				i %= 3600;

				*p++ = ':';
				
				*p++ = '0' + (i / 600);
				i %= 600;
				*p++ = '0' + (i / 60);

				*p++ = ':';

				i %= 60;
				*p++ = '0' + (i / 10);
				i %= 10;
				*p++ = '0' + i;			

				*p++ = 0;

				lcd_gotoxy(8,1);				
				lcd_puts(SectorBuffer);
				lcd_putchar(0);	// volume indicator
				time_flag = 0;
			}

		}//if (isPlaying)

  	}    
}

