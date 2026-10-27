program scan;

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

const
  AddressTypes: array[TBleAddressType] of string = ('Public', 'Random', 'Unspecified');

var
  Printer: TScanPrinter;
  Adapter: TBleAdapter;
  Peripheral: TBlePeripheral;
  Connectable: string;
  Data: TBleManufacturerData;
  Service: TBleService;

begin
  Adapter := ChooseAdapter;
  WriteLn('Selected adapter: ', Adapter.Identifier, ' [', Adapter.Address, ']');

  Printer := TScanPrinter.Create;
  Adapter.OnScanStart := Printer.ScanStart;
  Adapter.OnScanStop := Printer.ScanStop;
  Adapter.OnScanFound := Printer.ScanFound;

  // Scan for 5 seconds
  Adapter.ScanFor(5000);

  WriteLn('The following peripherals were found:');
  for Peripheral in Adapter.ScanGetResults do
  begin
    if Peripheral.IsConnectable then
      Connectable := 'Connectable'
    else
      Connectable := 'Non-Connectable';
    WriteLn(Describe(Peripheral), ' - ', Connectable);
    WriteLn('    Address Type: ', AddressTypes[Peripheral.AddressType]);
    WriteLn('    Tx Power: ', Peripheral.TxPower, ' dBm');

    for Data in Peripheral.ManufacturerData do
    begin
      WriteLn('    Manufacturer ID: ', Data.CompanyId);
      WriteLn('    Manufacturer data: ', Hex(Data.Data));
    end;

    for Service in Peripheral.Services do
    begin
      WriteLn('    Service UUID: ', Service.Uuid);
      WriteLn('    Service data: ', Hex(Service.Data));
    end;
  end;
end.
