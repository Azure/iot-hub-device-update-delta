/**
 * @file applydiff.c
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>
#include <inttypes.h>

#include "adudiffapply.h"

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
	size_t error_count      = 0;
	adu_apply_handle handle = adu_diff_apply_create_session();

	if (!adu_diff_apply(handle, source, diff, target))
	{
		printf("Successfully expanded diff to %s\n", target);
	}
	else
	{
		size_t i;
		const char *error_text;
		int error_code;
		printf("Failed to expand diff. source: %s, diff: %s, target: %s\n", source, diff, target);

		error_count = adu_diff_apply_get_error_count(handle);

		printf("Found %" PRIuPTR " errors associated with this failure.\n", error_count);
		for (i = 0; i < error_count; i++)
		{
			error_text = adu_diff_apply_get_error_text(handle, i);
			error_code = adu_diff_apply_get_error_code(handle, i);
			printf("%" PRIuPTR ") %s (%d)", i, error_text, error_code);
		}
	}

	adu_diff_apply_close_session(handle);

	return (int)error_count;
}
