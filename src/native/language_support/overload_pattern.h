/**
 * @file overload_pattern.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <variant>

template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;