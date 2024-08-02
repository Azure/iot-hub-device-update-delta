#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
sudo apt install zlib1g
wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb
sudo apt-get install dotnet-sdk-8.0