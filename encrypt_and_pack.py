import os
from Crypto.Cipher import AES
from Crypto.Util import Padding

# --- AES-128 CBC Encryption Script ---
# Matches the decryption logic in refactored_loader.cpp

def encrypt_payload(input_dll, output_bin, key_hex):
    # Key must be 16 bytes for AES-128
    key = bytes.fromhex(key_hex)

    if not os.path.exists(input_dll):
        print(f"[-] Error: {input_dll} not found.")
        return

    with open(input_dll, 'rb') as f:
        data = f.read()

    # Use AES-CBC
    cipher = AES.new(key, AES.MODE_CBC)
    iv = cipher.iv

    # Pad data to block size (16 bytes)
    padded_data = Padding.pad(data, AES.block_size)
    encrypted_data = cipher.encrypt(padded_data)

    with open(output_bin, 'wb') as f:
        # Prepend IV to the file for the loader to read
        f.write(iv + encrypted_data)

    print(f"[+] Encrypted with AES-128 CBC. Output: {output_bin}")
    print(f"[+] Key: {key_hex}")
    print(f"[+] Size: {len(iv) + len(encrypted_data)} bytes")

if __name__ == "__main__":
    # Secret Key matching HV_SECRET_KEY logic (ZORO_DAYZORO_DAY)
    # Key must be 16 bytes (32 hex characters) for AES-128
    SECRET_KEY = "5A4F524F5F4441595A4F524F5F444159"
    encrypt_payload("core.dll", "packed_payload.bin", SECRET_KEY)
