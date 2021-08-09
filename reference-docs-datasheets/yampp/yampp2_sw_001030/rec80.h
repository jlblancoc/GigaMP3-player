
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


#ifndef _REC80_H
#define _REC80_h


#define IR_PORT		PORTD
#define IR_BIT		PD2

#define rec80_active	bit_is_clear( IR_PORT-2, IR_BIT )

void init_rec80(void);
unsigned int get_rec80(void);


// REC-80 codes

#define IR_PLAY		0x4e50
#define IR_STOP		0x1e00
#define IR_PAUSE	0x7e60
#define IR_PREV		0x5e40
#define IR_NEXT		0xdec0
#define IR_VOLUP	0x5846	// zoom_T	
#define IR_VOLDN	0xd8c6	// zoom_W
#define IR_RESET	0x342A

#endif
