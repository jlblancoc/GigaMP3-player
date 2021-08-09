
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


#include "i2c.h"
#include "delay.h"

#define DEV_DAC     0x9A
#define SR_REG      0xC1
#define AVOL      	0xC2
#define GCFG      	0xC3



void DAC_init(void)
{
    // set SR_REG

    i2c_send(DEV_DAC, SR_REG, 1, "\x0f");   // maybe 0x07 for auto samplerate control??
    delay(5);
    i2c_send(DEV_DAC, AVOL, 2, "\x2C\x2C"); // volumes at 0 dB
    delay(5);
    i2c_send(DEV_DAC, GCFG, 1, "\x04"); 	// 3 V operation, DAC on
}

void DAC_SetVolume(BYTE l, BYTE r)
{
    BYTE b[2];
    b[0] = l;
    b[1] = r;
    i2c_send(DEV_DAC, AVOL, 2, b); 	// set volumes
}


