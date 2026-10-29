// Part of SimpleDelphiBLE.pas: delivering events, and the dispatcher that queues them.

{ Events }

procedure ReportHandlerException(E: Exception);
var
  Handler: TBleHandlerExceptionEvent;
begin
  Handler := TSimpleDelphiBLE.FOnHandlerException;
  try
    if Assigned(Handler) then
      Handler(E)
    else if (TThread.CurrentThread.ThreadID = MainThreadID) and Assigned(ApplicationHandleException) then
      ApplicationHandleException(nil)
    else
      ShowException(E, ExceptAddr);
  except
    // NOTE: An exception from the reporting handler has nowhere left to go.
  end;
end;

procedure DeliverAdapterEvent(Adapter: TBleAdapter; const Event: TBleEvent);
var
  Handler: TBleAdapterEvent;
  ScanHandler: TBleScanEvent;
begin
  Handler := nil;
  ScanHandler := nil;
  case Event.Kind of
    ekPowerOn: Handler := Adapter.FOnPowerOn;
    ekPowerOff: Handler := Adapter.FOnPowerOff;
    ekScanStart: Handler := Adapter.FOnScanStart;
    ekScanStop: Handler := Adapter.FOnScanStop;
    ekScanFound: ScanHandler := Adapter.FOnScanFound;
    ekScanUpdated: ScanHandler := Adapter.FOnScanUpdated;
  end;
  if Assigned(Handler) then
    Handler(Adapter);
  if Assigned(ScanHandler) then
    ScanHandler(Adapter, Event.Peripheral);
end;

procedure DeliverPeripheralEvent(Peripheral: TBlePeripheral; const Event: TBleEvent);
var
  Handler: TBlePeripheralEvent;
  ValueHandler: TBleValueEvent;
  DisplayHandler: TBlePasskeyDisplayEvent;
begin
  Handler := nil;
  ValueHandler := nil;
  DisplayHandler := nil;
  case Event.Kind of
    ekConnected: Handler := Peripheral.FOnConnected;
    ekDisconnected: Handler := Peripheral.FOnDisconnected;
    ekPasskeyDisplay: DisplayHandler := Peripheral.FOnPasskeyDisplay;
    ekValue:
      begin
        // NOTE: A value that arrives after Unsubscribe finds no handler and is dropped.
        Lock.Enter;
        try
          if not Peripheral.FSubscriptions.TryGetValue(SubscriptionKey(Event.Service, Event.Characteristic),
            ValueHandler) then
            ValueHandler := nil;
        finally
          Lock.Leave;
        end;
      end;
  end;
  if Assigned(Handler) then
    Handler(Peripheral);
  if Assigned(ValueHandler) then
    ValueHandler(Peripheral, Event.Service, Event.Characteristic, Event.Data);
  if Assigned(DisplayHandler) then
    DisplayHandler(Peripheral, Event.Text);
end;

procedure DeliverLocalEvent(const Event: TBleEvent);
var
  ClientHandler: TBleClientEvent;
  WriteHandler: TBleLocalWriteEvent;
  Handler: TBleLocalCharacteristicEvent;
  Peripheral: TBleLocalPeripheral;
  Characteristic: TBleLocalCharacteristic;
begin
  ClientHandler := nil;
  WriteHandler := nil;
  Handler := nil;
  Peripheral := TBleLocalPeripheral(Event.Target);
  Characteristic := TBleLocalCharacteristic(Event.Target);
  case Event.Kind of
    ekClientConnected: ClientHandler := Peripheral.FOnClientConnected;
    ekClientDisconnected: ClientHandler := Peripheral.FOnClientDisconnected;
    ekLocalWrite: WriteHandler := Characteristic.FOnWrite;
    ekLocalSubscribed: Handler := Characteristic.FOnSubscribed;
    ekLocalUnsubscribed: Handler := Characteristic.FOnUnsubscribed;
  end;
  if Assigned(ClientHandler) then
    ClientHandler(Peripheral, Event.Text);
  if Assigned(WriteHandler) then
    WriteHandler(Characteristic, Event.Data);
  if Assigned(Handler) then
    Handler(Characteristic);
end;

procedure DeliverDone(const Event: TBleEvent);
begin
  try
    if Assigned(Event.OnReadDone) then
      Event.OnReadDone(TBlePeripheral(Event.Target), Event.Data, Event.Error)
    else if Assigned(Event.OnDone) then
      Event.OnDone(Event.Target, Event.Error);
  finally
    Event.Error.Free;
  end;
end;

procedure Deliver(const Event: TBleEvent);
var
  LogHandler: TBleLogEvent;
begin
  try
    case Event.Kind of
      ekPowerOn..ekScanUpdated:
        DeliverAdapterEvent(TBleAdapter(Event.Target), Event);
      ekConnected..ekPasskeyDisplay:
        DeliverPeripheralEvent(TBlePeripheral(Event.Target), Event);
      ekClientConnected..ekLocalUnsubscribed:
        DeliverLocalEvent(Event);
      ekLog:
        begin
          LogHandler := TBleLogging.FOnLog;
          if Assigned(LogHandler) then
            LogHandler(Event.Log);
        end;
      ekDone:
        DeliverDone(Event);
    end;
  except
    on E: Exception do
      ReportHandlerException(E);
  end;
end;

function DeliversOnMainThread: Boolean;
begin
  case TSimpleDelphiBLE.FEventDelivery of
    edMainThread: Result := True;
    edEventThread: Result := False;
  else
    // NOTE: VCL, FMX and LCL set WakeMainThread so other threads can wake their main loop.
    Result := Assigned(WakeMainThread);
  end;
end;

procedure TBleEventThread.Execute;
begin
  while not Terminated do
  begin
    Dispatcher.FWakeup.WaitFor(INFINITE);
    if not Terminated then
      Dispatcher.Pump;
  end;
end;

constructor TBleDispatcher.Create;
begin
  inherited Create;
  FMainQueue := TQueue<TBleEvent>.Create;
  FThreadQueue := TQueue<TBleEvent>.Create;
  FWakeup := TEvent.Create(nil, False, False, '');
end;

destructor TBleDispatcher.Destroy;
begin
  FWakeup.Free;
  FThreadQueue.Free;
  FMainQueue.Free;
  inherited;
end;

// Starts the event thread before the first callback is registered. Free Pascal also needs a thread started this way
// before SimpleBLE's threads call in.
procedure TBleDispatcher.Start;
begin
  Lock.Enter;
  try
    if FThread = nil then
      FThread := TBleEventThread.Create(False);
  finally
    Lock.Leave;
  end;
end;

procedure TBleDispatcher.Stop;
begin
  if FThread <> nil then
  begin
    FThread.Terminate;
    FWakeup.SetEvent;
    FreeAndNil(FThread);
  end;
  TThread.RemoveQueuedEvents(nil, DrainMain);
end;

procedure TBleDispatcher.Post(const Event: TBleEvent);
begin
  Lock.Enter;
  try
    if DeliversOnMainThread then
      FMainQueue.Enqueue(Event)
    else
      FThreadQueue.Enqueue(Event);
  finally
    Lock.Leave;
  end;
  FWakeup.SetEvent;
end;

procedure TBleDispatcher.Purge(Owner: TBleLocalPeripheral);

  procedure PurgeQueue(Queue: TQueue<TBleEvent>);
  var
    Count: Integer;
    Event: TBleEvent;
  begin
    for Count := Queue.Count downto 1 do
    begin
      Event := Queue.Dequeue;
      if Event.Owner <> Owner then
        Queue.Enqueue(Event);
    end;
  end;

begin
  PurgeQueue(FMainQueue);
  PurgeQueue(FThreadQueue);
end;

procedure TBleDispatcher.Pump;
var
  Event: TBleEvent;
  Found: Boolean;
  Schedule: Boolean;
begin
  repeat
    Lock.Enter;
    try
      Schedule := (FMainQueue.Count > 0) and not FMainScheduled;
      if Schedule then
        FMainScheduled := True;
      Found := FThreadQueue.Count > 0;
      if Found then
        Event := FThreadQueue.Dequeue;
    finally
      Lock.Leave;
    end;
    if Schedule then
      TThread.Queue(nil, DrainMain);
    if Found then
      Deliver(Event);
  until not Found;
end;

procedure TBleDispatcher.DrainMain;
var
  Event: TBleEvent;
  Count: Integer;
  Found: Boolean;
  Pending: Boolean;
begin
  // NOTE: Only the events queued so far are delivered, so a steady stream of them can't starve the main loop.
  Lock.Enter;
  try
    Count := FMainQueue.Count;
  finally
    Lock.Leave;
  end;
  while Count > 0 do
  begin
    Lock.Enter;
    try
      // NOTE: Freeing a local peripheral may have purged events since they were counted.
      Found := FMainQueue.Count > 0;
      if Found then
        Event := FMainQueue.Dequeue;
    finally
      Lock.Leave;
    end;
    if not Found then
      Break;
    Deliver(Event);
    Dec(Count);
  end;
  Lock.Enter;
  try
    FMainScheduled := False;
    Pending := FMainQueue.Count > 0;
  finally
    Lock.Leave;
  end;
  if Pending then
    FWakeup.SetEvent;
end;
