;-----------------------------------------------------------------------------
; FICHERO: ft245_usb.asm
;
; DESCRIPCION: Comunicacion con el chip FT245BM de FTDI para conectividad
; 		 usando USB.
;
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; USB_INICIAR
;
;  Iniciar pins
;-----------------------------------------------------------------------------
USB_INICIAR:
	; /RD: Salida a uno
	sbi	USB_DDR,  USB_PIN_RD
	sbi	USB_PORT, USB_PIN_RD
	
	; WR: Salida a cero
	sbi	USB_DDR,  USB_PIN_WR
	cbi	USB_PORT, USB_PIN_WR
	
	; RXF: Entrada, con tiron
	sbi	USB_PORT, USB_PIN_RXF
	cbi	USB_DDR, USB_PIN_RXF
	

	ret
	

;-----------------------------------------------------------------------------
;
;	En este modo solo se procesan paquetes desde el PC
;
;  Los paquetes que se reciben son:
;
;  COMANDO           		 DATOS 
; ---------			-------
;    0x10 Leer sector		4 bytes sector LBA 
;    0x20 Escribir sector	4 bytes sector LBA + 512 bytes datos + 4 bytes 
;				  de los que el 1º es el XOR de los 512 bytes de datos
;    0x05 Obtener ID
;    0x06 Reset de sistema	
;
;-----------------------------------------------------------------------------
MODO_USB:
	cli
	ldi	r16, 0
	out	TCCR0,r16
	
	
	rcall	USB_INICIAR

	rcall	DESHAB_EXTRAM

	rcall	LCD_BORRAR
	
	ldi	ZH, high( STR_USB * 2)
	ldi	ZL,  low( STR_USB * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH


modo_usb_lp:
	; Algun paquete?
	; ---------------------
	sbis	USB_PIN, USB_PIN_RXF
	rjmp	usb_procesa_paquete ; RXF = 0
	
		
	; Si se pulsa alguna tecla, se sale del modo USB:
	rcall	LEE_PULS_Y_IR
	lds	r16, COMANDO_USUARIO	; Del tecl o IR
	tst	r16
	breq	modo_usb_lp
	
	; Si, salir: Hacer un reset del sistema
usb_ve_reset:
	rjmp	RESET




;-----------------------------------------------------------------------------
;  Recibir paquete y procesarla
;-----------------------------------------------------------------------------
usb_procesa_paquete:
	; Recibir paquete en buffer:
	ldi	XH, high (USB_COMANDO)
	ldi	XL,  low (USB_COMANDO)
	
	rcall	DESHAB_EXTRAM
	
	; El numero de comando:
	rcall	USB_RX_BYTE
	
	
	rcall	HAB_EXTRAM
	st	X+, r16
	
	; Poner contador de bytes segun comando (en bloques de 4 bytes)
	ldi	r18, 1	; CMD = 0x10
	
	cpi	r16, 0x20 ; Escribir sector
	brne	usb_proc_paq_lp	
	
	ldi	r18, 130	; CMD = 0x20
	
	; Los datos:
usb_proc_paq_lp:	
	rcall	DESHAB_EXTRAM
	
	rcall	USB_RX_BYTE
	mov	r1, r16
	rcall	USB_RX_BYTE
	mov	r2, r16
	rcall	USB_RX_BYTE
	mov	r3, r16
	rcall	USB_RX_BYTE
	mov	r4, r16
	
	rcall	HAB_EXTRAM
	
	st	X+, r1
	st	X+, r2
	st	X+, r3
	st	X+, r4
	
	dec	r18
	brne	usb_proc_paq_lp
	
	
	; Ejecutar:
	lds	r16, USB_COMANDO
	cpi	r16, 0x10
	breq	USB_READ_SECTOR

	cpi	r16, 0x05
	breq	USB_TEST

	cpi	r16, 0x06
	breq	usb_ve_reset
			
	cpi	r16, 0x20
	breq	veUSB_WRITE_SECTOR

	rjmp	modo_usb_lp

veUSB_WRITE_SECTOR:
	rjmp	USB_WRITE_SECTOR

;-----------------------------------------------------------------------------
;  CMD: TEST
;
;  Pone en display los 4 bytes enviados y 
;
;  RESP: Una cadena de 14 caracteres fijos:
;-----------------------------------------------------------------------------
USB_TEST:
	rcall	HAB_EXTRAM
	
	lds	r1, USB_DATA+0
	lds	r2, USB_DATA+1
	lds	r3, USB_DATA+2
	lds	r4, USB_DATA+3
	
	rcall	DESHAB_EXTRAM
	
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16

	mov	r16, r1
	rcall	LCD_ESCRIBE_R16_HEX
	mov	r16, r2
	rcall	LCD_ESCRIBE_R16_HEX
	mov	r16, r3
	rcall	LCD_ESCRIBE_R16_HEX
	mov	r16, r4
	rcall	LCD_ESCRIBE_R16_HEX	


	rcall	DESHAB_EXTRAM

	ldi	r16, 'G'
	rcall	USB_TX_BYTE
	ldi	r16, 'i'
	rcall	USB_TX_BYTE
	ldi	r16, 'g'
	rcall	USB_TX_BYTE
	ldi	r16, 'a'
	rcall	USB_TX_BYTE
	ldi	r16, 'M'
	rcall	USB_TX_BYTE
	ldi	r16, 'P'
	rcall	USB_TX_BYTE
	ldi	r16, '3'
	rcall	USB_TX_BYTE
	ldi	r16, ' '
	rcall	USB_TX_BYTE
	ldi	r16, 'v'
	rcall	USB_TX_BYTE
	ldi	r16, 'J'
	rcall	USB_TX_BYTE
	ldi	r16, 'U'
	rcall	USB_TX_BYTE
	ldi	r16, 'L'
	rcall	USB_TX_BYTE
	ldi	r16, '0'
	rcall	USB_TX_BYTE
	ldi	r16, '3'
	rcall	USB_TX_BYTE
	
	
	; Fin de comando
	rjmp	modo_usb_lp
	

;-----------------------------------------------------------------------------
;  CMD: Leer sector
;  RESP: Los 512 bytes
;-----------------------------------------------------------------------------
USB_READ_SECTOR:
	rcall	HAB_EXTRAM
	
	lds	r16, USB_COMANDO+1
	sts	LBA_DIR+0,r16
	lds	r16, USB_COMANDO+2
	sts	LBA_DIR+1,r16
	lds	r16, USB_COMANDO+3
	sts	LBA_DIR+2,r16
	lds	r16, USB_COMANDO+4
	sts	LBA_DIR+3,r16
	
	ldi	r16, 0x01
	sts	SECTOR_CNT, r16
	
	ldi	YH, high ( USB_DATA )
	ldi	YL, low  ( USB_DATA )
	
	rcall	ATA_ReadSectors

	
	; Y ahora enviar los 512 bytes:
	; ---------------------------------
	ldi	YH, high ( USB_DATA )
	ldi	YL, low  ( USB_DATA )
	
	ldi	r18, 0
usb_cmd_rd_lp1:
	rcall	HAB_EXTRAM
	ld	r16, Y+
	rcall	DESHAB_EXTRAM
	rcall	USB_TX_BYTE		

	rcall	HAB_EXTRAM
	ld	r16, Y+
	rcall	DESHAB_EXTRAM
	rcall	USB_TX_BYTE		

	dec	r18
	brne	usb_cmd_rd_lp1

	; Fin de comando
	rjmp	modo_usb_lp
	
	
;-----------------------------------------------------------------------------
;  CMD: Write Sector
;
; 
;  RESP: 0x10 si XOR OK y escrito.
;        0x20 si XOR ERROR !
;-----------------------------------------------------------------------------
USB_WRITE_SECTOR:
	; Comprobar XOR !!

	ldi	XH, high ( USB_DATA+4 )
	ldi	XL, low  ( USB_DATA+4 )
	
	clr	r17	
usbwrsect_xor:
	ld	r16, X+
	eor	r17, r17
	
	cpi	XL, low  ( USB_DATA+4 +512 )
	brne	usbwrsect_xor
	cpi	XH, high ( USB_DATA+4 +512 )
	brne	usbwrsect_xor
	
	ld	r16, X+
	cp	r16, r17
	
	breq	usbwrsect_Ok

	; Enviar 0x20 indicando error de XOR:
	ldi	r16, 0x20	
	rcall	USB_TX_BYTE		
	
	; Fin de comando
	rjmp	modo_usb_lp

	
usbwrsect_Ok:
	; Poner LBA:
	lds	r16, USB_COMANDO+1
	sts	LBA_DIR+0,r16
	lds	r16, USB_COMANDO+2
	sts	LBA_DIR+1,r16
	lds	r16, USB_COMANDO+3
	sts	LBA_DIR+2,r16
	lds	r16, USB_COMANDO+4
	sts	LBA_DIR+3,r16
	
	ldi	r16, 0x01
	sts	SECTOR_CNT, r16
	
	ldi	YH, high ( USB_DATA+4 )
	ldi	YL, low  ( USB_DATA+4 )
	
	rcall	ATA_WriteSectors
	

	; Enviar 0x10 de confirmacion:
	ldi	r16, 0x10	
	rcall	USB_TX_BYTE		

	; Fin de comando
	rjmp	modo_usb_lp


;-----------------------------------------------------------------------------
;  Recibe un byte (en r16)
;-----------------------------------------------------------------------------
USB_RX_BYTE:
	; Asegurar que RXF = 0
	sbic	USB_PIN, USB_PIN_RXF
	rjmp	USB_RX_BYTE
	
	clr	r16
	out	DDRA,r16
	
	; Ciclo de lectura:
	cbi	USB_PORT, USB_PIN_RD
	nop
	in	r16, PINA
	sbi	USB_PORT, USB_PIN_RD

	
	ret
	
;-----------------------------------------------------------------------------
;  Envia un byte (en r16)
;-----------------------------------------------------------------------------
USB_TX_BYTE:
	; PORTC = entrada y pull-ups	
	clr	r17
	out	DDRC, r17		
	ser	r17
	out	PORTC, r17

	; PORTA = DATOS
	out	DDRA, r17
	out	PORTA, r16
	

usb_tx_lp:
	sbic	PINC, 0		; A8 conectada a TXE
	rjmp	usb_tx_lp	;  Debe estar a 0 para enviar
	
	
	; Ciclo de Escritura:
	sbi	USB_PORT, USB_PIN_WR
	nop
	cbi	USB_PORT, USB_PIN_WR
	
	ret
		
	

	