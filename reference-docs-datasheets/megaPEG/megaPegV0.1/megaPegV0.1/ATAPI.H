#ifndef ATAPI_H
  #define ATAPI_H

  u08 ATAPI_ReadSector(u32 lba, u08 *internal_buffer);
  u08 ATAPI_ReadSectorX(u32 lba, u08 *external_buffer);
  u08 ATAPI_SelectDevice(u08 dev);

	//ATA-ATAPI-4 Commands
	#define ATA_CFA_ERASE_SECTORS                 0xC0  //VS
	#define ATA_CFA_REQUEST_EXTENDED_ERROR_CODE   0x03
	#define ATA_CFA_TRANSLATE_SECTOR              0x87  //VS
	#define ATA_CFA_WRITE_MULTIPLE_WITHOUT_ERASE  0xCD
	#define ATA_CFA_WRITE_SECTORS_WITHOUT_ERASE   0x38
	#define ATA_CHECK_POWER_MODE                  0xE5
	#define ATA_DEVICE_RESET                      0x08
	#define ATA_DOWNLOAD_MICROCODE                0x92
	#define ATA_EXECUTE_DEVICE_DIAGNOSTIC         0x90
	#define ATA_FLUSH_CACHE                       0xE7
	#define ATA_GET_MEDIA_STATUS                  0xDA
	#define ATA_IDENTIFY_DEVICE                   0xEC
	#define ATA_IDENTIFY_PACKET_DEVICE            0xA1
	#define ATA_IDLE                              0xE3
	#define ATA_IDLE_IMMEDIATE                    0xE1
	#define ATA_INITIALIZE_DEVICE_PARAMETERS      0x91
	#define ATA_MEDIA_EJECT                       0xED
	#define ATA_MEDIA_LOCK                        0xDE
	#define ATA_MEDIA_UNLOCK                      0xDF
	#define ATA_NOP                               0x00
	#define ATA_PACKET                            0xA0
	#define ATA_READ_BUFFER                       0xE4
	#define ATA_READ_DMA                          0xC8  //(0xC9 for drive)
	#define ATA_READ_DMA_QUEUED                   0xC7
	#define ATA_READ_MULTIPLE                     0xC4
	#define ATA_READ_NATIVE_MAX_ADDRESS           0xF8
	#define ATA_READ_SECTOR_S                     0x20  //(0x21 for drive)
	#define ATA_READ_VERIFY_SECTOR_S              0x40  //(0x40 for drive)
	#define ATA_SECURITY_DISABLE_PASSWORD         0xF6
	#define ATA_SECURITY_ERASE_PREPARE            0xF3
	#define ATA_SECURITY_ERASE_UNIT               0xF4
	#define ATA_SECURITY_FREEZE_LOCK              0xF5
	#define ATA_SECURITY_SET_PASSWORD             0xF1
	#define ATA_SECURITY_UNLOCK                   0xF2
	#define ATA_SEEK                              0x70
	#define ATA_SERVICE                           0xA2
	#define ATA_SET_FEATURES                      0xEF
	#define ATA_SET_MAX                           0xF9
	#define ATA_SET_MULTIPLE_MODE                 0xC6
	#define ATA_SLEEP                             0xE6
	#define ATA_SMART                             0xB0  //and FEATURE=0xB0
	#define ATA_STANDBY                           0xE2
	#define ATA_STANDBY_IMMEDIATE                 0xE0
	#define ATA_WRITE_BUFFER                      0xE8
	#define ATA_WRITE_DMA                         0xCA  //(0xCB for drive)
	#define ATA_WRITE_DMA_QUEUED                  0xCC
	#define ATA_WRITE_MULTIPLE                    0xC5
	#define ATA_WRITE_SECTOR_S                    0x30  //(0x31 for drive)

#endif

