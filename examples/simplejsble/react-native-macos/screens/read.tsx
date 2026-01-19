import { useEffect, useRef, useState } from 'react';
import { ActivityIndicator, SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import { HybridAdapter, toHex, type Adapter, type Characteristic, type Peripheral, type Service } from 'simplejsble';

interface ReadExampleProps {
  onBack: () => void;
}

interface CharacteristicPair {
  serviceUuid: string;
  characteristicUuid: string;
  canRead: boolean;
}

interface ReadResult {
  index: number;
  data: string;
  timestamp: string;
}

export default function ReadExample({ onBack }: ReadExampleProps) {
  const [adapter, setAdapter] = useState<Adapter | null>(null);
  const [isBluetoothEnabled, setIsBluetoothEnabled] = useState<boolean>(false);
  const [isInitializing, setIsInitializing] = useState<boolean>(true);
  const [scanning, setScanning] = useState<boolean>(false);
  const [peripherals, setPeripherals] = useState<Peripheral[]>([]);
  const [connectedPeripheral, setConnectedPeripheral] = useState<Peripheral | null>(null);
  const [statusMessage, setStatusMessage] = useState<string>('Initializing Bluetooth...');
  const [characteristics, setCharacteristics] = useState<CharacteristicPair[]>([]);
  const [selectedCharacteristic, setSelectedCharacteristic] = useState<CharacteristicPair | null>(null);
  const [readResults, setReadResults] = useState<ReadResult[]>([]);
  const [isReading, setIsReading] = useState<boolean>(false);
  
  const adapterRef = useRef<Adapter | null>(null);

  useEffect(() => {
    const initAdapter = async () => {
      try {
        setIsInitializing(true);
        
        const isEnabled = await HybridAdapter.bluetooth_enabled();
        setIsBluetoothEnabled(isEnabled);
        
        if (!isEnabled) {
          setStatusMessage('Bluetooth is disabled. Please enable Bluetooth.');
          setIsInitializing(false);
          return;
        }

        const adapters = await HybridAdapter.get_adapters();
        if (adapters.length === 0) {
          setStatusMessage('No Bluetooth adapters found.');
          setIsInitializing(false);
          return;
        }

        const firstAdapter = adapters[0] as Adapter;
        adapterRef.current = firstAdapter;
        setAdapter(firstAdapter);
        setStatusMessage('Ready to scan. Press "Scan for Devices" to start.');
      } catch (error) {
        console.error('Error initializing adapter:', error);
        setStatusMessage('Error initializing Bluetooth adapter.');
      } finally {
        setIsInitializing(false);
      }
    };

    initAdapter();
  }, []);

  const startScan = () => {
    if (!adapter || scanning) return;

    setPeripherals([]);
    setStatusMessage('Starting scan...');
    setScanning(true);

    try {
      adapter.set_callback_on_scan_start(() => {
        console.log('Scan started.');
        setStatusMessage('Scanning...');
      });

      adapter.set_callback_on_scan_stop(() => {
        console.log('Scan stopped.');
        setScanning(false);
        const results = adapter.scan_get_results();
        const connectableCount = results.filter((p: Peripheral) => p.is_connectable()).length;
        setStatusMessage(`Scan complete. Found ${connectableCount} connectable device${connectableCount !== 1 ? 's' : ''}.`);
      });

      adapter.set_callback_on_scan_found((peripheral: Peripheral) => {
        console.log(`Found device: ${peripheral.identifier()} [${peripheral.address()}]`);
        if (peripheral.is_connectable()) {
          setPeripherals(prev => {
            const exists = prev.some(p => p.address() === peripheral.address());
            if (!exists) {
              return [...prev, peripheral];
            }
            return prev;
          });
        }
      });

      setTimeout(() => {
        adapter.scan_start();
      }, 0);
      
      setTimeout(() => {
        if (adapter.scan_is_active()) {
          adapter.scan_stop();
        }
      }, 5000);
    } catch (error) {
      console.error('Error during scan:', error);
      setScanning(false);
      setStatusMessage('Error during scan.');
    }
  };

  const connectToDevice = (peripheral: Peripheral) => {
    if (connectedPeripheral) {
      setStatusMessage('Please disconnect from current device first.');
      return;
    }

    setStatusMessage(`Connecting to ${peripheral.identifier()}...`);

    try {
      peripheral.set_callback_on_connected(() => {
        console.log('Successfully connected.');
        setConnectedPeripheral(peripheral);
        setStatusMessage('Connected. Discovering services...');
        
        setTimeout(() => {
          discoverCharacteristics(peripheral);
        }, 500);
      });

      peripheral.set_callback_on_disconnected(() => {
        console.log('Disconnected.');
        setConnectedPeripheral(null);
        setCharacteristics([]);
        setSelectedCharacteristic(null);
        setReadResults([]);
        setStatusMessage('Disconnected.');
      });

      peripheral.connect();
    } catch (error) {
      console.error('Error connecting to device:', error);
      setStatusMessage('Error connecting to device.');
    }
  };

  const discoverCharacteristics = (peripheral: Peripheral) => {
    try {
      const services: Service[] = peripheral.services();
      const charPairs: CharacteristicPair[] = [];

      for (const service of services) {
        const serviceUuid = service.uuid();
        const chars: Characteristic[] = service.characteristics();
        
        for (const char of chars) {
          charPairs.push({
            serviceUuid: serviceUuid,
            characteristicUuid: char.uuid(),
            canRead: char.can_read(),
          });
        }
      }

      setCharacteristics(charPairs);
      setStatusMessage(`Discovered ${charPairs.length} characteristic${charPairs.length !== 1 ? 's' : ''}. Select one to read.`);
    } catch (error) {
      console.error('Error discovering characteristics:', error);
      setStatusMessage('Error discovering characteristics.');
    }
  };

  const readCharacteristic = async (char: CharacteristicPair) => {
    if (!connectedPeripheral || isReading) return;

    setSelectedCharacteristic(char);
    setReadResults([]);
    setIsReading(true);
    setStatusMessage('Reading characteristic 5 times...');

    try {
      for (let i = 0; i < 5; i++) {
        const data: ArrayBuffer = connectedPeripheral.read(char.serviceUuid, char.characteristicUuid);
        const uint8Data = new Uint8Array(data);
        const hexString = toHex(uint8Data, true);
        const timestamp = new Date().toLocaleTimeString();

        setReadResults(prev => [...prev, {
          index: i + 1,
          data: hexString,
          timestamp: timestamp,
        }]);

        // Wait 1 second before next read
        if (i < 4) {
          await new Promise(resolve => setTimeout(resolve, 1000));
        }
      }

      setStatusMessage('Read complete. 5 values received.');
      setIsReading(false);
    } catch (error) {
      console.error('Error reading characteristic:', error);
      setStatusMessage('Error reading characteristic.');
      setIsReading(false);
    }
  };

  const disconnect = () => {
    if (!connectedPeripheral) return;

    try {
      connectedPeripheral.disconnect();
    } catch (error) {
      console.error('Error disconnecting:', error);
      setStatusMessage('Error disconnecting.');
    }
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView 
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}
      >
        <Text style={styles.title}>BLE Read Example</Text>
        
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Status:</Text>
          <Text style={styles.statusText}>{statusMessage}</Text>
        </View>

        {isInitializing && (
          <View style={styles.loadingContainer}>
            <ActivityIndicator size="large" color="#007AFF" />
            <Text style={styles.loadingText}>Initializing Bluetooth...</Text>
            <Text style={styles.loadingSubtext}>Please wait while we set up the Bluetooth adapter</Text>
          </View>
        )}

        {!isInitializing && !isBluetoothEnabled && (
          <View style={styles.warningContainer}>
            <Text style={styles.warningText}>⚠️ Bluetooth is disabled</Text>
          </View>
        )}

        {!isInitializing && scanning && (
          <View style={styles.loadingContainer}>
            <ActivityIndicator size="large" color="#007AFF" />
            <Text style={styles.loadingText}>Scanning for devices...</Text>
            <Text style={styles.loadingSubtext}>Please wait, this may take a few seconds</Text>
          </View>
        )}

        {!isInitializing && adapter && (
          <>
            {!connectedPeripheral && (
              <TouchableOpacity
                style={[styles.button, scanning && styles.buttonDisabled].filter(Boolean)}
                onPress={startScan}
                disabled={scanning}
              >
                {scanning ? (
                  <View style={styles.buttonContent}>
                    <ActivityIndicator size="small" color="#ffffff" style={{ marginRight: 10 }} />
                    <Text style={styles.buttonText}>Scanning...</Text>
                  </View>
                ) : (
                  <Text style={styles.buttonText}>Scan for Devices</Text>
                )}
              </TouchableOpacity>
            )}

            {connectedPeripheral && (
              <View style={styles.connectedContainer}>
                <View style={styles.connectionHeader}>
                  <Text style={styles.connectedTitle}>
                    Connected to {connectedPeripheral.identifier()}
                  </Text>
                </View>

                <TouchableOpacity
                  style={[styles.button, styles.disconnectButton]}
                  onPress={disconnect}
                >
                  <Text style={styles.buttonText}>Disconnect</Text>
                </TouchableOpacity>
              </View>
            )}

            {!connectedPeripheral && peripherals.length > 0 && (
              <View style={styles.devicesContainer}>
                <Text style={styles.devicesTitle}>
                  Connectable Devices ({peripherals.length}):
                </Text>
                <ScrollView 
                  style={styles.devicesList}
                  nestedScrollEnabled={true}
                  showsVerticalScrollIndicator={true}
                >
                  {peripherals.map((peripheral, index) => (
                    <View key={`${peripheral.address()}-${index}`} style={styles.deviceItem}>
                      <View style={styles.deviceInfoContainer}>
                        <Text style={styles.deviceName}>
                          [{index}] {peripheral.identifier()}
                        </Text>
                        <Text style={styles.deviceAddress}>
                          {peripheral.address()}
                        </Text>
                      </View>
                      <TouchableOpacity
                        style={styles.connectButton}
                        onPress={() => connectToDevice(peripheral)}
                      >
                        <Text style={styles.connectButtonText}>Connect</Text>
                      </TouchableOpacity>
                    </View>
                  ))}
                </ScrollView>
              </View>
            )}

            {connectedPeripheral && characteristics.length > 0 && !selectedCharacteristic && (
              <View style={styles.characteristicsContainer}>
                <Text style={styles.sectionTitle}>
                  Select a Characteristic to Read ({characteristics.length}):
                </Text>
                <ScrollView 
                  style={styles.characteristicsList}
                  nestedScrollEnabled={true}
                  showsVerticalScrollIndicator={true}
                >
                  {characteristics.map((char, index) => (
                    <View key={`${char.serviceUuid}-${char.characteristicUuid}`} style={styles.characteristicItem}>
                      <View style={styles.characteristicInfoContainer}>
                        <Text style={styles.characteristicIndex}>[{index}]</Text>
                        <View style={styles.characteristicTextContainer}>
                          <Text style={styles.characteristicLabel}>Service:</Text>
                          <Text style={styles.characteristicUuid}>{char.serviceUuid}</Text>
                          <Text style={styles.characteristicLabel}>Characteristic:</Text>
                          <Text style={styles.characteristicUuid}>{char.characteristicUuid}</Text>
                          {!char.canRead && (
                            <Text style={styles.readOnlyBadge}>⚠️ May not support read</Text>
                          )}
                        </View>
                      </View>
                      <TouchableOpacity
                        style={styles.readButton}
                        onPress={() => readCharacteristic(char)}
                        disabled={isReading}
                      >
                        <Text style={styles.readButtonText}>Read</Text>
                      </TouchableOpacity>
                    </View>
                  ))}
                </ScrollView>
              </View>
            )}

            {isReading && (
              <View style={styles.loadingContainer}>
                <ActivityIndicator size="large" color="#007AFF" />
                <Text style={styles.loadingText}>Reading characteristic...</Text>
                <Text style={styles.loadingSubtext}>Reading 5 times with 1 second delay</Text>
              </View>
            )}

            {selectedCharacteristic && readResults.length > 0 && (
              <View style={styles.resultsContainer}>
                <View style={styles.resultsHeader}>
                  <Text style={styles.resultsTitle}>Read Results ({readResults.length}/5)</Text>
                  {!isReading && (
                    <TouchableOpacity
                      style={styles.readAgainButton}
                      onPress={() => readCharacteristic(selectedCharacteristic)}
                    >
                      <Text style={styles.readAgainButtonText}>🔄 Read Again</Text>
                    </TouchableOpacity>
                  )}
                </View>
                <View style={styles.characteristicInfoBox}>
                  <Text style={styles.infoBoxLabel}>Service UUID:</Text>
                  <Text style={styles.infoBoxValue}>{selectedCharacteristic.serviceUuid}</Text>
                  <Text style={styles.infoBoxLabel}>Characteristic UUID:</Text>
                  <Text style={styles.infoBoxValue}>{selectedCharacteristic.characteristicUuid}</Text>
                </View>
                <ScrollView 
                  style={styles.resultsList}
                  nestedScrollEnabled={true}
                  showsVerticalScrollIndicator={true}
                >
                  {readResults.map((result) => (
                    <View key={result.index} style={styles.resultItem}>
                      <View style={styles.resultHeader}>
                        <Text style={styles.resultIndex}>Read #{result.index}</Text>
                        <Text style={styles.resultTimestamp}>{result.timestamp}</Text>
                      </View>
                      <View style={styles.resultDataContainer}>
                        <Text style={styles.resultData}>{result.data || '(empty)'}</Text>
                      </View>
                    </View>
                  ))}
                </ScrollView>
                {!isReading && (
                  <TouchableOpacity
                    style={styles.backButton}
                    onPress={() => {
                      setSelectedCharacteristic(null);
                      setReadResults([]);
                      setStatusMessage('Select another characteristic to read.');
                    }}
                  >
                    <Text style={styles.buttonText}>← Back to Characteristics</Text>
                  </TouchableOpacity>
                )}
              </View>
            )}
          </>
        )}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  scrollView: {
    flex: 1,
  },
  scrollContent: {
    padding: 20,
    paddingBottom: 40,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
    textAlign: 'center',
    color: '#333',
  },
  statusContainer: {
    backgroundColor: '#ffffff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  statusLabel: {
    fontSize: 14,
    fontWeight: '600',
    color: '#666',
    marginBottom: 5,
  },
  statusText: {
    fontSize: 16,
    color: '#333',
  },
  warningContainer: {
    backgroundColor: '#fff3cd',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
    borderLeftWidth: 4,
    borderLeftColor: '#ffc107',
  },
  warningText: {
    fontSize: 16,
    color: '#856404',
  },
  button: {
    backgroundColor: '#007AFF',
    padding: 15,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 15,
    minHeight: 50,
    justifyContent: 'center',
  },
  buttonDisabled: {
    backgroundColor: '#cccccc',
  },
  buttonContent: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },
  connectedContainer: {
    backgroundColor: '#ffffff',
    padding: 20,
    borderRadius: 8,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  connectionHeader: {
    marginBottom: 15,
  },
  connectedTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
  },
  disconnectButton: {
    backgroundColor: '#FF3B30',
  },
  devicesContainer: {
    backgroundColor: '#ffffff',
    borderRadius: 8,
    padding: 15,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
    maxHeight: 400,
  },
  devicesTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 15,
    color: '#333',
  },
  devicesList: {
    maxHeight: 350,
  },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 15,
    backgroundColor: '#f9f9f9',
    borderRadius: 8,
    marginBottom: 10,
    borderLeftWidth: 4,
    borderLeftColor: '#007AFF',
  },
  deviceInfoContainer: {
    flex: 1,
    marginRight: 10,
  },
  deviceName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 5,
  },
  deviceAddress: {
    fontSize: 14,
    color: '#666',
  },
  connectButton: {
    backgroundColor: '#34C759',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 6,
  },
  connectButtonText: {
    color: '#ffffff',
    fontSize: 14,
    fontWeight: '600',
  },
  loadingContainer: {
    backgroundColor: '#ffffff',
    padding: 30,
    borderRadius: 12,
    marginBottom: 20,
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  loadingText: {
    fontSize: 18,
    fontWeight: '600',
    color: '#333',
    marginTop: 15,
    marginBottom: 5,
  },
  loadingSubtext: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
  },
  characteristicsContainer: {
    backgroundColor: '#ffffff',
    borderRadius: 8,
    padding: 15,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 15,
    color: '#333',
  },
  characteristicsList: {
    maxHeight: 500,
  },
  characteristicItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 15,
    backgroundColor: '#f9f9f9',
    borderRadius: 8,
    marginBottom: 10,
    borderLeftWidth: 4,
    borderLeftColor: '#34C759',
  },
  characteristicInfoContainer: {
    flex: 1,
    flexDirection: 'row',
    marginRight: 10,
  },
  characteristicIndex: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
    marginRight: 10,
  },
  characteristicTextContainer: {
    flex: 1,
  },
  characteristicLabel: {
    fontSize: 12,
    fontWeight: '600',
    color: '#666',
    marginTop: 5,
  },
  characteristicUuid: {
    fontSize: 12,
    color: '#333',
    fontFamily: 'monospace',
    marginBottom: 5,
  },
  readOnlyBadge: {
    fontSize: 11,
    color: '#856404',
    marginTop: 5,
  },
  readButton: {
    backgroundColor: '#007AFF',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 6,
  },
  readButtonText: {
    color: '#ffffff',
    fontSize: 14,
    fontWeight: '600',
  },
  resultsContainer: {
    backgroundColor: '#ffffff',
    borderRadius: 8,
    padding: 15,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  resultsHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 15,
  },
  resultsTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
  },
  readAgainButton: {
    backgroundColor: '#007AFF',
    paddingHorizontal: 15,
    paddingVertical: 8,
    borderRadius: 6,
  },
  readAgainButtonText: {
    color: '#ffffff',
    fontSize: 14,
    fontWeight: '600',
  },
  characteristicInfoBox: {
    backgroundColor: '#f9f9f9',
    padding: 12,
    borderRadius: 6,
    marginBottom: 15,
  },
  infoBoxLabel: {
    fontSize: 12,
    fontWeight: '600',
    color: '#666',
    marginTop: 5,
  },
  infoBoxValue: {
    fontSize: 12,
    color: '#333',
    fontFamily: 'monospace',
    marginBottom: 5,
  },
  resultsList: {
    maxHeight: 400,
  },
  resultItem: {
    backgroundColor: '#f9f9f9',
    padding: 15,
    borderRadius: 8,
    marginBottom: 10,
    borderLeftWidth: 4,
    borderLeftColor: '#34C759',
  },
  resultHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  resultIndex: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
  },
  resultTimestamp: {
    fontSize: 12,
    color: '#666',
  },
  resultDataContainer: {
    backgroundColor: '#263238',
    padding: 12,
    borderRadius: 6,
  },
  resultData: {
    fontSize: 12,
    color: '#aed581',
    fontFamily: 'monospace',
  },
  backButton: {
    backgroundColor: '#666',
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 10,
  },
});
