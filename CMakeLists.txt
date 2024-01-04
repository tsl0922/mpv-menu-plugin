cmake_minimum_required(VERSION 3.13)

project(mpv-menu-plugin C)
set(CMAKE_C_STANDARD 11)

include(FindPkgConfig)
pkg_search_module(MPV REQUIRED mpv>=0.37.0)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(menu SHARED
    mpv/misc/bstr.c
    mpv/misc/dispatch.c
    mpv/misc/node.c
    mpv/ta/ta.c
    mpv/ta/ta_talloc.c
    mpv/ta/ta_utils.c

    src/menu.c
    src/plugin.c
)
set_property(TARGET menu PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(menu PRIVATE mpv ${MPV_INCLUDE_DIRS})
target_link_libraries(menu PRIVATE shlwapi)
target_compile_definitions(menu PRIVATE MPV_CPLUGIN_DYNAMIC_SYM)

install(TARGETS menu RUNTIME DESTINATION .)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME menu)
set(CPACK_INSTALLED_DIRECTORIES ${CMAKE_SOURCE_DIR}/lua .)
include(CPack)
