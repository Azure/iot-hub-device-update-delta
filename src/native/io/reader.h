/**
 * @file reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <optional>
#include <memory>
#include <set>

#include "io_device_view.h"
#include "nul_device_reader.h"

namespace archive_diff::io
{
class reader
{
	public:
	reader()
	{
		std::shared_ptr<io_device> device = std::make_shared<nul_io_device>();
		m_impl                            = std::make_shared<simple_reader_impl>(io_device_view{device});
	}

	reader(const io_device_view &view) : m_impl(std::make_shared<simple_reader_impl>(view)) {}

	reader slice(uint64_t offset, uint64_t length) const { return m_impl->slice(offset, length); }

	reader slice_at(uint64_t offset) const
	{
		auto length = m_impl->size() - offset;
		return m_impl->slice(offset, length);
	}

	reader chain(reader &other) const
	{
		std::vector<reader> readers;
		readers.push_back(*this);
		readers.push_back(other);

		std::shared_ptr<reader_impl> impl = std::make_shared<chained_reader_impl>(readers);
		return reader{impl};
	}

	static reader chain(std::vector<reader>& readers)
	{
		std::shared_ptr<reader_impl> impl = std::make_shared<chained_reader_impl>(readers);
		return reader{impl};
	}

	std::vector<reader> unchain() const { return m_impl->unchain(); }

	std::optional<io_device_view> get_io_device_view() const { return m_impl->get_io_device_view(); }

	// bool operator<(const reader &other) const;

	uint64_t size() const { return m_impl->size(); }

	size_t read_some(uint64_t offset, std::span<char> buffer) const { return m_impl->read_some(offset, buffer); }

	template <typename T>
	void read(uint64_t offset, T *value)
	{
		read(offset, std::span{reinterpret_cast<char *>(value), sizeof(T)});
	}

	void read(uint64_t offset, std::span<char> buffer) const { return m_impl->read(offset, buffer); }
	void read_all(std::vector<char> &buffer) const;

	protected:
	class reader_impl
	{
		public:
		virtual reader slice(uint64_t offset, uint64_t length) const = 0;
		virtual std::vector<reader> unchain() const                  = 0;

		virtual size_t read_some(uint64_t offset, std::span<char> buffer) const = 0;
		void read(uint64_t offset, std::span<char> buffer) const
		{
			size_t actual = read_some(offset, buffer);
			if (actual != buffer.size())
			{
				std::string msg = "Wanted to read " + std::to_string(buffer.size()) + " bytes, but could only read "
				                + std::to_string(actual) + " bytes.";
				throw errors::user_exception(errors::error_code::io_reader_read_failure, msg);
			}
		}
		virtual uint64_t size() const = 0;

		virtual std::optional<io_device_view> get_io_device_view() const = 0;
	};

	class simple_reader_impl : public reader_impl
	{
		public:
		simple_reader_impl(const io_device_view &view) : m_view(view) {}

		virtual reader slice(uint64_t offset, uint64_t length) const override;
		virtual std::vector<reader> unchain() const override;

		virtual size_t read_some(uint64_t offset, std::span<char> buffer) const override
		{
			return m_view.read_some(offset, buffer);
		}

		virtual uint64_t size() const override { return m_view.size(); }

		virtual std::optional<io_device_view> get_io_device_view() const override { return m_view; }

		private:
		io_device_view m_view;
	};

	class chained_reader_impl : public reader_impl
	{
		public:
		chained_reader_impl() = default;

		chained_reader_impl(std::vector<reader> &readers)
		{
			for (auto &reader : readers)
			{
				auto unchained_readers = reader.unchain();
				for (auto &unchained : unchained_readers)
				{
					m_offsets.push_back(m_length);
					m_readers.push_back(unchained);
					m_length += unchained.size();
				}
			}
		}

		chained_reader_impl(const chained_reader_impl &other) :
			m_length(other.m_length), m_offsets(other.m_offsets), m_readers(other.m_readers)
		{}

		chained_reader_impl(const chained_reader_impl &chained, const reader_impl *other);

		virtual reader slice(uint64_t offset, uint64_t length) const override;
		virtual std::vector<reader> unchain() const override { return m_readers; }

		virtual size_t read_some(uint64_t offset, std::span<char> buffer) const override;

		virtual uint64_t size() const override { return m_length; }

		virtual std::optional<io_device_view> get_io_device_view() const override { return std::nullopt; }

		private:
		uint64_t m_length{};

		std::vector<uint64_t> m_offsets;
		std::vector<reader> m_readers;
	};

	reader(std::shared_ptr<reader_impl> &impl) : m_impl(impl) {}

	std::shared_ptr<reader_impl> m_impl;
};
	using shared_reader = std::shared_ptr<reader>;
	using unique_reader = std::unique_ptr<reader>;
} // namespace archive_diff::io
