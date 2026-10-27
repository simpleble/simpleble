// Part of SimpleDelphiBLE.pas: the native callbacks, which turn SimpleBLE's calls into events.

{ Native callbacks }

// Finds the object a callback was registered with, or nil once it's gone. Callbacks run between EnterCallback and
// LeaveCallback.
function FindTarget(Token: Pointer): TBleNativeObject;
begin
  Lock.Enter;
  try
    if not Targets.TryGetValue(NativeUInt(Token), Result) then
      Result := nil;
  finally
    Lock.Leave;
  end;
end;

// Queues an event for the object a callback was registered with. The lookup and the queueing happen under the lock,
// so an object that's being freed either has the event purged or isn't found.
procedure PostEvent(Token: Pointer; var Event: TBleEvent);
begin
  if not EnterCallback then
    Exit;
  try
    try
      Lock.Enter;
      try
        if Targets.TryGetValue(NativeUInt(Token), Event.Target) then
        begin
          if Event.Target is TBleLocalPeripheral then
            Event.Owner := TBleLocalPeripheral(Event.Target)
          else if Event.Target is TBleLocalCharacteristic then
            Event.Owner := TBleLocalCharacteristic(Event.Target).FPeripheral;
          Dispatcher.Post(Event);
        end;
      finally
        Lock.Leave;
      end;
    except
      // NOTE: An exception can't cross into SimpleBLE's C++ code.
    end;
  finally
    LeaveCallback;
  end;
end;

procedure PostSimpleEvent(Token: Pointer; Kind: TBleEventKind);
var
  Event: TBleEvent;
begin
  Event := Default(TBleEvent);
  Event.Kind := Kind;
  PostEvent(Token, Event);
end;

// Queues a scan event, interning the peripheral handle it received or releasing it once the adapter is gone.
procedure PostScanEvent(Token: Pointer; Kind: TBleEventKind; Peripheral: simpleble_peripheral_t);
var
  Event: TBleEvent;
  Adapter: TBleNativeObject;
begin
  if not EnterCallback then
  begin
    simpleble_peripheral_release_handle(Peripheral);
    Exit;
  end;
  try
    try
      Adapter := FindTarget(Token);
      if Adapter = nil then
      begin
        simpleble_peripheral_release_handle(Peripheral);
        Exit;
      end;
      Event := Default(TBleEvent);
      Event.Kind := Kind;
      Event.Peripheral := TBleAdapter(Adapter).Intern(Peripheral);
      PostEvent(Token, Event);
    except
      // NOTE: An exception can't cross into SimpleBLE's C++ code.
    end;
  finally
    LeaveCallback;
  end;
end;

procedure AdapterPowerOnCallback(adapter: simpleble_adapter_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekPowerOn);
end;

procedure AdapterPowerOffCallback(adapter: simpleble_adapter_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekPowerOff);
end;

procedure AdapterScanStartCallback(adapter: simpleble_adapter_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekScanStart);
end;

procedure AdapterScanStopCallback(adapter: simpleble_adapter_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekScanStop);
end;

procedure AdapterScanFoundCallback(adapter: simpleble_adapter_t; peripheral: simpleble_peripheral_t;
  userdata: Pointer); cdecl;
begin
  PostScanEvent(userdata, ekScanFound, peripheral);
end;

procedure AdapterScanUpdatedCallback(adapter: simpleble_adapter_t; peripheral: simpleble_peripheral_t;
  userdata: Pointer); cdecl;
begin
  PostScanEvent(userdata, ekScanUpdated, peripheral);
end;

procedure PeripheralConnectedCallback(peripheral: simpleble_peripheral_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekConnected);
end;

procedure PeripheralDisconnectedCallback(peripheral: simpleble_peripheral_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekDisconnected);
end;

procedure PeripheralValueCallback(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; data: PByte; data_length: NativeUInt; userdata: Pointer); cdecl;
var
  Event: TBleEvent;
begin
  try
    Event := Default(TBleEvent);
    Event.Kind := ekValue;
    Event.Service := FromUuid(service);
    Event.Characteristic := FromUuid(characteristic);
    Event.Data := CopyBytes(data, data_length);
  except
    // NOTE: An exception can't cross into SimpleBLE's C++ code.
    Exit;
  end;
  PostEvent(userdata, Event);
end;

function IsPasskey(const Value: string): Boolean;
var
  C: Char;
begin
  Result := Length(Value) = 6;
  for C in Value do
    Result := Result and (C >= '0') and (C <= '9');
end;

function PasskeyRequestCallback(handle: simpleble_peripheral_t; passkey: PAnsiChar; userdata: Pointer): Boolean; cdecl;
var
  Peripheral: TBlePeripheral;
  Handler: TBlePasskeyRequestEvent;
  Value: string;
  Accept: Boolean;
  Digits: RawByteString;
begin
  Result := False;
  if not EnterCallback then
    Exit;
  try
    try
      Peripheral := TBlePeripheral(FindTarget(userdata));
      if Peripheral = nil then
        Exit;
      Handler := Peripheral.FOnPasskeyRequest;
      if not Assigned(Handler) then
        Exit;
      Value := '';
      Accept := False;
      Handler(Peripheral, Value, Accept);
      if not Accept then
        Exit;
      if not IsPasskey(Value) then
        raise EBleInvalidArgument.Create(ecInvalidArgument, 'A passkey has exactly six decimal digits: ' + Value);
      Digits := UTF8Encode(Value);
      Move(Digits[1], passkey^, 6);
      passkey[6] := #0;
      Result := True;
    except
      on E: Exception do
        ReportHandlerException(E);
    end;
  finally
    LeaveCallback;
  end;
end;

procedure PasskeyDisplayCallback(handle: simpleble_peripheral_t; passkey: PAnsiChar; userdata: Pointer); cdecl;
var
  Event: TBleEvent;
begin
  try
    Event := Default(TBleEvent);
    Event.Kind := ekPasskeyDisplay;
    Event.Text := FromUtf8(passkey);
  except
    // NOTE: An exception can't cross into SimpleBLE's C++ code.
    Exit;
  end;
  PostEvent(userdata, Event);
end;

function NumericComparisonCallback(handle: simpleble_peripheral_t; passkey: PAnsiChar;
  userdata: Pointer): Boolean; cdecl;
var
  Peripheral: TBlePeripheral;
  Handler: TBleNumericComparisonEvent;
  Accept: Boolean;
begin
  Result := False;
  if not EnterCallback then
    Exit;
  try
    try
      Peripheral := TBlePeripheral(FindTarget(userdata));
      if Peripheral = nil then
        Exit;
      Handler := Peripheral.FOnNumericComparison;
      if not Assigned(Handler) then
        Exit;
      Accept := False;
      Handler(Peripheral, FromUtf8(passkey), Accept);
      Result := Accept;
    except
      on E: Exception do
        ReportHandlerException(E);
    end;
  finally
    LeaveCallback;
  end;
end;

procedure ClientConnectedCallback(handle: simpleble_local_peripheral_t; client_address: PAnsiChar;
  userdata: Pointer); cdecl;
var
  Event: TBleEvent;
begin
  try
    Event := Default(TBleEvent);
    Event.Kind := ekClientConnected;
    Event.Text := FromUtf8(client_address);
  except
    // NOTE: An exception can't cross into SimpleBLE's C++ code.
    Exit;
  end;
  PostEvent(userdata, Event);
end;

procedure ClientDisconnectedCallback(handle: simpleble_local_peripheral_t; client_address: PAnsiChar;
  userdata: Pointer); cdecl;
var
  Event: TBleEvent;
begin
  try
    Event := Default(TBleEvent);
    Event.Kind := ekClientDisconnected;
    Event.Text := FromUtf8(client_address);
  except
    // NOTE: An exception can't cross into SimpleBLE's C++ code.
    Exit;
  end;
  PostEvent(userdata, Event);
end;

function LocalReadCallback(handle: simpleble_local_characteristic_t; out data_length: NativeUInt;
  userdata: Pointer): PByte; cdecl;
var
  Characteristic: TBleLocalCharacteristic;
  Handler: TBleLocalReadEvent;
  Value: TBytes;
begin
  Result := nil;
  data_length := 0;
  if not EnterCallback then
    Exit;
  try
    try
      Characteristic := TBleLocalCharacteristic(FindTarget(userdata));
      if Characteristic = nil then
        Exit;
      Handler := Characteristic.FOnReadRequest;
      Value := nil;
      if Assigned(Handler) then
        Handler(Characteristic, Value);
      // NOTE: SimpleBLE copies the value after this returns, and needs it until the next read.
      Characteristic.FReadValue := Value;
      data_length := Length(Value);
      if data_length > 0 then
        Result := @Characteristic.FReadValue[0];
    except
      on E: Exception do
        ReportHandlerException(E);
    end;
  finally
    LeaveCallback;
  end;
end;

procedure LocalWriteCallback(handle: simpleble_local_characteristic_t; data: PByte; data_length: NativeUInt;
  userdata: Pointer); cdecl;
var
  Event: TBleEvent;
begin
  try
    Event := Default(TBleEvent);
    Event.Kind := ekLocalWrite;
    Event.Data := CopyBytes(data, data_length);
  except
    // NOTE: An exception can't cross into SimpleBLE's C++ code.
    Exit;
  end;
  PostEvent(userdata, Event);
end;

procedure LocalSubscribedCallback(handle: simpleble_local_characteristic_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekLocalSubscribed);
end;

procedure LocalUnsubscribedCallback(handle: simpleble_local_characteristic_t; userdata: Pointer); cdecl;
begin
  PostSimpleEvent(userdata, ekLocalUnsubscribed);
end;

procedure LogCallback(level: simpleble_log_level_t; module: PAnsiChar; file_name: PAnsiChar; line: UInt32;
  function_name: PAnsiChar; message: PAnsiChar); cdecl;
var
  Event: TBleEvent;
begin
  if not EnterCallback then
    Exit;
  try
    try
      Event := Default(TBleEvent);
      Event.Kind := ekLog;
      if (level >= Ord(Low(TBleLogLevel))) and (level <= Ord(High(TBleLogLevel))) then
        Event.Log.Level := TBleLogLevel(level)
      else
        Event.Log.Level := llVerbose;
      Event.Log.Module := FromUtf8(module);
      Event.Log.FileName := FromUtf8(file_name);
      Event.Log.Line := line;
      Event.Log.FunctionName := FromUtf8(function_name);
      Event.Log.Message := FromUtf8(message);
      Dispatcher.Post(Event);
    except
      // NOTE: An exception can't cross into SimpleBLE's C++ code.
    end;
  finally
    LeaveCallback;
  end;
end;
