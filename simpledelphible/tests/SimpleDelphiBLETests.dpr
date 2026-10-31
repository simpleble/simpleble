// Tests of the binding layer against the PLAIN backend's fixture (docs/extras/plain-backend.md).
program SimpleDelphiBLETests;

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
  Classes,
  SimpleDelphiBLE;

const
  BatteryService = '0000180f-0000-1000-8000-00805f9b34fb';
  BatteryLevel = '00002a19-0000-1000-8000-00805f9b34fb';
  TestService = '0000fff0-0000-1000-8000-00805f9b34fb';
  BinaryValue = '0000fff1-0000-1000-8000-00805f9b34fb';
  ErrorValue = '0000fff2-0000-1000-8000-00805f9b34fb';
  UserDescription = '00002901-0000-1000-8000-00805f9b34fb';
  ClientConfiguration = '00002902-0000-1000-8000-00805f9b34fb';
  MissingUuid = '0000abcd-0000-1000-8000-00805f9b34fb';
  HostService = '00001234-0000-1000-8000-00805f9b34fb';
  HostValue = '00005678-0000-1000-8000-00805f9b34fb';
  HostOther = '00005679-0000-1000-8000-00805f9b34fb';

type
  // Records what event handlers saw. The handlers run on the event thread or the main thread.
  TRecorder = class
  public
    PoweredOn: Boolean;
    PoweredOff: Boolean;
    ScanStarted: Boolean;
    ScanStopped: Boolean;
    Found: Boolean;
    FoundPeripheral: TBlePeripheral;
    Connected: Boolean;
    Disconnected: Boolean;
    ValueReceived: Boolean;
    ValueService: string;
    ValueCharacteristic: string;
    ValueData: TBytes;
    ClientConnected: Boolean;
    ClientDisconnected: Boolean;
    ClientAddress: string;
    LocalWritten: Boolean;
    LocalData: TBytes;
    Subscribed: Boolean;
    Unsubscribed: Boolean;
    Logged: Boolean;
    LogEntry: TBleLogEntry;
    HandlerThreads: array of TThreadID;
    HandlerFailed: Boolean;
    HandlerError: string;
    Done: Boolean;
    DoneSender: TObject;
    DoneData: TBytes;
    DoneError: string;
    procedure Reset;
    procedure PowerOn(Sender: TBleAdapter);
    procedure PowerOff(Sender: TBleAdapter);
    procedure ScanStart(Sender: TBleAdapter);
    procedure ScanStop(Sender: TBleAdapter);
    procedure ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
    procedure PeripheralConnected(Sender: TBlePeripheral);
    procedure PeripheralDisconnected(Sender: TBlePeripheral);
    procedure Value(Sender: TBlePeripheral; const Service, Characteristic: string; const Data: TBytes);
    procedure PasskeyRequest(Sender: TBlePeripheral; var Passkey: string; var Accept: Boolean);
    procedure PasskeyDisplay(Sender: TBlePeripheral; const Passkey: string);
    procedure NumericComparison(Sender: TBlePeripheral; const Passkey: string; var Accept: Boolean);
    procedure AdapterPasskeyRequest(Sender: TBleAdapter; const Peer: string; var Passkey: string;
      var Accept: Boolean);
    procedure AdapterPasskeyDisplay(Sender: TBleAdapter; const Peer, Passkey: string);
    procedure AdapterNumericComparison(Sender: TBleAdapter; const Peer, Passkey: string; var Accept: Boolean);
    procedure PairingAuthorization(Sender: TBleAdapter; const Peer: string; var Accept: Boolean);
    procedure ClientConnect(Sender: TBleLocalPeripheral; const Address: string);
    procedure ClientDisconnect(Sender: TBleLocalPeripheral; const Address: string);
    procedure LocalWrite(Sender: TBleLocalCharacteristic; const Data: TBytes);
    procedure LocalRead(Sender: TBleLocalCharacteristic; var Data: TBytes);
    procedure LocalSubscribe(Sender: TBleLocalCharacteristic);
    procedure LocalUnsubscribe(Sender: TBleLocalCharacteristic);
    procedure Log(const Entry: TBleLogEntry);
    procedure RaiseOnPowerOff(Sender: TBleAdapter);
    procedure HandlerException(E: Exception);
    procedure WakeMainThread(Sender: TObject);
    procedure Completed(Sender: TObject; Error: Exception);
    procedure ReadCompleted(Sender: TBlePeripheral; const Data: TBytes; Error: Exception);
  end;

var
  Recorder: TRecorder;

procedure TRecorder.Reset;
begin
  PoweredOn := False;
  PoweredOff := False;
  ScanStarted := False;
  ScanStopped := False;
  Found := False;
  FoundPeripheral := nil;
  Connected := False;
  Disconnected := False;
  ValueReceived := False;
  ValueService := '';
  ValueCharacteristic := '';
  ValueData := nil;
  ClientConnected := False;
  ClientDisconnected := False;
  ClientAddress := '';
  LocalWritten := False;
  LocalData := nil;
  Subscribed := False;
  Unsubscribed := False;
  Logged := False;
  HandlerThreads := nil;
  HandlerFailed := False;
  HandlerError := '';
  Done := False;
  DoneSender := nil;
  DoneData := nil;
  DoneError := '';
end;

procedure TRecorder.PowerOn(Sender: TBleAdapter);
begin
  SetLength(HandlerThreads, Length(HandlerThreads) + 1);
  HandlerThreads[High(HandlerThreads)] := TThread.CurrentThread.ThreadID;
  PoweredOn := True;
end;

procedure TRecorder.PowerOff(Sender: TBleAdapter);
begin
  SetLength(HandlerThreads, Length(HandlerThreads) + 1);
  HandlerThreads[High(HandlerThreads)] := TThread.CurrentThread.ThreadID;
  PoweredOff := True;
end;

procedure TRecorder.ScanStart(Sender: TBleAdapter);
begin
  ScanStarted := True;
end;

procedure TRecorder.ScanStop(Sender: TBleAdapter);
begin
  ScanStopped := True;
end;

procedure TRecorder.ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
begin
  if Peripheral.Identifier = 'Plain Peripheral' then
  begin
    FoundPeripheral := Peripheral;
    Found := True;
  end;
end;

procedure TRecorder.PeripheralConnected(Sender: TBlePeripheral);
begin
  Connected := True;
end;

procedure TRecorder.PeripheralDisconnected(Sender: TBlePeripheral);
begin
  Disconnected := True;
end;

procedure TRecorder.Value(Sender: TBlePeripheral; const Service, Characteristic: string; const Data: TBytes);
begin
  if ValueReceived then
    Exit;
  ValueService := Service;
  ValueCharacteristic := Characteristic;
  ValueData := Data;
  ValueReceived := True;
end;

procedure TRecorder.PasskeyRequest(Sender: TBlePeripheral; var Passkey: string; var Accept: Boolean);
begin
end;

procedure TRecorder.PasskeyDisplay(Sender: TBlePeripheral; const Passkey: string);
begin
end;

procedure TRecorder.NumericComparison(Sender: TBlePeripheral; const Passkey: string; var Accept: Boolean);
begin
end;

procedure TRecorder.AdapterPasskeyRequest(Sender: TBleAdapter; const Peer: string; var Passkey: string;
  var Accept: Boolean);
begin
end;

procedure TRecorder.AdapterPasskeyDisplay(Sender: TBleAdapter; const Peer, Passkey: string);
begin
end;

procedure TRecorder.AdapterNumericComparison(Sender: TBleAdapter; const Peer, Passkey: string;
  var Accept: Boolean);
begin
end;

procedure TRecorder.PairingAuthorization(Sender: TBleAdapter; const Peer: string; var Accept: Boolean);
begin
end;

procedure TRecorder.ClientConnect(Sender: TBleLocalPeripheral; const Address: string);
begin
  ClientAddress := Address;
  ClientConnected := True;
end;

procedure TRecorder.ClientDisconnect(Sender: TBleLocalPeripheral; const Address: string);
begin
  ClientDisconnected := True;
end;

procedure TRecorder.LocalWrite(Sender: TBleLocalCharacteristic; const Data: TBytes);
begin
  LocalData := Data;
  LocalWritten := True;
end;

procedure TRecorder.LocalRead(Sender: TBleLocalCharacteristic; var Data: TBytes);
begin
  Data := TBytes.Create(1, 2, 3);
end;

procedure TRecorder.LocalSubscribe(Sender: TBleLocalCharacteristic);
begin
  Subscribed := True;
end;

procedure TRecorder.LocalUnsubscribe(Sender: TBleLocalCharacteristic);
begin
  Unsubscribed := True;
end;

procedure TRecorder.Log(const Entry: TBleLogEntry);
begin
  if Logged then
    Exit;
  LogEntry := Entry;
  Logged := True;
end;

procedure TRecorder.RaiseOnPowerOff(Sender: TBleAdapter);
begin
  raise Exception.Create('raised by a handler');
end;

procedure TRecorder.HandlerException(E: Exception);
begin
  HandlerError := E.Message;
  HandlerFailed := True;
end;

procedure TRecorder.WakeMainThread(Sender: TObject);
begin
end;

procedure TRecorder.Completed(Sender: TObject; Error: Exception);
begin
  DoneSender := Sender;
  if Error <> nil then
    DoneError := Error.ClassName;
  Done := True;
end;

procedure TRecorder.ReadCompleted(Sender: TBlePeripheral; const Data: TBytes; Error: Exception);
begin
  DoneData := Data;
  Completed(Sender, Error);
end;

procedure Fail(const Description: string);
begin
  raise Exception.Create(Description);
end;

procedure Check(Condition: Boolean; const Description: string);
begin
  if not Condition then
    Fail(Description);
end;

procedure CheckEquals(Expected, Actual: Int64; const Description: string); overload;
begin
  if Expected <> Actual then
    Fail(Format('%s: expected %d, got %d', [Description, Expected, Actual]));
end;

procedure CheckEquals(const Expected, Actual: string; const Description: string); overload;
begin
  if Expected <> Actual then
    Fail(Format('%s: expected "%s", got "%s"', [Description, Expected, Actual]));
end;

function HexBytes(const Data: TBytes): string;
var
  B: Byte;
begin
  Result := '';
  for B in Data do
    Result := Result + IntToHex(B, 2);
end;

procedure CheckBytes(const Expected, Actual: TBytes; const Description: string);
begin
  CheckEquals(HexBytes(Expected), HexBytes(Actual), Description);
end;

// Waits for another thread or a queued event handler to set Flag, running the main thread's queue meanwhile.
procedure WaitFor(var Flag: Boolean; const Description: string; TimeoutMs: Integer = 5000);
var
  Deadline: TDateTime;
begin
  Deadline := Now + TimeoutMs / MSecsPerDay;
  while not Flag do
  begin
    if Now > Deadline then
      Fail('Timed out waiting for ' + Description);
    CheckSynchronize(10);
  end;
end;

function Bytes(const Values: array of Byte): TBytes;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, Length(Values));
  for I := 0 to High(Values) do
    Result[I] := Values[I];
end;

function PlainAdapter: TBleAdapter;
var
  Adapters: TArray<TBleAdapter>;
begin
  Adapters := TBleAdapter.GetAdapters;
  CheckEquals(1, Length(Adapters), 'adapter count');
  Result := Adapters[0];
end;

function FindPeripheral(const Identifier: string): TBlePeripheral;
var
  Adapter: TBleAdapter;
  Peripheral: TBlePeripheral;
begin
  Adapter := PlainAdapter;
  Adapter.ScanFor(500);
  for Peripheral in Adapter.ScanGetResults do
    if Peripheral.Identifier = Identifier then
      Exit(Peripheral);
  Fail(Identifier + ' not found');
  Result := nil;
end;

function PlainPeripheral: TBlePeripheral;
begin
  Result := FindPeripheral('Plain Peripheral');
end;

function Contains(const Peripherals: TArray<TBlePeripheral>; Peripheral: TBlePeripheral): Boolean;
var
  Item: TBlePeripheral;
begin
  Result := False;
  for Item in Peripherals do
    Result := Result or (Item = Peripheral);
end;

function FindService(const Services: TArray<TBleService>; const Uuid: string; out Service: TBleService): Boolean;
var
  Item: TBleService;
begin
  for Item in Services do
    if Item.Uuid = Uuid then
    begin
      Service := Item;
      Exit(True);
    end;
  Result := False;
end;

// Powers the adapter off and on. Recorder.HandlerThreads has the threads the two handlers ran on.
procedure CyclePower(Adapter: TBleAdapter);
begin
  Recorder.Reset;
  Adapter.OnPowerOn := Recorder.PowerOn;
  Adapter.OnPowerOff := Recorder.PowerOff;
  try
    Adapter.PowerOff;
    WaitFor(Recorder.PoweredOff, 'OnPowerOff');
    Adapter.PowerOn;
    WaitFor(Recorder.PoweredOn, 'OnPowerOn');
  finally
    Adapter.OnPowerOn := nil;
    Adapter.OnPowerOff := nil;
    if not Adapter.IsPowered then
      Adapter.PowerOn;
  end;
end;

{ Loading }

// Runs first, before any other call has loaded the library.
procedure TestLibraryLoading;
var
  Folder: string;
begin
  Folder := ExtractFilePath(ParamStr(0));
  TSimpleDelphiBLE.LibraryPath := Folder + 'missing';
  Check(not TSimpleDelphiBLE.IsAvailable, 'available from a file that doesn''t exist');
  TSimpleDelphiBLE.LibraryPath := Folder;
  Check(TSimpleDelphiBLE.IsAvailable, 'not available from the program''s folder after a failed load');
  Check(TSimpleDelphiBLE.Version <> '', 'no version from the loaded library');
end;

{ Scanning }

procedure TestVersionAndOperatingSystem;
begin
  Check(TSimpleDelphiBLE.Version <> '', 'version is empty');
{$IF DEFINED(MSWINDOWS)}
  CheckEquals(Ord(osWindows), Ord(TSimpleDelphiBLE.OperatingSystem), 'operating system');
{$ELSEIF DEFINED(DARWIN) OR DEFINED(MACOS)}
  CheckEquals(Ord(osMacOS), Ord(TSimpleDelphiBLE.OperatingSystem), 'operating system');
{$ELSE}
  CheckEquals(Ord(osLinux), Ord(TSimpleDelphiBLE.OperatingSystem), 'operating system');
{$ENDIF}
end;

procedure TestAdapters;
var
  Adapter: TBleAdapter;
begin
  Check(TBleAdapter.IsBluetoothEnabled, 'Bluetooth is disabled');
  Adapter := PlainAdapter;
  CheckEquals('Plain Adapter', Adapter.Identifier, 'identifier');
  CheckEquals('AA:BB:CC:DD:EE:FF', Adapter.Address, 'address');
  Check(PlainAdapter = Adapter, 'GetAdapters returned another object for the same adapter');
end;

procedure TestPower;
var
  Adapter: TBleAdapter;
begin
  Adapter := PlainAdapter;
  Recorder.Reset;
  Adapter.OnPowerOn := Recorder.PowerOn;
  Adapter.OnPowerOff := Recorder.PowerOff;
  try
    Adapter.PowerOff;
    WaitFor(Recorder.PoweredOff, 'OnPowerOff');
    Check(not Adapter.IsPowered, 'still powered after PowerOff');
    Check(not Recorder.PoweredOn, 'OnPowerOn ran for PowerOff');
    Adapter.PowerOn;
    WaitFor(Recorder.PoweredOn, 'OnPowerOn');
    Check(Adapter.IsPowered, 'not powered after PowerOn');
  finally
    Adapter.OnPowerOn := nil;
    Adapter.OnPowerOff := nil;
    if not Adapter.IsPowered then
      Adapter.PowerOn;
  end;
end;

procedure TestScanEvents;
var
  Adapter: TBleAdapter;
begin
  Adapter := PlainAdapter;
  Recorder.Reset;
  Adapter.OnScanStart := Recorder.ScanStart;
  Adapter.OnScanStop := Recorder.ScanStop;
  Adapter.OnScanFound := Recorder.ScanFound;
  try
    Adapter.ScanStart;
    WaitFor(Recorder.ScanStarted, 'OnScanStart');
    Check(Adapter.ScanIsActive, 'scan inactive after ScanStart');
    WaitFor(Recorder.Found, 'OnScanFound for Plain Peripheral');
    Adapter.ScanStop;
    WaitFor(Recorder.ScanStopped, 'OnScanStop');
    Check(not Adapter.ScanIsActive, 'scan active after ScanStop');
  finally
    Adapter.OnScanStart := nil;
    Adapter.OnScanStop := nil;
    Adapter.OnScanFound := nil;
  end;
  Check(Contains(Adapter.ScanGetResults, Recorder.FoundPeripheral),
    'ScanGetResults lacks the object OnScanFound received');
end;

procedure TestPeripheralProperties;
var
  Peripheral: TBlePeripheral;
  Data: TArray<TBleManufacturerData>;
  Service: TBleService;
begin
  Peripheral := PlainPeripheral;
  Check(PlainPeripheral = Peripheral, 'another object for the same peripheral');
  CheckEquals('11:22:33:44:55:66', Peripheral.Address, 'address');
  CheckEquals(-60, Peripheral.Rssi, 'RSSI');
  CheckEquals(5, Peripheral.TxPower, 'TX power');
  Check(Peripheral.IsConnectable, 'not connectable');
  Check(not Peripheral.IsConnected, 'connected');
  Data := Peripheral.ManufacturerData;
  CheckEquals(1, Length(Data), 'manufacturer data entries');
  CheckEquals($004C, Data[0].CompanyId, 'company identifier');
  CheckBytes(TEncoding.ASCII.GetBytes('test'), Data[0].Data, 'manufacturer data');
  Check(FindService(Peripheral.Services, TestService, Service), 'test service not advertised');
  CheckBytes(Bytes([$00, $7F, $80, $FF]), Service.Data, 'service data');
end;

procedure TestErrorMapping;
begin
  try
    PlainAdapter.ScanFor(-1);
    Fail('ScanFor(-1) raised nothing');
  except
    on E: EBleInvalidArgument do
    begin
      CheckEquals(Ord(ecInvalidArgument), Ord(E.Code), 'error code');
      Check(E.Message <> '', 'error message is empty');
    end;
  end;
end;

procedure TestEventThreads;
var
  Adapter: TBleAdapter;
begin
  Adapter := PlainAdapter;
  try
    TSimpleDelphiBLE.EventDelivery := edEventThread;
    CyclePower(Adapter);
    Check(Recorder.HandlerThreads[0] <> MainThreadID, 'edEventThread delivered on the main thread');
    Check(Recorder.HandlerThreads[0] = Recorder.HandlerThreads[1], 'edEventThread used two threads');

    TSimpleDelphiBLE.EventDelivery := edMainThread;
    CyclePower(Adapter);
    Check(Recorder.HandlerThreads[0] = MainThreadID, 'edMainThread delivered on another thread');
    Check(Recorder.HandlerThreads[1] = MainThreadID, 'edMainThread delivered on another thread');

    TSimpleDelphiBLE.EventDelivery := edAuto;
    CyclePower(Adapter);
    Check(Recorder.HandlerThreads[0] <> MainThreadID, 'edAuto used the main thread without a main loop');

    WakeMainThread := Recorder.WakeMainThread;
    CyclePower(Adapter);
    Check(Recorder.HandlerThreads[0] = MainThreadID, 'edAuto ignored the main loop');
  finally
    WakeMainThread := nil;
    TSimpleDelphiBLE.EventDelivery := edAuto;
  end;
end;

procedure TestHandlerException;
var
  Adapter: TBleAdapter;
begin
  Adapter := PlainAdapter;
  Recorder.Reset;
  TSimpleDelphiBLE.OnHandlerException := Recorder.HandlerException;
  Adapter.OnPowerOff := Recorder.RaiseOnPowerOff;
  Adapter.OnPowerOn := Recorder.PowerOn;
  try
    Adapter.PowerOff;
    WaitFor(Recorder.HandlerFailed, 'OnHandlerException');
    CheckEquals('raised by a handler', Recorder.HandlerError, 'exception message');
    Adapter.PowerOn;
    WaitFor(Recorder.PoweredOn, 'OnPowerOn after a handler raised');
  finally
    Adapter.OnPowerOff := nil;
    Adapter.OnPowerOn := nil;
    TSimpleDelphiBLE.OnHandlerException := nil;
    if not Adapter.IsPowered then
      Adapter.PowerOn;
  end;
end;

{ GATT }

procedure TestConnection;
var
  Adapter: TBleAdapter;
  Peripheral: TBlePeripheral;
begin
  Adapter := PlainAdapter;
  Peripheral := PlainPeripheral;
  Recorder.Reset;
  Peripheral.OnConnected := Recorder.PeripheralConnected;
  Peripheral.OnDisconnected := Recorder.PeripheralDisconnected;
  try
    Peripheral.Connect;
    WaitFor(Recorder.Connected, 'OnConnected');
    Check(Peripheral.IsConnected, 'not connected after Connect');
    CheckEquals(244, Peripheral.Mtu, 'MTU');
    Check(Peripheral.IsPaired, 'not paired after Connect');
    Check(Contains(Adapter.GetConnectedPeripherals, Peripheral), 'GetConnectedPeripherals lacks the object');
    Check(Contains(Adapter.GetPairedPeripherals, Peripheral), 'GetPairedPeripherals lacks the object');
    Peripheral.Disconnect;
    WaitFor(Recorder.Disconnected, 'OnDisconnected');
    Check(not Peripheral.IsConnected, 'connected after Disconnect');
    Peripheral.Unpair;
    Check(not Peripheral.IsPaired, 'paired after Unpair');
  finally
    Peripheral.OnConnected := nil;
    Peripheral.OnDisconnected := nil;
    if Peripheral.IsConnected then
      Peripheral.Disconnect;
  end;
end;

procedure TestServices;
var
  Peripheral: TBlePeripheral;
  Service: TBleService;
  Characteristic: TBleCharacteristic;
  Battery: TBleCharacteristic;
  Binary: TBleCharacteristic;
begin
  Peripheral := PlainPeripheral;
  Peripheral.Connect;
  try
    Check(FindService(Peripheral.Services, BatteryService, Service), 'battery service missing');
    CheckEquals(1, Length(Service.Characteristics), 'battery characteristics');
    Battery := Service.Characteristics[0];
    CheckEquals(BatteryLevel, Battery.Uuid, 'battery level UUID');
    Check(Battery.CanRead and Battery.CanNotify, 'battery level lacks read or notify');
    Check(not (Battery.CanWriteRequest or Battery.CanWriteCommand or Battery.CanIndicate),
      'battery level has write or indicate');
    Check(FindService(Peripheral.Services, TestService, Service), 'test service missing');
    for Characteristic in Service.Characteristics do
      if Characteristic.Uuid = BinaryValue then
        Binary := Characteristic;
    CheckEquals(BinaryValue, Binary.Uuid, 'binary value UUID');
    Check(Binary.CanRead and Binary.CanWriteRequest and Binary.CanWriteCommand and Binary.CanNotify and
      Binary.CanIndicate, 'binary value lacks a capability');
    CheckEquals(2, Length(Binary.Descriptors), 'binary value descriptors');
    Check((Binary.Descriptors[0] = UserDescription) or (Binary.Descriptors[1] = UserDescription),
      'user description descriptor missing');
    Check((Binary.Descriptors[0] = ClientConfiguration) or (Binary.Descriptors[1] = ClientConfiguration),
      'client configuration descriptor missing');
  finally
    Peripheral.Disconnect;
  end;
end;

procedure TestReadWrite;
var
  Peripheral: TBlePeripheral;
begin
  Peripheral := PlainPeripheral;
  Peripheral.Connect;
  try
    CheckBytes(Bytes([100]), Peripheral.Read(BatteryService, BatteryLevel), 'battery level');
    CheckBytes(Bytes([$00, $7F, $80, $FF]), Peripheral.Read(TestService, BinaryValue), 'initial value');
    Peripheral.WriteRequest(TestService, BinaryValue, Bytes([1, 2, 3]));
    CheckBytes(Bytes([1, 2, 3]), Peripheral.Read(TestService, BinaryValue), 'value after WriteRequest');
    Peripheral.WriteCommand(TestService, BinaryValue, Bytes([4]));
    CheckBytes(Bytes([4]), Peripheral.Read(TestService, BinaryValue), 'value after WriteCommand');
    Peripheral.WriteRequest(TestService, BinaryValue, nil);
    CheckEquals(0, Length(Peripheral.Read(TestService, BinaryValue)), 'length after an empty write');
  finally
    Peripheral.WriteRequest(TestService, BinaryValue, Bytes([$00, $7F, $80, $FF]));
    Peripheral.Disconnect;
  end;
end;

procedure TestDescriptors;
var
  Peripheral: TBlePeripheral;
begin
  Peripheral := PlainPeripheral;
  Peripheral.Connect;
  try
    CheckBytes(TEncoding.ASCII.GetBytes('Plain value'),
      Peripheral.ReadDescriptor(TestService, BinaryValue, UserDescription), 'user description');
    Peripheral.WriteDescriptor(TestService, BinaryValue, UserDescription, TEncoding.ASCII.GetBytes('Delphi'));
    CheckBytes(TEncoding.ASCII.GetBytes('Delphi'),
      Peripheral.ReadDescriptor(TestService, BinaryValue, UserDescription), 'user description after a write');
  finally
    Peripheral.WriteDescriptor(TestService, BinaryValue, UserDescription, TEncoding.ASCII.GetBytes('Plain value'));
    Peripheral.Disconnect;
  end;
end;

procedure TestNotifyAndIndicate;
var
  Peripheral: TBlePeripheral;
begin
  Peripheral := PlainPeripheral;
  Peripheral.Connect;
  try
    Recorder.Reset;
    Peripheral.Notify(BatteryService, BatteryLevel, Recorder.Value);
    WaitFor(Recorder.ValueReceived, 'a notification');
    Peripheral.Unsubscribe(BatteryService, BatteryLevel);
    CheckEquals(BatteryService, Recorder.ValueService, 'notification service');
    CheckEquals(BatteryLevel, Recorder.ValueCharacteristic, 'notification characteristic');
    CheckBytes(Bytes([100]), Recorder.ValueData, 'notification value');

    Recorder.Reset;
    Peripheral.Indicate(TestService, BinaryValue, Recorder.Value);
    WaitFor(Recorder.ValueReceived, 'an indication');
    Peripheral.Unsubscribe(TestService, BinaryValue);
    CheckEquals(BinaryValue, Recorder.ValueCharacteristic, 'indication characteristic');
    CheckBytes(Bytes([$00, $7F, $80, $FF]), Recorder.ValueData, 'indication value');
  finally
    Peripheral.Disconnect;
  end;
end;

procedure TestAsyncCalls;
var
  Peripheral: TBlePeripheral;
begin
  Peripheral := PlainPeripheral;
  Recorder.Reset;
  Peripheral.ConnectAsync(Recorder.Completed);
  WaitFor(Recorder.Done, 'ConnectAsync');
  Check(Recorder.DoneSender = Peripheral, 'ConnectAsync reported another sender');
  CheckEquals('', Recorder.DoneError, 'ConnectAsync error');
  Check(Peripheral.IsConnected, 'not connected after ConnectAsync');
  try
    Recorder.Reset;
    Peripheral.ReadAsync(TestService, BinaryValue, Recorder.ReadCompleted);
    WaitFor(Recorder.Done, 'ReadAsync');
    CheckBytes(Bytes([$00, $7F, $80, $FF]), Recorder.DoneData, 'ReadAsync value');
    Recorder.Reset;
    Peripheral.ReadAsync(TestService, ErrorValue, Recorder.ReadCompleted);
    WaitFor(Recorder.Done, 'failing ReadAsync');
    CheckEquals('EBleOperationFailed', Recorder.DoneError, 'ReadAsync error');
  finally
    Peripheral.Disconnect;
  end;
end;

procedure TestGattErrors;
var
  Peripheral: TBlePeripheral;
begin
  Peripheral := PlainPeripheral;
  try
    Peripheral.Read(BatteryService, BatteryLevel);
    Fail('reading while disconnected raised nothing');
  except
    on E: EBleNotConnected do
      CheckEquals(Ord(ecPeripheralNotConnected), Ord(E.Code), 'not connected code');
  end;
  Peripheral.Connect;
  try
    try
      Peripheral.Read(MissingUuid, BatteryLevel);
      Fail('reading a missing service raised nothing');
    except
      on E: EBleServiceNotFound do
        CheckEquals(Ord(ecGattServiceNotFound), Ord(E.Code), 'service code');
    end;
    try
      Peripheral.Read(BatteryService, MissingUuid);
      Fail('reading a missing characteristic raised nothing');
    except
      on E: EBleCharacteristicNotFound do
        CheckEquals(Ord(ecGattCharacteristicNotFound), Ord(E.Code), 'characteristic code');
    end;
    try
      Peripheral.ReadDescriptor(TestService, BinaryValue, MissingUuid);
      Fail('reading a missing descriptor raised nothing');
    except
      on E: EBleDescriptorNotFound do
        CheckEquals(Ord(ecGattDescriptorNotFound), Ord(E.Code), 'descriptor code');
    end;
    try
      Peripheral.Read(TestService, ErrorValue);
      Fail('reading the error characteristic raised nothing');
    except
      on E: EBleOperationFailed do
        CheckEquals(Ord(ecOperationFailed), Ord(E.Code), 'failed read code');
    end;
    try
      Peripheral.Read(TestService + '0', BinaryValue);
      Fail('a 37-character UUID raised nothing');
    except
      on EBleInvalidArgument do
        ;
    end;
    try
      Peripheral.Notify(TestService, BinaryValue, nil);
      Fail('Notify without a handler raised nothing');
    except
      on EBleInvalidArgument do
        ;
    end;
  finally
    Peripheral.Disconnect;
  end;
end;

{ Hosting }

procedure TestLocalConfiguration;
var
  Local: TBleLocalPeripheral;
  Service: TBleLocalService;
  Value: TBleLocalCharacteristic;
  Other: TBleLocalCharacteristic;
begin
  Local := PlainAdapter.CreateLocalPeripheral;
  try
    Service := Local.AddService(HostService);
    Value := Service.AddCharacteristic(HostValue, [lcRead, lcWriteRequest, lcNotify]);
    Other := Service.AddCharacteristic(HostOther, [lcWriteCommand, lcIndicate]);
    CheckEquals(HostService, Service.Uuid, 'service UUID');
    CheckEquals(HostValue, Value.Uuid, 'characteristic UUID');
    Check(Value.Capabilities = [lcRead, lcWriteRequest, lcNotify], 'capabilities');
    Check(Other.Capabilities = [lcWriteCommand, lcIndicate], 'other capabilities');
    Value.SetValue(Bytes([0, $FF]));
    CheckBytes(Bytes([0, $FF]), Value.Value, 'value');
    CheckEquals(1, Length(Local.Services), 'service count');
    Check(Local.Services[0] = Service, 'Services returned another object');
    Check(Service.Characteristics[0] = Value, 'Characteristics returned another object');
    Check(Service.Characteristics[1] = Other, 'Characteristics returned another object');
    Local.RemoveAllServices;
    CheckEquals(0, Length(Local.Services), 'service count after RemoveAllServices');
  finally
    Local.Free;
  end;
end;

procedure TestLocalLoopback;
var
  Local: TBleLocalPeripheral;
  Value: TBleLocalCharacteristic;
  Host: TBlePeripheral;
  Data: TArray<TBleManufacturerData>;
  Service: TBleService;
begin
  Recorder.Reset;
  Local := PlainAdapter.CreateLocalPeripheral;
  try
    Value := Local.AddService(HostService).AddCharacteristic(HostValue, [lcRead, lcWriteRequest, lcNotify]);
    Value.SetValue(Bytes([0, $FF]));
    Value.OnWrite := Recorder.LocalWrite;
    Value.OnSubscribed := Recorder.LocalSubscribe;
    Value.OnUnsubscribed := Recorder.LocalUnsubscribe;
    Local.AddAdvertisedService(HostService);
    Local.SetAdvertisedManufacturerData($FFFF, Bytes([1, 2]));
    Local.SetConnectable(True);
    Local.OnClientConnected := Recorder.ClientConnect;
    Local.OnClientDisconnected := Recorder.ClientDisconnect;
    Local.Start;
    try
      Check(Local.IsStarted, 'not started');
      Check(Local.IsAdvertising, 'not advertising');
      Host := FindPeripheral('Plain Adapter Peripheral');
      Data := Host.ManufacturerData;
      CheckEquals(1, Length(Data), 'advertised manufacturer data entries');
      CheckEquals($FFFF, Data[0].CompanyId, 'advertised company identifier');
      CheckBytes(Bytes([1, 2]), Data[0].Data, 'advertised manufacturer data');
      Check(FindService(Host.Services, HostService, Service), 'service not advertised');
      Host.Connect;
      try
        WaitFor(Recorder.ClientConnected, 'OnClientConnected');
        Check(Recorder.ClientAddress <> '', 'client address is empty');
        CheckBytes(Bytes([0, $FF]), Host.Read(HostService, HostValue), 'stored value');
        Value.OnReadRequest := Recorder.LocalRead;
        CheckBytes(Bytes([1, 2, 3]), Host.Read(HostService, HostValue), 'OnReadRequest value');
        Value.OnReadRequest := nil;
        CheckBytes(Bytes([0, $FF]), Host.Read(HostService, HostValue), 'stored value after OnReadRequest');
        Host.WriteRequest(HostService, HostValue, Bytes([9, 8]));
        WaitFor(Recorder.LocalWritten, 'OnWrite');
        CheckBytes(Bytes([9, 8]), Recorder.LocalData, 'written data');
        CheckBytes(Bytes([9, 8]), Value.Value, 'value after a write');
        Host.Notify(HostService, HostValue, Recorder.Value);
        WaitFor(Recorder.Subscribed, 'OnSubscribed');
        Value.SetValue(Bytes([7]));
        WaitFor(Recorder.ValueReceived, 'a notification of the new value');
        CheckBytes(Bytes([7]), Recorder.ValueData, 'notified value');
        Host.Unsubscribe(HostService, HostValue);
        WaitFor(Recorder.Unsubscribed, 'OnUnsubscribed');
      finally
        Host.Disconnect;
      end;
      WaitFor(Recorder.ClientDisconnected, 'OnClientDisconnected');
    finally
      Local.Stop;
    end;
    Check(not Local.IsStarted, 'started after Stop');
  finally
    Local.Free;
  end;
end;

// Freeing a local peripheral drops the events still queued for it and its characteristics.
procedure TestLocalFreeDropsEvents;
var
  Local: TBleLocalPeripheral;
  Host: TBlePeripheral;
  Deadline: TDateTime;
begin
  Recorder.Reset;
  TSimpleDelphiBLE.EventDelivery := edMainThread;
  try
    Local := PlainAdapter.CreateLocalPeripheral;
    try
      Local.AddService(HostService).AddCharacteristic(HostValue, [lcRead]);
      Local.OnClientConnected := Recorder.ClientConnect;
      Local.Start;
      Host := FindPeripheral('Plain Adapter Peripheral');
      Host.Connect;
      Host.Disconnect;
      Local.Stop;
    finally
      Local.Free;
    end;
    Deadline := Now + 200 / MSecsPerDay;
    while Now < Deadline do
      CheckSynchronize(10);
    Check(not Recorder.ClientConnected, 'an event of a freed local peripheral ran');
  finally
    TSimpleDelphiBLE.EventDelivery := edAuto;
  end;
end;

{ Backends, configuration, logging and pairing }

procedure TestBackends;
var
  Backends: TArray<TBleBackend>;
  Adapters: TArray<TBleAdapter>;
begin
  Backends := TBleBackend.GetBackends;
  CheckEquals(1, Length(Backends), 'backend count');
  CheckEquals('Plain', Backends[0].Identifier, 'backend identifier');
  Check(TBleBackend.GetBackends[0] = Backends[0], 'GetBackends returned another object');
  Check(Backends[0].IsBluetoothEnabled, 'Bluetooth is disabled');
  Adapters := Backends[0].GetAdapters;
  CheckEquals(1, Length(Adapters), 'backend adapter count');
  Check(Adapters[0] = PlainAdapter, 'the backend returned another adapter object');
  Check(TBleBackend.GetNative = Backends[0], 'GetNative returned another object');
end;

procedure TestConfig;
var
  UseSystemBus: Boolean;
  ConnectionTimeout: Int64;
  DisconnectionTimeout: Int64;
  InteractivePairing: Boolean;
  Reinitialize: Boolean;
  DeferredDisconnect: Boolean;
  NegotiateMtu: Boolean;
  Priority: TBleAndroidConnectionPriority;
  NewPriority: TBleAndroidConnectionPriority;
  UseDongl: Boolean;
  AutoUpdate: Boolean;
  ForceUpdate: Boolean;

  procedure ChangeAll;
  begin
    TBleConfig.SimpleBluez.UseSystemBus := not UseSystemBus;
    TBleConfig.SimpleBluez.ConnectionTimeoutMs := ConnectionTimeout + 1234;
    TBleConfig.SimpleBluez.DisconnectionTimeoutMs := DisconnectionTimeout + 4321;
    TBleConfig.SimpleBluez.InteractivePairing := not InteractivePairing;
    TBleConfig.WinRT.ExperimentalReinitializeWinrtApartmentOnMainThread := not Reinitialize;
    TBleConfig.WinRT.UseDeferredDisconnect := not DeferredDisconnect;
    TBleConfig.Android.NegotiateMtu := not NegotiateMtu;
    TBleConfig.Android.ConnectionPriority := NewPriority;
    TBleConfig.Dongl.UseDonglBackend := not UseDongl;
    TBleConfig.Dongl.AutoUpdate := not AutoUpdate;
    TBleConfig.Dongl.ForceUpdate := not ForceUpdate;
  end;

  procedure CheckSimpleBluez(Changed: Boolean; const Description: string);
  begin
    Check(TBleConfig.SimpleBluez.UseSystemBus = (UseSystemBus xor Changed), Description + ': UseSystemBus');
    CheckEquals(ConnectionTimeout + Ord(Changed) * 1234, TBleConfig.SimpleBluez.ConnectionTimeoutMs,
      Description + ': ConnectionTimeoutMs');
    CheckEquals(DisconnectionTimeout + Ord(Changed) * 4321, TBleConfig.SimpleBluez.DisconnectionTimeoutMs,
      Description + ': DisconnectionTimeoutMs');
    Check(TBleConfig.SimpleBluez.InteractivePairing = (InteractivePairing xor Changed),
      Description + ': InteractivePairing');
  end;

  procedure CheckWinRT(Changed: Boolean; const Description: string);
  begin
    Check(TBleConfig.WinRT.ExperimentalReinitializeWinrtApartmentOnMainThread = (Reinitialize xor Changed),
      Description + ': ExperimentalReinitializeWinrtApartmentOnMainThread');
    Check(TBleConfig.WinRT.UseDeferredDisconnect = (DeferredDisconnect xor Changed),
      Description + ': UseDeferredDisconnect');
  end;

  procedure CheckAndroid(Changed: Boolean; const Description: string);
  begin
    Check(TBleConfig.Android.NegotiateMtu = (NegotiateMtu xor Changed), Description + ': NegotiateMtu');
    if Changed then
      CheckEquals(Ord(NewPriority), Ord(TBleConfig.Android.ConnectionPriority), Description + ': ConnectionPriority')
    else
      CheckEquals(Ord(Priority), Ord(TBleConfig.Android.ConnectionPriority), Description + ': ConnectionPriority');
  end;

  procedure CheckDongl(Changed: Boolean; const Description: string);
  begin
    Check(TBleConfig.Dongl.UseDonglBackend = (UseDongl xor Changed), Description + ': UseDonglBackend');
    Check(TBleConfig.Dongl.AutoUpdate = (AutoUpdate xor Changed), Description + ': AutoUpdate');
    Check(TBleConfig.Dongl.ForceUpdate = (ForceUpdate xor Changed), Description + ': ForceUpdate');
  end;

begin
  TBleConfig.ResetAll;
  UseSystemBus := TBleConfig.SimpleBluez.UseSystemBus;
  ConnectionTimeout := TBleConfig.SimpleBluez.ConnectionTimeoutMs;
  DisconnectionTimeout := TBleConfig.SimpleBluez.DisconnectionTimeoutMs;
  InteractivePairing := TBleConfig.SimpleBluez.InteractivePairing;
  Reinitialize := TBleConfig.WinRT.ExperimentalReinitializeWinrtApartmentOnMainThread;
  DeferredDisconnect := TBleConfig.WinRT.UseDeferredDisconnect;
  NegotiateMtu := TBleConfig.Android.NegotiateMtu;
  Priority := TBleConfig.Android.ConnectionPriority;
  if Priority = cpDck then
    NewPriority := cpDisabled
  else
    NewPriority := cpDck;
  UseDongl := TBleConfig.Dongl.UseDonglBackend;
  AutoUpdate := TBleConfig.Dongl.AutoUpdate;
  ForceUpdate := TBleConfig.Dongl.ForceUpdate;
  try
    ChangeAll;
    CheckSimpleBluez(True, 'changed');
    CheckWinRT(True, 'changed');
    CheckAndroid(True, 'changed');
    CheckDongl(True, 'changed');

    TBleConfig.SimpleBluez.Reset;
    CheckSimpleBluez(False, 'SimpleBluez.Reset');
    CheckWinRT(True, 'SimpleBluez.Reset');
    TBleConfig.WinRT.Reset;
    CheckWinRT(False, 'WinRT.Reset');
    CheckAndroid(True, 'WinRT.Reset');
    TBleConfig.Android.Reset;
    CheckAndroid(False, 'Android.Reset');
    CheckDongl(True, 'Android.Reset');
    TBleConfig.Dongl.Reset;
    CheckDongl(False, 'Dongl.Reset');
    TBleConfig.CoreBluetooth.Reset;

    ChangeAll;
    TBleConfig.ResetAll;
    CheckSimpleBluez(False, 'ResetAll');
    CheckWinRT(False, 'ResetAll');
    CheckAndroid(False, 'ResetAll');
    CheckDongl(False, 'ResetAll');
  finally
    TBleConfig.ResetAll;
  end;
end;

procedure TestLogging;
var
  Adapter: TBleAdapter;
  Level: TBleLogLevel;
begin
  Level := TBleLogging.Level;
  try
    TBleLogging.Level := llDebug;
    CheckEquals(Ord(llDebug), Ord(TBleLogging.Level), 'log level');
    TBleLogging.Level := llWarn;
    CheckEquals(Ord(llWarn), Ord(TBleLogging.Level), 'log level');
  finally
    TBleLogging.Level := Level;
  end;
  // NOTE: Scanning while powered off logs a warning.
  Adapter := PlainAdapter;
  Recorder.Reset;
  TBleLogging.OnLog := Recorder.Log;
  try
    Adapter.PowerOff;
    Adapter.ScanFor(1);
    WaitFor(Recorder.Logged, 'OnLog');
    Check(Recorder.LogEntry.Message <> '', 'log message is empty');
    Check(Recorder.LogEntry.Module <> '', 'log module is empty');
    Check(Recorder.LogEntry.FileName <> '', 'log file name is empty');
    Check(Recorder.LogEntry.FunctionName <> '', 'log function name is empty');
    Check(Recorder.LogEntry.Line > 0, 'log line is zero');
  finally
    TBleLogging.LogToStdout;
    Adapter.PowerOn;
  end;
end;

procedure TestPairingHandlers;
var
  Peripheral: TBlePeripheral;
  Adapter: TBleAdapter;
begin
  // NOTE: Only a Dongl peripheral takes pairing handlers, so PLAIN's reports OPERATION_NOT_SUPPORTED.
  Peripheral := PlainPeripheral;
  try
    Peripheral.OnPasskeyRequest := Recorder.PasskeyRequest;
    Fail('OnPasskeyRequest raised nothing');
  except
    on E: EBleOperationNotSupported do
      CheckEquals(Ord(ecOperationNotSupported), Ord(E.Code), 'passkey request code');
  end;
  Check(not Assigned(Peripheral.OnPasskeyRequest), 'OnPasskeyRequest kept a rejected handler');
  try
    Peripheral.OnPasskeyDisplay := Recorder.PasskeyDisplay;
    Fail('OnPasskeyDisplay raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  Check(not Assigned(Peripheral.OnPasskeyDisplay), 'OnPasskeyDisplay kept a rejected handler');
  try
    Peripheral.OnNumericComparison := Recorder.NumericComparison;
    Fail('OnNumericComparison raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  Check(not Assigned(Peripheral.OnNumericComparison), 'OnNumericComparison kept a rejected handler');
  try
    Peripheral.SecureConnection(True);
    Fail('SecureConnection raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  // NOTE: Only a BlueZ adapter takes pairing handlers, so PLAIN's raises, like every adapter off Linux.
  Adapter := PlainAdapter;
  try
    Adapter.OnPasskeyRequest := Recorder.AdapterPasskeyRequest;
    Fail('adapter OnPasskeyRequest raised nothing');
  except
    on E: EBleOperationNotSupported do
      CheckEquals(Ord(ecOperationNotSupported), Ord(E.Code), 'adapter passkey request code');
  end;
  Check(not Assigned(Adapter.OnPasskeyRequest), 'adapter OnPasskeyRequest kept a rejected handler');
  try
    Adapter.OnPasskeyDisplay := Recorder.AdapterPasskeyDisplay;
    Fail('adapter OnPasskeyDisplay raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  Check(not Assigned(Adapter.OnPasskeyDisplay), 'adapter OnPasskeyDisplay kept a rejected handler');
  try
    Adapter.OnNumericComparison := Recorder.AdapterNumericComparison;
    Fail('adapter OnNumericComparison raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  Check(not Assigned(Adapter.OnNumericComparison), 'adapter OnNumericComparison kept a rejected handler');
  try
    Adapter.OnPairingAuthorization := Recorder.PairingAuthorization;
    Fail('OnPairingAuthorization raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  Check(not Assigned(Adapter.OnPairingAuthorization), 'OnPairingAuthorization kept a rejected handler');
end;

procedure TestPlatformCalls;
var
  Local: TBleLocalPeripheral;
begin
  // NOTE: PLAIN has neither feature, and Windows builds lack the functions, so both calls raise on every platform.
  try
    PlainAdapter.RetrieveCachedPeripherals(['11:22:33:44:55:66']);
    Fail('RetrieveCachedPeripherals raised nothing');
  except
    on EBleOperationNotSupported do
      ;
  end;
  Local := PlainAdapter.CreateLocalPeripheral;
  try
    try
      Local.SetAdvertisementLocalName('SimpleDelphiBLE');
      Fail('SetAdvertisementLocalName raised nothing');
    except
      on EBleOperationNotSupported do
        ;
    end;
  finally
    Local.Free;
  end;
end;

procedure TestOwnership;
var
  Adapter: TBleAdapter;
  Local: TBleLocalPeripheral;
  Service: TBleLocalService;
begin
  Adapter := PlainAdapter;
  try
    Adapter.Free;
    Fail('freeing an adapter raised nothing');
  except
    on EInvalidOperation do
      ;
  end;
  CheckEquals('Plain Adapter', Adapter.Identifier, 'identifier after Free');
  try
    PlainPeripheral.Free;
    Fail('freeing a peripheral raised nothing');
  except
    on EInvalidOperation do
      ;
  end;
  try
    TBleBackend.GetNative.Free;
    Fail('freeing a backend raised nothing');
  except
    on EInvalidOperation do
      ;
  end;
  Local := Adapter.CreateLocalPeripheral;
  try
    Service := Local.AddService(HostService);
    try
      Service.Free;
      Fail('freeing a local service raised nothing');
    except
      on EInvalidOperation do
        ;
    end;
  finally
    Local.Free;
  end;
end;

type
  TTest = procedure;

var
  Passed: Integer;
  Failed: Integer;
  Peripheral: TBlePeripheral;

procedure Run(const Name: string; Test: TTest);
begin
  try
    Test;
    Inc(Passed);
    WriteLn('PASS ', Name);
  except
    on E: Exception do
    begin
      Inc(Failed);
      WriteLn('FAIL ', Name, ': ', E.ClassName, ': ', E.Message);
    end;
  end;
end;

begin
  Recorder := TRecorder.Create;
  Run('library loading', TestLibraryLoading);
  Run('version and operating system', TestVersionAndOperatingSystem);
  Run('adapters', TestAdapters);
  Run('power', TestPower);
  Run('scan events', TestScanEvents);
  Run('peripheral properties', TestPeripheralProperties);
  Run('error mapping', TestErrorMapping);
  Run('event threads', TestEventThreads);
  Run('handler exception', TestHandlerException);
  Run('connection', TestConnection);
  Run('services', TestServices);
  Run('read and write', TestReadWrite);
  Run('descriptors', TestDescriptors);
  Run('notify and indicate', TestNotifyAndIndicate);
  Run('GATT errors', TestGattErrors);
  Run('async calls', TestAsyncCalls);
  Run('local configuration', TestLocalConfiguration);
  Run('local loopback', TestLocalLoopback);
  Run('freeing a local peripheral drops its events', TestLocalFreeDropsEvents);
  Run('backends', TestBackends);
  Run('config', TestConfig);
  Run('logging', TestLogging);
  Run('pairing handlers', TestPairingHandlers);
  Run('platform calls', TestPlatformCalls);
  Run('ownership', TestOwnership);
  WriteLn(Passed, ' passed, ', Failed, ' failed');
  ExitCode := Failed;
  // NOTE: The program ends while SimpleBLE scans and notifies, so the unit's finalization runs with callbacks
  // arriving.
  Peripheral := PlainPeripheral;
  Peripheral.Connect;
  Peripheral.Notify(BatteryService, BatteryLevel, Recorder.Value);
  PlainAdapter.OnScanFound := Recorder.ScanFound;
  PlainAdapter.ScanStart;
  Sleep(1500);
end.
