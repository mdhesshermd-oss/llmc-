#include <stdint.h>

/**
 * Simple compile-time string obfuscator (XOR).
 * Prevents anti-cheats from finding plain-text strings in the binary.
 */

#define XOR_STR(s) xor_string(s, sizeof(s))

static inline char* xor_string(char* data, size_t size) {
    static const char key = 0x55;
    for (size_t i = 0; i < size - 1; i++) {
        data[i] ^= key;
    }
    return data;
}

// Example usage:
// char* msg = XOR_STR("\x1b\x34\x34\x3a\x27"); // Decrypts "Error" at runtime
