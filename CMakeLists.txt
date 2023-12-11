cmake_minimum_required(VERSION 3.13)

project(mpv-plugin)

include(FindPkgConfig)
pkg_search_module(MPV REQUIRED mpv>=0.37.0)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(menu SHARED src/menu.c)
target_include_directories(menu PRIVATE ${MPV_INCLUDE_DIRS})
target_compile_definitions(menu PRIVATE MPV_CPLUGIN_DYNAMIC_SYM)
