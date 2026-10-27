// Part of SimpleDelphiBLE.pas: TBleConfig, TBleLogging and TSimpleDelphiBLE.

{ TBleConfig }

class procedure TBleConfig.ResetAll;
begin
  simpleble_config_reset_all;
end;

class procedure TBleConfig.SimpleBluez.Reset;
begin
  simpleble_config_simplebluez_reset;
end;

class function TBleConfig.SimpleBluez.GetUseSystemBus: Boolean;
begin
  Result := simpleble_config_simplebluez_get_use_system_bus;
end;

class procedure TBleConfig.SimpleBluez.SetUseSystemBus(Value: Boolean);
begin
  simpleble_config_simplebluez_set_use_system_bus(Value);
end;

class function TBleConfig.SimpleBluez.GetConnectionTimeoutMs: Int64;
begin
  Result := simpleble_config_simplebluez_get_connection_timeout_ms;
end;

class procedure TBleConfig.SimpleBluez.SetConnectionTimeoutMs(Value: Int64);
begin
  simpleble_config_simplebluez_set_connection_timeout_ms(Value);
end;

class function TBleConfig.SimpleBluez.GetDisconnectionTimeoutMs: Int64;
begin
  Result := simpleble_config_simplebluez_get_disconnection_timeout_ms;
end;

class procedure TBleConfig.SimpleBluez.SetDisconnectionTimeoutMs(Value: Int64);
begin
  simpleble_config_simplebluez_set_disconnection_timeout_ms(Value);
end;

class procedure TBleConfig.WinRT.Reset;
begin
  simpleble_config_winrt_reset;
end;

class function TBleConfig.WinRT.GetExperimentalReinitializeWinrtApartmentOnMainThread: Boolean;
begin
  Result := simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread;
end;

class procedure TBleConfig.WinRT.SetExperimentalReinitializeWinrtApartmentOnMainThread(Value: Boolean);
begin
  simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(Value);
end;

class function TBleConfig.WinRT.GetUseDeferredDisconnect: Boolean;
begin
  Result := simpleble_config_winrt_get_use_deferred_disconnect;
end;

class procedure TBleConfig.WinRT.SetUseDeferredDisconnect(Value: Boolean);
begin
  simpleble_config_winrt_set_use_deferred_disconnect(Value);
end;

class procedure TBleConfig.CoreBluetooth.Reset;
begin
  simpleble_config_corebluetooth_reset;
end;

class procedure TBleConfig.Android.Reset;
begin
  simpleble_config_android_reset;
end;

class function TBleConfig.Android.GetNegotiateMtu: Boolean;
begin
  Result := simpleble_config_android_get_negotiate_mtu;
end;

class procedure TBleConfig.Android.SetNegotiateMtu(Value: Boolean);
begin
  simpleble_config_android_set_negotiate_mtu(Value);
end;

class function TBleConfig.Android.GetConnectionPriority: TBleAndroidConnectionPriority;
var
  Value: simpleble_config_android_connection_priority_t;
begin
  // NOTE: SimpleBLE numbers the priorities from -1, for disabled.
  Value := simpleble_config_android_get_connection_priority + 1;
  if (Value >= Ord(Low(TBleAndroidConnectionPriority))) and (Value <= Ord(High(TBleAndroidConnectionPriority))) then
    Result := TBleAndroidConnectionPriority(Value)
  else
    Result := cpDisabled;
end;

class procedure TBleConfig.Android.SetConnectionPriority(Value: TBleAndroidConnectionPriority);
begin
  simpleble_config_android_set_connection_priority(Ord(Value) - 1);
end;

class procedure TBleConfig.Dongl.Reset;
begin
  simpleble_config_dongl_reset;
end;

class function TBleConfig.Dongl.GetUseDonglBackend: Boolean;
begin
  Result := simpleble_config_dongl_get_use_dongl_backend;
end;

class procedure TBleConfig.Dongl.SetUseDonglBackend(Value: Boolean);
begin
  simpleble_config_dongl_set_use_dongl_backend(Value);
end;

class function TBleConfig.Dongl.GetAutoUpdate: Boolean;
begin
  Result := simpleble_config_dongl_get_auto_update;
end;

class procedure TBleConfig.Dongl.SetAutoUpdate(Value: Boolean);
begin
  simpleble_config_dongl_set_auto_update(Value);
end;

class function TBleConfig.Dongl.GetForceUpdate: Boolean;
begin
  Result := simpleble_config_dongl_get_force_update;
end;

class procedure TBleConfig.Dongl.SetForceUpdate(Value: Boolean);
begin
  simpleble_config_dongl_set_force_update(Value);
end;

{ TBleLogging }

class function TBleLogging.GetLevel: TBleLogLevel;
var
  Value: simpleble_log_level_t;
begin
  Value := simpleble_logging_get_level;
  if (Value >= Ord(Low(TBleLogLevel))) and (Value <= Ord(High(TBleLogLevel))) then
    Result := TBleLogLevel(Value)
  else
    Result := llVerbose;
end;

class procedure TBleLogging.SetLevel(Value: TBleLogLevel);
begin
  simpleble_logging_set_level(Ord(Value));
end;

class procedure TBleLogging.SetOnLog(const Value: TBleLogEvent);
begin
  FOnLog := Value;
  if Assigned(Value) then
  begin
    Dispatcher.Start;
    simpleble_logging_set_callback(LogCallback);
  end
  else
    simpleble_logging_set_callback(nil);
end;

class procedure TBleLogging.LogToStdout;
begin
  FOnLog := nil;
  simpleble_logging_log_default_stdout;
end;

class procedure TBleLogging.LogToFile(const Path: string);
var
  Bytes: RawByteString;
begin
  FOnLog := nil;
  if Path = '' then
    simpleble_logging_log_default_file_path(nil)
  else
  begin
    Bytes := UTF8Encode(Path);
    simpleble_logging_log_default_file_path(PAnsiChar(Bytes));
  end;
end;

{ TSimpleDelphiBLE }

class function TSimpleDelphiBLE.GetVersion: string;
begin
  Result := FromUtf8(simpleble_get_version);
end;

class function TSimpleDelphiBLE.GetOperatingSystem: TBleOperatingSystem;
var
  Value: simpleble_os_t;
begin
  Value := simpleble_get_operating_system;
  if (Value >= Ord(Low(TBleOperatingSystem))) and (Value <= Ord(High(TBleOperatingSystem))) then
    Result := TBleOperatingSystem(Value)
  else
    Result := osUnknown;
end;
