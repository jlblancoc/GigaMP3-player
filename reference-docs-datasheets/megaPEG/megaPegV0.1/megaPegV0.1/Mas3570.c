#define MAS3570_C
#include "equates.h"

void i2s_sendbyte(u08 data){
  u08 i;

  for (i = 0;i < 8;i++){
    if (data & 0x80) sbi(prtI2S,pinSID); else cbi(prtI2S,pinSID);
    data <<= 1;
    sbi(prtI2S,pinSIC);
    cbi(prtI2S,pinSIC);
  }
}

void MAS_Init(void){
  u32 ltmp;

  printf("MAS3507D ");

  sbi(DDR(prtI2S),pinSIC);              // init I2S
  sbi(DDR(prtI2S),pinSID);

  ltmp = MAS_ReadMem(READ_D1,0x0ff7);
  if((u16)ltmp == 0x0501){              // version G10
    printf("G10\n\r");
    MAS_WriteMem(WRITE_D0,0x36D,MAS_PLLOFFSET48);
    MAS_RunAt(0x0475);
    MAS_WriteMem(WRITE_D0,0x36E,MAS_PLLOFFSET44);
    MAS_RunAt(0x0475);
    MAS_WriteReg(0x3B,0x0020);
    MAS_RunAt(0x0001);
  }
  else if((u16)ltmp == 0x0601){         // version F10
    printf("F10\n\r");
    MAS_WriteMem(WRITE_D0,0x32D,MAS_PLLOFFSET48);
    MAS_RunAt(0x0FCB);
    MAS_WriteMem(WRITE_D0,0x32E,MAS_PLLOFFSET44);
    MAS_RunAt(0x0FCB);
  }

  #ifdef DEBUG_MAS
    printf("INFO ...\n\r");
    ltmp = MAS_ReadMem(READ_D1,0x0ff6);
    printf("name       : %4x\n\r", (u16)ltmp);
    ltmp = MAS_ReadMem(READ_D1,0x0ff7);
    printf("design code: %4x\n\r", (u16)ltmp);
    ltmp = MAS_ReadMem(READ_D1,0x0ff8);
    printf("date       : %x.%x.%x\n\r", (u08)(ltmp>>16), (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0ff9);
    printf("description: %c%c", (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0ffa);
    printf("%c%c", (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0ffb);
    printf("%c%c", (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0ffc);
    printf("%c%c", (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0ffd);
    printf("%c%c", (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0ffe);
    printf("%c%c", (u08)(ltmp>>8), (u08)(ltmp));
    ltmp = MAS_ReadMem(READ_D1,0x0fff);
    printf("%c%c\n\r", (u08)(ltmp>>8), (u08)(ltmp));
    MAS_RegisterInfo();
  #endif
}

void MAS_RegisterInfo(void){
  u32 ltmp;

  printf("\n\rMAS3507D REGISTER INFO ...\n\r");

  ltmp = MAS_ReadReg(MAS_REG_DCCF);
  printf("DDCF      : 0x %6x\n\r",ltmp);

  ltmp = MAS_ReadReg(MAS_REG_MUTE);
  printf("MUTE      : ");
  if (ltmp==0)      printf("tone control active\n\r");
  else if (ltmp==1) printf("mute out, continue decoding\n\r");
  else if (ltmp==2) printf("bypass bass/treble/volume matrix\n\r");
  else              printf("error\n\r");

  ltmp = MAS_ReadReg(MAS_REG_PIODATA);
  printf("PIODATA   : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadReg(MAS_REG_PIODATA1);
  printf("PIODATA1  : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadReg(MAS_REG_StartUpConfig);
  printf("STARTUP   : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadReg(MAS_REG_KPRESCALE);
  printf("K PRESC   : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadReg(MAS_REG_KBASS);
  printf("K BASS    : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadReg(MAS_REG_DCCF);
  printf("K TREBLE  : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadReg(0x3b);
  printf("reg 0x3b  : 0x %6x\n\r", ltmp);
  ltmp = MAS_ReadMem(READ_D1,0x036f);
  printf("OutputCfg : 0x %6x\n\r", ltmp);
}

u32 convert_mem(void){
  return ((u32) (I2C_BUFFER[3] & 0x0F) << 16) | ((u16)(I2C_BUFFER[0] << 8)) | I2C_BUFFER[1];
}


void MAS_WriteMem(u08 block, u16 addr, u32 data){
  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_WRITE);                // set address
  i2c_write(block);
  i2c_write(0x00);
  i2c_write(0x00);
  i2c_write(0x01);
  i2c_write(high(addr));
  i2c_write(low(addr));
  i2c_write(data >> 8);
  i2c_write(data);
  i2c_write(0x00);
  i2c_write((data >> 16) & 0x0F);
  i2c_stop();
}

u32 MAS_ReadMem(u08 block, u16 addr){
  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_WRITE);                // set address
  i2c_write(block);
  i2c_write(0x00);
  i2c_write(0x00);
  i2c_write(0x01);
  i2c_write(high(addr));
  i2c_write(low(addr));
  i2c_stop();

  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_READ);                // set address
  i2c_rep_start(MAS3507 + I2C_READ);          // set device address and read mode
  I2C_BUFFER[0] = i2c_read(I2C_ACK);
  I2C_BUFFER[1] = i2c_read(I2C_ACK);
  I2C_BUFFER[2] = i2c_read(I2C_ACK);
  I2C_BUFFER[3] = i2c_read(I2C_NACK);
  i2c_stop();

  return convert_mem();
}

void MAS_RunAt(u16 addr){
  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_WRITE);                // set address
  i2c_write(addr >> 8);                     // write high data to address
  i2c_write(addr);                          // write low data to address
  i2c_stop();                               // set stop conditon = release bus
}

u32 MAS_ReadReg(u08 reg){
  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_WRITE);                // set address
  i2c_write(READ_REG | (reg >> 4));
  i2c_write(reg << 4);
  i2c_stop();

  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_READ);                // set address
  i2c_rep_start(MAS3507 + I2C_READ);          // set device address and read mode
  I2C_BUFFER[0] = i2c_read(I2C_ACK);
  I2C_BUFFER[1] = i2c_read(I2C_ACK);
  I2C_BUFFER[2] = i2c_read(I2C_ACK);
  I2C_BUFFER[3] = i2c_read(I2C_NACK);
  i2c_stop();

  return convert_mem();
}

void MAS_WriteReg(u08 reg, u16 data){
  i2c_start(MAS3507 + I2C_WRITE);           // set device address and write mode
  i2c_write(MAS_DATA_WRITE);                // set address
  i2c_write(WRITE_REG | (reg >> 4));
  i2c_write((reg << 4) | (u08)(data & 0x0F));
  i2c_write(data >> 12);
  i2c_write(data >> 4);
  i2c_stop();
}

