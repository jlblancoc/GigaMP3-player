//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOSUB.C
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
// This C source contains the low level interrupt set up and
// interrupt handler functions.
//********************************************************************

#include "ataio.h"

#include "ataiopd.h"

//*************************************************************
//
// sub_zero_return_data() -- zero the return data areas.
//
//*************************************************************

void sub_zero_return_data( void )

{

   reg_cmd_info.flg = TRC_FLAG_EMPTY;
   reg_cmd_info.ct  = TRC_TYPE_NONE;
   reg_cmd_info.cmd = 0;
   reg_cmd_info.fr1 = 0;
   reg_cmd_info.sc1 = 0;
   reg_cmd_info.sn1 = 0;
   reg_cmd_info.cl1 = 0;
   reg_cmd_info.ch1 = 0;
   reg_cmd_info.dh1 = 0;
   reg_cmd_info.dc1 = 0;
   reg_cmd_info.ec  = 0;
   reg_cmd_info.to  = 0;
   reg_cmd_info.st2 = 0;
   reg_cmd_info.as2 = 0;
   reg_cmd_info.er2 = 0;
   reg_cmd_info.sc2 = 0;
   reg_cmd_info.sn2 = 0;
   reg_cmd_info.cl2 = 0;
   reg_cmd_info.ch2 = 0;
   reg_cmd_info.dh2 = 0;
   reg_cmd_info.totalBytesXfer = 0L;
   reg_cmd_info.failbits = 0;
   reg_cmd_info.drqPackets = 0L;
}

//*************************************************************
//
// sub_trace_command() -- trace the end of a command.
//
//*************************************************************

void sub_trace_command( void )

{

   reg_cmd_info.st2 = pio_inbyte( CB_STAT );
   reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
   reg_cmd_info.er2 = pio_inbyte( CB_ERR );
   reg_cmd_info.sc2 = pio_inbyte( CB_SC );
   reg_cmd_info.sn2 = pio_inbyte( CB_SN );
   reg_cmd_info.cl2 = pio_inbyte( CB_CL );
   reg_cmd_info.ch2 = pio_inbyte( CB_CH );
   reg_cmd_info.dh2 = pio_inbyte( CB_DH );
   trc_cht();
}

//*************************************************************
//
// sub_select() - function used to select a drive.
//
// Function to select a drive. This subroutine waits for not BUSY,
// selects a drive and waits for READY and SEEK COMPLETE status.
//
//**************************************************************

int sub_select( int dev )

{
   unsigned char status;

   // PAY ATTENTION HERE
   // The caller may want to issue a command to a device that doesn't
   // exist (for example, Exec Dev Diag), so if we see this,
   // just select that device, skip all status checking and return.
   // We assume the caller knows what they are doing!

   if ( reg_config_info[ dev ] < REG_CONFIG_TYPE_ATA )
   {
      // select the device and return

      pio_outbyte( CB_DH, dev ? CB_DH_DEV1 : CB_DH_DEV0 );
      DELAY400NS;
      return 0;
   }

   // The rest of this is the normal ATA stuff for device selection
   // and we don't expect the caller to be selecting a device that
   // does not exist.
   // We don't know which drive is currently selected but we should
   // wait for it to be not BUSY.  Normally it will be not BUSY
   // unless something is very wrong!

   trc_llt( 0, 0, TRC_LLT_PNBSY );
   while ( 1 )
   {
      status = pio_inbyte( CB_STAT );
      if ( ( status & CB_STAT_BSY ) == 0 )
         break;
      if ( tmr_chk_timeout() )
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 11;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         reg_cmd_info.st2 = status;
         reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
         reg_cmd_info.er2 = pio_inbyte( CB_ERR );
         reg_cmd_info.sc2 = pio_inbyte( CB_SC );
         reg_cmd_info.sn2 = pio_inbyte( CB_SN );
         reg_cmd_info.cl2 = pio_inbyte( CB_CL );
         reg_cmd_info.ch2 = pio_inbyte( CB_CH );
         reg_cmd_info.dh2 = pio_inbyte( CB_DH );
         return 1;
      }
   }

   // Here we select the drive we really want to work with by
   // putting 0xA0 or 0xB0 in the Drive/Head register (1f6).

   pio_outbyte( CB_DH, dev ? CB_DH_DEV1 : CB_DH_DEV0 );
   DELAY400NS;

   // If the selected device is an ATA device,
   // wait for it to have READY and SEEK COMPLETE
   // status.  Normally the drive should be in this state unless
   // something is very wrong (or initial power up is still in
   // progress).  For any other type of device, just wait for
   // BSY=0 and assume the caller knows what they are doing.

   trc_llt( 0, 0, TRC_LLT_PRDY );
   while ( 1 )
   {
      status = pio_inbyte( CB_STAT );
      if ( reg_config_info[ dev ] == REG_CONFIG_TYPE_ATA )
      {
           if ( ( status & ( CB_STAT_BSY | CB_STAT_RDY | CB_STAT_SKC ) )
                     == ( CB_STAT_RDY | CB_STAT_SKC ) )
         break;
      }
      else
      {
         if ( ( status & CB_STAT_BSY ) == 0 )
            break;
      }
      if ( tmr_chk_timeout() )
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 12;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         reg_cmd_info.st2 = status;
         reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
         reg_cmd_info.er2 = pio_inbyte( CB_ERR );
         reg_cmd_info.sc2 = pio_inbyte( CB_SC );
         reg_cmd_info.sn2 = pio_inbyte( CB_SN );
         reg_cmd_info.cl2 = pio_inbyte( CB_CL );
         reg_cmd_info.ch2 = pio_inbyte( CB_CH );
         reg_cmd_info.dh2 = pio_inbyte( CB_DH );
         return 1;
      }
   }

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//*************************************************************
//
// sub_atapi_delay() - delay for at least two ticks of the bios
//                     timer (or at least 110ms).
//
//*************************************************************

void sub_atapi_delay( int dev )

{
   int ndx;
   long lw;

   if ( reg_config_info[dev] != REG_CONFIG_TYPE_ATAPI )
      return;
   if ( ! reg_atapi_delay_flag )
      return;
   trc_llt( 0, 0, TRC_LLT_DELAY );
   for ( ndx = 0; ndx < 3; ndx ++ )
   {
      lw = tmr_read_bios_timer();
      while ( lw == tmr_read_bios_timer() )
         /* do nothing */ ;
   }
}

//*************************************************************
//
// sub_xfer_delay() - delay until the bios timer ticks
//                    (from 0 to 55ms).
//
//*************************************************************

void sub_xfer_delay( void )

{
   long lw;

   trc_llt( 0, 0, TRC_LLT_DELAY2 );
   lw = tmr_read_bios_timer();
   while ( lw == tmr_read_bios_timer() )
      /* do nothing */ ;
}

// end ataiosub.c
