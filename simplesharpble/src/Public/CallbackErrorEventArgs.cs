namespace SimpleSharpBLE;

public sealed class CallbackErrorEventArgs(Exception exception) : EventArgs
{
    public Exception Exception { get; } = exception;
}

/// <summary>Reports contained exceptions from application callback handlers.</summary>
public static class CallbackErrors
{
    public static event EventHandler<CallbackErrorEventArgs>? Unhandled;
    internal static void Report(Exception error)
        => ThreadPool.QueueUserWorkItem(_ => Dispatch(error));
    private static void Dispatch(Exception error)
    {
        var handlers = Unhandled;
        if (handlers is null)
        {
            try { System.Diagnostics.Trace.TraceError(error.ToString()); } catch { /* Diagnostic listeners cannot escape. */ }
            return;
        }
        foreach (EventHandler<CallbackErrorEventArgs> handler in handlers.GetInvocationList())
            try { handler(null, new(error)); } catch { /* Never reenter native code with an exception. */ }
    }
}

public sealed class PeripheralEventArgs(Peripheral peripheral) : EventArgs
{
    /// <summary>An owned wrapper shared by this event's handlers. The receiver must dispose it.</summary>
    public Peripheral Peripheral { get; } = peripheral;
}
