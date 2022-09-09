#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import os
import sys
import tempfile
import shutil

import helpers
from working_folder_manager import WorkingFolderManager

def create_new_content(working_folder_manager, input_archive_path, output_archive_path, zstd_compress_file_path):
    print(f'Extracting files from {input_archive_path}')
    helpers.extract_archive_files(input_archive_path, working_folder_manager.subfolder_old)

    helpers.show_folder_files(working_folder_manager.subfolder_old)

    print(f'Parsing sw-description file data')
    sw_description_path = helpers.get_sw_description_path(working_folder_manager.subfolder_old)
    sw_data = helpers.get_file_content(sw_description_path)
    config = helpers.get_libconfig(sw_data)
    images = helpers.get_sw_description_images(config)

    processed_image_filenames = set()
    strings_to_replace = {}
    for image in images:
        extension = os.path.splitext(image['filename'])[1]
        if extension.lower() != '.raw':
            if image['filename'] not in processed_image_filenames:
                print(f"Processing {image['filename']}")
                processed_image_filenames.add(image['filename'])

                old_compressed_path = helpers.get_combined_path(working_folder_manager.subfolder_old, image['filename'])
                decompressed_path = helpers.get_combined_path(working_folder_manager.subfolder_old, helpers.get_filename_without_extension(image['filename']))
                zstd_compressed_path = f'{decompressed_path}.zst'.replace(working_folder_manager.subfolder_old, working_folder_manager.subfolder_new)

                print(f'Decompressing {old_compressed_path}')
                helpers.decompress_file(old_compressed_path, decompressed_path, extension)

                print(f'Compressing {decompressed_path} with zstd')
                helpers.compress_file_with_zstd(zstd_compress_file_path, decompressed_path, working_folder_manager.subfolder_new)

                print(f'Hashing {zstd_compressed_path}')
                hash = helpers.get_file_hash(zstd_compressed_path)

                # Add the new filename and hash as strings to replace in the sw-description content, also set the compressed value to 'zstd' if it has a string value instead of a bool
                strings_to_replace[image['filename']] = os.path.basename(zstd_compressed_path)
                strings_to_replace[image['sha256']] = hash
                strings_to_replace[image['compressed']] = 'zstd'

            # Update image values for checking whether string replacement correctly creates a new sw-description file
            image['filename'] = strings_to_replace[image['filename']]
            image['sha256'] = strings_to_replace[image['sha256']]
            image['compressed'] = strings_to_replace[image['compressed']]
        else:
            # TEMPORARY FIX: Add .raw files to new archive staging folder without doing any compression/sw-description changes
            old_path = helpers.get_combined_path(working_folder_manager.subfolder_old, image['filename'])
            new_path = helpers.get_combined_path(working_folder_manager.subfolder_new, image['filename'])
            if not os.path.exists(new_path):
                shutil.copyfile(old_path, new_path)

    helpers.create_output_compression_details_file(working_folder_manager.subfolder_new, output_archive_path)

    print('Verifying new sw-description file content matches expected result.')
    new_sw_data = helpers.get_new_sw_description_data(sw_data, strings_to_replace)
    new_config = helpers.get_libconfig(new_sw_data)
    if config != new_config:
        raise Exception('New sw-description file content is incorrect.')

    print('Creating new sw-description file')
    new_sw_description_path = helpers.get_sw_description_path(working_folder_manager.subfolder_new)
    helpers.write_to_file(new_sw_description_path, new_sw_data)

    with open(working_folder_manager.list_file, "r") as list_file:
        input_files = list_file.readlines()
        for input_file in input_files:
            input_file = input_file.rstrip()
            old_path = helpers.get_combined_path(working_folder_manager.subfolder_old, input_file)
            new_path = helpers.get_combined_path(working_folder_manager.subfolder_new, input_file)
            if not os.path.exists(new_path):
                shutil.copyfile(old_path, new_path)

    helpers.show_folder_files(working_folder_manager.subfolder_new)

    return strings_to_replace

def execute(input_archive_path, output_archive_path, zstd_compress_file_path, signing_command):
    working_folder_manager = WorkingFolderManager(tempfile.gettempdir())

    print('Getting order of files from input archive')
    helpers.list_archive_files(input_archive_path, working_folder_manager.list_file)

    print('Creating new content')
    strings_to_replace = create_new_content(working_folder_manager, input_archive_path, output_archive_path, zstd_compress_file_path)

    have_sig_file = False

    if (signing_command):
        print('Signing new sw-description file')
        new_sw_description_path = helpers.get_sw_description_path(working_folder_manager.subfolder_new)
        helpers.sign_file(signing_command, new_sw_description_path)
        have_sig_file = True

    print('Creating output archive')
    helpers.create_archive(output_archive_path, working_folder_manager, strings_to_replace=strings_to_replace, have_sig_file=have_sig_file)

    if (not os.environ.get('RECOMPRESS_TOOL_DEBUG')):
        working_folder_manager.dispose()

def print_usage():
    print(f'{os.path.basename(sys.argv[0])} <input archive path> <output archive path> <zstd_compress_file path>')
    print(f'or {os.path.basename(sys.argv[0])} <input archive path> <output archive path> <zstd_compress_file path> \"<signing command>\"')

def main():
    if len(sys.argv) < 4 or len(sys.argv) > 5:
        print_usage()
        return

    input_archive_path = sys.argv[1]
    output_archive_path = sys.argv[2]
    zstd_compress_file_path = sys.argv[3]

    if len(sys.argv) == 5:
        signing_command = sys.argv[4]
    else:
        signing_command = ""

    if not os.path.exists(input_archive_path):
        raise OSError(f'Input file {input_archive_path} does not exist.')
    if not os.path.exists(zstd_compress_file_path):
        raise OSError(f'{zstd_compress_file_path} does not exist, cannot compress files.')

    execute(input_archive_path, output_archive_path, zstd_compress_file_path, signing_command)

if __name__ == '__main__':
    main()
