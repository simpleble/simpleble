program list_adapters;

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
  WriteLn('Bluetooth enabled: ', TBleAdapter.IsBluetoothEnabled);

  Adapters := TBleAdapter.GetAdapters;
  if Length(Adapters) = 0 then
    WriteLn('No adapters found');

  for Adapter in Adapters do
    WriteLn('Adapter: ', Adapter.Identifier, ' [', Adapter.Address, ']');
end.
