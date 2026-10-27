program notify;

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
  TPrinter = class
    procedure ScanStart(Sender: TBleAdapter);
    procedure ScanStop(Sender: TBleAdapter);
    procedure ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
    procedure Notification(Sender: TBlePeripheral; const Service, Characteristic: string; const Data: TBytes);
  end;

function Describe(Peripheral: TBlePeripheral): string;
begin
  Result := Peripheral.Identifier + ' [' + Peripheral.Address + ']';
end;

function Hex(const Data: TBytes): string;
var
  B: Byte;
begin
  Result := '';
  for B in Data do
    Result := Result + LowerCase(IntToHex(B, 2));
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

procedure ChooseCharacteristic(Peripheral: TBlePeripheral; out ServiceUuid, CharacteristicUuid: string);
var
  Services: TArray<string>;
  Characteristics: TArray<string>;
  Names: TArray<string>;
  Service: TBleService;
  Characteristic: TBleCharacteristic;
  Choice: Integer;
begin
  for Service in Peripheral.Services do
    for Characteristic in Service.Characteristics do
    begin
      Services := Services + [Service.Uuid];
      Characteristics := Characteristics + [Characteristic.Uuid];
      Names := Names + [Service.Uuid + ' ' + Characteristic.Uuid];
    end;
  if Length(Names) = 0 then
    raise Exception.Create('No characteristics found');
  Choice := Choose('a service/characteristic pair', Names);
  ServiceUuid := Services[Choice];
  CharacteristicUuid := Characteristics[Choice];
end;

procedure TPrinter.ScanStart(Sender: TBleAdapter);
begin
  WriteLn('Scan started.');
end;

procedure TPrinter.ScanStop(Sender: TBleAdapter);
begin
  WriteLn('Scan complete.');
end;

procedure TPrinter.ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
begin
  WriteLn('Found ', Describe(Peripheral));
end;

procedure TPrinter.Notification(Sender: TBlePeripheral; const Service, Characteristic: string; const Data: TBytes);
begin
  WriteLn('Notification: ', Hex(Data));
end;

var
  Printer: TPrinter;
  Adapter: TBleAdapter;
  Peripheral: TBlePeripheral;
  ServiceUuid: string;
  CharacteristicUuid: string;

begin
  Adapter := ChooseAdapter;
  WriteLn('Selected adapter: ', Adapter.Identifier, ' [', Adapter.Address, ']');

  Printer := TPrinter.Create;
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
  ChooseCharacteristic(Peripheral, ServiceUuid, CharacteristicUuid);

  Peripheral.Notify(ServiceUuid, CharacteristicUuid, Printer.Notification);
  Sleep(5000);

  Peripheral.Unsubscribe(ServiceUuid, CharacteristicUuid);
  Sleep(1000);

  Peripheral.Disconnect;
end.
