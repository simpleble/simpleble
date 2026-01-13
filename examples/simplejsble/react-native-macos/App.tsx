import {StyleSheet, Text, View} from 'react-native';
import {useState, useEffect, useRef} from 'react';
import {HybridAdapter} from 'simplejsble';

interface AdapterInfo {
  name: string;
  index: number;
}

export default function HomeScreen() {
  const [greeting, setGreeting] = useState<string>('');
  const [isBluetoothEnabled, setIsBluetoothEnabled] = useState<boolean>(false);
  const [adapters, setAdapters] = useState<AdapterInfo[]>([]);
  const hasInitialized = useRef(false);
  // Store full adapter references in a ref to avoid Fast Refresh issues
  // Access via: adapterRefs.current[index]
  const adapterRefs = useRef<any[]>([]);
  const adapterMapByName = useRef<Map<string, any>>(new Map());

  // Helper function to get adapter by index
  const getAdapterByIndex = (index: number) => {
    return adapterRefs.current[index];
  };

  useEffect(() => {
    // Prevent multiple initializations that could cause infinite reloads
    if (hasInitialized.current) {
      return;
    }
    hasInitialized.current = true;

    const message = HybridAdapter.greet('Alejo macOS');
    setGreeting(message);

    const isBluetoothEnabled = HybridAdapter.bluetooth_enabled();
    console.log('isBluetoothEnabled', isBluetoothEnabled);
    setIsBluetoothEnabled(isBluetoothEnabled);

    const adapterObjects = HybridAdapter.get_adapters();
    console.log('adapters', adapterObjects);
    
    // Store full adapter references in refs (outside React state)
    // This prevents Fast Refresh from detecting changes and causing infinite reloads
    adapterRefs.current = adapterObjects;
    adapterMapByName.current.clear();
    
    // Extract only the data we need for rendering (to avoid storing hybrid objects in state)
    const adapterData: AdapterInfo[] = adapterObjects.map((adapter: any, index: number) => {
      const name = adapter.name || 'Unknown';
      // Store reference in map for easy lookup by name
      adapterMapByName.current.set(name, adapter);
      return {
        name,
        index,
      };
    });
    
    setAdapters(adapterData);
  }, []);

  // Example: Use adapter methods when needed
  // const handleUseAdapter = (index: number) => {
  //   const adapter = getAdapterByIndex(index);
  //   if (adapter) {
  //     // Use adapter methods here
  //     // adapter.someMethod();
  //   }
  // };

  return (
    <View style={styles.container}>
      {greeting ? <Text style={styles.greeting}>{greeting}</Text> : null}
      {isBluetoothEnabled ? (
        <Text style={styles.greeting}>Bluetooth is enabled</Text>
      ) : (
        <Text style={styles.greeting}>Bluetooth is disabled</Text>
      )}
      {adapters.map((adapter) => (
        <Text key={`${adapter.name}-${adapter.index}`} style={styles.greeting}>
          {adapter.name}
        </Text>
      ))}
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
  titleContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  stepContainer: {
    gap: 8,
    marginBottom: 8,
  },
  reactLogo: {
    height: 178,
    width: 290,
    bottom: 0,
    left: 0,
    position: 'absolute',
  },
});
