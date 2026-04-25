import zlib
import sys

def pack_dll(input_path, output_header, key=0xAA):
    """
    Compresses the input DLL using zlib and encrypts it with an XOR key.
    Outputs a C header file with the byte array.
    """
    try:
        with open(input_path, 'rb') as f:
            data = f.read()

        # 1. Compress
        compressed = zlib.compress(data, level=9)

        # 2. XOR Encrypt
        encrypted = bytearray()
        for i in range(len(compressed)):
            encrypted.append(compressed[i] ^ key)

        # 3. Generate C Header
        with open(output_header, 'w') as f:
            f.write("#pragma once\n")
            f.write(f"// Encrypted and Compressed Payload\n")
            f.write(f"const unsigned int PAYLOAD_SIZE = {len(encrypted)};\n")
            f.write(f"const unsigned int ORIGINAL_SIZE = {len(data)};\n")
            f.write(f"const unsigned char XOR_KEY = 0x{key:02X};\n")
            f.write("const unsigned char PAYLOAD_DATA[] = {\n    ")

            for i, byte in enumerate(encrypted):
                f.write(f"0x{byte:02x}")
                if i != len(encrypted) - 1:
                    f.write(", ")
                if (i + 1) % 12 == 0:
                    f.write("\n    ")

            f.write("\n};\n")

        print(f"Successfully packed {input_path} -> {output_header}")
        print(f"Original: {len(data)} bytes, Compressed/Encrypted: {len(encrypted)} bytes")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python encrypt_and_pack.py <input.dll> <output_payload.h>")
    else:
        pack_dll(sys.argv[1], sys.argv[2])
