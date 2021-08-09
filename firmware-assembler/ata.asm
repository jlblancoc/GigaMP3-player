;-----------------------------------------------------------------------------
; FICHERO: ata.asm
;
; DESCRIPCION: Protocolo de conexion al disco duro por interfaz
;
;-----------------------------------------------------------------------------


; Comandos ATA:
; --------------------------------------------------------
.equ	ATA_CMD  = 1
.equ	ATA_CTRL = 0

.equ	ATA_CTRL_DEVCTRL 	= 0x06 ; Solo escritura
.equ	ATA_CTRL_ALTSTATUS 	= 0x06 ; Solo lectura
.equ	ATA_CMD_STATUS  	= 0x07 ; Solo lectura
.equ	ATA_CMD_COMMAND  	= 0x07 ; Solo escritura
.equ	ATA_CMD_DEVHEAD 	= 0x06
.equ	ATA_CMD_CYL_HI		= 0x05
.equ	ATA_CMD_CYL_LO		= 0x04
.equ	ATA_CMD_SECNUM  	= 0x03
.equ	ATA_CMD_SECCNT  	= 0x02
.equ	ATA_CMD_FEATURES  	= 0x01 ; Solo escritura
.equ	ATA_CMD_ERROR  		= 0x01 ; Solo lectura
.equ	ATA_CMD_DATA  		= 0x00 


;-----------------------------------------------------------------------------
; SetAddress:
;
;	r16: direccion de registro a leer 
;	r17: 0 = bloque de commandos / 1= bloque de control
;
;
; addressing bits
; 35 DA0	A0	0x01	Address Line 0
; 33 DA1	A1	0x02	Address Line 1
; 36 DA2	A2	0x04	Address Line 2
;
; chip selects
; 37 CS0	A3 	0x08	Command Block Select
; 38 CS1	A4	0x10	Control Block Select
;	
;
;	u16 i;
;	
; 	if (cs==CTRL)  
;		i = adr+0x08;		// select A4 low -> CS1 -> CTRL
;	else 
;		i = adr+0x10;		// select A3 low -> CS0 -> CMD
;
;	return *(u08 *) (i+0xE000);
;-----------------------------------------------------------------------------
ATA_SetAddress:
	push	r18

	andi	r16, 0x07
	
	; A15=1 -> No ram
	ldi	XH, 0x80
	
	; Bit 4=0, correr a la izquierda si direccionamos 
	ldi	XL, 0x08
	sbrc	r17, 0		; r17=0 -> Comando, dejar A4=0
	lsl	XL		; r17=1 -> Control, poner A3=0
		
	or	XL, r16
	
	; Hab RAM externa
	rcall	HAB_EXTRAM
	nop

	; Falsa lectura para poner valores en bus
	ld	r16, X

	; Deshab. RAM externa
	rcall	DESHAB_EXTRAM

	pop	r18
	ret


;-----------------------------------------------------------------------------
;u08 ReadBYTE(u08 cs, u08 adr) 
;
;	r16: direccion de registro a leer 
;	r17: 0 = bloque de commandos / 1= bloque de control
;
;	-> Salida en r16
;
;
;-----------------------------------------------------------------------------
ATA_ReadByte:
	cli
	
	rcall	ATA_SetAddress
	
	clr	temp
	out	DDRA, temp
;	out	DDRC, temp
	
	cbi	IDE_PORT, IDE_RD
	nop
	in	r16, PINA	
	sbi	IDE_PORT, IDE_RD

	sei
	
	ret	



;-----------------------------------------------------------------------------
; ATA_WriteByte
;
;	r16: direccion de registro a leer 
;	r17: 0 = bloque de commandos / 1= bloque de control
;	r18: Dato a escribir
;
;-----------------------------------------------------------------------------
ATA_WriteByte:
	cli
	
	rcall	ATA_SetAddress

	out	PORTA,r18
	ser	temp
	out	DDRA, temp
	
	cbi	IDE_PORT, IDE_WR
	nop		
	sbi	IDE_PORT, IDE_WR
	
;	clr	temp
;	out	DDRA, temp
	
	sei
	
	ret	

;-----------------------------------------------------------------------------
; ATA_SeleccionarDisco
;-----------------------------------------------------------------------------
ATA_SeleccionarDisco:
	; Esperar a BSY y DRQ = 0, DRDY=1
	ldi	r16, 0x40
	sts	ATA_WAITMASK_UNOS, r16
	ldi	r16, 0x88
	sts	ATA_WAITMASK_CEROS, r16
	rcall	ATA_ESPERA_MASKS

	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_DEVHEAD
	ldi	r18, 0x00
	rcall	ATA_WriteByte
	
	ldi	r25, 20
	rcall	RETARDO_R25_MAX100us
	
	; Esperar a BSY y DRQ = 0 (VALORES IGUAL QUE ANTES)
	rcall	ATA_ESPERA_MASKS
	
	ret
	
	
;-----------------------------------------------------------------------------
; ATA_ESPERA_MASKS
;-----------------------------------------------------------------------------
ATA_ESPERA_MASKS:
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_STATUS
	rcall	ATA_ReadByte

	mov	r17, r16
	
	; Comprobar mascara de ceros:
	lds	r18, ATA_WAITMASK_CEROS
	and	r17, r18	; Solo quedan los bits a testear:
	brne	ATA_ESPERA_MASKS ; Alguno != 0 
	
	mov	r17, r16
	
	; Comprobar mascara de unos:	
	lds	r18, ATA_WAITMASK_UNOS	
	and	r17, r18	; Quitar resto de bits:	
	cp	r17, r18	; Si son iguales es q estan todos a 1	
	brne	ATA_ESPERA_MASKS ; Alguno != 0 
	
	; Se cumplen los ceros y unos, salir:
	ret
	


;-----------------------------------------------------------------------------
;	ATA_ReadSectors
;
; Parametros:
;
;	- LBA_DIR: en RAM, numero de sector a leer (en LBA)
;	- SECTOR_CNT: Num. de sectores a leer. (max=16)
;	- (Y): Apunta a destino de datos en RAM externa.
;	
;-----------------------------------------------------------------------------
ATA_ReadSectors:
	push	XH
	push	XL
	

	; Esperar a BUSY = 0, DRDY=1
	; -----------------------------
	ldi	r16, 0x40
	sts	ATA_WAITMASK_UNOS, r16
	ldi	r16, 0x80
	sts	ATA_WAITMASK_CEROS, r16
	rcall	ATA_ESPERA_MASKS


	; Calcular num. de buffers a rellenar:
	ldi	r17, 512/ATA_TEMP_BUF_TAM ; buffers por sector

	lds	r16, SECTOR_CNT
	clr	r18
atardsec_multiplica:
	add	r18, r17
	dec	r16
	brne	atardsec_multiplica
	
	sts	ATA_RDSECT_CUANTOSBUFS, r18


	; Mandar parametros
	; --------------------------------
	; r18 = 0x40 | LBA[27:24]
	ldi	r18, 0x40	; LBA activado
	lds	r16, LBA_DIR+3
	andi	r16, 0x0F
	add	r18, r16
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_DEVHEAD
	rcall	ATA_WriteByte
	
		
	; r18 = LBA[23:16]
	lds	r18, LBA_DIR+2
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_CYL_HI
	rcall	ATA_WriteByte
	
	; r18 = LBA[15:8]
	lds	r18, LBA_DIR+1
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_CYL_LO
	rcall	ATA_WriteByte
	
	; r18 = LBA[7:0]
	lds	r18, LBA_DIR
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_SECNUM
	rcall	ATA_WriteByte
	
	; r18 = NUM. SECTORES
	lds	r18, SECTOR_CNT
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_SECCNT
	rcall	ATA_WriteByte
	

	; Enviar comando de lectura:
	; -------------------------------
	ldi	r18, 0x20
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_COMMAND
	rcall	ATA_WriteByte


	; Bucle para cada bloque de buffer interno que se reciba:	
	; --------------------------------------------------------
	
	; Esperar a BUSY = 0 y DRQ=1
ATA_ReadSectors_read_loop:
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_STATUS
	rcall	ATA_ReadByte
		
	sbrc	r16, 7	; BSY=0? Salir del bucle
	rjmp	ATA_ReadSectors_read_loop
	sbrs	r16, 3	; DRQ=1? Salir del bucle
	rjmp	ATA_ReadSectors_read_loop

	
	sbrc	r16, 0  ; Bit ERR?
	rjmp	ATA_Error

	sbrs	r16, 3  ; DRQ=1 -> Deberia haber datos que recibir !!
	rjmp	ATA_Error


	cli
		
	; Poner lineas de direcciones:
	ldi	r17, ATA_CMD
	ldi	r16, ATA_CMD_DATA
	rcall	ATA_SetAddress

	
	; A y C entradas:
	clr	r16
	out	DDRA, r16
	out	DDRC, r16
	
	; Leer el buffer interno:
	ldi	XH, high( ATA_TEMP_BUF )
	ldi	XL,  low( ATA_TEMP_BUF )
ata_rd_sect_leer_intbuf:
;	cli
	
;	; Poner lineas de direcciones:
;	ldi	r17, ATA_CMD
;	ldi	r16, ATA_CMD_DATA
;	rcall	ATA_SetAddress


	cbi	IDE_PORT, IDE_RD
	nop
	
	in	r16, PINA
	in	r17, PINC
	
	sbi	IDE_PORT, IDE_RD
	
;	sei	
	
	st	X+, r16
	st	X+, r17


	cpi	XL, low( ATA_TEMP_BUF+ATA_TEMP_BUF_TAM )
	brne	ata_rd_sect_leer_intbuf
	

	sei
	
	
	; Y ahora copiar ese buffer a la direccion destino
	;   en RAM externa:
	rcall	HAB_EXTRAM
	
	ldi	XH, high( ATA_TEMP_BUF )
	ldi	XL,  low( ATA_TEMP_BUF )
ata_rd_sect_copia_loop:
	ld	r16, X+
	st	Y+, r16

	cpi	XL, low( ATA_TEMP_BUF+ATA_TEMP_BUF_TAM )
	brne	ata_rd_sect_copia_loop

	; Deshab. RAM ext:
	rcall	DESHAB_EXTRAM


	; Hemos acabado los buffers??
	lds	r16, ATA_RDSECT_CUANTOSBUFS
	dec	r16
	sts	ATA_RDSECT_CUANTOSBUFS,r16
	
	brne	ATA_ReadSectors_read_loop



	pop	XL
	pop	XH	
	
	ret



;-----------------------------------------------------------------------------
;	ATA_WriteSectors
;
; Parametros:
;
;	- LBA_DIR: en RAM, numero de sector a escribir (en LBA)
;	- SECTOR_CNT: Num. de sectores  (max=16)
;	- (Y): Apunta a origen de datos en RAM externa.
;	
;-----------------------------------------------------------------------------
ATA_WriteSectors:
	push	XH
	push	XL
	

	; Esperar a BUSY = 0, DRDY=1
	; -----------------------------
	ldi	r16, 0x40
	sts	ATA_WAITMASK_UNOS, r16
	ldi	r16, 0x80
	sts	ATA_WAITMASK_CEROS, r16
	rcall	ATA_ESPERA_MASKS


	; Calcular num. de buffers a rellenar:
	ldi	r17, 512/ATA_TEMP_BUF_TAM ; buffers por sector

	lds	r16, SECTOR_CNT
	clr	r18
atawrsec_multiplica:
	add	r18, r17
	dec	r16
	brne	atawrsec_multiplica
	
	sts	ATA_RDSECT_CUANTOSBUFS, r18


	; Mandar parametros
	; --------------------------------
	; r18 = 0x40 | LBA[27:24]
	ldi	r18, 0x40	; LBA activado
	lds	r16, LBA_DIR+3
	andi	r16, 0x0F
	add	r18, r16
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_DEVHEAD
	rcall	ATA_WriteByte
	
		
	; r18 = LBA[23:16]
	lds	r18, LBA_DIR+2
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_CYL_HI
	rcall	ATA_WriteByte
	
	; r18 = LBA[15:8]
	lds	r18, LBA_DIR+1
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_CYL_LO
	rcall	ATA_WriteByte
	
	; r18 = LBA[7:0]
	lds	r18, LBA_DIR
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_SECNUM
	rcall	ATA_WriteByte
	
	; r18 = NUM. SECTORES
	lds	r18, SECTOR_CNT
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_SECCNT
	rcall	ATA_WriteByte
	

	; Enviar comando de Escritura:
	; -------------------------------
	ldi	r18, 0x30
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_COMMAND
	rcall	ATA_WriteByte


	; Bucle para cada bloque de buffer interno que se envie:	
	; --------------------------------------------------------
	
	; Esperar a BUSY = 0 y DRQ=1
ATA_WriteSectors_write_loop:
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_STATUS
	rcall	ATA_ReadByte
		
	sbrc	r16, 7	; BSY=0? Salir del bucle
	rjmp	ATA_WriteSectors_write_loop
	sbrs	r16, 3	; DRQ=1? Salir del bucle
	rjmp	ATA_WriteSectors_write_loop

	
	sbrc	r16, 0  ; Bit ERR?
	rjmp	ATA_Error

	sbrs	r16, 3  ; DRQ=1 -> Deberia estar esperando datos !!
	rjmp	ATA_Error


	cli
		
	
	; Cargar de mem. externa al bufer interno:
	; -------------------------------------------
	rcall	HAB_EXTRAM
	
	ldi	XH, high( ATA_TEMP_BUF )
	ldi	XL,  low( ATA_TEMP_BUF )
ata_wr_sect_copia_loop:
	ld	r16, Y+
	st	X+, r16

	cpi	XL, low( ATA_TEMP_BUF+ATA_TEMP_BUF_TAM )
	brne	ata_wr_sect_copia_loop

	; Deshab. RAM ext:
	rcall	DESHAB_EXTRAM


	; Poner lineas de direcciones:
	ldi	r17, ATA_CMD
	ldi	r16, ATA_CMD_DATA
	rcall	ATA_SetAddress
	
	; A y C salidas:
	ser	r16
	out	DDRA, r16
	out	DDRC, r16
	
	
	; Enviar el buffer interno:
	ldi	XH, high( ATA_TEMP_BUF )
	ldi	XL,  low( ATA_TEMP_BUF )

ata_wr_sect_envia_intbuf:
	ld	r16, X+
	ld	r17, X+

	out	PORTA, r16
	out	PORTC, r17
	nop

	cbi	IDE_PORT, IDE_WR
	nop
	sbi	IDE_PORT, IDE_WR
	

	cpi	XL, low( ATA_TEMP_BUF+ATA_TEMP_BUF_TAM )
	brne	ata_wr_sect_envia_intbuf
	

	sei
	

	; Hemos acabado los buffers??
	lds	r16, ATA_RDSECT_CUANTOSBUFS
	dec	r16
	sts	ATA_RDSECT_CUANTOSBUFS,r16
	
	brne	ATA_WriteSectors_write_loop


	pop	XL
	pop	XH	
	
	ret
	
	
;-----------------------------------------------------------------------------
;	ERROR GRAVE:
;-----------------------------------------------------------------------------
ATA_Error:
	cli
	
	rcall	LCD_BORRAR

	ldi	ZH, high( STR_ATAERR * 2)
	ldi	ZL,  low( STR_ATAERR * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
	
	; Leer error:
	ldi	r17,ATA_CMD
	ldi	r16,ATA_CMD_ERROR
	rcall	ATA_ReadByte
		
	rcall	LCD_ESCRIBE_R16_HEX
	
	; Esperar y poner registros:
;	ldi	r25, 60
;	rcall	RETARDO_R25x25ms
			
;ATA_DEBUG_DumpRegistrosLOOP:
;	rcall	ATA_DEBUG_DumpRegistros
;	rjmp	ATA_DEBUG_DumpRegistrosLOOP
	
;ATA_DEBUG_DumpRegistrosBLOQ:
;	rcall	ATA_DEBUG_DumpRegistros
atadead:
	rjmp	atadead	
	
	
	
