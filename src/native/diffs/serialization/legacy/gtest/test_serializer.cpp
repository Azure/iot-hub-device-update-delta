/**
 * @file test_serializer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include "main.h"

#include <diffs/serialization/legacy/deserializer.h>
#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>
//
// void test_open_and_write(fs::path in_path, fs::path out_path)
//{
//	archive_diff::diffs::serialization::legacy::deserializer deserializer;
//
//	auto reader = archive_diff::io::file::io_device::make_reader(in_path.string());
//
//	deserializer.read(reader);
//
//	std::shared_ptr<archive_diff::io::writer> writer =
//		std::make_shared<archive_diff::io::file::binary_file_writer>(out_path.string());
//
//	deserializer.write(writer);
//
//	printf("wrote to: %s\n", out_path.string().c_str());
//}
//
// TEST(serializer, read_and_write_nested_nested)
//{
//	auto diff_path_in  = g_test_data_root / "nested_nested.diff";
//	auto diff_path_out = fs::temp_directory_path() / "nested_nested.diff.out";
//
//	test_open_and_write(diff_path_in, diff_path_out);
//}
//
// TEST(serializer, read_and_write_nested)
//{
//	auto diff_path_in  = g_test_data_root / "nested.diff";
//	auto diff_path_out = fs::temp_directory_path() / "nested.diff.out";
//
//	test_open_and_write(diff_path_in, diff_path_out);
//}
//
// TEST(serializer, read_and_write)
//{
//	auto diff_path_in  = g_test_data_root / "main.diff";
//	auto diff_path_out = fs::temp_directory_path() / "main.diff.out";
//
//	test_open_and_write(diff_path_in, diff_path_out);
//}
