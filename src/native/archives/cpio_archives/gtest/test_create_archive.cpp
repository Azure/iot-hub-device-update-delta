/**
 * @file test_io_device.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <memory>
#include <vector>

#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/file/binary_file_writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <archives/cpio_archives/cpio_archive.h>

TEST(test_create_archive, create)
{
	archive_diff::cpio_archive cpio(archive_diff::cpio_format::newc_ascii);

	std::string filenames[]    = {"a", "b", "c", "d"};
	std::string filecontents[] = {"a", "abc", "abcdefghijklmnopqrstuvwxyz", "12345"};

	uint32_t inode = 0;

	for (size_t i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++)
	{
		const auto &filename = filenames[i];
		const auto &contents = filecontents[i];
		auto len             = contents.size();

		auto buffer = std::make_shared<std::vector<char>>();
		buffer->resize(len);
		memcpy(buffer->data(), contents.data(), len);

		auto reader = archive_diff::io::buffer::io_device::make_reader(
			buffer, archive_diff::io::buffer::io_device::size_kind::vector_size);

		cpio.add_file(filename, inode++, reader);
	}

	auto output_buffer = std::make_shared<std::vector<char>>();
	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(output_buffer);
	archive_diff::io::sequential::basic_writer_wrapper wrapped_buffer_writer(buffer_writer);

	cpio.write(wrapped_buffer_writer);

	auto buffer_reader = archive_diff::io::buffer::io_device::make_reader(
		output_buffer, archive_diff::io::buffer::io_device::size_kind::vector_size);

	archive_diff::cpio_archive cpio_from_reader;
	ASSERT_TRUE(cpio_from_reader.try_read(buffer_reader));
}