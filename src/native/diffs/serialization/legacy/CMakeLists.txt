add_library(diffs_serialization_legacy STATIC 
	deserializer.cpp
	legacy_recipe_type.cpp
	serializer.cpp
	)
	
target_link_libraries(diffs_serialization_legacy PUBLIC 
	diffs_core
	diffs_recipes_basic
	diffs_recipes_compressed
	)

target_include_directories(diffs_serialization_legacy PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	)
