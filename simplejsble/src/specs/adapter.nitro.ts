import { type HybridObject } from 'react-native-nitro-modules'

export interface Adapter extends HybridObject<{
  ios: 'c++',
  android: 'c++'
}> {
  greet(name: string): string
}