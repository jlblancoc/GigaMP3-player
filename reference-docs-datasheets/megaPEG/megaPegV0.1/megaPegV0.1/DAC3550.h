#ifndef DAC3550_H
  #define DAC3550_H

  void DAC_Init(void);
  u08  DAC_Volume(u08 vol);
  void DAC_WriteReg8(u08 reg, u08 data);
  void DAC_WriteReg16(u08 reg, u08 dataH, u08 dataL);
  /*
	void dac_WriteReg16(unsigned char reg, unsigned char dataH, unsigned char dataL);
  */
	#define	DAC3550				0x9A

	#define	DAC_SAMPLE_RATE				0xC1
	#define	DAC_ANALOG_VOLUME			0xC2
	#define	DAC_GLOB_CONFIG				0xC3
#endif

