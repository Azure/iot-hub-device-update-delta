# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $repoRoot)

# Public ADU SWU files available for use
# https://github.com/Azure/iot-hub-device-update/releases/download/0.8.2/adu-update-image-raspberrypi3-0.8.8641.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.8.1/adu-update-image-raspberrypi3-0.8.7848.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.7.0/adu-update-image-raspberrypi3-0.7.5199.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.7.0-rc1/adu-update-image-raspberrypi3-0.6.5073.1.swu
# https://github.com/Azure/iot-hub-device-update/releases/download/0.6.0/adu-update-image-raspberrypi3-.4889.1.swu

mkdir "$repoRoot/src/managed/DiffGen/tests/samples/diffs/swu"
curl -L https://github.com/Azure/iot-hub-device-update/releases/download/0.8.1/adu-update-image-raspberrypi3-0.8.7848.1.swu --output "$repoRoot/src/managed/DiffGen/tests/samples/diffs/swu/source.swu"
curl -L https://github.com/Azure/iot-hub-device-update/releases/download/0.8.2/adu-update-image-raspberrypi3-0.8.8641.1.swu --output "$repoRoot/src/managed/DiffGen/tests/samples/diffs/swu/target.swu"


