
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


#ifndef __TYPES_H__
#define __TYPES_H__


#define MAX_U16  65535
#define MAX_S16  32767

#define DDR(x) ((x)-1)    /* address of data direction register of port x */
#define PIN(x) ((x)-2)    /* address of input register of port x */


typedef unsigned char  BYTE;
typedef unsigned int   WORD;
typedef unsigned long  DWORD;

typedef unsigned short  USHORT;
typedef unsigned char   BOOL;

typedef unsigned char  UCHAR;
typedef unsigned int   UINT;
typedef unsigned long  ULONG;

typedef char  CHAR;
typedef int   INT;
typedef long  LONG;

// alternative types

typedef unsigned char  u08;
typedef unsigned short u16;
typedef unsigned long  u32;

typedef char  s08;
typedef short s16;
typedef long  s32;


/*
typedef char  S8;
typedef short S16;
typedef long  S32;
*/

//#define NULL    0

#endif
