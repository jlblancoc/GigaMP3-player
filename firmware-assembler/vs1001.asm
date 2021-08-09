;-----------------------------------------------------------------------------
; FICHERO: vs1001.asm
;
; DESCRIPCION: Comunicacion con chip decodificador de MP3: VS1001k
;
;
;-----------------------------------------------------------------------------



.equ	VS1001_CONFIG_XTAL	= 	0x9BEE	; 14.3 Mhz x 2

					;0x9388  ; Valor para 10 Mhz


.equ	VS1001ADDR_MODE		= 0
.equ	VS1001ADDR_STATUS	= 1
.equ	VS1001ADDR_INT_FNTLH	= 2
.equ	VS1001ADDR_CLOCKF	= 3
.equ	VS1001ADDR_DECODE_TIME	= 4
.equ	VS1001ADDR_AUDATA	= 5
.equ	VS1001ADDR_WRAM		= 6
.equ	VS1001ADDR_WRAMADDR	= 7
.equ	VS1001ADDR_HDAT0	= 8
.equ	VS1001ADDR_HDAT1	= 9
.equ	VS1001ADDR_AIADDR	= 10
.equ	VS1001ADDR_VOL		= 11


;--------------------------------------------------------
;  VS1001_INICIAR
;--------------------------------------------------------
VS1001_INICIAR:
	; DREQ entrada, tiron
	cbi	VS1001_DDR, VS1001_DREQ
	sbi	VS1001_PORT, VS1001_DREQ
	
	; BSYNC salida, a cero
	sbi	VS1001_DDR, VS1001_BSYNC
	cbi	VS1001_PORT, VS1001_BSYNC
	
	; VS1001_MP3CS salida, a uno ahora: Deshabilitado
	sbi	VS1001_DDR, VS1001_MP3CS
	sbi	VS1001_PORT, VS1001_MP3CS
	
	; MOSI , SS y SCK salida:
	sbi	DDRB, 4 ; SS
	sbi	DDRB, 5 ; MOSI
	sbi	DDRB, 7 ; SCK
	
	cbi	PORTB, 7; SCK = 0
	
	
	; Configurar SPI:
	;
	;	CLK= osc / 4 	(2 Mhz)
	;	Fase reloj = positiva y 
	;	Modo: MASTER
	;	Activar SPI!!	
	ldi	r16, 0b01010000
	out	SPCR, r16
	
	
	; Quitar posibles bytes recibidos, etc...
	in	r16, SPSR	



	ret
	

;--------------------------------------------------------
;  VS1001_ESPERA_TX_LIBRE
;
;  Rutina auxiliar que espera a que termine la 
;   transmision/recepcion de un byte por SPI:
;--------------------------------------------------------
VS1001_ESPERA_TX_LIBRE:
	sbis	SPSR, SPIF
	rjmp	VS1001_ESPERA_TX_LIBRE
	
	ret

	
;--------------------------------------------------------
;  VS1001_ESC_REGISTRO
;
;   r17: Direccion
;   r18/r19: Bytes a enviar (Alto / bajo)
;--------------------------------------------------------
VS1001_ESC_REGISTRO:
	; Seleccionar chip: xCS = 0
	cbi	VS1001_PORT, VS1001_MP3CS
	
	; Comando: WRITE
	ldi	r16, 0x02
	out	SPDR, r16
	rcall	VS1001_ESPERA_TX_LIBRE
	
	; Direccion:
	out	SPDR, r17
	rcall	VS1001_ESPERA_TX_LIBRE
	
	; Byte alto:
	out	SPDR, r18
	rcall	VS1001_ESPERA_TX_LIBRE
			
	; Byte bajo:	
	out	SPDR, r19
	rcall	VS1001_ESPERA_TX_LIBRE
		

	; De - seleccionar chip: xCS = 1
	sbi	VS1001_PORT, VS1001_MP3CS
	
		
	
	; Retardo para asegurar que no se escribe 
	;  datos MPEG hasta 5us despues!!
	ldi	r25, 15
	rcall	RETARDO_R25_MAX100us

	ret
	
	
;--------------------------------------------------------
;  VS1001_LEE_REGISTRO
;
;   r17: Direccion
;
;   r18/r19: Bytes leidos (Alto / bajo)
;--------------------------------------------------------
VS1001_LEE_REGISTRO:
	; Seleccionar chip: xCS = 0
	cbi	VS1001_PORT, VS1001_MP3CS
	
	; Comando: READ
	ldi	r16, 0x03
	out	SPDR, r16
	rcall	VS1001_ESPERA_TX_LIBRE
	
	; Direccion:
	out	SPDR, r17
	rcall	VS1001_ESPERA_TX_LIBRE
	
	; Byte alto:
	ldi	r16, 0x00
	out	SPDR, r16
	rcall	VS1001_ESPERA_TX_LIBRE
	in	r18, SPDR
			
	; Byte bajo:	
	ldi	r16, 0x00
	out	SPDR, r16
	rcall	VS1001_ESPERA_TX_LIBRE
	in	r19, SPDR
		

	; De - seleccionar chip: xCS = 1
	sbi	VS1001_PORT, VS1001_MP3CS
	
	
	; Retardo para asegurar que no se escribe 
	;  datos MPEG hasta 5us despues!!
	ldi	r25, 15
	rcall	RETARDO_R25_MAX100us

	ret
	
	

;--------------------------------------------------------
;  VS1001_TX_MPEG_DATA
;
;   r16: Dato
;--------------------------------------------------------
VS1001_TX_MPEG_DATA:
	; Sinc. de byte
	sbi	VS1001_PORT, VS1001_BSYNC
	
	out	SPDR, r16
	
	nop
	nop
	nop
	
	; Ya esta bien
	cbi	VS1001_PORT, VS1001_BSYNC
	
	; Esperar final:
	rcall	VS1001_ESPERA_TX_LIBRE
		

	ret
	

;--------------------------------------------------------
;  VS1001_TX_MPEG_DATA_32X
;
;   (X): 32 bytes de datos a mandar
;--------------------------------------------------------
VS1001_TX_MPEG_DATA_32X:
	push	r18
	
	; Sinc. de byte
	sbi	VS1001_PORT, VS1001_BSYNC
	
	ldi	r18, 32
vs1001_tx32_loop:
	ld	r16, X+
	out	SPDR, r16
	rcall	VS1001_ESPERA_TX_LIBRE
	
	dec	r18
	brne	vs1001_tx32_loop

	; Ya esta bien
	cbi	VS1001_PORT, VS1001_BSYNC		

	pop	r18
	ret
	


;--------------------------------------------------------
;  VS1001_RESET
;
;--------------------------------------------------------
VS1001_RESET:
	; Enviar ceros para asegurar fin de cancion
	ldi	r19, 2048/32
	rcall	VS1001_TX_R19x32_CEROS
	
	; Retardo: Grande por si es bitrate bajo :-(
;	ldi	r25, 18
	ldi	r25, 8
	rcall	RETARDO_R25x25ms

	; Reset SW:	
	; ------------------------------------------------------------
	ldi	r17, VS1001ADDR_MODE
	ldi	r18, 0x00
	ldi	r19, 0x04
	rcall	VS1001_ESC_REGISTRO
	
	; Retardo
	ldi	r25, 3
	rcall	RETARDO_R25x25ms

	; Asegurar DREQ=1 ( Preparado para decodificar )
vs1001_reset_esp_dreq:
	sbis	VS1001_PIN, VS1001_DREQ
	rjmp	vs1001_reset_esp_dreq
	
	ldi	r25, 2
	rcall	RETARDO_R25x25ms
		
	; Configurar cristal:
	ldi	r17, VS1001ADDR_CLOCKF
	ldi	r18, high ( VS1001_CONFIG_XTAL )
	ldi	r19, high ( VS1001_CONFIG_XTAL )
	rcall	VS1001_ESC_REGISTRO

	ldi	r25, 2
	rcall	RETARDO_R25x25ms

	; Configurar cristal:
	ldi	r17, VS1001ADDR_CLOCKF
	ldi	r18, high ( VS1001_CONFIG_XTAL )
	ldi	r19, high ( VS1001_CONFIG_XTAL )
	rcall	VS1001_ESC_REGISTRO

	ldi	r25, 2
	rcall	RETARDO_R25x25ms

	; Enviar algunos ceros para retrasar al chip
	ldi	r19, 1024/32
	rcall	VS1001_TX_R19x32_CEROS
	
	ret
	


;--------------------------------------------------------
; VS1001_TX_R19x32_CEROS
;--------------------------------------------------------
VS1001_TX_R19x32_CEROS:
	; No enviar datos hasta que DREQ = 1
	sbis	VS1001_PIN, VS1001_DREQ
	rjmp	VS1001_TX_R19x32_CEROS
	
	; Enviar 32 ceros seguidos:
	ldi	r16, 0x00
	ldi	r18, 32
VS1001_RESET_tx_ceros_loop2:
	rcall	VS1001_TX_MPEG_DATA
	dec	r18
	brne	VS1001_RESET_tx_ceros_loop2
	
	dec	r19
	brne	VS1001_TX_R19x32_CEROS
	
	ret
	


;--------------------------------------------------------
;  VS1001_CAMBIAR_VOLUMEN
;
;	r17: Nuevo volumen
;--------------------------------------------------------
VS1001_CAMBIAR_VOLUMEN:
	mov	r18, r17
	mov	r19, r17
	ldi	r17, VS1001ADDR_VOL
	rjmp	VS1001_ESC_REGISTRO
	