#include <gtest/gtest.h>
#include <cstdint>
#include <colortwist.h>

using namespace std;
using namespace colortwist;

struct ImplementationTypeFixture : public testing::TestWithParam<ImplementationType> {};

TEST_P(ImplementationTypeFixture, Bgr48SinglePixelCheckResult1)
{
    static const float kTwistMatrix[4 * 3] =
    {
      1, 2, 3, 4,
      5, 6, 7, 8,
      1.1f, 1.2f, 1.3f, 1.4f
    };

    uint16_t src[3] = { 57790,20950,17269 };
    uint16_t result[3] = {};

    const auto return_code = colorTwistRGB48(
        ImplementationType::X86_SSE,
        src,
        1,
        1,
        3 * 2,
        result,
        3 * 2,
        kTwistMatrix);

    ASSERT_EQ(return_code, StatusCode::OK);

    EXPECT_EQ(65535, result[0]);
    EXPECT_EQ(65535, result[1]);
    EXPECT_EQ(65535, result[2]);
}

TEST_P(ImplementationTypeFixture, Bgr48SinglePixelCheckResult2)
{
    const ImplementationType implementation_type = GetParam();

    if (!isOperationalRgb48(implementation_type))
    {
        GTEST_SKIP() << "type '" << GetImplementationTypeAsInformalString(implementation_type) << "' not operational.";
        return;
    }

    static const float kTwistMatrix[4 * 3] =
    {
      1, 2, 3, 4,
      5, 6, 7, 8,
      1.1f, 1.2f, 1.3f, 1.4f
    };

    uint16_t src[3] = { 13379,16197,1006 };
    uint16_t result[3] = {};

    const auto return_code = colorTwistRGB48(
        implementation_type,
        src,
        1,
        1,
        3 * 2,
        result,
        3 * 2,
        kTwistMatrix);

    ASSERT_EQ(return_code, StatusCode::OK);

    EXPECT_EQ(48795, result[0]);
    EXPECT_EQ(65535, result[1]);
    EXPECT_TRUE(35462 == result[2] || 35463 == result[2]);
}

TEST_P(ImplementationTypeFixture, Bgr48SinglePixelCheckResult3)
{
    const ImplementationType implementation_type = GetParam();

    if (!isOperationalRgb48(implementation_type))
    {
        GTEST_SKIP() << "type '" << GetImplementationTypeAsInformalString(implementation_type) << "' not operational.";
        return;
    }

    static constexpr float kTwistMatrix[4 * 3] =
    {
      1, 2, 3, 4,
      5, 6, 7, 8,
      1.1f, 1.2f, 1.3f, 1.4f
    };

    constexpr uint16_t src[3] = { 5556,8013,5363 };
    uint16_t result[3] = {};

    const auto return_code = colorTwistRGB48(
        implementation_type,
        src,
        1,
        1,
        3 * 2,
        result,
        3 * 2,
        kTwistMatrix);

    ASSERT_EQ(return_code, StatusCode::OK);

    EXPECT_EQ(37675, result[0]);
    EXPECT_EQ(65535, result[1]);
    EXPECT_TRUE(22700 == result[2] || 22701 == result[2]);
}

TEST_P(ImplementationTypeFixture, Bgr24SinglePixelCheckResult1)
{
    const ImplementationType implementation_type = GetParam();

    if (!isOperationalRgb24(implementation_type))
    {
        GTEST_SKIP() << "type '" << GetImplementationTypeAsInformalString(implementation_type) << "' not operational.";
        return;
    }

    static constexpr float kTwistMatrix[4 * 3] =
    {
        0.1f, 0.2f, 0.3f, 0.4f,
        0.5f, 0.6f, 0.7f, 0.8f,
        1.1f, 1.2f, 1.3f, 1.4f
    };

    constexpr uint8_t src[3] = { 41, 58,157 };
    uint8_t result[3] = {};

    const auto return_code = colorTwistRGB24(
        implementation_type,
        src,
        1,
        1,
        3 * 1,
        result,
        3 * 1,
        kTwistMatrix);

    ASSERT_EQ(return_code, StatusCode::OK);

    EXPECT_EQ(63, result[0]);
    EXPECT_EQ(166, result[1]);
    EXPECT_TRUE(255 == result[2]);
}

TEST_P(ImplementationTypeFixture, Bgr24InvalidArgumentsExpectError)
{
    const ImplementationType implementation_type = GetParam();

    if (!isOperationalRgb24(implementation_type))
    {
               GTEST_SKIP() << "type '" << GetImplementationTypeAsInformalString(implementation_type) << "' not operational.";
        return;
    }

    static constexpr float kTwistMatrix[4 * 3] =
    {
           0.1f, 0.2f, 0.3f, 0.4f,
           0.5f, 0.6f, 0.7f, 0.8f,
           1.1f, 1.2f, 1.3f, 1.4f
       };

    constexpr uint8_t src[3] = { 41, 58,157 };
    uint8_t result[3] = {};

    const auto return_code = colorTwistRGB24(
        implementation_type,
        nullptr,
        1,
        1,
        3 * 1,
        result,
        3 * 1,
        kTwistMatrix);

    ASSERT_EQ(return_code, StatusCode::InvalidPointer);
}


INSTANTIATE_TEST_SUITE_P(
    ColorTwist,
    ImplementationTypeFixture,
    testing::Values(ImplementationType::PlainC, ImplementationType::X86_SSE, ImplementationType::X64_AVX3, ImplementationType::ARM_NEON2));
