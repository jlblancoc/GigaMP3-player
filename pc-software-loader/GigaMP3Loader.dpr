program GigaMP3Loader;

uses
  Forms,
  MainUnit in 'MainUnit.pas' {Form1},
  D2XXUnit in 'D2XXUnit.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
