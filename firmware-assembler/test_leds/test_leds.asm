
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
		
	

	; LED como salida en PD5
	sbi	DDRD, 5
	
loop:
	cbi	PORTD, 5
	rcall	RETARDO
	rcall	RETARDO
	rcall	RETARDO
	rcall	RETARDO
	
	sbi	PORTD, 5
	rcall	RETARDO
	rcall	RETARDO
	rcall	RETARDO
	rcall	RETARDO
	
	rjmp	loop
	



; --------------------------------------------------------
;	RETARDO_R25
; --------------------------------------------------------
RETARDO_R25:
	dec	r25
	brne	RETARDO_R25
	ret
	


RETARDO:
	ldi	r18, 0xFF
	
RETARDO_LOOP:
	ldi	r25, 0
	rcall	RETARDO_R25
	dec	r18
	brne	RETARDO_LOOP
	
	ret

	
	
	


