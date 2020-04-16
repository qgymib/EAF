cmake_minimum_required(VERSION 3.2)
project(eafpowerpack)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_POWERPACK_C)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/powerpack SRC_POWERPACK_C)

set(SRC_POWERPACK_C ${SRC_POWERPACK_C} CACHE INTERNAL "POWERPACK source code")
add_library(${PROJECT_NAME} STATIC ${SRC_POWERPACK_C})
