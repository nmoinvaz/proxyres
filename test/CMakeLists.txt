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

add_executable(tpoolcli tpoolcli.c)

target_link_libraries(tpoolcli PRIVATE proxyres)