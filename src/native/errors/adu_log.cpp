/**+
 * @file adu_log.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "adu_log.h"

#ifndef DISABLE_ADU_LOGGING

#ifdef WIN32
#include <stdlib.h>
#endif

adu_log adu_log::m_singleton;

adu_log::adu_log()
{
	#ifdef WIN32
	char *value = nullptr;
	size_t value_size = 0;
	auto err = _dupenv_s(&value, &value_size, ADU_ENABLE_LOGGING);
	if (err != 0)
	{
		m_logging_enabled = false;
		return;
	}
	#else
	auto value = std::getenv(ADU_ENABLE_LOGGING);
	#endif

	if (value != nullptr)
	{
		printf("Found a value for %s: %s.\n", ADU_ENABLE_LOGGING, value);
		m_logging_enabled = !((value_size == 2) && (value[0] == '0'));
	}

	#ifdef WIN32
	if (value)
	{
		free(value);
	}
	#endif
}

#endif