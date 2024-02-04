cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GetGitVersion)
get_git_version(GIT_VERSION SEM_VER)

project(mpv-menu-plugin LANGUAGES C VERSION "${SEM_VER}")
set(CMAKE_C_STANDARD 11)

include(FindPkgConfig)
pkg_search_module(MPV REQUIRED mpv>=0.37.0)

configure_file(${PROJECT_SOURCE_DIR}/menu.rc.in ${PROJECT_BINARY_DIR}/menu.rc @ONLY)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(menu SHARED
    src/mpv/misc/bstr.c
    src/mpv/misc/dispatch.c
    src/mpv/misc/node.c
    src/mpv/ta/ta.c
    src/mpv/ta/ta_talloc.c
    src/mpv/ta/ta_utils.c

    src/dialog.c
    src/menu.c
    src/plugin.c

    ${PROJECT_BINARY_DIR}/menu.rc
)
set_property(TARGET menu PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(menu PRIVATE src/mpv ${MPV_INCLUDE_DIRS})
target_link_libraries(menu PRIVATE shlwapi)
target_compile_definitions(menu PRIVATE MPV_CPLUGIN_DYNAMIC_SYM)

install(TARGETS menu RUNTIME DESTINATION .)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME menu)
set(CPACK_INSTALLED_DIRECTORIES ${CMAKE_SOURCE_DIR}/src/lua .)
include(CPack)
