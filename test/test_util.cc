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
