add_library(hashing STATIC 
	algorithm.cpp
	hash.cpp
	hasher.cpp 
	hexstring_convert.cpp
	)

if (WIN32 OR MINGW)
target_link_libraries(hashing PUBLIC bcrypt.lib)

elseif(UNIX)
find_library(LIBGPG_ERROR_LIBRARY gpg-error)

find_path(LIBGCRYPT_INCLUDE_DIR gcrypt.h)
find_library(LIBGCRYPT_LIBRARY gcrypt)

target_include_directories(hashing PUBLIC ${LIBGCRYPT_INCLUDE_DIR})
target_link_libraries(hashing PUBLIC ${LIBGCRYPT_LIBRARY})
target_link_libraries(hashing PUBLIC ${LIBGPG_ERROR_LIBRARY})

endif()

target_link_libraries(hashing PUBLIC errors io io_sequential)

target_include_directories(hashing PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	)