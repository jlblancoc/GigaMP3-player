// -----------------------------------------------------------------------
// megaPeg®grega (grega@silon.si)
//
// v0.1 (8.1.2002 11:55)
// - hd version w/o irq   ...ok
// - play, pause, stop    ...ok
// - next, prev           ...ok
// - dir in, out          ...ok
// - rtc                  ...ok
//
// -----------------------------------------------------------------------

#include "equates.h"

event_e ReadKey(void)
{
	unsigned char i	= ~(inp(PINF) | 0xC1);
  unsigned char A = bit_is_set(PINE,rswIRQ);
  unsigned char B = bit_is_set(PINF,rswB);
  unsigned char C = bit_is_set(PINE,rswSW);

  uiEvent = EV_IDLE;
  if(A != uiRot){
    uiEvent = A ? (B ? EV_DOWN : EV_UP) : (B ? EV_UP : EV_DOWN);
    uiRot   = A;
	}
  else if(C != uiRotSw){
    uiEvent = C ? EV_MENU : EV_IDLE;
    uiRotSw = C;
  }
  else if(uiKey != i){
		switch (i){
      case KEY_1 : uiEvent = EV_PREV;  break;
      case KEY_2 : uiEvent = EV_PLAY;  break;
      case KEY_3 : uiEvent = EV_STOP;  break;
      case KEY_4 : uiEvent = EV_NEXT;  break;
      case KEY_5 : uiEvent = EV_OK;    break;
		}
    uiKey = i;
	}
  return uiEvent;
}

int main(void)
    {
      uiCur     = 1;
      uiKey     = 0;
      uiRot     = bit_is_set(PINE,rswIRQ);
      uiRotSw   = bit_is_set(PINE,rswSW);
      uiEvent   = EV_IDLE;

      megaPEG_init();
      printf("\n\rmegaPEG ®grega\n\r");
      printf("v0.1 (%s, %s)\n\r\n\r", __DATE__, __TIME__);
      MAS_Init();
      DAC_Init();
      rtc_init();
      ShowDateTime();

      ideDev = IDE_DEV0;
      ATAPI_SelectDevice(ideDev);
      FAT_Init();
      printf("\n\rDIR TREE ...\n\r",file.name);
      file.dtSec = 0;
      file = FAT_NextFile(file);
      printf("\n\r>");
      FAT_FileInfo(file);

      pBuffSTART = (u08 *)PLAY_XBUFFER;
      pBuffEND   = (u08 *)PLAY_XBUFFER + fatSecPerClus * 512;
      pBuffIN    = pBuffOUT = pBuffSTART;
      autoPlay  = TRUE;
      playState = PS_STOPPED;


      for(;;){
        if (playState == PS_PLAYING) pump();

        if (uiEvent == EV_IDLE)  uiEvent = ReadKey();
        switch (uiEvent){
          case EV_IDLE:
            break;
          case EV_PREV:
            file = FAT_PrevFile(file);
            printf("\n\r<");
            FAT_FileInfo(file);
            if((file.attr & 0x10) == 0x00) uiEvent = EV_OK;
            else uiEvent = EV_IDLE;
            break;
          case EV_NEXT:
            file = FAT_NextFile(file);
            printf("\n\r>");
            FAT_FileInfo(file);
            if((file.attr & 0x10) == 0x00) uiEvent = EV_OK;
            else uiEvent = EV_IDLE;
            break;
          case EV_PLAY:
            switch (playState){
              case PS_PAUSED :
                //ui_Play();
                playState = PS_PLAYING;
                uiEvent = EV_IDLE;
                break;
              case PS_PLAYING :
                //ui_Pause();
                playState = PS_PAUSED;
                uiEvent = EV_IDLE;
                break;
              case PS_STOPPED :
                //ui_Play();
                if((file.attr & 0x10) == 0x00) uiEvent = EV_OK;
                else uiEvent = EV_IDLE;
                break;
            }
            break;
          case EV_MENU:
          case EV_OK:
            select();
            break;
          case EV_UP:
            file = FAT_PrevFile(file);
            printf("\n\r<");
            FAT_FileInfo(file);
            uiEvent   = EV_IDLE;
            break;
          case EV_DOWN:
            file = FAT_NextFile(file);
            printf("\n\r>");
            FAT_FileInfo(file);
            uiEvent   = EV_IDLE;
            break;
          case EV_STOP:
            playState = PS_STOPPED;
          default:
            uiEvent   = EV_IDLE;
            break;
        }
      }//for(;;)
      return 0 ;
		} //main()
