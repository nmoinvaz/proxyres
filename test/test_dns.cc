#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "dns.h"
}

TEST(dns, resolve_local) {
    int32_t error = 0;
    char *ip = dns_resolve(NULL, &error);
    EXPECT_EQ(error, 0);
    EXPECT_NE(ip, nullptr);
    if (ip) {
        EXPECT_TRUE(strstr(ip, ".") != NULL);
        free(ip);
    }
}

TEST(dns, resolve_google) {
    int32_t error = 0;
    char *ip = dns_resolve("google.com", &error);
    EXPECT_EQ(error, 0);
    EXPECT_TRUE(ip != NULL);
    if (ip) {
        EXPECT_TRUE(strstr(ip, ".") != NULL);
        free(ip);
    }
}

TEST(dns, resolve_bad) {
    int32_t error = 0;
    char *ip = dns_resolve("hopefully-doesnt-exist.com", &error);
    EXPECT_EQ(ip, nullptr);
    EXPECT_NE(error, 0);
}
