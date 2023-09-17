cmake_minimum_required (VERSION 3.20)

# include(CheckPIESupported)

project (basic CXX C)

add_subdirectory(wasmVM)

set(CMAKE_CXX_STANDARD 20)

add_library(WASMloader SHARED plugin.cpp native_impl.cpp)
target_link_libraries(WASMloader PUBLIC wasmVM)

add_executable (tst "test/test.cpp" native_impl.cpp ../../counter.cpp)
target_link_libraries(tst PUBLIC wasmVM)
