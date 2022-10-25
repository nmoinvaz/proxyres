cmake_minimum_required(VERSION 3.12)

project(proxyres)

set(PROXYRES_HDRS
    proxyres.h
    proxyres_i.h)
set(PROXYRES_SRCS
    proxyres.c)
if(WIN32)
    list(APPEND PROXYRES_HDRS
        utils_win.h
        proxyres_win8.c
        proxyres_winxp.h)
    list(APPEND PROXYRES_SRCS
        proxyres_win8.c
        proxyres_winxp.c
        utils_win.c)
elseif(APPLE)
elseif(UNIX)
    list(APPEND PROXYRES_HDRS
        proxyres_gnome3.h)
    list(APPEND PROXYRES_SRCS
        proxyres_gnome3.c)
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
endif()

add_library(proxyres ${PROXYRES_HDRS} ${PROXYRES_SRCS})
set_property(TARGET proxyres PROPERTY C_STANDARD 99)
