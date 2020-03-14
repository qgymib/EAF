cmake_minimum_required(VERSION 3.2)
project(eaf_test C ASM)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/cases SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/cases/service SRC_C)

find_package(Threads)
add_executable(${PROJECT_NAME} ${SRC_C} ${SRC_S})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(TEST eaf_test)