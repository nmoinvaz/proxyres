add_executable(proxycli proxycli.c)
target_link_libraries(proxycli PRIVATE proxyres)

add_test(NAME proxycli-config COMMAND proxycli config)
add_test(NAME proxycli-resolve-google COMMAND proxycli resolve https://google.com/)