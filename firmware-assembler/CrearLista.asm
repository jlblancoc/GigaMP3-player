;----------------------------------------------------
; FICHERO: CrearLista.asm
;
; DESCRIPCION: Rutinas de exploracion de directorios
;               y creacion de listas de reproduccion
;
; Jose Luis Blanco Claraco @ 2001-2002
;----------------------------------------------------


; --------------------------------------------------------
;  SELEC_FICH_LISTA_ORDEN_R16
;
;  Selecciona el fichero numero r16 segun el indice en el 
;   array LST_REPR_ORDEN
; --------------------------------------------------------
SELEC_FICH_LISTA_ORDEN_R16:
	rcall	HAB_EXTRAM

	ldi	XH, high ( LST_REPR_ORDEN )
	ldi	XL,  low ( LST_REPR_ORDEN )
	
	; El primero es 1:
	dec	r16
	clr	r0
	
	add	XL, r16
	adc	XH, r0
	
	; Coger indice:
	ld	r16, X

	; Y ahora seleccionar ese fichero:
	rjmp	SELEC_FICH_LISTA_R16

; --------------------------------------------------------
;  SELEC_FICH_LISTA_R16
;
;  Selecciona el fichero numero r16 de la lista formada.
;   Coge el numero indicado, no el indexado por la lista 
;   de LST_REPR_ORDEN, para eso hay que leer esa entrada
;   y luego llamar a esta rutina.
; --------------------------------------------------------
SELEC_FICH_LISTA_R16:
	rcall	HAB_EXTRAM

	; Contador de entradas en la lista
	ldi	r21, 0
	
	; Puntero que recorre la lista:
	ldi	XH, high ( LST_REPR_LISTA )
	ldi	XL,  low ( LST_REPR_LISTA )
	
	; Contador de entradas que quedan en este directorio:
	ldi	r22, 0x00	; para q nada mas empezar cargue el primero:

selecfichlst_lp:
	; Fin de lista?
	lds	r17, LST_REPR_CUANTOS
	cp	r17, r21
	breq	selecfichlst_fin	; No encontrado en lista !!
	
	; Buscar siguiente 	
	tst	r22
	brne	selecfchlst_no_fin_dir
	
	; Si, cargar nuevo directorio:
	ld	r22, X+	; Numero de entradas en nuevo directorio:

	ld	r2, X+	; Cluster del directorio:
	ld	r3, X+
	ld	r4, X+
	ld	r5, X+
	
	
selecfchlst_no_fin_dir:	
	; Coger fichero:
	ld	r23, X+	; Es el numero de entrada en el dir. actual
	
	dec	r22	; Num. de fichs q quedan en directorio
	inc	r21	; Num. de entrada en la lista total
	
	; Es el pedido?
	cp	r21, r16
	brne	selecfichlst_lp
	
	; Si, es el pedido, cargar:
	sts	FAT_DIRECT_ACTUAL+0, r2
	sts	FAT_DIRECT_ACTUAL+1, r3
	sts	FAT_DIRECT_ACTUAL+2, r4
	sts	FAT_DIRECT_ACTUAL+3, r5
	
	ldi	r16, 0x01
	sts	FAT_PETICION, r16
	
	sts	FAT_PETICION_NUM_ENTR, r23
	
	
	rcall	FAT_EXPLORA_DIRECTORIO
	
selecfichlst_fin:

	ret



; --------------------------------------------------------
; ES_DIRENTRY_DIRECT_VALIDO
;
;  Devuelve CARRY=1 si en DE_XXX hay un directorio y no 
;   es ni "." ni ".."
; --------------------------------------------------------
ES_DIRENTRY_DIRECT_VALIDO:
	lds	r16, FATDE_ATTR
	sbrs	r16, 4	; 0x10 = DIR
	rjmp	ESDEDIRVAL_fin_no

	lds	r17, FATDE_FILENAME+0
	lds	r18, FATDE_FILENAME+1
	lds	r19, FATDE_FILENAME+2
	
	cpi	r17, '.'
	brne	ESDEDIRVAL_no_punto
	cpi	r18, 0
	brne	ESDEDIRVAL_no_punto
	
	; Es "."
	rjmp	ESDEDIRVAL_fin_no
	
ESDEDIRVAL_no_punto:

	cpi	r17, '.'
	brne	ESDEDIRVAL_no_punto_punto
	cpi	r18, '.'
	brne	ESDEDIRVAL_no_punto_punto
	cpi	r19, 0
	brne	ESDEDIRVAL_no_punto_punto
	
	; Es ".."
	rjmp	ESDEDIRVAL_fin_no

ESDEDIRVAL_no_punto_punto:
	; SI
	sec
	ret

ESDEDIRVAL_fin_no:
	clc
	ret


; --------------------------------------------------------
;  FORMAR_LISTA_REPRODUCCION
;
;  Rutina recursiva que crea una lista con ficheros .MP3 
;   a partir del directorio que hay en FATDE_CLUSTER
;
;LST_REPR_CUANTOS:	.byte	1	; Numero de entradas en lista
;LST_REPR_ORDEN:		.byte	256	; Indices dentro de la lista siguiente:
;LST_REPR_LISTA:		.byte	400	; Para que sobre... segun el numero de 
;
; --------------------------------------------------------
FORMAR_LISTA_REPRODUCCION:
	rcall	HAB_EXTRAM



	; Parte que solo se ejecuta una vez: Iniciar lista:
	; ---------------------------------------------------
	clr	r16
	sts	LST_REPR_CUANTOS, r16

	; Vaciar cola de directorios a explorar
	clr	r16
	sts	CREAR_LST_COLA_PTR_IN, r16
	sts	CREAR_LST_COLA_PTR_OUT, r16
	
	; Y meter en cola el directorio seleccionado ahora:
	rcall	FORMARLISTA_ADD_DIR_COLA

	; Con X voy apuntando a la entrada en la lista:
	ldi	XH, high ( LST_REPR_LISTA )
	ldi	XL, low  ( LST_REPR_LISTA )

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


; ---------------------------------------------------
;
;  Parte de bucle para cada directorio
;
; ---------------------------------------------------
frm_lst_repr_expl_sig_dir:
	; Coger directorio de la cola:
	rcall	FORMARLISTA_GET_DIR_COLA
	brcc	frm_lst_repr_si_mas_dirs 	; C=1 = No hay mas! Se acabo el formar la lista
	rjmp	frm_lst_repr_fin
	
frm_lst_repr_si_mas_dirs:
	; Guardar puntero para luego poner numero de fichs MP3 encontrados:
	push	XL
	push	XH	
	ldi	r16, 0
	st	X+, r16	; por ahora, dejar un cero:
	
	; Ahora el cluster del directorio y
	; Entrar en directorio:
	lds	r16, FATDE_CLUSTER+0
	sts	FAT_DIRECT_ACTUAL+0, r16
	st	X+, r16
	
	lds	r16, FATDE_CLUSTER+1
	sts	FAT_DIRECT_ACTUAL+1, r16
	st	X+, r16
	
	lds	r16, FATDE_CLUSTER+2
	sts	FAT_DIRECT_ACTUAL+2, r16
	st	X+, r16
	
	lds	r16, FATDE_CLUSTER+3
	sts	FAT_DIRECT_ACTUAL+3, r16
	st	X+, r16
	
	
	; Ver cuantas entradas hay aqui:
	ldi	r16, 0x00
	sts	FAT_PETICION, r16
	
	rcall	FAT_EXPLORA_DIRECTORIO
	
	rcall	HAB_EXTRAM

	; En r10 guardo el numero de entradas en este directorio:
	lds	r10, FAT_DIRENT_ENCONTRADAS
	inc	r10	; Para que al compararlo y sea igual, sea el fin
	
	; Con r11 voy recorriendo cada elemento:
	clr	r11
	inc	r11
	
	; Con r12 cuento el numero de fichs. MP3 encontrados:
	clr	r12
	
		
; ---------------------------------------------------
;
;  Parte de bucle para cada fich. en cada directorio
;
; ---------------------------------------------------
frm_lst_repr_sig_fich:
	cp	r11, r10	; Ya no hay mas?
	brne	frm_lst_repr_si_mas_fichs
	
	; Ya no hay mas ficheros: 
	; --------------------------
	
	; Ahora hay que terminar la entrada en la lista con 
	;  el numero de fichs q se han encontrado:
	pop	YH
	pop	YL
	
	st	Y, r12
	
	; Si en este directorio no habia MP3s, recuperar (X):
	tst	r12
	brne	frm_lst_repr_si_habia
	
	; Directorio sin MP3s:
	mov	XH, YH
	mov	XL, YL
	
frm_lst_repr_si_habia:	
	; Y pasar al siguiente directorio:
	rjmp	frm_lst_repr_expl_sig_dir
			
	
frm_lst_repr_si_mas_fichs:

	; Ver entrada:
	ldi	r16, 0x01
	sts	FAT_PETICION, r16
	sts	FAT_PETICION_NUM_ENTR, r11	
	rcall	FAT_EXPLORA_DIRECTORIO

	rcall	HAB_EXTRAM


	; Ver si es un directorio:
	lds	r16, FATDE_ATTR
	sbrc	r16, 4	; 0x10 = Dir:
	rjmp	frm_lst_repr_es_dir

	; Es un fichero:
	; ------------------------
	rcall	TEST_FILENAME_ES_MP3 ; C=1 si SI lo es.
	brcc	frm_lst_repr_siguiente	; NO
	
	; Si es MP3: Añadir a la lista:
	lds	r16, LST_REPR_CUANTOS	; Vigilar limite de 255!!
	cpi	r16, 0xFF
	breq	frm_lst_repr_siguiente


	inc	r12	; Contador de fichs. encontrados:	
	
	st	X+, r11	; Num. de entrada en este directorio	
	
	; Y contador global:
	lds	r16, LST_REPR_CUANTOS
	inc	r16
	sts	LST_REPR_CUANTOS, r16
	
	; Siguiente:
	rjmp	frm_lst_repr_siguiente
		
	
	; Es un directorio:
	; ------------------------
frm_lst_repr_es_dir:	
	rcall	ES_DIRENTRY_DIRECT_VALIDO
	brcc	frm_lst_repr_siguiente ; Dir no valido para explorar
	
	; Si, añadir a la cola de reproduccion:
	rcall	FORMARLISTA_ADD_DIR_COLA
	

	
frm_lst_repr_siguiente:
	; Pasar a la siguiente entrada dentro de este directorio
	; -------------------------------------------------------
	inc	r11		; Siguiente entrada
	rjmp	frm_lst_repr_sig_fich



	; Salir, lista ya formada	
frm_lst_repr_fin:


			
	ret


; --------------------------------------------------------
;  FORMARLISTA_ADD_DIR_COLA
; 
;   Añade el directorio DE_CLUSTER a la cola de exploracion
;
;
;CREAR_LST_COLA_DIRS:	.byte	4*70	; Numero de directorios max. en cola
;CREAR_LST_COLA_PTR_IN:	.byte   1	; Punteros de indice (1º=0) de entrada y salida en cola
;CREAR_LST_COLA_PTR_OUT:	.byte   1	
;
; --------------------------------------------------------
FORMARLISTA_ADD_DIR_COLA:
	push	r16
	push	r17
	push	XH
	push	XL
	
	; Apuntar a entrada:
	;  CREAR_LST_COLA_DIRS[ CREAR_LST_COLA_PTR_IN ]
	ldi	XH, high ( CREAR_LST_COLA_DIRS )
	ldi	XL,  low ( CREAR_LST_COLA_DIRS )
	
	lds	r16, CREAR_LST_COLA_PTR_IN
	clr	r17
	
	; *= 4
	lsl	r16
	rol	r17

	lsl	r16
	rol	r17
	
	; Sumar a puntero:
	add	XL, r16
	adc	XH, r17
	
	
	; X apunta a donde debemos meter la nueva entrada:
	lds	r16, FATDE_CLUSTER+0
	st	X+, r16
	lds	r16, FATDE_CLUSTER+1
	st	X+, r16
	lds	r16, FATDE_CLUSTER+2
	st	X+, r16
	lds	r16, FATDE_CLUSTER+3
	st	X+, r16


	
	; Incrementar puntero de entradas:
	lds	r16, CREAR_LST_COLA_PTR_IN
	inc	r16
	
	; Final? Es circular:
	cpi	r16, TAM_CREARLISTAS_COLA	
	brne	frmlst_add_fin
	
	; Al principio:
	clr	r16
	
frmlst_add_fin:
	sts	CREAR_LST_COLA_PTR_IN, r16
	
	pop	XL
	pop	XH
	pop	r17
	pop	r16
	ret
	
; --------------------------------------------------------
;  FORMARLISTA_GET_DIR_COLA
; 
;   Devuelve en DE_CLUSTER un directorio de la cola
;
;   CARRY=1 si ya no hay mas!
;
;
;CREAR_LST_COLA_DIRS:	.byte	4*70	; Numero de directorios max. en cola
;CREAR_LST_COLA_PTR_IN:	.byte   1	; Punteros de indice (1º=0) de entrada y salida en cola
;CREAR_LST_COLA_PTR_OUT:	.byte   1	
;
; --------------------------------------------------------
FORMARLISTA_GET_DIR_COLA:
	push	r16
	push	r17
	push	XH
	push	XL
	
	; No hay mas si PTR_IN=PTR_OUT:
	lds	r16, CREAR_LST_COLA_PTR_OUT
	lds	r17, CREAR_LST_COLA_PTR_IN
	cp	r16, r17
	brne	frmlstgetdir_sihay
	
	; No hay:
	sec	
	rjmp	frmlst_get_sal
	
frmlstgetdir_sihay:	
	; Apuntar a entrada:
	;  CREAR_LST_COLA_DIRS[ CREAR_LST_COLA_PTR_OUT ]
	ldi	XH, high ( CREAR_LST_COLA_DIRS )
	ldi	XL,  low ( CREAR_LST_COLA_DIRS )
	
	lds	r16, CREAR_LST_COLA_PTR_OUT
	clr	r17
	
	; *= 4
	lsl	r16
	rol	r17

	lsl	r16
	rol	r17
	
	; Sumar a puntero:
	add	XL, r16
	adc	XH, r17

	
	; X apunta a donde debemos meter la nueva entrada:
	ld	r16, X+
	sts	FATDE_CLUSTER+0, r16
	ld	r16, X+
	sts	FATDE_CLUSTER+1, r16
	ld	r16, X+
	sts	FATDE_CLUSTER+2, r16
	ld	r16, X+
	sts	FATDE_CLUSTER+3, r16




	; Incrementar puntero de entradas:
	lds	r16, CREAR_LST_COLA_PTR_OUT
	inc	r16
	
	; Final? Es circular:
	cpi	r16, TAM_CREARLISTAS_COLA	
	brne	frmlst_get_fin
	
	; Al principio:
	clr	r16
	
frmlst_get_fin:
	sts	CREAR_LST_COLA_PTR_OUT, r16
	
	clc	; No era el final
		
frmlst_get_sal:	
	pop	XL
	pop	XH
	pop	r17
	pop	r16
	ret

	
