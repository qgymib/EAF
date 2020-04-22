@echo off

pushd %cd%
cd %~dp0\..

mkdir build
cd build

@echo on
cmake -DPC_LIBUV_INCLUDEDIR=..\third_party\libuv\include -DPC_LIBUV_LIBDIR=..\third_party\libuv\lib\win32_msvc1800_vs2013 ..
@echo off

cd ..

@echo on
cmake --build build
@echo off

popd
