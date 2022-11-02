#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "log.h"
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
    char *ip = dns_resolve("google-hopefully-doesnt-exist.com", &error);
    EXPECT_EQ(ip, nullptr);
    EXPECT_NE(error, 0);
}

struct parse_url_host_param {
    const char *string;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const parse_url_host_param &param) {
        return os << "string: " << param.string;
    }
};

constexpr parse_url_host_param parse_url_host_tests[] = {
    {"https://google.com/", "google.com"},
    {"https://google.com", "google.com"},
    {"google.com", "google.com"},
    {"google.com/", "google.com"},
    {"https://u:p@google.com/", "google.com"},
    {"https://u:p@www.google.com", "www.google.com"},
};

class util_url_host : public ::testing::TestWithParam<parse_url_host_param> {};

INSTANTIATE_TEST_SUITE_P(util, util_url_host, testing::ValuesIn(parse_url_host_tests));

TEST_P(util_url_host, parse) {
    const auto &param = GetParam();
    char *host = parse_url_host(param.string);
    EXPECT_NE(host, nullptr);
    if (host) {
        EXPECT_STREQ(host, param.expected);
        free(host);
    }
}
