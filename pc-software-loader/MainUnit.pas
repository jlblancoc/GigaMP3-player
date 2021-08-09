unit MainUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ExtCtrls, ComCtrls, ToolWin, ImgList, StdCtrls,math, Buttons;


const
        TAM_SECTOR_CACHE        =  200 ;

type
 TDirectoryEntry = record
        FileName        : AnsiString;
        Attrib          : BYTE;
        FileSize        : DWORD;
        FirstCluster    : DWORD;
 end;

  TForm1 = class(TForm)
    Panel1: TPanel;
    ImageList1: TImageList;
    ToolBar1: TToolBar;
    btnConectar: TToolButton;
    btnSalir: TToolButton;
    PageControl1: TPageControl;
    TabSheet1: TTabSheet;
    TabSheet2: TTabSheet;
    TabSheet3: TTabSheet;
    lbDisps: TListBox;
    Panel2: TPanel;
    btnEnum: TButton;
    ToolButton1: TToolButton;
    ToolButton2: TToolButton;
    Memo1: TMemo;
    Button1: TButton;
    StatusBar1: TStatusBar;
    btnDisc: TToolButton;
    Button2: TButton;
    Button3: TButton;
    Button4: TButton;
    Panel3: TPanel;
    txtDir: TStaticText;
    btnRefresh: TBitBtn;
    lbExpl: TListBox;
    GroupBox1: TGroupBox;
    lbSPC: TLabel;
    lbRoot: TLabel;
    btnExplDirAct: TButton;
    btnRecibirFich: TBitBtn;
    edDestDir: TEdit;
    prog: TProgressBar;
    btnCalcEspacio: TButton;
    lbEspa: TLabel;
    edSectorNum: TEdit;
    btnEscri: TButton;
    lbPrimSecDat: TLabel;
    procedure btnSalirClick(Sender: TObject);
    procedure btnEnumClick(Sender: TObject);
    procedure btnConectarClick(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure btnDiscClick(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure btnRefreshClick(Sender: TObject);
    procedure btnExplDirActClick(Sender: TObject);
    procedure lbExplDblClick(Sender: TObject);
    procedure btnRecibirFichClick(Sender: TObject);
    procedure btnCalcEspacioClick(Sender: TObject);
    procedure btnEscriClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }

    function  LeerSector( LBA: LongInt): Boolean;

    function  EscribeSector( LBA: LongInt; var buffer: array of byte): Boolean;

    // ( Cluster - 2 ) * SecPerClus + FirstDataSec
    function  Cluster2Sector ( Clus: DWORD): DWORD;

        // Devuelve el siguiente cluster de uno dado:
        //  true: Si no hay mas
    function  SiguienteCluster ( ClusAct: DWORD; var ClusSig : DWORD): Boolean;

    // Explorar un directorio y analizar su contenido:
    function  FAT_Explorer( ClusDirActual : DWORD; SoloCuenta: Boolean; NumEntrada : WORD; var res : TDirectoryEntry ): WORD;


  end;

var
  Form1         : TForm1;
  SECTOR_BUF    : Array [0..511] of Byte;

  SECTORS_CACHE : array[ 0..TAM_SECTOR_CACHE*512 ] of BYTE;
  SECTORS_CACHE_INDEX: array [ 1..TAM_SECTOR_CACHE ] of DWORD; // Dice que LBAs estan guardados
  SECTORS_CACHE_COLA     : array [ 1..TAM_SECTOR_CACHE ] of Integer; // Para ver la antiguedad de la entrada 


  // Variables para sistema FAT:
  BPB_SECSPERCLUS       : BYTE;         // Sectores por cluster

  BPB_CLUS_DIR_RAIZ     : DWORD;        // Cluster de /

  FAT_PRIMER_SEC_DATOS  : DWORD;        // En LBA.
  FAT_PRIMER_SEC_FAT    : DWORD;

  CLUS_DIR_ACTUAL       : DWORD;        // Cluster
  Str_DIR_ACTUAL        : AnsiString;


implementation

uses D2XXUnit;

{$R *.dfm}

procedure ResetCache;
var
        i       : integer;
begin
        for i:=1 to TAM_SECTOR_CACHE do
        begin
                SECTORS_CACHE_INDEX[i]:= $FFFFFFFF;
                SECTORS_CACHE_COLA[i]:=0;
        end;
end;


procedure TForm1.btnSalirClick(Sender: TObject);
begin
        Application.Terminate;
end;

procedure TForm1.btnEnumClick(Sender: TObject);
var
        i,n     : Integer;
        s       : AnsiString;
begin
        GetFTDeviceCount;
        n:= FT_Device_Count;

        lbDisps.Clear;

        lbDisps.Items.add( inttostr(n)+' dispositivos:' );

        for i:=0 to n-1 do
        begin
                GetFTDeviceDescription(i);
                s:= FT_Device_String;
                GetFTDeviceSerialNo(i);
                s:= s+ '('+FT_Device_String+')';

                lbDisps.Items.add(s);
        end;


end;

procedure TForm1.btnConectarClick(Sender: TObject);
var
        i       : integer;
        s       : AnsiString;
begin
//        Open_USB_Device;
        if FT_OK <> Open_USB_Device_By_Device_Description( 'Giga MP3 by JLBC''03'  ) then
                exit;

        btnConectar.Enabled:=false;
        btnDisc.Enabled:=true;

        Set_USB_Device_TimeOuts( 700,700 );


        // Limpiar buffer de Rx:
        Get_USB_Device_QueueStatus;
        if (  FT_Q_Bytes > 0 ) then
                Read_USB_Device_Buffer( FT_Q_Bytes );


        // Identificacion: CMD $05
        FT_Out_Buffer[0]:= $05;

        FT_Out_Buffer[1]:= $00;
        FT_Out_Buffer[2]:= $96;
        FT_Out_Buffer[3]:= $69;
        FT_Out_Buffer[4]:= $FF;

        i:= Write_USB_Device_Buffer( 5 );
        if (i<>5) then
        begin
                ShowMessage('Error al enviar comando $05!');
                exit;
        end;


        Sleep(500);
        Get_USB_Device_QueueStatus;
        if ( FT_Q_Bytes <> 14 ) then
        begin
                ShowMessage('No hay respuesta! '+inttostr(FT_Q_Bytes));
                exit;
        end;

        // Recibir 14 bytes:
        i:=Read_USB_Device_Buffer( 14 );

        if (i<>14) then
        begin
                ShowMessage('Error al recibir cadena de ID! '+inttostr(i));
                exit;
        end;

        s:='';
        for i:=0 to 13 do
                s:=s + chr( FT_In_Buffer[i] );


        StatusBar1.Panels[0].Text:='Conectado. Version= "'+ s + '"';

        // Actualizar:
        btnRefreshClick( self );
        btnExplDirActClick( self );
end;

procedure TForm1.Button1Click(Sender: TObject);
var
        s       : AnsiString;
        i,j       : Integer;
begin
        LeerSector( Strtoint( edSectorNum.Text ) );

        memo1.clear;

        for i:=0 to (511 div 16) do
        begin
                s:= IntTohex (i*16, 4)+': ' ;
                for j:=0 to 15 do
                        s:=s+inttohex( SECTOR_BUF[i*16+j],2 );

                s:=s+ ' | ';

                for j:=0 to 15 do
                begin
                        if chr( SECTOR_BUF[i*16+j] ) in
                                ['a'..'z','A'..'Z','0'..'9',',','.','¿'  ] then
                        s:=s+  chr( SECTOR_BUF[i*16+j] )
                        else
                        s:=s+  '.';


                end;

                memo1.lines.add( s );

        end;


        

end;

//************************************************************
//  LeeSector del HD del reproductor MP3
//
//  Devuelve false si hay algun error.
//  Sector en "SECTOR_BUF"
//
//  Uso un sistema de Cache para mejorar el rendimiento:
//************************************************************
function TForm1.LeerSector( LBA: LongInt): Boolean;
var
        i : Integer;
        min_cache, indx_cache   : integer;
        descontar       : Boolean;
begin
        result:=false;

        // Esta ya en cache?
        for i:=1 to TAM_SECTOR_CACHE do
                if SECTORS_CACHE_INDEX[i]= LBA then
                begin
                        // Acierto!
                        if ( SECTORS_CACHE_COLA[i] < 3 ) then
                                SECTORS_CACHE_COLA[i]:=SECTORS_CACHE_COLA[i] + 1;

                        CopyMemory(
                                @SECTOR_BUF[0] ,
                                @SECTORS_CACHE [ 512* (i-1) ],
                                512);
                                
                        exit;
                end;



        // Enviar comando 0x10 para leer sector:
        FT_Out_Buffer[0]:= $10;
        FT_Out_Buffer[1]:= LOBYTE( LOWORD( LBA ));
        FT_Out_Buffer[2]:= HIBYTE( LOWORD( LBA ));
        FT_Out_Buffer[3]:= LOBYTE( HIWORD( LBA ));
        FT_Out_Buffer[4]:= HIBYTE( HIWORD( LBA ));

        i:= Write_USB_Device_Buffer( 5 );
        if (i<>5) then
        begin
                ShowMessage('Error al enviar comando $10!');
                exit;
        end;

        // Recibir bytes:
        i:=Read_USB_Device_Buffer( 512 );
        if (i<>512) then
        begin
                ShowMessage('Error al recibir sector ! '+inttostr(i));
                exit;
        end;

        // Copiar a destino
        CopyMemory( @SECTOR_BUF[0] , @FT_In_Buffer[0], 512);


        // Y a cache:
        // Elegir victima:
        // Descontar de todos los contadores:
        min_cache:=10000;
        indx_cache:=0;

        // Solo descontar si no hay ninguno a cero:
        descontar:=true;

        for i:=1 to TAM_SECTOR_CACHE do
                if SECTORS_CACHE_COLA[i]=0 then
                        descontar:=false;


        for i:=1 to TAM_SECTOR_CACHE do
        begin
                if descontar then
                  if SECTORS_CACHE_COLA[i]>0 then
                        dec ( SECTORS_CACHE_COLA[i] );

                if SECTORS_CACHE_COLA[i]<= min_cache then
                begin
                        min_cache:= SECTORS_CACHE_COLA[i];
                        indx_cache:= i;
                end;
        end;

        // Guardar en cache:
        CopyMemory( @ SECTORS_CACHE [ (indx_cache-1)*512 ] , @SECTOR_BUF[0], 512);
        SECTORS_CACHE_INDEX[ indx_cache ] := LBA;
        SECTORS_CACHE_COLA [indx_cache]:= 1;

        result:=true;
end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
        if FT_HANDLE<> 0 then
                Close_USB_Device;
end;

procedure TForm1.btnDiscClick(Sender: TObject);
begin
        // Reset de sistema
        FT_Out_Buffer[0]:= $06;


        Write_USB_Device_Buffer( 5 );


        if FT_HANDLE<> 0 then
                Close_USB_Device;

        btnConectar.Enabled:=true;
        btnDisc.Enabled:=false;

end;

procedure TForm1.Button2Click(Sender: TObject);
begin
        FT_Out_Buffer[0]:= $05;

        FT_Out_Buffer[1]:= $01;
        FT_Out_Buffer[2]:= $02;
        FT_Out_Buffer[3]:= $03;
        FT_Out_Buffer[4]:= $04;

        Write_USB_Device_Buffer( 5 );

end;

procedure TForm1.Button3Click(Sender: TObject);
begin
        FT_Out_Buffer[0]:= $05;

        FT_Out_Buffer[1]:= $FF;
        FT_Out_Buffer[2]:= $EE;
        FT_Out_Buffer[3]:= $DD;
        FT_Out_Buffer[4]:= $CC;

        Write_USB_Device_Buffer( 5 );

end;

procedure TForm1.Button4Click(Sender: TObject);
begin
        Get_USB_Device_QueueStatus;

        FillMemory( @FT_In_Buffer[0] ,10, 0) ;

        Read_USB_Device_Buffer( FT_Q_Bytes );

        memo1.lines.add( 'Buffer RX: '+ inttostr ( FT_Q_Bytes ) +' = '+
                inttohex(FT_In_Buffer[0],2)+' '+
                inttohex(FT_In_Buffer[1],2)+' '+
                inttohex(FT_In_Buffer[2],2)+' '+
                inttohex(FT_In_Buffer[3],2)+' '+
                inttohex(FT_In_Buffer[4],2)+' '+
                inttohex(FT_In_Buffer[5],2)+' '
                );


end;

procedure TForm1.btnRefreshClick(Sender: TObject);
var
        dw,dw2      : DWORD;
        BootSector  : DWORD;
begin
        // Inicia el sistema de cache:
        ResetCache;

        // Recargar datos de la FAT32
        LeerSector( $0 );       // MBR

        // Firma?
        if ( SECTOR_BUF[ 510 ] <> $55 ) or
           ( SECTOR_BUF[ 511 ] <> $AA ) then
        begin
                showmessage('Error en MBR!!');
                exit;
        end;

        // Leer el Boot sector:
        BootSector
          := (( SECTOR_BUF[ $01C6+0 ] ) shl 0) or
             (( SECTOR_BUF[ $01C6+1 ] ) shl 8) or
             (( SECTOR_BUF[ $01C6+2 ] ) shl 16) or
             (( SECTOR_BUF[ $01C6+3 ] ) shl 24);

        LeerSector( BootSector );


        //
        BPB_SECSPERCLUS:= SECTOR_BUF[ 13 ];
        lbSPC.Caption:='Sec/Clus='+inttostr( BPB_SECSPERCLUS );

        dw:= (( SECTOR_BUF[ 44+0 ] ) shl 0) or
             (( SECTOR_BUF[ 44+1 ] ) shl 8) or
             (( SECTOR_BUF[ 44+2 ] ) shl 16) or
             (( SECTOR_BUF[ 44+3 ] ) shl 24);

        BPB_CLUS_DIR_RAIZ:=dw;
        CLUS_DIR_ACTUAL := dw;
        Str_DIR_ACTUAL := '/';

        lbRoot.Caption:='RootClus='+inttohex(BPB_CLUS_DIR_RAIZ, 8);

        //
        dw:= (( SECTOR_BUF[ 36+0 ] ) shl 0) or
             (( SECTOR_BUF[ 36+1 ] ) shl 8) or
             (( SECTOR_BUF[ 36+2 ] ) shl 16) or
             (( SECTOR_BUF[ 36+3 ] ) shl 24);
        dw:= dw * SECTOR_BUF [ 16 ];

        // Secs reservados antes de fat
        dw2:= (( SECTOR_BUF[ 14+0 ] ) shl 0) or
              (( SECTOR_BUF[ 14+1 ] ) shl 8);

        FAT_PRIMER_SEC_FAT:= dw2 + BootSector;

        FAT_PRIMER_SEC_DATOS:= FAT_PRIMER_SEC_FAT + dw;

        lbPrimSecDat.caption:= inttohex( FAT_PRIMER_SEC_DATOS, 8 );


end;

procedure TForm1.btnExplDirActClick(Sender: TObject);
var
        i,n     : WORD;
        de      : TDirectoryEntry;
        s,tmp       : AnsiString;
begin
        // Explorar directorio actual:
        // -------------------------------------------
        txtDir.Caption:= 'Directorio actual: '+ Str_DIR_ACTUAL;

        lbExpl.Clear;


        n:=FAT_Explorer( CLUS_DIR_ACTUAL, true, 0, de );

        for i:=1 to n do
        begin
                FAT_Explorer( CLUS_DIR_ACTUAL, false, i, de );

                s:= de.FileName;

                // Directorio:
                if 0 <> (de.Attrib and $10) then
                        s:='['+s+']';

                // Columnas con mas datos:
                while (length(s)<80) do s:=s+' ';

                // Size:
                tmp:=inttostr( de.FileSize div 1024 )+' Kb';
                while (length(tmp)<10) do tmp:=' '+tmp;

                s:=s+ tmp;


                lbExpl.items.add( s );

        end;






end;

// ( Cluster - 2 ) * SecPerClus + FirstDataSec
function  TForm1.Cluster2Sector ( Clus: DWORD): DWORD;
begin
        result:= (clus-2) * BPB_SECSPERCLUS + FAT_PRIMER_SEC_DATOS;
end;

// Devuelve el siguiente cluster de uno dado:
//  true: Si no hay mas
function  TForm1.SiguienteCluster ( ClusAct: DWORD; var ClusSig : DWORD): Boolean;
var
        dw,dir     : DWORD;
begin
        result:=false;

        // Direccion de cluster con respecto a inicio de tabla FAT:
        dir:= clusact* 4 ;

        LeerSector( dir div 512 + FAT_PRIMER_SEC_FAT );

        dir:= dir mod 512;

        dw:= (( SECTOR_BUF[ dir+0 ] ) shl 0) or
             (( SECTOR_BUF[ dir+1 ] ) shl 8) or
             (( SECTOR_BUF[ dir+2 ] ) shl 16) or
             (( SECTOR_BUF[ dir+3 ] ) shl 24);

        ClusSig:= dw and $0FFFFFFF;

        // Ultimo si es 0x0FFFFFFF
        if ( ClusSig = $0FFFFFFF ) then result:=true;

end;

// Explorar un directorio y analizar su contenido:
function  TForm1.FAT_Explorer( ClusDirActual : DWORD; SoloCuenta: Boolean; NumEntrada : WORD; var res : TDirectoryEntry ): WORD;
var
        cuenta_entradas : WORD;
        clus_act,dw     : DWORD;
        es_larga        : Boolean;

        tmp_filename    : AnsiString;

        ptrEnSector     : WORD;
        NumSectEnDirec  : WORD;
        fin_exploracion : Boolean;
        atrib           : BYTE;

        str_buf         : array [0..20] of char;


        procedure FormaNombre( c : BYTE );
        begin
                if (c<>0)and(c<>$FF) then
                        tmp_filename:= chr(c) +tmp_filename;
        end;


begin
        clus_act:= ClusDirActual;

        es_larga:= false;
        fin_exploracion:= false;

        // Contador:
        cuenta_entradas:=0;

        ptrEnSector:= 512; // Para q cargue justo al empezar:
        NumSectEnDirec:= $FFFF;


        while not fin_exploracion do
        begin
                if (ptrEnSector = 512) then
                begin
                        // Cargar nuevo sector del directorio:
                        inc( NumSectEnDirec );

                        if ( NumSectEnDirec= BPB_SECSPERCLUS ) then
                        begin
                                // Pasar a siguiente cluster:
                                NumSectEnDirec:= 0;

                                if SiguienteCluster( clus_act,clus_act ) then
                                        // Fin de directorio
                                        fin_exploracion:=true;

                        end;

                        // Cargar sector:
                        dw:=Cluster2Sector( clus_act );
                        dw:= dw + NumSectEnDirec;

                        LeerSector( dw );

                        ptrEnSector:= 0 ;
                end;

        // Si DIR_NAME[0] = 0 -> final de entrada.
        if ( SECTOR_BUF[ ptrEnSector ] = $00 ) then
                fin_exploracion:= true
        else
        if ( SECTOR_BUF[ ptrEnSector ] <> $E5 ) then // Si no es borrada:
        begin
                // Procesar entrada:
                atrib:=SECTOR_BUF[ ptrEnSector + 11 ];

                if (atrib = $0F) then
                begin
                        // Entrada larga
                        if not es_larga then
                        begin
                                // Es la primera:
                                tmp_filename:='';
                                es_larga:=true;
                        end;

                        if not SoloCuenta then
                        begin
                                // Formar nombres:
                                
        {
        ; Cada entrada tiene hasta 13 caracteres en posiciones:
	; Z + ....  1 3 5 7 9 14 16 18 20 22 24 28 30
	;
	; Hay que cargarlos al reves por si acaban con un 0x00
	;   o un 0xFF e ignorar esos bytes. Al final se invertira
	;   el buffer.
        }
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 30 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 28 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 24 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 22 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 20 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 18 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 16 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 14 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 9 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 7 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 5 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 3 ] );
                                FormaNombre( SECTOR_BUF[ ptrEnSector + 1 ] );

                        end;

                end     // Fin de entrada larga
                else if ( (atrib and 8)<>0 ) then
                begin
                        // Es Volumen: Ignorar
                end
                else
                begin   // ENTRADA CORTA:
                        if es_larga then
                        begin
                                // Es de una larga encadenada:
                                //  El nombre ya se ha formado
                        end
                        else
                        begin
                                // No, es corta y ya esta:
                                CopyMemory( @str_buf[0], @SECTOR_BUF[ ptrEnSector + 0 ], 8);
                                str_buf[8]:=chr(0);
                                tmp_filename:= trim( StrPas( str_buf ) );

                                CopyMemory( @str_buf[0], @SECTOR_BUF[ ptrEnSector + 8 ], 3);
                                str_buf[3]:=chr(0);

                                // Solo poner extension si tiene:
                                if length(trim( StrPas( str_buf ) )) >0 then
                                tmp_filename:= tmp_filename + '.';
                                tmp_filename:= tmp_filename+
                                        trim( StrPas( str_buf ) );


                                tmp_filename:= copy ( tmp_filename, 1, 11);
                        end;

                        es_larga:=false;
                        inc( cuenta_entradas );

                        if cuenta_entradas= NumEntrada then
                        begin
                                // Recuperar los datos pedidos:
                                res.Attrib:= atrib;
                                res.FileName:= tmp_filename;

                                dw:= (( SECTOR_BUF[ ptrEnSector + 28 ] ) shl 0) or
                                     (( SECTOR_BUF[ ptrEnSector + 29 ] ) shl 8) or
                                     (( SECTOR_BUF[ ptrEnSector + 30 ] ) shl 16) or
                                     (( SECTOR_BUF[ ptrEnSector + 31 ] ) shl 24);
                                res.FileSize:= dw;

                                dw:= (( SECTOR_BUF[ ptrEnSector + 26 ] ) shl 0) or
                                     (( SECTOR_BUF[ ptrEnSector + 27 ] ) shl 8) or
                                     (( SECTOR_BUF[ ptrEnSector + 20 ] ) shl 16) or
                                     (( SECTOR_BUF[ ptrEnSector + 21 ] ) shl 24);
                                res.FirstCluster:= dw;

                                result:= cuenta_entradas;
                                exit;
                        end;

                        tmp_filename:='';


                end;


        end;

        // Siguiente:
        inc( ptrEnSector, 32 );

        end;

        






        result:= cuenta_entradas;

end;


procedure TForm1.lbExplDblClick(Sender: TObject);
var
        i       : Integer;
        de      : TDirectoryEntry;
begin
        // Doble clic en directorio: Entrar en el:

        i:= lbExpl.ItemIndex;
        if (i<0) then exit;

        Inc (i);        // Primero = 1

        FAT_Explorer( CLUS_DIR_ACTUAL, false, i, de );


        if 0<> (de.Attrib and $10) then
        begin
                CLUS_DIR_ACTUAL:= de.FirstCluster;

                if CLUS_DIR_ACTUAL=0 then CLUS_DIR_ACTUAL:= BPB_CLUS_DIR_RAIZ;

                // Actualizar texto de ruta:
                if de.FileName='.' then
                begin
                        // Nada
                end else
                if de.FileName='..' then
                begin
                        // Quitar el ultimo
                        i:= length( Str_DIR_ACTUAL )-1;

                        while (i>0) and ( Str_DIR_ACTUAL[i]<>'/' ) do dec(i);

                        Str_DIR_ACTUAL:= copy (Str_DIR_ACTUAL, 1, i );                       


                end else
                begin
                        // Poner nuevo:
                        Str_DIR_ACTUAL:=Str_DIR_ACTUAL+ de.FileName +'/';

                end;




                btnExplDirActClick( self );
        end;


end;

procedure TForm1.btnRecibirFichClick(Sender: TObject);
var
        i,numsec : Integer;
        de      : TDirectoryEntry;
        f       : File of byte;
        b       : BYTE;
        bytes_quedan,clus,LBA    : DWORD;
        tim1, tim2      : DWORD;
        aux     : AnsiString;
        cada_x  : Integer;

begin
        i:= lbExpl.ItemIndex;
        if (i<0) then exit;

        Inc (i);        // Primero = 1

        FAT_Explorer( CLUS_DIR_ACTUAL, false, i, de );


        if 0<> (de.Attrib and $10) then exit; // directorio

        // Es un fichero:
        AssignFile( f, edDestDir.Text+'\'+de.FileName );
        Rewrite(f);

        numsec:=0;
        clus:= de.FirstCluster;
        bytes_quedan:= de.FileSize;

        prog.min:=0;
        prog.max:= bytes_quedan div 512;
        prog.Position:=0;

        tim1:= GetTickCount;

        cada_x:=0;

        while (bytes_quedan>0) do
        begin
                // Leer sector:
                LBA:= Cluster2Sector( clus )+ numsec;

                if not LeerSector( lba ) then exit;

                inc (cada_x);

                if cada_x= 20 then
                begin
                        cada_x:=0;

                        prog.Position:= (de.filesize - bytes_quedan ) div 512;

                        tim2:= GetTickCount;
                        StatusBar1.Panels[1].Text:=
                                inttostr(de.filesize - bytes_quedan )+
                                '/'+
                                inttostr(de.filesize )+
                                '  VELOCIDAD='+

                                Format('%.2f Kb', [ (de.filesize - bytes_quedan)/ ( tim2-tim1 ) ] );
                end;



                application.processMessages;

                for i:=0 to 511 do
                   if (bytes_quedan>0) then
                        begin
                                Write( f, SECTOR_BUF[i] );
                                dec ( bytes_quedan );
                        end;

                // Siguiente sector
                inc ( numsec );
                if BPB_SECSPERCLUS = numsec then
                begin
                        // Sig. cluster:
                        numsec:=0;
                        SiguienteCluster( clus, clus );
                end;

        end;


        CloseFile( f);

end;

procedure TForm1.btnCalcEspacioClick(Sender: TObject);
var
        total   : Int64;

        num_fichs, num_dirs : Int64;

        procedure ExploraDirectorio( clus: DWORD );
        var
                i,n : Integer;
                de      : TDirectoryEntry;
        begin
                n:=FAT_Explorer( clus, true, 0, de );

                for i:=1 to n do
                begin
                        FAT_Explorer( clus, false, i, de );

                        if      ( 0<>(de.Attrib and $10)) and
                                (not (
                                      (de.FileName='.') or
                                      (de.FileName='..') )
                                     ) then
                                begin
                                        inc ( num_dirs );
                                        ExploraDirectorio( de.FirstCluster );
                                end;

                        // Ir sumando
                        total:=total+ de.FileSize;

                        if ( 0=(de.Attrib and $10) ) and
                           (not( (de.Attrib and $0F)= $0F) )
                          then inc ( num_fichs );


                end;


        end;

begin
        // Calcular espacio ocupado en el disco:
        // Recorrer todos los directorios y sumar tamaños de todos los
        //  ficheros:
        total:=0;
        num_fichs:=0;
        num_dirs:=0;

        lbEspa.caption:= '(Calculando)';
        Application.ProcessMessages;

        ExploraDirectorio( BPB_CLUS_DIR_RAIZ );

        lbEspa.caption:= inttostr( total shr 20  ) +
                ' Mb.FICHs='+
                inttostr(num_fichs)+
                '.DIRS='+
                inttostr(num_dirs);


end;


//************************************************************
//  Escribe un sector en el HD del reproductor MP3
//
//  Devuelve false si hay algun error.
//
//
//  Elimina el sector de la cache de lectura !!
//************************************************************
function TForm1.EscribeSector( LBA: LongInt; var buffer: array of byte): Boolean;
var
        i : Integer;
        EXOR : BYTE;
begin
        result:=false;

        // Esta en cache?
        for i:=1 to TAM_SECTOR_CACHE do
                if SECTORS_CACHE_INDEX[i]= LBA then
                begin
                        // Eliminar de la cache !!
                        SECTORS_CACHE_INDEX[i]:= $FFFFFFFF;
                        SECTORS_CACHE_COLA[i]:=0;
                end;


        // Enviar comando 0x20 para escribir sector:
        FT_Out_Buffer[0]:= $20;

        FT_Out_Buffer[1]:= LOBYTE( LOWORD( LBA ));
        FT_Out_Buffer[2]:= HIBYTE( LOWORD( LBA ));
        FT_Out_Buffer[3]:= LOBYTE( HIWORD( LBA ));
        FT_Out_Buffer[4]:= HIBYTE( HIWORD( LBA ));

        // + 512 bytes de datos
        EXOR:=0;
        for i:=0 to 511 do
        begin
                FT_Out_Buffer[5+i]:= buffer[i];
                EXOR:= EXOR xor buffer[i]; 
        end;

        // Mas 4 bytes, el 1º de XOR:
        FT_Out_Buffer[517]:= EXOR;
        FT_Out_Buffer[518]:= 0;
        FT_Out_Buffer[519]:= 0;
        FT_Out_Buffer[520]:= 0;

        // Total = 521 bytes

        i:= Write_USB_Device_Buffer( 1+4+4+512 );
        if (i<> 1+4+4+512 ) then
        begin
                ShowMessage('Error al enviar comando $20!');
                exit;
        end;

        // Recibir byte de confirmacion / error:
        i:=Read_USB_Device_Buffer( 1 );
        if (i<>1) then
        begin
                ShowMessage('Error al recibir respuesta a 0x20 ! '+inttostr(i));
                exit;
        end;

        // Un $10 es todo OK:
        if ($10 <> FT_In_Buffer[0] ) then
        begin
                ShowMessage( 'Error de XOR! '+ inttohex( FT_In_Buffer[0],2 ) );
                exit;
        end;
        

        result:=true;
end;


procedure TForm1.btnEscriClick(Sender: TObject);
var
        buf : array [0..511] of byte;
        i : Integer;
begin
        for i:=0 to 511 do
                buf[i]:= i ;

        EscribeSector ( Strtoint( edSectorNum.Text ), buf );

end;

end.

