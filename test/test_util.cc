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

TEST_P(util_url_host, get) {
    const auto &param = GetParam();
    char *host = get_url_host(param.string);
    EXPECT_NE(host, nullptr);
    if (host) {
        EXPECT_STREQ(host, param.expected);
        free(host);
    }
}

struct get_url_from_host_param {
    const char *scheme;
    const char *host;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const get_url_from_host_param &param) {
        return os << "scheme: " << param.scheme << std::endl
                  << "host: " << param.host;
    }
};

constexpr get_url_from_host_param get_url_from_host_tests[] = {
    // Http no scheme no port
    {"http", "1.1.1.1", "http://1.1.1.1:80"},
    {"http", "yahoo.com", "http://yahoo.com:80"},
    // Http no scheme with port
    {"http", "1.1.1.1:80", "http://1.1.1.1:80"},
    // Https no scheme no port
    {"https", "127.0.0.1", "http://127.0.0.1:80"},
    {"https://google.com", "google.com", "https://google.com:80"},
    // Http scheme no port
    {"http", "http://bing.com", "http://bing.com:80"},
    // Https scheme no port
    {"https", "https://bing.com", "https://bing.com:443"},
    // Https scheme with port
    {"https", "https://bing.com:443", "https://bing.com:443"},
    // Http mixed-scheme
    {"http", "https://bing.com", "https://bing.com:443"},
    // Https mixed-scheme
    {"https", "http://bing.com", "http://bing.com:80"},
};

class util_url_from_host : public ::testing::TestWithParam<get_url_from_host_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_url_from_host, testing::ValuesIn(get_url_from_host_tests));

TEST_P(util_url_from_host, get) {
    const auto &param = GetParam();
    char *host_url = get_url_from_host(param.scheme, param.host);
    EXPECT_NE(host_url, nullptr);
    if (host_url) {
        EXPECT_STREQ(host_url, param.expected);
        free(host_url);
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

struct should_bypass_list_param {
    const char *url;
    const char *bypass_list;
    bool expected;

    friend std::ostream &operator<<(std::ostream &os, const should_bypass_list_param &param) {
        return os << "url: " << param.url << std::endl << "bypass list: " << param.bypass_list;
    }
};

constexpr should_bypass_list_param should_bypass_list_tests[] = {
    // Bypass due to localhost hostname
    {"http://localhost/", "", true},
    // Bypass due to localhost ip
    {"http://127.0.0.1/", "", true},
    // Bypass due to <local> on simple hostnames
    {"http://apple/", "<local>", true},
    // Don't bypass due to <local> only applying to simple hostnames
    {"http://apple.com/", "<local>", false},
    // Don't bypass due to <-loopback> on localhost ip
    {"http://127.0.0.1/", "<-loopback>", false},
    // Don't bypass due to <-loopback> not applying to non-localhost address
    {"http://120.0.0.1/", "<-loopback>", false},
    // Don't bypass due to no bypass list specified
    {"http://google.com/", "", false},
    // Bypass due to url matches
    {"http://google.com/", "google.com", true},
    {"http://google.com/", "google.com,apple.com", true},
    // Bypass due to url matches with wildcard
    {"http://google.com/", "*google.com", true},
    {"http://google.com/", "google.com,apple.com", true},
    // Don't bypass due to no url matches
    {"http://google.com/", "apple.com", false},
    // Bypass subdomains
    {"http://my.google.com/", ".google.com", true},
    // Don't bypass if subdomain not supplied
    {"http://google.com/", ".google.com", false},
};

class util_should_bypass : public ::testing::TestWithParam<should_bypass_list_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_should_bypass, testing::ValuesIn(should_bypass_list_tests));

TEST_P(util_should_bypass, list) {
    const auto &param = GetParam();
    EXPECT_EQ(should_bypass_proxy(param.url, param.bypass_list), param.expected);
}
