
#include <gtest/gtest.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <tap.h>
#pragma GCC diagnostic pop

#include "compiler.h"
#include "c++/Validator.h"

/* unsigned integers */

TEST(RangeValidator, InRangeUint_a) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_TRUE(rv(1024));
}

TEST(RangeValidator, InRangeUint_b) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_TRUE(rv(2047));
}

TEST(RangeValidator, InRangeUint_c) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_TRUE(rv(1400));
}

TEST(RangeValidator, OutOfRangeUintLarge_a) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_FALSE(rv(2048));
}

TEST(RangeValidator, OutOfRangeUintLarge_b) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_FALSE(rv(4096));
}

TEST(RangeValidator, OutOfRangeUintSmall_a) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_FALSE(rv(1023));
}

TEST(RangeValidator, OutOfRangeUintSmall_b) {
    MicroFrameWork::RangeValidator<uint32_t> rv(1024, 2047);
    EXPECT_FALSE(rv(0));
}

/* signed integers */

TEST(RangeValidator, InRangeSint_a) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_TRUE(rv(-1024));
}

TEST(RangeValidator, InRangeSint_b) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_TRUE(rv(1023));
}

TEST(RangeValidator, InRangeSint_c) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_TRUE(rv(0));
}

TEST(RangeValidator, OutOfRangeSintLarge_a) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_FALSE(rv(1024));
}

TEST(RangeValidator, OutOfRangeSintLarge_b) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_FALSE(rv(4096));
}

TEST(RangeValidator, OutOfRangeSintSmall_a) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_FALSE(rv(-1025));
}

TEST(RangeValidator, OutOfRangeSintSmall_b) {
    MicroFrameWork::RangeValidator<int32_t> rv(-1024, 1023);
    EXPECT_FALSE(rv(-15600));
}

/* floats */

TEST(RangeValidator, InRangeFloat_a) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_TRUE(rv(-1023.9));
}

TEST(RangeValidator, InRangeFloat_b) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_TRUE(rv(1022.9));
}

TEST(RangeValidator, InRangeFloat_c) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_TRUE(rv(0.));
}

TEST(RangeValidator, OutOfRangeFloatLarge_a) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_FALSE(rv(1023.1));
}

TEST(RangeValidator, OutOfRangeFloatLarge_b) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_FALSE(rv(4096.));
}

TEST(RangeValidator, OutOfRangeFloatSmall_a) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_FALSE(rv(-1024.1));
}

TEST(RangeValidator, OutOfRangeFloatSmall_b) {
    MicroFrameWork::RangeValidator<float> rv(-1024., 1023.);
    EXPECT_FALSE(rv(-15600.));
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
