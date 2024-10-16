add_library(hashing STATIC
	algorithm.cpp
	hash.cpp
	hasher.cpp
	hexstring_convert.cpp
	)

if (WIN32 OR MINGW)
	set(USE_BCRYPT 1)
else()
	set(USE_OPENSSL 1)
endif()

if (USE_BCRYPT)
	target_link_libraries(hashing PUBLIC bcrypt.lib)
	target_compile_definitions(hashing PUBLIC USE_BCRYPT)
elseif (USE_OPENSSL)
	find_package(OpenSSL REQUIRED)
	target_link_libraries(hashing PRIVATE OpenSSL::SSL)
	target_link_libraries(hashing PRIVATE OpenSSL::Crypto)

	target_compile_definitions(hashing PUBLIC USE_OPENSSL)
elseif (USE_LIBGCRYPT)
	find_library(LIBGPG_ERROR_LIBRARY gpg-error)

	find_path(LIBGCRYPT_INCLUDE_DIR gcrypt.h)
	find_library(LIBGCRYPT_LIBRARY gcrypt)

	target_include_directories(hashing PUBLIC ${LIBGCRYPT_INCLUDE_DIR})
	target_link_libraries(hashing PUBLIC ${LIBGCRYPT_LIBRARY})
	target_link_libraries(hashing PUBLIC ${LIBGPG_ERROR_LIBRARY})

	target_compile_definitions(hashing PUBLIC USE_LIBGCRYPT)
else()
	message(FATAL_ERROR "No hashing library selected")
endif()

target_link_libraries(hashing PUBLIC errors io io_sequential)

target_include_directories(hashing PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	)