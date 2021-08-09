#ifndef MAS3570_H
  #define MAS3570_H

  void i2s_sendbyte(u08 data);
  void MAS_Init(void);
  void MAS_RegisterInfo(void);
  void MAS_WriteMem(u08 block, u16 addr, u32 data);
  u32  MAS_ReadMem(u08 block, u16 adrr);
  u32  MAS_ReadReg(u08 reg);
  void MAS_RunAt(u16 addr);
  void MAS_WriteReg(u08 reg, u16 data);

	#define	DAC3550				0x9A
	#define	MAS3507				0x3A

	#define	DAC_SAMPLE_RATE				0xC1
	#define	DAC_ANALOG_VOLUME			0xC2
	#define	DAC_GLOB_CONFIG				0xC3

	// registers..
	#define	MAS_DATA_WRITE				0x68
	#define MAS_DATA_READ					0x69
	#define	MAS_CONTROL						0x6a
	//	MAS register
	#define	MAS_REG_DCCF					0x8e
	#define	MAS_REG_MUTE					0xaa
	#define	MAS_REG_PIODATA				0xed
	#define	MAS_REG_PIODATA1			0xc8
	#define	MAS_REG_StartUpConfig	0xe6
	#define	MAS_REG_KPRESCALE			0xe7
	#define	MAS_REG_KBASS					0x6b
	#define	MAS_REG_KTREBLE				0x6f

	#define	WRITE_REG							0x90
	#define	WRITE_D0							0xA0
	#define	WRITE_D1							0xB0
	#define	READ_REG							0xD0
	#define	READ_D0								0xE0
	#define	READ_D1								0xF0
	#define	MAS_PLLOFFSET48				0x5D9D0  // 0,73135
	#define	MAS_PLLOFFSET44				0xCECE9  //-0,38432
#endif

