;-----------------------------------------------------------------------------
; FICHERO: Conversiones.asm
;
; DESCRIPCION: Implementa rutinas de conversiones varias y utilidades.
;
; RUTINAS:
; --------
;
;  - BYTE2ASCII_R18: Coge un numero de 8 bits con signo 
;      de r18 y lo guarda en DECIMAL en memoria, en X.
;
;  - R16_2_X_HEX: Escribe dos digitos en hexadecimal en r16 en X
;
;  - HEX_X_2_BIN: Pasa 2 bytes ascii en hexadecimal de X, y devuelve 
;	el valor en r16
;
;  - HEX2BIN:	Pasa un caracter ascii en r16, y lo devuelve como valor
;      numerico en el nibble bajo en r16
; 	Al salir: C=0 si todo bien, C=1 si no es digito hex.
;
;  - CERO_2_X: Guarda un byte a 0 en X e incrementa este.
;
;  - TABLA_SEGUN_R16_A_Z:  Pone en Z direccion segun la tabla en (Z) y temp
;
;-----------------------------------------------------------------------------


;--------------------------------------------------------
;  - BYTE2ASCII_R18: Coge un numero de 8 bits sin signo 
;      de r18 y lo guarda en ascii en memoria, en X.
;
;  MOD: Siempre imprime las decenas
;-----------------------------------------------------------------------------
BYTE2ASCII_R18:
	; Centenas ----------------------
	clr	r17
byte2ascii_cents_loop:
	mov	temp,r18
	subi	temp,100
	brmi	byte2ascii_cents_fin	; Es negativo?
	inc	r17
	mov	r18,temp		; Se va descontando de r18
	rjmp	byte2ascii_cents_loop

byte2ascii_cents_fin: 
	; En r17: No imprimirlo si es cero:
	tst	r17
	breq	byte2ascii_cents_0
	rcall	LCD_R17_BIN2BCD
byte2ascii_cents_0:
	
	; Decenas ----------------------
	clr	r17
byte2ascii_decs_loop:
	mov	temp,r18
	subi	temp,10
	brmi	byte2ascii_decs_fin	; Es negativo?
	inc	r17
	mov	r18,temp		; Se va descontando de r18
	rjmp	byte2ascii_decs_loop

byte2ascii_decs_fin: 
	; En r17:
	; En r17: No imprimirlo si es cero:
;	tst	r17
;	breq	byte2ascii_decs_0
	rcall	LCD_R17_BIN2BCD
byte2ascii_decs_0:
	
	; Unidades ----------------------
	mov	r17,r18			; Ya solo quedan unidades
	rcall	LCD_R17_BIN2BCD



	ret


;----------------------------------------------------
;           R16_2_X_HEX
;
;  Escribe dos digitos en hexadecimal en r16 en X
;----------------------------------------------------
R16_2_X_HEX:
	push	r16
	mov	r17,r16	; Guardar byte a mostrar en r16

	; Nibble superior:	
	swap	r17
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII
	st	X+,r16
	
	; Nibble inferior:
	pop	r16
	push	r16
	mov	r17,r16	; Guardar byte a mostrar en r16

	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII
	st	X+,r16

	pop	r16
	ret



;----------------------------------------------------
;  - HEX_X_2_BIN: Pasa 2 bytes ascii en hexadecimal 
;          de X, y devuelve el valor en r16
;----------------------------------------------------
HEX_X_2_BIN:
	; Nibble alto:
	ld	r16,X+
	rcall	HEX2BIN
	
	mov	aux,r16

	; Nibble bajo
	ld	r16,X+
	rcall	HEX2BIN
	
	; Unir nibbles:
	swap	aux
	andi	aux,0xF0
	
	andi	r16,0x0F
	or	r16,aux
	ret
	



;----------------------------------------------------
;  - HEX2BIN:	Pasa un caracter ascii en r16, y lo 
; 	devuelve como valor numerico en el nibble 
;	bajo en r16. Si es A..F debe estar en mayusculas
;
; Al salir: C=0 si todo bien, C=1 si no es digito hex.
;----------------------------------------------------
HEX2BIN:
	cpi	r16,'0'
	brlo	HEX2BIN_ERR
	
	cpi	r16,'9'
	brge	HEX2BIN_NO_NUM

	; Es un digito numerico:
	subi	r16,'0'

	clc
	ret
	
HEX2BIN_NO_NUM:
	cpi	r16,'A'
	brlo	HEX2BIN_ERR

	cpi	r16,'G'
	brge	HEX2BIN_ERR
	
	; Es digito entre 'A' y 'F':
	subi	r16,('A'-10)
	clc
	ret
	
HEX2BIN_ERR:
	; ERROR
	clr	r16
	sec	
	ret


	
	
	
;----------------------------------------------------
;  CERO_2_X
;
;  Guarda un byte a 0 en X e incrementa este.
;
;  Implementado como rutina por que se usa muchas veces
;    en el codigo.
;----------------------------------------------------
CERO_2_X:
	ldi	temp,0
	st	X+,temp
	ret	
	
	
	


;----------------------------------------------------
; LCD_R17_BIN2BCD : 2 DIGITOS BCD
;  Pasa el numero en r17 a decimal ASCII y lo guarda en X
;----------------------------------------------------
LCD_R17_BIN2BCD:
	push	r18
	push	r19
	
	
	; Dividir r17 entre 10, resultado en r19
	clr	r19
	mov	r18,r17	; Usar copia de r17 en r18
	
lcd_r17_bin2bcd_buc1:
	subi	r18,10
	brmi	lcd_r17_bin2bcd_buc1_fin
	inc	r19
	rjmp	lcd_r17_bin2bcd_buc1


lcd_r17_bin2bcd_buc1_fin:
	ldi	temp,10
	add	r18,temp	; En r18 tenemos las unidades

	tst	r19		; Quitar 0 en decenas si lo hay...
	breq	lcd_r17_bin2bcd_no_decenas
		
	; pasar las decenas en r19 a ASCII:
	push	r17
	mov	r17,r19
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+,r16	
	pop	r17
	
lcd_r17_bin2bcd_no_decenas:
	
	; pasar las unidades en r19 a ASCII:
	mov	r17,r18
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+,r16

	pop	r19
	pop	r18
	ret


; -------------------------------------------------
;   R1_R2_Decimal
;
;   R1 = HIGH BYTE
;   R2 = LOW BYTE
;
;    Lo escribe en ram en (X)
; -------------------------------------------------
R1_R2_Decimal:
	clr	r0
	
; -------------------------------------------------
;   R0_R1_R2_Decimal
;
;   R0 = +High
;   R1 = HIGH BYTE
;   R2 = LOW BYTE
;
;    Lo escribe en ram en (X)
; -------------------------------------------------
R0_R1_R2_Decimal:

	clr	r7	; Indicador de todavia todos ceros


	; Empezar por las cientos de miles:
	; -------------------------------------
r1r2dec_haz_100000:
	clr	r17	; Contador para digito

	ldi	r18, 0x01 ; MSB de 100.000
	ldi	r19, 0x86
	ldi	r20, 0xA0
		
r1r2dec_rst_100000:
	; Es < 100000 ? Hacer la resta y segun el signo...
	mov	r4, r0
	mov	r5, r1	
	mov	r6, r2

	sub	r6, r20
	sbc	r5, r19
	sbc	r4, r18
	
	; CARRY=1  -> No
	brcs	r1r2dec_fin_100000
	
	; Si:
	mov	r0,r4
	mov	r1,r5
	mov	r2,r6
	
	inc	r17
	rjmp	r1r2dec_rst_100000
	
r1r2dec_fin_100000:
	
	tst	r17
	breq	r1r2dec_haz_10000

	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+, r16
	ser	r16
	mov	r7, r16
	


	; Empezar por las decenas de millas:
	; -------------------------------------
r1r2dec_haz_10000:
	clr	r17	; Contador para digito

	ldi	r18, 0
	ldi	r19, high ( 10000 ) 
	ldi	r20,  low ( 10000 ) 
		
r1r2dec_rst_10000:
	; Es < 100000 ? Hacer la resta y segun el signo...
	mov	r4, r0
	mov	r5, r1	
	mov	r6, r2

	sub	r6, r20
	sbc	r5, r19
	sbc	r4, r18
	
	; CARRY=1  -> No
	brcs	r1r2dec_fin_10000
	
	; Si:
	mov	r2,r6
	mov	r1,r5
	mov	r0,r4
	
	
	inc	r17
	rjmp	r1r2dec_rst_10000
	
r1r2dec_fin_10000:
	sbrc	r7, 0
	rjmp	r1r2dec_si_10000

	tst	r17
	breq	r1r2dec_haz_1000

r1r2dec_si_10000:

	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+, r16
	ser	r16
	mov	r7, r16
	
		
	; Los miles:
	; -------------------------------------
r1r2dec_haz_1000:
	clr	r17	; Contador para digito

	ldi	r18, high ( 1000 ) 
	ldi	r19,  low ( 1000 ) 
		
r1r2dec_rst_1000:
	; Es < 1000 ?
	mov	r4, r1	
	mov	r5, r2

	sub	r5, r19
	sbc	r4, r18
	
	; CARRY=1  -> No
	brcs	r1r2dec_fin_1000
	
	; Si:
	mov	r2,r5
	mov	r1,r4
	
	inc	r17
	rjmp	r1r2dec_rst_1000
	
r1r2dec_fin_1000:
	sbrc	r7, 0
	rjmp	r1r2dec_si_1000

	tst	r17
	breq	r1r2dec_haz_100

r1r2dec_si_1000:
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+, r16
	ser	r16
	mov	r7, r16


	; Centenares:
	; -------------------------------------
r1r2dec_haz_100:
	clr	r17	; Contador para digito

	ldi	r18, high ( 100 ) 
	ldi	r19,  low ( 100 ) 
		
r1r2dec_rst_100:
	; Es < 100 ?
	mov	r4, r1	
	mov	r5, r2

	sub	r5, r19
	sbc	r4, r18
	
	; CARRY=1  -> No
	brcs	r1r2dec_fin_100
	
	; Si:
	mov	r2,r5
	mov	r1,r4
	
	inc	r17
	rjmp	r1r2dec_rst_100
	
r1r2dec_fin_100:
	sbrc	r7, 0
	rjmp	r1r2dec_si_100

	tst	r17
	breq	r1r2dec_haz_10

r1r2dec_si_100:
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+, r16
	ser	r16
	mov	r7, r16



	; Decenas
	; -------------------------------------
r1r2dec_haz_10:
	clr	r17	; Contador para digito

	ldi	r19, 10
		
r1r2dec_rst_10:
	; Es < 10 ?
	cp	r2, r19
	brlt	r1r2dec_fin_10

	sub	r2, r19
	
	inc	r17
	rjmp	r1r2dec_rst_10
	
r1r2dec_fin_10:
	sbrc	r7, 0
	rjmp	r1r2dec_si_10

	tst	r17
	breq	r1r2dec_haz_1

r1r2dec_si_10:
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+, r16
	ser	r16
	mov	r7, r16


	; Y las unidades que queden:
r1r2dec_haz_1:
	mov	r17, r2
	
	rcall	LCD_LOW_NIBBLE_R17_2_R16_ASCII	
	st	X+, r16



	ret
	
