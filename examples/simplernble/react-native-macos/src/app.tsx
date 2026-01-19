import * as React from 'react';
import { useState } from 'react';
import { SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import ConnectExample from './connect';
import ReadExample from './read';

type Screen = 'Home' | 'Connect' | 'Read';

function App() {
  const [currentScreen, setCurrentScreen] = useState<Screen>('Home');

  const navigateTo = (screen: Screen) => {
    setCurrentScreen(screen);
  };

  const goBack = () => {
    setCurrentScreen('Home');
  };

  const examples = [
    {
      id: 'connect',
      title: 'Connect Example',
      description: 'Scan for BLE devices, connect, and verify the connection',
      route: 'Connect' as const,
      icon: '🔗',
    },
    {
      id: 'read',
      title: 'Read Example',
      description: 'Connect to a device and read characteristic values',
      route: 'Read' as const,
      icon: '📖',
    },
  ];

  const renderScreen = () => {
    switch (currentScreen) {
      case 'Connect':
        return <ConnectExample onBack={goBack} />;
      case 'Read':
        return <ReadExample onBack={goBack} />;
      case 'Home':
      default:
        return (
          <SafeAreaView style={styles.safeArea}>
            <ScrollView 
              style={styles.scrollView}
              contentContainerStyle={styles.scrollContent}
              showsVerticalScrollIndicator={true}
            >
              <View style={styles.homeHeader}>
                <Text style={styles.title}>SimpleBLE Examples</Text>
                <Text style={styles.subtitle}>
                  Choose an example to explore BLE functionality
                </Text>
              </View>

              <View style={styles.examplesContainer}>
                {examples.map((example) => (
                  <TouchableOpacity
                    key={example.id}
                    style={styles.exampleCard}
                    onPress={() => navigateTo(example.route)}
                  >
                    <View style={styles.cardContent}>
                      <Text style={styles.exampleIcon}>{example.icon}</Text>
                      <View style={styles.exampleTextContainer}>
                        <Text style={styles.exampleTitle}>{example.title}</Text>
                        <Text style={styles.exampleDescription}>{example.description}</Text>
                      </View>
                      <Text style={styles.arrowIcon}>›</Text>
                    </View>
                  </TouchableOpacity>
                ))}
              </View>

              <View style={styles.footer}>
                <Text style={styles.footerText}>
                  These examples demonstrate SimpleBLE functionality on React Native
                </Text>
              </View>
            </ScrollView>
          </SafeAreaView>
        );
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
  homeHeader: {
    marginBottom: 30,
    alignItems: 'center',
  },
  title: {
    fontSize: 32,
    fontWeight: 'bold',
    marginBottom: 10,
    textAlign: 'center',
    color: '#333',
  },
  subtitle: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
  },
  examplesContainer: {
    marginBottom: 20,
  },
  exampleCard: {
    backgroundColor: '#ffffff',
    borderRadius: 12,
    marginBottom: 15,
    borderWidth: 1,
    borderColor: '#e0e0e0',
    overflow: 'hidden',
  },
  cardContent: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 20,
  },
  exampleIcon: {
    fontSize: 32,
    marginRight: 15,
  },
  exampleTextContainer: {
    flex: 1,
  },
  exampleTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 5,
  },
  exampleDescription: {
    fontSize: 14,
    color: '#666',
    lineHeight: 20,
  },
  arrowIcon: {
    fontSize: 28,
    color: '#007AFF',
    fontWeight: 'bold',
  },
  footer: {
    marginTop: 20,
    padding: 15,
    backgroundColor: '#ffffff',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  footerText: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
    lineHeight: 20,
  },
});

export default App;
