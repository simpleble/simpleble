using System.Runtime.InteropServices;
using System.Text;
using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>Platform- and backend-specific APIs. Platform methods throw PlatformNotSupportedException on other platforms.</summary>
public static class Advanced
{
    /// <summary>Pairing callbacks for peripherals reached through a Dongl adapter. Register them before connecting.</summary>
    public static class Dongl
    {
        private static readonly NativeMethods.PairCallback request = Request;
        private static readonly NativeMethods.PairCallback compare = Compare;
        [MonoPInvokeCallback(typeof(NativeMethods.PairCallback))]
        private static bool Request(nint handle, nint buffer, nint token)
        {
            try { return WritePasskey(CallbackSlot.Find(token)?.Invoke(null), buffer); }
            catch (Exception ex) { CallbackErrors.Report(ex); return false; }
        }
        [MonoPInvokeCallback(typeof(NativeMethods.PairCallback))]
        private static bool Compare(nint handle, nint text, nint token)
        {
            try { return CallbackSlot.Find(token)?.Invoke(Marshal.PtrToStringUTF8(text) ?? "") is true; }
            catch (Exception ex) { CallbackErrors.Report(ex); return false; }
        }
        /// <summary>Runs synchronously on a native pairing worker. Return null to reject.</summary>
        public static void SetPasskeyRequestCallback(Peripheral peripheral, Func<string?>? callback)
        {
            if (peripheral is null) throw new ArgumentNullException(nameof(peripheral));
            peripheral.callbacks.Set("pair-request", callback is null ? null : _ => callback(), (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_dongl_set_passkey_request_callback(h, id == 0 ? null : request, id, ref e); return 0; }));
        }
        /// <summary>Receives the six-digit passkey to show to the user. Pass null to unregister.</summary>
        public static void SetPasskeyDisplayCallback(Peripheral peripheral, Action<string>? callback)
        {
            if (peripheral is null) throw new ArgumentNullException(nameof(peripheral));
            peripheral.callbacks.Set("pair-display", callback is null ? null : value => { callback((string)value!); return null; }, (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_dongl_set_passkey_display_callback(h, id == 0 ? null : NativeCallbacks.OnText, id, ref e); return 0; }));
        }
        /// <summary>Runs synchronously on a native pairing worker. Return false to reject.</summary>
        public static void SetNumericComparisonCallback(Peripheral peripheral, Func<string, bool>? callback)
        {
            if (peripheral is null) throw new ArgumentNullException(nameof(peripheral));
            peripheral.callbacks.Set("pair-compare", callback is null ? null : value => callback((string)value!), (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_dongl_set_numeric_comparison_callback(h, id == 0 ? null : compare, id, ref e); return 0; }));
        }
        /// <summary>
        /// Secures the connection with LE Secure Connections. With mitm, the peer must also be authenticated through the passkey or numeric comparison callbacks.
        /// Pairing started by this call creates no bond, and the Dongl disconnects if it fails.
        /// </summary>
        public static void SecureConnection(Peripheral peripheral, bool mitm = false)
        {
            if (peripheral is null) throw new ArgumentNullException(nameof(peripheral));
            peripheral.handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_advanced_dongl_secure_connection(h, mitm, ref e));
        }
        /// <summary>Runs SecureConnection on a worker. Cancellation only applies before the operation starts.</summary>
        public static Task SecureConnectionAsync(Peripheral peripheral, bool mitm = false, CancellationToken cancellationToken = default) =>
            Task.Run(() => SecureConnection(peripheral, mitm), cancellationToken);
    }

    /// <summary>Android initialization.</summary>
    public static class Android
    {
        /// <summary>
        /// Initializes Android on the application thread after Bluetooth is enabled and permissions are granted.
        /// Load simplecble with JavaSystem.LoadLibrary first. Pass the Java VM invocation pointer
        /// and an application Context JNI handle; the native backend retains the context.
        /// </summary>
        public static void Initialize(nint javaVm, nint applicationContext)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Android);
            if (javaVm == 0) throw new ArgumentException("Java VM cannot be null.", nameof(javaVm));
            if (applicationContext == 0) throw new ArgumentException("Application context cannot be null.", nameof(applicationContext));
            NativeCall.Invoke((ref nint error) =>
            {
                NativeMethods.simpleble_advanced_android_set_jvm(javaVm, ref error);
                return 0;
            });
            NativeCall.Invoke((ref nint error) =>
            {
                NativeMethods.simpleble_advanced_android_set_context(applicationContext, ref error);
                return 0;
            });
            // Preload Java callback classes on the application thread before worker calls.
            nint backends = 0;
            NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_backend_get_backends(out backends, ref error));
            NativeMethods.simpleble_backend_list_release(backends);
        }
    }
    /// <summary>macOS CoreBluetooth extensions.</summary>
    public static class MacOS
    {
        /// <summary>Sets the local name advertised by a local peripheral, or null to remove the override. Call before Start.</summary>
        public static void SetAdvertisementLocalName(Local.Peripheral peripheral, string? name)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.MacOS);
            SetName(peripheral, name, NativeMethods.simpleble_advanced_macos_set_advertisement_local_name);
        }
        /// <summary>Returns already-known peripherals by CoreBluetooth identifier without scanning. Unknown identifiers are skipped; dispose each result.</summary>
        public static IReadOnlyList<Peripheral> RetrieveCachedPeripherals(Adapter adapter, IEnumerable<string> identifiers)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.MacOS);
            return Retrieve(adapter, identifiers, NativeMethods.simpleble_advanced_macos_retrieve_cached_peripheral);
        }
    }
    /// <summary>iOS, iPadOS, and Mac Catalyst CoreBluetooth extensions.</summary>
    public static class IOS
    {
        /// <summary>Sets the local name advertised by a local peripheral, or null to remove the override. Call before Start.</summary>
        public static void SetAdvertisementLocalName(Local.Peripheral peripheral, string? name)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.IOS);
            SetName(peripheral, name, NativeMethods.simpleble_advanced_ios_set_advertisement_local_name);
        }
        /// <summary>Returns already-known peripherals by CoreBluetooth identifier without scanning. Unknown identifiers are skipped; dispose each result.</summary>
        public static IReadOnlyList<Peripheral> RetrieveCachedPeripherals(Adapter adapter, IEnumerable<string> identifiers)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.IOS);
            return Retrieve(adapter, identifiers, NativeMethods.simpleble_advanced_ios_retrieve_cached_peripheral);
        }
    }
    /// <summary>
    /// Linux BlueZ extensions. The pairing callbacks of an adapter cover the peripherals it connects to and the clients
    /// of its local peripherals, and receive the peer's address. Passkey and numeric comparison requests reach them when
    /// Config.SimpleBluez.InteractivePairing is set. Adapters of other backends, such as Dongl, report OperationNotSupported.
    /// </summary>
    public static class Linux
    {
        private static readonly NativeMethods.PeerPairCallback request = Request;
        private static readonly NativeMethods.PeerTextCallback display = Display;
        private static readonly NativeMethods.PeerPairCallback compare = Compare;
        private static readonly NativeMethods.PairCallback authorize = Authorize;
        [MonoPInvokeCallback(typeof(NativeMethods.PeerPairCallback))]
        private static bool Request(nint handle, nint peer, nint buffer, nint token)
        {
            try { return WritePasskey(CallbackSlot.Find(token)?.Invoke(Marshal.PtrToStringUTF8(peer) ?? ""), buffer); }
            catch (Exception ex) { CallbackErrors.Report(ex); return false; }
        }
        [MonoPInvokeCallback(typeof(NativeMethods.PeerTextCallback))]
        private static void Display(nint handle, nint peer, nint text, nint token) => NativeCallbacks.Guard(() =>
            CallbackSlot.Post(token, (Marshal.PtrToStringUTF8(peer) ?? "", Marshal.PtrToStringUTF8(text) ?? "")));
        [MonoPInvokeCallback(typeof(NativeMethods.PeerPairCallback))]
        private static bool Compare(nint handle, nint peer, nint text, nint token)
        {
            try { return CallbackSlot.Find(token)?.Invoke((Marshal.PtrToStringUTF8(peer) ?? "", Marshal.PtrToStringUTF8(text) ?? "")) is true; }
            catch (Exception ex) { CallbackErrors.Report(ex); return false; }
        }
        [MonoPInvokeCallback(typeof(NativeMethods.PairCallback))]
        private static bool Authorize(nint handle, nint peer, nint token)
        {
            try { return CallbackSlot.Find(token)?.Invoke(Marshal.PtrToStringUTF8(peer) ?? "") is true; }
            catch (Exception ex) { CallbackErrors.Report(ex); return false; }
        }
        /// <summary>Sets the local name advertised by a local peripheral, or null to remove the override. Call before Start.</summary>
        public static void SetAdvertisementLocalName(Local.Peripheral peripheral, string? name)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Linux);
            SetName(peripheral, name, NativeMethods.simpleble_advanced_linux_set_advertisement_local_name);
        }
        /// <summary>
        /// Receives the peer's address and returns the six-digit passkey the peer shows, or null to reject. Runs synchronously
        /// on SimpleBLE's BlueZ event thread, which waits for it. Pass null to unregister.
        /// </summary>
        public static void SetPasskeyRequestCallback(Adapter adapter, Func<string, string?>? callback)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Linux);
            if (adapter is null) throw new ArgumentNullException(nameof(adapter));
            adapter.callbacks.Set("pair-request", callback is null ? null : peer => callback((string)peer!), (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_linux_set_passkey_request_callback(h, id == 0 ? null : request, id, ref e); return 0; }));
        }
        /// <summary>Receives the peer's address and the six-digit passkey to show to the user. Pass null to unregister.</summary>
        public static void SetPasskeyDisplayCallback(Adapter adapter, Action<string, string>? callback)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Linux);
            if (adapter is null) throw new ArgumentNullException(nameof(adapter));
            adapter.callbacks.Set("pair-display", callback is null ? null : value =>
            { var (peer, passkey) = ((string, string))value!; callback(peer, passkey); return null; }, (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_linux_set_passkey_display_callback(h, id == 0 ? null : display, id, ref e); return 0; }));
        }
        /// <summary>
        /// Receives the peer's address and the six digits to compare. Runs synchronously on SimpleBLE's BlueZ event thread,
        /// which waits for it. Return true only after the user confirms the peer shows the same digits. Pass null to unregister.
        /// </summary>
        public static void SetNumericComparisonCallback(Adapter adapter, Func<string, string, bool>? callback)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Linux);
            if (adapter is null) throw new ArgumentNullException(nameof(adapter));
            adapter.callbacks.Set("pair-compare", callback is null ? null : value =>
            { var (peer, passkey) = ((string, string))value!; return callback(peer, passkey); }, (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_linux_set_numeric_comparison_callback(h, id == 0 ? null : compare, id, ref e); return 0; }));
        }
        /// <summary>
        /// Receives the address of a peer pairing with Just Works. Runs synchronously on SimpleBLE's BlueZ event thread, which
        /// waits for it. Return false to reject. Without this callback, Just Works pairing is accepted. Pass null to unregister.
        /// </summary>
        public static void SetPairingAuthorizationCallback(Adapter adapter, Func<string, bool>? callback)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Linux);
            if (adapter is null) throw new ArgumentNullException(nameof(adapter));
            adapter.callbacks.Set("pair-authorize", callback is null ? null : peer => callback((string)peer!), (h, id) =>
                NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_advanced_linux_set_pairing_authorization_callback(h, id == 0 ? null : authorize, id, ref e); return 0; }));
        }
    }
    private static void Require(bool condition)
    {
        if (!condition) throw new PlatformNotSupportedException("This advanced API requires its corresponding native platform backend.");
    }
    // Copies the passkey a request callback returned into SimpleCBLE's buffer. Null rejects pairing.
    private static bool WritePasskey(object? value, nint buffer)
    {
        if (value is not string passkey) return false;
        if (passkey.Length != 6 || passkey.Any(c => c < '0' || c > '9'))
            throw new ArgumentException("A passkey must contain exactly six decimal digits.");
        Marshal.Copy(Encoding.ASCII.GetBytes(passkey + "\0"), 0, buffer, 7);
        return true;
    }
    private delegate void NameSetter(nint handle, string? name, ref nint error);
    private static void SetName(Local.Peripheral peripheral, string? name, NameSetter setter)
    {
        if (peripheral is null) throw new ArgumentNullException(nameof(peripheral));
        if (name?.Contains('\0') == true) throw new ArgumentException("Name cannot contain NUL.", nameof(name));
        peripheral.handle.Execute((nint h, ref nint e) => setter(h, name, ref e));
    }
    private delegate nint Retriever(nint adapter, string identifier, ref nint error);
    private static IReadOnlyList<Peripheral> Retrieve(Adapter adapter, IEnumerable<string> identifiers, Retriever retrieve)
    {
        if (adapter is null) throw new ArgumentNullException(nameof(adapter)); if (identifiers is null) throw new ArgumentNullException(nameof(identifiers));
        var result = new List<Peripheral>();
        try
        {
            foreach (string id in identifiers)
            {
                if (id is null) throw new ArgumentNullException(nameof(id));
                if (id.Contains('\0')) throw new ArgumentException("Identifier cannot contain NUL.", nameof(identifiers));
                nint value = adapter.handle.Query((nint h, ref nint e) => retrieve(h, id, ref e));
                if (value != 0) result.Add(new(value));
            }
            return result.AsReadOnly();
        }
        catch { foreach (var value in result) value.Dispose(); throw; }
    }
}
