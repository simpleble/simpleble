import { StyleSheet, Text, View } from 'react-native';
import { useState, useEffect } from 'react';
import { HybridAdapter } from 'simplejsble';

export default function HomeScreen() {
  const [greeting, setGreeting] = useState<string>('');

  useEffect(() => {
    const message = HybridAdapter.greet('Alejo');
    setGreeting(message);
  }, []);

  return (
    <View style={styles.container}>
      {greeting ? <Text style={styles.greeting}>{greeting}</Text> : null}
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
