add_executable(proxycli proxycli.c)

target_link_libraries(proxycli PRIVATE proxyres)

enable_testing()

add_test(NAME proxycli-config
    COMMAND proxycli config)
add_test(NAME proxycli-resolve-google
    COMMAND proxycli resolve https://google.com/)
add_test(NAME proxycli-execute-google
    COMMAND proxycli execute ${CMAKE_SOURCE_DIR}/test/pac/example.js https://google.com/)