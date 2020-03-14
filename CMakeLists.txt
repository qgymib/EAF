cmake_minimum_required(VERSION 3.2)
project(eaf C ASM)

# 探测目标系统
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
include_directories(${PROJECT_SOURCE_DIR}/src/arch/${CMAKE_SYSTEM_PROCESSOR})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/utils SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/filber SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/compat/${EAF_SYSTEM} SRC_C)
file(GLOB SRC_S ${PROJECT_SOURCE_DIR}/src/arch/${CMAKE_SYSTEM_PROCESSOR}/*.S)

add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_C} ${SRC_S})

# 编译测试用例
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
	add_subdirectory(test)
endif()
