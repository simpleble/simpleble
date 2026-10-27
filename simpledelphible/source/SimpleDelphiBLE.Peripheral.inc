// Part of SimpleDelphiBLE.pas: TBlePeripheral.

{ TBlePeripheral }

constructor TBlePeripheral.CreateFromHandle(Handle: Pointer);
begin
  inherited Create;
  FHandle := Handle;
  FSubscriptions := TDictionary<string, TBleValueEvent>.Create;
end;

destructor TBlePeripheral.Destroy;
begin
  FSubscriptions.Free;
  simpleble_peripheral_release_handle(FHandle);
  inherited;
end;

procedure TBlePeripheral.SetCallbacks(Enabled: Boolean);
begin
  if Enabled then
  begin
    simpleble_peripheral_set_callback_on_connected(FHandle, PeripheralConnectedCallback, Pointer(FToken));
    simpleble_peripheral_set_callback_on_disconnected(FHandle, PeripheralDisconnectedCallback, Pointer(FToken));
  end
  else
  begin
    simpleble_peripheral_set_callback_on_connected(FHandle, nil, nil);
    simpleble_peripheral_set_callback_on_disconnected(FHandle, nil, nil);
  end;
end;

procedure TBlePeripheral.ClearCallbacks;
var
  Error: psimpleble_error_t;
begin
  SetCallbacks(False);
  // NOTE: Only a Dongl peripheral accepts pairing callbacks, so only one with a handler has some to clear.
  Error := nil;
  if Assigned(FOnPasskeyRequest) then
    simpleble_advanced_dongl_set_passkey_request_callback(FHandle, nil, nil, Error);
  if Assigned(FOnPasskeyDisplay) then
    simpleble_advanced_dongl_set_passkey_display_callback(FHandle, nil, nil, Error);
  if Assigned(FOnNumericComparison) then
    simpleble_advanced_dongl_set_numeric_comparison_callback(FHandle, nil, nil, Error);
  simpleble_error_release(Error);
end;

function TBlePeripheral.GetIdentifier: string;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := TakeString(simpleble_peripheral_identifier(FHandle, Call.Error));
  Call.Leave;
end;

function TBlePeripheral.GetAddress: string;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := TakeString(simpleble_peripheral_address(FHandle, Call.Error));
  Call.Leave;
end;

function TBlePeripheral.GetAddressType: TBleAddressType;
var
  Call: TNativeCall;
  Value: simpleble_address_type_t;
begin
  Call.Enter;
  Value := simpleble_peripheral_address_type(FHandle, Call.Error);
  Call.Leave;
  if (Value >= Ord(Low(TBleAddressType))) and (Value <= Ord(High(TBleAddressType))) then
    Result := TBleAddressType(Value)
  else
    Result := atUnspecified;
end;

function TBlePeripheral.GetRssi: SmallInt;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_peripheral_rssi(FHandle, Call.Error);
  Call.Leave;
end;

function TBlePeripheral.GetTxPower: SmallInt;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_peripheral_tx_power(FHandle, Call.Error);
  Call.Leave;
end;

function TBlePeripheral.GetMtu: Word;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_peripheral_mtu(FHandle, Call.Error);
  Call.Leave;
end;

function TBlePeripheral.GetIsConnected: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_peripheral_is_connected(FHandle, Call.Error);
  Call.Leave;
end;

function TBlePeripheral.GetIsConnectable: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_peripheral_is_connectable(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBlePeripheral.Connect;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_peripheral_connect(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBlePeripheral.Disconnect;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_peripheral_disconnect(FHandle, Call.Error);
  Call.Leave;
end;

function TBlePeripheral.IsPaired: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_peripheral_is_paired(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBlePeripheral.Unpair;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_peripheral_unpair(FHandle, Call.Error);
  Call.Leave;
end;

function ConvertService(const Native: simpleble_service_t): TBleService;
var
  Characteristic: psimpleble_characteristic_t;
  Descriptor: psimpleble_descriptor_t;
  I: Integer;
  J: Integer;
begin
  Result.Uuid := FromUuid(Native.uuid);
  Result.Data := CopyBytes(Native.data, Native.data_length);
  Result.Characteristics := nil;
  SetLength(Result.Characteristics, Native.characteristic_count);
  Characteristic := Native.characteristics;
  for I := 0 to High(Result.Characteristics) do
  begin
    Result.Characteristics[I].Uuid := FromUuid(Characteristic^.uuid);
    Result.Characteristics[I].CanRead := Characteristic^.can_read;
    Result.Characteristics[I].CanWriteRequest := Characteristic^.can_write_request;
    Result.Characteristics[I].CanWriteCommand := Characteristic^.can_write_command;
    Result.Characteristics[I].CanNotify := Characteristic^.can_notify;
    Result.Characteristics[I].CanIndicate := Characteristic^.can_indicate;
    SetLength(Result.Characteristics[I].Descriptors, Characteristic^.descriptor_count);
    Descriptor := Characteristic^.descriptors;
    for J := 0 to High(Result.Characteristics[I].Descriptors) do
    begin
      Result.Characteristics[I].Descriptors[J] := FromUuid(Descriptor^.uuid);
      Inc(Descriptor);
    end;
    Inc(Characteristic);
  end;
end;

function TBlePeripheral.Services: TArray<TBleService>;
var
  Call: TNativeCall;
  List: simpleble_service_list_t;
  Count: NativeUInt;
  I: Integer;
  Item: simpleble_service_t;
begin
  Call.Enter;
  Count := simpleble_peripheral_services(FHandle, List, Call.Error);
  Call.Leave;
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      FillChar(Item, SizeOf(Item), 0);
      Call.Enter;
      simpleble_service_list_get(List, I, Item, Call.Error);
      Call.Leave;
      try
        Result[I] := ConvertService(Item);
      finally
        simpleble_service_release(Item);
      end;
    end;
  finally
    simpleble_service_list_release(List);
  end;
end;

function TBlePeripheral.ManufacturerData: TArray<TBleManufacturerData>;
var
  Call: TNativeCall;
  List: simpleble_manufacturer_data_list_t;
  Count: NativeUInt;
  I: Integer;
  Item: simpleble_manufacturer_data_t;
begin
  Call.Enter;
  Count := simpleble_peripheral_manufacturer_data(FHandle, List, Call.Error);
  Call.Leave;
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      FillChar(Item, SizeOf(Item), 0);
      Call.Enter;
      simpleble_manufacturer_data_list_get(List, I, Item, Call.Error);
      Call.Leave;
      Result[I].CompanyId := Item.manufacturer_id;
      Result[I].Data := CopyBytes(Item.data, Item.data_length);
      simpleble_manufacturer_data_release(Item);
    end;
  finally
    simpleble_manufacturer_data_list_release(List);
  end;
end;

function TBlePeripheral.Read(const Service, Characteristic: string): TBytes;
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
  Data: PByte;
  Length: NativeUInt;
begin
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  Call.Enter;
  Data := simpleble_peripheral_read(FHandle, ServiceUuid, CharacteristicUuid, Length, Call.Error);
  Call.Leave;
  Result := TakeBytes(Data, Length);
end;

procedure TBlePeripheral.WriteRequest(const Service, Characteristic: string; const Data: TBytes);
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
begin
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  Call.Enter;
  simpleble_peripheral_write_request(FHandle, ServiceUuid, CharacteristicUuid, DataPointer(Data), Length(Data),
    Call.Error);
  Call.Leave;
end;

procedure TBlePeripheral.WriteCommand(const Service, Characteristic: string; const Data: TBytes);
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
begin
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  Call.Enter;
  simpleble_peripheral_write_command(FHandle, ServiceUuid, CharacteristicUuid, DataPointer(Data), Length(Data),
    Call.Error);
  Call.Leave;
end;

procedure TBlePeripheral.Subscribe(Indicate: Boolean; const Service, Characteristic: string;
  OnValue: TBleValueEvent);
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
  Key: string;
  Previous: TBleValueEvent;
  HadPrevious: Boolean;
begin
  if not Assigned(OnValue) then
    raise EBleInvalidArgument.Create(ecInvalidArgument, 'OnValue is nil');
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  Key := SubscriptionKey(Service, Characteristic);
  Lock.Enter;
  try
    HadPrevious := FSubscriptions.TryGetValue(Key, Previous);
    FSubscriptions.AddOrSetValue(Key, OnValue);
  finally
    Lock.Leave;
  end;
  try
    Call.Enter;
    if Indicate then
      simpleble_peripheral_indicate(FHandle, ServiceUuid, CharacteristicUuid, PeripheralValueCallback,
        Pointer(FToken), Call.Error)
    else
      simpleble_peripheral_notify(FHandle, ServiceUuid, CharacteristicUuid, PeripheralValueCallback,
        Pointer(FToken), Call.Error);
    Call.Leave;
  except
    Lock.Enter;
    try
      if HadPrevious then
        FSubscriptions.AddOrSetValue(Key, Previous)
      else
        FSubscriptions.Remove(Key);
    finally
      Lock.Leave;
    end;
    raise;
  end;
end;

procedure TBlePeripheral.Notify(const Service, Characteristic: string; OnValue: TBleValueEvent);
begin
  Subscribe(False, Service, Characteristic, OnValue);
end;

procedure TBlePeripheral.Indicate(const Service, Characteristic: string; OnValue: TBleValueEvent);
begin
  Subscribe(True, Service, Characteristic, OnValue);
end;

procedure TBlePeripheral.Unsubscribe(const Service, Characteristic: string);
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
begin
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  try
    Call.Enter;
    simpleble_peripheral_unsubscribe(FHandle, ServiceUuid, CharacteristicUuid, Call.Error);
    Call.Leave;
  finally
    Lock.Enter;
    try
      FSubscriptions.Remove(SubscriptionKey(Service, Characteristic));
    finally
      Lock.Leave;
    end;
  end;
end;

function TBlePeripheral.ReadDescriptor(const Service, Characteristic, Descriptor: string): TBytes;
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
  DescriptorUuid: simpleble_uuid_t;
  Data: PByte;
  Length: NativeUInt;
begin
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  DescriptorUuid := ToUuid(Descriptor);
  Call.Enter;
  Data := simpleble_peripheral_read_descriptor(FHandle, ServiceUuid, CharacteristicUuid, DescriptorUuid, Length,
    Call.Error);
  Call.Leave;
  Result := TakeBytes(Data, Length);
end;

procedure TBlePeripheral.WriteDescriptor(const Service, Characteristic, Descriptor: string; const Data: TBytes);
var
  Call: TNativeCall;
  ServiceUuid: simpleble_uuid_t;
  CharacteristicUuid: simpleble_uuid_t;
  DescriptorUuid: simpleble_uuid_t;
begin
  ServiceUuid := ToUuid(Service);
  CharacteristicUuid := ToUuid(Characteristic);
  DescriptorUuid := ToUuid(Descriptor);
  Call.Enter;
  simpleble_peripheral_write_descriptor(FHandle, ServiceUuid, CharacteristicUuid, DescriptorUuid, DataPointer(Data),
    Length(Data), Call.Error);
  Call.Leave;
end;

procedure TBlePeripheral.SetOnPasskeyRequest(const Value: TBlePasskeyRequestEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_dongl_set_passkey_request_callback(FHandle, PasskeyRequestCallback, Pointer(FToken),
      Call.Error)
  else
    simpleble_advanced_dongl_set_passkey_request_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnPasskeyRequest := Value;
end;

procedure TBlePeripheral.SetOnPasskeyDisplay(const Value: TBlePasskeyDisplayEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_dongl_set_passkey_display_callback(FHandle, PasskeyDisplayCallback, Pointer(FToken),
      Call.Error)
  else
    simpleble_advanced_dongl_set_passkey_display_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnPasskeyDisplay := Value;
end;

procedure TBlePeripheral.SetOnNumericComparison(const Value: TBleNumericComparisonEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_dongl_set_numeric_comparison_callback(FHandle, NumericComparisonCallback, Pointer(FToken),
      Call.Error)
  else
    simpleble_advanced_dongl_set_numeric_comparison_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnNumericComparison := Value;
end;

procedure TBlePeripheral.SecureConnection(Mitm: Boolean);
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_advanced_dongl_secure_connection(FHandle, Mitm, Call.Error);
  Call.Leave;
end;
