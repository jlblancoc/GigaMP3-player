

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


#ifndef LCD_H
#define LCD_H


/* change these definitions to adapt setting */

#define LCD_LINES           2     /* visible lines */
#define LCD_LINE_LENGTH  0x40     /* internal line length */

/* you shouldn't need to change anything below this line */

#define LCD_CLR             0      /* DB0: clear display */
#define LCD_HOME            1      /* DB1: return to home position */
#define LCD_ENTRY_MODE      2      /* DB2: set entry mode */
#define LCD_ENTRY_INC       1      /*   DB1: increment ? */
#define LCD_ENTRY_SHIFT     0      /*   DB2: shift ? */
#define LCD_ON              3      /* DB3: turn lcd/cursor on */
#define LCD_ON_DISPLAY      2      /*   DB2: turn display on */
#define LCD_ON_CURSOR       1      /*   DB1: turn cursor on */
#define LCD_ON_BLINK        0      /*     DB0: blinking cursor ? */
#define LCD_MOVE            4      /* DB4: move cursor/display */
#define LCD_MOVE_DISP       3      /*   DB3: move display (0-> cursor) ? */
#define LCD_MOVE_RIGHT      2      /*   DB2: move right (0-> left) ? */
#define LCD_FUNCTION        5      /* DB5: function set */
#define LCD_FUNCTION_8BIT   4      /*   DB4: set 8BIT mode (0->4BIT mode) */
#define LCD_FUNCTION_2LINES 3      /*   DB3: two lines (0->one line) */
#define LCD_FUNCTION_10DOTS 2      /*   DB2: 5x10 font (0->5x7 font) */
#define LCD_CGRAM           6      /* DB6: set CG RAM address */
#define LCD_DDRAM           7      /* DB7: set DD RAM address */

#define LCD_BUSY            7      /* DB7: LCD is busy */


#define LCD_IO_DATA         0x0001	// A0 goes to RS
#define LCD_IO_FUNCTION     0x0000

#define LCD_IO_READ			0x0002	// A1 goes to R/-W
#define LCD_IO_WRITE		0x0000


#define LCD_PORT_MASK 0xff
#define LCD_FDEF_1    (1<<LCD_FUNCTION_8BIT)

#define LCD_FDEF_2    (1<<LCD_FUNCTION_2LINES)

#define LCD_FUNCTION_DEFAULT ((1<<LCD_FUNCTION) | LCD_FDEF_1 | LCD_FDEF_2)

#define LCD_MODE_DEFAULT     ((1<<LCD_ENTRY_MODE) | (1<<LCD_ENTRY_INC))

#define LCD_DATA_PORT	PORTA

#define LCD_E_PORT		PORTD
#define LCD_E_PIN		PD5

#define lcd_e_high()    sbi(LCD_E_PORT, LCD_E_PIN); asm volatile ("nop"); asm volatile ("nop");
#define lcd_e_low()     cbi(LCD_E_PORT, LCD_E_PIN); 


/* prototypes */

void lcd_command(u08 cmd);
void lcd_data(u08 cmd);
void lcd_gotoxy(u08 x, u08 y);
void lcd_clrscr(void);
void lcd_home(void);
void lcd_putchar(u08 data);
void lcd_init(u08 cursor, u08 fnc);
void lcd_puts(char s[]);


#endif
