
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

#include "delay.h"



// delay for a minimum of <us> microseconds 
// the time resolution is dependent on the time the loop takes 
// e.g. with 4Mhz and 5 cycles per loop, the resolution is 1.25 us 

void delay(unsigned short us) 
{
    unsigned short  delay_loops;
    register unsigned short  i;

    delay_loops = (us+3)/5*CYCLES_PER_US; // +3 for rounding up (dirty) 

	// one loop takes 5 cpu cycles 
    for (i=0; i < delay_loops; i++) {};
} 
