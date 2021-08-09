;-----------------------------------------------------------------------------
; FICHERO: fat32.asm
;
; DESCRIPCION: Funciones de FAT32, directorios, ficheros, etc...
;
;
; RUTINAS:
;
;
; * FAT_INICIAR
;
; * FAT_CLUSTER2SECTOR
;    -> r2..r5: Numero de cluster (r2= LSB)
;    <- En RAM: LBA_DIR el sector en LBA correspondiente
;
; * FAT_SIGUIENTE_CLUSTER
;    -> r2..r5: Numero de cluster (r2= LSB) de un fichero/directorio
;    <- r2..r5: Numero del siguiente cluster segun la FAT
;    <- CARRY=1 si ya no hay mas clusters (fin de fichero)
;
; * FAT_EXPLORA_DIRECTORIO
;     Va explorando las entradas del directorio y realiza una accion segun
;      un parametro
;
;  -> FAT_DIRECT_ACTUAL: Cluster del directorio actual
;	.....
;
;
;
;-----------------------------------------------------------------------------


;-----------------------------------------------------------------------------
;	FAT_FATAL_ERROR
;-----------------------------------------------------------------------------
FAT_FATAL_ERROR:
	cli

	rcall	LCD_BORRAR	
	
	ldi	ZH, high( STR_FATERR * 2)
	ldi	ZL,  low( STR_FATERR * 2)
	rcall	LCD_ESCRIBE_CADENA_Z_FLASH
	
fat_fatalerror:
	rjmp	fat_fatalerror
	
;-----------------------------------------------------------------------------
;
;	FAT_INICIAR
;
;-----------------------------------------------------------------------------
FAT_INICIAR:

	; Leer sector MBR
	; ---------------------------
	clr	r16
	sts	LBA_DIR, r16
	sts	LBA_DIR+1, r16
	sts	LBA_DIR+2, r16
	sts	LBA_DIR+3, r16

	ldi	r16, 1
	sts	SECTOR_CNT, r16
	
	ldi	YH, high( SECTOR_BUFFER )
	ldi	YL,  low( SECTOR_BUFFER )
	
	rcall	ATA_ReadSectors
	
	; Comprobar firma $55AA en $0400 + $01FE
	rcall	HAB_EXTRAM
	
	lds	r16, SECTOR_BUFFER + 0x01FE
	cpi	r16, 0x55
	brne	FAT_FATAL_ERROR
	lds	r16, SECTOR_BUFFER + 0x01FF
	cpi	r16, 0xAA
	brne	FAT_FATAL_ERROR
	
	; Es un disco formateado OK

	; Buscar el boot sector:
	ldi	XH, high( SECTOR_BUFFER +0x01C6 )
	ldi	XL,  low( SECTOR_BUFFER +0x01C6 )
	ldi	YH, high( LBA_DIR )
	ldi	YL,  low( LBA_DIR )
	rcall	COPIAR_X_2_Y_32bits
		
	ldi	YH, high( SECTOR_BUFFER )
	ldi	YL,  low( SECTOR_BUFFER )

	rcall	ATA_ReadSectors		; Leer el BOOT SECTOR

	; Cargar datos del boot sector:
	; ------------------------------------
;	rcall	DESHAB_EXTRAM
;	ldi	r16, 'H'
;	rcall	LCD_TX_DATO
;	ldi	r16, 'D'
;	rcall	LCD_TX_DATO
;	ldi	r16, ':'
;	rcall	LCD_TX_DATO
	
	
	rcall	HAB_EXTRAM
	
	; Numero de sectores por cluster
	lds	r16, SECTOR_BUFFER+ 13
	sts	BPB_SECSPERCLUS,r16
	
	; Cluster del dir. raiz en memoria y como
	;  directorio actual
	ldi	XH, high( SECTOR_BUFFER+ 44 )
	ldi	XL,  low( SECTOR_BUFFER+ 44 )
	ldi	YH, high( BPB_CLUS_DIR_RAIZ )
	ldi	YL,  low( BPB_CLUS_DIR_RAIZ )
	rcall	COPIAR_X_2_Y_32bits

	ldi	XH, high( SECTOR_BUFFER+ 44 )
	ldi	XL,  low( SECTOR_BUFFER+ 44 )
	ldi	YH, high( FAT_DIRECT_ACTUAL )
	ldi	YL,  low( FAT_DIRECT_ACTUAL )
	rcall	COPIAR_X_2_Y_32bits


	; Primer sector de datos:
	; -------------------------------
	lds	r16,  SECTOR_BUFFER+ 16	; Numero de fats:
	
	; Solo soporto que sean 2 !!
	cpi	r16, 2
	breq	fat_ini_copias_2_ok
	rjmp	FAT_FATAL_ERROR
fat_ini_copias_2_ok:
	
	lds	r8,  SECTOR_BUFFER+ 36   ; Numero de sectores / fat
	lds	r9,  SECTOR_BUFFER+ 36 +1
	lds	r10, SECTOR_BUFFER+ 36 +2
	lds	r11, SECTOR_BUFFER+ 36 +3

	; Multiplicar x numero de fats (2):
	lsl	r8	; LSByte
	rol	r9
	rol	r10
	rol	r11	; MSByte
	

	; Calc. primer sector de FAT:	
	; ----------------------------
	lds	r2, SECTOR_BUFFER + 14	; + Num. de sectores reservados:
	lds	r3, SECTOR_BUFFER + 15
	clr	r4
	clr	r5
	
	; y sumar el offset a partir del BOOT SECTOR:
	lds	r6, LBA_DIR	; Suponer tb. q solo es 1 byte... :-(
	add	r2,r6
	adc	r3,r0
	adc	r4,r0
	adc	r5,r0

	; Guardarlo:
	sts	FAT_PRIMER_SEC_FAT,   r2	
	sts	FAT_PRIMER_SEC_FAT+1, r3
	sts	FAT_PRIMER_SEC_FAT+2, r4	
	sts	FAT_PRIMER_SEC_FAT+3, r5
	
	; Primer sector de datos = primer sector de fat + tam. de FATs	
;	clr	r0
	add	r8,r2
	adc	r9,r3
	adc	r10,r4
	adc	r11,r5


	; Guardar como primer sector de datos:
	sts	FAT_PRIMER_SEC_DATOS,   r8
	sts	FAT_PRIMER_SEC_DATOS+1, r9
	sts	FAT_PRIMER_SEC_DATOS+2, r10	
	sts	FAT_PRIMER_SEC_DATOS+3, r11
	
		
	; Mostrar en display nombre del volumen:
	; ---------------------------------------
;	ldi	XH, high( SECTOR_BUFFER+ 71 )
;	ldi	XL,  low( SECTOR_BUFFER+ 71 )
	
;fat_ini_label_loop:
;	ld	r16, X+
;	rcall	DESHAB_EXTRAM
;	rcall	LCD_TX_DATO
;	rcall	HAB_EXTRAM
;	cpi	XL,  low( SECTOR_BUFFER+ 71 +11 )
;	brne	fat_ini_label_loop

	
	
	; Cache de FAT:
	; ----------------
	clr	r16
	sts	CACHE_FAT_SECTOR+0, r16
	sts	CACHE_FAT_SECTOR+1, r16
	sts	CACHE_FAT_SECTOR+2, r16
	sts	CACHE_FAT_SECTOR+3, r16

	
	rcall	DESHAB_EXTRAM
	ret
	
	


;-----------------------------------------------------------------------------
;	FAT_CLUSTER2SECTOR
;
; -> r2..r5: Numero de cluster (r2= LSB)
;
; <- En RAM: LBA_DIR el sector en LBA correspondiente
;-----------------------------------------------------------------------------
FAT_CLUSTER2SECTOR:
	; ( Cluster - 2 ) * SecPerClus + FirstDataSec
	
	; Cluster - 2
	clr	r0
	ldi	r16, 0x02
	sub	r2, r16
	sbc	r3, r0		
	sbc	r4, r0		
	sbc	r5, r0		
	
	; * SecPerClus: Es potencia de 2, luego hacer como LSLs:
	lds	r6, BPB_SECSPERCLUS
fat_clus2sec_mulloop:
	sbrc	r6,0	
	rjmp	fat_clus2sec_mulfin
	
	; *= 2
	lsl	r2
	rol	r3
	rol	r4
	rol	r5
	
	; Siguiente bit:
	lsr	r6
	rjmp	fat_clus2sec_mulloop	
fat_clus2sec_mulfin:

	; Sumar FAT_PRIMER_SEC_DATOS:
	lds	r6, FAT_PRIMER_SEC_DATOS
	lds	r7, FAT_PRIMER_SEC_DATOS+1
	lds	r8, FAT_PRIMER_SEC_DATOS+2
	lds	r9, FAT_PRIMER_SEC_DATOS+3
	
	add	r2, r6
	adc	r3, r7
	adc	r4, r8
	adc	r5, r9
	
	; Y guardar resultado en: LBA_DIR
	sts	LBA_DIR+0, r2
	sts	LBA_DIR+1, r3
	sts	LBA_DIR+2, r4
	sts	LBA_DIR+3, r5	

	ret

;-----------------------------------------------------------------------------
;	FAT_SIGUIENTE_CLUSTER
;
; -> r2..r5: Numero de cluster (r2= LSB) de un fichero/directorio
;
; <- r2..r5: Numero del siguiente cluster segun la FAT
;
; 	Hay q leer del disco un sector para consultar la FAT
;	  y se escribe en el buffer FAT_SEC_BUFFER en RAM.
;	Se usa tambien como CACHE de lectura, ya que si es el 
; 	 mismo sector que la vez anterior, no se lee del disco :-)
;
;	Si es el final de fichero, en la fat hay un 
;	 0x0FFFFFFF
;  	y devuelvo CARRY=1, sino CARRY=0
;
;-----------------------------------------------------------------------------
FAT_SIGUIENTE_CLUSTER:
	; Calcular direccion en bytes de la entrada en la FAT:
	;  Numero de cluster * 4:
	lsl	r2
	rol	r3
	rol	r4
	rol	r5
	
	lsl	r2
	rol	r3
	rol	r4
	rol	r5
	
	; Guardar para luego: Offset sobre el sector
	push	r2
	push	r3

	; % 512 para saber numero de sector con respecto
	;  al comienzo de la FAT: 
	; Guardar en r6..r8
	mov	r6, r3
	mov	r7, r4
	mov	r8, r5
	lsr	r8
	ror	r7
	ror	r6	; r6 = LSByte
	
	; Sumar al primer sector de la fat para leer 
	;  el sector que tiene la entrada del cluster 
	;  que buscamos:
	
	lds	r16, FAT_PRIMER_SEC_FAT	+ 0
	lds	r17, FAT_PRIMER_SEC_FAT	+ 1
	lds	r18, FAT_PRIMER_SEC_FAT	+ 2
	lds	r19, FAT_PRIMER_SEC_FAT	+ 3
	
	clr	r0
	add	r16, r6
	adc	r17, r7
	adc	r18, r8
	adc	r19, r0

	; Leer ese sector:
	sts	LBA_DIR+0, r16	
	sts	LBA_DIR+1, r17
	sts	LBA_DIR+2, r18	
	sts	LBA_DIR+3, r19
	
	
	; CACHE: Comprobar si ya esta leido!!
	; ------
	lds	r1, CACHE_FAT_SECTOR+0
	cp	r1, r16
	brne	fatsigclus_no_en_cache
	lds	r1, CACHE_FAT_SECTOR+1
	cp	r1, r17
	brne	fatsigclus_no_en_cache
	lds	r1, CACHE_FAT_SECTOR+2
	cp	r1, r18
	brne	fatsigclus_no_en_cache
	lds	r1, CACHE_FAT_SECTOR+3
	cp	r1, r19
	brne	fatsigclus_no_en_cache
	
	; Si esta en cache:
	; ----------------------
	rjmp	fatsigclus_si_en_cache
	
	
fatsigclus_no_en_cache:
	; Pues guardar la direccion de este sector para saber que 
	;  ya esta cargado por si la proxima se repite:
	sts	CACHE_FAT_SECTOR+0, r16
	sts	CACHE_FAT_SECTOR+1, r17
	sts	CACHE_FAT_SECTOR+2, r18
	sts	CACHE_FAT_SECTOR+3, r19	

	ldi	r16, 1
	sts	SECTOR_CNT, r16
	
	ldi	YH, high( FAT_SEC_BUFFER )
	ldi	YL, low ( FAT_SEC_BUFFER )
	
	rcall	ATA_ReadSectors
	
fatsigclus_si_en_cache:

	; Offset del sector: r2 + bit 0 de r3:
	pop	r3
	pop	r2

	ldi	r16, 0x01
	and	r3, r16
	
	ldi	YH, high( FAT_SEC_BUFFER )
	ldi	YL, low ( FAT_SEC_BUFFER )
	
	add	YL, r2
	adc	YH, r3
	
	; Y apunta a siguiente cluster en LSB primero:
	rcall	HAB_EXTRAM
	
	ld	r2, Y+
	ld	r3, Y+
	ld	r4, Y+
	ld	r5, Y+
	
	; En realidad son 28 bits: Sobran 4:
	mov	r16, r5
	andi	r16, 0x0F
	mov	r5, r16
	

	; Fin de fichero?
	ldi	r16, 0x0F
	cp	r5, r16
	brne	fatsigclus_fin
	ldi	r16, 0xFF
	cp	r4, r16
	brne	fatsigclus_fin
	cp	r3, r16
	brne	fatsigclus_fin
	cp	r2, r16
	brne	fatsigclus_fin
	
	sec	
	ret
	
fatsigclus_fin:
	clc
	ret
	


; ******************************************
; ******************************************
FAT_DEBUG_DUMP_Y:
	rcall	DESHAB_EXTRAM
	
dump_loop2:
	rcall	LCD_BORRAR	
	
	ldi	r23, 10
dump_loop:
	rcall	HAB_EXTRAM
	ld	r16, Y+

	rcall	DESHAB_EXTRAM
	rcall	LCD_ESCRIBE_R16_HEX
;	rcall	LCD_TX_DATO
	
	dec	r23
	brne	dump_loop
	
	ldi	r16, 0x40
	rcall	LCD_SET_CURSOR_POS_R16
	ldi	r23, 10
dump_loop3:
	rcall	HAB_EXTRAM
	ld	r16, Y+

	rcall	DESHAB_EXTRAM
	rcall	LCD_ESCRIBE_R16_HEX
;	rcall	LCD_TX_DATO
		
	dec	r23
	brne	dump_loop3
	

	ldi	r25, 180
	rcall	RETARDO_R25x25ms
	
	rjmp	dump_loop2




;-----------------------------------------------------------------------------
;	FAT_EXPLORA_DIRECTORIO
;
; Va explorando las entradas del directorio y realiza una accion segun
;  un parametro
;
;  -> FAT_DIRECT_ACTUAL: Cluster del directorio actual
;
;  -> FAT_PETICION: Indica que hacer:
;	0x00: Contar: devuelve en FAT_DIRENT_ENCONTRADAS el numero de ficheros y directorios
;	0x01: Leer entrada de directorio decodificada y guardarla en 
;		su registro en ram ( Campos FATDE_XXXX )
;
;  -> FAT_PETICION_NUM_ENTR: Numero de entrada a leer.	
; 
;
;-----------------------------------------------------------------------------
FAT_EXPLORA_DIRECTORIO:
	push	XH
	push	XL
	push	YH
	push	YL

	; r13 = numero de sector del "directorio" en el que estamos:
	ldi	r16, 0xFF
	mov	r13, r16	; Nada mas empezar se incrementa y 
				;  pasa a cero
				
	; Iniciar cluster actual: FATEXPLORER_CLUSACT
	ldi	XH, high( FAT_DIRECT_ACTUAL )
	ldi	XL, low ( FAT_DIRECT_ACTUAL )
	ldi	YH, high( FATEXPLORER_CLUSACT )
	ldi	YL, low ( FATEXPLORER_CLUSACT )
	rcall	COPIAR_X_2_Y_32bits
	
	
	
	; r14 = 1 si estoy analizando una entrada larga.
	clr	r14
	
	; FAT_DIRENT_ENCONTRADAS a cero:
	clr	r16
	sts	FAT_DIRENT_ENCONTRADAS, r16
	
	; (Z) apunta en SECTOR_BUFFER a la entrada analizada en cada momento
	ldi	ZH, high ( SECTOR_BUFFER +512 )
	ldi	ZL, low ( SECTOR_BUFFER + 512 ) ; +512 para q nada mas empezar
						;  cargue el primer sector:
	


	; -------------------------------
	; 	      BUCLE
	; -------------------------------
fat_explor_dir_loop:
	; Siguiente sector ?
	cpi	ZH, high( SECTOR_BUFFER +512 )	
	brne	ve_fat_explor_dir_no_sector

	cpse	r0,r0
ve_fat_explor_dir_no_sector:	
	rjmp	fat_explor_dir_no_sector
	

	; Si, necesitamos incrementar el contador de sector 
	;  actual en el directorio:
	inc	r13
	
	; Siguiente cluster??
	lds	r16, BPB_SECSPERCLUS
	cp	r16, r13
	brne	fat_explor_dir_no_cluster
	
	; Si, hay que pasar al siguiente cluster:
	lds	r2, FATEXPLORER_CLUSACT+0
	lds	r3, FATEXPLORER_CLUSACT+1
	lds	r4, FATEXPLORER_CLUSACT+2
	lds	r5, FATEXPLORER_CLUSACT+3



	rcall	FAT_SIGUIENTE_CLUSTER
	brcc	fat_explor_no_fin_direct
	rjmp	fat_explor_dir_ve_fin		; Final de directorio !!	
fat_explor_no_fin_direct:
	

	; Guardar nuevo cluster actual
	sts	FATEXPLORER_CLUSACT+0,r2
	sts	FATEXPLORER_CLUSACT+1,r3
	sts	FATEXPLORER_CLUSACT+2,r4
	sts	FATEXPLORER_CLUSACT+3,r5
		
	clr	r13	; y ver primer sector en el nuevo cluster

fat_explor_dir_no_cluster:

	; Cargar el sector numero "r13" del cluster 
	;  "FATEXPLORER_CLUSACT"
	
	; Pasar de cluster a sector y sumarle r13:
	lds	r2, FATEXPLORER_CLUSACT+0
	lds	r3, FATEXPLORER_CLUSACT+1
	lds	r4, FATEXPLORER_CLUSACT+2
	lds	r5, FATEXPLORER_CLUSACT+3
	
	rcall	FAT_CLUSTER2SECTOR

	lds	r2, LBA_DIR+0
	lds	r3, LBA_DIR+1
	lds	r4, LBA_DIR+2
	lds	r5, LBA_DIR+3

	clr	r0
	add	r2, r13
	adc	r3, r0
	adc	r4, r0
	adc	r5, r0

	sts	LBA_DIR+0,r2
	sts	LBA_DIR+1,r3
	sts	LBA_DIR+2,r4
	sts	LBA_DIR+3,r5
	
	; Y cargar ese sector:
	ldi	YH, high ( SECTOR_BUFFER  )
	ldi	YL, low  ( SECTOR_BUFFER  ) 

	ldi	r16,0x01
	sts	SECTOR_CNT, r16
		
	rcall	ATA_ReadSectors


	; Iniciar puntero a comienzo del nuevo sector:
	ldi	ZH, high ( SECTOR_BUFFER  )
	ldi	ZL, low  ( SECTOR_BUFFER  ) 	

fat_explor_dir_no_sector:
	rcall	HAB_EXTRAM

	; Final de entradas?? 
	; Si DIR_Name[0]= 0x00
	; Si DIR_Name[0]= 0xE5 es borrada: IGNORAR
	ld	r23, Z
	
	cpi	r23, 0x00
	breq	fat_explor_dir_ve_fin
	cpi	r23, 0xE5
	breq	fat_explo_ignorar_ent
	

	cpse	r0,r0	; Saltarse el RJMP	
fat_explor_dir_ve_fin:
	; Fin de entradas:
	rjmp	fat_explor_dir_fin

	; Ver ATTRIB de DirectoryEntry:
	ldd	r16, Z+11
	
	; Entrada larga??
	cpi	r16, 0x0F	
	breq	fat_explor_entrada_larga
	
	; Si es un "volume" ignorar:
	sbrc	r16, 3	; 0x08 = VOLUME_ID
	rjmp	fat_explor_dir_sig
	
	; Sera una entrada corta:
	sbrc	r16, 4	; 0x10
	rjmp	fat_explor_entrada_corta	; Es un directorio
	
	; El resto de valores, deben de ser entradas cortas de ficheros:
;	sbrc	r16, 5	; 0x20
	rjmp	fat_explor_entrada_corta	; Es un fichero

	
fat_explo_ignorar_ent:	
	rjmp	fat_explor_dir_sig


	;  ENTRADA LARGA
	; ------------------------------------
fat_explor_entrada_larga:
	; Es la primera?
	tst	r14
	brne	fat_explor_entrada_larga_prim

	; Si es la primera
	
	; Iniciar puntero para formar nombres largos:
	clr	r16
	sts	FATDE_FILENAME_CONTA, r14
		

	; No es la primera
fat_explor_entrada_larga_prim:

	; Indicar entrada larga:
	ldi	r16, 0x01
	mov	r14, r16

	; Si solo estamos contando las entradas, no formar
	;  los nombres:
	lds	r16, FAT_PETICION
	cpi	r16, 0x00	; No contar: Formar nombre
	brne	fat_explor_entlar_formar	
	
	; Solo contar: No hacer trabajo innecesario
	rjmp	fat_explor_dir_sig	

fat_explor_entlar_formar:

	; Ir formando nombre de fichero en buffer
	;  FATDE_FILENAME 
	;  Contador de bytes: 
	;  FATDE_FILENAME_CONTA	
	;
	; Cada entrada tiene hasta 13 caracteres en posiciones:
	; Z + ....  1 3 5 7 9 14 16 18 20 22 24 28 30
	; 
	; Hay que cargarlos al reves por si acaban con un 0x00 
	;   o un 0xFF e ignorar esos bytes. Al final se invertira
	;   el buffer.
	ldd	r16, Z + 30
	rcall	fat_formar_nombre
	ldd	r16, Z + 28
	rcall	fat_formar_nombre
	ldd	r16, Z + 24
	rcall	fat_formar_nombre
	ldd	r16, Z + 22
	rcall	fat_formar_nombre
	ldd	r16, Z + 20
	rcall	fat_formar_nombre
	ldd	r16, Z + 18
	rcall	fat_formar_nombre
	ldd	r16, Z + 16
	rcall	fat_formar_nombre
	ldd	r16, Z + 14
	rcall	fat_formar_nombre
	ldd	r16, Z + 9
	rcall	fat_formar_nombre
	ldd	r16, Z + 7
	rcall	fat_formar_nombre
	ldd	r16, Z + 5
	rcall	fat_formar_nombre
	ldd	r16, Z + 3
	rcall	fat_formar_nombre
	ldd	r16, Z + 1
	rcall	fat_formar_nombre
	
		
	rjmp	fat_explor_dir_sig


	;  ENTRADA CORTA
	; ------------------------------------
fat_explor_entrada_corta:
	; Forma parte de una entrada larga encadenada??
	tst	r14
	breq	fat_explor_entcorta
	
	; Es la ultima de una entrada larga: Poner cero de fin de cadena:
	ldi	XH, high ( FATDE_FILENAME )
	ldi	XL,  low ( FATDE_FILENAME )
	
	lds	r17, FATDE_FILENAME_CONTA
	
	; Posicionar 
	clr	r0
	add	XL, r17
	adc	XH, r0
	
	; Acabar cadena
	st	X+, r0
	
	; Dar la vuelta al nombre de fichero que se habra guardado 
	;  en el buffer en orden inverso:
	ldi	XH, high ( FATDE_FILENAME )
	ldi	XL,  low ( FATDE_FILENAME )
	mov	YH, XH
	mov	YL, XL

	lds	r17, FATDE_FILENAME_CONTA
	dec	r17	; Direccionar el ultimo caracter ESCRITO
	
	clr	r0
	add	XL, r17
	adc	XH, r0

	; X apunta al FINAL
	; Y apunta al INICIO
	; Recorrer NUM_CARACTERES / 2
	lds	r18, FATDE_FILENAME_CONTA
	lsr	r18

	clr	r0
	ldi	r19, 0x01
fat_explor_reordenar_filename:
	ld	r16, Y
	ld	r17, X
	
	st	Y+, r17
	st	X, r16
		
	sub	XL, r19
	sbc	XH, r0
	
	
	dec	r18
	brne	fat_explor_reordenar_filename
	
	rjmp	fat_explor_entcorta_sig
		

fat_explor_entcorta:
	; Es corta sin mas: 
	
	; Copiar nombre de fichero: 8 caracteres:
	mov	XH, ZH
	mov	XL, ZL
	
	ldi	YH, high( FATDE_FILENAME )
	ldi	YL,  low( FATDE_FILENAME )
	
	ldi	r18, 8
	rcall	COPIAR_X_2_Y_R18_BYTES
	
	; Hay extension??
	ldd	r16, Z+8
	cpi	r16, ' '
	brne	fat_explor_si_extension
	ldd	r16, Z+9
	cpi	r16, ' '
	brne	fat_explor_si_extension
	ldd	r16, Z+10
	cpi	r16, ' '
	brne	fat_explor_si_extension
	
	; Si no hay extension, no poner ni el punto!
	rjmp	fat_explor_fin_extension	

fat_explor_si_extension:
	ldi	r16, '.'
	st	Y+, r16

	; Y copiar extension: 3 caracteres
	ldi	r18, 3
	rcall	COPIAR_X_2_Y_R18_BYTES
	
	; Quitar espacios en blanco al final, si los hay:
fat_explor_fin_extension:
	ld	r16, -Y

fat_explor_nombre_trim:
	ld	r16, Y
	cpi	r16, ' '
	brne	fat_explor_fin_trim
	
	; Quitar ese espacio y seguir:
	ld	r16, -Y
	rjmp	fat_explor_nombre_trim
	
	
fat_explor_fin_trim:
	; Terminar cadena con un 0x00:
	clr	r16
	adiw	YL, 1
	st	Y+, r16
		
	
fat_explor_entcorta_sig:	
	; Quitar flag de estoy en entrada larga:
	clr	r14
	
	; Resetear contador de direccion de texto largo:
	sts	FATDE_FILENAME_CONTA, r14
	
	
	; Incrementar contador de entradas encontradas:
	lds	r16, FAT_DIRENT_ENCONTRADAS
	inc	r16
	sts	FAT_DIRENT_ENCONTRADAS, r16
	
	; Si solo contando entradas no coger mas datos:
	lds	r16, FAT_PETICION
	cpi	r16, 0x00
	breq	fat_explor_dir_sig	; Solo contar
	
	
	; Se supone que es un comando de recuperar entrada:
	; ----------------------------------------------------
			
	; Si el numero de entrada pedida es la actual, 
	;  copiar el resto de datos al FAT_DE y salir
	lds	r16, FAT_PETICION_NUM_ENTR
	lds	r17, FAT_DIRENT_ENCONTRADAS
	cp	r16, r17
	brne	fat_explor_dir_sig
	
	; Si, es la que buscamos:
	; --------------------------------
	 
	; Atributo:
	ldd	r16, Z+11
	sts	FATDE_ATTR, r16
	
	; Clusters:
	ldd	r16, Z+26
	sts	FATDE_CLUSTER+0, r16
	ldd	r16, Z+27
	sts	FATDE_CLUSTER+1, r16
	ldd	r16, Z+20
	sts	FATDE_CLUSTER+2, r16
	ldd	r16, Z+21
	sts	FATDE_CLUSTER+3, r16

	; Tamaño fichero:
	ldd	r16, Z+28
	sts	FATDE_FILESIZE+0,r16
	ldd	r16, Z+29
	sts	FATDE_FILESIZE+1,r16
	ldd	r16, Z+30
	sts	FATDE_FILESIZE+2,r16
	ldd	r16, Z+31
	sts	FATDE_FILESIZE+3,r16
	
	; Ya esta, salir:
	rjmp	fat_explor_dir_fin
		


	
fat_explor_dir_sig:

	; Siguiente entrada de directorio:
	adiw	ZL, 32
	rjmp	fat_explor_dir_loop
	
fat_explor_dir_fin:

	rcall	DESHAB_EXTRAM

	pop	YL
	pop	YH
	pop	XL
	pop	XH
	
	ret
	

; --------------------------------------------------------
; fat_formar_nombre
;
;   Va formando el nombre largo con los bytes en trozos
;    Byte entra en r16, si ve que vale, lo guarda en 
;     FATDE_FILENAME, segun el indice FATDE_FILENAME_CONTA
; --------------------------------------------------------
fat_formar_nombre:
	push 	ZH
	push	ZL
	
	cpi	r16, 0x00
	breq	fat_formar_nombre_fin
	cpi	r16, 0xFF
	breq	fat_formar_nombre_fin
	
	; Caracter valido:
	ldi	XH, high ( FATDE_FILENAME )
	ldi	XL,  low ( FATDE_FILENAME )
	
	lds	r17, FATDE_FILENAME_CONTA
	
	; Si estamos en el maximo del buffer, no guardar caracter:
	cpi	r17, FAT_MAX_FILENAME
	breq	fat_formar_nombre_fin
	
	clr	r0
	add	XL, r17
	adc	XH, r0
	
	; Transformaciones de caracteres: Si 
	ldi	ZH, high ( FAT_TRANSF_CHARS*2 )
	ldi	ZL,  low ( FAT_TRANSF_CHARS*2 )
	
fat_frm_nom_lp:
	; En r1 esta el origen, en r0 por lo que se debe cambiar:
	lpm
	mov	r1, r0
	adiw	ZL, 1
	lpm
	adiw	ZL, 1
	
	tst	r1
	breq	fat_frmnom_ok
	
	; Es r1?
	cp	r1, r16
	brne	fat_frm_nom_lp
	
	; Si, cambiar:
	mov	r16, r0
		
fat_frmnom_ok:
	st	X+, r16
	
	inc	r17
	sts	FATDE_FILENAME_CONTA, r17

fat_formar_nombre_fin:

	pop	ZL
	pop	ZH
	
	ret
	

; Tabla para transformar caracteres que no se verían bien
;  en el display:
FAT_TRANSF_CHARS:
	.db	0xE1,'a'
	.db	0xE9,'e'
	.db	0xED,'i'
	.db	0xF3,'o'
	.db	0xFA,'u'
	.db	0xC1,'A'
	.db	0xC9,'E'
	.db	0xCD,'I'
	.db	0xD3,'O'
	.db	0xDA,'U'
	.db	0xA1,'!'
	.db	0xBF,'?'
	.db	0xF1,0xEE	; Eñe mayusc y minus.
	.db	0xD1,0xEE	; Eñe mayusc y minus.
	
	.db	0,0
	
; --------------------------------------------------------
;  CARACTER_MINUSCULA
;  
;  Transforma en caracter en r16 a minuscula
; --------------------------------------------------------
CARACTER_MINUSCULA:
	push	r17
	
	cpi	r16, 'A'
	brlo	car_minus_fin
	cpi	r16, 'Z'+1
	brsh	car_minus_fin
	
	; Es mayuscula:
	ldi	r17, 'a'-'A'
	add	r16, r17
	
car_minus_fin:

	pop	r17
	ret
	

	
; --------------------------------------------------------
;   TEST_FILENAME_ES_MP3
;
; Devuelve CARRY = 1/0 si el nombre de fichero en 
;   FATDE_FILENAME acaba/o no en ".mp3"
;	
; --------------------------------------------------------
TEST_FILENAME_ES_MP3:
	push	XH
	push	XL
	
	ldi	XH, high (FATDE_FILENAME)
	ldi	XL,  low (FATDE_FILENAME)
	rcall	STRLEN

	; X apunta a despues del 0x00
	sbiw 	XL, 1
	; Ahora apunta al cero del final:
	
	ld	r16, -X
	rcall	CARACTER_MINUSCULA
	cpi	r16, '3'
	brne	tst_file_mp3_no

	ld	r16, -X
	rcall	CARACTER_MINUSCULA
	cpi	r16, 'p'
	brne	tst_file_mp3_no

	ld	r16, -X
	rcall	CARACTER_MINUSCULA
	cpi	r16, 'm'
	brne	tst_file_mp3_no
	
	; Si es MP3:
	
	pop	XL
	pop	XH
	
	sec
	ret	

tst_file_mp3_no:
	pop	XL
	pop	XH

	clc
	ret