using System.Runtime.InteropServices;
using System.Text;

namespace SimpleSharpBLE.Internal.Interop;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeUuid
{
    internal fixed byte Value[37];
    internal NativeUuid(string value)
    {
        if (value is null) throw new ArgumentNullException(nameof(value));
        // C++ accepts short Bluetooth UUIDs as well as full UUID strings.
        if ((value.Length != 4 && value.Length != 8 && value.Length != 36) ||
            (value.Length == 36 ? !Guid.TryParseExact(value, "D", out _) : !value.All(Uri.IsHexDigit)))
            throw new ArgumentException("Expected a 4/8-digit Bluetooth UUID or a canonical 128-bit UUID.", nameof(value));
        this = default;
        fixed (byte* ptr = Value) Encoding.ASCII.GetBytes(value.ToLowerInvariant(), new Span<byte>(ptr, 36));
    }
    public override string ToString()
    {
        fixed (byte* ptr = Value)
        {
            var bytes = new ReadOnlySpan<byte>(ptr, 37);
            int end = bytes.IndexOf((byte)0);
            return Encoding.ASCII.GetString(end < 0 ? bytes : bytes[..end]);
        }
    }
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeDescriptor { internal NativeUuid Uuid; }
[StructLayout(LayoutKind.Sequential)]
internal struct NativeCharacteristic
{
    internal NativeUuid Uuid;
    internal byte CanRead, CanWriteRequest, CanWriteCommand, CanNotify, CanIndicate;
    internal nuint DescriptorCount;
    internal nint Descriptors;
}
[StructLayout(LayoutKind.Sequential)]
internal struct NativeService
{
    internal NativeUuid Uuid;
    internal nuint DataLength;
    internal nint Data;
    internal nuint CharacteristicCount;
    internal nint Characteristics;
}
[StructLayout(LayoutKind.Sequential)]
internal struct NativeManufacturerData
{
    internal ushort ManufacturerId;
    internal nuint DataLength;
    internal nint Data;
}
