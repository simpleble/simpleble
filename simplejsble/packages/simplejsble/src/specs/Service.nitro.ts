import { type HybridObject } from 'react-native-nitro-modules'
import { type Characteristic } from './Characteristic.nitro'

export interface Service
  extends HybridObject<{
    ios: 'c++'
    android: 'c++'
  }> {
  /**
   * Check if the service is initialized (has a valid internal handle).
   */
  initialized(): boolean

  /**
   * Get the service UUID.
   */
  uuid(): string

  /**
   * Get the service data (if advertised).
   * Returns raw ArrayBuffer containing the service data bytes.
   */
  data(): ArrayBuffer

  /**
   * Get all characteristics belonging to this service.
   */
  characteristics(): Characteristic[]
}
