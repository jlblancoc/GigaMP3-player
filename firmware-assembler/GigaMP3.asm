;-----------------------------------------------------------------------------
; FICHERO: GIGAMP3.asm
;
; DESCRIPCION: Modulo principal del reproductor.
;   Se definen los simbolos, constantes, etc... y se llaman a los diversos
;    modulos.
;
;-----------------------------------------------------------------------------

; Declaraciones estandar para este micro AT90S8515:
.include "8515def.inc"

; --------------------------------------------------------
; 	Declaraciones  globales
; --------------------------------------------------------

.equ  	PILA   			= 0x25F
.equ	ATA_TEMP_BUF_TAM	= 128
.equ	FAT_MAX_FILENAME	= 80		; Max. tam. de nombre de fich.
.equ	ANCHO_LCD		= 20		; Tamaño del LCD
.equ	VALOR_EXPL_LCD_RET      = 8		; Retardo hasta q texto empiza a deslizar

.equ	VALOR_IR_TIMEOUT	= 1		; Retardo en teclas tras una tecla IR no de volumen

.equ	PASO_VOLUMEN		= 4		; Cuando baja una pulsacion

; 32 secs = 16 kb = 1 segundo de buffer (a 128kbps)
.equ	SECTORES_MP3_EN_RAM 	= 32		; Numero de sectores MP3 que se guardan 
						;  en buffer de envio
						
.equ	ENVIOS_MP3_HASTA_CARGAR = 10* (512/32)	; Numero de envios de 32 bytes al decod. MP3
						;  del bufer circular hasta que se vuelvan a 
						;  cargar sectores nuevos.
						; MAXIMO = 13 * ...

.equ	FRECUENCIA_DISPLAY	= 0x7000	; Frecuencia de refresco de display para
						;  el titulo de la cancion, etc.. (En us)


           .DSEG	; Variables en RAM
           .ORG		0x60
                      
; Variables generales
; ------------------------------------------------
STR_BUF:		.BYTE	20	; Buffer para textos en general

          
; Variables del modulo ATA:
; ------------------------------------------------
LBA_DIR:		.BYTE 	4	; Direccion LBA 32 bits a un sector del disco
					; Primer byte = Menos significativo
SECTOR_CNT:		.BYTE	1

ATA_WAITMASK_UNOS:	.BYTE	1
ATA_WAITMASK_CEROS:	.BYTE	1

		; Buffer de lectura mientras RAM externa deshab.           
ATA_TEMP_BUF:		.BYTE	ATA_TEMP_BUF_TAM
; Tamaño de buffer     Tiempo para leer 2 Mbs (consecutivos)
; -----------------    --------------------------------------
;	32			5.86 seg
;	64			5.12 seg
;	128			4.75 seg ...(optimizado) 437 Kb/seg
;	256			4.56 seg  (demasiado grande, no merece la pena?)
;


ATA_RDSECT_CUANTOSBUFS:	.BYTE 	1 	; Variable de la rutina de lectura q dice cuantos 
					;   buffers hay q hacer para terminar la peticion


; Variables del modulo FAT:
; ------------------------------------------------
BPB_SECSPERCLUS:	.BYTE	1	; Sectores por cluster

BPB_CLUS_DIR_RAIZ:	.BYTE	4	; Numero de cluster de root

FAT_PRIMER_SEC_DATOS:	.BYTE	4	; En LBA: primer byte= Menos sig.
FAT_PRIMER_SEC_FAT:	.BYTE	4	; En LBA: primer byte= menos sig

FAT_DIRECT_ACTUAL:	.BYTE	4	; Cluster del directorio actual

			; El numero de sector LBA del sector cargado
			;  en el buffer FAT_SEC_BUFFER, un sector de la FAT.
			; Si se vuelve a consultar el mismo, no hace falta
			;  volver al disco :-)
CACHE_FAT_SECTOR:	.BYTE	4	


; Parametros de la funcion FAT_EXPLORA_DIRECTORIO
; -------------------------------------------------------------
FAT_PETICION:		.BYTE	1	
FAT_PETICION_NUM_ENTR:	.BYTE	1	; Num. de entrada a leer por el explorador FAT32.
FAT_DIRENT_ENCONTRADAS:	.BYTE	1	; Num. de entradas encontradas por el explorador

FATEXPLORER_CLUSACT:	.BYTE	4	; Cluster actual en el directorio

; Directory Entry, recuperada por el FAT_EXPLORA_DIRECTORIO
;  Ya estan procesados campos como el cluster, nombre, etc...
; --------------------------------------------------------------
FATDE_ATTR:		.BYTE	1	; Atributo: Fichero, directorio,...
FATDE_CLUSTER:		.BYTE	4	; El primer cluster del fichero/directorio
FATDE_FILESIZE:		.BYTE	4	; Tamaño en bytes
					; Nombre del fichero, acabado en 0x00
FATDE_FILENAME:		.BYTE	FAT_MAX_FILENAME+1 	
FATDE_FILENAME_CONTA:	.BYTE	1


;  MODO: Explorador de ficheros
; --------------------------------------------------------------
EXPLORER_NUMENTS:	.BYTE	1
EXPLORER_ENTACT:	.BYTE	1
EXPLORER_LCD_OFFSET:	.BYTE	1	; Offset para texto deslizante en display
EXPLORER_LCD_SENTIDO:	.BYTE	1	; 1 o -1 segun hacia donde vaya el texto
EXPLORER_LCD_RETARDO:	.BYTE	1	; Contador hasta que el texto empiece a deslizarse

COMANDO_USUARIO:	.BYTE	1	; Donde se guardan los comandos desde teclado o IR.


;  MODO: Reproduciendo MP3
; --------------------------------------------------------------
REPROD_NUM_FICH_EN_DIR:	.BYTE	1	; Numero del fichero en el directorio o en la lista

REPROD_NUM_FICHS_TOTAL:	.BYTE	1	; Numero de ficheros en total en directorio o lista

	; Cluster actual en el fichero
REPROD_FICH_CLUSTER:	.BYTE	4	

 	; Direccion en el buffer MP3_BUFFER de siguiente byte a 
 	;  enviar al chip decodificador cuando pida mas datos
 	;  es una especie de "cola circular"
REPROD_PTR_ENVIANDO:	.BYTE	2	; MSB primero

	; Puntero en el buffer MP3_BUFFER del siguiente sector
	;  a cargar de disco. No puede pasar de REPROD_PTR_ENVIANDO
	;  asi que se configura un buffer circular :)
REPROD_PTR_CARGAR:	.BYTE	2	; MSB primero

	; Sector actual dentro del cluster actual (0..BPB_SECSPERCLUS)
REPROD_FICH_SECT:	.BYTE	1

	; Contador de bytes enviados QUE QUEDAN
REPROD_BYTES_TX:	.BYTE	4	; LSB primero

ENVIOS_MP3_PARA_CARGA:	.BYTE	1	; Contador de envios de 32 bytes para saber cuando 
					;  volver a cargar sectores

REPROD_EOF:		.BYTE	1	; Indicador de que se ha llegado al final del fichero


; Volumen de reproduccion: 0 = maximo. unidad = -0.5 db
VOLUMEN:		.BYTE	1	


; Flags varios:
REPROD_FLAGS:		.BYTE	1

.equ	REPR_FLAG_SORROUND 	= 0
.equ	REPR_FLAG_BASS 		= 1
.equ	REPR_MODO_LISTA		= 2
.equ	REPR_FLAG_PAUSE		= 3

; MODO MENU:
; ------------------------------------------------
MENU_OPCION:		.BYTE	1




; Codigos del mando a distancia
; ------------------------------------------------
IR_RX_BITS:		.BYTE	1	; Contador de bit actual
IR_BITS_1:		.BYTE	1	; Contador de intervalos a 1, para saber si es corto o largo
IR_CUENTA_0:		.BYTE	1	; Para saber cuando ha terminado la recepcion
IR_TIMEOUT:		.BYTE	1	; Para retrasar entre una tecla y otra

IR_CODE:		.BYTE	2	; Codigo recibido (temporal)

IR_CODE_FINAL:		.BYTE	2	; Codigo recibido final


; Memoria RAM externa:
; ------------------------------------------------
	.ORG	0x400
SECTOR_BUFFER:		.byte	512	; Buffer general
	
FAT_SEC_BUFFER:		.byte	512	; Para cargar un sector de la FAT

MP3_BUFFER:		.byte	512*SECTORES_MP3_EN_RAM
	

; Buffer para una trama recibida por USB
USB_COMANDO: 		.byte	1	
USB_DATA:		.byte	550


; Cola FIFO usada por el creador de listas de reproduccion:
;  Las entradas son clusters de directorios a los que hay 
;  que examinar. Asi se evita usar recursividad y mas cosas:
.equ	TAM_CREARLISTAS_COLA	= 70
					; Numero de directorios max. en cola
CREAR_LST_COLA_DIRS:	.byte	4*TAM_CREARLISTAS_COLA
					
CREAR_LST_COLA_PTR_IN:	.byte   1	; Punteros de indice (1º=0) de entrada y salida en cola
CREAR_LST_COLA_PTR_OUT:	.byte   1	
					; Es cola circular para mayor capacidad !
	
; Listas de reproduccion:
LST_REPR_CUANTOS:	.byte	1	; Numero de entradas en lista
LST_REPR_ORDEN:		.byte	256	; Indices dentro de la lista siguiente:
LST_REPR_LISTA:		.byte	400	; Para que sobre... segun el numero de 
					;  fichs. en cada directorio
	; ------------------------------------------------
	; El fomato de la lista es:
	;
	;  BYTES   CONTENIDO
	; ====================
	;    1       Numero de entradas en este directorio
	;    4       Cluster del directorio 
	;   Nx1	     Numero del fichero en ese directorio
	;
	; ------------------------------------------------


; Para guardar el directorio y la entrada desde donde se empezo
;  a reproducir, y poder volver al pulsar STOP mientras se reproduce:
CONTEXT_EXPL_DIR:	.BYTE	4	; Cluster del direct. actual
CONTEXT_EXPL_ENT:	.BYTE	1	; La entrada en el dir. act.

; Pila de elemento seleccionado en directorios anteriores, para
;  que al volver, se seleccione la carpeta en la que se entro:
PILA_DIRECTORIOS_PTR:	.byte	2	; Puntero en la lista siguiente

	; El numero de entrada:
PILA_DIRECTORIOS_ENT:	.byte	1*100	; Profundidad de hasta 100 directorios !!


	

.def	temp		=r16
.def	conta		=r17
.def	temp2		=r18
.def	aux		=r19

; Señales del modulo LCD
; -------------------------
.equ	PULSADORES_PORT = PORTD
.equ	PULSADORES_DDR  = DDRD
.equ	PULSADORES_PIN  = PIND
.equ	IR_PORT		= PORTD
.equ	IR_DDR		= DDRD
.equ	IR_PIN		= PIND
.equ	IR_NUM_PIN	= 3
.equ	PULS_NUM_PIN	= 4



; Bus IDE
; -------------------------
.equ	IDE_PORT = PORTB
.equ	IDE_DDR  = DDRB
.equ	IDE_RD   = 1	; Los pins
.equ	IDE_WR   = 0


; Señales para VS1001k
; -------------------------
.equ	VS1001_PORT	= PORTB
.equ	VS1001_DDR	= DDRB
.equ	VS1001_PIN	= PINB
.equ	VS1001_DREQ	= 2
.equ	VS1001_BSYNC	= 3
.equ	VS1001_MP3CS	= 4	; Pin SS 

; Señales para USB:
; -------------------
.equ	USB_PORT	= PORTD
.equ	USB_PIN		= PIND
.equ	USB_DDR		= DDRD
.equ	USB_PIN_RD	= 0
.equ	USB_PIN_WR	= 1
.equ	USB_PIN_RXF	= 2
          

;----------------------------------------------------
; 	   Vectores de interrupcion
;----------------------------------------------------
           .CSEG
           .ORG     0x0000
	rjmp	RESET			; RESET
	reti				; INT0
	reti				; INT1
	reti
	reti
	reti
	reti

	rjmp	TMR0_INT		; TIMER 0 OVR.

	reti
	reti				; Byte recibido UART





; --------------------------------------------------------
;
;               	RUTINA DE RESET
;
; --------------------------------------------------------
RESET:	
	ldi	temp,HIGH ( PILA )		; Preparar puntero de pila:
	out	SPH,temp		
	ldi	r16,LOW (PILA ) 		
	out	SPL,temp		 	

	;---------------------------------------------
	; Inicializacion de puertos, modulos, etc...
	;---------------------------------------------
	
	; Valores por defecto en pins (para uso "no alternativo" del pin)
	clr	temp		; PORTC a 0
	out	DDRC,temp
	out	PORTC,temp
	
	
	; Pins del disco duro: IDE_WR y IDE_RD a 1
	sbi	PORTB,0
	sbi	PORTB,1
	sbi	DDRB,0
	sbi	DDRB,1
	
	
	; Pin de LCD_E salida a 0
	sbi	DDRD,  5
	cbi	PORTD, 5
	
	; Pulsadores: Salida a 1 (No detectar pulsaciones)
	sbi	PULSADORES_PORT, PULS_NUM_PIN
	sbi	PULSADORES_DDR,  PULS_NUM_PIN
	
	; IR: Entrada con tiron
	sbi	IR_PORT, IR_NUM_PIN
	cbi	IR_DDR,  IR_NUM_PIN
	


	; Desactivar RAM externa:
	rcall	DESHAB_EXTRAM

	; WR y RD de RAM externa: A 1 ambas para q no se active la RAM 
	;  cuando se lea D1..D15 del disco duro (RAM externa desactivada)
	sbi	DDRD, 6
	sbi	PORTD, 6
	sbi	DDRD, 7
	sbi	PORTD, 7
	

	; Pins de USB:
	rcall	USB_INICIAR
		
	; Interrupciones externas: Deshabilitar. Con tiron
;	cbi	DDRD, 2
;	sbi	PORTD, 2
;	cbi	DDRD, 3
;	sbi	PORTD, 3
	
;	ldi	temp, 0x00
;	out	GIMSK, temp
	
	; TIMER0: Interrumpe cada x us
	; ---------------------------------
	
	; CLK / 64:
	ldi	r16, 0x03
	out	TCCR0,r16
	
	; Habilitar su int:
	ldi	r16, 0x02
	out	TIMSK, r16
	
	


	; Variables 
	; --------------------------		
	ldi	r16, 0x00
	sts	COMANDO_USUARIO, r16

	sts	IR_RX_BITS,r16
	sts	IR_BITS_1,r16
	sts	IR_CUENTA_0, r16
	
	sts	IR_CODE_FINAL+0, r16
	sts	IR_CODE_FINAL+1, r16
	
	sts	IR_TIMEOUT, r16
	
	sts	REPROD_FLAGS, r16
	
	ldi	r16, 3
	sts	REPROD_NUM_FICH_EN_DIR, r16
	
	ldi	r16, 0x00
	sts	VOLUMEN, r16

	; --------------------------------------------
	;     INICIO DEL PROGRAMA PRINCIPAL 
	; --------------------------------------------
	
	; Esperar unas decimas:
	ldi	r25, 30
	rcall	RETARDO_R25x25ms
	
	rcall	LCD_INICIAR
	

	; Mostrar primera pantalla:
	; --------------------------------------
	rcall	LCD_BORRAR
	; 1º linea
	ldi	ZH, high( PRIMERA_PANTALLA1 * 2)
	ldi	ZL,  low( PRIMERA_PANTALLA1 * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH

	; 2º linea:
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16
	ldi	ZH, high( PRIMERA_PANTALLA2 * 2)
	ldi	ZL,  low( PRIMERA_PANTALLA2 * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
	
	; Esperar un segundo aprox:
	ldi	r25,  95
	rcall	RETARDO_R25x25ms
	
	; Hacer scroll para q se vaya el texto:
	rcall	LCD_SCROLL_ON

	ldi	conta, 16
scroll_portada_loop:
	ldi	r16, ' '
	rcall	LCD_TX_DATO

	ldi	r25, 2
	rcall	RETARDO_R25x25ms
	
	dec	conta
	brne	scroll_portada_loop
	
	
	rcall	LCD_SCROLL_OFF
	rcall	LCD_BORRAR


	; Cargar caracteres para la barra del volumen:
	; -------------------------------------------------
	ldi	aux, 8*LCD_USERCHARS_CUANTOS2
	ldi	ZH, high( LCD_USERCHARS2*2 )
	ldi	ZL,  low( LCD_USERCHARS2*2 )	
	rcall	LCD_CARGA_CARACTERES

	
	rcall	LCD_BORRAR		
	
	; Check de RAM externa
	; --------------------------------------
	ldi	ZH, high( STR_CHK_RAM * 2)
	ldi	ZL,  low( STR_CHK_RAM * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH


	; X = 0x0260 - 0x7FFF

	; RAM externa:
	rcall	HAB_EXTRAM

	ldi	XH, 0x02
	ldi	XL, 0x60
check_ram_wr_loop:
	mov	r16, XH
	eor	r16, XL
	st	X+, r16
	
	cpi	XH, 0x80
	brne	check_ram_wr_loop
	

	;	
	ldi	XH, 0x02
	ldi	XL, 0x60
check_ram_rd_loop:
	mov	r17, XH
	eor	r17, XL
	ld	r16, X+
	
	cp	r16,r17
	brne	chk_ram_error
	
	cpi	XH, 0x80
	brne	check_ram_rd_loop
	
	; Ningun error!
	rjmp	chk_ram_ok	

		
	; RAM error:
chk_ram_error:
	; RAM externa:
	rcall	DESHAB_EXTRAM

	ldi	ZH, high( STR_ER * 2)
	ldi	ZL,  low( STR_ER * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
colgado:rjmp	colgado
	

	; RAM Ok:
chk_ram_ok:
	; RAM externa:
	rcall	DESHAB_EXTRAM

	ldi	ZH, high( STR_OK * 2)
	ldi	ZL,  low( STR_OK * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH


	; Variables en ram externa:
	; -------------------------------
	rcall	HAB_EXTRAM
	
	ldi	r16, high (PILA_DIRECTORIOS_ENT)	
	sts	PILA_DIRECTORIOS_PTR+0, r16
	
	ldi	r16,  low (PILA_DIRECTORIOS_ENT)	
	sts	PILA_DIRECTORIOS_PTR+1, r16
	
	rcall	DESHAB_EXTRAM	

	

	; Checkear disco duro:
	; ---------------------------------
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16
	
	ldi	ZH, high( STR_TSTHD * 2)
	ldi	ZL,  low( STR_TSTHD * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH


	rcall	ATA_SeleccionarDisco


	ldi	ZH, high( STR_OK * 2)
	ldi	ZL,  low( STR_OK * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
	
	ldi	r25, 30
	rcall	RETARDO_R25x25ms
	
	rcall	LCD_BORRAR



	; Comprobar FAT de disco OK y cargar valores importantes
	; -----------------------------------------------------------
	rcall	FAT_INICIAR	; Escribe la primera linea LCD
		
	; Iniciar chip MP3:
	; -----------------------------------------------------------
	rcall	VS1001_INICIAR	
	rcall	VS1001_RESET
	

	; Y habilitar ints:
	; --------------------------
	sei

	; Empezar en modo explorador, en directorio raiz:
	; ---------------------------------------------------
	rjmp	MODO_EXPLORADOR
	




; -----------------------------------------------------------
;   	MODO REPRODUCTOR
;
;  Llamado con RJMP. No sale hasta que no termina fichero
;
;
;  Entradas:
; -----------
; 
;	REPROD_NUM_FICH_EN_DIR: El numero de fich. a reproducir.
;
;
; -----------------------------------------------------------
MODO_REPRODUCTOR:
	; Quitar indicador de PAUSE:
	; ------------------------------
	lds	r16, REPROD_FLAGS
	andi	r16, 0xF7
	sts	REPROD_FLAGS, r16


	; Reiniciar el decodificador entre cada cancion:
	; -----------------------------------------------
;	ldi	r19, 2*2048/32
;	rcall	VS1001_TX_R19x32_CEROS
	; Y esperar a q acabe:
;	ldi	r25, 9
;	rcall	RETARDO_R25x25ms	

	rcall	VS1001_RESET
	
	; Y Enviar config:
	; -----------------------
	lds	r16, REPROD_FLAGS
	clr	r18
	clr	r19
	
	sbrc	r16, REPR_FLAG_BASS
	ori	r19, 0x80
	sbrc	r16, REPR_FLAG_SORROUND
	ori	r19, 0x01	
	
	; Enviar conf en r18/r19
	ldi	r17, VS1001ADDR_MODE
	rcall	VS1001_ESC_REGISTRO
	

	lds	r16, REPROD_FLAGS
	sbrc	r16, REPR_MODO_LISTA
	rjmp	modorepr_modo_lista
	
	; NO modo lista:
	; --------------------
	
	; Coger datos del fichero a reproducir:
	; ------------------------------------------
	ldi	r16, 1		; Peticion = coger datos
	sts	FAT_PETICION,r16
	; Numero de fichero a reproducir en el directorio actual:
	lds	r16, REPROD_NUM_FICH_EN_DIR
	sts	FAT_PETICION_NUM_ENTR,r16
	rcall	FAT_EXPLORA_DIRECTORIO

	; Si es un directorio, pasar a siguiente:
	lds	r16, FATDE_ATTR
	
	sbrc	r16, 4
	rjmp	MODO_REPR_SIG_FICHERO	; Si es un directorio, ir a siguiente:
	
	
	; Si no es MP3, pasar al siguiente:
	rcall	TEST_FILENAME_ES_MP3
	brcs	modo_repr_si_es_mp3	
	rjmp	MODO_REPR_SIG_FICHERO	; No es ".MP3"
	
modo_repr_si_es_mp3:

	
	; En FATDE_CLUSTER esta el primer cluster del fichero:
	lds	r16, FATDE_CLUSTER+0	
	sts	REPROD_FICH_CLUSTER+0,r16
	lds	r16, FATDE_CLUSTER+1	
	sts	REPROD_FICH_CLUSTER+1,r16
	lds	r16, FATDE_CLUSTER+2	
	sts	REPROD_FICH_CLUSTER+2,r16
	lds	r16, FATDE_CLUSTER+3
	sts	REPROD_FICH_CLUSTER+3,r16

	rjmp	modorepr_ini_vals

	; SI modo lista:
	; -------------------------
modorepr_modo_lista:
	; Selecciona el fichero de la lista:
	lds	r16, REPROD_NUM_FICH_EN_DIR
	rcall	SELEC_FICH_LISTA_ORDEN_R16 ;SELEC_FICH_LISTA_R16
		
	; Guardar cluster del fichero y empezar:
	rjmp	modo_repr_si_es_mp3	


modorepr_ini_vals:
	; Iniciar valores:
	; -------------------
	; EOF = 0
	clr	r16
	sts	REPROD_EOF, r16
	
	; Puntero de datos a enviar al comienzo del buffer:
	ldi	r16, high ( MP3_BUFFER )
	sts	REPROD_PTR_ENVIANDO+0, r16
	ldi	r16,  low ( MP3_BUFFER )
	sts	REPROD_PTR_ENVIANDO+1, r16
	
	; Puntero de datos a cargar, al comienzo tambien:
	ldi	r16, high ( MP3_BUFFER )
	sts	REPROD_PTR_CARGAR+0, r16
	ldi	r16,  low ( MP3_BUFFER )
	sts	REPROD_PTR_CARGAR+1, r16

	; Primer sector:
	clr	r16
	sts	REPROD_FICH_SECT, r16

	; Contador de bytes que quedan por enviar:
	lds	r16, FATDE_FILESIZE+0
	sts	REPROD_BYTES_TX+0, r16
	lds	r16, FATDE_FILESIZE+1
	sts	REPROD_BYTES_TX+1, r16
	lds	r16, FATDE_FILESIZE+2
	sts	REPROD_BYTES_TX+2, r16
	lds	r16, FATDE_FILESIZE+3
	sts	REPROD_BYTES_TX+3, r16
	
	; Contador de envios
	clr	r16
	sts	ENVIOS_MP3_PARA_CARGA, r16

	; Preparar valores para el display:
	; ------------------------------------------------------------
	rcall	PREPARAR_DATOS_LCD_FICHERO
	
	; En segunda linea poner donde estamos:
	; ----------------
	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )
	
	lds	r18, REPROD_NUM_FICH_EN_DIR
	rcall	BYTE2ASCII_R18
	
	ldi	r16, '/'
	st	X+, r16
	
	lds	r18, REPROD_NUM_FICHS_TOTAL; EXPLORER_NUMENTS
	rcall	BYTE2ASCII_R18
	rcall	CERO_2_X
	
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16

	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )
	rcall	LCD_ESCRIBE_X



	; Preparar TMR1 para que se desborde aprox. cada 0.3 segs:
	; ------------------------------------------------------------
	rcall	PREPARA_TMR1_OVR_300ms
	

	ldi	r16, 0x00
	sts	COMANDO_USUARIO, r16

	; Inicialmente llenar el buffer con sectores desde el disco:
	; ------------------------------------------------------------
	rcall	MODO_REPR_CARGAR_SECTOR
	
	; Seguir cargando hasta dar la vuelta al buffer:
modorepr_carga_inicial:
	lds	r16, REPROD_PTR_CARGAR
	cpi	r16, high ( MP3_BUFFER )
	breq	modorepr_bucle	; Ya esta !
	
	rcall	MODO_REPR_CARGAR_SECTOR
	
	rjmp	modorepr_carga_inicial	
	
	
modorepr_bucle:
	; ----------------------------------
	; 	Ver que hay que hacer:
	; ----------------------------------	

	; Enviar MP3 ?	
	; ----------------------------------
	; Solo si no estamos en pausa:
	lds	r16, REPROD_FLAGS
	sbrc	r16, REPR_FLAG_PAUSE
	rjmp	modorepr_ver_si_cargar	; En pausa, saltar comprobacion:
	
	
	sbic	VS1001_PIN, VS1001_DREQ
	rjmp	modorepr_envia_mp3	; DREQ=1 : Quiere datos

	
modorepr_ver_si_cargar:
	; Cargar datos al buffer ?
	; ----------------------------------
	;   Solo NO se lee un sector si pilla justo el sector que 
	;    se esta reproduciendo en este momento:
	
	
	; Se calcula solo con las partes altas de las direcciones !
	lds	r16, REPROD_PTR_CARGAR
	lds	r17, REPROD_PTR_ENVIANDO 	
	
	; ( brlo : si C=1 )
	; Se tiene que cumplir que antes NO pille debajo 
	;  y luego si. Si r20 acaba con un 0x02 no se puede
	;  cargar el sector
	clr	r20
	
	; Con respecto al sector actual:
	cp	r17, r16
	in	r18, SREG
	sbrs	r18, 0	; Saltar si C=1: Si por debajo
	inc	r20 	; No reproduciendo por debajo
	
	
	; Si leemos un sector, +=512 (0x0200)
	inc	r16
	inc	r16
			
	; Se supone que el valor maximo del ptr_cargar es el ultimo
	;  sector, luego como mucho, quedara en ultimo_byte +1 y 
	;  es mas facil no pasarlo al inicio, parar la comparacion:
	cp	r17, r16
	in	r18, SREG
	sbrc	r18, 0	; Saltar si C=0: No por debajo
	inc	r20 	; Si, reproduciendo por debajo

	; Si se han cumplido las dos es porque el sector actual
	;  esta siendo reproducido:
	cpi	r20, 0x02
	breq	modorepr_nocargar_lleno
	
	
	; Si, cargar, pero solo si el contador de envios esta OK:
	; ----------------------------------------------------------
	lds	r16, ENVIOS_MP3_PARA_CARGA
	cpi	r16, ENVIOS_MP3_HASTA_CARGAR
	brlo	modorepr_nocargar
	
	
	; Si el fichero acabo, no leer mas!!
	lds	r16, REPROD_EOF
	tst	r16
	brne	modorepr_nocargar_lleno
	
	
	; Si tengo al menos un sector para cargar:
	; ------------------------------------------
	; Cargar un sector, en el siguiente otro, etc...
	;   cuando este lleno el buffer, se reteara el contador que 
	;   hace que se cargen sentores, dando un buen margen de 
	;   cache de lectura :-)
	rcall	MODO_REPR_CARGAR_SECTOR
	rjmp	modorepr_nocargar

modorepr_nocargar_lleno:	
	; Resetear contador: Solo si ya estamos con el buffer lleno del todo
	clr	r16
	sts	ENVIOS_MP3_PARA_CARGA, r16
	
		
modorepr_nocargar:
	rcall	DESHAB_EXTRAM
	
	; Actualizar pantalla?
	; --------------------------
	;  Si el TIMER1 se ha desbordado, es que han pasado 300 ms:
	in	r16, TIFR
	
	sbrs	r16, OCF1A	
	rjmp	modorepr_bucle
		
	
	;  Llamar a las rutinas de actualizacion de display:
	rcall	ACTUALIZA_NOMBRE_FICH_LCD
	rcall	ACTUALIZA_DESPLAZ_FICH_LCD
	
	; Actualizar Kbps y tiempo de reproduccion en la segunda linea
	;   alineado a la derecha
	ldi	XH, high (STR_BUF)
	ldi	XL, low (STR_BUF)

	ldi	r16, ' '
	st	X+, r16
	st	X+, r16
	st	X+, r16

	; Primero el Kbps o "PAUSE":
	; ------------------------------
	lds	r16, REPROD_FLAGS
	sbrs	r16, REPR_FLAG_PAUSE
	rjmp	modo_repr_no_pause

	; Si en PAUSE:
	ldi	r16, 'P'
	st	X+, r16
	ldi	r16, 'A'
	st	X+, r16
	ldi	r16, 'U'
	st	X+, r16
	ldi	r16, 'S'
	st	X+, r16
	ldi	r16, 'E'
	st	X+, r16	
	rjmp	modo_repr_tras_kbps


modo_repr_no_pause:
	ldi	r17, VS1001ADDR_AUDATA
	rcall	VS1001_LEE_REGISTRO
	
	; Byte bajo (r19) es kbps:
	mov	r2, r19
	clr	r1	
	rcall	R1_R2_Decimal
	
	ldi	r16, 'K'
	st	X+, r16

modo_repr_tras_kbps:
	ldi	r16, ' '
	st	X+, r16
	
	
	; Ahora el tiempo de reproduccion:
	ldi	r17, VS1001ADDR_DECODE_TIME
	rcall	VS1001_LEE_REGISTRO
	
	
	; Lo tengo en r18:r19 (MSB:LSB)
	rcall	CALCULA_MINUTOS
	
	; Devuelve en r1:r2 ( MSB:LSB ) el numero de minutos enteros
	;  a partir del numero de segundos en r18:r19
	; En r19 deja el numero de segundos que sobran
	
	push	r19
	
	; "MIN:SEC"
	rcall	R1_R2_Decimal
	
	ldi	r16, ':'
	st	X+, r16

	; Los segundos: Siempre las decenas
	pop	r18
	rcall	BYTE2ASCII_R18
	
	rcall	CERO_2_X
		
	; Alinear a la derecha del display
	ldi	XH, high (STR_BUF)
	ldi	XL, low (STR_BUF)
	rcall	LCD_ESCRIBE_ABAJO_DERECHA_X	


	; Ahora se miran las teclas, para que solo sean 
	;  una pulsacion cada XXX ms		
	; Algun comando del user? Puls y IR
	; -----------------------------------
	rcall	LEE_PULS_Y_IR

	lds	r16, COMANDO_USUARIO	; Del tecl o IR
	tst	r16
	breq	modorepr_no_cmds
	
	; Borrar para q no se repita!
	clr	r17
	sts	COMANDO_USUARIO, r17
	
	; Si hay un comando: Enrutar segun sea:
	cpi	r16, PULS_ALANTE
	brne	modorepr_sk1
	rjmp	MODO_REPR_SIG_FICHERO
modorepr_sk1:	
	cpi	r16, PULS_ATRAS
	brne	modorepr_sk2
	rjmp	MODO_REPR_ANT_FICHERO
modorepr_sk2:	
	cpi	r16, PULS_VOL_MAS
	brne	modorepr_sk3
	rcall	MODO_REPR_VOL_MAS
	rjmp	modorepr_no_cmds
	
modorepr_sk3:	
	cpi	r16, PULS_VOL_MENOS
	brne	modorepr_sk4
	rcall	MODO_REPR_VOL_MENOS
	rjmp	modorepr_no_cmds
	
modorepr_sk4:	
	cpi	r16, PULS_STOP
	brne	modorepr_sk5
	rjmp	MODO_REPR_STOP
modorepr_sk5:	
	cpi	r16, PULS_PLAY
	brne	modorepr_sk6
	rjmp	MODO_REPR_PLAY
modorepr_sk6:	



	
modorepr_no_cmds:	

	; Quitar flags y volver a cargar:
	rcall	PREPARA_TMR1_OVR_300ms	
	
	; No salir del bucle:
	rjmp	modorepr_bucle

	

	; El chip quiere datos, darle hasta que no pida mas:
	; ------------------------------------------------------
modorepr_envia_mp3:
	rcall	HAB_EXTRAM

	; En (X) el puntero a posicion actual:
	lds	XH, REPROD_PTR_ENVIANDO 
	lds	XL, REPROD_PTR_ENVIANDO + 1
	
	
modorepr_envia_mp3_lp:
	rcall	VS1001_TX_MPEG_DATA_32X

	; Contador de envios
	lds	r16, ENVIOS_MP3_PARA_CARGA
	inc	r16
	sts	ENVIOS_MP3_PARA_CARGA, r16
	
	; Contador de bytes que quedan: Restar 32:
	lds	r2, REPROD_BYTES_TX+0
	lds	r3, REPROD_BYTES_TX+1
	lds	r4, REPROD_BYTES_TX+2
	lds	r5, REPROD_BYTES_TX+3

	clr	r0
	ldi	r16, 32
	sub	r2, r16
	sbc	r3, r0
	sbc	r4, r0
	sbc	r5, r0
	
		
	sts	REPROD_BYTES_TX+0, r2
	sts	REPROD_BYTES_TX+1, r3
	sts	REPROD_BYTES_TX+2, r4
	sts	REPROD_BYTES_TX+3, r5
	
	; Es cero (r2=r3=r4=r5=0) o negativo(C=1) ??
	brcs	modorepr_envmp3_EOF
	
	tst	r2
	brne	modorepr_envmp3_sigue	
	tst	r3
	brne	modorepr_envmp3_sigue	
	tst	r4
	brne	modorepr_envmp3_sigue	
	tst	r5
	brne	modorepr_envmp3_sigue	
	
modorepr_envmp3_EOF:
	; Fin de fichero, pasar a siguiente:
	rjmp	MODO_REPR_SIG_FICHERO
	
	
modorepr_envmp3_sigue:	
	; Fin del buffer?
	cpi	XH, high ( MP3_BUFFER + 512*SECTORES_MP3_EN_RAM  )
	brne	modorepr_envmp3_no_ovr
	
	; Dar la vuelta al puntero:
	ldi	XH, high ( MP3_BUFFER )
	ldi	XL,  low ( MP3_BUFFER )
	
modorepr_envmp3_no_ovr:	

	; Hay mas datos para enviar ??
	;  Si XH = high ( REPROD_PTR_CARGAR )  y XL = 0x00
	;  no hay mas datos !!
	tst	XL
	brne	modorepr_envia_mp3_si
	
	lds	r16, REPROD_PTR_CARGAR + 0
	cp	r16, XH
	brne	modorepr_envia_mp3_si
	
	; Pues no hay mas datos, cargar antes:
	rjmp	modorepr_envmp3_fin_envios
	
modorepr_envia_mp3_si:
	
	; Aun quiere mas ?	
	sbic	VS1001_PIN, VS1001_DREQ
	rjmp	modorepr_envia_mp3_lp; DREQ = 1 : Mas
	; DREQ = 0 : No mas datos por ahora

modorepr_envmp3_fin_envios:
		
	rcall	DESHAB_EXTRAM

	; Guardar puntero para seguir luego
	sts	REPROD_PTR_ENVIANDO, XH
	sts	REPROD_PTR_ENVIANDO + 1, XL

	; Despues de enviar, cargar nuevos datos
	rjmp	modorepr_ver_si_cargar
	
	
; -----------------------------------------------------------
;   MODO_REPR_CARGAR_SECTOR
;
;  Rutina que carga un nuevo sector en el buffer circular, 
;   actualizando punteros, numero de sector, clusters, ...
; -----------------------------------------------------------
MODO_REPR_CARGAR_SECTOR:
	; Pasar de cluster actual a sector:
	lds	r2, REPROD_FICH_CLUSTER + 0
	lds	r3, REPROD_FICH_CLUSTER + 1
	lds	r4, REPROD_FICH_CLUSTER + 2
	lds	r5, REPROD_FICH_CLUSTER + 3
	
	rcall	FAT_CLUSTER2SECTOR
	
	; Sumar el sector actual dentro de este cluster:
	lds	r13, REPROD_FICH_SECT
	
	lds	r2, LBA_DIR+0
	lds	r3, LBA_DIR+1
	lds	r4, LBA_DIR+2
	lds	r5, LBA_DIR+3

	clr	r0
	add	r2, r13
	adc	r3, r0
	adc	r4, r0
	adc	r5, r0

	; Leer ese sector en donde indica el puntero REPROD_PTR_CARGAR
	sts	LBA_DIR+0, r2
	sts	LBA_DIR+1, r3
	sts	LBA_DIR+2, r4
	sts	LBA_DIR+3, r5
	
	ldi	r16, 0x01	; 1 sector
	sts	SECTOR_CNT, r16
	
	; Destino en (Y):
	lds	YH, REPROD_PTR_CARGAR + 0 
	lds	YL, REPROD_PTR_CARGAR + 1 
	
	rcall	ATA_ReadSectors
	
	; Sector leido !
	
	; Ahora (Y) apunta detras del ultimo byte
	; Si se pasa del buffer, resetearlo:
	cpi	YH, high ( MP3_BUFFER + 512 * SECTORES_MP3_EN_RAM )
	brne	modorpr_crgsec_no_ovr
	
	; Si, resetear puntero:
	ldi	YH, high ( MP3_BUFFER )
	ldi	YL,  low ( MP3_BUFFER )

modorpr_crgsec_no_ovr:
	; Y guardarlo!!
	sts	REPROD_PTR_CARGAR , YH
	sts	REPROD_PTR_CARGAR+1,YL
	

	; Incrementar contador de sector dentro del cluster actual:
	lds	r13, REPROD_FICH_SECT
	lds	r16, BPB_SECSPERCLUS
	
	inc	r13
	cp	r13, r16
	brne	modorpr_crgsec_no_secovr
	
	; Se ha pasado:
	clr	r13
	sts	REPROD_FICH_SECT, r13
	
	; Siguiente cluster:
	lds	r2, REPROD_FICH_CLUSTER + 0
	lds	r3, REPROD_FICH_CLUSTER + 1
	lds	r4, REPROD_FICH_CLUSTER + 2
	lds	r5, REPROD_FICH_CLUSTER + 3

	rcall	FAT_SIGUIENTE_CLUSTER
	; Fin de fichero? Si C=1
	brcc	modorpr_crgsec_no_eof

	; EOF = 1
	ldi	r16, 0x01
	sts	REPROD_EOF, r16

modorpr_crgsec_no_eof:
	sts	REPROD_FICH_CLUSTER + 0 , r2
	sts	REPROD_FICH_CLUSTER + 1 , r3
	sts	REPROD_FICH_CLUSTER + 2 , r4
	sts	REPROD_FICH_CLUSTER + 3 , r5	
	
	
	; Ya no hay que guardar el valor de r13
	cpse	r0,r0	
modorpr_crgsec_no_secovr:
	; Guardar nuevo numero de sector:
	sts	REPROD_FICH_SECT, r13

	

	ret

;==================================================================
; 		 CMD: Siguiente fichero
;==================================================================
MODO_REPR_SIG_FICHERO:
	; Enviar ceros para asegurar fin de cancion
	rcall	VS1001_RESET

	; Siguiente archivo
	lds	r16, REPROD_NUM_FICH_EN_DIR
	lds	r17, REPROD_NUM_FICHS_TOTAL
	inc	r16
	
	cp	r17, r16
	brsh	modo_repr_sig
	
	; Seleccionar primer directorio/fichero:
	ldi	r16, 0x01
modo_repr_sig:	
	sts	REPROD_NUM_FICH_EN_DIR, r16
	

	; Reproducirlo:
	rjmp	MODO_REPRODUCTOR


;==================================================================
; 		 CMD: Anterior
;==================================================================
MODO_REPR_ANT_FICHERO:
	; Enviar ceros para asegurar fin de cancion
	rcall	VS1001_RESET

	; Anterior archivo
	lds	r16, REPROD_NUM_FICH_EN_DIR
	
	; Si ya es el primero, ir al ultimo:
	cpi	r16, 0x01
	breq	modo_repr_ant

	dec	r16
	rjmp	modo_repr_ant_grd
		
modo_repr_ant:	
	; Ir al ultimo:
	lds	r16, REPROD_NUM_FICHS_TOTAL
	
modo_repr_ant_grd:
	sts	REPROD_NUM_FICH_EN_DIR, r16

	; Reproducirlo:
	rjmp	MODO_REPRODUCTOR


;==================================================================
; 		 CMD: Subir volumen:
;  Es RUTINA !!
;==================================================================
MODO_REPR_VOL_MAS:
	lds	r16, VOLUMEN
	
	tst	r16
	breq	vol_mas_max
	
	; Subir volumen:
	subi	r16, PASO_VOLUMEN
vol_mas_max:	
	
	; Y guardarlo en ram y cambiarlo en el chip:
	sts	VOLUMEN, r16
	mov	r17, r16
	rcall	VS1001_CAMBIAR_VOLUMEN

volumen_en_display:
	; Y mostrarlo en display:
	; -------------------------------
	rcall	DESHAB_EXTRAM
	
	ldi	XH, high ( STR_BUF )	
	ldi	XL,  low ( STR_BUF )	

	
	; Poner en dBs

	; Solo poner el - si es !=0
	clr	r1
	lds	r2, VOLUMEN
	lsr	r2
	
	ldi	r16, ' '
	tst	r2
	breq	vol_en_displ_cero	
	ldi	r16, '-'
vol_en_displ_cero:
	st	X+, r16

	rcall	R1_R2_Decimal
	
	ldi	r16, 'd'
	st	X+, r16
	ldi	r16, 'B'
	st	X+, r16

vol_en_displ_rellena:
	cpi	XL, low ( STR_BUF + 7 )
	breq	vol_en_displ_rellena_fin

	ldi	r16, ' '
	st	X+, r16
	rjmp	vol_en_displ_rellena
	
vol_en_displ_rellena_fin:	

	; Dibujar la barra del volumen:
	;
	;  Caracteres    0x20 y 1,2,3,4,5: barras verticales
	; corresponden a:  0    1 2 3 4 5 barras de 5
	; ---------------------------------------------
	lds	r17, VOLUMEN
	com	r17
vol_en_lcd_lp:
	cpi	r17, 4*5
	brsh	vol_en_lcd_mas

	; Si es < 4*5, poner el caracter que sea y fin de la barra:
	; Dividir r17 entre 4
	lsr	r17
	lsr	r17		
	
	st	X+, r17
	rjmp	vol_en_lcd_fin
		
	; Es > 25 todavia: Poner cuadro entero y restar:
vol_en_lcd_mas:
	ldi	r16, 0x05
	st	X+, r16

	subi	r17, 4*5
	
	rjmp	vol_en_lcd_lp
	
vol_en_lcd_fin:

	rcall	CERO_2_X

	; Al LCD:
	ldi	r16, 0
	rcall	LCD_SET_CURSOR_POS_R16
	rcall	LCD_BORRAR_LINEA
	
		
	ldi	r16, 0
	rcall	LCD_SET_CURSOR_POS_R16
	
	ldi	XH, high ( STR_BUF )	
	ldi	XL,  low ( STR_BUF )	
	rcall	LCD_ESCRIBE_X
	
	

		
	
	ret
	
;==================================================================
; 		 CMD: Bajar volumen:
;  Es RUTINA !!
;==================================================================
MODO_REPR_VOL_MENOS:
	lds	r16, VOLUMEN
	
	cpi	r16, (0x100 - PASO_VOLUMEN)
	breq	vol_menos_max
	
	; Subir volumen:
	ldi	r17, PASO_VOLUMEN
	add	r16, r17
vol_menos_max:	
	
	; Y guardarlo en ram y cambiarlo en el chip:
	sts	VOLUMEN, r16
	mov	r17, r16
	rcall	VS1001_CAMBIAR_VOLUMEN

	; Y mostrarlo en display:
	rjmp	volumen_en_display		

	
;==================================================================
; 		 CMD: PLAY/PAUSE
;==================================================================
MODO_REPR_PLAY:
	lds	r16, REPROD_FLAGS
	ldi	r17, 1 << REPR_FLAG_PAUSE
	eor 	r16, r17
	sts	REPROD_FLAGS, r16
	
	rjmp	modorepr_no_cmds
	
	
	
;==================================================================
; 		 CMD: Stop
;==================================================================
MODO_REPR_STOP:
	; Salir al modo explorador:
	rcall	VS1001_RESET
	
	; Quitar modo lista
	lds	r16, REPROD_FLAGS
	andi	r16, 0xFB	
	sts	REPROD_FLAGS, r16
	
	; Recuperar el directorio y entrada por la que ibamos:
	rcall	RECUPERA_CONTEXT_EXPLORADOR

	rjmp	MODO_EXPLORADOR_CAMBIA_DIRECT2
			

	
; -----------------------------------------------------------
;
;   	MODO EXPLORADOR
;
; -----------------------------------------------------------
MODO_EXPLORADOR:

	ldi	r16, 0x00
	sts	COMANDO_USUARIO, r16

; ------------------------------------	
;   Refrescar cuando cambia el 
;  directorio actual
; ------------------------------------
MODO_EXPLORADOR_CAMBIA_DIRECT:
	; Seleccionar primer directorio/fichero:
	ldi	r16, 0x01
	sts	EXPLORER_ENTACT, r16
	
MODO_EXPLORADOR_CAMBIA_DIRECT2:
	; Contar entradas en directorio actual:
	ldi	r16, 0		; Peticion = solo contar entradas
	sts	FAT_PETICION,r16
	rcall	FAT_EXPLORA_DIRECTORIO
		
	; Guardar cuantas son:
	lds	r16, FAT_DIRENT_ENCONTRADAS
	sts	EXPLORER_NUMENTS, r16
	
	
	; Para asegurar q el buffer esta vacio de entrada:
	ldi	r16, 0x00
	sts	FATDE_FILENAME, r16	

; ------------------------------------	
;   Refrescar cuando cambia el 
;  fichero seleccionado	
; ------------------------------------
MODO_EXPLORADOR_CAMBIA_FICH:

	; Cargar datos del fich/dir seleccionado:
	; -----------------------------------------
	ldi	r16, 1		; Peticion = coger datos
	sts	FAT_PETICION,r16
	lds	r16, EXPLORER_ENTACT
	sts	FAT_PETICION_NUM_ENTR,r16
	
	rcall	FAT_EXPLORA_DIRECTORIO

	; ***
	rcall	PREPARAR_DATOS_LCD_FICHERO

	; En segunda linea poner donde estamos:
	; ----------------
	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )
	
	lds	r18, EXPLORER_ENTACT
	rcall	BYTE2ASCII_R18
	
	ldi	r16, '/'
	st	X+, r16
	
	lds	r18, EXPLORER_NUMENTS
	rcall	BYTE2ASCII_R18
	rcall	CERO_2_X
	
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16

	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )
	rcall	LCD_ESCRIBE_X

	
	; Poner a la derecha el tamaño en Kbs:
	ldi	XH, high ( STR_BUF )
	ldi	XL,  low ( STR_BUF )


	; En Kbs es >> 10:
	lds	r0, FATDE_FILESIZE+3
	lds	r1, FATDE_FILESIZE+2
	lds	r2, FATDE_FILESIZE+1
	
	; Ahora >> 2
	lsr	r0
	ror	r1
	ror	r2

	lsr	r0
	ror	r1
	ror	r2

	rcall	R0_R1_R2_Decimal

	
	ldi	r16, ' '
	st	X+, r16
	ldi	r16, 'K'
	st	X+, r16
	ldi	r16, 'b'
	st	X+, r16
	rcall	CERO_2_X


	; Alinear a la derecha del display
	rcall	LCD_ESCRIBE_ABAJO_DERECHA_X

;	ldi	r16, 0x00
;	sts	COMANDO_USUARIO, r16

	rcall	ACTUALIZA_NOMBRE_FICH_LCD
	
; ------------------------------------	
;    Solo refresca primera linea
; ------------------------------------
MODO_EXPLORADOR_LOOP:
	; Retardo de cada ciclo: 300 ms aprox
	; ------------------------------------------
	ldi	r25, 10
	rcall	RETARDO_R25x25ms

	rcall	ACTUALIZA_NOMBRE_FICH_LCD

				
	; Algo del chip USB?
	; --------------------
	sbis	USB_PIN, USB_PIN_RXF
	rjmp	MODO_USB 		; RXF = 0

		
	; Algun comando del teclado / mando a distancia??
	; -------------------------------------------------
	rcall	LEE_PULS_Y_IR
	
modoexpl_seguncomando:
	lds	r16, COMANDO_USUARIO	; Del tecl o IR
	tst	r16
	breq	modoexplo_nocmd
	
	ldi	r17, 0x00
	sts	COMANDO_USUARIO, r17
	
	;  Segun comando:
	cpi	r16, PULS_ALANTE
	brne	modoexplo_sk1
	rjmp	MODO_EXPLO_SIG_FICHERO
modoexplo_sk1:	
	cpi	r16, PULS_ATRAS
	brne	modoexplo_sk2
	rjmp	MODO_EXPLO_ANT_FICHERO
modoexplo_sk2:	
	cpi	r16, PULS_VOL_MAS
	brne	modoexplo_sk3
	rcall	MODO_REPR_VOL_MAS
	rjmp	modoexplo_nocmd
	
modoexplo_sk3:	
	cpi	r16, PULS_VOL_MENOS
	brne	modoexplo_sk4
	rcall	MODO_REPR_VOL_MENOS
	rjmp	modoexplo_nocmd
	
modoexplo_sk4:	
	cpi	r16, PULS_PLAY
	brne	modoexplo_sk5
	rjmp	MODO_EXPLO_PLAY

modoexplo_sk5:	
	cpi	r16, PULS_STOP
	brne	modoexplo_sk6
	rcall	MODO_MENU
	
	; Si estamos en modo lista hay que ir al reproductor:
	lds	r16, REPROD_FLAGS
	sbrc	r16, REPR_MODO_LISTA
	rjmp	MODO_REPRODUCTOR	; Modo LISTA = 1
	; No
	
	; Para redibujar toda la pantalla:
	rjmp	MODO_EXPLORADOR_CAMBIA_DIRECT
	
modoexplo_sk6:	
	cpi	r16, PULS_SUBE_DIR
	brne	modoexplo_sk7
	rjmp	MODO_EXPL_SUB_DIRECT	


modoexplo_sk7:	



	

modoexplo_nocmd:
	; Rutina que se debe llamar aprox. cada 300 ms para
	;  que se actualice el desplazamiento del nombre del fich
	;  en el display:
	rcall	ACTUALIZA_DESPLAZ_FICH_LCD


	rjmp	MODO_EXPLORADOR_LOOP
	



;==================================================================
;
;	  CMD: Subir un directorio
;
;==================================================================
MODO_EXPL_SUB_DIRECT:
	; Subir un directorio: 
	; Si estamos en ROOT, ignorar:
	lds	r16, BPB_CLUS_DIR_RAIZ+0
	lds	r17, FAT_DIRECT_ACTUAL+0
	cp	r16, r17
	brne	modoexplsubdir_si
	lds	r16, BPB_CLUS_DIR_RAIZ+1
	lds	r17, FAT_DIRECT_ACTUAL+1
	cp	r16, r17
	brne	modoexplsubdir_si
	lds	r16, BPB_CLUS_DIR_RAIZ+2
	lds	r17, FAT_DIRECT_ACTUAL+2
	cp	r16, r17
	brne	modoexplsubdir_si
	lds	r16, BPB_CLUS_DIR_RAIZ+3
	lds	r17, FAT_DIRECT_ACTUAL+3
	cp	r16, r17
	brne	modoexplsubdir_si	

	; Como estamos en root, ignorar:
	rjmp	MODO_EXPLORADOR_LOOP

modoexplsubdir_si:
	; Subir un directorio:
	ldi	r16, 2
	sts	EXPLORER_ENTACT, r16

	; Coger la entrada de ".."
	ldi	r16, 1		; Peticion = coger datos
	sts	FAT_PETICION,r16
	lds	r16, EXPLORER_ENTACT
	sts	FAT_PETICION_NUM_ENTR,r16
	
	rcall	FAT_EXPLORA_DIRECTORIO
	
	; Poner corchetes al directorio:
	ldi	r16, '['
	sts	FATDE_FILENAME+0, r16
	ldi	r16, '.'
	sts	FATDE_FILENAME+1, r16
	sts	FATDE_FILENAME+2, r16
	ldi	r16, ']'
	sts	FATDE_FILENAME+3, r16
	ldi	r16, 0
	sts	FATDE_FILENAME+4, r16

	; Hacer como que pulsamos PLAY en ella:
	ldi	r16, PULS_PLAY
	sts	COMANDO_USUARIO, r16
	
	rjmp	modoexpl_seguncomando

	

;==================================================================
;
;	  CMD: Siguiente fichero
;
;==================================================================
MODO_EXPLO_SIG_FICHERO:
	; Siguiente archivo / directorio:
	lds	r16, EXPLORER_ENTACT
	lds	r17, EXPLORER_NUMENTS
	inc	r16
	
	cp	r17, r16
	brsh	modo_explo_sig
	
	; Seleccionar primer directorio/fichero:
	ldi	r16, 0x01
modo_explo_sig:	
	sts	EXPLORER_ENTACT, r16


	rjmp	MODO_EXPLORADOR_CAMBIA_FICH

;==================================================================
;
;	  CMD: Anterior fich
;
;==================================================================
MODO_EXPLO_ANT_FICHERO:
	; Siguiente archivo / directorio:
	lds	r16, EXPLORER_ENTACT
	
	cpi	r16, 0x01
	brne	modoexplantfich_ant
	
	; Si es el primero, ir al ultimo:
	lds	r16, EXPLORER_NUMENTS
	cpse	r0,r0	; saltarse el DEC
	
modoexplantfich_ant:	
	dec	r16
		
	sts	EXPLORER_ENTACT, r16

	rjmp	MODO_EXPLORADOR_CAMBIA_FICH

;==================================================================
;
;	  CMD: PLAY
;
;  Si es un fichero, reproduce
;  Si es un directorio, entrar:
;
;==================================================================
MODO_EXPLO_PLAY:
	lds	r23, FATDE_ATTR
	
	sbrc	r23, 4
	rjmp	modoexplplay_dir
	
	; Es un fichero:
	; ---------------------
	; antes de nada, guardar donde estabamos
	rcall	SALVA_CONTEXT_EXPLORADOR

	;  Poner como fichero seleccionado y reproducirlo:
	lds	r16, EXPLORER_ENTACT
	sts	REPROD_NUM_FICH_EN_DIR, r16
	

	lds	r16, EXPLORER_NUMENTS
	sts	REPROD_NUM_FICHS_TOTAL, r16
	
	; Modo lista = NO
	lds	r16, REPROD_FLAGS
	andi	r16, 0xFB
	sts	REPROD_FLAGS, r16
	
	rjmp	MODO_REPRODUCTOR
		
	
	; Es un directorio:
	; ---------------------
modoexplplay_dir:
	; Si es "[.]" ignorar:
	lds	r16, FATDE_FILENAME+1
	cpi	r16, '.'
	brne	modoexplplay_no_punto

	lds	r16, FATDE_FILENAME+3
	cpi	r16, 0
	brne	modoexplplay_no_punto
	; Si, es "." -> Ignorar
	rjmp	modoexplplay_fin

modoexplplay_no_punto:
	; Si el cluster es 0x00000000 es el raiz:
	lds	r16,  FATDE_CLUSTER+0
	tst	r16
	brne	modoexplplay_no_root
	lds	r16,  FATDE_CLUSTER+1
	tst	r16
	brne	modoexplplay_no_root
	lds	r16,  FATDE_CLUSTER+2
	tst	r16
	brne	modoexplplay_no_root
	lds	r16,  FATDE_CLUSTER+3
	tst	r16
	brne	modoexplplay_no_root
	
	; Es el raiz:
	lds	r16, BPB_CLUS_DIR_RAIZ+0	
	sts	FATDE_CLUSTER+0, r16
	lds	r16, BPB_CLUS_DIR_RAIZ+1
	sts	FATDE_CLUSTER+1, r16
	lds	r16, BPB_CLUS_DIR_RAIZ+2	
	sts	FATDE_CLUSTER+2, r16
	lds	r16, BPB_CLUS_DIR_RAIZ+3	
	sts	FATDE_CLUSTER+3, r16
	

modoexplplay_no_root:
	; Poner como directorio actual:
	lds	r16,  FATDE_CLUSTER+0
	sts	FAT_DIRECT_ACTUAL+0 , r16
	lds	r16,  FATDE_CLUSTER+1
	sts	FAT_DIRECT_ACTUAL+1 , r16
	lds	r16,  FATDE_CLUSTER+2
	sts	FAT_DIRECT_ACTUAL+2 , r16
	lds	r16,  FATDE_CLUSTER+3
	sts	FAT_DIRECT_ACTUAL+3 , r16


	; Si es "[..]" recuperar posicion en ese directorio:
	lds	r16, FATDE_FILENAME+1
	cpi	r16, '.'
	brne	modoexplplay_no_puntpunt		
	lds	r16, FATDE_FILENAME+2
	cpi	r16, '.'
	brne	modoexplplay_no_puntpunt		
	lds	r16, FATDE_FILENAME+4
	cpi	r16, 0
	brne	modoexplplay_no_puntpunt		
	
	; Es ".."
	rcall	HAB_EXTRAM
	
	lds	XH, PILA_DIRECTORIOS_PTR+0
	lds	XL, PILA_DIRECTORIOS_PTR+1
	
	sbiw	XL, 1
	ld	r16, X
	sts	EXPLORER_ENTACT, r16

	sts	PILA_DIRECTORIOS_PTR+0, XH
	sts	PILA_DIRECTORIOS_PTR+1, XL


	; Seguir por alli, pero no sobreescribir el num. de 
	;  fichero seleccionado
	rjmp	MODO_EXPLORADOR_CAMBIA_DIRECT2
	
modoexplplay_no_puntpunt:
	rcall	HAB_EXTRAM
	; Se ha entrado en un directorio: Guardar posicion actual
	;  en una pila para luego saber donde ibamos:
	lds	XH, PILA_DIRECTORIOS_PTR+0
	lds	XL, PILA_DIRECTORIOS_PTR+1
	
	lds	r16, EXPLORER_ENTACT
	st	X, r16
	adiw	XL, 1

	sts	PILA_DIRECTORIOS_PTR+0, XH
	sts	PILA_DIRECTORIOS_PTR+1, XL
	

modoexplplay_fin:		
	rjmp	MODO_EXPLORADOR_CAMBIA_DIRECT
	
	

; -------------------------------------------------
; Rutina que se debe llamar aprox. cada 300 ms para
;  que se actualice el desplazamiento del nombre del fich
;  en el display:
; -------------------------------------------------
ACTUALIZA_DESPLAZ_FICH_LCD:	
	; El texto debe deslizarse??
	; -------------------------------------------------
	ldi	XH, high( FATDE_FILENAME )
	ldi	XL,  low( FATDE_FILENAME )
	rcall	STRLEN

	; Guardar long. de cadena en r1 para luego
	mov	r1, r16	
	
	cpi	r16, ANCHO_LCD+1
	brge	modo_explo_proc_deslizamiento	

	; No hay que deslizar:
ve_MODO_EXPLORADOR_LOOP:
	ret

	
modo_explo_proc_deslizamiento:
	; Inc. contador para retardo:
	lds	r16, EXPLORER_LCD_RETARDO
	
	cpi	r16, VALOR_EXPL_LCD_RET
	breq	modo_explo_desliz_noinccnt	; Ya esta deslizando!
	
	inc	r16
	sts	EXPLORER_LCD_RETARDO, r16
modo_explo_desliz_noinccnt:

	; Si aun no ha llegado a su valor, no mover:
	cpi	r16, VALOR_EXPL_LCD_RET
	brne	ve_MODO_EXPLORADOR_LOOP
	
	; Deslizar texto en el sentido actual:
	lds	r17, EXPLORER_LCD_SENTIDO
	
	lds	r16, EXPLORER_LCD_OFFSET
	add	r16, r17
	sts	EXPLORER_LCD_OFFSET, r16
	
	; Si llega a un borde, cambiar el sentido de deslizamiento:
	; Bordes: 0 y STRLEN(S)-ANCHO_LCD
	tst	r16
	breq	modoexpl_cambia_sentido

	; r1 era la longitud del texto:
	mov	r17, r1
	subi	r17, ANCHO_LCD
	cp	r16, r17
	breq	modoexpl_cambia_sentido

	; Pues nada, a seguir	
	
	ret
	

modoexpl_cambia_sentido:
	; hacer q se detenga un momento en los bordes:
	ldi	r16, VALOR_EXPL_LCD_RET / 2
	sts	EXPLORER_LCD_RETARDO, r16

	lds	r17, EXPLORER_LCD_SENTIDO
	neg	r17	
	sts	EXPLORER_LCD_SENTIDO, r17

	ret



; ---------------------------------------------------------
;    ACTUALIZA_NOMBRE_FICH_LCD
;
;  Pone el nombre del fich. en display, con su desplazamiento
; ---------------------------------------------------------
ACTUALIZA_NOMBRE_FICH_LCD:
	; No borrar LCD para q no parpadee ;-)
	rcall	DESHAB_EXTRAM

	; Primera linea: El nombre del fich. seleccionado.
	; --------------------------------------------------
	ldi	r16, 0
	rcall	LCD_SET_CURSOR_POS_R16
	rcall	LCD_BORRAR_LINEA

	ldi	r16, 0
	rcall	LCD_SET_CURSOR_POS_R16

	ldi	XH, high( FATDE_FILENAME )
	ldi	XL,  low( FATDE_FILENAME )
	lds	r17, EXPLORER_LCD_OFFSET
	ldi	r18, ANCHO_LCD
	rcall	LCD_ESC_X_OFF_R17_MAX_R18
	
	ret


; ------------------------------------
; PREPARAR_DATOS_LCD_FICHERO
;
;  Usado por el explorador y el reproductor:
;   Inicia valores de desplazamiento de texto 
;  , pone corchetes a directorios, etc...
;
;  Borra LCD y pone en 2º linea: "N/M"
; ------------------------------------
PREPARAR_DATOS_LCD_FICHERO:	
	; Si es directorio, poner corchetes:
	lds	r23, FATDE_ATTR
	
	sbrs	r23, 4
	rjmp	modoexpl_no_corchetes
	
	; Si es directorio, poner corchetes:
	; ------------------------------------
	ldi	XH, high( FATDE_FILENAME )
	ldi	XL,  low( FATDE_FILENAME )
	rcall	STRLEN
	
	mov	r1, r16
	
	; Mover hacia la derecha una posicion:
	ldi	YH, high( FATDE_FILENAME )
	ldi	YL,  low( FATDE_FILENAME )
	
	clr	r0
	add	YL, r1
	adc	YH, r0
	mov	r18, r1
	inc	r18
modoexpl_corch_loop:
	ld	r16, Y
	std	Y+1, r16
	
	subi	YL, 1
	sbc	YH, r0

	dec	r18
	brne	modoexpl_corch_loop
	
	
	ldi	XH, high( FATDE_FILENAME )
	ldi	XL,  low( FATDE_FILENAME )

	ldi	r16, '['
	st	X+, r16
	
	clr	r0
	add	XL, r1
	adc	XH, r0
	
	ldi	r16, ']'
	st	X+, r16
	
	ldi	r16, 0x00
	st	X+, r16
	
modoexpl_no_corchetes:
	; Iniciar valores para desplazamiento en display:
	clr	r16
	sts	EXPLORER_LCD_OFFSET, r16
	sts	EXPLORER_LCD_RETARDO,r16
	
	ldi	r16, 1	; Empezar hacia la derecha
	sts	EXPLORER_LCD_SENTIDO, r16

	
	; Borrar display del todo al cambiar de fichero:
	rcall	DESHAB_EXTRAM
	rcall	LCD_BORRAR


	ret


; ------------------------------------------------------------
;  PREPARA_TMR1_OVR_300ms
;
; Preparar TMR1 para que se desborde aprox. cada 0.3 segs:
; ------------------------------------------------------------
PREPARA_TMR1_OVR_300ms:
	; Divisor a CK/64
	clr	r16		; Modo normal
	out	TCCR1A, r16

	; Seguir contando:
	ldi	r16, 0x03
	out	TCCR1B, r16

	; Contador de 8 us cada unidad:
	; Poner en el contador 0xFFFF - FRECUENCIA_DISPLAY(us)/8
	clr	r16
	out	TCNT1H, r16
	out	TCNT1L, r16

	ldi	r16, high ( FRECUENCIA_DISPLAY )
	out	OCR1AH, r16
	ldi	r16,  low ( FRECUENCIA_DISPLAY )
	out	OCR1AL, r16
	
	; Quitar posible flag de desborde:
	ldi	r16, 0xE8
	out	TIFR, r16
	
	ret	




; -----------------------------------------------------------
;   	MODO MENU
;
;  Llamado con RCALL desde el modo explorador
;
;
; 	Es un menu con opciones y configuraciones, etc...
;
;
; -----------------------------------------------------------
MODO_MENU:
	ldi	r25, 5
	rcall	RETARDO_R25x25ms
	
	
	; Seleccionar primera opcion:
	ldi	r16, 0x01
	sts	MENU_OPCION,r16


	; Reconstruir pantalla:
	; Primera linea: Opcion actual
	; Segunda linea: Posicion dentro del menu, teclas usables.
MODO_MENU_LOOP:
	; Primera linea:
	; ------------------------------
	rcall	LCD_BORRAR
	
	lds	r16, MENU_OPCION
	
	cpi	r16, 1
	brne	mnu_no_1
	; OPCION 1
	ldi	ZH, high( MNU_1 * 2)
	ldi	ZL,  low( MNU_1 * 2)
	rjmp	modomnu_esc_lin
mnu_no_1:
	cpi	r16, 2
	brne	mnu_no_2
	; OPCION 2
	ldi	ZH, high( MNU_2 * 2)
	ldi	ZL,  low( MNU_2 * 2)
	rjmp	modomnu_esc_lin
mnu_no_2:
	cpi	r16, 3
	brne	mnu_no_3
	; OPCION 3: Bajos
	lds	r17, REPROD_FLAGS
	ldi	r16, 6		; Cuadro sin marcar
	sbrc	r17, REPR_FLAG_BASS
	ldi	r16, 7		; Cuadro con marca
	rcall	LCD_TX_DATO
	ldi	r16, ' '
	rcall	LCD_TX_DATO

	ldi	ZH, high( MNU_3 * 2)
	ldi	ZL,  low( MNU_3 * 2)
	rjmp	modomnu_esc_lin
mnu_no_3:
	cpi	r16, 4
	brne	mnu_no_4
	; OPCION 4: Sorround
	lds	r17, REPROD_FLAGS
	ldi	r16, 6		; Cuadro sin marcar
	sbrc	r17, REPR_FLAG_SORROUND
	ldi	r16, 7		; Cuadro con marca
	rcall	LCD_TX_DATO
	ldi	r16, ' '
	rcall	LCD_TX_DATO

	ldi	ZH, high( MNU_4 * 2)
	ldi	ZL,  low( MNU_4 * 2)
	rjmp	modomnu_esc_lin
mnu_no_4:
	cpi	r16, 5
	brne	mnu_no_5
	; OPCION 5
	ldi	ZH, high( MNU_5 * 2)
	ldi	ZL,  low( MNU_5 * 2)
	rjmp	modomnu_esc_lin
mnu_no_5:

	
	
modomnu_esc_lin:
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
	

modomnu_seglin:
	; Segunda linea:
	; ------------------------------
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16
	
	; Escribir numero de opcion con respecto al total:
	ldi	XH, high (STR_BUF)
	ldi	XL,  low (STR_BUF)
	
	lds	r18, MENU_OPCION
	rcall	BYTE2ASCII_R18
	
	ldi	r16, '/'
	st	X+, r16

	ldi	r18, NUM_MENUS
	rcall	BYTE2ASCII_R18

	ldi	r16, ' '
	st	X+, r16
	ldi	r16, 0x7F
	st	X+, r16
	ldi	r16, 0x7E
	st	X+, r16
		
	rcall	CERO_2_X

	
	ldi	XH, high (STR_BUF)
	ldi	XL,  low (STR_BUF)
	rcall	LCD_ESCRIBE_X
		
	
	; Retardo de cada ciclo: 300 ms aprox
	; ------------------------------------------
	ldi	r25, 14
	rcall	RETARDO_R25x25ms
	
		
	; Algun comando del teclado / mando a distancia??
	; -------------------------------------------------
	rcall	LEE_PULS_Y_IR

	lds	r16, COMANDO_USUARIO	; Del tecl o IR
	tst	r16
	breq	modomenu_nocmd
	
	ldi	r17, 0x00
	sts	COMANDO_USUARIO, r17
	
	;  Segun comando:
	cpi	r16, PULS_ALANTE
	brne	modomenu_sk1
	
	; Siguiente linea
	lds	r18, MENU_OPCION
	cpi	r18, NUM_MENUS
	breq	modomenu_sk1
	inc	r18
	sts	MENU_OPCION, r18
	rjmp	modomenu_nocmd
		
modomenu_sk1:	
	cpi	r16, PULS_ATRAS
	brne	modomenu_sk2
	
	; Anterior
	lds	r18, MENU_OPCION
	cpi	r18, 1
	breq	modomenu_sk2
	dec	r18
	sts	MENU_OPCION, r18
	rjmp	modomenu_nocmd
		
modomenu_sk2:	
	cpi	r16, PULS_STOP
	brne	modomenu_sk3
	
	; Salir del modo menu
	ret
		
modomenu_sk3:	
	cpi	r16, PULS_PLAY
	breq	modomenu_nosk3
	rjmp	modomenu_sk4
	
	; EJECUTAR OPCION:
	; ---------------------------
modomenu_nosk3:	
	lds	r18, MENU_OPCION
	cpi	r18, 0x01
	brne	modomnu_sel_sk1
	rjmp	MODO_MENU_SELEC_1
modomnu_sel_sk1:
	cpi	r18, 0x02
	brne	modomnu_sel_sk2
	rjmp	MODO_MENU_SELEC_2
modomnu_sel_sk2:
	cpi	r18, 0x03
	brne	modomnu_sel_sk3
	rjmp	MODO_MENU_SELEC_3
modomnu_sel_sk3:
	cpi	r18, 0x04
	brne	modomnu_sel_sk4
	rjmp	MODO_MENU_SELEC_4
modomnu_sel_sk4:
	cpi	r18, 0x05
	brne	modomnu_sel_sk5
	rjmp	MODO_MENU_SELEC_5
modomnu_sel_sk5:


	rjmp	MODO_MENU_LOOP
		
modomenu_sk4:	
	
	
	

modomenu_nocmd:
	rjmp	MODO_MENU_LOOP
	
	
; ---------------------------------------------
;   Seleccionado elemento del menu
;
;  Reproducir en modo lista todo el directorio 
;   seleccionado en modo aleatorio
; ---------------------------------------------
MODO_MENU_SELEC_2:

; ---------------------------------------------
;   Seleccionado elemento del menu
;
;  Reproducir en modo lista todo el directorio 
;   seleccionado:
; ---------------------------------------------
MODO_MENU_SELEC_1:
	; Estamos en un directorio?
	rcall	ES_DIRENTRY_DIRECT_VALIDO
	brcc	pon_error_no_dir

	; antes de nada, guardar donde estabamos
	rcall	SALVA_CONTEXT_EXPLORADOR
	

	rcall	FORMAR_LISTA_REPRODUCCION

;	ldi	YH, high (LST_REPR_LISTA)
;	ldi	YL,  low (LST_REPR_LISTA)
;	rjmp	FAT_DEBUG_DUMP_Y
	
	; Poner modo lista y salir con RET, de donde vinimos
	;  ya pondra en modo reproduccion:

	; Guardar num. de fichs en lista:
	lds	r16, LST_REPR_CUANTOS
	sts	REPROD_NUM_FICHS_TOTAL, r16
	
	; Si no hay, dar error:
	tst	r16
	breq	pon_error_no_mp3s	

	; Si hay ficheros
	
	; Formar la lista de orden. En este caso secuencial: 1, 2,...
	ldi	XH, high ( LST_REPR_ORDEN )
	ldi	XL,  low ( LST_REPR_ORDEN )
	ldi	r16, 0x01
	
	rcall	HAB_EXTRAM
	
modomn1_haz_orden_lp:
	st	X+, r16
	inc	r16
	brne	modomn1_haz_orden_lp
		
	; Poner flag de modo lista	
	lds	r16, REPROD_FLAGS
	ori	r16, 0x04	; MODO LISTA
	sts	REPROD_FLAGS, r16
		
	; Empezar por primer fichero:
	ldi	r16, 0x01	
	sts	REPROD_NUM_FICH_EN_DIR, r16
	
	; Si era aleatorio, desordenar la lista de orden:
	lds	r18, MENU_OPCION
	cpi	r18, 2
	breq	modomnu_aleatorio
	
	ret

; Aleatorizar la lista LST_REPR_ORDEN
;
;  Son REPROD_NUM_FICHS_TOTAL valores
; 
; --------------------------------------------------------
modomnu_aleatorio:
	lds	r18, REPROD_NUM_FICHS_TOTAL

	; X va apuntando a los valores de orden
	; Y va apuntando a valores x ahi en la RAM, 
	;   para hacer algo medio pseudoaleatorio, 
	;   a la vez q se XORea con TCNT0
	ldi	XH, high ( LST_REPR_ORDEN )
	ldi	XL,  low ( LST_REPR_ORDEN )
	
	ldi	YH, high ( SECTOR_BUFFER )
	ldi	YL,  low ( SECTOR_BUFFER )
	clr	r0
	in	r1, TCNT0
	add	YL, r1
	adc	YH, r0
	

	rcall	HAB_EXTRAM
	
	; Para el modulo:
	lds	r2, REPROD_NUM_FICHS_TOTAL
		
	; Aleatoriza entrada:
modomnu_aleat_lp:
	rcall	GENERAR_PSEUDOALEAT 	; En r16
	rcall	R16_MODULO_R2

	; Intercambiar LST_REPR_ORDEN[ R16 ] con [ X ]
	; ---------------------------------------------
	ldi	ZH, high ( LST_REPR_ORDEN )
	ldi	ZL,  low ( LST_REPR_ORDEN )
	add	ZL, r16
	adc	ZH, r0
	
	; Intercambiar X y Z
	ld	r16, X
	ld	r17, Z
	st	Z, r16
	st	X+, r17
	
	; Y asi hasta el final:
	dec	r18
	brne	modomnu_aleat_lp


	ret




pon_error_no_mp3s:
	rcall	DESHAB_EXTRAM
	
	rcall	LCD_BORRAR
	
	ldi	ZH, high( STR_NO_MP3s * 2)
	ldi	ZL,  low( STR_NO_MP3s* 2)
	
	rjmp	pon_error_sig

	
pon_error_no_dir:
	rcall	DESHAB_EXTRAM
	
	rcall	LCD_BORRAR
	
	ldi	ZH, high( STR_NO_DIR * 2)
	ldi	ZL,  low( STR_NO_DIR * 2)

pon_error_sig:
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
	
	ldi	r25, 40
	rcall	RETARDO_r25x25ms


	; Salir del modo MENU:
	ret
	

; ---------------------------------------------
;   Seleccionado elemento del menu
;
;   Bajos
; ---------------------------------------------
MODO_MENU_SELEC_3:
	lds	r16, REPROD_FLAGS
	ldi	r17, ( 1 << REPR_FLAG_BASS )
	eor	r16, r17
	sts	REPROD_FLAGS, r16

	rjmp	MODO_MENU_LOOP

; ---------------------------------------------
;   Seleccionado elemento del menu
;
;   Sorround
; ---------------------------------------------
MODO_MENU_SELEC_4:
	lds	r16, REPROD_FLAGS
	ldi	r17, ( 1 << REPR_FLAG_SORROUND )
	eor	r16, r17
	sts	REPROD_FLAGS, r16
	
	
	rjmp	MODO_MENU_LOOP


; ---------------------------------------------
;   Seleccionado elemento del menu
;
;   Probar IR:
; ---------------------------------------------
MODO_MENU_SELEC_5:
	; Ir mostrando el texto correspondiente a la tecla
	;   de la tabla IR_TABLA_COD_CMDS y grabar en EEP. nuevo codigo
	
	ldi	r20, 0	; r20 = indice de tecla actual
	
		
modomnu5_lp:
	rcall	LCD_BORRAR
	
	; Obtener texto de la tecla actual 
	ldi	ZH, high (IR_TABLA_COD_CMDS*2)
	ldi	ZL,  low (IR_TABLA_COD_CMDS*2)
	
	clr	r0	
	add	ZL, r20
	adc	ZH, r0
	
	; Cargar codigo de la tecla actual:
	lpm
	
	; Segun el codigo, poner su nombre:	
	ldi	ZH, high (TABLA_CODS_TXTS*2)
	ldi	ZL,  low (TABLA_CODS_TXTS*2)
	
	mov	r16, r0
	lsl	r16
	clr	r0
	add	ZL, r16
	adc	ZH, r0
	
	lpm
	mov	r17, r0
	adiw	ZL, 1
	lpm
	mov	ZH, r0
	mov	ZL, r17
	
	; Poner texto:
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH

	ldi	XH, high ( STR_BUF )	
	ldi	XL,  low ( STR_BUF )	
	
	ldi	r16, ' '
	st	X+, r16
	st	X+, r16
	
	mov	r18, r20
	rcall	BYTE2ASCII_R18
	
	rcall	CERO_2_X
	
	ldi	XH, high ( STR_BUF )	
	ldi	XL,  low ( STR_BUF )	
	rcall	LCD_ESCRIBE_X

	
	; Hay una nueva?
	lds	r16, IR_CODE_FINAL+0
	tst	r16
	brne	monomnu5_puede_ir
	rjmp	modomnu5_no_ir
	
monomnu5_puede_ir:	
	lds	r16, IR_CODE_FINAL+1
	tst	r16
	breq	modomnu5_no_ir
	
	; Si, hay un nuevo codigo, cogerlo y grabarlo en eeprom:
	ldi	XH, 0x00
	ldi	XL, 0x10	
	mov	r16, r20
	lsl	r16
	add	XL, r16
	
	lds	r16, IR_CODE_FINAL+0
	rcall	GRB_EEP
	lds	r16, IR_CODE_FINAL+1
	rcall	GRB_EEP

	; Y borrarlo ya:
	clr	r16
	sts	IR_CODE_FINAL+0, r16
	sts	IR_CODE_FINAL+1, r16	
	
modomnu5_no_ir:
	; 2º linea: Poner codigo de tecla de eeprom:
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16
	
	; Posicion en eeprom de los codigos
	ldi	XH, 0x00
	ldi	XL, 0x10	
	mov	r16, r20
	lsl	r16
	add	XL, r16
	
	rcall	LEE_EEPROM
	rcall	LCD_ESCRIBE_R16_HEX
	rcall	LEE_EEPROM
	rcall	LCD_ESCRIBE_R16_HEX



	; Retardo en cada ciclo:
	ldi	r25, 20
	rcall	RETARDO_R25x25ms

	
	; Procesar siguiente tecla y acabar:
	rcall	LEE_PULSADORES
	tst	r16
	breq	modomnu5_fin
	
	cpi	r16, PULS_STOP
	brne	modomnu5_sk1	
modomnu5_salir:	
	rjmp	MODO_MENU_LOOP	; Salir
	
modomnu5_sk1:
	cpi	r16, PULS_PLAY
	brne	modomnu5_sk2	
	
	; Pasar a siguiente tecla:
	inc	r20
	cpi	r20, (FIN_IR_TABLA_COD_CMDS- IR_TABLA_COD_CMDS)*2
	breq	modomnu5_salir

	rjmp	modomnu5_lp
modomnu5_sk2:





modomnu5_fin:	
	rjmp	modomnu5_lp
	
	


TABLA_CODS_TXTS:
;.equ	PULS_VOL_MAS	= 8
;.equ	PULS_VOL_MENOS	= 7
;.equ	PULS_ATRAS	= 6
;.equ	PULS_ALANTE	= 5
;.equ	PULS_STOP	= 4
;.equ	PULS_PLAY	= 3
; PULS_SUBE_DIR =2

	.dw	0	; 0
	.dw	0	; 1
	.dw	str_SUBDIR	* 2
	.dw	str_PLAY 	* 2
	.dw	str_STOP 	* 2
	.dw	str_ALAN 	* 2
	.dw	str_ATRAS 	* 2
	.dw	str_VOLMEN 	* 2
	.dw	str_VOLMAS 	* 2


str_PLAY:	.db	"Play", 0,0
str_STOP:	.db	"Stop", 0,0
str_ALAN:	.db	"Alante", 0,0
str_ATRAS:	.db	"Atras", 0
str_VOLMAS:	.db	"Vol +", 0
str_VOLMEN:	.db	"Vol -", 0
str_SUBDIR:	.db 	"Subir directorio", 0,0
	
; --------------------------------------------------------
; Prepara el comando correspondiente en COMANDO_USUARIO
;  si hay alguno, a traves del teclado o del mando.
; --------------------------------------------------------
LEE_PULS_Y_IR:
	rcall	LEE_PULSADORES
	
	tst	r16
	breq	leepuls_no
	
	sts	COMANDO_USUARIO, r16

leepuls_no:	
	lds	r16, IR_TIMEOUT
	tst	r16
	breq	leepuls_lee_ir_ok
	
	dec	r16
	push	r16

	rcall	IR_PROCESA_CODIGO	; Del IR:

	pop	r16
	sts	IR_TIMEOUT, r16
		
	ldi	r16, 0x00
	sts	COMANDO_USUARIO, r16
	
	ret
	
	
leepuls_lee_ir_ok:
	rcall	IR_PROCESA_CODIGO	; Del IR:

	ret


; ------------------------------------------------------------
; Guarda el direct. actual y posicion:
; ------------------------------------------------------------
SALVA_CONTEXT_EXPLORADOR:
	rcall	HAB_EXTRAM

	lds	r16, 	FAT_DIRECT_ACTUAL+0
	sts	CONTEXT_EXPL_DIR+0, r16
	lds	r16, 	FAT_DIRECT_ACTUAL+1
	sts	CONTEXT_EXPL_DIR+1, r16
	lds	r16, 	FAT_DIRECT_ACTUAL+2
	sts	CONTEXT_EXPL_DIR+2, r16
	lds	r16, 	FAT_DIRECT_ACTUAL+3
	sts	CONTEXT_EXPL_DIR+3, r16
	
	lds	r16, EXPLORER_ENTACT
	sts	CONTEXT_EXPL_ENT, r16


	rcall	DESHAB_EXTRAM
	ret

; ------------------------------------------------------------
; Recupera el direct. actual y posicion:
; ------------------------------------------------------------
RECUPERA_CONTEXT_EXPLORADOR:
	rcall	HAB_EXTRAM

	lds	r16, CONTEXT_EXPL_DIR+0
	sts	FAT_DIRECT_ACTUAL+0, r16
	lds	r16, CONTEXT_EXPL_DIR+1
	sts	FAT_DIRECT_ACTUAL+1, r16
	lds	r16, CONTEXT_EXPL_DIR+2
	sts	FAT_DIRECT_ACTUAL+2, r16
	lds	r16, CONTEXT_EXPL_DIR+3
	sts	FAT_DIRECT_ACTUAL+3, r16
	
	lds	r16, CONTEXT_EXPL_ENT
	sts	EXPLORER_ENTACT, r16
	
	rcall	DESHAB_EXTRAM
	ret
	

	

		
fin:	rjmp	fin




.include "fat32.asm"
.include "retardos.asm"
.include "conversiones.asm"
.include "ata.asm"
.include "lcd.asm"
.include "textos.asm"
.include "utils.asm"
.include "vs1001.asm"
.include "pulsadores.asm"
.include "ir.asm"
.include "ft245_usb.asm"
.include "CrearLista.asm"