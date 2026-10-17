// Runs the binding tests in the example app. scripts/test.mjs serves the app
// with this component in place of the example, and collects the results.
import { useEffect, useState } from 'react';
import { ScrollView, StyleSheet, Text } from 'react-native';

import { run, type Result } from './index';

// NOTE: scripts/test.mjs listens here. Android reaches it through adb reverse.
const REPORT_URL = 'http://localhost:8099';

function report(path: string, body: unknown): Promise<unknown> {
  return fetch(`${REPORT_URL}/${path}`, { method: 'POST', body: JSON.stringify(body) }).catch(() => undefined);
}

export default function TestApp() {
  const [results, setResults] = useState<Result[]>([]);
  const [summary, setSummary] = useState('Running...');

  useEffect(() => {
    // Each report arrives before the next test starts, and before 'done'.
    run(result => {
      setResults(previous => [...previous, result]);
      return report('result', result);
    }).then(all => {
      const failed = all.filter(result => !result.ok).length;
      setSummary(`${all.length - failed} passed, ${failed} failed`);
      report('done', { passed: all.length - failed, failed });
    });
  }, []);

  return (
    <ScrollView contentContainerStyle={styles.screen}>
      <Text style={styles.summary}>{summary}</Text>
      {results.map(result => (
        <Text key={result.name} style={result.ok ? styles.passed : styles.failed}>
          {result.ok ? '✓' : '✗'} {result.name}
          {result.error ? `\n  ${result.error}` : ''}
        </Text>
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  screen: { padding: 16, paddingTop: 64 },
  summary: { fontWeight: '600', marginBottom: 12 },
  passed: { color: 'green', marginBottom: 4 },
  failed: { color: 'red', marginBottom: 4 },
});
