add_library(hash_utility STATIC hash_utility.cpp)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(hash_utility PRIVATE Microsoft.GSL::GSL)

if (WIN32 OR MINGW)
target_link_libraries(hash_utility PRIVATE bcrypt.lib)

elseif(UNIX)
find_library(LIBGPG_ERROR_LIBRARY gpg-error)

find_path(LIBGCRYPT_INCLUDE_DIR gcrypt.h)
find_library(LIBGCRYPT_LIBRARY gcrypt)

target_include_directories(hash_utility PUBLIC ${LIBGCRYPT_INCLUDE_DIR})
target_link_libraries(hash_utility PUBLIC ${LIBGCRYPT_LIBRARY})
target_link_libraries(hash_utility PUBLIC ${LIBGPG_ERROR_LIBRARY})

endif()

target_link_libraries(hash_utility PUBLIC error_utility)

target_include_directories(hash_utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})