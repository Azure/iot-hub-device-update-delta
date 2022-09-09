/**
 * @file apply_session.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "apply_session.h"

#include "user_exception.h"
#include "binary_file_reader.h"

#include "diff.h"

int diffs::api::apply_session::apply(const char *source_path, const char *diff_path, const char *target_path)
{
	errors.clear();

	try
	{
		io_utility::binary_file_reader diff_reader(diff_path);
		diffs::diff diff(&diff_reader);

		diffs::blob_cache blob_cache;

		apply_context::root_context_parameters params;
		params.m_diff           = &diff;
		params.m_diff_reader    = &diff_reader;
		params.m_source_file    = source_path;
		params.m_target_file    = target_path;
		params.m_blob_cache     = &blob_cache;

		diffs::apply_context context = diffs::apply_context::root_context(params);

		auto reader = context.get_source_reader();

		if (reader != nullptr) 
		{
			diff.verify_source(*reader);
		}

		diff.apply(context);
	}
	catch (std::exception &e)
	{
		std::string error_text = "Failed to apply diff. Exception: ";
		error_text += +e.what();

		errors.push_back(error{error_utility::error_code::standard_library_exception, error_text});

		return static_cast<int>(errors.size());
	}
	catch (error_utility::user_exception &e)
	{
		errors.push_back(error{e.get_error(), e.get_message()});
	}

	return static_cast<int>(errors.size());
}
