/**
 * @file diff_writer_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "diff_writer_context.h"

void diffs::diff_writer_context::write_raw(std::string_view buffer) { m_diff_sequential_writer.write(buffer); }

void diffs::diff_writer_context::write_inline_assets() { m_diff_sequential_writer.write(m_inline_asset_reader); }

void diffs::diff_writer_context::write_remainder() { m_diff_sequential_writer.write(m_remainder_reader); }
