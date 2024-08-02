FLAVOR ?= amd64

VALID_FLAVOR=

ifeq ($(FLAVOR), amd64)
	CC=gcc
	CXX=g++
	ZLIB_A=/usr/lib/x86_64-linux-gnu/libz.a
	LIBMHASH_A=/usr/lib/x86_64-linux-gnu/libmhash.a
	VALID_FLAVOR=1
endif

ifeq ($(FLAVOR), arm64)
	CC=/usr/bin/aarch64-linux-gnu-gcc-10
	CXX=/usr/bin/aarch64-linux-gnu-g++-10
	VALID_FLAVOR=1

endif

ifeq ($(VALID_FLAVOR),)
	echo "No valid flavor specified. FLAVOR: $(FLAVOR)"
	exit 1
endif

OUTPUT_DIR=$(FLAVOR)/output/
OBJ_DIR=$(FLAVOR)/obj/

OUTPUT_FILE=$(OUTPUT_DIR)applydiff

all: $(OUTPUT_FILE)

$(OUTPUT_FILE): applydiff.c
	mkdir -p $(OUTPUT_DIR)
	$(CC) -Wall -Werror $(CCFLAGS) applydiff.c ../../diffs/api/$(OUTPUT_DIR)libadudiffexpand.so -o $(OUTPUT_FILE) -I../../diffs/api $(LIBMHASH_A) $(ZLIB_A)

clean:
	rm -r -f $(FLAVOR)
