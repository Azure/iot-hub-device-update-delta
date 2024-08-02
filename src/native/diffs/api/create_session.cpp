/**
 * @file create_session.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "create_session.h"

#include "aduapicall.h"

#include <io/file/binary_file_writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <diffs/serialization/standard/serializer.h>
#include <diffs/serialization/standard/builtin_recipe_types.h>

#include "apply_session.h"

#include "aduapi_type_conversion.h"

namespace archive_diff::diffs::api
{
uint32_t create_session::set_target(const diffc_item_definition &item)
{
	API_CALL_PROLOG();

	auto target_item = diffc_item_definition_to_core_item_definition(item);
	m_deserializer.set_target_item(target_item);
	API_CALL_EPILOG();
}

uint32_t create_session::set_source(const diffc_item_definition &item)
{
	API_CALL_PROLOG();
	auto source_item = diffc_item_definition_to_core_item_definition(item);
	m_deserializer.set_source_item(source_item);
	API_CALL_EPILOG();
}

uint32_t create_session::add_recipe(
	const char *recipe_name,
	const diffc_item_definition *result_diffc_item,
	uint64_t *number_ingredients,
	size_t number_ingredients_count,
	const diffc_item_definition **item_ingredients,
	size_t item_ingredient_count)
{
	API_CALL_PROLOG();

	auto result_item = diffc_item_definition_to_core_item_definition(*result_diffc_item);
	std::vector<uint64_t> numbers;

	for (size_t i = 0; i < number_ingredients_count; i++)
	{
		numbers.push_back(number_ingredients[i]);
	}

	std::vector<core::item_definition> items;
	for (size_t i = 0; i < item_ingredient_count; i++)
	{
		auto item = diffc_item_definition_to_core_item_definition(*(item_ingredients[i]));
		items.push_back(item);
	}

	m_deserializer.add_recipe(recipe_name, result_item, numbers, items);

	API_CALL_EPILOG();
}

uint32_t diffs::api::create_session::set_inline_assets(const char *path)
{
	API_CALL_PROLOG();
	m_deserializer.set_inline_assets(path);
	API_CALL_EPILOG();
}

uint32_t diffs::api::create_session::set_remainder(const char *path)
{
	API_CALL_PROLOG();
	m_deserializer.set_remainder(path);
	API_CALL_EPILOG();
}

uint32_t diffs::api::create_session::add_nested_archive(const char *path)
{
	API_CALL_PROLOG();

	m_deserializer.add_nested_archive(path);

	API_CALL_EPILOG();
}

uint32_t create_session::add_payload(const char *payload_name, const diffc_item_definition &item)
{
	API_CALL_PROLOG();

	auto payload_item = diffc_item_definition_to_core_item_definition(item);
	m_deserializer.add_payload(payload_name, payload_item);

	API_CALL_EPILOG();
}

uint32_t create_session::write_diff(const char *path)
{
	API_CALL_PROLOG();

	std::lock_guard<std::mutex> lock_guard(m_archive_mutex);

	std::shared_ptr<io::writer> writer = std::make_shared<io::file::binary_file_writer>(path);
	io::sequential::basic_writer_wrapper seq(writer);

	auto archive = m_deserializer.get_archive();

	diffs::serialization::standard::serializer serializer(archive);
	serializer.write(seq);

	API_CALL_EPILOG();
}

apply_session *create_session::new_apply_session() const
{
	auto session = std::make_unique<apply_session>();
	auto archive = m_deserializer.get_archive();
	session->add_archive(archive);
	return session.release();
}
} // namespace archive_diff::diffs::api