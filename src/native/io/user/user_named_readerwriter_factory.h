/**
 * @file user_named_readerwriter_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <io/user/user_readerwriter.h>
#include <io/user/named_readerwriter_factory.h>

namespace archive_diff::io::user
{
class user_named_readerwriter_factory : public named_readerwriter_factory
{
	public:
	user_named_readerwriter_factory(void *handle, user_readerwriter_create_pfn create_readerwriter) :
		m_handle(handle), m_create_readerwriter(create_readerwriter)
	{}

	virtual std::unique_ptr<io::readerwriter> create(const char *name) override
	{
		auto readerwriter_handle = m_create_readerwriter(m_handle, name);

		return std::unique_ptr<io::user::user_readerwriter>{
			reinterpret_cast<io::user::user_readerwriter *>(readerwriter_handle)};
	}

	virtual std::shared_ptr<io::readerwriter> create_shared(const char *name) override
	{
		auto readerwriter_handle = m_create_readerwriter(m_handle, name);

		return std::shared_ptr<io::user::user_readerwriter>{
			reinterpret_cast<io::user::user_readerwriter *>(readerwriter_handle)};
	}

	void *m_handle{};
	user_readerwriter_create_pfn m_create_readerwriter{};
};

} // namespace archive_diff::io::user