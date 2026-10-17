import { useCallback, useEffect, useState } from 'react';
import { Button, FlatList, PermissionsAndroid, Platform, Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';
import { SafeAreaProvider, useSafeAreaInsets } from 'react-native-safe-area-context';
import { Adapter, type Characteristic, type Peripheral } from 'simplernble';

// Android asks the user for the Bluetooth permissions at runtime.
async function requestPermissions(): Promise<boolean> {
  if (Platform.OS !== 'android') return true;
  const result = await PermissionsAndroid.requestMultiple([
    PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
    PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
  ]);
  return Object.values(result).every(status => status === PermissionsAndroid.RESULTS.GRANTED);
}

const hex = (value: Uint8Array) => Array.from(value, byte => byte.toString(16).padStart(2, '0')).join(' ');

export default function App() {
  return (
    <SafeAreaProvider>
      <Screen />
    </SafeAreaProvider>
  );
}

function Screen() {
  const insets = useSafeAreaInsets();
  const [selected, setSelected] = useState<Peripheral>();
  return (
    <View style={[styles.screen, { paddingTop: insets.top + 16, paddingBottom: insets.bottom }]}>
      {selected ? <PeripheralView peripheral={selected} onClose={() => setSelected(undefined)} /> : <Scanner onSelect={setSelected} />}
    </View>
  );
}

// Scans and lists the peripherals found.
function Scanner({ onSelect }: { onSelect: (peripheral: Peripheral) => void }) {
  const [adapter, setAdapter] = useState<Adapter>();
  const [status, setStatus] = useState('Looking for adapters...');
  const [peripherals, setPeripherals] = useState<Peripheral[]>([]);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    (async () => {
      if (!(await requestPermissions())) return setStatus('Bluetooth permission denied.');
      const [first] = await Adapter.getAdapters();
      if (!first) return setStatus('No Bluetooth adapters found.');
      setAdapter(first);
      setPeripherals(first.scanGetResults());
      setStatus(`Adapter: ${first.identifier()} [${first.address()}]`);
    })().catch(error => setStatus(String(error)));
  }, []);

  useEffect(() => {
    if (!adapter) return;
    const update = () => setPeripherals(adapter.scanGetResults());
    const start = () => setScanning(true);
    const stop = () => setScanning(false);
    adapter.on('scanFound', update).on('scanUpdated', update).on('scanStart', start).on('scanStop', stop);
    return () => {
      adapter.off('scanFound', update).off('scanUpdated', update).off('scanStart', start).off('scanStop', stop);
    };
  }, [adapter]);

  const scan = useCallback(() => {
    adapter?.scanFor(5000).catch(error => setStatus(String(error)));
  }, [adapter]);

  return (
    <>
      <Text style={styles.status}>{status}</Text>
      <Button title={scanning ? 'Scanning...' : 'Scan for 5 seconds'} onPress={scan} disabled={!adapter || scanning} />
      <FlatList
        data={peripherals}
        keyExtractor={peripheral => peripheral.address()}
        renderItem={({ item }) => (
          <Pressable style={styles.row} onPress={() => onSelect(item)} disabled={!item.isConnectable()}>
            <Text style={styles.name}>{item.identifier() || '(unnamed)'}</Text>
            <Text>
              {item.address()} {item.rssi() ?? '?'} dBm{item.isConnectable() ? '' : ', not connectable'}
            </Text>
          </Pressable>
        )}
      />
    </>
  );
}

// Connects to a peripheral and lists its services and characteristics.
function PeripheralView({ peripheral, onClose }: { peripheral: Peripheral; onClose: () => void }) {
  const [status, setStatus] = useState('Connecting...');
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const disconnected = () => setStatus('Disconnected.');
    peripheral.on('disconnected', disconnected);
    peripheral
      .connect()
      .then(() => {
        setConnected(true);
        setStatus(`Connected, MTU ${peripheral.mtu()}`);
      })
      .catch(error => setStatus(String(error)));
    return () => {
      peripheral.off('disconnected', disconnected);
      peripheral.disconnect().catch(() => {});
    };
  }, [peripheral]);

  return (
    <>
      <Text style={styles.name}>{peripheral.identifier() || peripheral.address()}</Text>
      <Text style={styles.status}>{status}</Text>
      <Button title="Disconnect" onPress={onClose} />
      <ScrollView>
        {connected &&
          peripheral.services().map(service => (
            <View key={service.uuid} style={styles.row}>
              <Text style={styles.name}>Service {service.uuid}</Text>
              {service.characteristics.map(characteristic => (
                <CharacteristicView
                  key={characteristic.uuid}
                  peripheral={peripheral}
                  service={service.uuid}
                  characteristic={characteristic}
                />
              ))}
            </View>
          ))}
      </ScrollView>
    </>
  );
}

// Reads a characteristic, or follows its notifications.
function CharacteristicView(props: { peripheral: Peripheral; service: string; characteristic: Characteristic }) {
  const { peripheral, service, characteristic } = props;
  const [value, setValue] = useState('');
  const [following, setFollowing] = useState(false);
  const canRead = characteristic.capabilities.includes('read');
  const canNotify = characteristic.capabilities.includes('notify');

  const read = () =>
    peripheral
      .read(service, characteristic.uuid)
      .then(data => setValue(hex(data)))
      .catch(error => setValue(String(error)));

  const toggle = () => {
    const change = following
      ? peripheral.unsubscribe(service, characteristic.uuid)
      : peripheral.notify(service, characteristic.uuid, data => setValue(hex(data)));
    change.then(() => setFollowing(!following)).catch(error => setValue(String(error)));
  };

  return (
    <View style={styles.characteristic}>
      <Text>{characteristic.uuid}</Text>
      <Text style={styles.capabilities}>{characteristic.capabilities.join(', ')}</Text>
      <View style={styles.actions}>
        {canRead && <Button title="Read" onPress={read} />}
        {canNotify && <Button title={following ? 'Stop' : 'Notify'} onPress={toggle} />}
      </View>
      {value !== '' && <Text style={styles.value}>{value}</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, paddingHorizontal: 16 },
  status: { marginVertical: 8 },
  row: { paddingVertical: 8, borderBottomWidth: StyleSheet.hairlineWidth },
  name: { fontWeight: '600' },
  characteristic: { marginTop: 8, marginLeft: 8 },
  capabilities: { color: 'gray' },
  actions: { flexDirection: 'row', gap: 8 },
  value: { fontFamily: Platform.select({ ios: 'Menlo', default: 'monospace' }) },
});
