add_library(diffs STATIC 
    archive.cpp
	all_zero_recipe.cpp
	apply_context.cpp
	archive_item.cpp
	blob_cache.cpp
	bsdiff_recipe.cpp
	concatenation_recipe.cpp
	copy_recipe.cpp
	copy_source_recipe.cpp
	diff.cpp
	diff_reader_context.cpp
	diff_resources_context.cpp
	diff_viewer_context.cpp
	diff_writer_context.cpp
	dump_diff.cpp 
    gz_decompression_recipe.cpp 
	hash.cpp 
	inline_asset_recipe.cpp 
	nested_diff_recipe.cpp 
	recipe.cpp 
    recipe_host.cpp
	recipe_parameter.cpp 
	region_recipe.cpp 
	remainder_chunk_recipe.cpp 
	source_context.cpp 
	target_context.cpp 
	zstd_compression_recipe.cpp 
	zstd_decompression_recipe.cpp 
	zstd_delta_recipe.cpp
)

target_link_libraries(diffs PUBLIC error_utility hash_utility io_utility)

target_include_directories(diffs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
