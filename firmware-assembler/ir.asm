;-----------------------------------------------------------------------------
; FICHERO: ir.asm
;
; DESCRIPCION: Mando a distancia
;
;-----------------------------------------------------------------------------



; --------------------------------------------------------
;	Int. del TIMER 0
;
; Usada para recoger los codigos del mando a distancia
;
; Se llama cada 304 us, la mitad del periodo de bit de IR
; 
; --------------------------------------------------------
TMR0_INT:
	push	r16

	in	r16, SREG
	push	r16

	in	r16, MCUCR
	push	r16

	push	r17
	push	r18
	
	; Quitar ram ext:
	;  Esto evita pequeños glitchs en ALE aunque accedamos 
	;   a RAM interna, lo que modificaria el valor en el 
	;   registro 74HC373 externo !!
	ldi	r16, 0x00
	out	MCUCR,r16
	

	; Salir si: 
	;  - Ni estamos recibiendo
	;  - Ni se detecta nada nuevo:
	
	; Leer bit de entrada del IR:
	lds	r16, IR_RX_BITS	
	tst	r16
	brne	tmr0_si
	
	sbic	IR_PIN, IR_NUM_PIN
	rjmp	tmr0_fin	; Es = 1 -> No detectado nada
	
tmr0_si:

	; Procesar:
	sbic	IR_PIN, IR_NUM_PIN
	rjmp	tmr0_proc_0
	
	; Si hay señal:
	; -------------
tmr0_proc_1:
	clr	r16
	sts	IR_CUENTA_0, r16

	lds	r16, IR_BITS_1
	
	; Si es el inicio, incrementar contador de bits:
	tst	r16
	brne	tmr0_no_prim
	
	lds	r17, IR_RX_BITS
	inc	r17	
	sts	IR_RX_BITS, r17
tmr0_no_prim:
	
	inc	r16
	sts	IR_BITS_1, r16

	rjmp	tmr0_fin
	
	; No hay señal
	; -------------
tmr0_proc_0:
	; Final de rafaga de unos?
	lds	r16, IR_BITS_1
	tst	r16
	breq	tmr0_no_fin1s
	
	; Es el final: Ver si era corto o largo (0/1):
	cpi	r16, 3

	lds	r16, IR_CODE+0
	lds	r17, IR_CODE+1
	
	ror	r17	; LSB
	ror	r16
	
	sts	IR_CODE+0, r16
	sts	IR_CODE+1, r17
	
	
	; Dejar limpio contador para siguiente bit:	
	clr	r17
	sts	IR_BITS_1, r17

	; Ya hay 16 bits?
	; Es mejor que solo acabe un codigo cuando ya 
	;  se vean mucho rato la señal a cero :-)
;	lds	r17, IR_RX_BITS
;	cpi	r17, 16
;	breq	tmr0_analiza

	
	rjmp	tmr0_fin

tmr0_no_fin1s:
	; Si ya llevamos un rato con ceros, dar por terminada
	;  la secuencia y procesar:
	lds	r16, IR_CUENTA_0
	inc	r16
	sts	IR_CUENTA_0, r16
	
	cpi	r16, 6
	brne	tmr0_fin
	
tmr0_analiza:
	; Desplazar el codigo los bits que falten hasta 16:
	lds	r18, IR_RX_BITS
	lds	r16, IR_CODE+0
	lds	r17, IR_CODE+1
	
tmr0_despl:
	cpi	r18, 16
	breq	tmr0_fin_despl

	ror	r17	; LSB
	ror	r16

	inc	r18
	rjmp	tmr0_despl

	
tmr0_fin_despl:
	com	r16
	com	r17
	
	; En IR_CODE_FINAL queda el codigo de la tecla:
	; -----------------------------------------------

	; Ok, dar por bueno en codigo recibido:	
	sts	IR_CODE_FINAL+0, r16
	sts	IR_CODE_FINAL+1, r17
	

	; Reponer variables para proxima recepcion
	clr	r16	
	sts	IR_CUENTA_0, r16
	sts	IR_BITS_1, r16
	sts	IR_RX_BITS, r16
		
	
			

tmr0_fin:
	; Reponer valores y salir:
	ldi	r16, ( 255  - 37 )
	out	TCNT0, r16
	
	ldi	r16, 0x02
	out	TIFR, r16


	pop	r18
	pop	r17

	pop	r16
	out	MCUCR, r16
	
	pop	r16
	out	SREG, r16
	pop	r16
	reti




; --------------------------------------------------------
;   IR_PROCESA_CODIGO
;
;  Transforma los codigos de teclas en comandos
; --------------------------------------------------------
IR_PROCESA_CODIGO:
	lds	r1, IR_CODE_FINAL+0
	lds	r2, IR_CODE_FINAL+1
	
	tst	r1
	brne	ir_proc_cod_si
	tst	r2
	brne	ir_proc_cod_si
	
	ret
	
	
ir_proc_cod_si:
	; Borrar codigo en RAM
	clr	r18
	sts	IR_CODE_FINAL+0, r18
	sts	IR_CODE_FINAL+1, r18


	; Y ver que comando es el codigo actual:
	ldi	XH, 0x00
	ldi	XL, 0x10	; Posicion en eeprom de los codigos
	ldi	ZH, high ( 2 * IR_TABLA_COD_CMDS )
	ldi	ZL,  low ( 2 * IR_TABLA_COD_CMDS )
	
ir_proc_cod_lp:
	lpm
	adiw	ZL, 1
	
	tst	r0
	breq	ir_proc_cod_fin	
	
	; Leer de eep. el codigo para comparalo. 
	; Guardarlo en r3, r4
	rcall	LEE_EEPROM
	mov	r3, r16	
	rcall	LEE_EEPROM
	mov	r4, r16	


	; El codigo estaba en r1:r2
	cp	r1, r3
	brne	ir_proc_cod_lp
	cp	r2, r4
	brne	ir_proc_cod_lp
	
	; Si, esta es:
	sts	COMANDO_USUARIO, r0
	
	; Si no es de volumen, poner un retardo:
	mov	r16, r0
	cpi	r16, PULS_VOL_MAS
	breq	ir_proc_cod_fin
	cpi	r16, PULS_VOL_MENOS
	breq	ir_proc_cod_fin
	
	
	ldi	r16, VALOR_IR_TIMEOUT
	sts	IR_TIMEOUT, r16	

ir_proc_cod_fin:

	ret
	

; Aqui estan por orden los numeros de comandos en el mismo orden 
;  en que en eeprom estan los codigos a partir de 0x0010
IR_TABLA_COD_CMDS:
	.db	PULS_PLAY,     PULS_PLAY
	.db	PULS_STOP,     PULS_STOP
	.db	PULS_ALANTE,   PULS_ALANTE
	.db	PULS_ATRAS,    PULS_ATRAS
	.db	PULS_VOL_MAS,  PULS_VOL_MAS 
	.db 	PULS_VOL_MENOS,PULS_VOL_MENOS
	.db	PULS_SUBE_DIR, PULS_SUBE_DIR
FIN_IR_TABLA_COD_CMDS:
	.db	0,0	; Fin de lista




; --------------------------------------------------------
;  LEE_EEPROM
;
;  EEP[X++] -> r16
; --------------------------------------------------------
LEE_EEPROM:   SBIC    EECR,1        
              RJMP    LEE_EEPROM      
              OUT     EEARL,XL     
              OUT     EEARH,XH
              SBI     EECR,0        
              IN      R16,EEDR      
              ADIW    XL,0x1         
              RET

              
; --------------------------------------------------------
;  GRB_EEPROM
;
;  EEP[X++] <- r16
; --------------------------------------------------------
GRB_EEP:     SBIC    EECR,1   
             RJMP    GRB_EEP 
             OUT     EEARL,XL  
             OUT     EEARH,XH
             OUT     EEDR,R16    ;EEPROM data register 
             SBI     EECR,2         
             SBI     EECR,1
             ADIW    XL,0x1     
             RET
