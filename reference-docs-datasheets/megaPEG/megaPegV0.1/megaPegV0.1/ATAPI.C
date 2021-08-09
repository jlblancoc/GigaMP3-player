#define ATAPI_C
#include "equates.h"

u08 ATAPI_ReadSector(u32 lba, u08 *internal_buffer)
    {
      u16 tmp;
      u08 *pB = (u08 *)internal_buffer;

      IDE_WriteReg(REG_SEC_CNT, 1);
      tmp = lba & 0x0000ffff;
      IDE_WriteReg(REG_SEC_NUM, low(tmp));
      IDE_WriteReg(REG_CYL_LOW, high(tmp));
      tmp = (lba & 0xffff0000) >> 16;
      IDE_WriteReg(REG_CYL_HIGH, low(tmp));
      IDE_WriteReg(REG_DEV_HEAD, (DH_LBA | ideDev | (high(tmp) & 0x0f)));
      IDE_WriteReg(REG_COMMAND, ATA_READ_SECTOR_S);
                                                        // | ST_DRQ
      if(IDE_WaitState((ST_DRDY | ST_DRQ), (ST_BSY | ST_DF | ST_ERR)) == 0) return FALSE;

      disableXRAM();
      outp(REG_DATA, IDE_CTRL_PORT);
      for(tmp = 0; tmp < 256; tmp++){
        cbi(IDE_CTRL_PORT, ideDIOR);
        _NOP();
        *pB++ = inp(PIN(IDE_DATAL_PORT));
        *pB++ = inp(PIN(IDE_DATAH_PORT));
        sbi(IDE_CTRL_PORT, ideDIOR);
      }
      enableXRAM();
      tmp = IDE_WaitState(ST_DRDY , ST_BSY);

      return (tmp ?  TRUE : FALSE);
    }

u08 ATAPI_ReadSectorX(u32 lba, u08 *external_buffer)
    {
      u08 dl,dh;
      u16 tmp;
      u08 *pB = (u08 *)external_buffer;

      IDE_WriteReg(REG_SEC_CNT, 1);
      tmp = lba & 0x0000ffff;
      IDE_WriteReg(REG_SEC_NUM, low(tmp));
      IDE_WriteReg(REG_CYL_LOW, high(tmp));
      tmp = (lba & 0xffff0000) >> 16;
      IDE_WriteReg(REG_CYL_HIGH, low(tmp));
      IDE_WriteReg(REG_DEV_HEAD, (DH_LBA | ideDev | (high(tmp) & 0x0f)));
      IDE_WriteReg(REG_COMMAND, ATA_READ_SECTOR_S);
                                                        // | ST_DRQ
      if(IDE_WaitState((ST_DRDY | ST_DRQ), (ST_BSY | ST_DF | ST_ERR)) == 0) return FALSE;

      disableXRAM();
      outp(REG_DATA, IDE_CTRL_PORT);
      for(tmp = 0; tmp < 256; tmp++){
        disableXRAM();
        cbi(IDE_CTRL_PORT, ideDIOR);
        _NOP();
        dl = inp(PIN(IDE_DATAL_PORT));
        dh = inp(PIN(IDE_DATAH_PORT));
        sbi(IDE_CTRL_PORT, ideDIOR);
        enableXRAM();
        *pB++ = dl;
        *pB++ = dh;
      }
      tmp = IDE_WaitState(ST_DRDY , ST_BSY);

      return (tmp ?  TRUE : FALSE);
    }

u08 ATAPI_SelectDevice(u08 dev)
    {
      u08 i = 0xff;
      ideDev = dev;

      do{
          Delay_10ms(100);
          IDE_WriteReg(REG_DEV_HEAD, ideDev);
        } while ((IDE_WaitState(ST_DRDY, (ST_BSY | ST_DRQ)) == 0) && (--i != 0));

      printf("\n\rSELECT IDE DEV%d ", (ideDev ? 1 : 0));
      if(i==0) {printf(" ... NOK\n\r"); return TRUE;}
      else {printf(" ... OK\n\r"); return FALSE;}

      return TRUE;
    }
