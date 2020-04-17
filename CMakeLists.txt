cmake_minimum_required(VERSION 3.2)
project(eaf C ASM)

# 探测目标系统
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(EAF_SYSTEM linux)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(EAF_SYSTEM win32)
else ()
	set(EAF_SYSTEM linux)
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")
endif ()

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/compat/${EAF_SYSTEM})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_EAF_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/core SRC_EAF_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/infra SRC_EAF_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/utils SRC_EAF_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/compat/${EAF_SYSTEM} SRC_EAF_C)
add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_EAF_C})

add_subdirectory(powerpack)

# 编译测试用例
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
	add_subdirectory(test)
endif()
