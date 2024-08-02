/**
 * @file applydiff.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string>

#include <diffs/api/legacy_adudiffapply.h>

int apply(const char *source, const char *diff, const char *target);

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("Usage: applydiff <source path> <diff path> <target path>\n");
		return 1;
	}

	return apply(argv[1], argv[2], argv[3]);
}

int apply(const char *source, const char *diff, const char *target)
{
	printf("Applying diff: %s\n", diff);
	printf("Using source : %s\n", source);
	printf("To file      : %s\n", target);

	auto handle      = adu_diff_apply_create_session();
	auto error_count = adu_diff_apply(handle, source, diff, target);

	int ret = 0;

	if (error_count)
	{
		printf("Encountered errors while trying to apply diff:\n");
		for (int i = 0; i < error_count; i++)
		{
			auto error_code = adu_diff_apply_get_error_code(handle, i);
			auto error_text = adu_diff_apply_get_error_text(handle, i);

			printf("\t%d) Code: %d, Text: %s\n", i, (int)error_code, error_text);
		}

		ret = error_count;
	}
	else
	{
		printf("Finished successfully.\n");
	}

	adu_diff_apply_close_session(handle);

	return ret;
}
