
// Example1.c -- Some ATA command examples

#if 0

   Some simple instructions for those of you that would like to
   compile my driver code and try it.

   1) I've always used Borland C (or C++, the driver code is plain C
   code).  One file contains a few lines of embedded ASM code.

   2) I've always used small memory model but I think the driver code
   will work with any memory mode.

   3) Here is a very small program that shows how to use the driver
   code to issue some ATA commands.

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

   printf( "EXAMPLE1 Version " ATA_DRIVER_VERSION ".\n" );

   // initialize far pointer to the I/O buffer
   bufferPtr = (unsigned char far *) buffer;

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

   // that's the basics, now do some stuff in polling mode

   // do an ATA soft reset (SRST) and return the command block
   // regs for device 0 in struct reg_cmd_info
   printf( "Soft Reset...\n" );
   rc = reg_reset( 0, dev );
   if ( rc )
      ShowAll();

   // do a seek command in CHS mode to CHS=100,2,5
   printf( "Seek, CHS, polling...\n" );
   rc = reg_non_data( dev, CMD_SEEK, 0, 0, 100, 2, 5 );
   if ( rc )
      ShowAll();

   // do a seek command in LBA mode to LBA=5025
   printf( "Seek, LBA, polling...\n" );
   rc = reg_non_data_lba( dev, CMD_SEEK, 0, 0, 5025L );
   if ( rc )
      ShowAll();

   // do an ATA Identify command in CHS mode
   printf( "ATA Identify, CHS, polling...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_pio_data_in(
               dev, CMD_IDENTIFY_DEVICE,
               0, 0,
               0, 0, 0,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr ),
               1, 0
               );
   if ( rc )
      ShowAll();

   // do an ATA Identify command in LBA mode
   printf( "ATA Identify, LBA, polling...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_pio_data_in_lba(
               dev, CMD_IDENTIFY_DEVICE,
               0, 0,
               0L,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr ),
               1, 0
               );
   if ( rc )
      ShowAll();

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
      #if 0
         // DO NOT DO THIS SET FEATURES UNLESS REQUIRED !
         // YOU MUST KNOW WHICH MODE THE HOST ADAPTER HAS
         // BEEN PROGRAMMED TO USE!
         // do a set features to turn on MW DMA
         printf( "Set Feature, FR=03H, SC=20H, polling...\n" );
         rc = reg_non_data( dev, CMD_SET_FEATURES, 0x03, 0x20, 0, 0, 0 );
         if ( rc )
            ShowAll();
      #endif

      // note that we are doing DMA in polling mode here.

      // do an ATA Read DMA command in LBA mode
      // lets read 3 sectors starting at LBA=5
      printf( "ATA Read DMA, LBA, polling...\n" );
      memset( buffer, 0, sizeof( buffer ) );
      rc = dma_isa_ata_lba(
                  dev, CMD_READ_DMA,
                  0, 3,
                  5L,
                  FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
                  );
      if ( rc )
         ShowAll();
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
      #if 0
         // DO NOT DO THIS SET FEATURES UNLESS REQUIRED!
         // YOU MUST KNOW WHICH MODE THE HOST ADAPTER HAS
         // BEEN PROGRAMMED TO USE!
         // do a set features to turn on MW DMA (or UltraDMA)
         printf( "Set Feature, FR=03H, SC=20H, polling...\n" );
         rc = reg_non_data( dev, CMD_SET_FEATURES, 0x03, 0x40, 0, 0, 0 );
         if ( rc )
            ShowAll();
      #endif

      // note that we are doing DMA in polling mode here.

      // do an ATA Read DMA command in LBA mode
      // lets read 3 sectors starting at LBA=5
      printf( "ATA Read DMA, LBA, polling...\n" );
      memset( buffer, 0, sizeof( buffer ) );
      rc = dma_pci_ata_lba(
                  dev, CMD_READ_DMA,
                  0, 3,
                  5L,
                  FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
                  );
      if ( rc )
         ShowAll();
   }

#endif

   // OK, so you want to try using interrupts...
   // first you must enable the appropriate irq
   // HOWEVER, if call this function you MUST make
   // sure your program does not terminate without
   // calling int_disable() !!!
   printf( "Interrupt on...\n" );
   rc = int_enable_irq( intnum );
   if ( rc )
   {
      printf( "Unable to set interrupt mode using IRQ 14 !\n" );
      printf( "Polling mode remains in effect.\n" );
   }

   // do a seek command in CHS mode to CHS=100,2,5
   printf( "Seek, CHS, interrupt...\n" );
   rc = reg_non_data( dev, CMD_SEEK, 0, 0, 100, 2, 5 );
   if ( rc )
      ShowAll();

   // do a seek command in LBA mode to LBA=5025
   printf( "Seek, LBA, interrupt...\n" );
   rc = reg_non_data_lba( dev, CMD_SEEK, 0, 0, 5025L );
   if ( rc )
      ShowAll();

   // do an ATA Identify command in CHS mode
   printf( "ATA Identify, CHS, interrupt...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_pio_data_in(
               dev, CMD_IDENTIFY_DEVICE,
               0, 0,
               0, 0, 0,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr ),
               1, 0
               );
   if ( rc )
      ShowAll();

   // do an ATA Identify command in LBA mode
   printf( "ATA Identify, LBA, interrupt...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = reg_pio_data_in_lba(
               dev, CMD_IDENTIFY_DEVICE,
               0, 0,
               0L,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr ),
               1, 0
               );
   if ( rc )
      ShowAll();

#if INCL_ISA_DMA

   // Now do ISA DMA with interrupts.

   // do an ATA Read DMA command in LBA mode
   // lets read 3 sectors starting at LBA=5
   printf( "ATA Read DMA, LBA, interrupt...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = dma_isa_ata_lba(
               dev, CMD_READ_DMA,
               0, 3,
               5L,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
               );
   if ( rc )
      ShowAll();

#endif

#if INCL_PCI_DMA

   // Now do PCI DMA with interrupts.

   // do an ATA Read DMA command in LBA mode
   // lets read 3 sectors starting at LBA=5
   printf( "ATA Read DMA, LBA, interrupt...\n" );
   memset( buffer, 0, sizeof( buffer ) );
   rc = dma_pci_ata_lba(
               dev, CMD_READ_DMA,
               0, 3,
               5L,
               FP_SEG( bufferPtr ), FP_OFF( bufferPtr )
               );
   if ( rc )
      ShowAll();

#endif

   // disable intrq -- you MUST do this if you
   // called int_enable_irq() !!!
   printf( "Interrupt off (back to polling)...\n" );
   int_disable_irq();

   return 0;
}

// end example1.c
