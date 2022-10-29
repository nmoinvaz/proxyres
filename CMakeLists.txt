cmake_minimum_required(VERSION 3.12)

project(proxyres)

set(PROXYRES_HDRS
    config.h
    config_i.h
    execute.h
    proxyres.h
    mozilla_js.h
    resolver.h
    resolver_i.h
    util.h)
set(PROXYRES_SRCS
    config.c
    execute.c
    proxyres.c
    resolver.c
    util.c)
if(WIN32)
    list(APPEND PROXYRES_HDRS
        config_win.h
        execute_jsproxy.h
        resolver_win8.h
        resolver_winxp.h
        util_win.h)
    list(APPEND PROXYRES_SRCS
        config_win.c
        execute_jsproxy.c
        resolver_win8.c
        resolver_winxp.c
        util_win.c)
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
        execute_jscoregtk.h
        resolver_gnome3.h)
    list(APPEND PROXYRES_SRCS
        config_gnome2.c
        config_gnome3.c
        execute_jscoregtk.c
        resolver_gnome3.c)
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1 _CRT_NONSTDC_NO_WARNINGS=1)
endif()

add_library(proxyres ${PROXYRES_HDRS} ${PROXYRES_SRCS})
set_property(TARGET proxyres PROPERTY C_STANDARD 99)
target_include_directories(proxyres PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(proxyres PRIVATE winhttp ws2_32)
elseif(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(proxyres PRIVATE ${COREFOUNDATION_LIBRARY})

    find_library(JAVASCRIPTCORE_LIBRARY JavaScriptCore)
    target_link_libraries(proxyres PRIVATE ${JAVASCRIPTCORE_LIBRARY})

    set_target_properties(proxyres PROPERTIES LINK_FLAGS -Wl,-F/Library/Frameworks)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(proxyres PkgConfig::deps)

    pkg_check_modules(GConf REQUIRED gconf-2.0)
    # Don't set libraries since we are dynamically linking against them
    unset(GConf_LDFLAGS CACHE)
    unset(GConf_LIBRARIES CACHE)
    target_include_directories(proxyres PRIVATE ${GConf_INCLUDE_DIRS})

    pkg_search_module(JSCoreGTK REQUIRED javascriptcoregtk-4.0 javascriptcoregtk-3.0 javascriptcoregtk-1.0)
    # Don't set libraries since we are dynamically linking against them
    unset(JSCoreGTK_LDFLAGS CACHE)
    unset(JSCoreGTK_LIBRARIES CACHE)
    target_include_directories(proxyres PRIVATE ${JSCoreGTK_INCLUDE_DIRS})

    #find_package(Threads REQUIRED)
    #target_compile_definitions(proxyres PRIVATE HAVE_PTHREADS)
    #target_link_libraries(proxyres ${CMAKE_THREAD_LIBS_INIT})

    target_link_libraries(proxyres dl)
endif()

add_subdirectory(test)