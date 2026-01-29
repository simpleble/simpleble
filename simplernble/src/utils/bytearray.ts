/**
 * Convert a hex string to a Uint8Array.
 * Spaces and common separators (-, :) are automatically removed.
 * 
 * @param hex Hex string (e.g., "48656c6c6f" or "48 65 6c 6c 6f")
 * @returns Uint8Array containing the decoded bytes
 * 
 * @example
 * const data = fromHex("48656c6c6f");
 * const data2 = fromHex("48 65 6c 6c 6f");
 * const data3 = fromHex("48:65:6c:6c:6f");
 */
export function fromHex(hex: string): Uint8Array {
  const cleanHex = hex.replace(/[\s\-:]/g, '');
  
  if (cleanHex.length % 2 !== 0) {
    throw new Error('Hex string must have an even number of characters');
  }
  
  if (!/^[0-9a-fA-F]*$/.test(cleanHex)) {
    throw new Error('Invalid hex string');
  }
  
  const bytes = new Uint8Array(cleanHex.length / 2);
  for (let i = 0; i < cleanHex.length; i += 2) {
    bytes[i / 2] = parseInt(cleanHex.substring(i, i + 2), 16);
  }
  
  return bytes;
}

/**
 * Convert a Uint8Array to a hex string.
 * 
 * @param data Uint8Array to convert
 * @param withSpaces If true, separates bytes with spaces (default: false)
 * @returns Hex string representation
 * 
 * @example
 * const data = new Uint8Array([72, 101, 108, 108, 111]);
 * toHex(data);           // "48656c6c6f"
 * toHex(data, true);     // "48 65 6c 6c 6f"
 */
export function toHex(data: Uint8Array, withSpaces: boolean = false): string {
  const hex = Array.from(data)
    .map(byte => byte.toString(16).padStart(2, '0'))
    .join(withSpaces ? ' ' : '');
  return hex;
}

/**
 * Convert a Uint8Array to a UTF-8 string.
 * 
 * @param data Uint8Array containing UTF-8 encoded text
 * @returns Decoded string
 * 
 * @example
 * const data = new Uint8Array([72, 101, 108, 108, 111]);
 * toString(data); // "Hello"
 */
export function toString(data: Uint8Array): string {
  const decoder = new TextDecoder('utf-8');
  return decoder.decode(data);
}

/**
 * Convert a UTF-8 string to a Uint8Array.
 * 
 * @param str String to encode
 * @returns Uint8Array containing UTF-8 encoded bytes
 * 
 * @example
 * const data = fromString("Hello");
 * // Uint8Array([72, 101, 108, 108, 111])
 */
export function fromString(str: string): Uint8Array {
  const encoder = new TextEncoder();
  return encoder.encode(str);
}
