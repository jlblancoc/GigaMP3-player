
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

#include "types.h"
#include "delay.h"
#include "uart.h"

#include "lcd.h"


static u08 lcd_x, lcd_y;


/*************************************************************/
/********************** LOCAL FUNCTIONS **********************/
/*************************************************************/


u08 LCDSetAddress(u08 adr) 
{ 
	return *(u08 *) (0xE000+adr);		
	// dummy return to avoid optimization problems
}



static void lcd_write(u08 data,u08 rs) 
{
	// setup RS and RW pins
    if (rs) 
    	LCDSetAddress(LCD_IO_DATA + LCD_IO_WRITE);
    else    
    	LCDSetAddress(LCD_IO_FUNCTION + LCD_IO_WRITE);

	cbi(MCUCR, SRE);				// disable EXTRAM
	outp(0xff, DDR(LCD_DATA_PORT));	// set port as output
	outp(data, LCD_DATA_PORT);		// write byte
	lcd_e_high();					// set LCD enable high
	lcd_e_low();					// set LCD enable low
	
	sbi(MCUCR, SRE);	// enable RAM
}



static u08 lcd_read(u08 rs) 
{
    register u08 data;

	// setup RS and RW pins
    if (rs) 
    	LCDSetAddress(LCD_IO_DATA + LCD_IO_READ);
    else    
    	LCDSetAddress(LCD_IO_FUNCTION + LCD_IO_READ);

	cbi(MCUCR, SRE);				// disable EXTRAM
	outp(0x00, DDR(LCD_DATA_PORT));	// set port as input
	lcd_e_high();					// set LCD enable high
	data = inp(PIN(LCD_DATA_PORT));	// read byte
	lcd_e_low();					// set LCD enable low
	sbi(MCUCR, SRE);				// enable EXTRAM

    return data;
}


static void lcd_waitbusy(void)
/* loops while lcd is busy */
{
    while (lcd_read(0) & (1<<LCD_BUSY)) {}
}


static void lcd_newline(void)
/* goto start of next line */
{
    lcd_x = 0;
    if (++lcd_y >= LCD_LINES)
        lcd_y = 0;
}


static void lcd_goto(void)
/* goto position (lcd_x,lcd_y) */
{
    lcd_command((1<<LCD_DDRAM)+LCD_LINE_LENGTH*lcd_y+lcd_x);
}


/*************************************************************/
/********************* PUBLIC FUNCTIONS **********************/
/*************************************************************/

void lcd_command(u08 cmd)
/* send commando <cmd> to LCD */
{
    lcd_waitbusy();
    lcd_write(cmd, 0);
}

void lcd_data(u08 data)
/* send data <data> to LCD */
{
    lcd_waitbusy();
    lcd_write(data, 1);
}


void lcd_gotoxy(u08 x, u08 y)
/* goto position (x,y) */
{
    lcd_x = x; lcd_y = y;
    lcd_goto();
}


void lcd_clrscr(void)
/* clear lcd */
{
    lcd_x = lcd_y = 0;
    lcd_command(1<<LCD_CLR);
    delay(2000);
}

void lcd_home(void)
/* set cursor to home position */
{
    lcd_x = lcd_y = 0;
    lcd_command(1<<LCD_HOME);
    delay(2000);
}

void lcd_putchar(u08 data)
/* print character to current cursor position */
{
    lcd_waitbusy();
    if (data=='\n') {
        lcd_newline();
        lcd_goto();
    }
    else {
        if (++lcd_x >= LCD_LINE_LENGTH) {
            lcd_newline();
            lcd_goto();
        }
        lcd_write(data, 1);
    }
}


void lcd_puts(char s[])
/* print string on lcd (no auto linefeed) */
{
    register u08 *c;

    if (!(c=s)) return;

    while (*c) {
        lcd_putchar(*c);    
        c++;
    }
}

void lcd_init(u08 cursor, u08 fnc)
/* cursor:   0 = off, 2 = on, 3 = blinking */
/* fnc: see LCD_FUNCTION_xxx */
{
    u08 wait[] = { 250, 78, 1, 1 };
    register u08 i;

    /* configure control line pins as output */
    sbi(DDR(LCD_E_PORT),  LCD_E_PIN);

    /* set enable line high */
    sbi(LCD_E_PORT, LCD_E_PIN);

    /* enable external SRAM (memory mapped lcd) and wait states */
    outp((1<<SRE)|(1<<SRW), MCUCR);


    fnc |= (1<<LCD_FUNCTION);
    /* reset lcd */
    for (i=0; i<4; i++) {
        delay(wait[i]<<6);                     /* 16ms, 5ms, 64us, 64us */
        lcd_write(fnc, 0);                     /* reset function */    
    }

    lcd_command(1<<LCD_ON);
    lcd_clrscr();
    
    lcd_command(LCD_MODE_DEFAULT);
    lcd_command((1<<LCD_ON)|(1<<LCD_ON_DISPLAY)|cursor);
} 


                             
