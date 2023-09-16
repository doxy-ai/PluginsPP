cmake_minimum_required (VERSION 3.20)

# include(CheckPIESupported)

project (basic CXX C)

add_subdirectory(wasmVM)

set(CMAKE_CXX_STANDARD 20)

#add_executable (tst "test.cpp" native_impl.cpp)
add_executable (tst "test.cpp" native_impl.cpp ../../counter.cpp)
target_link_libraries(tst PUBLIC wasmVM)
