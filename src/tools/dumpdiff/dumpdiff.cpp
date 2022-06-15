/**
 * @file dumpdiff.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <string>
#include <iostream>

#include "user_exception.h"
#include "dump_diff.h"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cout << "Usage: DumpDiff <diff path>" << std::endl;
		return 1;
	}

	try
	{
		diffs::dump_diff(argv[1], std::cout);
	}
	catch (error_utility::user_exception &e)
	{
		std::cout << "Failed with an exception: " << e.get_message() << std::endl;
		return (int)e.get_error();
	}
	catch (std::exception &e)
	{
		std::cout << "Failed with an exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
