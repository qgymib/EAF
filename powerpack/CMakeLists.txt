cmake_minimum_required(VERSION 3.2)
project(eafpowerpack)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/powerpack SRC_C)

add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_C})
