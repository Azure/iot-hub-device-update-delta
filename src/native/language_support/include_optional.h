/**
 * @file include_optional.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#if __has_include(<optional>)
	#include <optional>


#else
adfasdf
	#include <experimental/optional>
namespace std
{
template <class T>
using optional = std::experimental::optional<T>;
}
#endif