/**
 * @file recipe_parameter.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "recipe_parameter.h"
#include "recipe.h"
#include "archive_item.h"
#include "diff.h"

#include "user_exception.h"

diffs::recipe_parameter::recipe_parameter(std::uint64_t number) :
	m_type(recipe_parameter_type::number), m_number_value(number)
{}

diffs::recipe_parameter::~recipe_parameter() = default;

void diffs::recipe_parameter::read(diff_reader_context &context)
{
	context.read(&m_type);

	switch (m_type)
	{
	case recipe_parameter_type::archive_item:
		m_archive_item_value = std::make_unique<archive_item>();
		m_archive_item_value->read(context, false);
		break;
	case recipe_parameter_type::number:
		context.read(&m_number_value);
		break;
	default:
		std::string msg = "diffs::recipe_parameter::read(): Invalid type: " + std::to_string(static_cast<int>(m_type));
		throw error_utility::user_exception(error_utility::error_code::diff_recipe_parameter_read_invalid_type, msg);
	}
}

void diffs::recipe_parameter::write(diff_writer_context &context)
{
	context.write(m_type);

	switch (m_type)
	{
	case recipe_parameter_type::archive_item:
		m_archive_item_value->write(context, false);
		break;
	case recipe_parameter_type::number:
		context.write(m_number_value);
		break;
	default:
		std::string msg = "diffs::recipe_parameter::write(): Invalid recipe parameter type: "
		                + std::to_string(static_cast<int>(m_type));
		throw error_utility::user_exception(error_utility::error_code::diff_invalid_recipe_parameter_type, msg);
	}
}

// void diffs::recipe_parameter::dump(diff_viewer_context &context, std::ostream &ostream, const std::string &indent)
// const
//{
//	ostream << indent << "Parameter. Type: " << get_recipe_parameter_type_string(m_type);
//
//	std::string indentNext = indent;
//	indentNext += "  ";
//
//	switch (m_type)
//	{
//	case diffs::recipe_parameter_type::number:
//		ostream << " Number: " << m_number_value << std::endl;
//		break;
//	case diffs::recipe_parameter_type::archive_item:
//		ostream << " ";
//		m_archive_item_value->dump(context, ostream, indentNext);
//		break;
//	}
//}

uint64_t diffs::recipe_parameter::get_inline_asset_byte_count() const
{
	switch (m_type)
	{
	case diffs::recipe_parameter_type::archive_item:
		return m_archive_item_value->get_inline_asset_byte_count();
	case diffs::recipe_parameter_type::number:
		return 0;
	}
	std::string msg = "diffs::get_inline_asset_byte_count(): Invalid recipe parameter type: "
	                + std::to_string(static_cast<int>(m_type));
	throw error_utility::user_exception(error_utility::error_code::diff_invalid_recipe_parameter_type, msg);
}

void diffs::recipe_parameter::apply(apply_context &context) const
{
	if (m_type == diffs::recipe_parameter_type::archive_item)
	{
		m_archive_item_value->apply(context);
		return;
	}

	std::string msg = "diffs::recipe_parameter::apply(): Invalid type: " + std::to_string(static_cast<int>(m_type));
	throw error_utility::user_exception(error_utility::error_code::diff_recipe_parameter_invalid_type_for_apply, msg);
}

uint64_t diffs::recipe_parameter::get_number_value() const
{
	if (m_type != diffs::recipe_parameter_type::number)
	{
		std::string msg = "diffs::recipe_parameter::apply(): Invalid type: " + std::to_string(static_cast<int>(m_type));
		throw error_utility::user_exception(
			error_utility::error_code::diff_recipe_parameter_invalid_type_for_get_number_value, msg);
	}

	return m_number_value;
}

diffs::recipe_parameter::recipe_parameter(archive_item &&item)
{
	m_type               = recipe_parameter_type::archive_item;
	m_archive_item_value = std::make_unique<archive_item>(std::move(item));
}
