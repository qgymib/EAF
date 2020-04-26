#!/bin/bash

# run test and generate coverage
(rm -rf build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=1 .. && make coverage) || exit 1

# doxygen
(doxygen Doxyfile) || exit 1
