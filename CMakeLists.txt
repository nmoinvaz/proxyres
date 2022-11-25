cmake_minimum_required(VERSION 3.12)

option(PROXYRES_CURL "Build with cURL to download PAC scripts." OFF)
option(PROXYRES_EXECUTE "Build with support for PAC script execution." ON)

option(PROXYRES_BUILD_CLI "Builds command line utility." ON)
option(PROXYRES_BUILD_TESTS "Builds Googletest unit tests project." ON)

option(PROXYRES_CODE_COVERAGE "Build for code coverage." OFF)

if(UNIX AND NOT APPLE AND NOT PROXYRES_EXECUTE)
    message(STATUS "PAC script execution enabled on linux")
    set(PROXYRES_EXECUTE ON)
endif()

project(proxyres)

set(PROXYRES_SRCS
    config.h
    config_i.h
    config.c
    event.h
    log.h
    mutex.h
    proxyres.h
    proxyres.c
    resolver.h
    resolver_i.h
    resolver.c
    threadpool.h
    util.h
    util.c)
if(PROXYRES_EXECUTE)
    list(APPEND PROXYRES_SRCS
        dns.h
        dns.c
        execute.h
        execute_i.h
        execute.c
        fetch.h
        mozilla_js.h
        net_adapter.h
        net_adapter.c
        resolver_posix.h
        resolver_posix.c
        wpad_dhcp.h
        wpad_dhcp.c
        wpad_dhcp_posix.h
        wpad_dhcp_posix.c
        wpad_dns.h
        wpad_dns.c)
endif()
if(WIN32)
    list(APPEND PROXYRES_SRCS
        config_win.h
        config_win.c
        event_win.c
        mutex_win.c
        util_win.h
        util_win.c)
    if(PROXYRES_EXECUTE)
        list(APPEND PROXYRES_SRCS
            net_adapter_winxp.c
            wpad_dhcp_win.h
            wpad_dhcp_win.c)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
        list(APPEND PROXYRES_SRCS
            resolver_winrt.h
            resolver_winrt.c)
    else()
        list(APPEND PROXYRES_SRCS
            resolver_win8.h
            resolver_win8.c
            resolver_winxp.h
            resolver_winxp.c)
        if(PROXYRES_EXECUTE)
            list(APPEND PROXYRES_SRCS
                execute_wsh.h
                execute_wsh.c)
        endif()
    endif()

    if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION GREATER_EQUAL 6.1)
        list(APPEND PROXYRES_SRCS
            threadpool_winvista.c)
    else()
        list(APPEND PROXYRES_SRCS
            threadpool_winxp.c)
    endif()
elseif(APPLE)
    list(APPEND PROXYRES_SRCS
        config_mac.h
        config_mac.c
        event_pthread.c
        mutex_pthread.c
        resolver_mac.h
        resolver_mac.c
        threadpool_pthread.c)
    if(PROXYRES_EXECUTE)
        list(APPEND PROXYRES_SRCS
            execute_jscore.h
            execute_jscore.c
            net_adapter_mac.c
            wpad_dhcp_mac.h
            wpad_dhcp_mac.c)
    endif()
elseif(UNIX)
    list(APPEND PROXYRES_SRCS
        config_env.h
        config_env.c
        config_gnome2.h
        config_gnome2.c
        config_gnome3.h
        config_gnome3.c
        config_kde.h
        config_kde.c
        event_pthread.c
        execute_jscore.h
        execute_jscore.c
        mutex_pthread.c
        net_adapter_linux.c
        resolver_gnome3.h
        resolver_gnome3.c
        threadpool_pthread.c
        util_linux.h
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

add_compile_definitions($<IF:$<CONFIG:Debug>,_DEBUG,_NDEBUG>)

if(PROXYRES_CODE_COVERAGE AND NOT MSVC)
    include(CheckCCompilerFlag)

    # Check for -coverage flag support for Clang/GCC
    if(CMAKE_VERSION VERSION_LESS 3.14)
        set(CMAKE_REQUIRED_LIBRARIES -lgcov)
    else()
        set(CMAKE_REQUIRED_LINK_OPTIONS -coverage)
    endif()
    check_c_compiler_flag(-coverage HAVE_COVERAGE)
    set(CMAKE_REQUIRED_LIBRARIES)
    set(CMAKE_REQUIRED_LINK_OPTIONS)

    if(HAVE_COVERAGE)
        add_compile_options(-coverage)
        add_link_options(-coverage)

        if(CMAKE_C_COMPILER_ID MATCHES "GNU")
            link_libraries(gcov)
        endif()

        # Disable optimization for code coverage. Prepend the optimization flags to
        # override any default flags set by CMake for the configuration type on Windows.
        set(CMAKE_C_FLAGS "-O0 ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-O0 ${CMAKE_CXX_FLAGS}")

        message(STATUS "Code coverage enabled using: -coverage")
    else()
        message(WARNING "Code coverage not supported")
    endif()
endif()

add_library(proxyres ${PROXYRES_SRCS})
set_property(TARGET proxyres PROPERTY C_STANDARD 99)
target_include_directories(proxyres PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(PROXYRES_EXECUTE)
    target_compile_definitions(proxyres PUBLIC PROXYRES_EXECUTE=1)

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
            set(CURL_DISABLE_GETOPTIONS ON CACHE BOOL "Disables curl_easy_options API" FORCE)
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

    if(TARGET CURL::libcurl)
        target_compile_definitions(proxyres PUBLIC HAVE_CURL=1)
        target_sources(proxyres PRIVATE fetch_curl.c)
        target_link_libraries(proxyres CURL::libcurl)
    else()
        target_sources(proxyres PRIVATE fetch_posix.c)
    endif()
endif()

if(WIN32)
    target_link_libraries(proxyres dhcpcsvc.lib iphlpapi.lib winhttp ws2_32)
elseif(APPLE)
    find_library(CFNETWORK_LIBRARY CFNetwork)
    target_link_libraries(proxyres ${CFNETWORK_LIBRARY})

    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(proxyres ${COREFOUNDATION_LIBRARY})

    find_library(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration)
    target_link_libraries(proxyres ${SYSTEMCONFIGURATION_LIBRARY})

    find_package(Threads REQUIRED)
    target_compile_definitions(proxyres PRIVATE HAVE_PTHREADS)
    target_link_libraries(proxyres ${CMAKE_THREAD_LIBS_INIT})

    set_target_properties(proxyres PROPERTIES LINK_FLAGS -Wl,-F/Library/Frameworks)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(proxyres PkgConfig::deps)

    pkg_check_modules(GConf REQUIRED gconf-2.0)
    # Don't link libraries at compile time since we dynamically load them at runtime
    target_include_directories(proxyres PRIVATE ${GConf_INCLUDE_DIRS})

    pkg_search_module(JSCoreGTK REQUIRED javascriptcoregtk-4.0 javascriptcoregtk-3.0 javascriptcoregtk-1.0)
    # Don't link libraries at compile time since we dynamically load them at runtime
    target_include_directories(proxyres PRIVATE ${JSCoreGTK_INCLUDE_DIRS})

    find_package(Threads REQUIRED)
    target_compile_definitions(proxyres PRIVATE HAVE_PTHREADS)
    target_link_libraries(proxyres ${CMAKE_THREAD_LIBS_INIT})

    target_link_libraries(proxyres ${CMAKE_DL_LIBS})
endif()

add_subdirectory(test)
