#define DAC3550_C
#include "equates.h"

void DAC_Init(void){
  printf("DAC3550A\n\r");
  DAC_WriteReg8(DAC_GLOB_CONFIG,0x5C);
  DAC_Volume(33);
}

u08 DAC_Volume(u08 vol){
  if(vol > 56) vol = 56;
  else if(vol < 18) vol = 18;
  DAC_WriteReg16(DAC_ANALOG_VOLUME,vol,vol);
  return vol;
}

void DAC_WriteReg8(u08 reg, u08 data){
  i2c_start(DAC3550 + I2C_WRITE);           // set device address and write mode
  i2c_write(reg);                           // set address
  i2c_write(data);                          // write data to address
  i2c_stop();                               // set stop conditon = release bus
}

void DAC_WriteReg16(u08 reg, u08 dataH, u08 dataL){
  i2c_start(DAC3550 + I2C_WRITE);           // set device address and write mode
  i2c_write(reg);                           // set address
  i2c_write(dataH);                         // write high data to address
  i2c_write(dataL);                         // write low data to address
  i2c_stop();                               // set stop conditon = release bus
}
