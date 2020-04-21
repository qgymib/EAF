set(TEST_BUILD_NAME etest)

# test
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_TEST_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/eaf SRC_TEST_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/powerpack SRC_TEST_C)
add_executable(${TEST_BUILD_NAME} ${SRC_TEST_C})

# add include search pathes
target_include_directories(${TEST_BUILD_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

# add warning check
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${TEST_BUILD_NAME} PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TEST_BUILD_NAME} PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TEST_BUILD_NAME} PRIVATE /W4 /WX)
endif ()

# add dependencies
target_link_libraries(${TEST_BUILD_NAME}
	PRIVATE
		eaf
		powerpack
)

# linux need -lrt
if (NOT WIN32)
	target_link_libraries(${TEST_BUILD_NAME}
		PRIVATE
			rt
	)
endif ()

# add cmake test
add_test(NAME ${TEST_BUILD_NAME}
	COMMAND ${TEST_BUILD_NAME})

if (ENABLE_COVERAGE)
	setup_target_for_coverage_lcov(
		NAME coverage
		EXECUTABLE ${TEST_BUILD_NAME}
	)
endif ()

