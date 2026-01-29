import { StyleSheet, Text, View, TouchableOpacity, ScrollView } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { useRouter } from 'expo-router';

export default function HomeScreen() {
  const router = useRouter();

  const examples = [
    {
      id: 'connect',
      title: 'Connect Example',
      description: 'Scan for BLE devices, connect, and verify the connection',
      route: '/connect',
      icon: '🔗',
    },
    {
      id: 'read',
      title: 'Read Example',
      description: 'Connect to a device and read characteristic values',
      route: '/read',
      icon: '📖',
    },
  ];

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView 
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}
      >
        <View style={styles.header}>
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
              onPress={() => router.push(example.route as any)}
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
  header: {
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
