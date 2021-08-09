
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
#include <timer.h>

#include "types.h"
#include "rec80.h"

/*
	The REC-80 format used by Panasonic is a space coded 48 bit code consisting 
	of a 32 bit group id, followed by a 16 bit commmand word.
	Leading this is a header consisting of a 10T signal and a 4T pause.
	All bits start with a pulse of length T. The length of the pause following
	indicates the bit value. A T pause for a 0-bit and a 3T pause for a 1-bit.


*/



void init_rec80()
{
	cbi(IR_PORT-1, IR_BIT); 	// PD2 input for RC5
	cbi(IR_PORT,   IR_BIT); 	// no pullups
}

u16 get_rec80(void)
{
	u08 i, tmp;
	u08 time;
	u08 T2,T4;
	u08 cl,ch;

	tmp = 0;
	cl = ch = 0;	// only needed to get rid of compiler warning

	loop_until_bit_is_set  (IR_PORT-2, IR_BIT);		// skip leading signal


	timer0_stop();
	timer0_source(CK256);	//update every 32us
	timer0_start();

	while (bit_is_set(IR_PORT-2, IR_BIT))
	{
		T2 = inp(TCNT0);
		if (T2 >= 100)	// max wait time
			return 0;
	}
	
//	loop_until_bit_is_clear(IR_PORT-2, IR_BIT);	    // skip leading space


	// measure time T

	timer0_stop();
	timer0_source(CK256);	//update every 32us
	timer0_start();

	loop_until_bit_is_set(IR_PORT-2, IR_BIT);
	
	T2 = inp(TCNT0);		// T is normally around 0E-10 hex = 15 -> 480 uS
	timer0_stop();

	T2 = T2 * 2;
	// max time is 4T
	T4 = T2 * 2;		

	for (i=0; i<48; i++)
	{
		timer0_source(CK256);
		timer0_start();
			
		while(1)
		{
			time=inp(TCNT0);
		
			if (time > T4)
				return 0;
			
			// measure time on the lo flank
			if (bit_is_clear(IR_PORT-2, IR_BIT))
			{
				tmp <<= 1;
				if (time >= T2)
					tmp += 1;
				break;
			}
		}
		timer0_stop();

		// save command data as we go
		if( i == 39)
			cl = tmp;
		if( i == 47)
			ch = tmp;
		
		// syncronize - wait for next hi flank
		loop_until_bit_is_set(IR_PORT-2, IR_BIT);
	}
	
	return (u16) (ch<<8) + cl;
}
