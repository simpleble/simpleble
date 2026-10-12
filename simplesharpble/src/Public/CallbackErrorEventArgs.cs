namespace SimpleSharpBLE;

/// <summary>Carries an exception thrown by an application callback.</summary>
public sealed class CallbackErrorEventArgs(Exception exception) : EventArgs
{
    /// <summary>The exception the callback threw.</summary>
    public Exception Exception { get; } = exception;
}

/// <summary>Reports contained exceptions from application callback handlers.</summary>
public static class CallbackErrors
{
    /// <summary>Raised on a worker thread for each contained exception. Without handlers, exceptions go to Trace.</summary>
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

/// <summary>Carries a peripheral reported by a scan event.</summary>
public sealed class PeripheralEventArgs(Peripheral peripheral) : EventArgs
{
    /// <summary>An owned wrapper shared by this event's handlers. The receiver must dispose it.</summary>
    public Peripheral Peripheral { get; } = peripheral;
}
