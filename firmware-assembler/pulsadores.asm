;-----------------------------------------------------------------------------
; FICHERO: pulsadores.asm
;
; DESCRIPCION: 
;
;
;-----------------------------------------------------------------------------


; Son los mismos numeros que los comandos:
; ------------------------------------------
.equ	PULS_VOL_MAS	= 8
.equ	PULS_VOL_MENOS	= 7
.equ	PULS_ATRAS	= 6
.equ	PULS_ALANTE	= 5
.equ	PULS_STOP	= 4
.equ	PULS_PLAY	= 3

; Estos ya no son pulsadores, pero son comandos accesibles a 
;  traves del mando a distancia: (o botones si alguien los quiere poner)
.equ	PULS_SUBE_DIR	= 2



;--------------------------------------------------------
;  LEE_PULSADORES
;
;  Devuelve en COMANDO_USUARIO la tecla pulsada o 0x00 si ninguna
;--------------------------------------------------------
LEE_PULSADORES:
	rcall	DESHAB_EXTRAM
	; Salida a cero para detectar pulsadas:
	cbi	PULSADORES_PORT, PULS_NUM_PIN
	
	; Sacar en PORTA todos a 1 pero con res. tiron nada mas:
	clr	r16
	out	DDRA, r16
	ser	r16
	out	PORTA, r16
	
	; Retardo de algunos micros:
	ldi	r25, 10
	rcall	RETARDO_R25_MAX100us
	
	in	r17, PINA
	
	; Ahora, habra un cero en el pulsador que este activo:
	
	ldi	r16, 0x08
	
lee_tecl_lp:
	sec
	rol	r17
	brcc	lee_tecl_fin	; Si, era un cero: SALIR con r16=tecla
	
	; No estaba pulsada:
	dec	r16
	brne	lee_tecl_lp
	
	; Si no habia ninguna, saldra r16 con cero:
		
	
lee_tecl_fin:

	; Salida a uno de nuevo
	sbi	PULSADORES_PORT, PULS_NUM_PIN
	
	ret
	