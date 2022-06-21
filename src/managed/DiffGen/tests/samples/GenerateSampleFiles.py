# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

samples_directory = os.path.dirname(os.path.abspath(__file__))
diffs_directory = samples_directory + '\\diffs'
temp_diffs_directory = samples_directory + '\\TEMP'

diff_gen_directory = samples_directory.replace('\\tests\\samples', '')
diff_gen_demo_exe = diff_gen_directory + '\\demos\\DiffGenDemo\\bin\\x64\\Debug\\net5.0\\win-x64\\DiffGenDemo.exe'
archive_loader_demo_exe = diff_gen_directory + '\\demos\\ArchiveLoaderDemo\\bin\\Debug\\net5.0\\ArchiveLoaderDemo.exe'
json_expansion_demo_exe = diff_gen_directory + '\\demos\\JSONExpansionDemo\\bin\\Debug\\net5.0\\JSONExpansionDemo.exe'

def runProcess(arg_string):
    print(f'Calling "{arg_string}"')
    args = arg_string.split(' ')

    if not os.path.exists(args[0]):
        raise Exception(f' Executable {args[0]} does not exist!')

    p = subprocess.run(args, stdout=subprocess.PIPE)

def createDirectoryForFilePath(path):
    directory = os.path.dirname(path)
    if not os.path.isdir(directory):
        os.mkdir(directory)

def copyDiffLogs(logs, working):
    for path in Path(logs).rglob('diff.json'):
        with open(path, 'r') as reader:
            data = reader.read()
            double_backslashed_working = working.replace('\\', '\\\\')
            modified_data = data.replace(double_backslashed_working, '')

            sample_path = str(path).replace('TEMP', 'diffs')
            createDirectoryForFilePath(sample_path)

            with open(sample_path, 'w') as writer:
                writer.write(modified_data)

def runDiffGen(test_case):
    source = diffs_directory + f'\\{test_case}\\source.cpio'
    target = diffs_directory + f'\\{test_case}\\target.cpio'
    output = temp_diffs_directory + f'\\{test_case}\\output.file'
    logs = temp_diffs_directory + f'\\{test_case}\\logs'
    working = temp_diffs_directory + f'\\{test_case}\\working'

    runProcess(f'{diff_gen_demo_exe} {source} {target} {output} {logs} {working}')

    print(f'Copying log files for {test_case} test case\n')
    copyDiffLogs(logs, working)

    shutil.rmtree(temp_diffs_directory)

def runArchiveLoader(archive_path, sample_path):
    runProcess(f'{archive_loader_demo_exe} {archive_path}')

    file_name = os.path.basename(sample_path)
    temp_file_path = tempfile.gettempdir() + '\\' + file_name

    createDirectoryForFilePath(sample_path)
    shutil.copy(temp_file_path, sample_path)

def runJsonExpander(archive_path, json_path, expansion_folder_name):
    chunk_folder = samples_directory + f'\\expanded_chunks\\{expansion_folder_name}'
    runProcess(f'{json_expansion_demo_exe} step3 {archive_path} {json_path} {chunk_folder}')

    payload_folder = samples_directory + f'\\expanded_payloads\\{expansion_folder_name}'
    runProcess(f'{json_expansion_demo_exe} step4 {archive_path} {json_path} {payload_folder}')

def runArchive(file_subpath, expansion_folder_name):
    archive_path = samples_directory + '\\' + file_subpath
    json_path = archive_path.replace('\\archives\\', '\\archive_manifests\\') + '.json'

    runArchiveLoader(archive_path, json_path)
    runJsonExpander(archive_path, json_path, expansion_folder_name)

runDiffGen('simple')
runDiffGen('nested')
runDiffGen('complex')

runArchive('archives\\cpio1.swu', 'cpio1')
runArchive('archives\\cpio2.cpio', 'cpio2')
runArchive('archives\\cpio3.cpio', 'cpio3')
runArchive('archives\\cpio4.cpio', 'cpio4')
runArchive('archives\\cpio5.cpio', 'cpio5')
runArchive('archives\\cpio6.cpio', 'cpio6')
runArchive('archives\\ext4\\sample1.ext4', 'ext4_1')
runArchive('archives\\ext4\\sample2.ext4', 'ext4_2')
runArchive('archives\\ext4\\sample3.ext4', 'ext4_3')
