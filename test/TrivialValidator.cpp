#include <gtest/gtest.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <tap.h>
#pragma GCC diagnostic pop

#include "common/compiler.h"
#include "c++/Validator.h"

TEST(TrivialValidator, AlwaysReturnsTrue0) {
    MicroFrameWork::TrivialValidator<bool> tv;
    EXPECT_TRUE(tv(true));
}

TEST(TrivialValidator, AlwaysReturnsTrue1) {
    MicroFrameWork::TrivialValidator<bool> tv;
    EXPECT_TRUE(tv(false));
}

int
main(UNUSED int argc, UNUSED char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::TestEventListeners& listeners =
          testing::UnitTest::GetInstance()->listeners();

    /* Delete the default listener */
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new tap::TapListener());
    return RUN_ALL_TESTS();
}
