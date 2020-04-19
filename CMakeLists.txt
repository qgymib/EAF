cmake_minimum_required(VERSION 3.2)
project(eaf)

# add custom cmake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(ENABLE_COVERAGE "Enable code coverage. GCC/Clang only" OFF)

# check system
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(EAF_SYSTEM win32)
else ()
	set(EAF_SYSTEM linux)
endif ()

# libeaf
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_EAF_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/core SRC_EAF_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/infra SRC_EAF_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/utils SRC_EAF_C)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/compat/${EAF_SYSTEM} SRC_EAF_C)
add_library(${PROJECT_NAME} ${SRC_EAF_C})

# define target include search paths
target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/src/compat/${EAF_SYSTEM}
)

# add warning check
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif ()

# add library dependencies
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		Threads::Threads
)

if (ENABLE_COVERAGE)
	include(CodeCoverage)
	coverage_setup_target_compiler(NAME ${PROJECT_NAME})
endif ()

# build powerpack
add_subdirectory(powerpack)

# build test
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
	add_subdirectory(test)
endif()
