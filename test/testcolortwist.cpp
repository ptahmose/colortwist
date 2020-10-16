#include <iostream>
#include <chrono>
#include <colortwist.h>

using namespace std;

static void TestBgr48();
static void Compare(const char *functionName, const void *ptr1, const void *ptr2, size_t size);

int main(int argc, char **argv)
{
    TestBgr48();
    //bool b = colorTwistRGB24_C(nullptr, 10, 11, 12, nullptr, 13, nullptr);
    cout << "Hello World" << endl;
    return 0;
}

void FillWithRandom(void *p, size_t size)
{
    uint8_t *dst = (uint8_t *)p;
    for (size_t i = 0; i < size; ++i)
    {
        *dst++ = (uint8_t)i;
    }
}

void TestBgr48()
{
    const int Repeats = 100;
    const int Width = 2048;
    const int Height = 2048;

    size_t bitmapSize = Width * Height * 3 * sizeof(uint16_t);
    std::unique_ptr<uint16_t, void (*)(uint16_t *)> upSrc((uint16_t *)malloc(bitmapSize), [](uint16_t *p) -> void { free(p); });
    FillWithRandom(upSrc.get(), bitmapSize);
    std::unique_ptr<uint16_t, void (*)(uint16_t *)> upDstC((uint16_t *)malloc(bitmapSize), [](uint16_t *p) -> void { free(p); });

#if COLORTWISTLIB_HASIPP
    std::unique_ptr<uint16_t, void (*)(uint16_t *)> upDstIpp((uint16_t *)malloc(bitmapSize), [](uint16_t *p) -> void { free(p); });
#endif
#if COLORTWISTLIB_HASAVX
    std::unique_ptr<uint16_t, void (*)(uint16_t *)> upDstAvx((uint16_t *)malloc(bitmapSize), [](uint16_t *p) -> void { free(p); });
#endif

    static const float twistMatrix[4 * 3] =
        {
            1, 2, 3, 4,
            5, 6, 7, 8,
            1.1f, 1.2f, 1.3f, 1.4f};

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Repeats; ++i)
    {
        colorTwistRGB48_C(upSrc.get(), Width, Height, Width * 3 * 2, upDstC.get(), Width * 3 * 2, twistMatrix);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << "colorTwistRGB48_C -> " << elapsed_seconds.count() << "s" << endl;

#if COLORTWISTLIB_HASIPP
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Repeats; ++i)
    {
        colorTwistRGB48_IPP(upSrc.get(), Width, Height, Width * 3 * 2, upDstIpp.get(), Width * 3 * 2, twistMatrix);
    }

    end = std::chrono::high_resolution_clock::now();
    elapsed_seconds = end - start;
    cout << "colorTwistRGB48_IPP -> " << elapsed_seconds.count() << "s" << endl;
#endif

#if COLORTWISTLIB_HASAVX
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Repeats; ++i)
    {
        colorTwistRGB48_AVX(upSrc.get(), Width, Height, Width * 3 * 2, upDstAvx.get(), Width * 3 * 2, twistMatrix);
    }

    end = std::chrono::high_resolution_clock::now();
    elapsed_seconds = end - start;
    cout << "colorTwistRGB48_AVX -> " << elapsed_seconds.count() << "s" << endl;
#endif

#if COLORTWISTLIB_HASIPP
    Compare("colorTwistRGB48- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize);
#endif
#if COLORTWISTLIB_HASAVX
    Compare("colorTwistRGB48- C vs AVX", upDstC.get(), upDstAvx.get(), bitmapSize);
#endif
}

void Compare(const char *functionName, const void *ptr1, const void *ptr2, size_t size)
{
    int r = memcmp(ptr1, ptr2, size);
    cout << functionName << " -> " << ((r == 0) ? "OK" : "FAILURE") << endl;
}