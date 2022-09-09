#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
chmod +x dumpextfs
chmod +x applydiff
chmod +x dumpdiff
chmod +x bsdiff
chmod +x bspatch
chmod +x zstd_compress_file
chmod +x recompress_tool.py
chmod +x helper.py
chmod +x DiffGenTool

debian_packages=( ./ms-adu_diffs*.deb )
sudo dpkg -i "${debian_packages[0]}"