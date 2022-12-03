if(PROXYRES_BUILD_CLI)
    set(PROXYCLI_SRCS proxycli.c)

    if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
        list(APPEND PROXYCLI_SRCS
            uwp/Square44x44Logo.png
            uwp/Square150x150Logo.png
            uwp/StoreLogo.png
            uwp/Package.appxmanifest
            uwp/Windows_TemporaryKey.pfx)
    endif()

    add_executable(proxycli ${PROXYCLI_SRCS})
    target_link_libraries(proxycli PRIVATE proxyres)

    if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
        return()
    endif()

    add_test(NAME proxycli-help
        COMMAND proxycli help)
    add_test(NAME proxycli-config
        COMMAND proxycli config)
    add_test(NAME proxycli-resolve-google
        COMMAND proxycli resolve https://simple.com/ https://google.com/)
    if(PROXYRES_EXECUTE OR (UNIX AND NOT APPLE))
        add_test(NAME proxycli-execute-google
            COMMAND proxycli execute ${CMAKE_SOURCE_DIR}/test/pac.js https://simple.com/ https://google.com/)
    endif()
endif()

if(PROXYRES_BUILD_TESTS)
    if(NOT TARGET GTest::GTest)
        find_package(GTest QUIET)
    endif()

    if(NOT TARGET GTest::GTest)
        include(FetchContent)

        # Prevent overriding the parent project's compiler/linker settings for Windows
        set(gtest_force_shared_crt ON CACHE BOOL
            "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)

        # Allow specifying alternative Google test repository
        if(NOT DEFINED GTEST_REPOSITORY)
            set(GTEST_REPOSITORY https://github.com/google/googletest.git)
        endif()
        if(NOT DEFINED GTEST_TAG)
            # Use older version of Google test to support older versions of GCC
            if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 5.3)
                set(GTEST_TAG release-1.10.0)
            else()
                set(GTEST_TAG release-1.11.0)
            endif()
        endif()

        # Fetch Google test source code from official repository
        FetchContent_Declare(googletest
            GIT_REPOSITORY ${GTEST_REPOSITORY}
            GIT_TAG ${GTEST_TAG})

        FetchContent_GetProperties(googletest)
        if(NOT googletest_POPULATED)
            FetchContent_Populate(googletest)
            add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()

        add_library(GTest::GTest ALIAS gtest)
        add_library(GTest::Main ALIAS gtest_main)
    endif()

    set(TEST_SRCS
        test_config.cc
        test_main.cc
        test_threadpool.cc
        test_util.cc)
    if(WIN32)
        list(APPEND TEST_SRCS
            test_util_win.cc)
    elseif(UNIX AND NOT APPLE)
        list(APPEND TEST_SRCS
            test_util_linux.cc)
    endif()
    if(PROXYRES_EXECUTE)
        list(APPEND TEST_SRCS
            test_dns.cc
            test_execute.cc
            test_fetch.cc
            test_net_adapter.cc
            test_wpad.cc)
    endif()

    add_executable(gtest_proxyres ${TEST_SRCS})
    target_link_libraries(gtest_proxyres PRIVATE proxyres GTest::GTest)
    target_include_directories(gtest_proxyres PRIVATE ${CMAKE_SOURCE_DIR})

    add_test(NAME gtest_proxyres COMMAND gtest_proxyres)
endif()
