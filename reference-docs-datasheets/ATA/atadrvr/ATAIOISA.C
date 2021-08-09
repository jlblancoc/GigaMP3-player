//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOISA.C
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
// This C source contains the main body of the driver code:
// Determine what devices are present, issue ATA Soft Reset,
// execute Non-Data, PIO data in and PIO data out commands.
//
// Note: ATA-4 has added a few timing requirements for the
// host software that were not previously in ATA-2 or ATA-3.
// The new requirements are not implemented in this code
// and are probably never required for anything other than
// ATAPI devices.
//********************************************************************

#include <dos.h>

#include "ataio.h"

#include "ataiopd.h"

//***********************************************************
//
// isa bus dma channel configuration stuff,
// see dma_isa_config().
//
//***********************************************************

static int dmaChan = 0;          // dma channel number (5, 6 or 7)

static int dmaPageReg;           // page reg addr
static int dmaAddrReg;           // addr reg addr
static int dmaCntrReg;           // cntr reg addr

static int dmaChanSel;           // channel selection bits...
                                 // also see modeByte below

#define DMA_SEL5 0x01            // values used in dmaChanSel
#define DMA_SEL6 0x02
#define DMA_SEL7 0x03

static int dmaTCbit;             // terminal count bit status

#define DMA_TC5 0x02             // values used in dmaTCbit
#define DMA_TC6 0x04
#define DMA_TC7 0x08

//***********************************************************
//
// isa bus dma channel configuration and control macros
//
//***********************************************************

#define DMA_MASK_ENABLE  0x00    // bits for enable/disable
#define DMA_MASK_DISABLE 0x04

#define enableChan()  outportb( 0xd4, DMA_MASK_ENABLE  | dmaChanSel )
#define disableChan() outportb( 0xd4, DMA_MASK_DISABLE | dmaChanSel )

#define clearFF() outportb( 0xd8, 0 )  // macro to reset flip-flop
                                       // so we access the low byte
                                       // of the address and word
                                       // count registers

//***********************************************************
//
// dma channel programming stuff
//
//***********************************************************

int doTwo;              // transfer crosses a physical boundary if != 0

unsigned int page1;     // upper part of physical memory address
                        // for 1st (or only) transfer
unsigned int page2;     // upper part of physical memory address
                        // for 2nd transfer

unsigned long addr1;    // physical address for 1st (or only) transfer
unsigned long addr2;    // physical address for 2nd transfer

unsigned long count1;   // byte/word count for 1st (or only) transfer
unsigned long count2;   // byte/word count for 2nd transfer

int modeByte;           // mode byte for the dma channel...
                        // also see dmaChanSel above

#define DMA_MODE_DEMAND 0x00     // modeByte bits for various dma modes
#define DMA_MODE_BLOCK  0x80
#define DMA_MODE_SINGLE 0x40

#define DMA_MODE_MEMR 0x08       // modeByte memory read or write
#define DMA_MODE_MEMW 0x04

//***********************************************************
//
// set_up_xfer() -- set up for 1 or 2 dma transfers -- either
//                  1 or 2 transfers are required per ata
//                  or atapi command.
//
//***********************************************************

static void set_up_xfer( int dir, long bc, unsigned seg, unsigned off );

static void set_up_xfer( int dir, long bc, unsigned seg, unsigned off )

{
   unsigned long count;    // total byte/word count
   unsigned long addr;     // absolute memory address

   // determine number of bytes to be transferred

   count = bc;

   // convert transfer address from seg:off to a 20-bit absolute memory address

   addr = (unsigned long) seg;
   addr = addr << 4;
   addr = addr + (unsigned long) off;

   // determine first transfer address,
   // determine first and second transfer counts.
   // The absolute address bits a19 - a0 are used as follows:
   // bits 7-4 of the page register are set to 0.
   // a19 - a17 are placed into the page register bits 3-1.
   // bit 0 of the page register is set to 0.
   // a16-a1 are placed into the address register bits 15-0.
   // a0 is discarded.
   // assume that only one transfer is needed and determine the
   // page, addr1, count1 and count2 values.
   // the transfer count is converted from byte to word counts.

   page1 = (int) ( ( addr & 0x000e0000L ) >> 16 );
   page2 = page1 + 2;
   addr1 = ( addr & 0x0001fffeL ) >> 1;
   addr2 = 0;
   count1 = count >> 1;
   count2 = 0;

   // if a dma boundary will be crossed, determine the
   // first and second transfer counts.  again the
   // transfer counts must be converted from byte to word counts.

   doTwo = 0;
   if ( ( ( addr + count - 1L ) & 0x000e0000L ) != ( addr & 0x000e0000L ) )
   {
      doTwo = 1;

      // determine first and second transfer counts

      count1 = ( ( addr & 0x000e0000L ) + 0x00020000L ) - addr;
      count2 = count - count1;

      // convert counts to word values

      count1 = count1 >> 1;
      count2 = count2 >> 1;
   }

   // get dma channel mode

   modeByte = DMA_MODE_DEMAND;      // change this line for single word dma
   modeByte = modeByte | ( dir ? DMA_MODE_MEMR : DMA_MODE_MEMW );
   modeByte = modeByte | dmaChanSel;
}

//***********************************************************
//
// prog_dma_chan() -- config the dma channel we will use --
//                    we call this function to set each
//                    part of a dma transfer.
//
//***********************************************************

static void prog_dma_chan( unsigned int page, unsigned long addr,
                           unsigned long count, int mode );

static void prog_dma_chan( unsigned int page, unsigned long addr,
                           unsigned long count, int mode )

{

   // disable interrupts

   disable();

   // disable the dma channel

   trc_llt( 0, 0, TRC_LLT_DMA3 );
   disableChan();

   // reset channel status (required by some systems)

   inportb( 0xd0 );

   // set dma channel transfer address

   clearFF();
   outportb( dmaAddrReg, (int) ( addr & 0x000000ffL ) );
   addr = addr >> 8;
   outportb( dmaAddrReg, (int) ( addr & 0x000000ffL ) );

   // set dma channel page

   outportb( dmaPageReg, page );

   // set dma channel word count

   count -- ;
   clearFF();
   outportb( dmaCntrReg, (int) ( count & 0x000000ffL ) );
   count = count >> 8;
   outportb( dmaCntrReg, (int) ( count & 0x000000ffL ) );

   // set dma channel mode

   outportb( 0xd6, mode );

   #if 0
      trc_llt( 0, mode, TRC_LLT_DEBUG );  // for debugging
   #endif

   // enable the dma channel

   trc_llt( 0, 0, TRC_LLT_DMA1 );
   enableChan();

   // enable interrupts

   enable();
}

//***********************************************************
//
// chk_cmd_done() -- check for command completion
//
//***********************************************************

static int chk_cmd_done( void );

static int chk_cmd_done( void )

{
   int doneStatus;

   // check for interrupt or poll status

   if ( int_use_intr_flag )
   {
      if ( int_intr_flag )                    // check for INT 76
      {
         doneStatus = pio_inbyte( CB_ASTAT ); // read status
         return 1;                            // cmd done
      }
   }
   else
   {
      doneStatus = pio_inbyte( CB_ASTAT );    // poll for not busy & DRQ/errors
      if ( ( doneStatus & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == 0 )
         return 1;                  // cmd done
      if ( ( doneStatus & ( CB_STAT_BSY | CB_STAT_DF ) ) == CB_STAT_DF )
         return 1;                  // cmd done
      if ( ( doneStatus & ( CB_STAT_BSY | CB_STAT_ERR ) ) == CB_STAT_ERR )
         return 1;                  // cmd done
   }
   return 0;                        // not done yet
}

//***********************************************************
//
// dma_isa_config() - configure/setup for Read/Write DMA
//
// The caller must call this function before attempting
// to use any ATA or ATAPI commands in ISA DMA mode.
//
//***********************************************************

int dma_isa_config( int chan )

{

   // channel must be 0 (disable) or 5, 6 or 7.

   switch ( chan )
   {
      case 0:
         dmaChan = 0;            // disable ISA DMA
         return 0;
      case 5:                    // set up channel 5
         dmaChan = 5;
         dmaPageReg = 0x8b;
         dmaAddrReg = 0xc4;
         dmaCntrReg = 0xc6;
         dmaChanSel = DMA_SEL5;
         dmaTCbit   = DMA_TC5;
         break;
      case 6:                    // set up channel 6
         dmaChan = 6;
         dmaPageReg = 0x89;
         dmaAddrReg = 0xc8;
         dmaCntrReg = 0xca;
         dmaChanSel = DMA_SEL6;
         dmaTCbit   = DMA_TC6;
         break;
      case 7:                    // set up channel 7
         dmaChan = 7;
         dmaPageReg = 0x8a;
         dmaAddrReg = 0xcc;
         dmaCntrReg = 0xce;
         dmaChanSel = DMA_SEL7;
         dmaTCbit   = DMA_TC7;
         break;
      default:                   // not channel 5, 6 or 7
         dmaChan = 0;               // disable ISA DMA
         return 1;                  // return error
   }

   return 0;
}

//***********************************************************
//
// dma_isa_ata_lba() - DMA in ISA Multiword for ATA R/W DMA
//
//***********************************************************

int dma_isa_ata_lba( int dev, int cmd,
                            int fr, int sc,
                            long lba,
                            unsigned seg, unsigned off )

{
   unsigned int cyl;
   int head, sect;

   sect = (int) ( lba & 0x000000ffL );
   lba = lba >> 8;
   cyl = (int) ( lba & 0x0000ffffL );
   lba = lba >> 16;
   head = ( (int) ( lba & 0x0fL ) ) | 0x40;

   return dma_isa_ata( dev, cmd,
                       fr, sc,
                       cyl, head, sect,
                       seg, off );
}

//***********************************************************
//
// dma_isa_ata() - DMA in ISA Multiword for ATA R/W DMA
//
//***********************************************************

int dma_isa_ata( int dev, int cmd,
                 int fr, int sc,
                 unsigned int cyl, int head, int sect,
                 unsigned seg, unsigned off )

{
   unsigned char devHead;
   unsigned char devCtrl;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char status;
   int ns;
   unsigned long lw1, lw2;

   // mark start of a R/W DMA command in low level trace

   trc_llt( 0, 0, TRC_LLT_S_RWD );

   // setup register values

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   devHead = devHead | ( head & 0x4f );
   cylLow = cyl & 0x00ff;
   cylHigh = ( cyl & 0xff00 ) >> 8;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATA;
   reg_cmd_info.ct  = ( cmd == CMD_WRITE_DMA )
                      ? TRC_TYPE_ADMAO : TRC_TYPE_ADMAI;
   reg_cmd_info.cmd = cmd;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = sc;
   reg_cmd_info.sn1 = sect;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Quit now if the command is incorrect.

   if ( ( cmd != CMD_READ_DMA ) && ( cmd != CMD_WRITE_DMA ) )
   {
      reg_cmd_info.ec = 77;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_RWD );
      return 1;
   }

   // Quit now if no dma channel set up.

   if ( ! dmaChan )
   {
      reg_cmd_info.ec = 70;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_RWD );
      return 1;
   }

   // set up the dma transfer(s)

   ns = sc;
   if ( ! ns )
      ns = 256;
   set_up_xfer( cmd == CMD_WRITE_DMA, ns * 512L, seg, off );

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the sub_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_RWD );
      return 1;
   }

   // Set up all the registers except the command register.

   pio_outbyte( CB_DC, devCtrl );
   pio_outbyte( CB_FR, fr );
   pio_outbyte( CB_SC, sc );
   pio_outbyte( CB_SN, sect );
   pio_outbyte( CB_CL, cylLow );
   pio_outbyte( CB_CH, cylHigh );
   pio_outbyte( CB_DH, devHead );

   // For interrupt mode,
   // Take over INT 7x and initialize interrupt controller
   // and reset interrupt flag.

   int_save_int_vect();

   // program the dma channel for the first or only transfer.

   prog_dma_chan( page1, addr1, count1, modeByte );

   // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.

   pio_outbyte( CB_CMD, cmd );

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   DELAY400NS;

   // Data transfer...
   // If this transfer requires two dma transfers,
   // wait for the first transfer to complete and
   // then program the dma channel for the second transfer.

   if ( ( reg_cmd_info.ec == 0 ) && doTwo )
   {
      // Data transfer...
      // wait for dma chan to transfer first part by monitoring
      // the dma channel's terminal count status bit
      // -or-
      // watch for command completion due to an error
      // -or-
      // time out if this takes too long.

      trc_llt( 0, 0, TRC_LLT_DMA2 );
      while ( 1 )
      {
         if ( inportb( 0xd0 ) & dmaTCbit )   // terminal count yet ?
            break;                           // yes - ok!
         if ( chk_cmd_done() )               // command end ?
         {
            reg_cmd_info.ec = 71;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            trc_llt( 0, 0, TRC_LLT_TOUT );
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 72;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
      }

      // if no error, set up 2nd transfer

      if ( reg_cmd_info.ec == 0 )
      {
         // update bytes transferred count
         reg_cmd_info.totalBytesXfer += count1 << 1;

         // program dma channel to transfer the second part
         prog_dma_chan( page2, addr2, count2, modeByte );
         count1 = count2;
      }
   }

   // End of command...
   // if no error,
   // wait for drive to signal command completion
   // -or-
   // time out if this takes to long.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( int_use_intr_flag )
         trc_llt( 0, 0, TRC_LLT_WINT );
      else
         trc_llt( 0, 0, TRC_LLT_PNBSY );
      while ( 1 )
      {
         if ( chk_cmd_done() )               // done yet ?
            break;                           // yes
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 73;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
      }
   }

   // End of command...
   // disable the dma channel

   trc_llt( 0, 0, TRC_LLT_DMA3 );
   disableChan();

   // End of command...
   // Read the primary status register.  In keeping with the rules
   // stated above the primary status register is read only
   // ONCE.

   status = pio_inbyte( CB_STAT );

   // Final status check...
   // if no error, check final status...
   // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 74;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      }
   }

   // Final status check...
   // if no error, check dma channel terminal count flag

   if ( ( reg_cmd_info.ec == 0 )
        &&
        ( ( inportb( 0xd0 ) & dmaTCbit ) == 0 )
      )
   {
      reg_cmd_info.ec = 71;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
   }

   // Final status check...
   // update total bytes transferred

   if ( reg_cmd_info.ec == 0 )
      reg_cmd_info.totalBytesXfer += count1 << 1;
   else
   {
      clearFF();
      lw1 = inportb( dmaCntrReg );
      lw2 = inportb( dmaCntrReg );
      lw1 = ( ( lw2 << 8 ) | lw1 ) + 1;
      lw1 = lw1 & 0x0000ffffL;
      reg_cmd_info.totalBytesXfer += ( count1 - lw1 ) << 1;
   }

   // Done...
   // Read the output registers and trace the command.

   sub_trace_command();

   // Done...
   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // Done...
   // mark end of a R/W DMA command in low level trace

   trc_llt( 0, 0, TRC_LLT_E_RWD );

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//***********************************************************
//
// dma_isa_packet() - DMA in ISA Multiword for ATAPI Packet
//
//***********************************************************

int dma_isa_packet( int dev,
                    unsigned int cpbc,
                    unsigned int cpseg, unsigned int cpoff,
                    int dir,
                    long dpbc,
                    unsigned int dpseg, unsigned int dpoff )

{
   unsigned char devCtrl;
   unsigned char devHead;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char fr;
   unsigned char status;
   unsigned char reason;
   unsigned char lowCyl;
   unsigned char highCyl;
   int ndx;
   unsigned char * cp;
   unsigned char far * cfp;
   unsigned long lw1, lw2;

   // mark start of isa dma PI cmd in low level trace

   trc_llt( 0, 0, TRC_LLT_S_PID );

   // setup register values

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   cylLow = dpbc & 0x00ff;
   cylHigh = ( dpbc & 0xff00 ) >> 8;
   fr = 0x01;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATAPI;
   reg_cmd_info.ct  = dir ? TRC_TYPE_PDMAO : TRC_TYPE_PDMAI;
   reg_cmd_info.cmd = CMD_PACKET;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = 0;
   reg_cmd_info.sn1 = 0;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Make sure the command packet size is either 12 or 16
   // and save the command packet size and data.

   cpbc = cpbc < 12 ? 12 : cpbc;
   cpbc = cpbc > 12 ? 16 : cpbc;
   reg_atapi_cp_size = cpbc;
   cp = reg_atapi_cp_data;
   cfp = MK_FP( cpseg, cpoff );
   for ( ndx = 0; ndx < cpbc; ndx ++ )
   {
      * cp = * cfp;
      cp ++ ;
      cfp ++ ;
   }

   // Quit now if no dma channel set up

   if ( ! dmaChan )
   {
      reg_cmd_info.ec = 70;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_PID );
      reg_atapi_max_bytes = 32768L;    // reset max bytes
      return 1;
   }

   // set up the dma transfer(s) --
   // the data packet byte count must be even
   // and must not be zero

   if ( dpbc == 0xffff )
      dpbc = 0xfffe;
   if ( dpbc < 2 )
      dpbc = 2;
   if ( dpbc & 0x0001 )
      dpbc ++ ;
   set_up_xfer( dir, dpbc, dpseg, dpoff );

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the reg_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_PID );
      reg_atapi_max_bytes = 32768L;    // reset max bytes
      return 1;
   }

   // Set up all the registers except the command register.

   pio_outbyte( CB_DC, devCtrl );
   pio_outbyte( CB_FR, fr );
   pio_outbyte( CB_SC, 0 );
   pio_outbyte( CB_SN, 0 );
   pio_outbyte( CB_CL, 0 );
   pio_outbyte( CB_CH, 0 );
   pio_outbyte( CB_DH, devHead );

   // For interrupt mode,
   // Take over INT 7x and initialize interrupt controller
   // and reset interrupt flag.

   int_save_int_vect();

   // program the dma channel for the first or only transfer.

   prog_dma_chan( page1, addr1, count1, modeByte );

   // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.

   pio_outbyte( CB_CMD, CMD_PACKET );

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   DELAY400NS;

   // Command packet transfer...
   // Check for protocol failures,
   // the device should have BSY=1 or
   // if BSY=0 then either DRQ=1 or CHK=1.

   sub_atapi_delay( dev );
   status = pio_inbyte( CB_ASTAT );
   if ( status & CB_STAT_BSY )
   {
      // BSY=1 is OK
   }
   else
   {
      if ( status & ( CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         // BSY=0 and DRQ=1 is OK
         // BSY=0 and ERR=1 is OK
      }
      else
      {
         reg_cmd_info.failbits |= FAILBIT0;  // not OK
      }
   }

   // Command packet transfer...
   // Poll Alternate Status for BSY=0.

   trc_llt( 0, 0, TRC_LLT_PNBSY );
   while ( 1 )
   {
      status = pio_inbyte( CB_ASTAT );       // poll for not busy
      if ( ( status & CB_STAT_BSY ) == 0 )
         break;
      if ( tmr_chk_timeout() )               // time out yet ?
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );      // yes
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 75;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         break;
      }
   }

   // Command packet transfer...
   // Check for protocol failures... no interrupt here please!
   // Clear any interrupt the command packet transfer may have caused.

   if ( int_intr_flag )
      reg_cmd_info.failbits |= FAILBIT1;
   int_intr_flag = 0;

   // Command packet transfer...
   // If no error, transfer the command packet.

   if ( reg_cmd_info.ec == 0 )
   {

      // Command packet transfer...
      // Read the primary status register and the other ATAPI registers.

      status = pio_inbyte( CB_STAT );
      reason = pio_inbyte( CB_SC );
      lowCyl = pio_inbyte( CB_CL );
      highCyl = pio_inbyte( CB_CH );

      // Command packet transfer...
      // check status: must have BSY=0, DRQ=1 now

      if (    ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
           != CB_STAT_DRQ
         )
      {
         reg_cmd_info.ec = 76;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      }
      else
      {
         // Command packet transfer...
         // Check for protocol failures...
         // check: C/nD=1, IO=0.

         if ( ( reason &  ( CB_SC_P_TAG | CB_SC_P_REL | CB_SC_P_IO ) )
              || ( ! ( reason &  CB_SC_P_CD ) )
            )
            reg_cmd_info.failbits |= FAILBIT2;
         if ( ( lowCyl != cylLow ) || ( highCyl != cylHigh ) )
            reg_cmd_info.failbits |= FAILBIT3;

         // Command packet transfer...
         // trace cdb byte 0,
         // xfer the command packet (the cdb)

         trc_llt( 0, * (unsigned char far *) MK_FP( cpseg, cpoff ), TRC_LLT_P_CMD );
         pio_rep_outword( CB_DATA, cpseg, cpoff, cpbc >> 1 );
      }
   }

   // Data transfer...
   // If this transfer requires two dma transfers,
   // wait for the first transfer to complete and
   // then program the dma channel for the second transfer.

   if ( ( reg_cmd_info.ec == 0 ) && doTwo )
   {
      // Data transfer...
      // wait for dma chan to transfer first part by monitoring
      // the dma channel's terminal count status bit
      // -or-
      // watch for command completion due to an error
      // -or-
      // time out if this takes too long.

      trc_llt( 0, 0, TRC_LLT_DMA2 );
      while ( 1 )
      {
         if ( inportb( 0xd0 ) & dmaTCbit )   // terminal count yet ?
            break;                           // yes - ok!
         if ( chk_cmd_done() )               // command end ?
         {
            reg_cmd_info.ec = 71;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            trc_llt( 0, 0, TRC_LLT_TOUT );
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 72;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
      }

      // if no error, set up 2nd transfer

      if ( reg_cmd_info.ec == 0 )
      {
         // update bytes transferred count
         reg_cmd_info.totalBytesXfer += count1 << 1;

         // program dma channel to transfer the second part
         prog_dma_chan( page2, addr2, count2, modeByte );
         count1 = count2;
      }
   }

   // End of command...
   // if no error,
   // wait for drive to signal command completion
   // -or-
   // time out if this takes to long.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( int_use_intr_flag )
         trc_llt( 0, 0, TRC_LLT_WINT );
      else
         trc_llt( 0, 0, TRC_LLT_PNBSY );
      while ( 1 )
      {
         if ( chk_cmd_done() )               // done yet ?
            break;                           // yes
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 73;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
      }
   }

   // End of command...
   // disable/stop the dma channel

   trc_llt( 0, 0, TRC_LLT_DMA3 );
   disableChan();

   // End of command...
   // Read the primary status register.  In keeping with the rules
   // stated above the primary status register is read only
   // ONCE.

   status = pio_inbyte( CB_STAT );

   // Final status check...
   // if no error, check final status...
   // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 74;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      }
   }

   // Final status check...
   // Check for protocol failures...
   // check: C/nD=1, IO=1.

   reason = pio_inbyte( CB_SC );
   if ( ( reason & ( CB_SC_P_TAG | CB_SC_P_REL ) )
        || ( ! ( reason & CB_SC_P_IO ) )
        || ( ! ( reason & CB_SC_P_CD ) )
      )
      reg_cmd_info.failbits |= FAILBIT8;

   // Final status check...
   // update total bytes transferred

   clearFF();
   lw1 = inportb( dmaCntrReg );
   lw2 = inportb( dmaCntrReg );
   lw1 = ( ( lw2 << 8 ) | lw1 ) + 1;
   lw1 = lw1 & 0x0000ffffL;
   reg_cmd_info.totalBytesXfer += ( count1 - lw1 ) << 1;

   // Done...
   // Read the output registers and trace the command.

   sub_trace_command();

   // Done...
   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // Done...
   // mark end of isa dma PI cmd in low level trace

   trc_llt( 0, 0, TRC_LLT_E_PID );

   // All done.  The return values of this function are described in
   // ATAIO.H.

   reg_atapi_max_bytes = 32768L;    // reset max bytes
   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

// end ataioisa.c
