add_executable(proxycli proxycli.c)

target_link_libraries(proxycli PRIVATE proxyres)

add_test(NAME proxycli-config
    COMMAND proxycli config)
add_test(NAME proxycli-resolve-google
    COMMAND proxycli resolve
        https://example1.com/ https://google.com/)
add_test(NAME proxycli-execute-google
    COMMAND proxycli execute ${CMAKE_SOURCE_DIR}/test/pac/example.js
        https://example1.com/ https://google.com/)

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

add_executable(gtest_proxyres ${TEST_SRCS})
target_link_libraries(gtest_proxyres PRIVATE proxyres GTest::GTest)
