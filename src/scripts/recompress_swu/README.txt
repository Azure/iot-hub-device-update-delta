
These scripts are used for decompressing an archive's image files and recompressing them with zstd.
This ensures all archive files used for diff generation have the same compression algorithm for the images inside the archives.

Commands to install required packages/libraries:
$ sudo apt update
$ sudo apt-get install -y python3 python3-pip
$ sudo pip3 install libconf zstandard

Tool Usage:
There are two ways to use the recompression scripts:
    1. Run recompress_tool.py to create a test archive and then sign_tool.py to create the final production archives
    2. Run recompress_tool.py with a signing_command parameter to run the full E2E workflow

    Recompressing/signing in separate steps
    $ python3 recompress_tool.py <input archive path> <output test archive path> <zstd_compress_file path>
    $ python3 sign_tool.py sign <input test archive path> <output archive path> "<signing command>"
        example "<signing command>": "gpg --detach-sign"

    Recompressing/signing in one step
    $ python3 recompress_tool.py <input archive path> <output archive path> <zstd_compress_file path> "<signing command>"
        example "<signing command>": "gpg --detach-sign"
