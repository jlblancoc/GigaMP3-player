
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
#include "mem.h"

#define DEV_MAS     0x3A

#define MAS_DATA   	0x68    // WRITE (68) and READ (69)
#define MAS_CTRL	0x6A    // WRITE ONLY (6A)


#define MAS_CMD_RUN 	0x00    // Run Base
#define MAS_CMD_RUN1 	0x10    // Alternative Run Base

#define MAS_CMD_CID 	0x30    // Read Control Interface Base
#define MAS_CMD_WRREG	0x90    // Write Register Base
#define MAS_CMD_WRD0	0xA0    // Write D0 Memory Base
#define MAS_CMD_WRD1	0xB0    // Write D1 Memory Base
#define MAS_CMD_RDREG	0xD0    // Read Register Base
#define MAS_CMD_RDD0	0xE0    // Read D0 Memory Base
#define MAS_CMD_RDD1	0xF0    // Read D1 Memory Base



// registers

#define MAS_DCCF    	0x8e    // DC/DC converter mode
#define MAS_MUTE    	0xaa    // MUTE/BYPASS tone control
#define MAS_PIO     	0xc8    // PIO pin data
#define MAS_PIO_F10 	0xed    // F10 specific PIO readback
#define MAS_START_CFG   0xe6    // Startup Config
#define MAS_KPRESCALE   0xe7    // Tone filter Prescaler
#define MAS_KBASS       0x6b    // Bass filter
#define MAS_KTREBLE     0x6f    // Treble filter
#define MAS_LTRAILING   0xc5    // Trailing bits on SDO, Left Channel
#define MAS_RTRAILING   0xc6    // Trailing bits on SDO, Right Channel


/* DDCF - 0x8e
bit 16-14   PUPLIMIT    DC/DC converter output  (default = 2)
bit 13-10   DCFR        Clock Frequency of the converter (default = 0)
bit  9-0    not used
*/

/* MUTE - 0xaa
bit 16-2 	not used
bit 1  	BYPASS			1 = Bypass Bass/Treble/Volume Matrix, 0 = normal
bit 0   MUTE      		1 = Mute output, 0  = no mute
*/

/* KPRESCALE - 0xe7
   KBASS     - 0x6b
   KTREBLE   - 0x6f
table controlled, see doc.
*/

/* StartupConfig    - 0xe6
bit
 8          Clock Divider, 0 = 1,2 or 4 as to MPEG 2/2.5, 1 = fixed
 5  (F10)   0 = SW download disabled, 1 = SW Download enabled
 4          clock input (D8) / switch to parallel mode (F10)
 3          0 = Enable Layer 3, 1 = Disable Layer 3
 2          0 = Enable Layer 2, 1 = Disable Layer 2
 1          SDO output: 0 = 32 bit, 1 = 16 bit
 0          0 = Multimedia mode, 1 = Broadcast mode
*/

/*
    LEFT & RIGHT TRAILING
bits 0..11 should be set to 0 at startup

*/



/* readonly status information

D0:300  	MPEGFrameCount
D0:301      MPEGStatus1
            bits
            19-15   not used
            14-13   MPEG HEADER bits 11,12
                    00  MPEG 2.5
                    01  reserved
                    10  MPEG 2
                    11  MPEG 1 (not valid in F10)
            12-11   MPEG Layer
                    00  reserved
                    01  Layer 3
                    10  Layer 2
                    11  Layer 1
            10      no CRC protection
            9-2     private
            1       CRC Error
            0       Invalid Frame
D0:302      MPEGStatus2
            bits
            19-16   not used
            15-12   MPEG Bit Rate
                    0000 free
                    .
                    1111 forbidden
            11-10   Sample Frequency (MPEG 1 Rate, MPEG2 and 2.5 rates are 0.5 and 0.25 times these)
                    00  44.1 kHz
                    01  48 kHz
                    10  32 kHz
                    11  reserved
            9       Padding bit
            8       Private
            7-6     Mode
                    00 Stereo
                    01 Joint Stereo
                    10 Dual Channel
                    11 Single Channel
            5-4     Mode Extension (Joint Stereo only)
            		00  Intensity Stereo OFF, MS Stereo OFF
            		01  Intensity Stereo ON,  MS Stereo OFF
            		10  Intensity Stereo OFF, MS Stereo ON
            		11  Intensity Stereo ON,  MS Stereo ON
            3       copyright protected
            2       -copy/original
            1-0     Emphasis
                    00  None
                    01  10/15 uS
                    10  reserved
                    11  CCITT J.17
D0:303      CRCErrorCount
D0:304      NumberOfAncillaryBits
D0:305-321  AncillaryData


*/



/*
    Configuration memory

D0:32d      PPLLOffset48    ( for 14.31818 MHz, 0.7314   ( 0x5D9E8 ) )
D0:32e      PPLLOffset44    ( for 14.31818 MHz, -0.3843  ( 0xCECF4 ) )
D0:32f      OutputConfig
            bits
            19-12   dont care
            11      0 = no delay, 1 = additional delay
            10-6    dont care
            5       invert outgoing word strobe
            4       0 = 32 bits per sample, 1 = 16 bits per sample
            3-0     dont care
*/

/*
    Volume Matrix

D1:7f8      LL  Left->Left Gain (default 0dB = 80000)
D1:7f9      LR  Left->Right Gain (default 0)
D1:7fa      RL  Right->Left Gain (default 0)
D1:7fb      RR  Right->Right Gain (default 0dB = 80000)

*/

/*
	Version Info

D1:ff6      Name of MAS version
D1:ff7      HW/SW Design code
D1:ff8      Date of tape
D1:ff9..fff Description

*/


BYTE *masdata;

void write_D0D1_mem(BYTE memtype, UINT address, UINT nWords, ULONG *data )
{
    BYTE *p = masdata;
    int i;

    *p++ = memtype;
    *p++ = 0;
    // set count
    *p++ = nWords >> 8;
    *p++ = nWords;
    // set address
    *p++ = address >> 8;
    *p++ = address;
    // set data
    for (i=0;i<nWords;i++)
    {
    	*p++ = *data >> 8;
    	*p++ = *data;
    	*p++ = 0;
    	*p++ = *data >> 16;
        data++;
    }
    i2c_send(DEV_MAS, MAS_DATA, nWords*4+6, masdata );
}

void write_register(BYTE reg, ULONG data)
{

    masdata[0] = MAS_CMD_WRREG + (reg >> 4);    	// $9, r1
    masdata[1] = (reg << 4) + (data & 15);  		// r0, d0
    masdata[2] = data >> 12;						// d4, d3
    masdata[3] = data >> 4;  				   		// d2, d1
	
    i2c_send(DEV_MAS, MAS_DATA, 4, masdata);
}


void read_D0D1_mem(BYTE memtype, UINT address, UINT nWords, ULONG *data )
{
    BYTE *p = masdata;
    int i;

	*p++ = memtype;
    *p++ = 0;
    // set count
    *p++ = nWords >> 8;
    *p++ = nWords;
    // set address
    *p++ = address >> 8;
    *p++ = address;
    // send data
    i2c_send(DEV_MAS, MAS_DATA, 6, masdata );
    
    delay(1000);	// 1 mS
    
    i2c_receive(DEV_MAS, MAS_DATA, nWords*4, masdata);

	nWords <<= 2;
    for (i=0;i<nWords;i+=4)
    {
    	*data = (ULONG)masdata[i] << 8;
    	*data |= masdata[1+i];
    	*data |= (ULONG)masdata[3+i] << 16;
        data++;
    }

}


ULONG read_register(BYTE reg)
{
    ULONG u,a;

    masdata[0] = MAS_CMD_RDREG + (reg >> 4);    	// $D, r1
    masdata[1] = ((reg & 15) << 4);  				// r0, $0
    i2c_send(DEV_MAS, MAS_DATA, 2, masdata);

    delay(100);

    i2c_receive(DEV_MAS, MAS_DATA, 4, (BYTE*)&u);
    a = (u & 0x0f) << 16;
    a |= u >> 16;

    return a;
}





void MAS_init(void)
{
    ULONG uldata[4];

	masdata = (BYTE *)MAS_BUFFER;

    // do a SW reset
    i2c_send(DEV_MAS, MAS_CTRL, 2, "\x01\x00");
    delay(1000);   // wait some time
    i2c_send(DEV_MAS, MAS_CTRL, 2, "\x00\x00");
    delay(10000);   // wait some time

// new G10 init here
    write_register(0x3B, 0x0020);   //	new G10 serial interface startup
    i2c_send(DEV_MAS, MAS_DATA, 2, "\x00\x01");   // RUN 1

    delay(10);

    write_register(MAS_START_CFG, 0x00002);   //
	i2c_send(DEV_MAS, MAS_DATA, 2, "\x0f\xcd");   // activate configuration TODO !! add CMD_RUN
    delay(10);

    // Clear Left & Right Trailing bits
    write_register(MAS_LTRAILING, 0);   // Left
    write_register(MAS_RTRAILING, 0);   // Right

  
    // set offsets
    uldata[0] = 0x5D9E8;    // PLLOffset48
    uldata[1] = 0xCECF4;    // PLLOffset44
    uldata[2] = 0x00800;    // OutputConfig 16 bits per sample

//    write_D0D1_mem(MAS_CMD_WRD0, 0x32d, 0x03, uldata); // F10 version
    write_D0D1_mem(MAS_CMD_WRD0, 0x36d, 0x03, uldata);	// new offset for G10

    // activate changes
    i2c_send(DEV_MAS, MAS_DATA, 2, "\x04\x75");   // run 475 (F10) to validate offsets
}


// return the frame count
//
/*
USHORT default_read(void)
{
    USHORT u,a;
    i2c_receive(DEV_MAS, MAS_DATA, 2, (BYTE*)&u);
    a = u << 8;
    a |= u >> 8;
    return a;
}
*/