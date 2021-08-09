//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOTMR.C
//
// by Hale Landis (hlandis@ibm.net)
//
// There is no copyright and there are no restrictions on the use
// of this ATA Low Level I/O Driver code.  It is distributed to
// help other programmers understand how the ATA device interface
// works and it is distributed without any warranty.  Use this
// code at your own risk.
//
// This code is based on the ATA-2, ATA-3 and ATA-4 standards and
// on interviews with various ATA controller and drive designers.
//
// This code has been run on many ATA (IDE) drives and
// MFM/RLL controllers.  This code may be a little
// more picky about the status it sees at various times.  A real
// BIOS probably would not check the status as carefully.
//
// Compile with one of the Borland C or C++ compilers.
//
// This C source file contains functions to access the BIOS
// Time of Day information and to set and check the command
// time out period.
//********************************************************************

#include <dos.h>

#include "ataio.h"

#include "ataiopd.h"

//**************************************************************

long tmr_time_out = 20L;      // max command execution time in seconds

//**************************************************************

static long timeOut;          // command timeout time - see the
                              // tmr_set_timeout() and
                              // tmr_chk_timeout() functions.

//*************************************************************
//
// tmr_read_bios_timer() - function to read the BIOS timer
//
//**************************************************************

long tmr_read_bios_timer( void )

{
   long curTime;

   // Pointer to the low order word
   // of the BIOS time of day counter at
   // location 40:6C in the BIOS data area.

   static volatile long far * todPtr = MK_FP( 0x40, 0x6c );

   // loop so we get a valid value without
   // turning interrupts off and on again

   do
   {
      curTime = * todPtr;
   } while ( curTime != * todPtr );

   return curTime;
}

//*************************************************************
//
// tmr_set_timeout() - function used to set command timeout time
//
// The command time out time is computed as follows:
//
//    timer + ( tmr_time_out * 18 )
//
//**************************************************************

void tmr_set_timeout( void )

{

   // get value of BIOS timer

   timeOut = tmr_read_bios_timer();

   // add command timeout value

   timeOut = timeOut + ( tmr_time_out * 18L );

   // adjust timeout value if we are about to pass midnight

   if ( timeOut >= 1573040L )
      timeOut = timeOut - 1573040L;

   // ignore the lower 4 bits

   timeOut = timeOut & 0xfffffff0;
}

//*************************************************************
//
// tmr_chk_timeout() - function used to check for command timeout.
//
// Gives non-zero return if command has timed out.
//
//**************************************************************

int tmr_chk_timeout( void )

{
   long curTime;

   // get current time

   curTime = tmr_read_bios_timer();

   // ignore lower 4 bits

   curTime = curTime & 0xfffffff0;

   // timed out yet ?

   if ( curTime == timeOut )
      return 1;

   return 0;
}

// end ataiotmr.c
