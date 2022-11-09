cmake_minimum_required(VERSION 3.12)

option(PROXYRES_TEST_WPAD "Enable WPAD unit tests" OFF)

project(proxyres)

set(PROXYRES_HDRS
    config.h
    config_i.h
    execute.h
    fetch.h
    log.h
    mozilla_js.h
    proxyres.h
    resolver.h
    resolver_i.h
    threadpool.h
    util.h
    wpad_dhcp.h)
set(PROXYRES_SRCS
    config.c
    execute.c
    fetch_posix.c
    proxyres.c
    resolver.c
    util.c
    wpad_dhcp.c)
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
    if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION GREATER_EQUAL 6.1)
        list(APPEND PROXYRES_SRCS
            threadpool_winvista.c)
    else()
        list(APPEND PROXYRES_SRCS
            threadpool_winxp.c)
    endif()
elseif(APPLE)
    list(APPEND PROXYRES_HDRS
        config_mac.h
        execute_jscore.h
        resolver_mac.h)
    list(APPEND PROXYRES_SRCS
        config_mac.c
        execute_jscore.c
        resolver_mac.c
        threadpool_pthread.c)
elseif(UNIX)
    list(APPEND PROXYRES_HDRS
        config_env.h
        config_gnome2.h
        config_gnome3.h
        config_kde.h
        execute_jscore.h
        resolver_gnome3.h
        util_linux.h)
    list(APPEND PROXYRES_SRCS
        config_env.c
        config_gnome2.c
        config_gnome3.c
        config_kde.c
        execute_jscore.c
        resolver_gnome3.c
        threadpool_pthread.c
        util_linux.c)
endif()

if(WIN32)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS=1
        _CRT_NONSTDC_NO_WARNINGS=1
        _WINSOCK_DEPRECATED_NO_WARNINGS=1)
elseif(APPLE)
elseif(UNIX)
    add_compile_definitions(_GNU_SOURCE=1)
endif()

add_library(proxyres ${PROXYRES_HDRS} ${PROXYRES_SRCS})
set_property(TARGET proxyres PROPERTY C_STANDARD 99)
target_include_directories(proxyres PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(proxyres PRIVATE winhttp ws2_32)
elseif(APPLE)
    find_library(CFNETWORK_LIBRARY CFNetwork)
    target_link_libraries(proxyres PRIVATE ${CFNETWORK_LIBRARY})

    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(proxyres PRIVATE ${COREFOUNDATION_LIBRARY})

    #find_library(JAVASCRIPTCORE_LIBRARY JavaScriptCore)
    # Don't set libraries since we are dynamically linking against them
    #target_link_libraries(proxyres PRIVATE ${JAVASCRIPTCORE_LIBRARY})

    find_package(Threads REQUIRED)
    target_compile_definitions(proxyres PRIVATE HAVE_PTHREADS)
    target_link_libraries(proxyres PRIVATE ${CMAKE_THREAD_LIBS_INIT})

    set_target_properties(proxyres PROPERTIES LINK_FLAGS -Wl,-F/Library/Frameworks)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(proxyres PRIVATE PkgConfig::deps)

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

    find_package(Threads REQUIRED)
    target_compile_definitions(proxyres PRIVATE HAVE_PTHREADS)
    target_link_libraries(proxyres PRIVATE ${CMAKE_THREAD_LIBS_INIT})

    target_link_libraries(proxyres PRIVATE dl)
endif()

enable_testing()

add_subdirectory(test)