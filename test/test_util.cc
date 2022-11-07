#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "util.h"
}

TEST(util, dns_resolve_local) {
    int32_t error = 0;
    char *ip = dns_resolve(NULL, &error);
    EXPECT_EQ(error, 0);
    EXPECT_NE(ip, nullptr);
    if (ip) {
        EXPECT_TRUE(strstr(ip, ".") != NULL);
        free(ip);
    }
}

TEST(util, dns_resolve_google) {
    int32_t error = 0;
    char *ip = dns_resolve("google.com", &error);
    EXPECT_EQ(error, 0);
    EXPECT_TRUE(ip != NULL);
    if (ip) {
        EXPECT_TRUE(strstr(ip, ".") != NULL);
        free(ip);
    }
}

TEST(util, dns_resolve_bad) {
    int32_t error = 0;
    char *ip = dns_resolve("hopefully-doesnt-exist.com", &error);
    EXPECT_EQ(ip, nullptr);
    EXPECT_NE(error, 0);
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
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const convert_proxy_list_to_uri_list_param &param) {
        return os << "proxy list: " << param.proxy_list;
    }
};

constexpr convert_proxy_list_to_uri_list_param convert_proxy_list_to_uri_list_tests[] = {
    {"DIRECT", "direct://"},
    {"PROXY 127.0.0.1:80", "http://127.0.0.1:80"},
    {"PROXY 127.0.0.1:8080", "http://127.0.0.1:8080"},
    {"PROXY 127.0.0.1:443", "https://127.0.0.1:443"},
    {"HTTP myproxy1.com:80;SOCKS myproxy2.com:1080", "http://myproxy1.com:80,socks://myproxy2.com:1080"},
    {"HTTP myproxy3.com:80;SOCKS myproxy4.com:1080;DIRECT", "http://myproxy3.com:80,socks://myproxy4.com:1080,direct://"}
    {"PROXY proxy1.example.com:80; PROXY proxy2.example.com:8080", "http://proxy1.example.com:80,http://proxy2.example.com:8080"}
};

class util_uri_list : public ::testing::TestWithParam<convert_proxy_list_to_uri_list_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_uri_list, testing::ValuesIn(convert_proxy_list_to_uri_list_tests));

TEST_P(util_uri_list, convert_proxy_list) {
    const auto &param = GetParam();
    char *host = convert_proxy_list_to_uri_list(param.proxy_list);
    EXPECT_NE(host, nullptr);
    if (host) {
        EXPECT_STREQ(host, param.expected);
        free(host);
    }
}
