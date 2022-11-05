#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "util_win.h"
}

struct get_proxy_by_protocol_param {
    const char *proxy_list;
    const char *protocol;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const get_proxy_by_protocol_param &param) {
        return os << "proxy_list: " << param.proxy_list << "\n"
                  << "key: " << param.protocol;
    }
};

constexpr get_proxy_by_protocol_param get_proxy_by_protocol_tests[] = {
    {"1proxy.com", "http", "1proxy.com"},
    {"2proxy.com", "https", "2proxy.com"},
    {"http=3proxy.com", "http", "3proxy.com"},
    {"http=4proxy.com:80", "http", "4proxy.com:80"},
    {"https=5proxy.com", "https", "5proxy.com"},
    {"https=6proxy.com:443", "https", "6proxy.com:443"},
    {"http=7proxy.com;https=8proxy.com", "http", "7proxy.com"},
    {"https=9proxy.com;http=10proxy.com", "http", "10proxy.com"},
    {"http=11proxy.com;https=12proxy.com:443", "https", "12proxy.com:443"},
    {"https=13proxy.com;http=14proxy.com:80;", "https", "13proxy.com"},
};

class util_by_protocol : public ::testing::TestWithParam<get_proxy_by_protocol_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_by_protocol, testing::ValuesIn(get_proxy_by_protocol_tests));

TEST_P(util_by_protocol, get_proxy) {
    const auto &param = GetParam();
    char *proxy = get_proxy_by_protocol(param.protocol, param.proxy_list);
    EXPECT_NE(proxy, nullptr);
    if (proxy) {
        EXPECT_STREQ(proxy, param.expected);
        free(proxy);
    }
}
struct convert_proxy_list_to_uri_list_param {
    const char *proxy_list;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const convert_proxy_list_to_uri_list_param &param) {
        return os << "proxy_list: " << param.proxy_list;
    }
};

constexpr convert_proxy_list_to_uri_list_param convert_proxy_list_to_uri_list_tests[] = {
    {"http=127.0.0.1;myproxy1.com", "http://127.0.0.1,http://myproxy1.com"},
    {"https=mysecureproxy1.com;http=myproxy2.com", "https://mysecureproxy1.com,http://myproxy2.com"},
    {"myproxy3.com", "http://myproxy3.com"},
    {"myproxy4.com:8080", "http://myproxy4.com:8080"},
    {"socks socksprox.com;http httpprox.com", "socks://socksproxy.com,http://httpprox.com"}
};

class util_to_uri_list : public ::testing::TestWithParam<convert_proxy_list_to_uri_list_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_to_uri_list, testing::ValuesIn(convert_proxy_list_to_uri_list_tests));

TEST_P(util_to_uri_list, convert_proxy_list) {
    const auto &param = GetParam();
    char *uri_list = convert_proxy_list_to_uri_list(param.proxy_list);
    EXPECT_NE(uri_list, nullptr);
    if (uri_list) {
        EXPECT_STREQ(uri_list, param.expected);
        free(uri_list);
    }
}
