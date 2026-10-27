program peripheral;

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

const
  ServiceUuid = '12345678-1234-5678-1234-56789abcdef0';
  CharacteristicUuid = '12345678-1234-5678-1234-56789abcdef1';

type
  THandlers = class
    procedure Written(Sender: TBleLocalCharacteristic; const Data: TBytes);
    procedure Subscribed(Sender: TBleLocalCharacteristic);
    procedure Unsubscribed(Sender: TBleLocalCharacteristic);
    procedure ClientConnected(Sender: TBleLocalPeripheral; const Address: string);
    procedure ClientDisconnected(Sender: TBleLocalPeripheral; const Address: string);
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

procedure THandlers.Written(Sender: TBleLocalCharacteristic; const Data: TBytes);
begin
  WriteLn('Write: ', Hex(Data));
  // Echo the written value to subscribed clients.
  Sender.SetValue(Data);
end;

procedure THandlers.Subscribed(Sender: TBleLocalCharacteristic);
begin
  WriteLn('Client subscribed.');
end;

procedure THandlers.Unsubscribed(Sender: TBleLocalCharacteristic);
begin
  WriteLn('Client unsubscribed.');
end;

procedure THandlers.ClientConnected(Sender: TBleLocalPeripheral; const Address: string);
begin
  WriteLn('Client connected: ', Address);
end;

procedure THandlers.ClientDisconnected(Sender: TBleLocalPeripheral; const Address: string);
begin
  WriteLn('Client disconnected: ', Address);
end;

var
  Handlers: THandlers;
  Adapter: TBleAdapter;
  Host: TBleLocalPeripheral;
  Characteristic: TBleLocalCharacteristic;

begin
  Adapter := ChooseAdapter;
  Handlers := THandlers.Create;

  Host := Adapter.CreateLocalPeripheral;
  try
    // Add a service UUID to the advertisement.
    Host.AddAdvertisedService(ServiceUuid);

    Characteristic := Host.AddService(ServiceUuid).AddCharacteristic(CharacteristicUuid,
      [lcRead, lcWriteRequest, lcWriteCommand, lcNotify, lcIndicate]);
    Characteristic.SetValue(TEncoding.UTF8.GetBytes('ready'));
    Characteristic.OnWrite := Handlers.Written;
    Characteristic.OnSubscribed := Handlers.Subscribed;
    Characteristic.OnUnsubscribed := Handlers.Unsubscribed;

    Host.OnClientConnected := Handlers.ClientConnected;
    Host.OnClientDisconnected := Handlers.ClientDisconnected;

    Host.Start;
    WriteLn('Advertising. Press Enter to stop.');
    ReadLn;
    Host.Stop;
  finally
    Host.Free;
  end;
end.
