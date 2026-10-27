program dongl;

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
  THandlers = class
    procedure ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
    procedure PasskeyRequest(Sender: TBlePeripheral; var Passkey: string; var Accept: Boolean);
    procedure PasskeyDisplay(Sender: TBlePeripheral; const Passkey: string);
    procedure NumericComparison(Sender: TBlePeripheral; const Passkey: string; var Accept: Boolean);
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

function ChooseDongl: TBleAdapter;
var
  Adapters: TArray<TBleAdapter>;
  Names: TArray<string>;
  I: Integer;
begin
  Adapters := TBleBackend.GetDongl.GetAdapters;
  if Length(Adapters) = 0 then
    raise Exception.Create('No Dongl found');
  SetLength(Names, Length(Adapters));
  for I := 0 to High(Adapters) do
    Names[I] := Adapters[I].Identifier + ' [' + Adapters[I].Address + ']';
  Result := Adapters[Choose('a Dongl', Names)];
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

procedure THandlers.ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
begin
  WriteLn('Found ', Describe(Peripheral));
end;

procedure THandlers.PasskeyRequest(Sender: TBlePeripheral; var Passkey: string; var Accept: Boolean);
begin
  Write('Enter the passkey shown on the device (empty to reject): ');
  ReadLn(Passkey);
  Accept := Passkey <> '';
end;

procedure THandlers.PasskeyDisplay(Sender: TBlePeripheral; const Passkey: string);
begin
  WriteLn('Enter ', Passkey, ' on the device.');
end;

procedure THandlers.NumericComparison(Sender: TBlePeripheral; const Passkey: string; var Accept: Boolean);
var
  Answer: string;
begin
  Write('Does the device show ', Passkey, '? [y/N] ');
  ReadLn(Answer);
  Accept := Answer = 'y';
end;

var
  Handlers: THandlers;
  Adapter: TBleAdapter;
  Peripheral: TBlePeripheral;
  Service: TBleService;
  Characteristic: TBleCharacteristic;

begin
  Handlers := THandlers.Create;
  Adapter := ChooseDongl;
  WriteLn('Selected Dongl: ', Adapter.Identifier, ' [', Adapter.Address, ']');

  Adapter.OnScanFound := Handlers.ScanFound;

  // Scan for 5 seconds
  Adapter.ScanFor(5000);
  Adapter.OnScanFound := nil;

  Peripheral := ChoosePeripheral(Adapter);

  // Pairing can start as soon as the link is up, so set the handlers before connecting. The Dongl pairs without the
  // operating system.
  Peripheral.OnPasskeyRequest := Handlers.PasskeyRequest;
  Peripheral.OnPasskeyDisplay := Handlers.PasskeyDisplay;
  Peripheral.OnNumericComparison := Handlers.NumericComparison;

  WriteLn('Connecting to: ', Describe(Peripheral));
  Peripheral.Connect;

  WriteLn('Successfully connected, listing services...');
  for Service in Peripheral.Services do
  begin
    WriteLn('Service: ', Service.Uuid);
    for Characteristic in Service.Characteristics do
      WriteLn('    Characteristic: ', Characteristic.Uuid);
  end;
  WriteLn('Paired: ', Peripheral.IsPaired);

  Peripheral.Disconnect;
end.
