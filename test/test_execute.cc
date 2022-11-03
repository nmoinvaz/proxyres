#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtest/gtest.h>

extern "C" {
#include "execute.h"
}

struct execute_param {
    const char *url;
    const char *expected;

    friend std::ostream &operator<<(std::ostream &os, const execute_param &param) {
        return os << "url: " << param.url;
    }
};

static const char *script = R"(
function FindProxyForURL(url, host) {
  if (host == "127.0.0.1") {
    return "PROXY localhost:30";
  }
  if (isPlainHostName(host)) {
    return "HTTP plain";
  }
  if (host == "example1.com") {
    return "PROXY no-such-proxy:80";
  }
  if (shExpMatch(url, '*microsoft.com/*')) {
    return "PROXY microsoft.com:80";
  }
  return "DIRECT";
})";

constexpr execute_param execute_tests[] = {
    {"your-pc", "HTTP plain"},
    {"127.0.0.1", "PROXY localhost:30"},
    {"http://127.0.0.1/", "PROXY localhost:30"},
    {"http://example1.com/", "PROXY no-such-proxy:80"},
    {"http://example2.com/", "DIRECT"},
    {"http://microsoft.com/test", "PROXY microsoft.com:80"}
};

class execute : public ::testing::TestWithParam<execute_param> {};

INSTANTIATE_TEST_SUITE_P(execute, execute, testing::ValuesIn(execute_tests));

TEST_P(execute, get_proxies_for_url) {
    const auto &param = GetParam();
    void *proxy_execute = proxy_execute_create();
    EXPECT_NE(proxy_execute, nullptr);
    if (proxy_execute) {
        int32_t error = 0;
        EXPECT_TRUE(proxy_execute_get_proxies_for_url(proxy_execute, script, param.url));
        EXPECT_TRUE(proxy_execute_get_error(proxy_execute, &error));
        EXPECT_EQ(error, 0);
        const char *list = proxy_execute_get_list(proxy_execute);
        EXPECT_NE(list, nullptr);
        if (list)
            EXPECT_STREQ(list, param.expected);
        proxy_execute_delete(&proxy_execute);
    }
}