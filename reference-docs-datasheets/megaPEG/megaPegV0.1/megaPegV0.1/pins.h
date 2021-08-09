#ifndef PINS_H
  #define PINS_H


  // i2c port&pins
  #define prtI2C    PORTE
  #define pinSCL    PE2
  #define pinSDA    PE3

  // i2s port&pins
  #define prtI2S    PORTD
  #define pinSIC    PD5
  #define pinSID    PD6

  // ide port&pins
  #define prtDATAL  PORTA
  #define prtDATAH  PORTB
  #define prtCTRL   PORTC

  #define pinCS1    PC0
  #define pinDA2    PC1
  #define pinDA0    PC2
  #define pinDA1    PC3
  #define pinDIOR   PC4
  #define pinDIOW   PC5
  #define pinIDESEL PC7
  //old---------------------------------------
  #define ideCS1    PC0 // ide
  #define ideDA2    PC1
  #define ideDA0    PC2
  #define ideDA1    PC3
  #define ideDIOR   PC4
  #define ideDIOW   PC5
  #define ideSEL    PC7 //A15

  #define lcdCS2    PC0
	#define lcdCS1    PC1
  #define lcdRW     PC2
  #define lcdDI     PC3
  #define lcdENABLE PC6
  #define lcdSEL    PC7 //A15

  #define ideIRQ    PD0
  #define i2cIRQ    PD1
  #define LED       PD7

  #define RXD       PE0
	#define TXD				PE1
  #define masDEM    PE4
  #define rswIRQ    PE5
  #define rswSW     PE6
  #define IR        PE7

  #define masFSYNC  PINF0
  #define KEY1      PINF1
  #define KEY2      PINF2
  #define KEY3      PINF3
  #define KEY4      PINF4
  #define KEY5      PINF5
  #define rswB      PINF6
  //#define           PINF7
  //old---------------------------------------



#endif
