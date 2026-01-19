import * as React from 'react';
import { useState } from 'react';
import { StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import ConnectExample from './screens/connect';
import Home from './screens/home';
import ReadExample from './screens/read';

type Screen = 'Home' | 'Connect' | 'Read';

function App() {
  const [currentScreen, setCurrentScreen] = useState<Screen>('Home');

  const navigateTo = (screen: Screen) => {
    setCurrentScreen(screen);
  };

  const goBack = () => {
    setCurrentScreen('Home');
  };

  const renderScreen = () => {
    switch (currentScreen) {
      case 'Connect':
        return <ConnectExample onBack={goBack} />;
      case 'Read':
        return <ReadExample onBack={goBack} />;
      case 'Home':
      default:
        return <Home onNavigate={navigateTo} />;
    }
  };

  return (
    <View style={styles.container}>
      {currentScreen !== 'Home' && (
        <View style={styles.header}>
          <TouchableOpacity style={styles.backButton} onPress={goBack}>
            <Text style={styles.backButtonText}>← Back</Text>
          </TouchableOpacity>
          <Text style={styles.headerTitle}>
            {currentScreen === 'Connect' ? 'Connect Example' : 'Read Example'}
          </Text>
          <View style={styles.headerSpacer} />
        </View>
      )}
      {renderScreen()}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 16,
    paddingVertical: 12,
    backgroundColor: '#007AFF',
    borderBottomWidth: 1,
    borderBottomColor: '#0056b3',
  },
  backButton: {
    paddingHorizontal: 12,
    paddingVertical: 6,
  },
  backButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },
  headerTitle: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: 'bold',
    flex: 1,
    textAlign: 'center',
  },
  headerSpacer: {
    width: 60,
  },
});

export default App;
