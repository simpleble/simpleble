import { StyleSheet, Text, View } from 'react-native';
import { useState, useEffect } from 'react';
import { HybridAdapter } from 'simplejsble';

export default function HomeScreen() {
  const [greeting, setGreeting] = useState<string>('Hello Again!');
  const [isBluetoothEnabled, setIsBluetoothEnabled] = useState<boolean>(false);
  const [adapters, setAdapters] = useState<any[]>([]);


  useEffect(() => {
    const message = HybridAdapter.greet('Android');
    setGreeting(message);

    const isBluetoothEnabled = HybridAdapter.bluetooth_enabled();
    console.log('isBluetoothEnabled', isBluetoothEnabled);
    setIsBluetoothEnabled(isBluetoothEnabled);

    const adapters = HybridAdapter.get_adapters();
    console.log('adapters', adapters);
    setAdapters(adapters);

  }, []);

  return (
    <View style={styles.container}>
      {greeting ? <Text style={styles.greeting}>{greeting}</Text> : null}
      {isBluetoothEnabled ? <Text style={styles.greeting}>Bluetooth is enabled</Text> : <Text style={styles.greeting}>Bluetooth is disabled</Text>}
      {adapters.map((adapter) => <Text key={adapter.name} style={styles.greeting}>{adapter.name}</Text>)}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  greeting: {
    marginTop: 16,
    fontSize: 18,
    fontWeight: '600',
  },
});
