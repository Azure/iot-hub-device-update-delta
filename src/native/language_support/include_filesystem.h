/**
 * @file include_filesystem.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#if __has_include(<filesystem>)
	#include <filesystem>
namespace fs = std::filesystem;
#else
	#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif