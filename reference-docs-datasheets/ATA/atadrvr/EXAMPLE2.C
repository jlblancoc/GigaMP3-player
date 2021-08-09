
// Example2.c -- Some ATAPI command examples

#if 0

   Some simple instructions for those of you that would like to
   compile my driver code and try it.

   1) I've always used Borland C (or C++, the driver code is plain C
   code).  One file contains a few lines of embedded ASM code.

   2) I've always used small memory model but I think the driver code
   will work with any memory mode.

   3) Here is a very small program that shows how to use the driver
   code to issue some ATAPI commands.

   This program has one optional command line parameter to specify
   the device to run on: P0, P1, S0 or S1.

#endif

#define INCL_ISA_DMA 0     // set to 1 to include ISA DMA

#define INCL_PCI_DMA 1     // set to 1 to include PCI DMA

#if INCL_ISA_DMA && INCL_PCI_DMA

   #ERROR Only one type of DMA can be included !

#endif

// begin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "ataio.h"

unsigned char * devTypeStr[]
       = { "no device found", "unknown type", "ATA", "ATAPI" };

unsigned char cdb[16];
unsigned char far * cdbPtr;

unsigned char buffer[4096];
unsigned char far * bufferPtr;

//**********************************************

// a little function to display all the error
// and trace information from the driver

void ShowAll( void );

void ShowAll( void )

{
   unsigned char * cp;

   printf( "ERROR !\n" );

   // display the command error information
   trc_err_dump1();           // start
   while ( 1 )
   {
      cp = trc_err_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
   }

   // display the command history
   trc_cht_dump1();           // start
   while ( 1 )
   {
      cp = trc_cht_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
   }

   // display the low level trace
   trc_llt_dump1();           // start
   while ( 1 )
   {
      cp = trc_llt_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
   }

   // now clear the command history and low level traces
   trc_cht_dump0();     // zero the command history
   trc_llt_dump0();     // zero the low level trace
}

//**********************************************

int main( int ac, char * av[] )
{
   int base = 0x1f0;
   int intnum = 14;
   int dev = 0;
   int numDev;
   int rc;
   int ndx;

   printf( "EXAMPLE2 Version " ATA_DRIVER_VERSION ".\n" );

   // initialize far pointer to the I/O buffers
   bufferPtr = (unsigned char far *) buffer;
   cdbPtr = (unsigned char far *) cdb;

   // process command line parameter
   if ( ac > 1 )
   {
      if ( ! strnicmp( av[1], "p0", 2 ) )
         /* do nothing */ ;
      else
      if ( ! strnicmp( av[1], "p1", 2 ) )
         dev = 1;
      else
      if ( ! strnicmp( av[1], "s0", 2 ) )
      {
         base = 0x170;
         intnum = 15;
      }
      else
      if ( ! strnicmp( av[1], "s1", 2 ) )
      {
         base = 0x170;
         intnum = 15;
         dev = 1;
      }
      else
      {
         printf( "\nInvalid command line parameter !\n" );
         return 1;
      }
   }

   // 1) must tell the driver what the I/O port addresses.
   //    note that the driver also supports all the PCMCIA
   //    PC Card modes (I/O and memory).
   pio_set_iobase_addr( base, base + 0x200 );
   printf( "\nUsing I/O base addresses %03X and %03X with IRQ %d.\n",
           base, base + 0x200, intnum );

   // 2) find out what devices are present -- this is the step
   // many driver writers ignore.  You really can't just do
   // resets and commands without first knowing what is out there.
   // Even if you don't care the driver does care.
   numDev = reg_config();
   printf( "\nFound %d devices on this ATA interface:\n", numDev );
   printf( "   Device 0 type is %s.\n", devTypeStr[ reg_config_info[0] ] );
   printf( "   Device 1 type is %s.\n", devTypeStr[ reg_config_info[1] ] );
   printf( "\nDevice %d is the selected device for this run...\n", dev );

   // turn on the ~100ms delay for atapi devices
   reg_atapi_delay_flag = 1;

   // that's the basics, now do some stuff in polling mode
   // (see example1.c for interrupt mode use)

   // do an ATA soft reset (SRST) and return the command block
   // regs for device 0 in struct reg_cmd_info
   printf( "Soft Reset...\n" );
   rc = reg_reset( 0, dev );
   if ( rc )
      ShowAll();

   // do an ATAPI Identify command in LBA mode
   printf( "ATAPI Identify, polling...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_pio_data_in_lba(
               dev, CMD_IDENTIFY_DEVICE_PACKET,
               0, 0,
               0L,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr ),
               1, 0 );
   if ( rc )
      ShowAll();
   else
   {
      // you get to add the code here to display the ID data
   }

   // do an ATAPI Request Sense command and display some of the data
   printf( "ATAPI Request Sense, polling...\n" );
   memset( cdb, 0, sizeof( cdb ) );
   cdb[0] = 0x03;    // RS command code
   cdb[4] = 32;      // allocation length
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_packet(
               dev,
               12, FP_SEG( cdbPtr ), FP_OFF( cdbPtr ),
               0,
               4096, FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
               );
   if ( rc )
      ShowAll();
   else
   {
      // here I'll give you some help -- lets look at the RS data
      // first a problem with ATAPI device: how much data was
      // really transferred?  You can never really be sure...
      if ( reg_cmd_info.totalBytesXfer != ( 8U + buffer[7] ) )
         printf( "Number of bytes transferred (%ld) does not match \n"
                 "8 + byte 7 in the RS data received (%u) ! \n",
                 reg_cmd_info.totalBytesXfer,
                 8U + buffer[7] );
      // display some of the RS data
      printf( "Error code=%02X, Sense Key=%02X, ASC=%02X, ASCQ=%02X\n",
              buffer[0], buffer[2] & 0x0f, buffer[12], buffer[13] );
   }

   // do an ATAPI Read TOC command and display the TOC data
   printf( "ATAPI CD-ROM Read TOC, polling...\n" );
   memset( cdb, 0, sizeof( cdb ) );
   cdb[0] = 0x43;    // command code
   cdb[1] = 0x02;    // MSF flag
   cdb[7] = 0x10;    // allocation length
   cdb[8] = 0x00;    //    of 4096
   cdb[9] = 0x80;    // TOC format
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_packet(
               dev,
               12, FP_SEG( cdbPtr ), FP_OFF( cdbPtr ),
               0,
               4096, FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
               );
   if ( rc )
      ShowAll();
   else
   {
      // and here too I'll give you some help looking at the TOC data
      // again, check the number of bytes transferred (notice
      // how every SCSI like command is different here?
      if ( reg_cmd_info.totalBytesXfer
           !=
           ( 2U + ( buffer[0] * 256U ) + buffer[1] )
         )
         printf( "Number of bytes transferred (%ld) does not match \n"
                 "2 + bytes 0-1 in the TOC data received (%u) ! \n",
                 reg_cmd_info.totalBytesXfer,
                 2U + ( buffer[0] * 256U ) + buffer[1] );
      // display the TOC data
      printf( "First Session=%02X, Last Session=%02X\n",
               buffer[2], buffer[3] );
      printf( "TOC entries (11 bytes each)... \n" );
      rc = ( ( buffer[0] * 256U ) + buffer[1] - 2U ) / 11;
      ndx = 4;
      while ( rc > 0 )
      {
         printf( "   %02X %02X %02X "
                    "%02X %02x %02X "
                    "%02X %02X %02X "
                    "%02X %02x \n",
                     buffer[ndx+0], buffer[ndx+1], buffer[ndx+2],
                     buffer[ndx+3], buffer[ndx+4], buffer[ndx+5],
                     buffer[ndx+6], buffer[ndx+7], buffer[ndx+8],
                     buffer[ndx+9], buffer[ndx+10], buffer[ndx+11]
               );
         rc -- ;
         ndx = ndx + 11;
      }
   }

#if INCL_ISA_DMA

   // If you set INCL_ISA_DMA to 1 AND you have read the Driver
   // User's Guide AND you have an ISA bus ATA host adapter with
   // the two additional wires installed (for DMARQ and DMACK), then
   // you can have fun with the following...

   // You MUST config the DMA channel to use.  Only
   // ISA DMA channels 5, 6 or 7 can be used and your ISA
   // ATA host adapter MUST be setup to use the selected
   // channel.  Lets use channel 6.

   rc = dma_isa_config( 6 );
   if ( rc )
      printf( "ERROR !  Call to dma_isa_config() failed!\n" );
   else
   {
      // do an ATAPI Read TOC command and display the TOC data
      // but do it in DMA mode
      printf( "ATAPI CD-ROM Read TOC in DMA, polling...\n" );
      memset( cdb, 0, sizeof( cdb ) );
      cdb[0] = 0x43;    // command code
      cdb[1] = 0x02;    // MSF flag
      cdb[7] = 0x10;    // allocation length
      cdb[8] = 0x00;    //    of 4096
      cdb[9] = 0x80;    // TOC format
      memset( buffer, 0, sizeof( buffer ) );
      rc = dma_isa_packet(
                  dev,
                  12, FP_SEG( cdbPtr ), FP_OFF( cdbPtr ),
                  0,
                  4096, FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
                  );
      if ( rc )
         ShowAll();
      else
      {
         // and here too I'll give you some help looking at the TOC data
         // again, check the number of bytes transferred (notice
         // how every SCSI like command is different here?
         if ( reg_cmd_info.totalBytesXfer
              !=
              ( 2U + ( buffer[0] * 256U ) + buffer[1] )
            )
            printf( "Number of bytes transferred (%ld) does not match \n"
                    "2 + bytes 0-1 in the TOC data received (%u) ! \n",
                    reg_cmd_info.totalBytesXfer,
                    2U + ( buffer[0] * 256U ) + buffer[1] );
         // display the TOC data
         printf( "First Session=%02X, Last Session=%02X\n",
                  buffer[2], buffer[3] );
         printf( "TOC entries (11 bytes each)... \n" );
         rc = ( ( buffer[0] * 256U ) + buffer[1] - 2U ) / 11;
         ndx = 4;
         while ( rc > 0 )
         {
            printf( "   %02X %02X %02X "
                       "%02X %02x %02X "
                       "%02X %02X %02X "
                       "%02X %02x \n",
                        buffer[ndx+0], buffer[ndx+1], buffer[ndx+2],
                        buffer[ndx+3], buffer[ndx+4], buffer[ndx+5],
                        buffer[ndx+6], buffer[ndx+7], buffer[ndx+8],
                        buffer[ndx+9], buffer[ndx+10], buffer[ndx+11]
                  );
            rc -- ;
            ndx = ndx + 11;
         }
      }
   }

#endif

#if INCL_PCI_DMA

   // If you set INCL_PCI_DMA to 1 AND you have read the Driver
   // User's Guide AND you have an PCI bus ATA host adapter and
   // you must know the I/O address of the Bus Master Control
   // Registers (BMCR or SFF-8038) registers. This address is
   // found by scanning all the PCI bus devices and finding the
   // ATA host adapter you want to use. The BMCR I/O address
   // is at offset 20H of the PCI configuration space for the
   // ATA host adapter.

   #define BMCR_IO_ADDR 0xFF00   // YOU MUST SUPPLY THIS VALUE

   // first, tell the driver where the BMCR is located.

   rc = dma_pci_config( BMCR_IO_ADDR );
   if ( rc )
   {
      printf( "ERROR !  Call to dma_pci_config() failed,\n" );
      printf( "         dma_pci_config() returned %d!\n", rc );
   }
   else
   {
      // do an ATAPI Read TOC command and display the TOC data
      // but do it in DMA mode
      printf( "ATAPI CD-ROM Read TOC in DMA, polling...\n" );
      memset( cdb, 0, sizeof( cdb ) );
      cdb[0] = 0x43;    // command code
      cdb[1] = 0x02;    // MSF flag
      cdb[7] = 0x10;    // allocation length
      cdb[8] = 0x00;    //    of 4096
      cdb[9] = 0x80;    // TOC format
      memset( buffer, 0, sizeof( buffer ) );
      rc = dma_pci_packet(
                  dev,
                  12, FP_SEG( cdbPtr ), FP_OFF( cdbPtr ),
                  0,
                  4096, FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
                  );
      if ( rc )
         ShowAll();
      else
      {
         // and here too I'll give you some help looking at the TOC data
         // again, check the number of bytes transferred (notice
         // how every SCSI like command is different here?
         if ( reg_cmd_info.totalBytesXfer
              !=
              ( 2U + ( buffer[0] * 256U ) + buffer[1] )
            )
            printf( "Number of bytes transferred (%ld) does not match \n"
                    "2 + bytes 0-1 in the TOC data received (%u) ! \n",
                    reg_cmd_info.totalBytesXfer,
                    2U + ( buffer[0] * 256U ) + buffer[1] );
         // display the TOC data
         printf( "First Session=%02X, Last Session=%02X\n",
                  buffer[2], buffer[3] );
         printf( "TOC entries (11 bytes each)... \n" );
         rc = ( ( buffer[0] * 256U ) + buffer[1] - 2U ) / 11;
         ndx = 4;
         while ( rc > 0 )
         {
            printf( "   %02X %02X %02X "
                       "%02X %02x %02X "
                       "%02X %02X %02X "
                       "%02X %02x \n",
                        buffer[ndx+0], buffer[ndx+1], buffer[ndx+2],
                        buffer[ndx+3], buffer[ndx+4], buffer[ndx+5],
                        buffer[ndx+6], buffer[ndx+7], buffer[ndx+8],
                        buffer[ndx+9], buffer[ndx+10], buffer[ndx+11]
                  );
            rc -- ;
            ndx = ndx + 11;
         }
      }
   }

#endif

   return 0;
}

// end example2.c
