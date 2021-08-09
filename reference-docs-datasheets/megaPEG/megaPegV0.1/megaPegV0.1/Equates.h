#ifndef EQUATES_H
  #define EQUATES_H

  //#define DEBUG_IDE
  //#define DEBUG_MAS
  //#define DEBUG_FAT
  //#define DEBUG_CURINBUF
  //#define DEBUG_DIR
  //#define DEBUG_LFN

  //#define DEBUG_FILE

//********************************
  #include <io.h>
  #include "typedef.h"
  #include "global.h"
  #include "lib/ap_uart.h"
  #include "lib/printf_P.h"
  #include "lib/i2cmaster.h"
  #include "pins.h"
  #include "init.h"
  #include "math.h"
  #include "delay.h"
  #include "ide.h"
  #include "atapi.h"
  #include "fat.h"
  #include "rtc8563.h"
  #include "mas3570.h"
  #include "dac3550.h"
  #include "play.h"

  typedef enum {
		PS_STOPPED,
		PS_PLAYING,
		PS_PAUSED
	} playstate_e;

  typedef enum {
		EV_IDLE,
		EV_PLAY,
		EV_STOP,
		EV_NEXT,
		EV_PREV,
    EV_UP,
    EV_DOWN,
		EV_NEUTRAL,
		EV_MUTE,
    EV_MENU,
    EV_OK,
		EV_LASTEVENT
	} event_e;

  playstate_e   playState;
  event_e       uiEvent;
  u08 uiCur;
  u08 uiRot;
  u08 uiRotSw;
  u08 uiKey;
  u08 uiKey;
  u08 autoPlay;
//********************************
  //#define FCLK     10000000ul         // define in ap_uart.h
  //#define UART_BAUD    9600ul         // define in ap_uart.h

//********************************
  #define F_CPU       10000000ul

//*** external memory organization ***
  #define XRAM_START  0x1000
  #define XRAM_END    0x7FFF


  #define enableXRAM()      _SEI();sbi(MCUCR, SRE)
  #define disableXRAM()     cbi(MCUCR, SRE);_CLI()

//*** internal memory organization ***
  //u08 MP3_BUFFER[512];
  u08 SEC_BUFFER[512];
  u08 FAT_BUFFER[512];
  u08 TMP_BUFFER[256];
  u08 I2C_BUFFER[10];

  #define PLAY_XBUFFER  XRAM_START
  #define PUMP_SECCNT   16               // (BPB_SecPerClus)32 : 8 = 4

  u08 ideDev;

  u08 fatType;
  u08 fatSecPerClus;
  u08 fatSPCshift;
  u16 fatBytsPerSec;
  u32 fat1stFATSec;
  u32 fat1stDirSec;
  u32 fat1stDataSec;

  u32 curInSecBuff;
  u32 nxtInSecBuff;
  u32 curInFatBuff;
  u32 nxtInFatBuff;
  STRUCT_FILE_INFO file;

  u08 *pBuffSTART;
  u08 *pBuffEND;
  u08 *pBuffIN;
  u08 *pBuffOUT;

  u16 curFreeBuff;
  u08 curSecInCls;
  u32 curSec;
  u32 curCls;
  u32 mp3FileSize;





//********************************
  #define KEY_5     0x20
	#define KEY_4 		0x10
	#define KEY_3 		0x08
	#define KEY_2 		0x04
	#define KEY_1 		0x02

#endif

