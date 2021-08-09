;----------------------------------------------------
; FICHERO: Textos.asm
;
; DESCRIPCION: Para declarar cadenas de texto
;
; Jose Luis Blanco Claraco @ 2001-2002
;----------------------------------------------------


PRIMERA_PANTALLA1: 
;	.db $01,$02,"   GigaMP3  ",$01,$02,0,0
	.db $01,$02,"       GigaMP3    ",0,0
PRIMERA_PANTALLA2: 
;	.db $03,$04," JLBC @2003 ",$03,$04,0,0
	.db $03,$04," Jose Luis B. '03 ",0,0

STR_CHK_RAM: 
	.db "32 Kb RAM...",0,0

STR_TSTHD: 
	.db "Reset disco...",0,0
	
STR_OK: 
	.db "OK",0,0
STR_ER: 
	.db "ER",0,0
	
STR_ATAERR:
	.db "ATA ERROR:",0,0 
	
STR_FATERR:
	.db "FAT ERROR!",0,0 

STR_USB:
	.db "** Modo USB **",0,0

STR_NO_DIR:
	.db "No direct. valido!",0,0
STR_NO_MP3s:
	.db "No hay fichs MP3!", 0
	
; Textos de menus:
; --------------------------
.equ	NUM_MENUS	=  5

MNU_1: .db "Repr. directorio",0,0
MNU_2: .db "Repr. dir. aleat",0,0
MNU_3: .db "Mejorar bajos",0
MNU_4: .db "Virtual sorround",0,0
MNU_5: .db "Config. mando",0


