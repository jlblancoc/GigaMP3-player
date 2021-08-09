;-----------------------------------------------------------------------------
; FICHERO: LCD.asm
;
; DESCRIPCION: Implementa rutinas de manejo del LCD
;
; RUTINAS:
; --------
;
;
;	- LCD_INICIAR: Inicializa el diplay
;
;       - LCD_ESCRIBE_X
;
;-----------------------------------------------------------------------------


; Caracteres definidos por usuario: 0x01, 0x02, ... 0x07
;  Tamaño: 5 x 8 pixels
.equ	LCD_USERCHARS_CUANTOS = 4

LCD_USERCHARS:
	.db	$00,$00,$00,$00,$01,$03,$07,$03
	.db	$00,$07,$0F,$1F,$1F,$1C,$18,$1C
	.db	$01,$01,$03,$07,$0C,$18,$10,$00	
	.db	$1E,$1C,$10,$00,$00,$00,$00,$00	


.equ	LCD_USERCHARS_CUANTOS2 = 7
LCD_USERCHARS2:
	.db	$00,$00,$10,$10,$10,$10,$00,$00 ; 0x01 ; Para una barra
	.db	$00,$00,$18,$18,$18,$18,$00,$00 ; 0x02
	.db	$00,$00,$1C,$1C,$1C,$1C,$00,$00 ; 0x03
	.db	$00,$00,$1E,$1E,$1E,$1E,$00,$00 ; 0x04
	.db	$00,$00,$1F,$1F,$1F,$1F,$00,$00 ; 0x05
	
	.db	$00,$00,$1F,$11,$11,$11,$1F,$00 ; 0x06 ; Cuadro con y sin marcar
	.db	$00,$00,$1F,$1B,$15,$1B,$1F,$00 ; 0x07



;--------------------------------------------------------
;	LCD_INICIAR
;--------------------------------------------------------
LCD_INICIAR:

	; Iniciar el LCD 3 veces como recomiendan los fabricantes:
	; ---------------------------------------------------------
	
	ldi	conta,3	
	
lcd_init_bucle:	
	ldi	r16,$30
	rcall	LCD_TX_COMANDO
		
	; Retardo de proceso de comando:200ms
	ldi	r25, 5
	rcall	RETARDO_R25x25ms
	
	dec	conta
	brne	lcd_init_bucle

	; Activar modo de 2 lineas:
	ldi	r16,$38
	rcall	LCD_TX_COMANDO
	
	; Borrar pantalla:
	rcall	LCD_BORRAR
	
	; Activar display y cursor off
	ldi	r16,$0C
	rcall	LCD_TX_COMANDO
	

	ldi	aux, 8*LCD_USERCHARS_CUANTOS
	ldi	ZH, high( LCD_USERCHARS*2 )
	ldi	ZL,  low( LCD_USERCHARS*2 )
	
	rcall	LCD_CARGA_CARACTERES
	
	
	ret
	

	
;--------------------------------------------------------
;  LCD_CARGA_CARACTERES
;
;  (Z) y aux
;  Primer caracter = 0x01
;--------------------------------------------------------
LCD_CARGA_CARACTERES:
	; Cargar caracteres definidos por usuario:
	ldi	r16, 0x48	; Caracter 1 el primero def. x usuario
	rcall	LCD_TX_COMANDO

lcdcrgchar_lp:
	lpm		
	adiw	ZL, 1
	mov	r16, r0
	rcall	LCD_TX_DATO
	
	dec	aux
	brne	lcdcrgchar_lp
	
	
	ret
	

;--------------------------------------------------------
;	LCD_BORRAR
;--------------------------------------------------------
LCD_BORRAR:
	ldi	r16,$01
	rjmp	LCD_TX_COMANDO

;--------------------------------------------------------
;	LCD_SCROLL_ON
;--------------------------------------------------------
LCD_SCROLL_ON:
	ldi	r16,0x07
	rjmp	LCD_TX_COMANDO

;--------------------------------------------------------
;	LCD_SCROLL_OFF
;--------------------------------------------------------
LCD_SCROLL_OFF:
	ldi	r16,0x06
	rjmp	LCD_TX_COMANDO
	

;--------------------------------------------------------
;	LCD_TX_COMANDO
;
; Envia un byte en r16 al LCD como comando
;--------------------------------------------------------
LCD_TX_COMANDO:
	; RS=0, E=0, RW=0, datos = r16
	
	out	PORTA, r16
	
	ser	r16
	out	DDRA,r16
	out	DDRC,r16
	
	ldi	r16, 0x00
	out	PORTC, r16
	
		
	; Dar pulso en E:
	rcall	LCD_PULSO_E
	rcall	LCD_WAIT_BUSY	
	
	ret

	
;--------------------------------------------------------
;	LCD_TX_DATO
;
; Envia un byte en r16 al LCD como caracter
;--------------------------------------------------------
LCD_TX_DATO:
	; RS=1, E=0, RW=0, datos = r16
	out	PORTA, r16
	
	ser	r16
	out	DDRA,r16
	out	DDRC,r16
	
	ldi	r16, 0x01
	out	PORTC, r16
	
	; Dar pulso en E:
	rcall	LCD_PULSO_E
	rcall	LCD_WAIT_BUSY	
	
	ret
	
	
;--------------------------------------------------------
;  LCD_WAIT_BUSY
;
;  Desactivo RAM externa para leer el byte de estado del
;    display:
;
;--------------------------------------------------------
LCD_WAIT_BUSY:
	; Preparar contenido del PORTC: 
	;  A15(CS de RAM)=1
	;  A9=1 R/W
	;  A8=0 RS
	ldi	temp, 0b10000010
	out	PORTC, temp
	ser	temp
	out	DDRC, temp

	; Preparar PORTA como entrada:
	clr	temp
	out	DDRA, temp
	out	PORTA, temp
		
	; POLL hasta que bit 7 sea 0
LCD_WAIT_BUSY_loop:
	
	; E=1
	sbi	PORTD, 5
	
	ldi	r25, 40
	rcall	RETARDO_R25_MAX100us
	
	; Busy??
	in	temp2, PINA
	
	; E=0
	cbi	PORTD, 5

	; Fin?
	sbrs	temp2, 7
	rjmp	LCD_WAIT_BUSY_fin	; Solo si bit 7 = 0
	
	; retardo y a seguir:
	ldi	r25, 40
	rcall	RETARDO_R25_MAX100us
	rjmp	LCD_WAIT_BUSY_loop


LCD_WAIT_BUSY_fin:	
	; Y dejar E=0:
	cbi	PORTD, 5
	
	ret

;--------------------------------------------------------
;	LCD_PULSO_E
;
;  Dar pulso en E: Se supone X es lo q hay en el bus 
;    y que XH=0x8X
;--------------------------------------------------------
LCD_PULSO_E:
	ldi	r25,50
	rcall	RETARDO_R25_MAX100us		; aprox. 1.5us
	
	sbi	PORTD, 5

	ldi	r25,50
	rcall	RETARDO_R25_MAX100us		; aprox. 1.5us

	cbi	PORTD, 5

	ldi	r25,50
	rcall	RETARDO_R25_MAX100us		; aprox. 1.5us
	
	ret
	


;----------------------------------------------------
; LCD_ESCRIBE_CADENA_Z_FLASH
;
;  Escribe una cadena de bytes terminada en 0, 
;   almacenada en flash en direccion Z
;----------------------------------------------------
LCD_ESCRIBE_CADENA_Z_FLASH:
	push	r16
	push	r0
	
lcd_esc_cad_bucl:
	; Cargar un byte de flash:
	lpm
	mov	r16,r0
	tst	r16			; 0=fin de cadena
	breq	lcd_esc_cad_fin
	
	rcall	LCD_TX_DATO		; Escribir caracter
	; Incrementar contador Z:
	adiw	ZL,1
	rjmp	lcd_esc_cad_bucl


lcd_esc_cad_fin:
	pop	r0
	pop	r16
	ret

;----------------------------------------------------
;  LCD_SET_CURSOR_POS_R16
;
;	Mueve el cursor a la posicion del LCD "r16"
;
; 1º linea: $00 a $0F
; 2º linea: $40 a $4F
;----------------------------------------------------
LCD_SET_CURSOR_POS_R16:
	push	r16
	ori	r16,$80		;instruccion : 0b1XXXXXX
	
	rcall	LCD_TX_COMANDO
	
	pop	r16
	ret


;----------------------------------------------------
;           LCD_ESCRIBE_R16_HEX
;
;  Escribe dos digitos en hexadecimal de valor (r16)
;----------------------------------------------------
LCD_ESCRIBE_R16_HEX:
	push	r17
	push	r16

	mov	r17,r16	; Guardar byte a mostrar en r16

	; Nibble superior:	
	swap	r17
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII
	rcall	LCD_TX_DATO
	
	; Nibble inferior:
	pop	r16
	push	r16
	mov	r17,r16	; Guardar byte a mostrar en r16

	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII
	rcall	LCD_TX_DATO

	pop	r16
	pop	r17
	ret

;--------------------------------------------------------
; Rutina auxiliar: Modifica el valor en r17 
;--------------------------------------------------------
LCD_LOW_NIBBLE_R17_2_R16_ASCII:
	push	r18
	
	andi	r17,0x0F
	; Tratar como si fuese digito 0-9:
	ldi	r16,'0'
	add	r16,r17
	
	; Ver si se trata de A-F:
	ldi	r18,10
	cp	r17,r18
	brmi	fin_r17_2_r16_asc	; Saltar si NUM-10 < 0 -> NUM < 10
	
	; Es una letra:	
	subi	r17,10
	ldi	r16,'A'
	add	r16,r17	


fin_r17_2_r16_asc:
	pop	r18
	ret



;-------------------------------------------------------
;           LCD_ESCRIBE_X
;
;  Escribe desde memoria (int / ext) (X), acaba con 0x00
;-------------------------------------------------------
LCD_ESCRIBE_X:
;	rcall	HAB_EXTRAM
	ld	r16, X+
	
	tst	r16
	breq	LCD_ESCRIBE_X_FIN
	
;	rcall	DESHAB_EXTRAM
	rcall	LCD_TX_DATO
	
	rjmp	LCD_ESCRIBE_X
	
LCD_ESCRIBE_X_FIN:
	ret
	
	
;-------------------------------------------------------
;	Escribe texto de X, offset en r17, hasta un 
;   maximo de r18 caracteres o 0x00 en cadena
;-------------------------------------------------------
LCD_ESC_X_OFF_R17_MAX_R18:
	clr	r0
	add	XL, r17
	adc	XH, r0

LCD_ESC_X_OFF_R17_MAX_R18_lp:
	ld	r16, X+
	
	tst	r16
	breq	LCD_ESCRIBE_X_OFF_FIN

	push	r18	
	rcall	LCD_TX_DATO
	pop	r18
	
	dec	r18
	brne 	LCD_ESC_X_OFF_R17_MAX_R18_lp
	
	
LCD_ESCRIBE_X_OFF_FIN:
	ret




;-------------------------------------------------------
;  LCD_ESCRIBE_ABAJO_DERECHA_X
;
;  Escribe alineado abajo a la derecha el texto en (X)
;-------------------------------------------------------
LCD_ESCRIBE_ABAJO_DERECHA_X:
	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )
	rcall	STRLEN
	ldi	r17, 0x40
	ldi	r18, ANCHO_LCD
	add	r17, r18
	sub	r17, r16
	mov	r16, r17
	rcall	LCD_SET_CURSOR_POS_R16
	
	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )
	rjmp	LCD_ESCRIBE_X


; -----------------------------------
;  LCD_BORRAR_LINEA
;
;   Borra linea actual
; -----------------------------------
LCD_BORRAR_LINEA:	
	ldi	r18, ANCHO_LCD
lcdbrlin_lp:		
	ldi	r16, ' '
	push	r18
	
	rcall	LCD_TX_DATO
	
	pop	r18
	
	dec	r18
	brne	lcdbrlin_lp
	
	ret
