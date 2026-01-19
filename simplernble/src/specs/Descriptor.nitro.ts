import { type HybridObject } from 'react-native-nitro-modules'

export interface Descriptor
  extends HybridObject<{
    ios: 'c++'
    android: 'c++'
  }> {
  /**
   * Check if the descriptor is initialized (has a valid internal handle).
   */
  initialized(): boolean

  /**
   * Get the descriptor UUID.
   */
  uuid(): string
}
