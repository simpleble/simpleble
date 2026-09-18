using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal.Interop;
using Xunit;

namespace SimpleSharpBLE.Tests;

public sealed class GattTests
{
    internal const string Uuid = "0000180f-0000-1000-8000-00805f9b34fb";
    [Fact]
    public void ManagedLayoutsMatchTheCompiledCAbi()
    {
        nuint[] expected = [
            (nuint)Marshal.SizeOf<NativeUuid>(), (nuint)Marshal.SizeOf<NativeDescriptor>(),
            (nuint)Marshal.SizeOf<NativeCharacteristic>(), (nuint)Marshal.SizeOf<NativeService>(),
            (nuint)Marshal.SizeOf<NativeManufacturerData>(),
            (nuint)Marshal.OffsetOf<NativeCharacteristic>("DescriptorCount"),
            (nuint)Marshal.OffsetOf<NativeCharacteristic>("Descriptors"),
            (nuint)Marshal.OffsetOf<NativeService>("DataLength"), (nuint)Marshal.OffsetOf<NativeService>("Data"),
            (nuint)Marshal.OffsetOf<NativeService>("CharacteristicCount"), (nuint)Marshal.OffsetOf<NativeService>("Characteristics"),
            (nuint)Marshal.OffsetOf<NativeManufacturerData>("DataLength"), (nuint)Marshal.OffsetOf<NativeManufacturerData>("Data")];
        for (int i = 0; i < expected.Length; i++) Assert.Equal(expected[i], NativeFixture.test_layout(i));
    }

    [Fact]
    public void SnapshotsContainFullPayloadsAndSurviveNativeRelease()
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        Assert.Equal("Fixture é", peripheral.Identifier);
        var advertised = Assert.Single(peripheral.Services);
        Assert.Equal(257, advertised.Data.Length);
        Assert.Empty(advertised.Characteristics);
        var manufacturer = peripheral.ManufacturerData;
        peripheral.Connect();
        var service = Assert.Single(peripheral.Services);
        var characteristic = Assert.Single(service.Characteristics);
        Assert.Equal(Uuid, Assert.Single(characteristic.Descriptors).Uuid);
        Assert.Equal(5, characteristic.Capabilities.Count);
        peripheral.Disconnect();
        peripheral.Dispose();
        Assert.Equal(Enumerable.Range(0, 257).Select(x => (byte)x), advertised.Data);
        Assert.Equal(advertised.Data, manufacturer[0x1234]);
        Assert.Equal(Uuid, characteristic.Uuid);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(27)]
    [InlineData(28)]
    [InlineData(257)]
    [InlineData(65537)]
    public void ReadWriteOverloadsPreserveBinaryPayloads(int length)
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        Assert.Equal(BleErrorCode.PeripheralNotConnected, Assert.Throws<BleException>(() => peripheral.Read(Uuid, Uuid)).Code);
        peripheral.Connect();
        var bytes = Enumerable.Range(0, length).Select(x => (byte)x).ToArray();
        peripheral.WriteRequest(Uuid, Uuid, bytes);
        Assert.Equal(bytes, peripheral.Read(Uuid, Uuid));
        peripheral.WriteCommand(Uuid, Uuid, bytes);
        Assert.Equal(bytes, peripheral.Read(Uuid, Uuid));
        peripheral.Write(Uuid, Uuid, Uuid, bytes);
        Assert.Equal(bytes, peripheral.Read(Uuid, Uuid, Uuid));
    }

    [Fact]
    public void InvalidInputsNeverReachNativeUuidBuffers()
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        Assert.Throws<ArgumentException>(() => peripheral.Read(new string('a', 37), Uuid));
        Assert.Throws<ArgumentException>(() => peripheral.Read("123\0", Uuid));
        Assert.Throws<ArgumentNullException>(() => peripheral.WriteCommand(Uuid, Uuid, null!));
        Assert.Equal("abcd", new NativeUuid("ABCD").ToString());
    }
}
