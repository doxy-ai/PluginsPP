cmake_minimum_required (VERSION 3.21)
project (Plugins++ CXX C)

set(CMAKE_CXX_STANDARD 20)

add_library(pluginsplusplus STATIC counter.cpp dynamic_plugin_handle.hpp plugin_handle.hpp plugin_manager.hpp plugin/plugin_base.hpp)
target_include_directories(pluginsplusplus PUBLIC ${CMAKE_SOURCE_DIR})

if(PROJECT_IS_TOP_LEVEL)
    add_executable (tst "test/main.cpp")
    target_link_libraries(tst PUBLIC pluginsplusplus)

    add_library(DebugPlugin SHARED "test/DebugPlugin.cpp")
    target_link_libraries(DebugPlugin PUBLIC pluginsplusplus)
endif()
