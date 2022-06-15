#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
sudo apt-get install -y g++-9
sudo apt-get install -y g++
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 10
sudo apt-get install -y pkg-config libmhash-dev cmake curl zip unzip tar autoconf autopoint libtool python3 python3-pip
pip3 install libconf zstandard