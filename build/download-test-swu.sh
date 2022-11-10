#!/bin/sh
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

output_folder="$1"

# Public ADU SWU files available for use
# https://github.com/Azure/iot-hub-device-update/releases/download/0.8.2/adu-update-image-raspberrypi3-0.8.8641.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.8.1/adu-update-image-raspberrypi3-0.8.7848.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.7.0/adu-update-image-raspberrypi3-0.7.5199.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.7.0-rc1/adu-update-image-raspberrypi3-0.6.5073.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.6.0/adu-update-image-raspberrypi3-.4889.1.swu

mkdir "${output_folder}"
source_swu_file="${output_folder}/source.swu"
target_swu_file="${output_folder}/target.swu" 

echo "Downloading files to ${source_swu_file} and ${target_swu_file}."
curl -L https://github.com/Azure/iot-hub-device-update/releases/download/0.8.1/adu-update-image-raspberrypi3-0.8.7848.1.swu --output ${source_swu_file}
curl -L https://github.com/Azure/iot-hub-device-update/releases/download/0.8.2/adu-update-image-raspberrypi3-0.8.8641.1.swu --output ${target_swu_file}
