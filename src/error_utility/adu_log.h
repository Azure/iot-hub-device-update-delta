/**
 * @file adu_log.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <stdio.h>

#define DISABLE_ADU_LOGGING

#ifndef DISABLE_ADU_LOGGING

class adu_log
{
	public:
	static int get_log_index()
	{
		static int index = 0;
		return index++;
	}
};

	#define ADU_LOG(_FORMAT, ...) printf("%d: " _FORMAT "\n", adu_log::get_log_index(), __VA_ARGS__);

#else

	#define ADU_LOG(...)

#endif