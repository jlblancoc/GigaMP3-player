#define FAT_C
#include "equates.h"

u08 *pSecBuff = (u08 *)SEC_BUFFER;      // pointer to Sector Buffer
u08 *pFatBuff = (u08 *)FAT_BUFFER;      // pointer to FileAlocationTable Buffer
u08 *pTmpBuff = (u08 *)TMP_BUFFER;      // pointer to Temp LFN Buffer


void FAT_FileInfo(STRUCT_FILE_INFO file){
  #ifdef DEBUG_FILE
    printf("%20s ", file.name);
    printf("a:0x%x ", file.attr);
    printf("cls:%lu ", file.clus);
    printf("sz:%lu ", file.size);
    printf("dtsec:%lu ", file.dtSec);
    printf("dtde:%u\n\r", file.dtDE);
  #else
    printf(" %s", file.name);
  #endif
}

STRUCT_FILE_INFO FAT_ChangeDir(STRUCT_FILE_INFO dir){
  STRUCT_FILE_INFO nextDir;

  nextDir = dir;
  nextDir.dtSec = FAT_cls2sec(dir.clus) - 1;//FAT_cls2sec(dir.clus);
  nextDir.dtDE  = 15;//0;
  #ifdef DEBUG_FAT
    printf("\n\rCHANGE DIR\n\r");
    printf("in  dir.dtDE(%2u) .clus(%lu)\n\r", dir.dtDE, dir.clus);
    printf("out dir.dtDE(%2u) .dtSec(%lu)\n\r", nextDir.dtDE, nextDir.dtSec+1);
  #endif

  return nextDir;
}

STRUCT_FILE_INFO FAT_PrevFile(STRUCT_FILE_INFO file){
  struct STRUCT_DIR_ENTRY *pDirEnt = 0;
  struct STRUCT_LFN_ENTRY *pLfnEnt;
  STRUCT_FILE_INFO prevFile;
  u08 sfn[13];
  u08 lfnCnt;
  u08 sfnDE  = 0;
  u32 sfnSec = 0;
  u08 sfnIS = FALSE;
  u08 sfnOK = FALSE;
  u08 lfnOK = FALSE;
  u16 i;
  u08 *p;

  if((file.dtSec == fat1stDataSec) && !file.dtDE){
    return file;
  }else{
    if(file.dtDE){
      prevFile.dtSec = file.dtSec;
      prevFile.dtDE  = file.dtDE - 1;
    }else{
      prevFile.dtSec = file.dtSec - 1;
      prevFile.dtDE  = 15;
    }
  }

  #ifdef DEBUG_DIR
    printf("\n\r");
  #endif

  do{
    if(prevFile.dtSec != curInSecBuff){
      ATAPI_ReadSector(prevFile.dtSec, pSecBuff);
      curInSecBuff = prevFile.dtSec;
      #ifdef DEBUG_CURINBUF
        printf("  curInSecBuff(%lu)\n\r", curInSecBuff);
      #endif
    }
    pDirEnt = ((struct STRUCT_DIR_ENTRY *) pSecBuff) + prevFile.dtDE;
    pLfnEnt = (struct STRUCT_LFN_ENTRY *) pDirEnt;

    #ifdef DEBUG_DIR
      printf(" n<%x>a<%x>", pDirEnt->name[0], pDirEnt->attr);
    #endif

    if(pDirEnt->name[0] == DE_END) return file;                     // last DirEntry
    if(pDirEnt->name[0] == DE_FREE);                                // free DirEntry
    else if(*pDirEnt->name == '.'){          // dot & dotdot DirEntry
      if(pDirEnt->name[1] == '.'){      // dotdot
        prevFile.name[0] = '.';
        prevFile.name[1] = '.';
        prevFile.name[2] = 0;
        prevFile.clus = ((u32)pDirEnt->firstClusHI << 16) + pDirEnt->firstClusLO;
        prevFile.attr = pDirEnt->attr;
        prevFile.size = pDirEnt->fileSize;
        return prevFile;
      }
      else return file;             // dot
    }
    else if(sfnIS && (pDirEnt->attr == ATTR_LONG_NAME)){  // lfn
      lfnCnt = 13 * ((pLfnEnt->cnt - 1) & CNT_MASK);
      p = &pTmpBuff[lfnCnt];
      lfnOK = FALSE;
      for (i=0; i<10; i+=2) *p++ = pLfnEnt->name1[i]; // copy first part
      for (i=0; i<12; i+=2) *p++ = pLfnEnt->name2[i]; // second part
      for (i=0; i< 4; i+=2) *p++ = pLfnEnt->name3[i]; // and third part
      if((pLfnEnt->cnt & CNT_1stMASK)  == CNT_1stMASK){
        *p = 0;
        lfnOK = TRUE;
        sfnOK = TRUE;
        sfnIS = FALSE;
      }
    }
    else if (((pDirEnt->attr & 0x1f) == ATTR_FILE)||(pDirEnt->attr == ATTR_DIRECTORY)){ // dir or file
      if(!sfnIS){
        p = &sfn[0];
        for (i=0; i<8; i++){ if(pDirEnt->name[i] != ' ')    *p++ = pDirEnt->name[i]; }
        if((pDirEnt->attr & 0x1f) == ATTR_FILE) *p++ = '.';
        for (i=0; i<3; i++){ if(pDirEnt->nameExt[i] != ' ') *p++ = pDirEnt->nameExt[i]; }
        *p = '\0';
        prevFile.clus = ((u32)pDirEnt->firstClusHI << 16) + pDirEnt->firstClusLO;
        prevFile.attr = pDirEnt->attr;
        prevFile.size = pDirEnt->fileSize;
        sfnDE  = prevFile.dtDE;
        sfnSec = prevFile.dtSec;
        sfnIS  = TRUE;
        lfnOK  = FALSE;
      }
      else sfnOK = TRUE;
    }
    else if(sfnIS) sfnOK = TRUE;

    if(lfnOK && sfnOK){
      lfnOK = FALSE;
      for (i=0; i<30; i++) prevFile.name[i] = pTmpBuff[i];
      prevFile.name[30] = 0;
      prevFile.dtDE = sfnDE;
      prevFile.dtSec = sfnSec;

      return prevFile;
    }
    else if(sfnOK){
      sfnOK = FALSE;
      for (i=0; i<13; i++) prevFile.name[i] = sfn[i];
      prevFile.dtDE = sfnDE;
      prevFile.dtSec = sfnSec;

      return prevFile;
    }
    else lfnOK = FALSE;

    if(prevFile.dtDE == 0){
      prevFile.dtSec--;
      prevFile.dtDE = 15;
    }else{
      prevFile.dtDE--;
      pDirEnt--;
    }
  } while (prevFile.dtSec >= fat1stDirSec);
  return file;
}

STRUCT_FILE_INFO FAT_NextFile(STRUCT_FILE_INFO file){
  struct STRUCT_DIR_ENTRY *pDirEnt = 0;
  struct STRUCT_LFN_ENTRY *pLfnEnt;
  STRUCT_FILE_INFO nextFile;
  u08 lfnCnt;
  u08 lfnOK = FALSE;
  u16 i;
  u08 *p;

  if(file.dtSec == 0){
    nextFile.dtSec = fat1stDirSec;
    nextFile.dtDE  = 0;
  }else{
    nextFile.dtSec = file.dtSec;
    nextFile.dtDE  = file.dtDE + 1;
  }

    #ifdef DEBUG_DIR
      printf("\n\r");
    #endif
  do{
    if(nextFile.dtDE == 16){
      nextFile.dtSec++;
      nextFile.dtDE = 0;
    }

    if(nextFile.dtSec != curInSecBuff){
      ATAPI_ReadSector(nextFile.dtSec, pSecBuff);
      curInSecBuff = nextFile.dtSec;
      #ifdef DEBUG_CURINBUF
        printf("  curInSecBuff(%lu)\n\r", curInSecBuff);
      #endif
    }
    pDirEnt = ((struct STRUCT_DIR_ENTRY *) pSecBuff) + nextFile.dtDE;
    pLfnEnt = (struct STRUCT_LFN_ENTRY *) pDirEnt;

    #ifdef DEBUG_DIR
      printf(" n<%x>a<%x>", pDirEnt->name[0], pDirEnt->attr);
    #endif

    if(pDirEnt->name[0] == DE_END) return file;                     // last DirEntry
    if(pDirEnt->name[0] == DE_FREE);                                // free DirEntry
    else if(pDirEnt->name[0] == '.'){   // dot DirEntry
      if(pDirEnt->name[1] == '.'){      // dotdot DirEntry
        nextFile.name[0] = '.';
        nextFile.name[1] = '.';
        nextFile.name[2] = 0;
        nextFile.clus = ((u32)pDirEnt->firstClusHI << 16) + pDirEnt->firstClusLO;
        nextFile.attr = pDirEnt->attr;
        nextFile.size = pDirEnt->fileSize;

        return nextFile;
      }
    }
    else if(pDirEnt->attr == ATTR_LONG_NAME){  // lfn
      lfnCnt = 13 * ((pLfnEnt->cnt - 1) & CNT_MASK);
      p = &pTmpBuff[lfnCnt];
      lfnOK = FALSE;
      for (i=0; i<10; i+=2) *p++ = pLfnEnt->name1[i]; // copy first part
      for (i=0; i<12; i+=2) *p++ = pLfnEnt->name2[i]; // second part
      for (i=0; i< 4; i+=2) *p++ = pLfnEnt->name3[i]; // and third part
      if((pLfnEnt->cnt & CNT_1stMASK)  == CNT_1stMASK) *p = 0;
      if((pLfnEnt->cnt & ~CNT_1stMASK) == CNT_LAST)    lfnOK = TRUE;
    }
    else if (((pDirEnt->attr & 0x1f) == ATTR_FILE)||(pDirEnt->attr == ATTR_DIRECTORY)){ // dir or file
      if(!lfnOK){
        p = &pTmpBuff[0];
        for (i=0; i<8; i++){ if(pDirEnt->name[i] != ' ')    *p++ = pDirEnt->name[i]; }
        if((pDirEnt->attr & 0x1f) == ATTR_FILE) *p++ = '.';
        for (i=0; i<3; i++){ if(pDirEnt->nameExt[i] != ' ') *p++ = pDirEnt->nameExt[i]; }
        *p = '\0';
      }
      for (i=0; i<30; i++) nextFile.name[i] = pTmpBuff[i];
      nextFile.name[30] = 0;
      nextFile.clus = ((u32)pDirEnt->firstClusHI << 16) + pDirEnt->firstClusLO;
      nextFile.attr = pDirEnt->attr;
      nextFile.size = pDirEnt->fileSize;

      return nextFile;
    }
    nextFile.dtDE++;
    pDirEnt++;

  } while ((pDirEnt->name[0] != DE_END) || (nextFile.dtDE == 16));
  return file;
}

u08 FAT_Init(void)
    {
      u32 MBR_BootSec;
      u32 RootDirSec;
      u32 fatSz;

      struct STRUCT_PART_REC *pPR;
      struct STRUCT_BPB      *pBPB;
      struct STRUCT_BPB16    *pBPB16;
      struct STRUCT_BPB32    *pBPB32;

      curInSecBuff  = 0;
      ATAPI_ReadSector(0, pSecBuff);

      pPR = (struct STRUCT_PART_REC *) ((struct STRUCT_PART_SEC *) pSecBuff)->Part;

      MBR_BootSec = pPR->StartLBA;

      #ifdef DEBUG_FAT
        printf("\n\rFAT_INIT ...\n\r");
        printf("Partition Type : ");

        switch(pPR->PartType){
          case PART_TYPE_FAT16 :
            printf("FAT16 >32MB\n\r");
            break;
          case PART_TYPE_FAT32 :
            printf("FAT32 <2048GB\n\r");
            break;
          case PART_TYPE_FAT16LBA :
            printf("FAT16 LBA <32MB\n\r");
            break;
          case PART_TYPE_FAT32LBA :
            printf("FAT32 LBA <2048GB\n\r");
            break;
          default :
            printf("fat type unknown 0x%x ???\n\r", SEC_BUFFER[0x01be + 0x04]);
        }

        printf("MBR_Boot Sec   : %lu\n\r", MBR_BootSec);
        printf("No. Of Sectors : %lu\n\r", pPR->Size);
      #endif

      ATAPI_ReadSector(MBR_BootSec, pSecBuff);

      pBPB   = (struct STRUCT_BPB *)   ((struct STRUCT_BOOT_SEC *) pSecBuff)->BPB;
      pBPB16 = (struct STRUCT_BPB16 *) ((struct STRUCT_BOOT_SEC *) pSecBuff)->BPBext;
      pBPB32 = (struct STRUCT_BPB32 *) ((struct STRUCT_BOOT_SEC *) pSecBuff)->BPBext;

      fatSecPerClus = pBPB->SecPerClus;


        switch(fatSecPerClus){
          case   2 : fatSPCshift =  1; break;
          case   4 : fatSPCshift =  2; break;
          case   8 : fatSPCshift =  3; break;
          case  16 : fatSPCshift =  4; break;
          case  32 : fatSPCshift =  5; break;
          case  64 : fatSPCshift =  6; break;
          case 128 : fatSPCshift =  7; break;
          default  :
            #ifdef DEBUG_FAT
              printf("!!! ERROR >>> fatSecPerCls %d !!!\n\r", fatSecPerClus);
            #endif
            break;
        }

      fatBytsPerSec = pBPB->BytsPerSec;
      RootDirSec = ((pBPB->RootEntCnt * 32) + (fatBytsPerSec - 1)) / pBPB->BytsPerSec;
      //  ???????? rounds up

      if(pBPB->FATSz16 != 0){
        fatSz   = pBPB->FATSz16;
        fatType = FAT16;
      }else{
        fatSz    = pBPB32->FATSz32;
        fatType = FAT32;
      }

      fat1stFATSec  = MBR_BootSec   + pBPB->RsvdSecCnt;
      fat1stDirSec  = fat1stFATSec + (pBPB->NumFATs * fatSz);
      fat1stDataSec = fat1stDirSec + RootDirSec;

      #ifdef DEBUG_FAT
        printf("\n\rBOOT RECORD ...\n\r");
        printf("BytsPerSec : %u\n\r", fatBytsPerSec);
        printf("SecPerClus : %u\n\r", fatSecPerClus);
        printf("fatSPCshift: %u\n\r", fatSPCshift);
        printf("RsvdSecCnt : %u\n\r", pBPB->RsvdSecCnt);
        printf("NumFATs    : %u\n\r", pBPB->NumFATs);
        printf("RootEntCnt : %u\n\r", pBPB->RootEntCnt);
        printf("TotSec32   : %u\n\r", pBPB->TotSec32);
        printf("RootDirSec : %lu\n\r", RootDirSec);
        printf("fatSz      : %lu\n\r", fatSz);
        printf("1stDirSec  : %lu\n\r", fat1stDirSec);
        printf("1stDataSec : %lu\n\r", fat1stDataSec);
      #endif

      return TRUE;
    }

// find next cluster in the FAT table
u32 FAT_NextCls(u32 cluster){
  u32 nextCls       = cluster << fatType;
  u32 ThisFATSecNum    = fat1stFATSec + (nextCls / fatBytsPerSec);
  u16 ThisFATEntOffset = nextCls % fatBytsPerSec;

  if (ThisFATSecNum != curInFatBuff){
    ATAPI_ReadSector(ThisFATSecNum, pFatBuff);
    curInFatBuff = ThisFATSecNum;
  }

  if (fatType == FAT16) {
    nextCls = (*((u16*)  &pFatBuff[ThisFATEntOffset]));
    if (nextCls >= 0xfff8) nextCls = 0;     // EndOffClusterchain
	}else{
    nextCls = (*((u32*) &pFatBuff[ThisFATEntOffset])) & FAT32_MASK;
    if (nextCls >= 0x0ffffff8) nextCls = 0; // EndOffClusterchain
	}
  return nextCls;
}

u32 FAT_cls2sec(u32 cluster){
  return cluster ? (((cluster - 2) << fatSPCshift) + fat1stDataSec) : fat1stDataSec;
}
