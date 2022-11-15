cmake_minimum_required(VERSION 3.12)

option(PROXYRES_CURL "Enable building against CURL" OFF)
option(PROXYRES_JSPROXY "Enable building with deprecated Windows JSProxy support" OFF)

option(PROXYRES_TEST_WPAD "Enable WPAD unit tests" OFF)

project(proxyres)

set(PROXYRES_HDRS
    config.h
    config_i.h
    dns.h
    execute.h
    execute_i.h
    event.h
    fetch.h
    log.h
    mozilla_js.h
    mutex.h
    net_adapter.h
    proxyres.h
    resolver.h
    resolver_i.h
    resolver_posix.h
    threadpool.h
    util.h
    util_socket.h
    wpad_dhcp.h
    wpad_dhcp_posix.h
    wpad_dns.h)
set(PROXYRES_SRCS
    config.c
    dns.c
    execute.c
    proxyres.c
    net_adapter.c
    resolver.c
    resolver_posix.c
    util.c
    wpad_dhcp.c
    wpad_dhcp_posix.c
    wpad_dns.c)
if(WIN32)
    list(APPEND PROXYRES_HDRS
        config_win.h
        execute_wsh.h
        resolver_win8.h
        resolver_winxp.h
        util_win.h
        wpad_dhcp_win.h)
    list(APPEND PROXYRES_SRCS
        config_win.c
        event_win.c
        execute_wsh.c
        net_adapter_winxp.c
        mutex_win.c
        resolver_win8.c
        resolver_winxp.c
        util_win.c
        wpad_dhcp_win.c)
    if(PROXYRES_JSPROXY)
        list(APPEND PROXYRES_HDRS
            execute_jsproxy.h)
        list(APPEND PROXYRES_SRCS
            execute_jsproxy.c)
        add_compile_definitions(HAVE_JSPROXY)
    endif()
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
        resolver_mac.h
        wpad_dhcp_mac.h)
    list(APPEND PROXYRES_SRCS
        config_mac.c
        event_pthread.c
        execute_jscore.c
        net_adapter_mac.c
        mutex_pthread.c
        resolver_mac.c
        threadpool_pthread.c
        wpad_dhcp_mac.c)
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
        event_pthread.c
        execute_jscore.c
        mutex_pthread.c
        net_adapter_linux.c
        resolver_gnome3.c
        threadpool_pthread.c
        util_linux.c)
endif()

if(WIN32)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS=1
        _CRT_NONSTDC_NO_WARNINGS=1
        _WINSOCK_DEPRECATED_NO_WARNINGS=1
        COBJMACROS)
elseif(APPLE)
elseif(UNIX)
    add_compile_definitions(_GNU_SOURCE=1)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
else()
    add_compile_definitions(_NDEBUG)
endif()

add_library(proxyres ${PROXYRES_HDRS} ${PROXYRES_SRCS})
set_property(TARGET proxyres PROPERTY C_STANDARD 99)
target_include_directories(proxyres PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(PROXYRES_CURL)
    if(NOT TARGET CURL::libcurl)
        include(FetchContent)

        # Disable compression and SSL support to avoid pulling in zlib and OpenSSL
        set(CURL_ZLIB OFF CACHE BOOL "Build curl with ZLIB support" FORCE)
        set(CURL_USE_LIBSSH2 OFF CACHE BOOL "Use libSSH2" FORCE)

        # Disable tests
        set(BUILD_CURL_EXE OFF CACHE BOOL "Build cURL executable" FORCE)
        set(CURL_DISABLE_TESTS ON CACHE BOOL "Disable cURL tests" FORCE)
        set(CURL_ENABLE_EXPORT_TARGET OFF CACHE BOOL "to enable cmake export target" FORCE)

        # Disable unnecessary protocols
        set(CURL_DISABLE_HSTS ON CACHE BOOL "to disable HSTS support" FORCE)
        set(CURL_DISABLE_FTP ON CACHE BOOL "Disable FTP" FORCE)
        set(CURL_DISABLE_TELNET ON CACHE BOOL "Disable TELNET" FORCE)
        set(CURL_DISABLE_LDAP ON CACHE BOOL "Disable LDAP" FORCE)
        set(CURL_DISABLE_LDAPS ON CACHE BOOL "Disable LDAPS" FORCE)
        set(CURL_DISABLE_DICT ON CACHE BOOL "Disable DICT" FORCE)
        set(CURL_DISABLE_TFTP ON CACHE BOOL "Disable TFTP" FORCE)
        set(CURL_DISABLE_GOPHER ON CACHE BOOL "Disable GOPHER" FORCE)
        set(CURL_DISABLE_POP3 ON CACHE BOOL "Disable POP3" FORCE)
        set(CURL_DISABLE_IMAP ON CACHE BOOL "Disable IMAP" FORCE)
        set(CURL_DISABLE_SMB ON CACHE BOOL "Disable SMB" FORCE)
        set(CURL_DISABLE_SMTP ON CACHE BOOL "Disable SMTP" FORCE)
        set(CURL_DISABLE_RTSP ON CACHE BOOL "Disable RTSP" FORCE)
        set(CURL_DISABLE_MQTT ON CACHE BOOL "Disable MQTT" FORCE)
        set(CURL_DISABLE_ALTSVC ON CACHE BOOL "Disable alt-svc support" FORCE)
        set(CURL_DISABLE_GETOPTIONS ON CACHE BOOL "Disables curl_easy_options API for existing options to curl_easy_setopt" FORCE)
        set(CURL_DISABLE_MIME ON CACHE BOOL "Disables MIME support" FORCE)
        set(CURL_DISABLE_NETRC ON CACHE BOOL "Disables netrc parser" FORCE)
        set(CURL_DISABLE_PROGRESS_METER ON CACHE BOOL "Disables built-in progress meter" FORCE)

        # Allow specifying alternative curl repository
        if(NOT DEFINED CURL_REPOSITORY)
            set(CURL_REPOSITORY https://github.com/curl/curl.git)
        endif()

        # Fetch curl source code from official repository
        FetchContent_Declare(curl
            GIT_REPOSITORY ${CURL_REPOSITORY})

        FetchContent_GetProperties(curl)
        if(NOT curl_POPULATED)
            FetchContent_Populate(curl)
            add_subdirectory(${curl_SOURCE_DIR} ${curl_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()

        add_library(CURL::libcurl ALIAS libcurl)
    endif()
endif()

if(PROXYRES_CURL AND TARGET CURL::libcurl)
    target_compile_definitions(proxyres PUBLIC HAVE_LIBCURL=1)
    target_sources(proxyres PRIVATE fetch_curl.c)
    set(LIBS CURL::libcurl)
    target_link_libraries(proxyres CURL::libcurl)
else()
    set(LIBS)
    target_sources(proxyres PRIVATE fetch_posix.c)
endif()

if(WIN32)
    target_link_libraries(proxyres dhcpcsvc.lib iphlpapi.lib winhttp ws2_32)
elseif(APPLE)
    find_library(CFNETWORK_LIBRARY CFNetwork)
    target_link_libraries(proxyres PRIVATE ${CFNETWORK_LIBRARY})

    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(proxyres PRIVATE ${COREFOUNDATION_LIBRARY})

    find_library(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration)
    target_link_libraries(proxyres PRIVATE ${SYSTEMCONFIGURATION_LIBRARY})

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