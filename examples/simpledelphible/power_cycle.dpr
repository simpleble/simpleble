program power_cycle;

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
  Adapters: TArray<TBleAdapter>;
  Adapter: TBleAdapter;

begin
  Adapters := TBleAdapter.GetAdapters;
  if Length(Adapters) = 0 then
    WriteLn('No adapters found');

  for Adapter in Adapters do
  begin
    WriteLn('Adapter: ', Adapter.Identifier, ' [', Adapter.Address, ']');

    WriteLn('Powering off adapter');
    Adapter.PowerOff;
    WriteLn('Adapter powered: ', Adapter.IsPowered);
    Sleep(1000);

    WriteLn('Powering on adapter');
    Adapter.PowerOn;
    WriteLn('Adapter powered: ', Adapter.IsPowered);
  end;
end.
