// Part of SimpleDelphiBLE.pas: TBleAdapter.

{ TBleAdapter }

// Returns the one adapter object for a handle, taking ownership of the handle.
function InternAdapter(Handle: simpleble_adapter_t): TBleAdapter;
var
  Identity: Pointer;
  Created: Boolean;
begin
  Identity := simpleble_adapter_identity(Handle);
  Lock.Enter;
  try
    Created := not Adapters.TryGetValue(Identity, Result);
    if Created then
    begin
      Result := TBleAdapter.CreateFromHandle(Handle);
      Adapters.Add(Identity, Result);
    end;
  finally
    Lock.Leave;
  end;
  if Created then
    Result.SetCallbacks(True)
  else
    simpleble_adapter_release_handle(Handle);
end;

function TakeAdapters(List: simpleble_adapter_list_t; Count: NativeUInt): TArray<TBleAdapter>;
var
  Call: TNativeCall;
  I: Integer;
  Handle: simpleble_adapter_t;
begin
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      Call.Enter;
      Handle := simpleble_adapter_list_get(List, I, Call.Error);
      Call.Leave;
      Result[I] := InternAdapter(Handle);
    end;
  finally
    simpleble_adapter_list_release(List);
  end;
end;

function TBleBackend.GetAdapters: TArray<TBleAdapter>;
var
  Call: TNativeCall;
  List: simpleble_adapter_list_t;
  Count: NativeUInt;
begin
  Call.Enter;
  Count := simpleble_backend_get_adapters(FHandle, List, Call.Error);
  Call.Leave;
  Result := TakeAdapters(List, Count);
end;

constructor TBleAdapter.CreateFromHandle(Handle: Pointer);
begin
  inherited Create;
  FHandle := Handle;
  FPeripherals := TDictionary<Pointer, TBlePeripheral>.Create;
end;

destructor TBleAdapter.Destroy;
var
  Peripheral: TBlePeripheral;
begin
  if FPeripherals <> nil then
  begin
    for Peripheral in FPeripherals.Values do
      Peripheral.FreeOwned;
    FPeripherals.Free;
  end;
  simpleble_adapter_release_handle(FHandle);
  inherited;
end;

procedure TBleAdapter.SetCallbacks(Enabled: Boolean);
var
  Userdata: Pointer;
begin
  if Enabled then
  begin
    Userdata := Pointer(FToken);
    simpleble_adapter_set_callback_on_power_on(FHandle, AdapterPowerOnCallback, Userdata);
    simpleble_adapter_set_callback_on_power_off(FHandle, AdapterPowerOffCallback, Userdata);
    simpleble_adapter_set_callback_on_scan_start(FHandle, AdapterScanStartCallback, Userdata);
    simpleble_adapter_set_callback_on_scan_stop(FHandle, AdapterScanStopCallback, Userdata);
    simpleble_adapter_set_callback_on_scan_found(FHandle, AdapterScanFoundCallback, Userdata);
    simpleble_adapter_set_callback_on_scan_updated(FHandle, AdapterScanUpdatedCallback, Userdata);
  end
  else
  begin
    simpleble_adapter_set_callback_on_power_on(FHandle, nil, nil);
    simpleble_adapter_set_callback_on_power_off(FHandle, nil, nil);
    simpleble_adapter_set_callback_on_scan_start(FHandle, nil, nil);
    simpleble_adapter_set_callback_on_scan_stop(FHandle, nil, nil);
    simpleble_adapter_set_callback_on_scan_found(FHandle, nil, nil);
    simpleble_adapter_set_callback_on_scan_updated(FHandle, nil, nil);
  end;
end;

procedure TBleAdapter.ClearCallbacks;
{$IFDEF SIMPLEDELPHIBLE_LINUX}
var
  Error: psimpleble_error_t;
{$ENDIF}
begin
  SetCallbacks(False);
{$IFDEF SIMPLEDELPHIBLE_LINUX}
  // NOTE: Only a BlueZ adapter accepts pairing callbacks, so only one with a handler has some to clear.
  Error := nil;
  if Assigned(FOnPasskeyRequest) then
    simpleble_advanced_linux_set_passkey_request_callback(FHandle, nil, nil, Error);
  if Assigned(FOnPasskeyDisplay) then
    simpleble_advanced_linux_set_passkey_display_callback(FHandle, nil, nil, Error);
  if Assigned(FOnNumericComparison) then
    simpleble_advanced_linux_set_numeric_comparison_callback(FHandle, nil, nil, Error);
  if Assigned(FOnPairingAuthorization) then
    simpleble_advanced_linux_set_pairing_authorization_callback(FHandle, nil, nil, Error);
  simpleble_error_release(Error);
{$ENDIF}
end;

// Returns the one peripheral object for a handle, taking ownership of the handle.
function TBleAdapter.Intern(Handle: Pointer): TBlePeripheral;
var
  Identity: Pointer;
  Created: Boolean;
begin
  Identity := simpleble_peripheral_identity(Handle);
  Lock.Enter;
  try
    Created := not FPeripherals.TryGetValue(Identity, Result);
    if Created then
    begin
      Result := TBlePeripheral.CreateFromHandle(Handle);
      FPeripherals.Add(Identity, Result);
    end;
  finally
    Lock.Leave;
  end;
  if Created then
    Result.SetCallbacks(True)
  else
    simpleble_peripheral_release_handle(Handle);
end;

function TBleAdapter.TakePeripherals(List: Pointer; Count: NativeUInt): TArray<TBlePeripheral>;
var
  Call: TNativeCall;
  I: Integer;
  Handle: simpleble_peripheral_t;
begin
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      Call.Enter;
      Handle := simpleble_peripheral_list_get(List, I, Call.Error);
      Call.Leave;
      Result[I] := Intern(Handle);
    end;
  finally
    simpleble_peripheral_list_release(List);
  end;
end;

class function TBleAdapter.IsBluetoothEnabled: Boolean;
var
  Call: TNativeCall;
begin
  RequireLibrary;
  Call.Enter;
  Result := simpleble_adapter_is_bluetooth_enabled(Call.Error);
  Call.Leave;
end;

class function TBleAdapter.GetAdapters: TArray<TBleAdapter>;
var
  Call: TNativeCall;
  List: simpleble_adapter_list_t;
  Count: NativeUInt;
begin
  RequireLibrary;
  Call.Enter;
  Count := simpleble_adapter_get_adapters(List, Call.Error);
  Call.Leave;
  Result := TakeAdapters(List, Count);
end;

function TBleAdapter.GetIdentifier: string;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := TakeString(simpleble_adapter_identifier(FHandle, Call.Error));
  Call.Leave;
end;

function TBleAdapter.GetAddress: string;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := TakeString(simpleble_adapter_address(FHandle, Call.Error));
  Call.Leave;
end;

function TBleAdapter.IsPowered: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_adapter_is_powered(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBleAdapter.PowerOn;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_adapter_power_on(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBleAdapter.PowerOff;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_adapter_power_off(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBleAdapter.ScanStart;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_adapter_scan_start(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBleAdapter.ScanStop;
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_adapter_scan_stop(FHandle, Call.Error);
  Call.Leave;
end;

procedure TBleAdapter.ScanFor(TimeoutMs: Integer);
var
  Call: TNativeCall;
begin
  Call.Enter;
  simpleble_adapter_scan_for(FHandle, TimeoutMs, Call.Error);
  Call.Leave;
end;

function TBleAdapter.ScanIsActive: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_adapter_scan_is_active(FHandle, Call.Error);
  Call.Leave;
end;

function TBleAdapter.ScanGetResults: TArray<TBlePeripheral>;
var
  Call: TNativeCall;
  List: simpleble_peripheral_list_t;
  Count: NativeUInt;
begin
  Call.Enter;
  Count := simpleble_adapter_scan_get_results(FHandle, List, Call.Error);
  Call.Leave;
  Result := TakePeripherals(List, Count);
end;

function TBleAdapter.GetPairedPeripherals: TArray<TBlePeripheral>;
var
  Call: TNativeCall;
  List: simpleble_peripheral_list_t;
  Count: NativeUInt;
begin
  Call.Enter;
  Count := simpleble_adapter_get_paired_peripherals(FHandle, List, Call.Error);
  Call.Leave;
  Result := TakePeripherals(List, Count);
end;

function TBleAdapter.GetConnectedPeripherals: TArray<TBlePeripheral>;
var
  Call: TNativeCall;
  List: simpleble_peripheral_list_t;
  Count: NativeUInt;
begin
  Call.Enter;
  Count := simpleble_adapter_get_connected_peripherals(FHandle, List, Call.Error);
  Call.Leave;
  Result := TakePeripherals(List, Count);
end;

function TBleAdapter.RetrieveCachedPeripherals(const Identifiers: array of string): TArray<TBlePeripheral>;
{$IFDEF SIMPLEDELPHIBLE_MACOS}
var
  Call: TNativeCall;
  I, Count: Integer;
  Bytes: RawByteString;
  Handle: simpleble_peripheral_t;
begin
  Result := nil;
  SetLength(Result, Length(Identifiers));
  Count := 0;
  for I := 0 to High(Identifiers) do
  begin
    Bytes := UTF8Encode(Identifiers[I]);
    Call.Enter;
    Handle := simpleble_advanced_macos_retrieve_cached_peripheral(FHandle, PAnsiChar(Bytes), Call.Error);
    Call.Leave;
    // NOTE: An unknown identifier returns no peripheral and no error.
    if Handle <> nil then
    begin
      Result[Count] := Intern(Handle);
      Inc(Count);
    end;
  end;
  SetLength(Result, Count);
end;
{$ELSE}
begin
  Result := nil;
  raise EBleOperationNotSupported.Create(ecOperationNotSupported, 'Retrieving cached peripherals requires macOS');
end;
{$ENDIF}

{$IFDEF SIMPLEDELPHIBLE_LINUX}
procedure TBleAdapter.SetOnPasskeyRequest(const Value: TBleAdapterPasskeyRequestEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_linux_set_passkey_request_callback(FHandle, AdapterPasskeyRequestCallback, Pointer(FToken),
      Call.Error)
  else
    simpleble_advanced_linux_set_passkey_request_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnPasskeyRequest := Value;
end;

procedure TBleAdapter.SetOnPasskeyDisplay(const Value: TBleAdapterPasskeyDisplayEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_linux_set_passkey_display_callback(FHandle, AdapterPasskeyDisplayCallback, Pointer(FToken),
      Call.Error)
  else
    simpleble_advanced_linux_set_passkey_display_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnPasskeyDisplay := Value;
end;

procedure TBleAdapter.SetOnNumericComparison(const Value: TBleAdapterNumericComparisonEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_linux_set_numeric_comparison_callback(FHandle, AdapterNumericComparisonCallback,
      Pointer(FToken), Call.Error)
  else
    simpleble_advanced_linux_set_numeric_comparison_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnNumericComparison := Value;
end;

procedure TBleAdapter.SetOnPairingAuthorization(const Value: TBleAdapterPairingAuthorizationEvent);
var
  Call: TNativeCall;
begin
  Call.Enter;
  if Assigned(Value) then
    simpleble_advanced_linux_set_pairing_authorization_callback(FHandle, AdapterPairingAuthorizationCallback,
      Pointer(FToken), Call.Error)
  else
    simpleble_advanced_linux_set_pairing_authorization_callback(FHandle, nil, nil, Call.Error);
  Call.Leave;
  FOnPairingAuthorization := Value;
end;
{$ELSE}
procedure RaisePairingNotSupported;
begin
  raise EBleOperationNotSupported.Create(ecOperationNotSupported, 'Adapter pairing handlers require Linux');
end;

procedure TBleAdapter.SetOnPasskeyRequest(const Value: TBleAdapterPasskeyRequestEvent);
begin
  RaisePairingNotSupported;
end;

procedure TBleAdapter.SetOnPasskeyDisplay(const Value: TBleAdapterPasskeyDisplayEvent);
begin
  RaisePairingNotSupported;
end;

procedure TBleAdapter.SetOnNumericComparison(const Value: TBleAdapterNumericComparisonEvent);
begin
  RaisePairingNotSupported;
end;

procedure TBleAdapter.SetOnPairingAuthorization(const Value: TBleAdapterPairingAuthorizationEvent);
begin
  RaisePairingNotSupported;
end;
{$ENDIF}

function TBleAdapter.CreateLocalPeripheral: TBleLocalPeripheral;
var
  Call: TNativeCall;
  Handle: simpleble_local_peripheral_t;
begin
  Call.Enter;
  Handle := simpleble_adapter_create_local_peripheral(FHandle, Call.Error);
  Call.Leave;
  Result := TBleLocalPeripheral.CreateFromHandle(Handle);
  Result.SetCallbacks(True);
end;
