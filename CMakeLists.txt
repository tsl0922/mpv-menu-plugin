cmake_minimum_required(VERSION 3.13)

project(mpv-plugin)

include(FindPkgConfig)
pkg_search_module(MPV REQUIRED mpv>=0.37.0)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(menu SHARED
    src/misc/bstr.c
    src/misc/dispatch.c
    src/ta/ta.c
    src/ta/ta_talloc.c
    src/ta/ta_utils.c
    src/menu.c
    src/plugin.c
)

target_include_directories(menu PRIVATE src ${MPV_INCLUDE_DIRS})
target_link_libraries(menu PRIVATE shlwapi)
target_compile_definitions(menu PRIVATE MPV_CPLUGIN_DYNAMIC_SYM)
