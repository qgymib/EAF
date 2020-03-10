cmake_minimum_required(VERSION 3.2)
project(eaf)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(EAF_SYSTEM linux)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(EAF_SYSTEM win32)
else ()
	set(EAF_SYSTEM linux)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/compat/${EAF_SYSTEM})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/utils SRC_FILES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/compat/${EAF_SYSTEM} SRC_FILES)

add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_FILES})

# 编译测试用例
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
	add_subdirectory(test)
endif()
