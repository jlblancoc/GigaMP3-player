#define PLAY_C
#include "equates.h"

void pump(void){
  while(bit_is_clear(PINE,masDEM)){
    i2s_sendbyte(*pBuffOUT++);
    if(pBuffOUT == pBuffEND) pBuffOUT = pBuffSTART;
    if(--mp3FileSize <= 0){              // try autoplay next song
      if(autoPlay == TRUE){
        file = FAT_NextFile(file);
        printf("\n\r>");
        FAT_FileInfo(file);
        select();
      }else{
        playState = PS_STOPPED;
        break;
      }
    }
  }
  curFreeBuff = (pBuffOUT > pBuffIN) ? (pBuffOUT - pBuffIN) : (pBuffEND - pBuffIN);
  if((curSecInCls) == fatSecPerClus){
    curCls = FAT_NextCls(curCls);
    curSec = FAT_cls2sec(curCls);
    curSecInCls = 0;
  }
  if(curFreeBuff >= (512 * PUMP_SECCNT)){
    u08 j;
    for(j = 0; j < PUMP_SECCNT; j++){
      ATAPI_ReadSectorX(curSec++, pBuffIN);
      pBuffIN += 512;
      curFreeBuff -= 512;
      curSecInCls++;
    }
    if(pBuffIN == pBuffEND) pBuffIN = pBuffSTART;
  }
}

void select(void){
  u08 j;

  if((file.attr & ATTR_DIRECTORY) == ATTR_DIRECTORY){
    printf("\n\r#");
    FAT_FileInfo(file);
    file = FAT_ChangeDir(file);
    printf("\n\r=");
    file = FAT_NextFile(file);
    FAT_FileInfo(file);
    uiEvent = EV_IDLE;
  }else{                                 // play mp3
    curCls = file.clus;
    curSec = FAT_cls2sec(curCls);
    pBuffIN = pBuffSTART;
    for(j=0; j<fatSecPerClus; j++){
      ATAPI_ReadSectorX(curSec++, pBuffIN);
      pBuffIN += 512;
    }
    curCls = FAT_NextCls(curCls);  // follow FAT chain
    curSec = FAT_cls2sec(curCls);
    curSecInCls = 0;
    pBuffIN = pBuffOUT = pBuffSTART;
    mp3FileSize = file.size;
    playState = PS_PAUSED;
    uiEvent = EV_PLAY;
  }
}
