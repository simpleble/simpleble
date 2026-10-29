// Part of SimpleDelphiBLE.pas: the Async methods, which run a blocking call on a worker thread.

{ TBleAsyncWorker }

constructor TBleAsyncWorker.Create(Call: TBleAsyncCall; Target: TBleNativeObject);
begin
  inherited Create(True);
  FreeOnTerminate := True;
  FCall := Call;
  FTarget := Target;
end;

procedure TBleAsyncWorker.Execute;
var
  Event: TBleEvent;
  Peripheral: TBlePeripheral;
begin
  // NOTE: The worker counts as a callback, so the unit's state outlives the call it makes.
  if not EnterCallback then
    Exit;
  try
    Event := Default(TBleEvent);
    Event.Kind := ekDone;
    Event.Target := FTarget;
    Event.OnDone := FOnDone;
    Event.OnReadDone := FOnReadDone;
    Peripheral := TBlePeripheral(FTarget);
    try
      case FCall of
        acScanFor: TBleAdapter(FTarget).ScanFor(FTimeoutMs);
        acConnect: Peripheral.Connect;
        acDisconnect: Peripheral.Disconnect;
        acRead: Event.Data := Peripheral.Read(FService, FCharacteristic);
        acWriteRequest: Peripheral.WriteRequest(FService, FCharacteristic, FData);
        acWriteCommand: Peripheral.WriteCommand(FService, FCharacteristic, FData);
        acNotify: Peripheral.Notify(FService, FCharacteristic, FOnValue);
        acIndicate: Peripheral.Indicate(FService, FCharacteristic, FOnValue);
        acUnsubscribe: Peripheral.Unsubscribe(FService, FCharacteristic);
        acReadDescriptor: Event.Data := Peripheral.ReadDescriptor(FService, FCharacteristic, FDescriptor);
        acWriteDescriptor: Peripheral.WriteDescriptor(FService, FCharacteristic, FDescriptor, FData);
        acSecureConnection: Peripheral.SecureConnection(FMitm);
      end;
    except
      on Exception do
        Event.Error := Exception(AcquireExceptionObject);
    end;
    if Assigned(Event.OnDone) or Assigned(Event.OnReadDone) then
      Dispatcher.Post(Event)
    else
      Event.Error.Free;
  finally
    LeaveCallback;
  end;
end;

{ Async methods }

procedure TBleAdapter.ScanForAsync(TimeoutMs: Integer; OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := TBleAsyncWorker.Create(acScanFor, Self);
  Worker.FTimeoutMs := TimeoutMs;
  Worker.FOnDone := OnDone;
  Worker.Start;
end;

function NewPeripheralWorker(Call: TBleAsyncCall; Peripheral: TBlePeripheral; const Service,
  Characteristic: string; OnDone: TBleCompletionEvent): TBleAsyncWorker;
begin
  Result := TBleAsyncWorker.Create(Call, Peripheral);
  Result.FService := Service;
  Result.FCharacteristic := Characteristic;
  Result.FOnDone := OnDone;
end;

procedure TBlePeripheral.ConnectAsync(OnDone: TBleCompletionEvent);
begin
  NewPeripheralWorker(acConnect, Self, '', '', OnDone).Start;
end;

procedure TBlePeripheral.DisconnectAsync(OnDone: TBleCompletionEvent);
begin
  NewPeripheralWorker(acDisconnect, Self, '', '', OnDone).Start;
end;

procedure TBlePeripheral.ReadAsync(const Service, Characteristic: string; OnDone: TBleReadCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acRead, Self, Service, Characteristic, nil);
  Worker.FOnReadDone := OnDone;
  Worker.Start;
end;

procedure TBlePeripheral.WriteRequestAsync(const Service, Characteristic: string; const Data: TBytes;
  OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acWriteRequest, Self, Service, Characteristic, OnDone);
  Worker.FData := Copy(Data);
  Worker.Start;
end;

procedure TBlePeripheral.WriteCommandAsync(const Service, Characteristic: string; const Data: TBytes;
  OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acWriteCommand, Self, Service, Characteristic, OnDone);
  Worker.FData := Copy(Data);
  Worker.Start;
end;

procedure TBlePeripheral.NotifyAsync(const Service, Characteristic: string; OnValue: TBleValueEvent;
  OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acNotify, Self, Service, Characteristic, OnDone);
  Worker.FOnValue := OnValue;
  Worker.Start;
end;

procedure TBlePeripheral.IndicateAsync(const Service, Characteristic: string; OnValue: TBleValueEvent;
  OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acIndicate, Self, Service, Characteristic, OnDone);
  Worker.FOnValue := OnValue;
  Worker.Start;
end;

procedure TBlePeripheral.UnsubscribeAsync(const Service, Characteristic: string; OnDone: TBleCompletionEvent);
begin
  NewPeripheralWorker(acUnsubscribe, Self, Service, Characteristic, OnDone).Start;
end;

procedure TBlePeripheral.ReadDescriptorAsync(const Service, Characteristic, Descriptor: string;
  OnDone: TBleReadCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acReadDescriptor, Self, Service, Characteristic, nil);
  Worker.FDescriptor := Descriptor;
  Worker.FOnReadDone := OnDone;
  Worker.Start;
end;

procedure TBlePeripheral.WriteDescriptorAsync(const Service, Characteristic, Descriptor: string; const Data: TBytes;
  OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acWriteDescriptor, Self, Service, Characteristic, OnDone);
  Worker.FDescriptor := Descriptor;
  Worker.FData := Copy(Data);
  Worker.Start;
end;

procedure TBlePeripheral.SecureConnectionAsync(Mitm: Boolean; OnDone: TBleCompletionEvent);
var
  Worker: TBleAsyncWorker;
begin
  Worker := NewPeripheralWorker(acSecureConnection, Self, '', '', OnDone);
  Worker.FMitm := Mitm;
  Worker.Start;
end;
