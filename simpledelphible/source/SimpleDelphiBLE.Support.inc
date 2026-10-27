// Part of SimpleDelphiBLE.pas: conversions, atomics and errors.

{ Conversions }

function FromUtf8(Text: PAnsiChar): string;
begin
{$IFDEF FPC}
  Result := UTF8Decode(RawByteString(Text));
{$ELSE}
  Result := UTF8ToString(Text);
{$ENDIF}
end;

// Converts a string that SimpleCBLE allocated, and frees it.
function TakeString(Text: PAnsiChar): string;
begin
  Result := FromUtf8(Text);
  simpleble_free(Text);
end;

function CopyBytes(Data: PByte; Length: NativeUInt): TBytes;
begin
  Result := nil;
  SetLength(Result, Length);
  if Length > 0 then
    Move(Data^, Result[0], Length);
end;

// Converts a buffer that SimpleCBLE allocated, and frees it.
function TakeBytes(Data: PByte; Length: NativeUInt): TBytes;
begin
  Result := CopyBytes(Data, Length);
  simpleble_free(Data);
end;

function DataPointer(const Data: TBytes): PByte;
begin
  if Length(Data) = 0 then
    Result := @EmptyByte
  else
    Result := @Data[0];
end;

function ToUuid(const Value: string): simpleble_uuid_t;
var
  Bytes: RawByteString;
begin
  Bytes := UTF8Encode(Value);
  if (Length(Bytes) >= SIMPLEBLE_UUID_STR_LEN) or (Pos(#0, Value) > 0) then
    raise EBleInvalidArgument.Create(ecInvalidArgument, 'Not a UUID: ' + Value);
  FillChar(Result, SizeOf(Result), 0);
  if Length(Bytes) > 0 then
    Move(Bytes[1], Result.value[0], Length(Bytes));
end;

function FromUuid(const Uuid: simpleble_uuid_t): string;
begin
  Result := FromUtf8(PAnsiChar(@Uuid.value[0]));
end;

function SubscriptionKey(const Service, Characteristic: string): string;
begin
  Result := Service + '/' + Characteristic;
end;

function CapabilityFlags(Capabilities: TBleLocalCapabilities): UInt32;
var
  Capability: TBleLocalCapability;
begin
  Result := 0;
  for Capability in Capabilities do
    Result := Result or (UInt32(1) shl Ord(Capability));
end;

{ Atomics }

procedure AtomicAdd(var Target: Integer; Amount: Integer);
begin
{$IFDEF FPC}
  InterLockedExchangeAdd(Target, Amount);
{$ELSE}
  AtomicIncrement(Target, Amount);
{$ENDIF}
end;

// Called by every native callback before it touches the unit's state, and paired with LeaveCallback when it returns
// True.
function EnterCallback: Boolean;
begin
  AtomicAdd(InFlight, 1);
  Result := ShuttingDown = 0;
  if not Result then
    AtomicAdd(InFlight, -1);
end;

procedure LeaveCallback;
begin
  AtomicAdd(InFlight, -1);
end;

{ Errors }

constructor EBleError.Create(Code: TBleErrorCode; const Msg: string);
begin
  // NOTE: Free Pascal's Exception keeps its message as an AnsiString.
  inherited Create({$IFDEF FPC}AnsiString(Msg){$ELSE}Msg{$ENDIF});
  FCode := Code;
end;

procedure RaiseError(var Error: psimpleble_error_t);
var
  Code: Int32;
  ErrorCode: TBleErrorCode;
  Message: string;
begin
  Code := simpleble_error_code(Error);
  Message := FromUtf8(simpleble_error_message(Error));
  simpleble_error_release(Error);
  if (Code >= Ord(Low(TBleErrorCode))) and (Code <= Ord(High(TBleErrorCode))) then
    ErrorCode := TBleErrorCode(Code)
  else
    ErrorCode := ecUnclassifiedException;
  raise ErrorClasses[ErrorCode].Create(ErrorCode, Message);
end;

procedure TNativeCall.Enter;
begin
  Error := nil;
  Mask := GetExceptionMask;
  SetExceptionMask(exAllArithmeticExceptions);
end;

procedure TNativeCall.Leave;
begin
  SetExceptionMask(Mask);
  if Error <> nil then
    RaiseError(Error);
end;
