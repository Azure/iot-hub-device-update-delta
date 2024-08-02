/**
 * @file main.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include "main.h"

fs::path g_test_data_root;

int main(int argc, char **argv)
{
	if (argc > 2 && strcmp(argv[1], "--test_data_root") == 0)
	{
		g_test_data_root = argv[2];
	}
	else
	{
		printf("Must specify test data root with --test_data_root <path>\n");
		return 1;
	}

	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}