# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import json
import sys
import os
import hashlib

COMPRESSION_TYPE="zstd"
COMPRESSION_LEVEL=3
COMPRESSION_MAJOR_VERSION=1
COMPRESSION_MINOR_VERSION=5

COMPRESSION_DETAILS_JSON_FILE_NAME = "compression-details.json"

def sha256_file(file_path):
    hasher = hashlib.sha256()
    block_size = 64 * 1024

    handle = open(file_path, "rb")
    data = handle.read(block_size)
    while data:
        hasher.update(data)
        data = handle.read(block_size)

    return hasher.hexdigest()

def run_zstd_compress_file(zstd_compress_file_path, source, target):
    cmd_line = f"{zstd_compress_file_path} {source} {target}"
    os.system(cmd_line)

def usage():
    print("Usage: compress <zstd_compress_file_path> <folder> [files...]")

def main(argv):
    if len(argv) < 2:
        usage()
        sys.exit()

    zstd_compress_file_path = argv[0]
    folder = argv[1]

    print(f"zstd_compress_file_path: {zstd_compress_file_path}")
    print(f"Folder: {folder}")

    files = argv[2:]

    all_file_compression_details = {}

    for file in files:
        compressed_file_name = f"{file}.zst"
        original_file_path = os.path.join(folder, file)
        compressed_file_path = os.path.join(folder, compressed_file_name)
        run_zstd_compress_file(zstd_compress_file_path, original_file_path, compressed_file_path)

        original_file_hash = sha256_file(original_file_path)
        compressed_file_hash = sha256_file(compressed_file_path)

        original_file_size = os.path.getsize(original_file_path)
        compressed_file_size = os.path.getsize(compressed_file_path)

        file_compression_details = {
            "type": COMPRESSION_TYPE,
            "level": COMPRESSION_LEVEL,
            "major-version": COMPRESSION_MAJOR_VERSION,
            "minor-version": COMPRESSION_MINOR_VERSION,
            "original-file-name": file,
            "original-file-sha256hash": original_file_hash,
            "compressed-file-sha256hash": compressed_file_hash,
            "original-file-size": original_file_size,
            "compressed-file-size": compressed_file_size,
        }
        all_file_compression_details[compressed_file_name] = file_compression_details

    json_string = json.dumps(all_file_compression_details)
    json_path = os.path.join(folder, COMPRESSION_DETAILS_JSON_FILE_NAME)
    json_file_handle = open(json_path, "w")
    json_file_handle.write(json_string)
    json_file_handle.close()

if __name__ == "__main__":
    main(sys.argv[1:])