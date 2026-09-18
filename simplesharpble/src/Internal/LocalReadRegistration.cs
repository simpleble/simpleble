using System.Collections.Concurrent;
using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Internal;

// Local GATT children are append-only and enumerated in insertion order.
// Index-based state keeps aliases together even when UUIDs are duplicated.
internal sealed class LocalChildren<T> where T : new()
{
    private readonly List<T> items = [];
    internal T At(nuint index)
    {
        int position = checked((int)index);
        while (items.Count <= position) items.Add(new());
        return items[position];
    }
    internal void Clear() => items.Clear();
}

internal sealed class LocalReadState
{
    internal nint Current;
}

internal sealed class LocalReadRegistration : IDisposable
{
    private static long nextId;
    private static readonly ConcurrentDictionary<nint, WeakReference<LocalReadRegistration>> registrations = new();
    private static readonly NativeMethods.ReadCallback onRead = Read;
    [ThreadStatic] private static bool reading;
    private readonly nint id = checked((nint)Interlocked.Increment(ref nextId));
    private readonly NativeHandle handle;
    private readonly LocalReadState state;
    private bool acquired, disposed;
    private nint buffer;
    internal Func<byte[]>? Handler { get; private set; }

    private LocalReadRegistration(NativeHandle handle, LocalReadState state, Func<byte[]> handler)
    {
        this.handle = handle;
        this.state = state;
        Handler = handler;
        handle.DangerousAddRef(ref acquired);
        registrations[id] = new(this);
    }

    internal static void CheckMutation()
    {
        if (reading) throw new InvalidOperationException("Cannot replace or dispose a read handler from a read callback.");
    }

    internal static LocalReadRegistration? Set(NativeHandle handle, LocalReadState state, Func<byte[]>? handler)
    {
        CheckMutation();
        lock (state)
        {
            LocalReadRegistration? next = handler is null ? null : new(handle, state, handler);
            try
            {
                handle.Query((nint h, ref nint e) =>
                {
                    NativeMethods.simpleble_local_characteristic_set_callback_on_read(h, next is null ? null : onRead, next?.id ?? 0);
                    return 0;
                });
            }
            catch { next?.Dispose(); throw; }
            nint previous = state.Current;
            state.Current = next?.id ?? 0;
            if (registrations.TryGetValue(previous, out var weak) && weak.TryGetTarget(out var old)) old.Dispose();
            return next;
        }
    }

    private static nint Read(nint _, out nuint length, nint token)
    {
        length = 0;
        if (!registrations.TryGetValue(token, out var weak) || !weak.TryGetTarget(out var registration)) return 0;
        bool wasReading = reading;
        reading = true;
        try
        {
            if (wasReading) throw new InvalidOperationException("A read handler cannot perform a nested dynamic read.");
            // Native serialization keeps this buffer alive through the C bridge's copy.
            Marshal.FreeHGlobal(registration.buffer);
            registration.buffer = 0;
            var bytes = registration.Handler?.Invoke();
            if (bytes is null || bytes.Length == 0) return 0;
            registration.buffer = Marshal.AllocHGlobal(bytes.Length);
            Marshal.Copy(bytes, 0, registration.buffer, bytes.Length);
            length = (nuint)bytes.Length;
            return registration.buffer;
        }
        catch (Exception ex) { CallbackErrors.Report(ex); return 0; }
        finally { reading = wasReading; GC.KeepAlive(registration); }
    }

    public void Dispose()
    {
        CheckMutation();
        lock (state)
        {
            if (disposed) return;
            if (state.Current == id)
            {
                // Clearing waits for the native callback and its result copy to finish.
                NativeMethods.simpleble_local_characteristic_set_callback_on_read(handle.DangerousGetHandle(), null, 0);
                state.Current = 0;
            }
            disposed = true;
            registrations.TryRemove(id, out _);
            Handler = null;
            Marshal.FreeHGlobal(buffer);
            buffer = 0;
            if (acquired) { acquired = false; handle.DangerousRelease(); }
        }
        GC.SuppressFinalize(this);
    }

    ~LocalReadRegistration() => Dispose();
}
