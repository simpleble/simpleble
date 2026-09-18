using System.Collections.Concurrent;

namespace SimpleSharpBLE.Internal;

// Non-reused IDs let late callbacks identify expired registrations.
internal sealed class CallbackSlot : IDisposable
{
    private static long nextId;
    private static readonly ConcurrentDictionary<nint, WeakReference<CallbackSlot>> slots = new();
    internal nint Id { get; } = checked((nint)Interlocked.Increment(ref nextId));
    internal Func<object?, object?> Handler { get; }
    private readonly CallbackDispatcher dispatcher;
    private int disposed;
    internal bool IsDisposed => Volatile.Read(ref disposed) != 0;

    internal CallbackSlot(Func<object?, object?> handler, CallbackDispatcher? dispatcher = null)
    {
        Handler = handler;
        this.dispatcher = dispatcher ?? new();
        slots[Id] = new(this);
    }

    internal static CallbackSlot? Find(nint id) => slots.TryGetValue(id, out var weak) && weak.TryGetTarget(out var slot) ? slot : null;
    internal static void Post(nint id, object? value = null, Action? drop = null)
    {
        var slot = Find(id);
        if (slot is null) { drop?.Invoke(); return; }
        slot.dispatcher.Post(slot, value, drop);
    }

    internal object? Invoke(object? value)
    {
        if (IsDisposed) return null;
        try { return Handler(value); }
        catch (Exception ex) { CallbackErrors.Report(ex); return null; }
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) != 0) return;
        slots.TryRemove(Id, out _);
        dispatcher.Remove(this);
        GC.SuppressFinalize(this);
    }
    ~CallbackSlot() => Dispose();
}

// One queue per wrapper preserves arrival order across registrations.
internal sealed class CallbackDispatcher
{
    private readonly object gate = new();
    private readonly Queue<(CallbackSlot Slot, object? Value, Action? Drop)> pending = new();
    private bool running;
    internal void Post(CallbackSlot slot, object? value, Action? drop)
    {
        lock (gate)
        {
            if (slot.IsDisposed) { drop?.Invoke(); return; }
            pending.Enqueue((slot, value, drop));
            if (running) return;
            running = true;
            ThreadPool.QueueUserWorkItem(_ => Drain());
        }
    }
    internal void Remove(CallbackSlot slot)
    {
        lock (gate)
        {
            int count = pending.Count;
            for (int i = 0; i < count; i++)
            {
                var work = pending.Dequeue();
                if (ReferenceEquals(work.Slot, slot)) work.Drop?.Invoke();
                else pending.Enqueue(work);
            }
        }
    }
    private void Drain()
    {
        while (true)
        {
            (CallbackSlot Slot, object? Value, Action? Drop) work;
            lock (gate)
            {
                if (pending.Count == 0) { running = false; return; }
                work = pending.Dequeue();
                if (work.Slot.IsDisposed) { work.Drop?.Invoke(); continue; }
            }
            // Run outside the lock so handlers can unsubscribe or dispose.
            try { work.Slot.Handler(work.Value); }
            catch (Exception ex) { CallbackErrors.Report(ex); }
        }
    }

}

internal sealed class CallbackOwner(NativeHandle handle) : IDisposable
{
    private readonly object gate = new();
    private readonly Dictionary<string, CallbackSlot> registrations = new();
    private readonly CallbackDispatcher dispatcher = new();
    private bool disposed;

    internal void Set(string key, Func<object?, object?>? action, Action<nint, nint> install)
    {
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            CallbackSlot? slot = action is null ? null : new(action, dispatcher);
            try { handle.Query((nint h, ref nint e) => { install(h, slot?.Id ?? 0); return 0; }); }
            catch { slot?.Dispose(); throw; }
            if (registrations.Remove(key, out var previous)) previous.Dispose();
            if (slot is not null) registrations.Add(key, slot);
        }
    }

    public void Dispose()
    {
        lock (gate)
        {
            if (disposed) return;
            disposed = true;
            foreach (var slot in registrations.Values) slot.Dispose();
            registrations.Clear();
            // Native slots may now belong to another wrapper. Expire only our IDs.
            handle.Dispose();
        }
    }
}

internal static class Events
{
    internal static void Raise(EventHandler? handlers, object sender)
    {
        if (handlers is null) return;
        foreach (EventHandler handler in handlers.GetInvocationList())
            try { handler(sender, EventArgs.Empty); } catch (Exception ex) { CallbackErrors.Report(ex); }
    }
    internal static void Raise<T>(EventHandler<T>? handlers, object sender, T args)
    {
        if (handlers is null) return;
        foreach (EventHandler<T> handler in handlers.GetInvocationList())
            try { handler(sender, args); } catch (Exception ex) { CallbackErrors.Report(ex); }
    }
}
