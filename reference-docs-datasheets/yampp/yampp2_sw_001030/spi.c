
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

#include "io.h"
#include <signal.h>
#include <interrupt.h>

#include "spi.h"
#include "delay.h"


void init_spi()
{
	unsigned char dummy;

	// setup SPI I/O pins
	sbi(PORTB, 7);	// set SCK hi
	sbi(DDRB, 7);	// set SCK as output

	sbi(DDRB, 5);	// set MOSI a output
	sbi(DDRB, 4);	// SS must be output for Master mode to work
	
	// setup SPI interface :
	// clock = f/4
	// select clock phase negative going in middle of data
	// master mode
	// enable SPI
	outp((1<<CPHA)|(1<<MSTR)|(1<<SPE)/* |(1<<SPR0)*/ , SPCR );
	
	dummy = inp(SPSR);	// clear status

}
