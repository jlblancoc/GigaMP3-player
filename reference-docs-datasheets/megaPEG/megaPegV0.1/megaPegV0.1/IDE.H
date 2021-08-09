#ifndef IDE_H
	#define IDE_H

  u08 IDE_WaitState(u08 set, u08 clr);
	u16 IDE_ReadWord(void);
	u08 IDE_ReadReg(u08 reg);
	void IDE_WriteReg(u08 reg, u08 data);
  void IDE_WriteWord(u16 data);

  #define IDE_DATAL_PORT  PORTA
  #define IDE_DATAH_PORT  PORTB
  #define IDE_CTRL_PORT   PORTC

  #define IDE_DEV0     0x00
  #define IDE_DEV1     0x10

  #define REG_ALT_STAT 0xBA
  #define REG_DEV_CTRL 0xBA
  #define REG_STATUS   0xBF
  #define REG_COMMAND  0xBF
  #define REG_ERROR    0xB5
  #define REG_FEATURES 0xB5
  #define REG_CYL_HIGH 0xB7
  #define REG_CYL_LOW  0xB3
  #define REG_DATA     0xB1
  #define REG_DEV_HEAD 0xBB
  #define REG_SEC_CNT  0xB9
  #define REG_SEC_NUM  0xBD

  // REG_DEVICE_CONTROL bits def.
	#define DC_SRST			0x04	//2
	#define	DC_nIEN			0x02	//1
  // REG_STATUS bits def.
	#define	ST_BSY			0x80	//7
	#define	ST_DRDY			0x40	//6
	#define	ST_DF				0x20	//5
	#define ST_DSC			0x10	//4 atapi
	#define	ST_DRQ			0x08	//3
	#define ST_CORR			0x04	//2 atapi
	#define	ST_ERR			0x01	//0
	#define	ST_CHK			0x01	//0 atapi
  // REG_ERROR  bits def.
	#define ER_SENSE		0xF0	//7,6,5,4 atapi
	#define	ER_MCR			0x08	//3
	#define	ER_ABRT			0x04	//2
	#define	ER_EOM			0x02	//1 atapi
	#define	ER_ILI			0x01	//0 atapi
  // REG_DEVICE_HEAD bits def.
	#define	DH_DEV			0x10	//4
	#define	DH_DRV			0x10	//4 atapi
	#define	DH_LBA			0x40	//6
  // REG_SECTOR_COUNT bits def.
	#define	SC_RELEASE	0x04	//2 atapi
	#define	SC_IO				0x02	//1 atapi
	#define	SC_CoD			0x01	//0 atapi



#endif

