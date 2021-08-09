//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPIO.C
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
// This module contains inline assembler code so you'll
// also need Borland's Assembler.
//
// This C source contains the low level I/O port IN/OUT functions.
//********************************************************************

#include <dos.h>

#include "ataio.h"

#include "ataiopd.h"

//*************************************************************
//
// Host adapter base addresses.
//
//*************************************************************

unsigned int pio_base_addr1 = 0x1f0;
unsigned int pio_base_addr2 = 0x3f0;

unsigned int pio_memory_seg = 0;
int pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;

unsigned int pio_reg_addrs[10];

unsigned char pio_last_write[10];
unsigned char pio_last_read[10];

int pio_xfer_width = 16;

//*************************************************************
//
// Set the host adapter i/o base addresses.
//
//*************************************************************

void pio_set_iobase_addr( unsigned int base1, unsigned int base2 )

{

   pio_base_addr1 = base1;
   pio_base_addr2 = base2;
   pio_memory_seg = 0;
   pio_reg_addrs[ CB_DATA ] = pio_base_addr1 + 0;  // 0
   pio_reg_addrs[ CB_FR   ] = pio_base_addr1 + 1;  // 1
   pio_reg_addrs[ CB_SC   ] = pio_base_addr1 + 2;  // 2
   pio_reg_addrs[ CB_SN   ] = pio_base_addr1 + 3;  // 3
   pio_reg_addrs[ CB_CL   ] = pio_base_addr1 + 4;  // 4
   pio_reg_addrs[ CB_CH   ] = pio_base_addr1 + 5;  // 5
   pio_reg_addrs[ CB_DH   ] = pio_base_addr1 + 6;  // 6
   pio_reg_addrs[ CB_CMD  ] = pio_base_addr1 + 7;  // 7
   pio_reg_addrs[ CB_DC   ] = pio_base_addr2 + 6;  // 8
   pio_reg_addrs[ CB_DA   ] = pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// Set the host adapter memory base addresses.
//
//*************************************************************

void pio_set_memory_addr( unsigned int seg )

{

   pio_base_addr1 = 0;
   pio_base_addr2 = 8;
   pio_memory_seg = seg;
   pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;
   pio_reg_addrs[ CB_DATA ] = pio_base_addr1 + 0;  // 0
   pio_reg_addrs[ CB_FR   ] = pio_base_addr1 + 1;  // 1
   pio_reg_addrs[ CB_SC   ] = pio_base_addr1 + 2;  // 2
   pio_reg_addrs[ CB_SN   ] = pio_base_addr1 + 3;  // 3
   pio_reg_addrs[ CB_CL   ] = pio_base_addr1 + 4;  // 4
   pio_reg_addrs[ CB_CH   ] = pio_base_addr1 + 5;  // 5
   pio_reg_addrs[ CB_DH   ] = pio_base_addr1 + 6;  // 6
   pio_reg_addrs[ CB_CMD  ] = pio_base_addr1 + 7;  // 7
   pio_reg_addrs[ CB_DC   ] = pio_base_addr2 + 6;  // 8
   pio_reg_addrs[ CB_DA   ] = pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// These functions do basic IN/OUT of byte and word values.
//
//*************************************************************

unsigned char pio_inbyte( unsigned int addr )

{
   unsigned int regAddr;
   unsigned char uc;
   unsigned char far * ucp;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      ucp = (unsigned char far *) MK_FP( pio_memory_seg, regAddr );
      uc = * ucp;
   }
   else
   {
      uc = (unsigned char) inportb( regAddr );
   }
   pio_last_read[ addr ] = uc;
   if ( addr == CB_STAT || addr == CB_ASTAT )
      trc_llt( addr, uc & ( ~ CB_STAT_IDX ), TRC_LLT_INB );
   else
      trc_llt( addr, uc, TRC_LLT_INB );
   return uc;
}

//*********************************************************

void pio_outbyte( unsigned int addr, unsigned char data )

{
   unsigned int regAddr;
   unsigned char far * ucp;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      ucp = (unsigned char far *) MK_FP( pio_memory_seg, regAddr );
      * ucp = data;
   }
   else
   {
      outportb( regAddr, data );
   }
   pio_last_write[ addr ] = data;
   trc_llt( addr, data, TRC_LLT_OUTB );
}

//*********************************************************

unsigned int pio_inword( unsigned int addr )

{
   unsigned int regAddr;
   unsigned int ui;
   unsigned int far * uip;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      uip = (unsigned int far *) MK_FP( pio_memory_seg, regAddr );
      ui = * uip;
   }
   else
   {
      ui = inport( regAddr );
   }
   trc_llt( addr, 0, TRC_LLT_INW );
   return ui;
}

//*********************************************************

void pio_outword( unsigned int addr, unsigned int data )

{
   unsigned int regAddr;
   unsigned int far * uip;

   regAddr = pio_reg_addrs[ addr ];
   if ( pio_memory_seg )
   {
      uip = (unsigned int far *) MK_FP( pio_memory_seg, regAddr );
      * uip = data;
   }
   else
   {
      outport( regAddr, data );
   }
   trc_llt( addr, 0, TRC_LLT_OUTW );
}

//*************************************************************
//
// These functions do REP INS/OUTS (PIO data transfers).
//
//*************************************************************

// Note: pio_rep_inbyte() can be called directly but usually it
// is called by pio_rep_inword() based on the value of the
// pio_xfer_width variables.

void pio_rep_inbyte( unsigned int addrDataReg,
                     unsigned int bufSeg, unsigned int bufOff,
                     long byteCnt )

{
   unsigned int dataRegAddr;
   unsigned int bCnt;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   while ( byteCnt > 0L )
   {

      if ( byteCnt > 16384L )
         bCnt = 16384;
      else
         bCnt = (unsigned int) byteCnt;

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  di
      asm   push  es

      asm   mov   ax,bufSeg
      asm   mov   es,ax
      asm   mov   di,bufOff

      asm   mov   cx,bCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   insb

      asm   pop   es
      asm   pop   di
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

      trc_llt( addrDataReg, 0, TRC_LLT_INSB );

      byteCnt = byteCnt - (long) bCnt;

      pio_inbyte( CB_ASTAT );    // just for debugging

   }
}

//*********************************************************

// Note: pio_rep_outbyte() can be called directly but usually it
// is called by pio_rep_outword() based on the value of the
// pio_xfer_width variables.

void pio_rep_outbyte( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      long byteCnt )

{
   unsigned int dataRegAddr;
   unsigned int bCnt;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   while ( byteCnt > 0L )
   {

      if ( byteCnt > 16384L )
         bCnt = 16384;
      else
         bCnt = (unsigned int) byteCnt;

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  si
      asm   push  ds

      asm   mov   ax,bufSeg
      asm   mov   ds,ax
      asm   mov   si,bufOff

      asm   mov   cx,bCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   outsb

      asm   pop   ds
      asm   pop   si
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

      trc_llt( addrDataReg, 0, TRC_LLT_OUTSB );

      byteCnt = byteCnt - (long) bCnt;

      pio_inbyte( CB_ASTAT );    // just for debugging

   }
}

//*********************************************************

// Note: pio_rep_indword() can be called directly but usually it
// is called by pio_rep_inword() based on the value of the
// pio_xfer_width variable.

void pio_rep_indword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      unsigned int dwordCnt )

{
   unsigned int dataRegAddr;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   asm   .386

   asm   push  ax
   asm   push  cx
   asm   push  dx
   asm   push  di
   asm   push  es

   asm   mov   ax,bufSeg
   asm   mov   es,ax
   asm   mov   di,bufOff

   asm   mov   cx,dwordCnt
   asm   mov   dx,dataRegAddr

   asm   cld

   asm   rep   insd

   asm   pop   es
   asm   pop   di
   asm   pop   dx
   asm   pop   cx
   asm   pop   ax

   trc_llt( addrDataReg, 0, TRC_LLT_INSD );
}

//*********************************************************

// Note: pio_rep_outdword() can be called directly but usually it
// is called by pio_rep_outword() based on the value of the
// pio_xfer_width variable.

void pio_rep_outdword( unsigned int addrDataReg,
                       unsigned int bufSeg, unsigned int bufOff,
                       unsigned int dwordCnt )

{
   unsigned int dataRegAddr;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   asm   .386

   asm   push  ax
   asm   push  cx
   asm   push  dx
   asm   push  si
   asm   push  ds

   asm   mov   ax,bufSeg
   asm   mov   ds,ax
   asm   mov   si,bufOff

   asm   mov   cx,dwordCnt
   asm   mov   dx,dataRegAddr

   asm   cld

   asm   rep   outsd

   asm   pop   ds
   asm   pop   si
   asm   pop   dx
   asm   pop   cx
   asm   pop   ax

   trc_llt( addrDataReg, 0, TRC_LLT_OUTSD );
}

//*********************************************************

// Note: pio_rep_inword() is the primary way perform PIO
// Data In transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_rep_inword( unsigned int addrDataReg,
                     unsigned int bufSeg, unsigned int bufOff,
                     unsigned int wordCnt )

{
   unsigned int dataRegAddr;
   volatile unsigned int far * uip1;
   unsigned int far * uip2;
   volatile unsigned char far * ucp1;
   unsigned char far * ucp2;
   long bCnt;
   int memDtOpt;
   unsigned int randVal;

   if ( pio_memory_seg )
   {

      // PCMCIA Memory mode data transfer.

      // set Data reg address per pio_memory_dt_opt
      dataRegAddr = 0x0000;
      memDtOpt = pio_memory_dt_opt;
      if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
      {
         randVal = * (unsigned int *) MK_FP( 0x40, 0x6c );
         memDtOpt = randVal % 3;
      }
      if ( memDtOpt == PIO_MEMORY_DT_OPT8 )
         dataRegAddr = 0x0008;
      if ( memDtOpt == PIO_MEMORY_DT_OPTB )
      {
         dataRegAddr = 0x0400;
         if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
            dataRegAddr = dataRegAddr | ( randVal & 0x03fe );
      }

      if ( pio_xfer_width == 8 )
      {
         // PCMCIA Memory mode 8-bit
         bCnt = ( (long) wordCnt ) * 2L;
         ucp1 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
         ucp2 = (unsigned char far *) MK_FP( bufSeg, bufOff );
         for ( ; bCnt > 0; bCnt -- )
         {
            * ucp2 = * ucp1;
            ucp2 ++ ;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr ++ ;
               dataRegAddr = ( dataRegAddr & 0x03ff ) | 0x0400;
               ucp1 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_INSB );
      }
      else
      {
         // PCMCIA Memory mode 16-bit
         uip1 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
         uip2 = (unsigned int far *) MK_FP( bufSeg, bufOff );
         for ( ; wordCnt > 0; wordCnt -- )
         {
            * uip2 = * uip1;
            uip2 ++ ;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr = dataRegAddr + 2;
               dataRegAddr = ( dataRegAddr & 0x03fe ) | 0x0400;
               uip1 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_INSW );
      }
   }
   else
   {

      // Data transfer using INS instruction.

      dataRegAddr = pio_reg_addrs[ addrDataReg ];

      if ( pio_xfer_width == 8 )
      {
         // do REP INS
         pio_rep_inbyte( addrDataReg, bufSeg, bufOff, ( (long) wordCnt ) * 2L );
         return;
      }

      if ( ( pio_xfer_width == 32 ) && ( ! ( wordCnt & 0x0001 ) ) )
      {
         // do REP INSD
         pio_rep_indword( addrDataReg, bufSeg, bufOff, wordCnt / 2 );
         return;
      }

      // do REP INSW

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  di
      asm   push  es

      asm   mov   ax,bufSeg
      asm   mov   es,ax
      asm   mov   di,bufOff

      asm   mov   cx,wordCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   insw

      asm   pop   es
      asm   pop   di
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

      trc_llt( addrDataReg, 0, TRC_LLT_INSW );
   }
}

//*********************************************************

// Note: pio_rep_outword() is the primary way perform PIO
// Data Out transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_rep_outword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      unsigned int wordCnt )

{
   unsigned int dataRegAddr;
   unsigned int far * uip1;
   unsigned int far * uip2;
   unsigned char far * ucp1;
   unsigned char far * ucp2;
   long bCnt;
   int memDtOpt;
   unsigned int randVal;

   if ( pio_memory_seg )
   {

      // PCMCIA Memory mode data transfer.

      // set Data reg address per pio_memory_dt_opt
      dataRegAddr = 0x0000;
      memDtOpt = pio_memory_dt_opt;
      if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
      {
         randVal = * (unsigned int *) MK_FP( 0x40, 0x6c );
         memDtOpt = randVal % 3;
      }
      if ( memDtOpt == PIO_MEMORY_DT_OPT8 )
         dataRegAddr = 0x0008;
      if ( memDtOpt == PIO_MEMORY_DT_OPTB )
      {
         dataRegAddr = 0x0400;
         if ( pio_memory_dt_opt == PIO_MEMORY_DT_OPTR )
            dataRegAddr = dataRegAddr | ( randVal & 0x03fe );
      }

      if ( pio_xfer_width == 8 )
      {
         // PCMCIA Memory mode 8-bit
         bCnt = ( (long) wordCnt ) * 2L;
         ucp1 = (unsigned char far *) MK_FP( bufSeg, bufOff );
         ucp2 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
         for ( ; bCnt > 0; bCnt -- )
         {
            * ucp2 = * ucp1;
            ucp1 ++ ;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr ++ ;
               dataRegAddr = ( dataRegAddr & 0x03ff ) | 0x0400;
               ucp2 = (unsigned char far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_OUTSB );
      }
      else
      {
         // PCMCIA Memory mode 16-bit
         uip1 = (unsigned int far *) MK_FP( bufSeg, bufOff );
         uip2 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
         for ( ; wordCnt > 0; wordCnt -- )
         {
            * uip2 = * uip1;
            uip1 ++ ;
            if ( memDtOpt == PIO_MEMORY_DT_OPTB )
            {
               dataRegAddr = dataRegAddr + 2;
               dataRegAddr = ( dataRegAddr & 0x03fe ) | 0x0400;
               uip2 = (unsigned int far *) MK_FP( pio_memory_seg, dataRegAddr );
            }
         }
         trc_llt( addrDataReg, 0, TRC_LLT_OUTSW );
      }
   }
   else
   {

      // Data transfer using OUTS instruction.

      dataRegAddr = pio_reg_addrs[ addrDataReg ];

      if ( pio_xfer_width == 8 )
      {
         // do REP OUTS
         pio_rep_outbyte( addrDataReg, bufSeg, bufOff, ( (long) wordCnt ) * 2L );
         return;
      }

      if ( ( pio_xfer_width == 32 ) && ( ! ( wordCnt & 0x0001 ) ) )
      {
         // do REP OUTSD
         pio_rep_outdword( addrDataReg, bufSeg, bufOff, wordCnt / 2 );
         return;
      }

      // do REP OUTSW

      asm   .386

      asm   push  ax
      asm   push  cx
      asm   push  dx
      asm   push  si
      asm   push  ds

      asm   mov   ax,bufSeg
      asm   mov   ds,ax
      asm   mov   si,bufOff

      asm   mov   cx,wordCnt
      asm   mov   dx,dataRegAddr

      asm   cld

      asm   rep   outsw

      asm   pop   ds
      asm   pop   si
      asm   pop   dx
      asm   pop   cx
      asm   pop   ax

      trc_llt( addrDataReg, 0, TRC_LLT_OUTSW );
   }
}

// end ataiopio.c
