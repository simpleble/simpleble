// Part of SimpleDelphiBLE.pas: TBleLocalPeripheral, TBleLocalService and TBleLocalCharacteristic.

{ TBleLocalPeripheral }

constructor TBleLocalPeripheral.CreateFromHandle(Handle: Pointer);
begin
  inherited Create;
  FHandle := Handle;
  FFreeable := True;
  FServiceObjects := TList<TBleLocalService>.Create;
  FServices := TDictionary<Pointer, TBleLocalService>.Create;
end;

destructor TBleLocalPeripheral.Destroy;
var
  Service: TBleLocalService;
  Characteristic: TBleLocalCharacteristic;
begin
  if FServiceObjects <> nil then
  begin
    ClearCallbacks;
    // NOTE: Once this object and its characteristics are unregistered and their queued events dropped, callbacks
    // still running can't find them.
    Lock.Enter;
    try
      Unregister;
      for Service in FServiceObjects do
        for Characteristic in Service.FCharacteristicObjects do
          Characteristic.Unregister;
      Dispatcher.Purge(Self);
    finally
      Lock.Leave;
    end;
    for Service in FServiceObjects do
      Service.FreeOwned;
    FServiceObjects.Free;
    FServices.Free;
  end;
  simpleble_local_peripheral_release_handle(FHandle);
  inherited;
end;

procedure TBleLocalPeripheral.SetCallbacks(Enabled: Boolean);
begin
  if Enabled then
  begin
    simpleble_local_peripheral_set_callback_on_client_connected(FHandle, ClientConnectedCallback, Pointer(FToken));
    simpleble_local_peripheral_set_callback_on_client_disconnected(FHandle, ClientDisconnectedCallback,
      Pointer(FToken));
  end
  else
  begin
    simpleble_local_peripheral_set_callback_on_client_connected(FHandle, nil, nil);
    simpleble_local_peripheral_set_callback_on_client_disconnected(FHandle, nil, nil);
  end;
end;

procedure TBleLocalPeripheral.ClearCallbacks;
var
  Service: TBleLocalService;
begin
  SetCallbacks(False);
  for Service in FServiceObjects do
    Service.ClearCallbacks;
end;

// Returns the one service object for a handle, taking ownership of the handle.
function TBleLocalPeripheral.Intern(Handle: Pointer): TBleLocalService;
var
  Identity: Pointer;
  Created: Boolean;
begin
  Identity := simpleble_local_service_identity(Handle);
  Lock.Enter;
  try
    Created := not FServices.TryGetValue(Identity, Result);
    if Created then
    begin
      Result := TBleLocalService.CreateFromHandle(Self, Handle);
      FServices.Add(Identity, Result);
      FServiceObjects.Add(Result);
    end;
  finally
    Lock.Leave;
  end;
  if not Created then
    simpleble_local_service_release_handle(Handle);
end;

procedure TBleLocalPeripheral.AddAdvertisedService(const Uuid: string);
var
  Call: TNativeCall;
  NativeUuid: simpleble_uuid_t;
begin
  NativeUuid := ToUuid(Uuid);
  Call.Enter;
  simpleble_local_peripheral_add_advertised_service(FHandle, NativeUuid, Call.Error);
  Call.Leave;
end;

procedure TBleLocalPeripheral.SetAdvertisedManufacturerData(CompanyId: Word; const Data: TBytes);
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_local_peripheral_set_advertised_manufacturer_data(FHandle, CompanyId, DataPointer(Data), Length(Data),
    Call.Error);
  Call.Leave;
end;

procedure TBleLocalPeripheral.SetConnectable(Connectable: Boolean);
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_local_peripheral_set_connectable(FHandle, Connectable, Call.Error);
  Call.Leave;
end;

// Sets the advertised local name, or removes it when Name is nil.
procedure TBleLocalPeripheral.SetLocalName(Name: PAnsiChar);
{$IF DEFINED(SIMPLEDELPHIBLE_MACOS) or DEFINED(SIMPLEDELPHIBLE_LINUX)}
var
  Call: TNativeCall;
begin
  Call.Enter;
{$IFDEF SIMPLEDELPHIBLE_MACOS}
  simpleble_advanced_macos_set_advertisement_local_name(FHandle, Name, Call.Error);
{$ELSE}
  simpleble_advanced_linux_set_advertisement_local_name(FHandle, Name, Call.Error);
{$ENDIF}
  Call.Leave;
end;
{$ELSE}
begin
  raise EBleOperationNotSupported.Create(ecOperationNotSupported, 'Advertisement local names require macOS or Linux');
end;
{$ENDIF}

procedure TBleLocalPeripheral.SetAdvertisementLocalName(const Name: string);
var
  Bytes: RawByteString;
begin
  Bytes := UTF8Encode(Name);
  SetLocalName(PAnsiChar(Bytes));
end;

procedure TBleLocalPeripheral.ClearAdvertisementLocalName;
begin
  SetLocalName(nil);
end;

function TBleLocalPeripheral.AddService(const Uuid: string): TBleLocalService;
var
  Call: TNativeCall;
  NativeUuid: simpleble_uuid_t;
  Handle: simpleble_local_service_t;
begin
  NativeUuid := ToUuid(Uuid);
  Call.Enter;
  Handle := simpleble_local_peripheral_add_service(FHandle, NativeUuid, Call.Error);
  Call.Leave;
  Result := Intern(Handle);
end;

function TBleLocalPeripheral.Services: TArray<TBleLocalService>;
var
  Call: TNativeCall;
  List: simpleble_local_service_list_t;
  Count: NativeUInt;
  I: Integer;
  Handle: simpleble_local_service_t;
begin
  Call.Enter;
  Count := simpleble_local_peripheral_services(FHandle, List, Call.Error);
  Call.Leave;
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      Call.Enter;
      Handle := simpleble_local_service_list_get(List, I, Call.Error);
      Call.Leave;
      Result[I] := Intern(Handle);
    end;
  finally
    simpleble_local_service_list_release(List);
  end;
end;

procedure TBleLocalPeripheral.RemoveAllServices;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_local_peripheral_remove_all_services(FHandle, Call.Error);
  Call.Leave;
  // NOTE: The removed services' objects stay valid until this peripheral is freed, since the application may still
  // hold them.
  Lock.Enter;
  try
    FServices.Clear;
  finally
    Lock.Leave;
  end;
end;

procedure TBleLocalPeripheral.Start;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_local_peripheral_start(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBleLocalPeripheral.Stop;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_local_peripheral_stop(FHandle, Call.Error);
  Call.Leave;
end;

function TBleLocalPeripheral.GetIsStarted: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_local_peripheral_is_started(FHandle, Call.Error);
  Call.Leave;
end;

function TBleLocalPeripheral.IsAdvertising: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_local_peripheral_is_advertising(FHandle, Call.Error);
  Call.Leave;
end;

{ TBleLocalService }

constructor TBleLocalService.CreateFromHandle(Peripheral: TBleLocalPeripheral; Handle: Pointer);
begin
  inherited Create;
  FHandle := Handle;
  FPeripheral := Peripheral;
  FCharacteristicObjects := TList<TBleLocalCharacteristic>.Create;
  FCharacteristics := TDictionary<Pointer, TBleLocalCharacteristic>.Create;
end;

destructor TBleLocalService.Destroy;
var
  Characteristic: TBleLocalCharacteristic;
begin
  if FCharacteristicObjects <> nil then
  begin
    for Characteristic in FCharacteristicObjects do
      Characteristic.FreeOwned;
    FCharacteristicObjects.Free;
    FCharacteristics.Free;
  end;
  simpleble_local_service_release_handle(FHandle);
  inherited;
end;

procedure TBleLocalService.ClearCallbacks;
var
  Characteristic: TBleLocalCharacteristic;
begin
  for Characteristic in FCharacteristicObjects do
    Characteristic.ClearCallbacks;
end;

// Returns the one characteristic object for a handle, taking ownership of the handle.
function TBleLocalService.Intern(Handle: Pointer): TBleLocalCharacteristic;
var
  Identity: Pointer;
  Created: Boolean;
begin
  Identity := simpleble_local_characteristic_identity(Handle);
  Lock.Enter;
  try
    Created := not FCharacteristics.TryGetValue(Identity, Result);
    if Created then
    begin
      Result := TBleLocalCharacteristic.CreateFromHandle(FPeripheral, Handle);
      FCharacteristics.Add(Identity, Result);
      FCharacteristicObjects.Add(Result);
    end;
  finally
    Lock.Leave;
  end;
  if Created then
    Result.SetCallbacks(True)
  else
    simpleble_local_characteristic_release_handle(Handle);
end;

function TBleLocalService.GetUuid: string;
var
  Call: TNativeCall;
  NativeUuid: simpleble_uuid_t;
begin
  Call.Enter;
  simpleble_local_service_uuid(FHandle, NativeUuid, Call.Error);
  Call.Leave;
  Result := FromUuid(NativeUuid);
end;

function TBleLocalService.AddCharacteristic(const Uuid: string;
  Capabilities: TBleLocalCapabilities): TBleLocalCharacteristic;
var
  Call: TNativeCall;
  NativeUuid: simpleble_uuid_t;
  Handle: simpleble_local_characteristic_t;
begin
  NativeUuid := ToUuid(Uuid);
  Call.Enter;
  Handle := simpleble_local_service_add_characteristic(FHandle, NativeUuid, CapabilityFlags(Capabilities),
    Call.Error);
  Call.Leave;
  Result := Intern(Handle);
end;

function TBleLocalService.Characteristics: TArray<TBleLocalCharacteristic>;
var
  Call: TNativeCall;
  List: simpleble_local_characteristic_list_t;
  Count: NativeUInt;
  I: Integer;
  Handle: simpleble_local_characteristic_t;
begin
  Call.Enter;
  Count := simpleble_local_service_characteristics(FHandle, List, Call.Error);
  Call.Leave;
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      Call.Enter;
      Handle := simpleble_local_characteristic_list_get(List, I, Call.Error);
      Call.Leave;
      Result[I] := Intern(Handle);
    end;
  finally
    simpleble_local_characteristic_list_release(List);
  end;
end;

{ TBleLocalCharacteristic }

constructor TBleLocalCharacteristic.CreateFromHandle(Peripheral: TBleLocalPeripheral; Handle: Pointer);
begin
  inherited Create;
  FHandle := Handle;
  FPeripheral := Peripheral;
end;

destructor TBleLocalCharacteristic.Destroy;
begin
  simpleble_local_characteristic_release_handle(FHandle);
  inherited;
end;

procedure TBleLocalCharacteristic.SetCallbacks(Enabled: Boolean);
begin
  if Enabled then
  begin
    simpleble_local_characteristic_set_callback_on_write(FHandle, LocalWriteCallback, Pointer(FToken));
    simpleble_local_characteristic_set_callback_on_subscribed(FHandle, LocalSubscribedCallback, Pointer(FToken));
    simpleble_local_characteristic_set_callback_on_unsubscribed(FHandle, LocalUnsubscribedCallback, Pointer(FToken));
  end
  else
  begin
    simpleble_local_characteristic_set_callback_on_write(FHandle, nil, nil);
    simpleble_local_characteristic_set_callback_on_subscribed(FHandle, nil, nil);
    simpleble_local_characteristic_set_callback_on_unsubscribed(FHandle, nil, nil);
  end;
end;

procedure TBleLocalCharacteristic.ClearCallbacks;
begin
  SetCallbacks(False);
  if Assigned(FOnReadRequest) then
    simpleble_local_characteristic_set_callback_on_read(FHandle, nil, nil);
end;

procedure TBleLocalCharacteristic.SetOnReadRequest(const Value: TBleLocalReadEvent);
begin
  FOnReadRequest := Value;
  if Assigned(Value) then
    simpleble_local_characteristic_set_callback_on_read(FHandle, LocalReadCallback, Pointer(FToken))
  else
    simpleble_local_characteristic_set_callback_on_read(FHandle, nil, nil);
end;

function TBleLocalCharacteristic.GetUuid: string;
var
  Call: TNativeCall;
  NativeUuid: simpleble_uuid_t;
begin
  Call.Enter;
  simpleble_local_characteristic_uuid(FHandle, NativeUuid, Call.Error);
  Call.Leave;
  Result := FromUuid(NativeUuid);
end;

function TBleLocalCharacteristic.GetCapabilities: TBleLocalCapabilities;
var
  Call: TNativeCall;
  Flags: UInt32;
  Capability: TBleLocalCapability;
begin
  Call.Enter;
  Flags := simpleble_local_characteristic_capabilities(FHandle, Call.Error);
  Call.Leave;
  Result := [];
  for Capability := Low(TBleLocalCapability) to High(TBleLocalCapability) do
    if Flags and (UInt32(1) shl Ord(Capability)) <> 0 then
      Include(Result, Capability);
end;

function TBleLocalCharacteristic.GetValue: TBytes;
var
  Call: TNativeCall;
  Data: PByte;
  Length: NativeUInt;
begin
  Call.Enter;
  Data := simpleble_local_characteristic_value(FHandle, Length, Call.Error);
  Call.Leave;
  Result := TakeBytes(Data, Length);
end;

procedure TBleLocalCharacteristic.SetValue(const Value: TBytes);
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_local_characteristic_set_value(FHandle, DataPointer(Value), Length(Value), Call.Error);
  Call.Leave;
end;
