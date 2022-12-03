#include <stdint.h>

#include "gtest/gtest.h"

#include "proxyres/proxyres.h"

GTEST_API_ int main(int argc, char **argv) {
    int32_t error = 0;
    proxyres_init();
    testing::InitGoogleTest(&argc, argv);
    error = RUN_ALL_TESTS();
    proxyres_uninit();
    return error;
}