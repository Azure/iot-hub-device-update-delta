add_library(diffs_core STATIC 
	archive.cpp
	cookbook.cpp
	item_definition.cpp
	item_definition_helpers.cpp
	kitchen.cpp
	pantry.cpp
	prepared_item.cpp
	recipe.cpp
	slicer.cpp
	)
	
target_link_libraries(diffs_core PUBLIC 
	io
	io_file
	hashing
	)

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(diffs_core PRIVATE JsonCpp::JsonCpp)

target_include_directories(diffs_core PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	)
