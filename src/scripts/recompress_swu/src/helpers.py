# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import glob
import gzip
import hashlib
import json
import libconf
import os
import shutil
import subprocess
import zstandard
import hashlib
from subprocess import PIPE

def create_folder(folder_path):
    os.makedirs(folder_path, exist_ok=True)

def get_combined_path(*args):
    return os.sep.join(args)

def get_filename_without_extension(path):
    return os.path.splitext(os.path.basename(path))[0]

def get_file_content(path):
    with open(path, 'r') as file:
        return file.read()

def get_file_hash(path):
    with open(path, 'rb') as file:
        bytes = file.read()
        return hashlib.sha256(bytes).hexdigest()

def get_libconfig(data):
    return libconf.loads(data)

def write_to_file(path, data):
    with open(path, 'w') as file:
        file.write(data)

def sign_file(signing_command, path):
    full_signing_command = f'{signing_command} {path}'
    folder = os.path.dirname(path)
    result = subprocess.run(full_signing_command.split(' '), cwd=folder, stdout=PIPE, stderr=PIPE)
    if result.returncode != 0:
        raise Exception(f'Failed to sign file, ReturnCode={result.returncode}, StdErr={result.stderr.decode()}')

    sig_file_path = path + ".sig"
    if not os.path.exists(sig_file_path):
        raise Exception(f'Sig file missing: {sig_file_path}')

def decompress_file(file_path, decompressed_path, extension):
    if extension == '.gz':
        with gzip.open(file_path, 'rb') as compressed:
            with open(decompressed_path, 'wb') as decompressed:
                decompressed.writelines(compressed)
    elif extension == '.zst':
        with open(file_path, 'rb') as compressed:
            decompressor = zstandard.ZstdDecompressor()
            with open(decompressed_path, 'wb') as decompressed:
                decompressor.copy_stream(compressed, decompressed)
    else:
        raise Exception(f'Decompression not implemented for extension {extension} ({file_path})')

def compress_file_with_zstd(zstd_compress_file, decompressed_path, target_folder):
    # First check python folder for compress_files.py, then if not there use the relative path in the ADU repo instead
    compress_files_script = os.path.abspath(f'{__file__}/../compress_files.py')
    if not os.path.exists(compress_files_script):
        compress_files_script = os.path.abspath(f'{__file__}/../../../../scripts/compress_files/compress_files.py')

    script_command = f'python3 {compress_files_script} {zstd_compress_file} {os.path.dirname(decompressed_path)} {os.path.basename(decompressed_path)}'.split(' ')
    subprocess.run(script_command, stdout=PIPE, stderr=PIPE)

    # Copy .zst and compression-details files to target folder
    compressed_path = f'{decompressed_path}.zst'
    compressed_filename = os.path.basename(compressed_path)
    shutil.copyfile(compressed_path, get_combined_path(target_folder, compressed_filename))
    compression_details_source_path = os.path.abspath(f'{decompressed_path}/../compression-details.json')
    compression_details_target_filename = f'compression-details-{os.path.basename(decompressed_path)}.json'
    shutil.copyfile(compression_details_source_path, get_combined_path(target_folder, compression_details_target_filename))

def extract_archive_files(archive_path, extract_folder):
    with open(archive_path, 'r') as archive:
        subprocess.run(['cpio', '-iv'], stdin=archive, cwd=extract_folder, stdout=PIPE, stderr=PIPE)

def get_file_hash(path):
    sha256 = hashlib.sha256()

    READ_CHUNK_SIZE = 64 * 1024
    remaining = os.path.getsize(path)

    with open(path, 'rb') as file:
        while remaining > 0:
            contents = file.read(READ_CHUNK_SIZE)
            sha256.update(contents)
            remaining = remaining - len(contents)

    return sha256.hexdigest()

def show_folder_files(folder):
    files = os.listdir(folder)
    print(f'Files in {folder}:')
    for file in files:
        full_path = get_combined_path(folder, file)
        file_size = os.path.getsize(full_path)
        file_hash = get_file_hash(full_path)

        print(f'\t{file} ({file_size} bytes) ({file_hash} SHA256 Hash)')

def list_archive_files(archive_path, list_path):
    with open(archive_path, 'r') as archive:
        with open(list_path, 'w') as list:
            subprocess.run(['cpio', '--list'], stdin=archive, stdout=list)

def fix_file_list(list_path, strings_to_replace, should_have_sig_file):
    with open(list_path, 'r') as list:
        lines = list.readlines()

    for string in strings_to_replace:
        value = f'{string}\n'
        if value in lines:
            index = lines.index(value)
            lines[index] = f'{strings_to_replace[string]}\n'

    do_have_sig_file = ('sw-description.sig\n' in lines)
    if should_have_sig_file and not do_have_sig_file:
        manifest_index = lines.index('sw-description\n')
        lines.insert(manifest_index + 1, 'sw-description.sig\n')
    elif not should_have_sig_file and do_have_sig_file:
        lines.remove('sw-description.sig\n')

    with open(list_path, 'w') as list:
        lines = list.writelines(lines)

def create_archive(archive_path, working_folder_manager, strings_to_replace={}, have_sig_file=False):
    list_path = working_folder_manager.list_file
    content_folder = working_folder_manager.subfolder_new
    fix_file_list(list_path, strings_to_replace, have_sig_file)

    with open(list_path, 'r') as list:
        with open(archive_path, 'w') as archive:
            subprocess.run(['cpio', '-ov', '--format=crc'], stdin=list, stdout=archive, cwd=content_folder)

def get_compression_details_path(archive_path):
    archive_folder = os.path.dirname(archive_path)
    archive_name = get_filename_without_extension(os.path.basename(archive_path))
    return get_combined_path(archive_folder, f'{archive_name}.compression-details.json')

def create_output_compression_details_file(folder, output_archive_path):
    match_string = get_combined_path(folder, 'compression-details*.json')
    compression_details_file_paths = glob.glob(match_string)

    json_dict = {}
    for path in compression_details_file_paths:
        with open(path, 'r') as file:
            json_data = json.load(file)
            json_dict.update(json_data)
        os.remove(path)

    output_compression_details_path = get_compression_details_path(output_archive_path)
    with open(output_compression_details_path, 'w') as output_file:
        json.dump(json_dict, output_file)

def copy_input_compression_details_file_to_output(input_archive_path, output_archive_path):
    input_compression_details_path = get_compression_details_path(input_archive_path)
    output_compression_details_path = get_compression_details_path(output_archive_path)
    shutil.copyfile(input_compression_details_path, output_compression_details_path)

def get_sw_description_path(folder):
    return get_combined_path(folder, 'sw-description')

def get_sw_description_images(config):
    key1 = 'software'
    key3 = 'stable'
    images = []

    for key2 in list(config[key1].keys())[1:]:
        stable = config[key1][key2][key3]
        for key4 in stable.keys():
            images.extend(stable[key4]['images'])

    return images

def get_sw_description_formatted_string(input):
    return f'\"{input}\";'

def get_sw_description_formatted_bool_string(input):
    return f'{str(input).lower()};'

def get_new_sw_description_data(sw_data, strings_to_replace):
    new_sw_data = sw_data
    for key in strings_to_replace:
        if type(key) == str:
            original_value = get_sw_description_formatted_string(key)
            replace_value = get_sw_description_formatted_string(strings_to_replace[key])
            new_sw_data = new_sw_data.replace(original_value, replace_value)
        elif type(key) == bool:
            original_value = get_sw_description_formatted_bool_string(key)
            replace_value = get_sw_description_formatted_string(strings_to_replace[key])
            new_sw_data = new_sw_data.replace(original_value, replace_value)
    return new_sw_data
