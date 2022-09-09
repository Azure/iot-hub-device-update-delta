# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import argparse
import json
import os
import shutil
import subprocess
import tempfile
import unittest
import uuid

import sys
sys.path.insert(0, os.path.abspath(f'{__file__}/../../src'))
import helpers

GPG_KEY_ID = 'gpg_key_id'

# global string to let user pass in the path of their local version zstd_compress_file
zstd_compress_file = ''

class TestIntegration(unittest.TestCase):
    output_folder = helpers.get_combined_path(tempfile.gettempdir(), str(uuid.uuid4()))
    repo_root_folder = os.path.abspath(f'{__file__}/../../../../..')

    original_archive_path = helpers.get_combined_path(repo_root_folder, 'data/source/yocto.swu')
    test_archive_path = helpers.get_combined_path(output_folder, 'yocto_RECOMPRESSED.swu')
    production_archive_path = helpers.get_combined_path(output_folder, 'yocto_RECOMPRESSED_and_RE-SIGNED.swu')

    original_archive_folder_path = helpers.get_combined_path(output_folder, helpers.get_filename_without_extension(original_archive_path))
    test_archive_folder_path = helpers.get_combined_path(output_folder, helpers.get_filename_without_extension(test_archive_path))
    production_archive_folder_path = helpers.get_combined_path(output_folder, helpers.get_filename_without_extension(production_archive_path))

    @classmethod
    def setUpClass(cls):
        helpers.create_folder(cls.output_folder)
        cls.create_gpg_key()

        cls.create_test_archive(cls.original_archive_path, cls.test_archive_path)
        cls.create_production_archive(cls.test_archive_path, cls.production_archive_path)

        cls.extract_archive(cls.original_archive_path)
        cls.extract_archive(cls.test_archive_path)
        cls.extract_archive(cls.production_archive_path)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.output_folder)
        cls.delete_gpg_key()

    @classmethod
    def create_gpg_key(cls):
        cls.print_log_message('Creating temporary gpg key for signing.')
        subprocess.run(['gpg', '--batch', '--passphrase', '', '--quick-gen-key', GPG_KEY_ID, 'default', 'default'])

    @classmethod
    def delete_gpg_key(cls):
        cls.print_log_message('Deleting gpg key.')
        result = subprocess.run(['gpg', '--list-keys', GPG_KEY_ID], stdout=subprocess.PIPE)
        key_fingerprint = result.stdout.decode().split('\n')[1].strip()
        subprocess.run(['gpg', '--batch', '--yes', '--delete-secret-key', key_fingerprint])
        subprocess.run(['gpg', '--batch', '--yes', '--delete-key', key_fingerprint])

    @classmethod
    def create_test_archive(cls, input_archive_path, output_archive_path):
        cls.print_log_message('Running recompress_tool.py to create the test archive file.')
        recompression_script = helpers.get_combined_path(cls.repo_root_folder, 'src/scripts/recompress_swu/src/recompress_tool.py')
        subprocess.run(['python3', recompression_script, input_archive_path, output_archive_path, zstd_compress_file])

    @classmethod
    def create_production_archive(cls, input_archive_path, output_archive_path):
        cls.print_log_message('Running sign_tool.py to create the production archive file.')
        sign_content_script = helpers.get_combined_path(cls.repo_root_folder, 'src/scripts/recompress_swu/src/sign_tool.py')
        subprocess.run(['python3', sign_content_script, input_archive_path, output_archive_path, 'gpg --detach-sign'])

    @classmethod
    def extract_archive(cls, archive_path):
        extract_folder_name = helpers.get_filename_without_extension(archive_path)
        extract_folder = helpers.get_combined_path(cls.output_folder, extract_folder_name)

        cls.print_log_message(f'Extracting contents of {archive_path} to {extract_folder}.')
        helpers.create_folder(extract_folder)
        helpers.extract_archive_files(archive_path, extract_folder)

    @classmethod
    def print_log_message(cls, message):
        print(f'\n{message}')

    def test_output_archives_have_correct_files(self):
        self.print_run_test_method_message(self.id())

        original_archive_files = os.listdir(self.original_archive_folder_path)
        test_archive_files = os.listdir(self.test_archive_folder_path)
        production_archive_files = os.listdir(self.production_archive_folder_path)

        print('\tChecking number of files in the test and production archives.')

        assert len(test_archive_files) == len(original_archive_files) - 1
        assert len(production_archive_files) == len(original_archive_files)

        original_archive_image_files = filter(lambda file: 'sw-description' not in file, original_archive_files)
        for image_file in original_archive_image_files:
            expected_image_filename = f'{helpers.get_filename_without_extension(image_file)}.zst'
            print(f'\tChecking output archives contain file \'{expected_image_filename}\'.')
            assert expected_image_filename in test_archive_files
            assert expected_image_filename in production_archive_files

        print('\tChecking output archives have the correct sw-description/compression-details files.')
        assert 'sw-description' in test_archive_files
        assert 'sw-description.sig' not in test_archive_files
        assert 'sw-description' in production_archive_files
        assert 'sw-description.sig' in production_archive_files

    def test_output_archive_image_files_are_compressed_with_zstd(self):
        self.print_run_test_method_message(self.id())

        print('\tChecking image files in test archive are compressed with zstd.')
        self.check_files_are_compressed_with_zstd(self.test_archive_folder_path)

        print('\tChecking image files in production archive are compressed with zstd.')
        self.check_files_are_compressed_with_zstd(self.production_archive_folder_path)

    def check_files_are_compressed_with_zstd(self, archive_folder_path):
        files = os.listdir(archive_folder_path)
        for file in filter(lambda file: file.endswith('.zst'), files):
            path = helpers.get_combined_path(archive_folder_path, file)
            self.check_file_is_compressed_with_zstd(path)

    def check_file_is_compressed_with_zstd(self, path):
        result = subprocess.run(['file', path], stdout=subprocess.PIPE)
        print(f'\t\tChecking the file \'{os.path.basename(path)}\' has \'zstandard\' in its file information.')
        assert 'zstandard' in result.stdout.decode().lower()

    def test_verify_output_sw_description_files_are_correct(self):
        self.print_run_test_method_message(self.id())

        original_sw_description_path = helpers.get_combined_path(self.original_archive_folder_path, 'sw-description')
        test_sw_description_path = helpers.get_combined_path(self.test_archive_folder_path, 'sw-description')
        production_sw_description_path = helpers.get_combined_path(self.production_archive_folder_path, 'sw-description')

        original_sw_data = helpers.get_file_content(original_sw_description_path)
        test_sw_data = helpers.get_file_content(test_sw_description_path)
        production_sw_data = helpers.get_file_content(production_sw_description_path)

        print('\tChecking contents of test archive sw-description file.')
        self.check_sw_description_file_contents(original_sw_data, test_sw_data, self.test_archive_folder_path)

        print('\tChecking contents of production archive sw-description file.')
        self.check_sw_description_file_contents(original_sw_data, production_sw_data, self.production_archive_folder_path)

    def check_sw_description_file_contents(self, original_sw_data, new_sw_data, archive_folder_path):
        config = helpers.get_libconfig(original_sw_data)
        images = helpers.get_sw_description_images(config)
        for image in images:
            filename = image['filename']
            hash = image['sha256']

            print(f'\t\tGetting name/hash of zstd file corresponding to {filename}.')
            zstd_filename = f'{helpers.get_filename_without_extension(filename)}.zst'
            zstd_path = helpers.get_combined_path(archive_folder_path, zstd_filename)
            zstd_hash = helpers.get_file_hash(zstd_path)

            original_sw_data = original_sw_data.replace(filename, zstd_filename).replace(hash, zstd_hash).replace('true', '\"zstd\"')

        print(f'\t\tVerifying only changes to the sw-description file are the filenames/hashes for the zstd compressed files.')
        assert original_sw_data == new_sw_data

    def test_verify_compression_details_files_are_correct(self):
        self.print_run_test_method_message(self.id())

        test_compression_details_path = helpers.get_compression_details_path(self.test_archive_path)
        production_compression_details_path = helpers.get_compression_details_path(self.production_archive_path)

        test_data = self.load_json_file(test_compression_details_path)
        production_data = self.load_json_file(production_compression_details_path)

        print('\tChecking test and production compression-details.json files are identical.')
        assert test_data == production_data

        print('\tChecking contents of test archive compression-details.json file.')
        self.check_compression_details_file_contents(test_data, self.test_archive_folder_path)

        print('\tChecking contents of production archive compression-details.json file.')
        self.check_compression_details_file_contents(production_data, self.production_archive_folder_path)

    def load_json_file(self, path):
        with open(path, 'r') as file:
            return json.load(file)

    def check_compression_details_file_contents(self, data, archive_folder_path):
        for key in data.keys():
            zstd_filename = key
            decompressed_filename = helpers.get_filename_without_extension(zstd_filename)
            zstd_path = helpers.get_combined_path(archive_folder_path, zstd_filename)
            decompressed_path = helpers.get_combined_path(self.output_folder, decompressed_filename)

            print(f'\t\t Making temporary decompressed file {decompressed_path}.')
            helpers.decompress_file(zstd_path, decompressed_path, '.zst')

            print(f'\t\t Verifying the {decompressed_filename} JSON element information is correct.')
            assert data[key]['type'] == 'zstd'
            assert data[key]['original-file-name'] == decompressed_filename
            assert data[key]['original-file-sha256hash'] == helpers.get_file_hash(decompressed_path)
            assert data[key]['compressed-file-sha256hash'] == helpers.get_file_hash(zstd_path)
            assert data[key]['original-file-size'] == os.path.getsize(decompressed_path)
            assert data[key]['compressed-file-size'] == os.path.getsize(zstd_path)

            print(f'\t\t Deleting {decompressed_path}.')
            os.remove(decompressed_path)

    def print_run_test_method_message(self, full_method_name):
        method_name = full_method_name.split('.')[2]
        print(f'\nRunning {method_name}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--zstd_compress_file')
    parser.add_argument('unittest_args', nargs='*')
    args = parser.parse_args()

    if args.zstd_compress_file:
        zstd_compress_file = args.zstd_compress_file
        sys.argv[1:] = args.unittest_args
        unittest.main()
    else:
        print(f'{os.path.basename(sys.argv[0])} --zstd_compress_file <zstd_compress_file_path>')
