// SimpleDelphiBLE brings SimpleBLE to Delphi and Free Pascal.
unit SimpleDelphiBLE;

{$I SimpleDelphiBLE.inc}

{$IFDEF FPC}
  // NOTE: The constructors that wrap a SimpleBLE handle are private on purpose.
  {$WARN 3018 OFF}
  // NOTE: Free Pascal 3.2.2 warns about abstract methods in its own TDictionary enumerators when it's specialized.
  {$WARN 4046 OFF}
{$ENDIF}

interface

uses
  SysUtils, Classes, Generics.Collections;

type
  TBleAdapter = class;
  TBlePeripheral = class;
  TBleLocalPeripheral = class;
  TBleLocalService = class;
  TBleLocalCharacteristic = class;

  // SimpleBLE's classification of a failure, in the order of SimpleCBLE's simpleble_err_t.
  TBleErrorCode = (ecInvalidArgument, ecOutOfMemory, ecObjectNotInitialized, ecInvalidBackendReference,
    ecPeripheralNotConnected, ecGattServiceNotFound, ecGattCharacteristicNotFound, ecGattDescriptorNotFound,
    ecOperationNotSupported, ecOperationFailed, ecWinRTAccessDenied, ecWinRTException, ecCoreBluetoothException,
    ecUnclassifiedException, ecAdapterInUse);

  // A failure reported by SimpleBLE. Each kind has its own subclass, except ecOutOfMemory and
  // ecUnclassifiedException, which raise EBleError itself.
  EBleError = class(Exception)
  private
    FCode: TBleErrorCode;
  public
    constructor Create(Code: TBleErrorCode; const Msg: string);
    property Code: TBleErrorCode read FCode;
  end;

  EBleInvalidArgument = class(EBleError);
  EBleNotInitialized = class(EBleError);
  EBleInvalidReference = class(EBleError);
  EBleNotConnected = class(EBleError);
  EBleServiceNotFound = class(EBleError);
  EBleCharacteristicNotFound = class(EBleError);
  EBleDescriptorNotFound = class(EBleError);
  EBleOperationNotSupported = class(EBleError);
  EBleOperationFailed = class(EBleError);
  EBleWinRTAccessDenied = class(EBleError);
  EBleWinRT = class(EBleError);
  EBleCoreBluetooth = class(EBleError);
  EBleAdapterInUse = class(EBleError);

  // SimpleBLE's native library couldn't be loaded. The message says why for each file tried.
  EBleLibraryNotFound = class(Exception);

  TBleAddressType = (atPublic, atRandom, atUnspecified);

  TBleOperatingSystem = (osWindows, osMacOS, osLinux, osIOS, osAndroid, osUnknown);

  // Where event handlers run. edAuto uses the main thread when VCL, FMX or LCL runs the application, and
  // SimpleDelphiBLE's event thread otherwise.
  TBleEventDelivery = (edAuto, edMainThread, edEventThread);

  TBleLogLevel = (llNone, llFatal, llError, llWarn, llInfo, llDebug, llVerbose);

  TBleAndroidConnectionPriority = (cpDisabled, cpBalanced, cpHigh, cpLowPower, cpDck);

  TBleLocalCapability = (lcRead, lcWriteRequest, lcWriteCommand, lcNotify, lcIndicate);
  TBleLocalCapabilities = set of TBleLocalCapability;

  TBleManufacturerData = record
    CompanyId: Word;
    Data: TBytes;
  end;

  TBleCharacteristic = record
    Uuid: string;
    Descriptors: TArray<string>;
    CanRead: Boolean;
    CanWriteRequest: Boolean;
    CanWriteCommand: Boolean;
    CanNotify: Boolean;
    CanIndicate: Boolean;
  end;

  TBleService = record
    Uuid: string;
    // Service data from the advertisement.
    Data: TBytes;
    Characteristics: TArray<TBleCharacteristic>;
  end;

  TBleLogEntry = record
    Level: TBleLogLevel;
    Module: string;
    FileName: string;
    Line: Cardinal;
    FunctionName: string;
    Message: string;
  end;

  TBleAdapterEvent = procedure(Sender: TBleAdapter) of object;
  TBleScanEvent = procedure(Sender: TBleAdapter; Peripheral: TBlePeripheral) of object;
  TBlePeripheralEvent = procedure(Sender: TBlePeripheral) of object;
  TBleValueEvent = procedure(Sender: TBlePeripheral; const Service, Characteristic: string;
    const Data: TBytes) of object;
  TBlePasskeyRequestEvent = procedure(Sender: TBlePeripheral; var Passkey: string; var Accept: Boolean) of object;
  TBlePasskeyDisplayEvent = procedure(Sender: TBlePeripheral; const Passkey: string) of object;
  TBleNumericComparisonEvent = procedure(Sender: TBlePeripheral; const Passkey: string;
    var Accept: Boolean) of object;
  TBleAdapterPasskeyRequestEvent = procedure(Sender: TBleAdapter; const Peer: string; var Passkey: string;
    var Accept: Boolean) of object;
  TBleAdapterPasskeyDisplayEvent = procedure(Sender: TBleAdapter; const Peer, Passkey: string) of object;
  TBleAdapterNumericComparisonEvent = procedure(Sender: TBleAdapter; const Peer, Passkey: string;
    var Accept: Boolean) of object;
  TBleAdapterPairingAuthorizationEvent = procedure(Sender: TBleAdapter; const Peer: string;
    var Accept: Boolean) of object;
  TBleClientEvent = procedure(Sender: TBleLocalPeripheral; const Address: string) of object;
  TBleLocalCharacteristicEvent = procedure(Sender: TBleLocalCharacteristic) of object;
  TBleLocalWriteEvent = procedure(Sender: TBleLocalCharacteristic; const Data: TBytes) of object;
  TBleLocalReadEvent = procedure(Sender: TBleLocalCharacteristic; var Value: TBytes) of object;
  TBleLogEvent = procedure(const Entry: TBleLogEntry) of object;
  TBleHandlerExceptionEvent = procedure(E: Exception) of object;
  // Reports the end of an Async call: Error is nil when the call succeeded. SimpleDelphiBLE frees Error once the
  // handler returns.
  TBleCompletionEvent = procedure(Sender: TObject; Error: Exception) of object;
  TBleReadCompletionEvent = procedure(Sender: TBlePeripheral; const Data: TBytes; Error: Exception) of object;

  // The base of the classes that wrap a SimpleBLE object.
  TBleNativeObject = class
  private
    // Identifies the object to SimpleBLE's callbacks, which can't hold a reference to it.
    FToken: NativeUInt;
    // Whether the object may be freed now. Objects that SimpleDelphiBLE owns only allow it while their owner frees them.
    FFreeable: Boolean;
    procedure Unregister;
    procedure FreeOwned;
  protected
    procedure ClearCallbacks; virtual;
  public
    constructor Create;
    destructor Destroy; override;
    procedure BeforeDestruction; override;
  end;

  // A Bluetooth backend, such as the operating system's Bluetooth stack or the Dongl. SimpleDelphiBLE owns backends.
  TBleBackend = class(TBleNativeObject)
  private
    FHandle: Pointer;
    FIdentifier: string;
    constructor CreateFromHandle(Handle: Pointer; const Identifier: string);
  public
    destructor Destroy; override;
    class function GetBackends: TArray<TBleBackend>;
    class function GetNative: TBleBackend;
    class function GetDongl: TBleBackend;
    class function GetSimulator: TBleBackend;
    function IsBluetoothEnabled: Boolean;
    function GetAdapters: TArray<TBleAdapter>;
    property Identifier: string read FIdentifier;
  end;

  // A Bluetooth adapter. SimpleDelphiBLE owns adapters and keeps one object per adapter, so don't free them.
  TBleAdapter = class(TBleNativeObject)
  private
    FHandle: Pointer;
    // Peripherals by their SimpleBLE identity, guarded by the unit's lock.
    FPeripherals: TDictionary<Pointer, TBlePeripheral>;
    FOnPowerOn: TBleAdapterEvent;
    FOnPowerOff: TBleAdapterEvent;
    FOnScanStart: TBleAdapterEvent;
    FOnScanStop: TBleAdapterEvent;
    FOnScanFound: TBleScanEvent;
    FOnScanUpdated: TBleScanEvent;
    FOnPasskeyRequest: TBleAdapterPasskeyRequestEvent;
    FOnPasskeyDisplay: TBleAdapterPasskeyDisplayEvent;
    FOnNumericComparison: TBleAdapterNumericComparisonEvent;
    FOnPairingAuthorization: TBleAdapterPairingAuthorizationEvent;
    constructor CreateFromHandle(Handle: Pointer);
    procedure SetCallbacks(Enabled: Boolean);
    function Intern(Handle: Pointer): TBlePeripheral;
    function TakePeripherals(List: Pointer; Count: NativeUInt): TArray<TBlePeripheral>;
    function GetIdentifier: string;
    function GetAddress: string;
    procedure SetOnPasskeyRequest(const Value: TBleAdapterPasskeyRequestEvent);
    procedure SetOnPasskeyDisplay(const Value: TBleAdapterPasskeyDisplayEvent);
    procedure SetOnNumericComparison(const Value: TBleAdapterNumericComparisonEvent);
    procedure SetOnPairingAuthorization(const Value: TBleAdapterPairingAuthorizationEvent);
  protected
    procedure ClearCallbacks; override;
  public
    destructor Destroy; override;
    class function IsBluetoothEnabled: Boolean;
    class function GetAdapters: TArray<TBleAdapter>;
    function IsPowered: Boolean;
    procedure PowerOn;
    procedure PowerOff;
    procedure ScanStart;
    procedure ScanStop;
    procedure ScanFor(TimeoutMs: Integer);
    // Runs ScanFor on a worker thread and reports its end to OnDone, which is delivered like an event.
    procedure ScanForAsync(TimeoutMs: Integer; OnDone: TBleCompletionEvent);
    function ScanIsActive: Boolean;
    function ScanGetResults: TArray<TBlePeripheral>;
    function GetPairedPeripherals: TArray<TBlePeripheral>;
    function GetConnectedPeripherals: TArray<TBlePeripheral>;
    // Returns the peripherals CoreBluetooth already knows on macOS, by the identifiers Address returns, skipping
    // unknown ones. It doesn't scan, so a peripheral may be out of range. The adapter must be powered on. Other
    // platforms raise EBleOperationNotSupported.
    function RetrieveCachedPeripherals(const Identifiers: array of string): TArray<TBlePeripheral>;
    // Creates a GATT server hosted by this application, which the caller frees after stopping it.
    function CreateLocalPeripheral: TBleLocalPeripheral;
    property Identifier: string read GetIdentifier;
    property Address: string read GetAddress;
    property OnPowerOn: TBleAdapterEvent read FOnPowerOn write FOnPowerOn;
    property OnPowerOff: TBleAdapterEvent read FOnPowerOff write FOnPowerOff;
    property OnScanStart: TBleAdapterEvent read FOnScanStart write FOnScanStart;
    property OnScanStop: TBleAdapterEvent read FOnScanStop write FOnScanStop;
    property OnScanFound: TBleScanEvent read FOnScanFound write FOnScanFound;
    property OnScanUpdated: TBleScanEvent read FOnScanUpdated write FOnScanUpdated;
    // Pairing handlers for a BlueZ adapter on Linux, for the peripherals it connects to and the clients of its local
    // peripherals. Each receives the peer's address. Passkey and numeric comparison requests reach them when
    // TBleConfig.SimpleBluez.InteractivePairing is set. OnPasskeyRequest, OnNumericComparison and
    // OnPairingAuthorization run on SimpleBLE's BlueZ event thread, which waits for their answer. Without
    // OnPairingAuthorization, Just Works pairing is accepted. Other adapters and platforms raise
    // EBleOperationNotSupported.
    property OnPasskeyRequest: TBleAdapterPasskeyRequestEvent read FOnPasskeyRequest write SetOnPasskeyRequest;
    property OnPasskeyDisplay: TBleAdapterPasskeyDisplayEvent read FOnPasskeyDisplay write SetOnPasskeyDisplay;
    property OnNumericComparison: TBleAdapterNumericComparisonEvent read FOnNumericComparison
      write SetOnNumericComparison;
    property OnPairingAuthorization: TBleAdapterPairingAuthorizationEvent read FOnPairingAuthorization
      write SetOnPairingAuthorization;
  end;

  // A remote peripheral. Its adapter owns it and keeps one object per device, so don't free it.
  TBlePeripheral = class(TBleNativeObject)
  private
    FHandle: Pointer;
    // Value handlers by service and characteristic, guarded by the unit's lock.
    FSubscriptions: TDictionary<string, TBleValueEvent>;
    FOnConnected: TBlePeripheralEvent;
    FOnDisconnected: TBlePeripheralEvent;
    FOnPasskeyRequest: TBlePasskeyRequestEvent;
    FOnPasskeyDisplay: TBlePasskeyDisplayEvent;
    FOnNumericComparison: TBleNumericComparisonEvent;
    constructor CreateFromHandle(Handle: Pointer);
    procedure SetCallbacks(Enabled: Boolean);
    procedure Subscribe(Indicate: Boolean; const Service, Characteristic: string; OnValue: TBleValueEvent);
    function GetIdentifier: string;
    function GetAddress: string;
    function GetAddressType: TBleAddressType;
    function GetRssi: SmallInt;
    function GetTxPower: SmallInt;
    function GetMtu: Word;
    function GetIsConnected: Boolean;
    function GetIsConnectable: Boolean;
    procedure SetOnPasskeyRequest(const Value: TBlePasskeyRequestEvent);
    procedure SetOnPasskeyDisplay(const Value: TBlePasskeyDisplayEvent);
    procedure SetOnNumericComparison(const Value: TBleNumericComparisonEvent);
  protected
    procedure ClearCallbacks; override;
  public
    destructor Destroy; override;
    procedure Connect;
    procedure Disconnect;
    function IsPaired: Boolean;
    procedure Unpair;
    // The advertised services while disconnected, and the GATT services once connected.
    function Services: TArray<TBleService>;
    function ManufacturerData: TArray<TBleManufacturerData>;
    function Read(const Service, Characteristic: string): TBytes;
    procedure WriteRequest(const Service, Characteristic: string; const Data: TBytes);
    procedure WriteCommand(const Service, Characteristic: string; const Data: TBytes);
    // Subscribes to a characteristic and returns once subscribed. OnValue receives each value until Unsubscribe.
    procedure Notify(const Service, Characteristic: string; OnValue: TBleValueEvent);
    procedure Indicate(const Service, Characteristic: string; OnValue: TBleValueEvent);
    procedure Unsubscribe(const Service, Characteristic: string);
    function ReadDescriptor(const Service, Characteristic, Descriptor: string): TBytes;
    procedure WriteDescriptor(const Service, Characteristic, Descriptor: string; const Data: TBytes);
    // Secures a Dongl connection with LE Secure Connections. With Mitm, the peer must also be authenticated through
    // OnPasskeyRequest, OnPasskeyDisplay or OnNumericComparison.
    procedure SecureConnection(Mitm: Boolean = False);
    // The Async versions run the call on a worker thread and report its end to OnDone, which is delivered like an
    // event, so GUI applications get it on the main thread.
    procedure ConnectAsync(OnDone: TBleCompletionEvent);
    procedure DisconnectAsync(OnDone: TBleCompletionEvent);
    procedure ReadAsync(const Service, Characteristic: string; OnDone: TBleReadCompletionEvent);
    procedure WriteRequestAsync(const Service, Characteristic: string; const Data: TBytes;
      OnDone: TBleCompletionEvent);
    procedure WriteCommandAsync(const Service, Characteristic: string; const Data: TBytes;
      OnDone: TBleCompletionEvent);
    procedure NotifyAsync(const Service, Characteristic: string; OnValue: TBleValueEvent;
      OnDone: TBleCompletionEvent);
    procedure IndicateAsync(const Service, Characteristic: string; OnValue: TBleValueEvent;
      OnDone: TBleCompletionEvent);
    procedure UnsubscribeAsync(const Service, Characteristic: string; OnDone: TBleCompletionEvent);
    procedure ReadDescriptorAsync(const Service, Characteristic, Descriptor: string;
      OnDone: TBleReadCompletionEvent);
    procedure WriteDescriptorAsync(const Service, Characteristic, Descriptor: string; const Data: TBytes;
      OnDone: TBleCompletionEvent);
    procedure SecureConnectionAsync(Mitm: Boolean; OnDone: TBleCompletionEvent);
    property Identifier: string read GetIdentifier;
    property Address: string read GetAddress;
    property AddressType: TBleAddressType read GetAddressType;
    // The received signal strength in dBm, or -32768 when unavailable.
    property Rssi: SmallInt read GetRssi;
    // The advertised transmit power in dBm, or -32768 when unavailable.
    property TxPower: SmallInt read GetTxPower;
    property Mtu: Word read GetMtu;
    property IsConnected: Boolean read GetIsConnected;
    property IsConnectable: Boolean read GetIsConnectable;
    property OnConnected: TBlePeripheralEvent read FOnConnected write FOnConnected;
    property OnDisconnected: TBlePeripheralEvent read FOnDisconnected write FOnDisconnected;
    // Pairing handlers for peripherals reached through a Dongl, set before connecting. OnPasskeyRequest and
    // OnNumericComparison run on a SimpleBLE thread, which waits for their answer.
    property OnPasskeyRequest: TBlePasskeyRequestEvent read FOnPasskeyRequest write SetOnPasskeyRequest;
    property OnPasskeyDisplay: TBlePasskeyDisplayEvent read FOnPasskeyDisplay write SetOnPasskeyDisplay;
    property OnNumericComparison: TBleNumericComparisonEvent read FOnNumericComparison write SetOnNumericComparison;
  end;

  // A GATT server hosted by this application. The application owns it: stop it, then free it.
  TBleLocalPeripheral = class(TBleNativeObject)
  private
    FHandle: Pointer;
    // Every service object made for this peripheral, which it owns, and the current ones by SimpleBLE identity.
    FServiceObjects: TList<TBleLocalService>;
    FServices: TDictionary<Pointer, TBleLocalService>;
    FOnClientConnected: TBleClientEvent;
    FOnClientDisconnected: TBleClientEvent;
    constructor CreateFromHandle(Handle: Pointer);
    procedure SetCallbacks(Enabled: Boolean);
    function Intern(Handle: Pointer): TBleLocalService;
    function GetIsStarted: Boolean;
    procedure SetLocalName(Name: PAnsiChar);
  protected
    procedure ClearCallbacks; override;
  public
    destructor Destroy; override;
    procedure AddAdvertisedService(const Uuid: string);
    procedure SetAdvertisedManufacturerData(CompanyId: Word; const Data: TBytes);
    procedure SetConnectable(Connectable: Boolean);
    // Sets the name in the advertisement on macOS and Linux, before Start. The system's Bluetooth name stays the
    // same. Other platforms raise EBleOperationNotSupported.
    procedure SetAdvertisementLocalName(const Name: string);
    // Removes the name SetAdvertisementLocalName set, before Start.
    procedure ClearAdvertisementLocalName;
    function AddService(const Uuid: string): TBleLocalService;
    function Services: TArray<TBleLocalService>;
    procedure RemoveAllServices;
    procedure Start;
    procedure Stop;
    function IsAdvertising: Boolean;
    property IsStarted: Boolean read GetIsStarted;
    property OnClientConnected: TBleClientEvent read FOnClientConnected write FOnClientConnected;
    property OnClientDisconnected: TBleClientEvent read FOnClientDisconnected write FOnClientDisconnected;
  end;

  // A service of a local peripheral, which owns it.
  TBleLocalService = class(TBleNativeObject)
  private
    FHandle: Pointer;
    FPeripheral: TBleLocalPeripheral;
    FCharacteristicObjects: TList<TBleLocalCharacteristic>;
    FCharacteristics: TDictionary<Pointer, TBleLocalCharacteristic>;
    constructor CreateFromHandle(Peripheral: TBleLocalPeripheral; Handle: Pointer);
    function Intern(Handle: Pointer): TBleLocalCharacteristic;
    function GetUuid: string;
  protected
    procedure ClearCallbacks; override;
  public
    destructor Destroy; override;
    function AddCharacteristic(const Uuid: string; Capabilities: TBleLocalCapabilities): TBleLocalCharacteristic;
    function Characteristics: TArray<TBleLocalCharacteristic>;
    property Uuid: string read GetUuid;
  end;

  // A characteristic of a local service, which owns it.
  TBleLocalCharacteristic = class(TBleNativeObject)
  private
    FHandle: Pointer;
    FPeripheral: TBleLocalPeripheral;
    // The value OnReadRequest returned last, which SimpleBLE reads until the next request.
    FReadValue: TBytes;
    FOnReadRequest: TBleLocalReadEvent;
    FOnWrite: TBleLocalWriteEvent;
    FOnSubscribed: TBleLocalCharacteristicEvent;
    FOnUnsubscribed: TBleLocalCharacteristicEvent;
    constructor CreateFromHandle(Peripheral: TBleLocalPeripheral; Handle: Pointer);
    procedure SetCallbacks(Enabled: Boolean);
    procedure SetOnReadRequest(const Value: TBleLocalReadEvent);
    function GetUuid: string;
    function GetCapabilities: TBleLocalCapabilities;
    function GetValue: TBytes;
  protected
    procedure ClearCallbacks; override;
  public
    destructor Destroy; override;
    // Updates the value, and sends it to subscribed clients of a notifying or indicating characteristic.
    procedure SetValue(const Value: TBytes);
    property Uuid: string read GetUuid;
    property Capabilities: TBleLocalCapabilities read GetCapabilities;
    property Value: TBytes read GetValue;
    // Supplies the value for each read instead of Value. It runs on a SimpleBLE thread and must return promptly.
    property OnReadRequest: TBleLocalReadEvent read FOnReadRequest write SetOnReadRequest;
    property OnWrite: TBleLocalWriteEvent read FOnWrite write FOnWrite;
    property OnSubscribed: TBleLocalCharacteristicEvent read FOnSubscribed write FOnSubscribed;
    property OnUnsubscribed: TBleLocalCharacteristicEvent read FOnUnsubscribed write FOnUnsubscribed;
  end;

  // SimpleBLE's settings. Set them before getting the first adapter or backend.
  TBleConfig = class
  public
    type
      SimpleBluez = class
      private
        class function GetUseSystemBus: Boolean; static;
        class procedure SetUseSystemBus(Value: Boolean); static;
        class function GetConnectionTimeoutMs: Int64; static;
        class procedure SetConnectionTimeoutMs(Value: Int64); static;
        class function GetDisconnectionTimeoutMs: Int64; static;
        class procedure SetDisconnectionTimeoutMs(Value: Int64); static;
        class function GetInteractivePairing: Boolean; static;
        class procedure SetInteractivePairing(Value: Boolean); static;
      public
        class procedure Reset; static;
        class property UseSystemBus: Boolean read GetUseSystemBus write SetUseSystemBus;
        class property ConnectionTimeoutMs: Int64 read GetConnectionTimeoutMs write SetConnectionTimeoutMs;
        class property DisconnectionTimeoutMs: Int64 read GetDisconnectionTimeoutMs write SetDisconnectionTimeoutMs;
        // Registers SimpleBLE's pairing agent as KeyboardDisplay and as BlueZ's default agent, so passkey and numeric
        // comparison requests reach the adapter's pairing handlers.
        class property InteractivePairing: Boolean read GetInteractivePairing write SetInteractivePairing;
      end;

      WinRT = class
      private
        class function GetExperimentalReinitializeWinrtApartmentOnMainThread: Boolean; static;
        class procedure SetExperimentalReinitializeWinrtApartmentOnMainThread(Value: Boolean); static;
        class function GetUseDeferredDisconnect: Boolean; static;
        class procedure SetUseDeferredDisconnect(Value: Boolean); static;
      public
        class procedure Reset; static;
        class property ExperimentalReinitializeWinrtApartmentOnMainThread: Boolean
          read GetExperimentalReinitializeWinrtApartmentOnMainThread
          write SetExperimentalReinitializeWinrtApartmentOnMainThread;
        class property UseDeferredDisconnect: Boolean read GetUseDeferredDisconnect write SetUseDeferredDisconnect;
      end;

      CoreBluetooth = class
      public
        class procedure Reset; static;
      end;

      Android = class
      private
        class function GetNegotiateMtu: Boolean; static;
        class procedure SetNegotiateMtu(Value: Boolean); static;
        class function GetConnectionPriority: TBleAndroidConnectionPriority; static;
        class procedure SetConnectionPriority(Value: TBleAndroidConnectionPriority); static;
      public
        class procedure Reset; static;
        class property NegotiateMtu: Boolean read GetNegotiateMtu write SetNegotiateMtu;
        class property ConnectionPriority: TBleAndroidConnectionPriority read GetConnectionPriority
          write SetConnectionPriority;
      end;

      Dongl = class
      private
        class function GetUseDonglBackend: Boolean; static;
        class procedure SetUseDonglBackend(Value: Boolean); static;
        class function GetAutoUpdate: Boolean; static;
        class procedure SetAutoUpdate(Value: Boolean); static;
        class function GetForceUpdate: Boolean; static;
        class procedure SetForceUpdate(Value: Boolean); static;
      public
        class procedure Reset; static;
        class property UseDonglBackend: Boolean read GetUseDonglBackend write SetUseDonglBackend;
        class property AutoUpdate: Boolean read GetAutoUpdate write SetAutoUpdate;
        class property ForceUpdate: Boolean read GetForceUpdate write SetForceUpdate;
      end;

    class procedure ResetAll; static;
  end;

  // SimpleBLE's log, for the whole process.
  TBleLogging = class
  private
    class var FOnLog: TBleLogEvent;
    class function GetLevel: TBleLogLevel; static;
    class procedure SetLevel(Value: TBleLogLevel); static;
    class procedure SetOnLog(const Value: TBleLogEvent); static;
  public
    class procedure LogToStdout; static;
    // Appends the log to a file, or to a file named after the current time when Path is empty.
    class procedure LogToFile(const Path: string = ''); static;
    class property Level: TBleLogLevel read GetLevel write SetLevel;
    // Receives each log entry, like other events. LogToStdout and LogToFile replace it.
    class property OnLog: TBleLogEvent read FOnLog write SetOnLog;
  end;

  TSimpleDelphiBLE = class
  private
    class var FLibraryPath: string;
    class var FEventDelivery: TBleEventDelivery;
    class var FOnHandlerException: TBleHandlerExceptionEvent;
    class function GetVersion: string; static;
    class function GetOperatingSystem: TBleOperatingSystem; static;
  public
    // Loads SimpleBLE's native library if it isn't loaded yet, and tells whether it is. Every other call loads it the
    // same way, and raises EBleLibraryNotFound when it can't.
    class function IsAvailable: Boolean; static;
    // The native library's file, or the folder that holds it. When empty, SimpleDelphiBLE loads it from the program's
    // folder, then through the system's library search. Set it before the library is first loaded.
    class property LibraryPath: string read FLibraryPath write FLibraryPath;
    class property Version: string read GetVersion;
    class property OperatingSystem: TBleOperatingSystem read GetOperatingSystem;
    class property EventDelivery: TBleEventDelivery read FEventDelivery write FEventDelivery;
    // Receives exceptions that event handlers raise. Without it, the application's exception handler gets those
    // raised on the main thread, and ShowException the others.
    class property OnHandlerException: TBleHandlerExceptionEvent read FOnHandlerException write FOnHandlerException;
  end;

implementation

uses
  SyncObjs, Math, SimpleDelphiBLE.Native;

{$IFDEF FPC}
type
  TArithmeticExceptionMask = TFPUExceptionMask;

const
  exAllArithmeticExceptions = [exInvalidOp, exDenormalized, exZeroDivide, exOverflow, exUnderflow, exPrecision];
{$ENDIF}

type
  TBleErrorClass = class of EBleError;

  // A call into SimpleCBLE. Enter masks floating-point exceptions, which Free Pascal and some Delphi applications
  // unmask, because SimpleBLE runs with the C defaults. Leave restores the mask and raises the reported error.
  TNativeCall = record
    Error: psimpleble_error_t;
    Mask: TArithmeticExceptionMask;
    procedure Enter;
    procedure Leave;
  end;

  TBleEventKind = (ekPowerOn, ekPowerOff, ekScanStart, ekScanStop, ekScanFound, ekScanUpdated,
    ekAdapterPasskeyDisplay, ekConnected, ekDisconnected, ekValue, ekPasskeyDisplay, ekClientConnected,
    ekClientDisconnected, ekLocalWrite, ekLocalSubscribed, ekLocalUnsubscribed, ekLog, ekDone);

  TBleEvent = record
    Kind: TBleEventKind;
    Target: TBleNativeObject;
    // The local peripheral a local event belongs to, so freeing it can drop its queued events.
    Owner: TBleLocalPeripheral;
    Peripheral: TBlePeripheral;
    // The address of the peer an adapter's pairing event is about.
    Peer: string;
    Service: string;
    Characteristic: string;
    Text: string;
    Data: TBytes;
    Log: TBleLogEntry;
    // The end of an Async call.
    OnDone: TBleCompletionEvent;
    OnReadDone: TBleReadCompletionEvent;
    Error: Exception;
  end;

  TBleAsyncCall = (acScanFor, acConnect, acDisconnect, acRead, acWriteRequest, acWriteCommand, acNotify, acIndicate,
    acUnsubscribe, acReadDescriptor, acWriteDescriptor, acSecureConnection);

  // Runs the blocking call behind an Async method, then posts its end as an event.
  TBleAsyncWorker = class(TThread)
  private
    FCall: TBleAsyncCall;
    FTarget: TBleNativeObject;
    FService: string;
    FCharacteristic: string;
    FDescriptor: string;
    FData: TBytes;
    FTimeoutMs: Integer;
    FMitm: Boolean;
    FOnValue: TBleValueEvent;
    FOnDone: TBleCompletionEvent;
    FOnReadDone: TBleReadCompletionEvent;
  protected
    procedure Execute; override;
  public
    constructor Create(Call: TBleAsyncCall; Target: TBleNativeObject);
  end;

  TBleEventThread = class(TThread)
  protected
    procedure Execute; override;
  end;

  // Delivers events in the order SimpleBLE reported them, on the main thread or on the event thread. The event
  // thread also hands the main thread its events, because TThread.Queue runs a method at once on the main thread.
  TBleDispatcher = class
  private
    FMainQueue: TQueue<TBleEvent>;
    FThreadQueue: TQueue<TBleEvent>;
    FMainScheduled: Boolean;
    FWakeup: TEvent;
    FThread: TBleEventThread;
    procedure Pump;
    procedure DrainMain;
  public
    constructor Create;
    destructor Destroy; override;
    procedure Start;
    procedure Stop;
    procedure Post(const Event: TBleEvent);
    // Drops the queued events of a local peripheral. The caller holds the unit's lock.
    procedure Purge(Owner: TBleLocalPeripheral);
  end;

const
  ErrorClasses: array[TBleErrorCode] of TBleErrorClass = (EBleInvalidArgument, EBleError, EBleNotInitialized,
    EBleInvalidReference, EBleNotConnected, EBleServiceNotFound, EBleCharacteristicNotFound, EBleDescriptorNotFound,
    EBleOperationNotSupported, EBleOperationFailed, EBleWinRTAccessDenied, EBleWinRT, EBleCoreBluetooth, EBleError,
    EBleAdapterInUse);

var
  // Guards the registries, the subscriptions and the dispatcher's queues. Code holding it never calls SimpleBLE,
  // whose callbacks may be waiting for it.
  Lock: TCriticalSection;
  Targets: TDictionary<NativeUInt, TBleNativeObject>;
  NextToken: NativeUInt;
  // Adapters by their SimpleBLE identity, and backends by their identifier.
  Adapters: TDictionary<Pointer, TBleAdapter>;
  Backends: TDictionary<string, TBleBackend>;
  Dispatcher: TBleDispatcher;
  // Set once the unit finalizes. InFlight counts the native callbacks using the unit's state.
  ShuttingDown: Integer;
  InFlight: Integer;
  // Points writes of empty values somewhere valid, since SimpleCBLE rejects a NULL buffer.
  EmptyByte: Byte;
  // Guards loading the native library, which happens once.
  LibraryLock: TCriticalSection;
  LibraryLoaded: Boolean;

// NOTE: The implementation continues in include files, in the order its routines depend on each other.
{$I SimpleDelphiBLE.Support.inc}
{$I SimpleDelphiBLE.Events.inc}
{$I SimpleDelphiBLE.Callbacks.inc}

{ TBleNativeObject }

constructor TBleNativeObject.Create;
begin
  inherited Create;
  Lock.Enter;
  try
    Inc(NextToken);
    FToken := NextToken;
    Targets.Add(FToken, Self);
  finally
    Lock.Leave;
  end;
  Dispatcher.Start;
end;

destructor TBleNativeObject.Destroy;
begin
  Unregister;
  inherited;
end;

procedure TBleNativeObject.BeforeDestruction;
begin
  if not FFreeable then
    raise EInvalidOperation.CreateFmt('SimpleDelphiBLE owns this %s, so it can''t be freed', [ClassName]);
  inherited;
end;

procedure TBleNativeObject.Unregister;
begin
  Lock.Enter;
  try
    Targets.Remove(FToken);
  finally
    Lock.Leave;
  end;
end;

procedure TBleNativeObject.FreeOwned;
begin
  FFreeable := True;
  Free;
end;

procedure TBleNativeObject.ClearCallbacks;
begin
end;

{$I SimpleDelphiBLE.Backend.inc}
{$I SimpleDelphiBLE.Adapter.inc}
{$I SimpleDelphiBLE.Peripheral.inc}
{$I SimpleDelphiBLE.LocalPeripheral.inc}
{$I SimpleDelphiBLE.Config.inc}
{$I SimpleDelphiBLE.Async.inc}

{ Shutdown }

procedure Shutdown;
var
  Objects: TArray<TBleNativeObject>;
  Item: TBleNativeObject;
  Adapter: TBleAdapter;
  Backend: TBleBackend;
  Waited: Integer;
begin
  AtomicAdd(ShuttingDown, 1);
  Lock.Enter;
  try
    Objects := Targets.Values.ToArray;
  finally
    Lock.Leave;
  end;
  for Item in Objects do
    try
      Item.ClearCallbacks;
    except
      // NOTE: Shutting down continues for the other objects.
    end;
  if Assigned(TBleLogging.FOnLog) then
    simpleble_logging_set_callback(nil);
  // NOTE: Callbacks that can't be cleared, such as notifications, may still be running. When one doesn't return,
  // such as a handler waiting for the main thread, the unit's state stays allocated instead of being freed under it.
  Waited := 0;
  while (InFlight > 0) and (Waited < 5000) do
  begin
    Sleep(1);
    Inc(Waited);
  end;
  if InFlight > 0 then
    Exit;
  Dispatcher.Stop;
  for Adapter in Adapters.Values do
    Adapter.FreeOwned;
  for Backend in Backends.Values do
    Backend.FreeOwned;
  Adapters.Free;
  Backends.Free;
  Dispatcher.Free;
  Targets.Free;
  LibraryLock.Free;
  Lock.Free;
end;

initialization
  // NOTE: SimpleBLE's threads call into Pascal code, so the memory manager must be thread-safe from the start.
  IsMultiThread := True;
  Lock := TCriticalSection.Create;
  LibraryLock := TCriticalSection.Create;
  Targets := TDictionary<NativeUInt, TBleNativeObject>.Create;
  Adapters := TDictionary<Pointer, TBleAdapter>.Create;
  Backends := TDictionary<string, TBleBackend>.Create;
  Dispatcher := TBleDispatcher.Create;

finalization
  Shutdown;

end.
