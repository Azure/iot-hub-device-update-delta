#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import helpers
import os
import sys
import tempfile
from working_folder_manager import WorkingFolderManager

SW_DESCRIPTION_FILENAME = 'sw-description'

def execute(input_archive_path, output_archive_path, signing_command):
    working_folder_manager = WorkingFolderManager(tempfile.gettempdir())

    print(f'Extracting files from {input_archive_path}')
    helpers.extract_archive_files(input_archive_path, working_folder_manager.subfolder_new)

    print('Signing new sw-description file')
    new_sw_description_path = helpers.get_sw_description_path(working_folder_manager.subfolder_new)
    helpers.sign_file(signing_command, new_sw_description_path)

    print('Creating output compression details file from input')
    helpers.copy_input_compression_details_file_to_output(input_archive_path, output_archive_path)

    print('Getting order of files from input archive')
    helpers.list_archive_files(input_archive_path, working_folder_manager.list_file)

    print('Creating output archive')
    helpers.create_archive(output_archive_path, working_folder_manager, have_sig_file=True)

    working_folder_manager.dispose()

def print_usage():
    print(f'{os.path.basename(sys.argv[0])} <input test archive path> <output archive path> \"<signing command>\"')

def main():
    if len(sys.argv) == 4:
        input_archive_path = sys.argv[1]
        output_archive_path = sys.argv[2]
        signing_command = sys.argv[3]
        execute(input_archive_path, output_archive_path, signing_command)
    else:
        print_usage()

if __name__ == '__main__':
    main()