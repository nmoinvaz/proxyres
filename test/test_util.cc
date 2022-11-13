#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "util.h"
}

struct get_url_host_param {
    const char *string;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const get_url_host_param &param) {
        return os << "string: " << param.string;
    }
};

constexpr get_url_host_param get_url_host_tests[] = {
    {"https://google.com/", "google.com"},
    {"https://google.com", "google.com"},
    {"google.com", "google.com"},
    {"google.com/", "google.com"},
    {"https://u:p@google.com/", "google.com"},
    {"https://u:p@www.google.com", "www.google.com"},
};

class util_url_host : public ::testing::TestWithParam<get_url_host_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_url_host, testing::ValuesIn(get_url_host_tests));

TEST_P(util_url_host, parse) {
    const auto &param = GetParam();
    char *host = get_url_host(param.string);
    EXPECT_NE(host, nullptr);
    if (host) {
        EXPECT_STREQ(host, param.expected);
        free(host);
    }
}

struct convert_proxy_list_to_uri_list_param {
    const char *proxy_list;
    const char *fallback_scheme;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const convert_proxy_list_to_uri_list_param &param) {
        return os << "proxy list: " << param.proxy_list;
    }
};

constexpr convert_proxy_list_to_uri_list_param convert_proxy_list_to_uri_list_tests[] = {
    {"DIRECT", NULL, "direct://"},
    {"PROXY 127.0.0.1:80", NULL, "http://127.0.0.1:80"},
    {"PROXY 127.0.0.1:8080", NULL, "http://127.0.0.1:8080"},
    {"PROXY 127.0.0.1:443", NULL, "https://127.0.0.1:443"},
    {"PROXY myproxy0.com:90", "http", "http://myproxy0.com:90"},
    {"PROXY myproxy1.com:90", "https", "https://myproxy1.com:90"},
    {"HTTP myproxy1.com:80;SOCKS myproxy2.com:1080", NULL, "http://myproxy1.com:80,socks://myproxy2.com:1080"},
    {"HTTP myproxy3.com:80;SOCKS myproxy4.com:1080;DIRECT", NULL,
     "http://myproxy3.com:80,socks://myproxy4.com:1080,direct://"},
    {"PROXY proxy1.example.com:80; PROXY proxy2.example.com:8080", NULL,
     "http://proxy1.example.com:80,http://proxy2.example.com:8080"},
};

class util_uri_list : public ::testing::TestWithParam<convert_proxy_list_to_uri_list_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_uri_list, testing::ValuesIn(convert_proxy_list_to_uri_list_tests));

TEST_P(util_uri_list, convert_proxy_list) {
    const auto &param = GetParam();
    char *host = convert_proxy_list_to_uri_list(param.proxy_list, param.fallback_scheme);
    EXPECT_NE(host, nullptr);
    if (host) {
        EXPECT_STREQ(host, param.expected);
        free(host);
    }
}
