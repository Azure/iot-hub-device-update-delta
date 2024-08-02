add_library(diffs_recipes_compressed STATIC
	bspatch_decompression_recipe.cpp
	zlib_compression_recipe.cpp
	zlib_decompression_recipe.cpp
	zstd_compression_recipe.cpp
	zstd_decompression_recipe.cpp
	)

target_link_libraries(diffs_recipes_compressed PUBLIC io_compressed diffs_core)

target_include_directories(diffs_recipes_compressed PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	)
