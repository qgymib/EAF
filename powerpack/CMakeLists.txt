set(POWERPACK_BUILD_NAME powerpack)

# libpowerpack
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_POWERPACK_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/powerpack SRC_POWERPACK_C)
add_library(${POWERPACK_BUILD_NAME} ${SRC_POWERPACK_C})

# require libuv
find_package(LibUV REQUIRED)

# define target include search paths
target_include_directories(${POWERPACK_BUILD_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${LIBUV_INCLUDE_DIRS}
)

# add library dependencies
target_link_libraries(${POWERPACK_BUILD_NAME}
    PUBLIC
        eaf
    PRIVATE
        ${LIBUV_LIBRARIES}
)

# add warning check
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${POWERPACK_BUILD_NAME} PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${POWERPACK_BUILD_NAME} PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${POWERPACK_BUILD_NAME} PRIVATE /W4 /WX)
endif ()

if (ENABLE_COVERAGE)
    include(CodeCoverage)
    coverage_setup_target_compiler(NAME ${POWERPACK_BUILD_NAME})
endif ()

