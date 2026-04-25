import zlib
import sys
import os
import struct

def pack_dll(input_path, output_bin, key=0xAA):
    """
    Compresses the input DLL using zlib and encrypts it with an XOR key.
    Includes original size metadata at the beginning.
    """
    try:
        if not os.path.exists(input_path):
            print(f"Error: {input_path} not found.")
            return

        with open(input_path, 'rb') as f:
            data = f.read()

        orig_size = len(data)

        # 1. Compress
        compressed = zlib.compress(data, level=9)

        # 2. Add header (Original Size) and XOR Encrypt
        full_payload = struct.pack("<I", orig_size) + compressed

        encrypted = bytearray()
        for i in range(len(full_payload)):
            encrypted.append(full_payload[i] ^ key)

        # 3. Output Binary File
        with open(output_bin, 'wb') as f:
            f.write(encrypted)

        print(f"Successfully packed {input_path} -> {output_bin}")
        print(f"Original: {orig_size} bytes, Final Packed: {len(encrypted)} bytes")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python encrypt_and_pack.py <input.dll> <output.bin>")
    else:
        pack_dll(sys.argv[1], sys.argv[2])
