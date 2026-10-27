program connect;

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

type
  TScanPrinter = class
    procedure ScanStart(Sender: TBleAdapter);
    procedure ScanStop(Sender: TBleAdapter);
    procedure ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
  end;

function Describe(Peripheral: TBlePeripheral): string;
begin
  Result := Peripheral.Identifier + ' [' + Peripheral.Address + ']';
end;

// Lists the items and returns the index the user picks.
function Choose(const Title: string; const Items: array of string): Integer;
var
  I: Integer;
  Line: string;
begin
  WriteLn('Please select ', Title, ':');
  for I := 0 to High(Items) do
    WriteLn(I, ': ', Items[I]);
  Write('Enter choice: ');
  ReadLn(Line);
  Result := StrToIntDef(Line, -1);
  if (Result < 0) or (Result > High(Items)) then
    raise Exception.Create('Invalid choice');
end;

function ChooseAdapter: TBleAdapter;
var
  Adapters: TArray<TBleAdapter>;
  Names: TArray<string>;
  I: Integer;
begin
  Adapters := TBleAdapter.GetAdapters;
  if Length(Adapters) = 0 then
    raise Exception.Create('No adapters found');
  SetLength(Names, Length(Adapters));
  for I := 0 to High(Adapters) do
    Names[I] := Adapters[I].Identifier + ' [' + Adapters[I].Address + ']';
  Result := Adapters[Choose('an adapter', Names)];
end;

function ChoosePeripheral(Adapter: TBleAdapter): TBlePeripheral;
var
  Peripherals: TArray<TBlePeripheral>;
  Names: TArray<string>;
  I: Integer;
begin
  Peripherals := Adapter.ScanGetResults;
  if Length(Peripherals) = 0 then
    raise Exception.Create('No peripherals found');
  SetLength(Names, Length(Peripherals));
  for I := 0 to High(Peripherals) do
    Names[I] := Describe(Peripherals[I]);
  Result := Peripherals[Choose('a peripheral', Names)];
end;

function Capabilities(const Characteristic: TBleCharacteristic): string;
begin
  Result := '';
  if Characteristic.CanRead then
    Result := Result + ' read';
  if Characteristic.CanWriteRequest then
    Result := Result + ' write_request';
  if Characteristic.CanWriteCommand then
    Result := Result + ' write_command';
  if Characteristic.CanNotify then
    Result := Result + ' notify';
  if Characteristic.CanIndicate then
    Result := Result + ' indicate';
  Result := Trim(Result);
end;

procedure TScanPrinter.ScanStart(Sender: TBleAdapter);
begin
  WriteLn('Scan started.');
end;

procedure TScanPrinter.ScanStop(Sender: TBleAdapter);
begin
  WriteLn('Scan complete.');
end;

procedure TScanPrinter.ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
begin
  WriteLn('Found ', Describe(Peripheral));
end;

var
  Printer: TScanPrinter;
  Adapter: TBleAdapter;
  Peripheral: TBlePeripheral;
  Service: TBleService;
  Characteristic: TBleCharacteristic;

begin
  Adapter := ChooseAdapter;
  WriteLn('Selected adapter: ', Adapter.Identifier, ' [', Adapter.Address, ']');

  Printer := TScanPrinter.Create;
  Adapter.OnScanStart := Printer.ScanStart;
  Adapter.OnScanStop := Printer.ScanStop;
  Adapter.OnScanFound := Printer.ScanFound;

  // Scan for 5 seconds
  Adapter.ScanFor(5000);
  Adapter.OnScanFound := nil;

  Peripheral := ChoosePeripheral(Adapter);

  WriteLn('Connecting to: ', Describe(Peripheral));
  Peripheral.Connect;

  WriteLn('Successfully connected, listing services...');
  for Service in Peripheral.Services do
  begin
    WriteLn('Service: ', Service.Uuid);
    for Characteristic in Service.Characteristics do
    begin
      WriteLn('    Characteristic: ', Characteristic.Uuid);
      WriteLn('    Capabilities: ', Capabilities(Characteristic));
    end;
  end;

  Peripheral.Disconnect;
end.
