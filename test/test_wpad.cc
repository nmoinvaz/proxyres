#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "wpad_dhcp.h"
#include "wpad_dns.h"
}

TEST(wpad, dhcp) {
    char *wpad = wpad_dhcp(5);
    EXPECT_NE(wpad, nullptr);
    if (wpad) {
        printf("wpad: %s\n", wpad);
        EXPECT_STREQ(wpad, "http://wpad.com/wpad.dat");
    }
    free(wpad);
}

TEST(wpad, dns) {
    char *wpad = wpad_dns(NULL);
    EXPECT_NE(wpad, nullptr);
    if (wpad) {
        printf("wpad: %s\n", wpad);
        EXPECT_STREQ(wpad, "http://wpad.com/wpad.dat");
    }
    free(wpad);
}
