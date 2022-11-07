#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "fetch.h"
}

TEST(fetch, get) {
    int32_t error = 0;
    char *body = fetch_get("http://google.com", &error);
    EXPECT_EQ(error, 0);
    EXPECT_NE(body, nullptr);
    if (body) {
        EXPECT_TRUE(strstr(body, "google") != nullptr);
        EXPECT_TRUE(strstr(body, "document has moved") != nullptr);
        free(body);
    }
}
