add_library(diffs_recipes_basic STATIC
	all_zeros_recipe.cpp
	chain_recipe.cpp
	slice_recipe.cpp
	)

target_link_libraries(diffs_recipes_basic PUBLIC io diffs_core)

target_include_directories(diffs_recipes_basic PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	)
