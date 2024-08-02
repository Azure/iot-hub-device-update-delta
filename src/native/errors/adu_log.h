/**
 * @file adu_log.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#if !defined(DEBUG) && !defined(_DEBUG)
	#define DISABLE_ADU_LOGGING
#endif

#ifndef DISABLE_ADU_LOGGING

#include <fmt/core.h> 
#include <iostream>

class adu_log
{
	public:
	static int get_log_index()
	{
		static int index = 0;
		return index++;
	}

	static bool is_logging_enabled()
	{
		return m_singleton.m_logging_enabled;
	}

	private:
	const char *ADU_ENABLE_LOGGING = "ADU_ENABLE_LOGGING";

	adu_log();

	static adu_log m_singleton;

	bool m_logging_enabled{};
};

#define ADU_LOG(_FORMAT, ...) \
	if (adu_log::is_logging_enabled()) \
	{ \
		std::cout << fmt::format(_FORMAT, __VA_ARGS__) << std::endl;\
	} \

#else

	#define ADU_LOG(...)

#endif