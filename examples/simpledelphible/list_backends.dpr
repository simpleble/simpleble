program list_backends;

{$IFDEF FPC}
  {$MODE DELPHIUNICODE}
{$ENDIF}
{$IFDEF MSWINDOWS}
  {$APPTYPE CONSOLE}
{$ENDIF}

uses
  {$IFDEF UNIX}
  cthreads,
  {$ENDIF}
  SysUtils,
  SimpleDelphiBLE;

var
  Backend: TBleBackend;
  Adapter: TBleAdapter;

begin
  for Backend in TBleBackend.GetBackends do
  begin
    WriteLn('Backend: ', Backend.Identifier);
    for Adapter in Backend.GetAdapters do
      WriteLn('    Adapter: ', Adapter.Identifier, ' [', Adapter.Address, ']');
  end;
end.
