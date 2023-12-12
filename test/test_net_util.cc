#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

#include "net_util.h"

TEST(net_util, my_ip_address) {
    char *address = my_ip_address();
    EXPECT_TRUE(address != NULL);
    if (address) {
        EXPECT_TRUE(strstr(address, ".") != NULL);
        EXPECT_TRUE(strstr(address, ";") == NULL);
        EXPECT_TRUE(strstr(address, ":") == NULL);
        free(address);
    }
}

TEST(net_util, my_ip_address_ex) {
    char *addresses = my_ip_address_ex();
    EXPECT_TRUE(addresses != NULL);
    if (addresses) {
        EXPECT_TRUE(strstr(addresses, ".") != NULL || strstr(addresses, ":") != NULL);
        free(addresses);
    }
}

TEST(net_util, dns_resolve_google) {
    int32_t error = 0;
    char *ip = dns_resolve("google.com", &error);
    EXPECT_EQ(error, 0);
    EXPECT_TRUE(ip != NULL);
    if (ip) {
        EXPECT_TRUE(strstr(ip, ".") != NULL);
        EXPECT_TRUE(strstr(ip, ";") == NULL);
        EXPECT_TRUE(strstr(ip, ":") == NULL);
        free(ip);
    }
}

TEST(net_util, dns_resolve_bad) {
    int32_t error = 0;
    char *ip = dns_resolve("hopefully-doesnt-exist.com", &error);
    EXPECT_EQ(ip, nullptr);
    EXPECT_NE(error, 0);
}

TEST(net_util, dns_ex_resolve_google) {
    int32_t error = 0;
    char *ips = dns_resolve_ex("google.com", &error);
    EXPECT_EQ(error, 0);
    EXPECT_TRUE(ips != NULL);
    if (ips) {
        EXPECT_TRUE(strstr(ips, ".") != NULL || strstr(ips, ":") != NULL);
        free(ips);
    }
}
