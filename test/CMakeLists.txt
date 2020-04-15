cmake_minimum_required(VERSION 3.2)
project(eaf_test C ASM)

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
endif ()

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/eaf SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/powerpack SRC_C)

find_package(Threads)
add_executable(${PROJECT_NAME} ${SRC_C} ${SRC_S})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME} rt)

add_test(TEST eaf_test)
