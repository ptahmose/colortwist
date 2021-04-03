#include <stdio.h>

#if defined _WIN32 && (defined(_M_ARM) || defined(_M_ARM64))
# include <Intrin.h>
# include <arm_neon.h>
# define CV_NEON 1
#elif defined(__ARM_NEON__) || defined(__ARM_FEATURE_SIMD32) || (defined (__ARM_NEON) && defined(__aarch64__))
#  include <arm_neon.h>
#  define CV_NEON 1
#endif


#if defined CV_NEON
int test()
{
  const float src[] = { 10.8f, 2.0f, 4.9f, 1.3f };
  float32x4_t value = vld1q_f32((const float32_t*)(src));
  uint32x4_t r = vcvtnq_u32_f32(value);
  uint32_t results[4];
  vst1q_u32(results, r);
  //if (r.val[0] == 11 && r.val[1] == 2 && r.val[2] == 5 && r.val[3] == 1)
  if (results[0] == 11 && results[1] == 2 && results[2] == 5 && results[3] == 1)
  {
    return 0;
  }

  return -1;
}
#else
#error "NEON is not supported"
#endif

int main()
{
  printf("%d\n", test());
  return 0;
}
