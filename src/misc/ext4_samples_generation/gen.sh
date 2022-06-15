#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

#preparation
rm -r working
rm -r result
mkdir working
mkdir working/mount_folder
mkdir result

SAMPLE_CONTENTS="1 2 3"

for i in $SAMPLE_CONTENTS;do
    #create and mount ext4 file
    dd if=/dev/zero of=working/img.ext4 bs=4k count=1000
    mkfs.ext4 working/img.ext4
    tune2fs -c0 -i0 working/img.ext4
    mount -o loop working/img.ext4 working/mount_folder/

    #copy files into it
    cp -r inputs/$i/* working/mount_folder
    #ls -R working/mount_folder

    #and close the file
    sync
    umount working/mount_folder/
    mv working/img.ext4 result/sample$i.ext4
done

#cleanup
rm -r working
