import lzma
import sys
import os
import struct

def pack_dll(input_path, output_bin):
    """
    Compresses the input DLL using LZMA to match the original loader's logic.
    """
    try:
        if not os.path.exists(input_path):
            print(f"Error: {input_path} not found.")
            return

        with open(input_path, 'rb') as f:
            data = f.read()

        orig_size = len(data)

        # Use LZMA compression (matches the Range Decoder logic in 'start')
        # format=lzma.FORMAT_ALONE is the raw LZMA format used by legacy loaders
        compressed = lzma.compress(data, format=lzma.FORMAT_ALONE)

        # The original code likely stores the compressed blob directly as a resource.
        # We output it as a binary file.
        with open(output_bin, 'wb') as f:
            f.write(compressed)

        print(f"Successfully packed {input_path} -> {output_bin}")
        print(f"Original: {orig_size} bytes, LZMA Compressed: {len(compressed)} bytes")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python encrypt_and_pack.py <input.dll> <output.bin>")
    else:
        pack_dll(sys.argv[1], sys.argv[2])
