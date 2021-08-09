

/*
  Jesper Hansen <jesperh@telia.com>
  
  Original Author: Volker Oth <volkeroth@gmx.de>

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


#ifndef __UART_H__
#define __UART_H__

/* Global definitions */
/*
typedef unsigned char  u08;
typedef          char  s08;
typedef unsigned short u16;
typedef          short s16;
typedef unsigned long  u32;
typedef          long  s32;
*/

/* UART Baud rate calculation */
#define UART_CPU               8000000      /* CPU speed */
//#define UART_CPU               11059200      /* CPU speed */
//#define UART_CPU               3686400      /* CPU speed */
#define UART_BAUD_RATE         19200        /* baud rate*/
#define UART_BAUD_SELECT       (UART_CPU/(UART_BAUD_RATE*16L)-1)

/* Global functions */
extern void UART_SendByte       (u08 Data);
extern u08  UART_ReceiveByte    (void);
extern void UART_PrintfProgStr  (u08* pBuf);
extern void UART_PrintfEndOfLine(void);
extern void UART_Printfu08      (u08 Data);
extern void UART_Printfu16      (u16 Data);
extern void UART_Printfu32      (u32 Data);
extern void UART_Init           (void);
extern unsigned char UART_HasChar(void);
extern void UART_Puts  			(u08* pBuf);
extern void UART_Putsln			(u08* pBuf);

extern void print_number(int base, int unsigned_p, long n);


/* Macros */
#define PRINT(string) (UART_PrintfProgStr(PSTR(string)))
#define EOL           UART_PrintfEndOfLine
#endif

