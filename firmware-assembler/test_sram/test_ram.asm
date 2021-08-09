
	.include "8515def.inc"


.equ	PILA	= 0x7F


; RAM --------------------------
	.DSEG
		
	.org	PILA+1
	
	


	.CSEG
	
	.org	0
	
	rjmp	RESET
	reti
	reti



RESET:
	; Poner PILA:
	ldi	r16, PILA
	out	SPL, r16
	
	; RAM externa:
	ldi	r16, 0xC0
	out	MCUCR, r16


	; LED como salida en PD5
	sbi	DDRD, 5
	rcall	PARPADEA_LED
	rcall	PARPADEA_LED
	rcall	PARPADEA_LED
	
	
	; Prueba de RAM
	ldi	r16, 0x69
	sts	0x1000, r16
	
	rcall	RETARDO
	
	lds	r17, 0x1000
	
	cp	r16, r17
	breq	FIN_WAPO
	rjmp	FIN_PENCO
			
	
	
FIN_PENCO:
	cbi	PORTD, 5
	rjmp	FIN_PENCO
	
	
FIN_WAPO:	
	rcall	PARPADEA_LED
	rjmp	FIN_WAPO	
	
	
	

	
; --------------------------------------------------------
; Parpadea el led 1 vez
; --------------------------------------------------------
PARPADEA_LED:
	cbi	PORTD, 5
	rcall	RETARDO_100ms
	
	sbi	PORTD, 5
	rcall	RETARDO_100ms
	ret
	



; --------------------------------------------------------
;	RETARDO_R25
; --------------------------------------------------------
RETARDO_R25:
	dec	r25
	brne	RETARDO_R25
	ret
	


; --------------------------------------------------------
;	RETARDO_100ms
; --------------------------------------------------------
RETARDO_100ms:
	ldi	r19,5
	
RET_100ms_loop:
	rcall	RETARDO	
	dec	r19
	brne	RET_100ms_loop
	ret
	
	

; --------------------------------------------------------
; --------------------------------------------------------
RETARDO:
	ldi	r18, 0xFF
	
RETARDO_LOOP:
	ldi	r25, 0
	rcall	RETARDO_R25
	dec	r18
	brne	RETARDO_LOOP
	
	ret

	
	
	


