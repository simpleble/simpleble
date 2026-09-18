using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Local;

public sealed class Service : IDisposable
{
    private readonly NativeHandle handle;
    private readonly LocalChildren<LocalReadState> children;
    internal Service(nint pointer, LocalChildren<LocalReadState> children)
    {
        handle = new(pointer, HandleKind.LocalService);
        this.children = children;
    }
    public string Uuid
    {
        get
        {
            NativeUuid uuid = default;
            handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_service_uuid(h, out uuid, ref e));
            return uuid.ToString();
        }
    }
    public IReadOnlyList<Characteristic> Characteristics
    {
        get
        {
            lock (children)
                return NativeCall.Collect(handle.Query(NativeMethods.simpleble_local_service_characteristics_count),
                    index => new Characteristic(handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_service_characteristics_get(h, index, ref e)), children.At(index)));
        }
    }
    public Characteristic AddCharacteristic(string uuid, CharacteristicCapability capabilities)
    {
        if (capabilities == 0 || ((uint)capabilities & ~31u) != 0) throw new ArgumentOutOfRangeException(nameof(capabilities));
        var value = new NativeUuid(uuid);
        lock (children)
        {
            var state = children.At(handle.Query(NativeMethods.simpleble_local_service_characteristics_count));
            return new(handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_service_add_characteristic(h, value, (uint)capabilities, ref e)), state);
        }
    }
    public void Dispose() => handle.Dispose();
}
