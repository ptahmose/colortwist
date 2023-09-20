#include <gtest/gtest.h>
#include <cstdint>
#include <colortwist.h>

using namespace std;
using namespace colortwist;

TEST(ColorTwist, Test)
{
  static const float kTwistMatrix[4 * 3] =
  {
    1, 2, 3, 4,
    5, 6, 7, 8,
    1.1f, 1.2f, 1.3f, 1.4f
  };

  uint16_t src[3] = { 57790,20950,17269 };
  uint16_t result[3];

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