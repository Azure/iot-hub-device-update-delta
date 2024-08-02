add_library(diffs_serialization_standard STATIC 
	builtin_recipe_types.cpp
	deserializer.cpp
	serializer.cpp
	)
	
target_link_libraries(diffs_serialization_standard PUBLIC 
	diffs_core
	diffs_recipes_basic
	diffs_recipes_compressed
	)

target_include_directories(diffs_serialization_standard PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	)
