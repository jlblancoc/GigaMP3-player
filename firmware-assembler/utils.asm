;-----------------------------------------------------------------------------
; FICHERO: utils.asm
;
; DESCRIPCION: 
;
;
;-----------------------------------------------------------------------------

;------------------------------------------------
;  GENERAR_PSEUDOALEAT
;------------------------------------------------
GENERAR_PSEUDOALEAT:
	; Generar valor pseudoaleatorio	
	in	r1, TCNT0
	ld	r16, Y+
	eor	r16, r1
	ret

;--------------------------------------------------------
; R16_MODULO_R2
; 
;  Devuelve en r16 = r16 mod r2
;--------------------------------------------------------
R16_MODULO_R2:
	cp	r16, r2
	brlo	R16_MODULO_R2_fin
	
	; Restar:	
	sub	r16, r2
	rjmp	R16_MODULO_R2
	
R16_MODULO_R2_fin:
	ret

;--------------------------------------------------------
; HAB_EXTRAM
;--------------------------------------------------------
HAB_EXTRAM:
	push	r16
	
	ldi	r16,0x80
HAB_EXTRAM_IN:
	out	MCUCR,r16
	
	pop	r16
	ret

;--------------------------------------------------------
; DESHAB_EXTRAM
;--------------------------------------------------------
DESHAB_EXTRAM:
	push	r16
	clr	r16
	rjmp	HAB_EXTRAM_IN

;--------------------------------------------------------
; COPIAR_X_2_Y_R18_BYTES
;--------------------------------------------------------
COPIAR_X_2_Y_32bits:
	ldi	r18, 4
COPIAR_X_2_Y_R18_BYTES:
	ld	r16, X+
	st	Y+,r16
	
	dec	r18
	brne	COPIAR_X_2_Y_R18_BYTES
	
	ret
	
	
;--------------------------------------------------------
;  Devuelve en r16 longitud de la cadena hasta 0x00 (no inc)
;    apuntada por X:
;--------------------------------------------------------
STRLEN:
	clr	r16
	
STRLEN_LOOP:
	ld	r17, X+
	tst	r17
	breq	STRLEN_FIN
	
	inc	r16
	
	rjmp	STRLEN_LOOP
	
STRLEN_FIN:
	ret

	
; ----------------------------------------------------
;  CALCULA_MINUTOS
;
; Devuelve en r1:r0 ( MSB:LSB ) el numero de minutos enteros
;  a partir del numero de segundos en r18:r19
; En r19 deja el numero de segundos que sobran
; ----------------------------------------------------
CALCULA_MINUTOS:
	clr	r0

	clr	r1
	clr	r2
	ldi	r16, 60
calcmins_lp:
	; Es r18:r19 > 60 ?
	tst	r18
	brne	calcmins_rst_60	; Si es > 256 si lo es :-)
	
	cp	r19, r16
	brsh	calcmins_rst_60
	
	; No, se acabo:
	ret
	

	; Si es > 60:
calcmins_rst_60:	
	; Incrementar numero de minutos
	ldi	r17, 0x01
	add	r2, r17
	adc	r1, r0
	
	; Y restar al num. de segundos:
	sub	r19, r16
	sbc	r18, r0
	
	rjmp	calcmins_lp
	
	
		
	


	ret
