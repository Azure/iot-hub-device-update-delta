/**
 * @file dump_json.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>

#include "ext2fs/ext2_types.h"
#include "et/com_err.h"
#include "ext2fs/ext2_io.h"
#include <ext2fs/ext2_ext_attr.h>
#include "ext2fs/ext2fs.h"

#include <string>
#include <map>
#include <vector>
#include <iterator>

#include "file_details.h"

class unique_id_provider
{
	private:
	int nextFreeId = 0;
	std::map<std::string, int> archive_item_to_id;

	public:
	int get_id_for_archive_item(const std::string &key, bool *new_item)
	{
		bool new_item_value = (archive_item_to_id.count(key) == 0);

		if (new_item_value)
		{
			archive_item_to_id[key] = nextFreeId++;
		}

		if (new_item != nullptr)
		{
			*new_item = new_item_value;
		}

		return archive_item_to_id[key];
	}

	int get_id_for_payload(const file_details &details) { return get_id_for_archive_item(details.full_path, nullptr); }

	int get_id_for_chunk(const file_region &region, bool *new_item = nullptr)
	{
		return get_id_for_archive_item(std::to_string(region.offset), new_item);
	}
};

static std::string get_chunk_name_for_file_region(const file_region &region)
{
	if (region.all_zeroes)
	{
		return std::string("AllZeroChunk" + std::to_string(region.length));
	}
	else
	{
		return std::string("Chunk@" + std::to_string(region.offset));
	}
}

static void dump_copy_recipe(FILE *stream, const file_details &details, int payloadId)
{
	fprintf(stream, "        {\n");
	fprintf(stream, "          \"Name\": \"Copy\",\n");
	fprintf(stream, "          \"Parameters\": {\n");
	fprintf(stream, "            \"Item\": {\n");
	fprintf(stream, "              \"Type\": \"ArchiveItem\",\n");
	fprintf(stream, "              \"Item\": {\n");
	fprintf(stream, "                \"Id\": %d,\n", payloadId);
	fprintf(stream, "                \"Name\": \"%s\",\n", details.full_path.c_str());
	fprintf(stream, "                \"Type\": \"Payload\",\n");
	fprintf(stream, "                \"Length\": %llu,\n", details.size);
	fprintf(stream, "                \"Hashes\": {\n");
	fprintf(stream, "                  \"Sha256\": \"%s\",\n", details.hash_sha256_string.c_str());
	fprintf(stream, "                  \"Md5\": \"%s\"\n", details.hash_md5_string.c_str());
	fprintf(stream, "                }\n");
	fprintf(stream, "              }\n");
	fprintf(stream, "            }\n");
	fprintf(stream, "          }\n");
	fprintf(stream, "        }\n");
}

static void dump_region_recipe(
	FILE *stream, const file_details &details, const file_region &region, int payloadId, __u64 offset_in_payload)
{
	fprintf(stream, "        {\n");
	fprintf(stream, "          \"Name\": \"Region\",\n");
	fprintf(stream, "          \"Parameters\": {\n");
	fprintf(stream, "            \"Item\": {\n");
	fprintf(stream, "              \"Type\": \"ArchiveItem\",\n");
	fprintf(stream, "              \"Item\": {\n");
	fprintf(stream, "                \"Id\": %d,\n", payloadId);
	fprintf(stream, "                \"Name\": \"%s\",\n", details.full_path.c_str());
	fprintf(stream, "                \"Type\": \"Blob\",\n");
	fprintf(stream, "                \"Length\": %llu,\n", details.size);
	fprintf(stream, "                \"Hashes\": {\n");
	fprintf(stream, "                  \"Sha256\": \"%s\",\n", details.hash_sha256_string.c_str());
	fprintf(stream, "                  \"Md5\": \"%s\"\n", details.hash_md5_string.c_str());
	fprintf(stream, "                },\n");
	fprintf(stream, "                \"Recipes\": [\n");
	dump_copy_recipe(stream, details, payloadId);
	fprintf(stream, "                ]\n");
	fprintf(stream, "              }\n");
	fprintf(stream, "            },\n");
	fprintf(stream, "            \"Offset\": {\n");
	fprintf(stream, "              \"Type\": \"Number\",\n");
	fprintf(stream, "              \"Number\": %llu\n", offset_in_payload);
	fprintf(stream, "            },\n");
	fprintf(stream, "            \"Length\": {\n");
	fprintf(stream, "              \"Type\": \"Number\",\n");
	fprintf(stream, "              \"Number\": %llu\n", region.length);
	fprintf(stream, "            }\n");
	fprintf(stream, "          }\n");
	fprintf(stream, "        }\n");
}

static void dump_file_chunks(
	unique_id_provider &id_provider, FILE *stream, const file_details &details, bool last_details)
{
	__u64 offset_in_payload = 0;
	int payloadId           = id_provider.get_id_for_payload(details);

	bool need_comma = false;

	for (int i = 0; i < details.regions.size(); i++)
	{
		const auto &region  = details.regions[i];
		auto current_offset = offset_in_payload;
		offset_in_payload += region.length;

		if (region.offset == -1)
		{
			continue;
		}

		bool new_id  = false;
		int chunk_id = id_provider.get_id_for_chunk(region, &new_id);
		if (!new_id)
		{
			continue;
		}

		// only output when we get to the next so we don't add an extra
		// comma when skipping
		if (need_comma)
		{
			fprintf(stream, ",\n");
		}

		need_comma = true;

		std::string chunk_name = get_chunk_name_for_file_region(region);
		fprintf(stream, "    {\n");
		fprintf(stream, "      \"Id\": %d,\n", chunk_id);
		fprintf(stream, "      \"Name\": \"%s\",\n", chunk_name.c_str());
		fprintf(stream, "      \"Type\": \"Chunk\",\n");
		fprintf(stream, "      \"Offset\": %llu,\n", region.offset);
		fprintf(stream, "      \"Length\": %llu,\n", region.length);
		fprintf(stream, "      \"Hashes\": {\n");
		fprintf(stream, "        \"Sha256\": \"%s\",\n", region.hash_sha256_string.c_str());
		fprintf(stream, "        \"Md5\": \"%s\"\n", region.hash_md5_string.c_str());
		fprintf(stream, "      },\n");
		fprintf(stream, "      \"Recipes\": [\n");

		if (region.length == details.size)
		{
			dump_copy_recipe(stream, details, payloadId);
		}
		else
		{
			dump_region_recipe(stream, details, region, payloadId, current_offset);
		}

		fprintf(stream, "      ]\n");
		fprintf(stream, "    }");
	}

	// don't add last comma if this is the last details
	if (need_comma && !last_details)
	{
		fprintf(stream, ",\n");
	}
}

static void dump_file_payload(unique_id_provider &idProvider, FILE *stream, const file_details &details)
{
	int payloadId = idProvider.get_id_for_payload(details);
	fprintf(stream, "    {\n");
	fprintf(stream, "      \"Id\": %d,\n", payloadId);
	fprintf(stream, "      \"Name\": \"%s\",\n", details.full_path.c_str());
	fprintf(stream, "      \"Type\": \"Payload\",\n");
	fprintf(stream, "      \"Length\": %llu,\n", details.size);
	fprintf(stream, "      \"Hashes\": {\n");
	fprintf(stream, "        \"Sha256\": \"%s\",\n", details.hash_sha256_string.c_str());
	fprintf(stream, "        \"Md5\": \"%s\"\n", details.hash_md5_string.c_str());
	fprintf(stream, "      },\n");
	fprintf(stream, "      \"Recipes\": [\n");
	fprintf(stream, "        {\n");
	fprintf(stream, "          \"Name\": \"%s\",\n", (details.regions.size() == 1) ? "Copy" : "Concatenation");
	fprintf(stream, "          \"Parameters\": {\n");

	for (int i = 0; i < details.regions.size(); i++)
	{
		if (details.regions.size() == 1)
		{
			fprintf(stream, "            \"Item\": {\n");
		}
		else
		{
			fprintf(stream, "            \"%d\": {\n", i);
		}
		const auto &region     = details.regions[i];
		std::string chunk_name = get_chunk_name_for_file_region(region);
		int chunk_id           = idProvider.get_id_for_chunk(region);

		if (region.all_zeroes)
		{
			fprintf(stream, "              \"Type\": \"ArchiveItem\",\n");
			fprintf(stream, "              \"Item\": {\n");
			fprintf(stream, "                \"Id\": %d,\n", chunk_id);
			fprintf(stream, "                \"Name\": \"%s\",\n", chunk_name.c_str());
			fprintf(stream, "                \"Type\": \"Blob\",\n");
			fprintf(stream, "                \"Length\": %llu,\n", region.length);
			fprintf(stream, "                \"Hashes\": {\n");
			fprintf(stream, "                  \"Sha256\": \"%s\",\n", region.hash_sha256_string.c_str());
			fprintf(stream, "                  \"Md5\": \"%s\"\n", region.hash_md5_string.c_str());
			fprintf(stream, "                },\n");
			fprintf(stream, "                \"Recipes\": [\n");
			fprintf(stream, "                  {\n");
			fprintf(stream, "                    \"Name\": \"AllZero\",\n");
			fprintf(stream, "                    \"Parameters\": {\n");
			fprintf(stream, "                      \"Length\": {\n");
			fprintf(stream, "                      \"Type\": \"Number\",\n");
			fprintf(stream, "                      \"Number\": %llu\n", region.length);
			fprintf(stream, "                      }\n");
			fprintf(stream, "                    }\n");
			fprintf(stream, "                  }\n");
			fprintf(stream, "                ]\n");
			fprintf(stream, "              }\n");
			fprintf(stream, "            }");
		}
		else
		{
			fprintf(stream, "              \"Type\": \"ArchiveItem\",\n");
			fprintf(stream, "              \"Item\": {\n");
			fprintf(stream, "                \"Id\": %d,\n", chunk_id);
			fprintf(stream, "                \"Name\": \"%s\",\n", chunk_name.c_str());
			fprintf(stream, "                \"Type\": \"Chunk\",\n");
			fprintf(stream, "                \"Offset\": %llu,\n", region.offset);
			fprintf(stream, "                \"Length\": %llu,\n", region.length);
			fprintf(stream, "                \"Hashes\": {\n");
			fprintf(stream, "                  \"Sha256\": \"%s\",\n", region.hash_sha256_string.c_str());
			fprintf(stream, "                  \"Md5\": \"%s\"\n", region.hash_md5_string.c_str());
			fprintf(stream, "                }\n");
			fprintf(stream, "              }\n");
			fprintf(stream, "            }");
		}

		if ((i + 1) < details.regions.size())
		{
			fprintf(stream, ",");
		}
		fprintf(stream, "\n");
	}
	fprintf(stream, "          }\n");
	fprintf(stream, "        }\n");
	fprintf(stream, "      ]\n");
	fprintf(stream, "    }");
}

void dump_all_files(FILE *stream, const std::vector<file_details> &all_files)
{
	unique_id_provider id_provider;

	fprintf(stream, "{\n");
	fprintf(stream, "  \"Type\": \"ext4\",\n");
	fprintf(stream, "  \"Subtype\": \"ext4\",\n");
	fprintf(stream, "  \"Chunks\": [\n");
	for (int i = 0; i < all_files.size(); i++)
	{
		const auto &details = all_files[i];

		if (details.regions.size() == 0)
		{
			continue;
		}
		bool last = i == (all_files.size() - 1);
		dump_file_chunks(id_provider, stream, details, last);

		fprintf(stream, "\n");
	}
	fprintf(stream, "  ],\n");
	fprintf(stream, "  \"Payload\": [\n");
	for (int i = 0; i < all_files.size(); i++)
	{
		const auto &details = all_files[i];

		dump_file_payload(id_provider, stream, details);
		if ((i + 1) < all_files.size())
		{
			fprintf(stream, ",");
		}
		fprintf(stream, "\n");
	}

	fprintf(stream, "  ]\n");
	fprintf(stream, "}\n");
}
