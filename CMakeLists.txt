cmake_minimum_required(VERSION 3.12)

project(proxyres)

set(PROXYRES_HDRS
    config.h
    config_i.h
    proxyres.h
    resolver.h
    resolver_i.h)
set(PROXYRES_SRCS
    config.c
    proxyres.c
    resolver.c)
if(WIN32)
    list(APPEND PROXYRES_HDRS
        config_win.h
        resolver_win8.h
        resolver_winxp.h
        utils_win.h)
    list(APPEND PROXYRES_SRCS
        config_win.c
        resolver_win8.c
        resolver_winxp.c
        utils_win.c)
elseif(APPLE)
    list(APPEND PROXYRES_HDRS
        config_mac.h
        resolver_mac.h)
    list(APPEND PROXYRES_SRCS
        config_mac.c
        resolver_mac.c)
elseif(UNIX)
    list(APPEND PROXYRES_HDRS
        config_gnome2.h
        config_gnome3.h
        resolver_gnome3.h)
    list(APPEND PROXYRES_SRCS
        config_gnome2.c
        config_gnome3.c
        resolver_gnome3.c)
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
endif()

add_library(proxyres ${PROXYRES_HDRS} ${PROXYRES_SRCS})
set_property(TARGET proxyres PROPERTY C_STANDARD 99)
target_include_directories(proxyres PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(proxyres PRIVATE winhttp)
elseif(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(proxyres PRIVATE ${COREFOUNDATION_LIBRARY})
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(proxyres PkgConfig::deps)

    pkg_check_modules(GConf REQUIRED gconf-2.0)
    # Don't set libraries since we are dynamically linking against them
    unset(GConf_LDFLAGS CACHE)
    unset(GConf_LIBRARIES CACHE)
    target_include_directories(proxyres PRIVATE ${GConf_INCLUDE_DIRS})
endif()

add_subdirectory(test)