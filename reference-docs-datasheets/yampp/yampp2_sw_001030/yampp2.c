

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


/*

	Power requirements
		CPU	(@4MHz)			4/8 mA	(3.3V/5V)
	SRAM				90 mA (active, 20 mA stby)
	DAC					5/8 mA	(3.3V/5V)
	MAS					55 mA
	Other				10 mA
						------
	Total				150-170 mA

	Disk				450-500 mA
	
	Grand finale		600-700 mA				


	Measurements show about 200-250 mA standby (no disk powerdown), and
	about 470 mA playing.
	
*/



/*
	PIN assignements on test board
	
	PA0-PA7		data bus 0..7 + Address 0..7
	PC0-=C7		address 8..15	
	
	PB0		T0		DIOW
	PB1		T1		DIOR
	PB2		AIN0	DEMAND
	PB3		AIN1	I2CD		
	PB4		SS		I2CC
	PB5		MOSI	SID
	PB6		MISO 	(not available)
	PB7		SCK		SIC
	

	PD0		RxD		RS-232
	PD1		TxD		RS-232
	PD2		INT0	IR_INPUT
	PD3		INT1	KEY_INPUT
	PD4		T0		free
	PD5		T1		LCD_E
	PD6		WR		RD
	PD7		RD		WR	


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 

	MAS DEMAND PIN @ 8 MHz
	
	 	 ___				____
	____|	|______________|	|____________

		1.5 mS     11 mS

	SIC frequency = 2 MHz  ( Prescaler = fClk/4 )

	About 3 kBits are sent in each burst

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
*/


#include <io.h>
#include <progmem.h>
#include <eeprom.h>
#include <signal.h>
#include <interrupt.h>

#include "uart.h"
#include "lcd.h"
#include "ata_if.h"
#include "delay.h"
#include "spi.h"
#include "i2c.h"
#include "fat.h"
#include "mas.h"
#include "dac.h"
#include "mem.h"

#include "rec80.h"



// eeprom positions
#define EEPROM_VOLUME	0x0010
#define EEPROM_AUTOPLAY	0x0011
#define EEPROM_SONGPOS	0x0012



//#define HEXDUMP 1
//#define MAS_INFO 1



#ifdef HEXDUMP

extern unsigned char *SectorBuffer;

void HexDump( unsigned char *buffer, int len ) 
{
    int i,j,k = 0;

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

void sectordump(unsigned long sector)
{
	ATA_Read_Sectors_LBA(DRIVE0,sector,1,SectorBuffer);
	UART_Printfu32(sector);
	HexDump(SectorBuffer, 512);
}

#endif


unsigned int scroll_length;
unsigned int scroll_pos = 0;
unsigned char scroll_flag = 0;
char * scroll_line  = (unsigned char *) SCROLL_LINE;
	


unsigned char * outptr;

unsigned char *buffer1;
unsigned char *buffer2;
unsigned char *buffer3;
unsigned char *buffer4;


unsigned char *i2cdata = (unsigned char *) I2CDATA;

unsigned long mycluster;


extern unsigned int  Sectors_Per_Cluster;

unsigned long lbasector = 0;
unsigned long temp;
unsigned char *updbuf;


typedef enum
{
	PS_STOPPED,
	PS_PLAYING,
	PS_PAUSED
} playstate_e;

playstate_e isPlaying = PS_STOPPED;


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
	EV_MUTE,
	EV_LASTEVENT	
} event_e;	



// 100 mS @ prescaler 4 (f/256)  (32 uS)
// - 3125
//#define TI1_H	0xf3
//#define TI1_L	0xcb


// 500 mS @ prescaler 5 (f/1024) (128 uS)
// - 3906
#define TI1_H	0xf0
#define TI1_L	0xbe

// 1000 mS @ prescaler 4 (f/256)  (32 uS)
// - 31250
//#define TI1_H	0x85
//#define TI1_L	0xee


volatile unsigned char time_flag = 0;
volatile char time[7];
volatile int time_now;


SIGNAL(SIG_OVERFLOW1)	//timer 1 overflow every 500 mS
{
	time_flag++;

	outp(TI1_H, TCNT1H);					//reload timer 
	outp(TI1_L, TCNT1L);

	if (isPlaying != PS_PLAYING)
		return;
		
	time_now++;
}


//
// Get a character event from the RS-232 port
//
event_e get_char_event(void)
{
	unsigned char c;
	
	switch ((c = UART_ReceiveByte())) 
    {
//				case 'r' : ATA_SW_Reset(); break;

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
		
/*		      	case 'R': 	
      		ShowRegisters(DRIVE0); 
      		break;
*/

		case 'V' : return EV_VOLUP;
		case 'v' : return EV_VOLDN;
		case 'N' : return EV_NEUTRAL;
		case 'm' : return EV_MUTE;

		case 'p' : return EV_PREV;
		case 'n' : return EV_NEXT;
		case 'g' : return EV_PLAY;
		case 'G' : return EV_STOP;
		default:   return EV_IDLE;
    }
}

/* 	uncomment if you want to use IR
	also uncomment code in REC-80
	
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

*/

//
// get a keyboard event
//
event_e get_key_event(void)
{
	static unsigned char keydown = 0;
	unsigned char i;
	event_e event = EV_IDLE;

	cbi(MCUCR, SRE);		// disable EXTRAM
	outp(0xff, PORTA-1);	// set port as output
	outp(0, PORTA);			// set all pins to 0
	asm volatile("nop");	// allow time for port to activate

	if (keydown)	// if a key was pressed earlier
	{
		// see if it has been released
		if (bit_is_set(PIND,PD3))	// check bit
		{
			// no key is down
			// return last key seen
			switch (keydown)
			{
				case 1 : event = EV_NEUTRAL; break;
				case 2 : event = EV_PLAY; break;
				case 3 : event = EV_PREV; break;
				case 4 : event = EV_NEXT; break;
			}
			keydown = 0;
		}
	}
	else
	{	
		if (bit_is_clear(PIND,PD3))	// check if a key is down
		{
			// a key is active, check which one
			for (i=0;i<8;i++)
			{
				outp(~(1<<i), PORTA);		// write bit mask
				asm volatile("nop");		// wait a while
				if (bit_is_clear(PIND,PD3))	// this bit ?
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





#ifdef MAS_INFO
//
// print some MAS identification strings e.t.c.
//
void mas_info(void)
{
    // request some MAS chip info
    i2c_send(0x3a,0x68, 6, "\xf0\x00\x00\x0a\x0f\xf6");		// ???
    delay(1000);
    i2c_receive(0x3a,0x69, 40, i2cdata );
	
    EOL();

    a1  = (ULONG) i2cdata[3] << 16;
    a1 += (ULONG) i2cdata[0] << 8;
    a1 += (ULONG) i2cdata[1];

    PRINT("MAS Version: ");
    UART_Printfu32(a1);
    EOL();

    a1  = (ULONG) i2cdata[7] << 16;
    a1 += (ULONG) i2cdata[4] << 8;
    a1 += (ULONG) i2cdata[5];

    PRINT("Design code: ");
    UART_Printfu32(a1);
    EOL();

    a1  = (ULONG) i2cdata[11] << 16;
    a1 += (ULONG) i2cdata[8] << 8;
    a1 += (ULONG) i2cdata[9];

    PRINT("Date: ");
    UART_Printfu32(a1);
    EOL();

    for (i=12;i<40;i+=4)
    {
    	UART_SendByte(i2cdata[i]);
    	UART_SendByte(i2cdata[i+1]);
	}
    EOL();
}

#endif





//----------------------------------------------------------------------------
// Main Lupe
//----------------------------------------------------------------------------
int main(void) 
{

 	unsigned int  i;
  	unsigned char c;
 	char *p, *q;
	unsigned int dentry = 0;
 	long filesize = 0;
	unsigned char volume = 0x2C;
	event_e event = EV_IDLE;
	unsigned char mute = 0;
	unsigned char autoplay;

 
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

	// PB3 and PB4 is used for I2C Bus

	// PB5 (MOSI) is used for SPI
	// PB6 (MISO) is used for SPI
	// PB7 (SCK) is used for SPI

//////////////////////////////////////////////////////////////////
// D - Port
//////////////////////////////////////////////////////////////////

	// PD0 and PD1 are TX and RX signals

	// PD2 is used for the IR receiver
	
	// PD3 is keyboard input

	cbi(DDRD, 3);	// pin PD3 is input
	cbi(PORTD, 3);	// disable pullups


	// PD4 is available
	
	// PD5 is used by LCD module

	// PD6 and PD7 are RD and WR signals
	// but we need to set their default states used
	// when the external memory interface is disabled
    // they should then both be high to avoid disturbing
	// RAM contents
	
	sbi(DDRD, 6);		// as outputs
	sbi(DDRD, 7);
	sbi(PORTD, 6);		// and both hi
	sbi(PORTD, 7);


 //----------------------------------------------------------------------------
 // 
 //----------------------------------------------------------------------------

    lcd_init(0/*(1<<LCD_ON_CURSOR)*/, LCD_FUNCTION_DEFAULT); 	// also sets up EXTRAM pins

	lcd_puts(" yampp-2 alive");


	UART_Init();	// init RS-232 link
	i2c_init();		// initialize I2C bus pins
	init_spi();		// set up SPI interface

/* 	uncomment if you want to use IR
	init_rec80();	// set up IR receiver interface
*/

	// say hello
  	PRINT("\nyampp-2\n");


 //----------------------------------------------------------------------------
 // Let things settle
 //----------------------------------------------------------------------------                        


	// wait for drive

  	PRINT("Wait..\n"); 
  	WriteBYTE(CMD, 6, 0xA0); // Set drive/Head register, ie. select drive 0
   	while (( (c=ReadBYTE(CMD,7)) & 0xC0)!=0x40); // Wait for DRDY and NOT BUSY
  	PRINT("Drive ready!\n");

	// reset drive 
	ATA_SW_Reset();

	// read structures
	init_fat(DRIVE0);

	// setup buffer structure

	buffer1 = (unsigned char *)BUFFERSTART;	 /* S_P_C is normally 8 */
	buffer2 = (unsigned char *)BUFFERSTART + Sectors_Per_Cluster * 512 * 1;
	buffer3 = (unsigned char *)BUFFERSTART + Sectors_Per_Cluster * 512 * 2;
	buffer4 = (unsigned char *)BUFFERSTART + Sectors_Per_Cluster * 512 * 3;

	MAS_init();		// init MAS decoder
	DAC_init();		// init D/A converter

#ifdef MAS_INFO
	mas_info();     // print some MAS chip info
#endif


	// load and set default volume
	volume = eeprom_rb(EEPROM_VOLUME);
	DAC_SetVolume(volume,volume);


	// load autoplay and position settings
	autoplay = eeprom_rb(EEPROM_AUTOPLAY);
	dentry 	 = eeprom_rb(EEPROM_SONGPOS);	

	if (autoplay)
		event = EV_PLAY;

///////////////////////////////////////////////////////////////////////////////


//setup timer1

	outp(0, TCCR1A);
	outp(5, TCCR1B);	// prescaler /1024  tPeriod = 128 uS
	outp(TI1_H, TCNT1H);
	outp(TI1_L, TCNT1L);
	sbi(TIMSK, TOIE1);	 	//enable timer1 interrupt

	sei();		// start timer

///////////////////////////////////////////////////////////////////////////////


  	while (1) 
  	{
		
		if (UART_HasChar())
			event = get_char_event();

/* uncomment this to use IR
		if (rec80_active)
			event = ir_event();
*/
		if (event == EV_IDLE)
			event = get_key_event();

		switch (event)
		{
			default:
			case EV_IDLE: 
				break;

			case EV_PREV:
				eeprom_wb(EEPROM_AUTOPLAY,0);
				dentry-=2;	
				if ((int)dentry <= -2)
					dentry = (unsigned int)-1;
				// fall through		

			case EV_NEXT:
				eeprom_wb(EEPROM_AUTOPLAY,1);
				dentry++;
				isPlaying = PS_STOPPED;
				// fall through

			case EV_PLAY:
				
				switch (isPlaying)
				{
					case PS_PAUSED :
						isPlaying = PS_PLAYING;
						break;
					case PS_PLAYING :
						isPlaying = PS_PAUSED;
						break;
					case PS_STOPPED :												

						eeprom_wb(EEPROM_SONGPOS,dentry);
					
						// setup for play
						if ( (mycluster = get_dir_entry(dentry,0)) == 0)	// no more
						{	
							event = EV_NEXT;
							dentry = -1;
							isPlaying = PS_STOPPED;
							break;
						}	
	
						filesize = get_filesize();
		
						// start playing
						PRINT("\r\nPlaying #"); UART_Printfu08(dentry); UART_SendByte(' ');
						UART_Puts( get_dirname() ); 
						UART_Putsln( (p = get_filename()) );
						q = scroll_line;
						scroll_pos = 0;
						scroll_length = 0;
						while ( (*q = *p) != 0)
						{	
							scroll_length++;
							q++;
							p++;
						}
		
						*((unsigned long*)q)++ = 0x20202020L;
						*q++ = 0;
		
		
					    // preload buffers
					    cli();
						load_sectors(mycluster,buffer1);
						mycluster = next_cluster(mycluster);
						load_sectors(mycluster,buffer2);
						mycluster = next_cluster(mycluster);
						load_sectors(mycluster,buffer3);
						mycluster = next_cluster(mycluster);
						sei();
						
					    outptr = buffer1;
						updbuf = 0;
						strcpy(time,"000:00");
						time_now = 0;
						isPlaying = PS_PLAYING;
						break;
				}
				event = EV_IDLE;
				break;					

			case EV_STOP:
				isPlaying = PS_STOPPED;
				event = EV_IDLE;
				break;				

			case EV_VOLUP: 
				if (volume < 0x38) volume++; 
				DAC_SetVolume(volume,volume); 
				eeprom_wb(EEPROM_VOLUME,volume);
				event = EV_IDLE;
				break;

			case EV_VOLDN: 
				if (volume > 0) volume--; 
				DAC_SetVolume(volume,volume); 
				eeprom_wb(EEPROM_VOLUME,volume);
				event = EV_IDLE;
				break;

			case EV_NEUTRAL: 
				volume = 0x2C;
				DAC_SetVolume(volume,volume); 
				eeprom_wb(EEPROM_VOLUME,volume);
				event = EV_IDLE;
				break;

			case EV_MUTE:
				mute = 1-mute;
				write_register(0xaa,(unsigned long)mute);				
				event = EV_IDLE;
				break;
		}
	
	
		// time to update a buffer ?
		if (updbuf)
		{
			if (mycluster)
			{
				cli();
				load_sectors(mycluster,updbuf);			// load a clusterfull of data
				mycluster = next_cluster(mycluster);	// follow FAT chain
				sei();
			}
			updbuf = (unsigned char *)0;			// mark as done
			scroll_flag++;
		}

		
		// 
		if (isPlaying == PS_PLAYING)
		{
			while ( bit_is_set(PINB, PB2) )			// while DEMAND is high 
			{
				//send data on SPI bus
				outp(*outptr++, SPDR);

				// buffers are updated approx every 250 mS  ( at 128 kBs datastream )

				// check for buffer wrap
				if (outptr == buffer2)
					updbuf = buffer1;
				else if (outptr == buffer3)
					updbuf = buffer2;
				else if (outptr == buffer4)
				{
					updbuf = buffer3;
					outptr = buffer1;
				}

				// check if we're done
				if (--filesize <= 0)
				{
					isPlaying = PS_STOPPED;				// the end is near 
					// try autoplay next song
					event = EV_NEXT;
					break;
				}

				// wait for data to be sent
				loop_until_bit_is_set(SPSR, SPIF);	
			}
			
			// scroll display on every buffer updates (approx 250 ms)
			if (scroll_flag == 1)
			{
				lcd_gotoxy(0,0);
				for (i=0;i<16;i++)
					lcd_putchar( scroll_line[ (scroll_pos+i) % (scroll_length+4) ] );
				scroll_pos++;
				scroll_flag = 0;
			}    
			
			// print time
			if (time_flag)
			{
				lcd_gotoxy(5,1);

				i = time_now/2;
	
				time[0] = '0' + (i / 6000);
				i %= 6000;
				time[1] = '0' + (i / 600);
				i %= 600;
				time[2] = '0' + (i / 60);
				i %= 60;
				time[4] = '0' + (i / 10);
				i %= 10;
				time[5] = '0' + i;
				
				lcd_puts(time);
				time_flag = 0;
			}
		}
		else if (isPlaying == PS_PAUSED)
		{
			if (time_flag == 1)
			{
				lcd_gotoxy(5,1);				
				lcd_puts("Paused");
			}
			else if (time_flag == 2)
			{
				lcd_gotoxy(5,1);				
				lcd_puts("      ");
				time_flag = 0;
			}
			
		}
  	}    
}



