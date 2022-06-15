/**
 * @file compress_utility.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

void compress_file(fs::path uncompressed_path, std::vector<char> *dictionary, fs::path compressed_path);
void compress_file(fs::path uncompressed_path, fs::path compressed_path);
void compress_file(fs::path uncompressed_path, fs::path delta_basis_path, fs::path compressed_path);

void verify_compression(fs::path uncompressed_path, fs::path *delta_basis_path, fs::path compressed_path);

void decompress_file(fs::path compressed_path, std::vector<char> *dictionary, fs::path uncompressed_path);
void decompress_file(fs::path compressed_path, fs::path uncompressed_path);
void decompress_file(fs::path compressed_path, fs::path delta_basis_path, fs::path uncompressed_path);