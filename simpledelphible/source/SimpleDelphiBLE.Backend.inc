// Part of SimpleDelphiBLE.pas: TBleBackend.

{ TBleBackend }

// Returns the one backend object for a handle, taking ownership of the handle.
function InternBackend(Handle: simpleble_backend_t): TBleBackend;
var
  Call: TNativeCall;
  Identifier: string;
  Created: Boolean;
begin
  try
    Call.Enter;
    Identifier := TakeString(simpleble_backend_identifier(Handle, Call.Error));
    Call.Leave;
  except
    simpleble_backend_release_handle(Handle);
    raise;
  end;
  Lock.Enter;
  try
    Created := not Backends.TryGetValue(Identifier, Result);
    if Created then
    begin
      Result := TBleBackend.CreateFromHandle(Handle, Identifier);
      Backends.Add(Identifier, Result);
    end;
  finally
    Lock.Leave;
  end;
  if not Created then
    simpleble_backend_release_handle(Handle);
end;

constructor TBleBackend.CreateFromHandle(Handle: Pointer; const Identifier: string);
begin
  inherited Create;
  FHandle := Handle;
  FIdentifier := Identifier;
end;

destructor TBleBackend.Destroy;
begin
  simpleble_backend_release_handle(FHandle);
  inherited;
end;

class function TBleBackend.GetBackends: TArray<TBleBackend>;
var
  Call: TNativeCall;
  List: simpleble_backend_list_t;
  Count: NativeUInt;
  I: Integer;
  Handle: simpleble_backend_t;
begin
  Call.Enter;
  Count := simpleble_backend_get_backends(List, Call.Error);
  Call.Leave;
  try
    Result := nil;
    SetLength(Result, Count);
    for I := 0 to Integer(Count) - 1 do
    begin
      Call.Enter;
      Handle := simpleble_backend_list_get(List, I, Call.Error);
      Call.Leave;
      Result[I] := InternBackend(Handle);
    end;
  finally
    simpleble_backend_list_release(List);
  end;
end;

class function TBleBackend.GetNative: TBleBackend;
var
  Call: TNativeCall;
  Handle: simpleble_backend_t;
begin
  Call.Enter;
  Handle := simpleble_backend_get_native(Call.Error);
  Call.Leave;
  Result := InternBackend(Handle);
end;

class function TBleBackend.GetDongl: TBleBackend;
var
  Call: TNativeCall;
  Handle: simpleble_backend_t;
begin
  Call.Enter;
  Handle := simpleble_backend_get_dongl(Call.Error);
  Call.Leave;
  Result := InternBackend(Handle);
end;

class function TBleBackend.GetSimulator: TBleBackend;
var
  Call: TNativeCall;
  Handle: simpleble_backend_t;
begin
  Call.Enter;
  Handle := simpleble_backend_get_simulator(Call.Error);
  Call.Leave;
  Result := InternBackend(Handle);
end;

function TBleBackend.IsBluetoothEnabled: Boolean;
var
  Call: TNativeCall;
begin
  Call.Enter;
  Result := simpleble_backend_is_bluetooth_enabled(FHandle, Call.Error);
  Call.Leave;
end;
