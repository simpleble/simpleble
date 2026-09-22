using System.Runtime.InteropServices;
using System.Text;
using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

public static class Advanced
{
    public static class Dongl
    {
        private static readonly NativeMethods.PairCallback request = Request;
        private static readonly NativeMethods.PairCallback compare = Compare;
        [MonoPInvokeCallback(typeof(NativeMethods.PairCallback))]
        private static bool Request(nint handle, nint buffer, nint token)
        {
            try
            {
                if (CallbackSlot.Find(token)?.Invoke(null) is not string value) return false;
                if (value.Length != 6 || value.Any(c => c < '0' || c > '9'))
                    throw new ArgumentException("A passkey must contain exactly six decimal digits.");
                Marshal.Copy(Encoding.ASCII.GetBytes(value + "\0"), 0, buffer, 7);
                return true;
            }
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
    }

    public static class Android
    {
        /// <summary>
        /// Initializes Android on the application thread after Bluetooth is enabled and permissions are granted.
        /// Load simpleble with JavaSystem.LoadLibrary first. Pass the Java VM invocation pointer
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
            NativeCall.Invoke(NativeMethods.simpleble_backend_get_count);
        }
    }
    public static class MacOS
    {
        public static void SetAdvertisementLocalName(Local.Peripheral peripheral, string? name)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.MacOS);
            SetName(peripheral, name, NativeMethods.simpleble_advanced_macos_set_advertisement_local_name);
        }
        public static IReadOnlyList<Peripheral> RetrieveCachedPeripherals(Adapter adapter, IEnumerable<string> identifiers)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.MacOS);
            return Retrieve(adapter, identifiers, NativeMethods.simpleble_advanced_macos_retrieve_cached_peripheral);
        }
    }
    public static class IOS
    {
        public static void SetAdvertisementLocalName(Local.Peripheral peripheral, string? name)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.IOS);
            SetName(peripheral, name, NativeMethods.simpleble_advanced_ios_set_advertisement_local_name);
        }
        public static IReadOnlyList<Peripheral> RetrieveCachedPeripherals(Adapter adapter, IEnumerable<string> identifiers)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.IOS);
            return Retrieve(adapter, identifiers, NativeMethods.simpleble_advanced_ios_retrieve_cached_peripheral);
        }
    }
    public static class Linux
    {
        public static void SetAdvertisementLocalName(Local.Peripheral peripheral, string? name)
        {
            Require(Utils.OperatingSystem == NativeOperatingSystem.Linux);
            SetName(peripheral, name, NativeMethods.simpleble_advanced_linux_set_advertisement_local_name);
        }
    }
    private static void Require(bool condition)
    {
        if (!condition) throw new PlatformNotSupportedException("This advanced API requires its corresponding native platform backend.");
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
