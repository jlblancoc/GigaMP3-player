#define RTC8563_C
#include "equates.h"

void rtc_init(void){
  i2c_start(RTC8563+I2C_WRITE);             // set device address and write mode
  i2c_write(RTC_CTRL1);                     // set address
  i2c_write(rtcSTART);                      // write data to address
  i2c_stop();                               // set stop conditon = release bus
  printf("RTC8563\n\r");
}

void ShowDateTime(void){
	GetRTC();
  switch(rtc.weekday) {
    case 0 : printf("NED"); break;
    case 1 : printf("PON"); break;
    case 2 : printf("TOR"); break;
    case 3 : printf("SRE"); break;
    case 4 : printf("CET"); break;
    case 5 : printf("PET"); break;
    case 6 : printf("SOB"); break;
  }
  printf(",%d.%d.%d,%d:%d:%d\n\r\n\r", rtc.day, rtc.month, rtc.year+2000, rtc.hours, rtc.minutes, rtc.seconds);
}

void GetRTC(void){
  u08 i;

  i2c_start_wait(RTC8563+I2C_WRITE);        // set device address and write mode
  i2c_write(RTC_SEC);                       // write address = 0
  i2c_rep_start(RTC8563+I2C_READ);          // set device address and read mode
  for (i = 0; i < 6; i++) I2C_BUFFER[i] = i2c_read(I2C_ACK);  // read 5 bytes
  I2C_BUFFER[6] = i2c_read(I2C_NACK);       //  read 6th byte
  i2c_stop();                               // set stop condition = release bus

  rtc.hundredths= 0;
  rtc.seconds   = 10 * (I2C_BUFFER[0] >> 4 & 0x07) + (I2C_BUFFER[0] & 0x0F);
  rtc.minutes   = 10 * (I2C_BUFFER[1] >> 4 & 0x07) + (I2C_BUFFER[1] & 0x0F);
  rtc.hours     = 10 * (I2C_BUFFER[2] >> 4 & 0x03) + (I2C_BUFFER[2] & 0x0F);
  rtc.day       = 10 * (I2C_BUFFER[3] >> 4 & 0x03) + (I2C_BUFFER[3] & 0x0F);
  rtc.weekday   = I2C_BUFFER[4] & 0x07;
  rtc.month     = 10 * (I2C_BUFFER[5] >> 4 & 0x01) + (I2C_BUFFER[5] & 0x0F);
  rtc.year      = 10 * (I2C_BUFFER[6] >> 4)        + (I2C_BUFFER[6] & 0x0F);
}

void PutRTC(void){
  u08 i;

  // stop RTC
  i2c_start(RTC8563+I2C_WRITE);             // set device address and write mode
  i2c_write(RTC_CTRL1);                      // set address
  i2c_write(rtcSTOP);                       // write data to address
  i2c_stop();                               // set stop conditon = release bus

  I2C_BUFFER[0]  = (rtc.seconds / 10)<<4 | (rtc.seconds %10);
  I2C_BUFFER[1]  = (rtc.minutes / 10)<<4 | (rtc.minutes %10);
  I2C_BUFFER[2]  = (rtc.hours   / 10)<<4 | (rtc.hours   %10);
  I2C_BUFFER[3]  = (rtc.day     / 10)<<4 | (rtc.day     %10);
  I2C_BUFFER[4]  = (rtc.weekday / 10)<<4 | (rtc.weekday %10);
  I2C_BUFFER[5]  = ((rtc.month  / 10)<<4 | (rtc.month   %10));// | 0x80);
  I2C_BUFFER[6]  = (rtc.year    / 10)<<4 | (rtc.year    %10);

  // set RTC
  i2c_start(RTC8563+I2C_WRITE);             // set device address and write mode
  i2c_write(RTC_SEC);                      // set address
  for(i=0;i<=6;i++) i2c_write(I2C_BUFFER[i]); // write data to address
  i2c_stop();                               // set stop conditon = release bus

  // start RTC
  i2c_start(RTC8563+I2C_WRITE);             // set device address and write mode
  i2c_write(RTC_CTRL1);                      // set address
  i2c_write(rtcSTART);                          // write data to address
  i2c_stop();                               // set stop conditon = release bus
}
