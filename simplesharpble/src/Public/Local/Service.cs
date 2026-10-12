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
            {
                var seen = new Dictionary<string, int>();
                return NativeCall.Collect(handle.Query(NativeMethods.simpleble_local_service_characteristics_count), index =>
                {
                    nint characteristic = handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_service_characteristics_get(h, index, ref e));
                    NativeUuid uuid = default;
                    try { NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_local_characteristic_uuid(characteristic, out uuid, ref e); return 0; }); }
                    catch { NativeMethods.simpleble_local_characteristic_release_handle(characteristic); throw; }
                    return new Characteristic(characteristic, children.At(uuid.ToString(), seen));
                });
            }
        }
    }
    public Characteristic AddCharacteristic(string uuid, CharacteristicCapability capabilities)
    {
        if (capabilities == 0 || ((uint)capabilities & ~31u) != 0) throw new ArgumentOutOfRangeException(nameof(capabilities));
        var value = new NativeUuid(uuid);
        lock (children)
        {
            nint characteristic = handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_service_add_characteristic(h, value, (uint)capabilities, ref e));
            return new(characteristic, children.Add(value.ToString()));
        }
    }
    public void Dispose() => handle.Dispose();
}
