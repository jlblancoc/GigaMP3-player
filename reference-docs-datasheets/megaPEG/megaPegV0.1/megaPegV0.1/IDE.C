#define IDE_C
#include "equates.h"

u08 IDE_WaitState(u08 set, u08 clr){
  u08 tmp, k;

  tmp = IDE_ReadReg(REG_STATUS);

  #ifdef DEBUG_IDE
    printf("WS i %x",tmp);
  #endif

  k = 0xff;
  while((((tmp & set) != set) || ((tmp & clr) != 0)) && (k > 0)){
    k--;
    Delay_100us(10);
    tmp = IDE_ReadReg(REG_STATUS);
  }

  #ifdef DEBUG_IDE
    printf("o %x ",tmp);
    if(k==0) printf("t %k ",tmp);
  #endif

  if(k>0) return TRUE;
  else return FALSE;
}

void IDE_WriteWord(u16 data){
  disableXRAM();
  outp(REG_DATA, IDE_CTRL_PORT);
  outp(data,     IDE_DATAH_PORT);
  outp((data>>8),IDE_DATAL_PORT);
  outp(0xFF,     DDR(IDE_DATAL_PORT));    // port output
  outp(0xFF,     DDR(IDE_DATAH_PORT));    // port output
  _NOP();
  cbi(IDE_CTRL_PORT, ideDIOW);
  _NOP();
  sbi(IDE_CTRL_PORT, ideDIOW);
  outp(0x00,     DDR(IDE_DATAL_PORT));    // port input
  outp(0x00,     DDR(IDE_DATAH_PORT));    // port input
  enableXRAM();
}

u16 IDE_ReadWord(void)
{
	u16 tmp;

  disableXRAM();
  outp(REG_DATA, IDE_CTRL_PORT);
  _NOP();
  cbi(IDE_CTRL_PORT, ideDIOR);
  _NOP();
  tmp = inp(PIN(IDE_DATAL_PORT));
  tmp = (tmp << 8) | inp(PIN(IDE_DATAH_PORT));
  sbi(IDE_CTRL_PORT, ideDIOR);
  enableXRAM();

	return tmp;
}

void IDE_WriteReg(u08 reg, u08 data){
  disableXRAM();
  outp(reg,  IDE_CTRL_PORT);
  outp(data, IDE_DATAL_PORT);
  outp(0xFF, DDR(IDE_DATAL_PORT));           // port output
  _NOP();
  cbi(IDE_CTRL_PORT, ideDIOW);
  _NOP();
  sbi(IDE_CTRL_PORT, ideDIOW);
  outp(0x00, DDR(IDE_DATAL_PORT));           // port input
  enableXRAM();
}

u08 IDE_ReadReg(u08 reg){
	u08 tmp;

  disableXRAM();
  outp(reg, IDE_CTRL_PORT);
  cbi(IDE_CTRL_PORT, ideDIOR);
  _NOP();
  tmp = inp(PIN(IDE_DATAL_PORT));
  sbi(IDE_CTRL_PORT, ideDIOR);
  enableXRAM();

	return tmp;
}

