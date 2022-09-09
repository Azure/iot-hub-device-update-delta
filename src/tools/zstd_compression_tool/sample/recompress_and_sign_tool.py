#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import helpers
import os
import sys
import tempfile
from recompress_tool import create_new_content
from working_folder_manager import WorkingFolderManager

def execute(input_archive_path, output_archive_path, zstd_compress_file_path, signing_command):
    working_folder_manager = WorkingFolderManager(tempfile.gettempdir())

    print('Getting order of files from input archive')
    helpers.list_archive_files(input_archive_path, working_folder_manager.list_file)

    print('Creating new content')
    strings_to_replace = create_new_content(working_folder_manager, input_archive_path, output_archive_path, zstd_compress_file_path)

    print('Signing new sw-description file')
    new_sw_description_path = helpers.get_sw_description_path(working_folder_manager.subfolder_new)
    helpers.sign_file(signing_command, new_sw_description_path)

    print('Creating output archive')
    helpers.create_archive(output_archive_path, working_folder_manager, strings_to_replace=strings_to_replace, have_sig_file=True)

    working_folder_manager.dispose()

def print_usage():
    print(f'{os.path.basename(sys.argv[0])} <input archive path> <output archive path> <zstd_compress_file path> \"<signing command>\"')

def main():
    if len(sys.argv) == 5:
        input_archive_path = sys.argv[1]
        output_archive_path = sys.argv[2]
        zstd_compress_file_path = sys.argv[3]
        signing_command = sys.argv[4]

        if not os.path.exists(input_archive_path):
            raise OSError(f'Input file {input_archive_path} does not exist.')
        if not os.path.exists(zstd_compress_file_path):
            raise OSError(f'{zstd_compress_file_path} does not exist, cannot compress files.')

        execute(input_archive_path, output_archive_path, zstd_compress_file_path, signing_command)
    else:
        print_usage()
    for arg in sys.argv:
        print(arg)

if __name__ == '__main__':
    main()