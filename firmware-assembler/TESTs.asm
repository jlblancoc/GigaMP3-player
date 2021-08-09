
; Prueba de velocidad de lectura del disco duro:
;  mide el tiempo de leer 2 Mbs

	; TMR1:
	ldi	r16, 0x05
	out	TCCR1B,r16
	
	clr	r16
	out	TCNT1H,r16
	out	TCNT1L,r16
	
		


	clr	r16
	sts	LBA_DIR, r16
	sts	LBA_DIR+1, r16
	sts	LBA_DIR+2, r16
	sts	LBA_DIR+3, r16
speed_loop:

	ldi	r16, 1
	sts	SECTOR_CNT, r16
	
	ldi	YH, high( SECTOR_BUFFER )
	ldi	YL,  low( SECTOR_BUFFER )
	
	rcall	ATA_ReadSectors
	
	
	; Siguiente sector:
	lds	r16, LBA_DIR+0
	lds	r17, LBA_DIR+1
	lds	r18, LBA_DIR+2
	lds	r19, LBA_DIR+3
	
	clr	r0
	ldi	r20, 0x01
	add	r16,r20
	adc	r17, r0
	adc	r18, r0
	adc	r19, r0

	sts	LBA_DIR, r16
	sts	LBA_DIR+1, r17
	sts	LBA_DIR+2, r18
	sts	LBA_DIR+3, r19
	
	cpi	r17, 0x10
	brne	speed_loop
	

	rcall	DESHAB_EXTRAM



	in	r1,TCNT1L
	in	r2,TCNT1H
	
	mov	r16, r2
	rcall	LCD_ESCRIBE_R16_HEX
	mov	r16, r1
	rcall	LCD_ESCRIBE_R16_HEX











; --------------------------------------------------
;	
; --------------------------------------------------
ATA_DEBUG_DumpRegistros:
	ldi	r16, 0x00
	rcall	LCD_SET_CURSOR_POS_R16

	ldi	r17,ATA_CMD
	ldi	r16,1
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	ldi	r17,ATA_CMD
	ldi	r16,2
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	ldi	r17,ATA_CMD
	ldi	r16,3
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	ldi	r17,ATA_CMD
	ldi	r16,4
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	ldi	r17,ATA_CMD
	ldi	r16,5
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	ldi	r17,ATA_CMD
	ldi	r16,6
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	ldi	r17,ATA_CMD
	ldi	r16,7
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16
	
	ldi	r17,ATA_CTRL
	ldi	r16,6
	rcall	ATA_ReadByte
	rcall	LCD_ESCRIBE_R16_HEX
	
	ret



