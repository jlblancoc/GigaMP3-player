;----------------------------------------------------
; FICHERO: Retardos.asm
;
; DESCRIPCION: Implementa distintas funciones para
;		 generar retardos de distinta longitud
;
; Jose Luis Blanco Claraco @ 2001-2002
;----------------------------------------------------



;----------------------------------------------------
;  RETARDO_R25_MAX100us  (r25: 1,2,...,255,0)
;
;----------------------------------------------------
RETARDO_R25_MAX100us:
	dec	r25
	brne	RETARDO_R25_MAX100us
	ret

;----------------------------------------------------
;  RETARDO_R25_MAX25ms   (r25: 1,2,...,255,0)
;
;----------------------------------------------------
RETARDO_R25_MAX25ms:
	push	r25
	ldi	r25,0
	rcall	RETARDO_R25_MAX100us
	pop	r25
	
	dec	r25
	brne	RETARDO_R25_MAX25ms
	ret

;----------------------------------------------------
;  RETARDO_R25x25ms
;
;    Retarda r25 veces 25 ms
;----------------------------------------------------
RETARDO_R25x25ms:
	push	r25

ret_25x_bucl:	
	push	r25
	ldi	r25,0
	rcall	RETARDO_R25_MAX25ms
	pop	r25
	dec	r25
	brne	ret_25x_bucl	
	
	pop	r25
	ret