#!/bin/bash

# skip coverity_scan branch
if [ "$TRAVIS_BRANCH" == "coverity_scan" ]; then
	exit 0
fi

if [ "$CC" = "gcc" ]; then
	(rm -rf build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=1 .. && make coverage)
	exit 0
fi

if [ "$CC" = "clang" ]; then
	(rm -rf build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && make CTEST_OUTPUT_ON_FAILURE=1 test)
	exit 0
fi
