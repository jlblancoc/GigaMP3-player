
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

#include <io.h>
#include <signal.h>
#include "i2c.h"

#include <progmem.h>
#include "uart.h"
#include "delay.h"

#define READ    0x01    // I2C READ bit

#define SCLPORT	PORTB
#define SDAPORT	PORTB

#define SCL	PB4
#define	SDA	PB3

#define QDEL	delay(5)
#define HDEL	delay(10)

#define I2C_SDL_LO      cbi( SDAPORT, SDA)
#define I2C_SDL_HI      sbi( SDAPORT, SDA)

#define I2C_SCL_LO      cbi( SCLPORT, SCL); 
#define I2C_SCL_HI      sbi( SCLPORT, SCL); 


//#define I2C_SCL_TOGGLE  HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO;
//#define I2C_START       I2C_SDL_LO; QDEL; I2C_SCL_LO; 
//#define I2C_STOP        HDEL; I2C_SCL_HI; QDEL; I2C_SDL_HI; HDEL;


#define I2CTRACE        PRINT
#define I2CTRACE2(a,b,c)	PRINT(a); UART_Printfu08(b); PRINT(c);


void i2ct(void)
{
	HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO;
}

void i2cstart(void)
{
	I2C_SDL_LO; QDEL; I2C_SCL_LO; 
}

void i2cstop(void)
{
	HDEL; I2C_SCL_HI; QDEL; I2C_SDL_HI; HDEL;
}


#define I2C_SCL_TOGGLE  i2ct();
#define I2C_START       i2cstart();
#define I2C_STOP        i2cstop();	



//int q; // for debug

UINT i2c_putbyte(BYTE b)
{
    int i;

//    I2CTRACE2("<",b,">");
	
	for (i=7;i>=0;i--)
	{
		if ( b & (1<<i) )
        	I2C_SDL_HI;
        else
            I2C_SDL_LO;   // address bit
        I2C_SCL_TOGGLE;     // clock HI, delay, then LO
    }

    I2C_SDL_HI;            // leave SDL HI

	// added    
	cbi(SDAPORT-1, SDA);	// change direction to input on SDA line (may not be needed)

	HDEL;
    I2C_SCL_HI;                     // clock back up
  	
  	b = inp(SDAPORT-2) & (1<<SDA);  // get the ACK bit

	HDEL;
    I2C_SCL_LO;	// not really ??

	sbi(SDAPORT-1, SDA);	// change direction back to output

	HDEL;

    return (b == 0);            // return ACK value
}


BYTE i2c_getbyte(UINT last)
{
    int i;
    BYTE c,b = 0;

		
    I2C_SDL_HI;            // make sure pullups are ativated

	cbi(SDAPORT-1, SDA);	// change direction to input on SDA line (may not be needed)

    for (i=7;i>=0;i--)
	{
		HDEL;
        I2C_SCL_HI;             // clock HI

	  	c = inp(SDAPORT-2) & (1<<SDA);  

        b <<= 1;
        if (c)
            b |= 1;

		HDEL;
    	I2C_SCL_LO;             // clock LO
    }

	sbi(SDAPORT-1, SDA);	// change direction to output on SDA line
  
    if (last)
	    I2C_SDL_HI;                	// set NAK
    else
	    I2C_SDL_LO;                	// set ACK

    I2C_SCL_TOGGLE;            // clock pulse

    I2C_SDL_HI;            // leave with SDL HI

//    I2CTRACE2("<",b,">");
	
    return b;            // return received byte
}





/**************************************************************************/
/* I2C public functions 												  */
/**************************************************************************/

/*
    Initialize I2C communication
*/
void i2c_init()
{

	sbi( SDAPORT-1, SDA);	// set SDA as output
	sbi( SCLPORT-1, SCL);	// set SCL as output

	I2C_SDL_HI;
	I2C_SCL_HI;
}


/*
    Send a byte sequence on the I2C line
*/
void i2c_send(BYTE dev, BYTE sub, BYTE length, BYTE *data)
{
//    I2CTRACE("i2c_send ");

    I2C_START;      		// do start transition
    i2c_putbyte(dev);   	// send DEVICE address
    i2c_putbyte(sub);   	// and the subaddress

    // send the data
    while (length--)
        i2c_putbyte(*data++);

    I2C_SDL_LO;             // clear data line and
	I2C_STOP;               // send STOP transition
//    I2CTRACE("\n");

}


/*
    Retrieve a byte sequence on the I2C line
*/
void i2c_receive(BYTE dev, BYTE sub, BYTE length, BYTE *data)
{
    int j = length;
    BYTE *p = data;

//    I2CTRACE("i2c_receive ");

    I2C_START;				// do start transition
    i2c_putbyte(dev);   	// send DEVICE address
    i2c_putbyte(sub | READ);   	// and the subaddress

	HDEL;
    I2C_SCL_HI;      		// do a repeated START
    I2C_START;          	// transition

    i2c_putbyte(dev | READ);// resend DEVICE, with READ bit set

    // receive data bytes
    while (j--)
        *p++ = i2c_getbyte(j == 0);

    I2C_SDL_LO;             // clear data line and
    I2C_STOP;               // send STOP transition

//    I2CTRACE("\n");
}

