

/*
  Jesper Hansen <jesperh@telia.com>
  
  From code by: Volker Oth <volkeroth@gmx.de>

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


#ifndef DELAY_H
#define DELAY_H

/* constants/macros */
#define F_CPU        8000000               		/* 8MHz processor */
#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	/* cpu cycles per microsecond */

/* prototypes */
void delay(unsigned short us);

#endif
