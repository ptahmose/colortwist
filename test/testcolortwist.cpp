#include <iostream>
#include <memory>
#include <chrono>
#include <cstring>
#include <colortwist.h>

using namespace std;
using namespace colortwist;

static void TestBgr48();
static void CompareUint16(const char* functionName, const uint16_t* ptr1, const uint16_t* ptr2, size_t length);
static void Test();


int main(int argc, char** argv)
{
    Test();
    TestBgr48();
    //bool b = colorTwistRGB24_C(nullptr, 10, 11, 12, nullptr, 13, nullptr);
    cout << "Hello World" << endl;
    return 0;
}

void Test()
{
    size_t bitmapSize = 32 * 3;
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upSrc((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
    for (size_t i = 0; i < bitmapSize / 2; ++i)
    {
        upSrc.get()[i] = (uint16_t)(1 + i);
    }
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDst((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstC((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });

    static const float twistMatrix[4 * 3] =
    {
        1, 2, 3, 4,
        5, 6, 7, 8,
        1.1f, 1.2f, 1.3f, 1.4f
    };

    colorTwistRGB48(ImplementationType::PlainC, upSrc.get(), 16, 1, 32 * 3, upDstC.get(), 32 * 3, twistMatrix);
    //colorTwistRGB48(ImplementationType::ARM_NEON2, upSrc.get(), 16, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
    colorTwistRGB48(ImplementationType::X64_AVX3, upSrc.get(), 16, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
}

//void Test()
//{
//    size_t bitmapSize = 32 * 3;
//    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upSrc((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
//    for (size_t i = 0; i < bitmapSize / 2; ++i)
//    {
//        upSrc.get()[i] = (uint16_t)(1 + i);
//    }
//    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
//    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDst((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
//
//    static const float twistMatrix[4 * 3] =
//    {
//        1, 2, 3, 4,
//        5, 6, 7, 8,
//        1.1f, 1.2f, 1.3f, 1.4f
//    };
//    colorTwistRGB48(ImplementationType::PlainC, upSrc.get(), 16, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
//    colorTwistRGB48(ImplementationType::X64_AVX2, upSrc.get(), 16, 1, 32 * 3, upDstAvx.get(), 32 * 3, twistMatrix);
//}


void FillWithRandom(void* p, size_t size)
{
    uint8_t* dst = (uint8_t*)p;
    for (size_t i = 0; i < size; ++i)
    {
        *dst++ = (uint8_t)i;
    }
}

static void TestBgr48(const string& name, ImplementationType type, int repeats, int width, int height, const void* pSrc, int strideSrc, void* pDst, int strideDst)
{
    static const float twistMatrix[4 * 3] =
    {
        1, 2, 3, 4,
        5, 6, 7, 8,
        1.1f, 1.2f, 1.3f, 1.4f
    };

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < repeats; ++i)
    {
        colorTwistRGB48(type, pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << name << " -> " << elapsed_seconds.count() << "s" << endl;
}

void TestBgr48()
{
    const int Repeats = 100;
    const int Width = 2048;
    const int Height = 2048;

    const size_t StrideSrc = Width * 3 * 2;
    const size_t StrideDst = Width * 3 * 2;

    size_t bitmapSize = Width * Height * 3 * sizeof(uint16_t);
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upSrc((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
    FillWithRandom(upSrc.get(), bitmapSize);
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstC((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });

    TestBgr48("colorTwistRGB48_C", ImplementationType::PlainC, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstC.get(), StrideDst);

    if (isAvailable(ImplementationType::IPP))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstIpp((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_IPP", ImplementationType::IPP, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstIpp.get(), StrideDst);
        CompareUint16("colorTwistRGB48- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize / 2);
    }

    if (isAvailable(ImplementationType::X64_AVX))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX", ImplementationType::X64_AVX, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX", upDstC.get(), upDstAvx.get(), bitmapSize / 2);
    }

    if (isAvailable(ImplementationType::X64_AVX2))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx2((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX2", ImplementationType::X64_AVX2, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx2.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX2", upDstC.get(), upDstAvx2.get(), bitmapSize / 2);
    }

    if (isAvailable(ImplementationType::X64_AVX3))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx3((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX3", ImplementationType::X64_AVX2, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx3.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX3", upDstC.get(), upDstAvx3.get(), bitmapSize / 2);
    }

    if (isAvailable(ImplementationType::ARM_NEON))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstNeon((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_NEON", ImplementationType::ARM_NEON, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs NEON", upDstC.get(), upDstNeon.get(), bitmapSize / 2);
    }

    if (isAvailable(ImplementationType::ARM_NEON2))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstNeon2((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_NEON2", ImplementationType::ARM_NEON2, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon2.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs NEON2", upDstC.get(), upDstNeon2.get(), bitmapSize / 2);
    }

    //#if COLORTWISTLIB_HASIPP
    //    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstIpp((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
    //#endif
    //#if COLORTWISTLIB_HASAVX
    //    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
    //#endif
    //
    //    static const float twistMatrix[4 * 3] =
    //    {
    //        1, 2, 3, 4,
    //        5, 6, 7, 8,
    //        1.1f, 1.2f, 1.3f, 1.4f };
    //
    //    auto start = std::chrono::high_resolution_clock::now();
    //    for (int i = 0; i < Repeats; ++i)
    //    {
    //        colorTwistRGB48_C(upSrc.get(), Width, Height, Width * 3 * 2, upDstC.get(), Width * 3 * 2, twistMatrix);
    //    }
    //
    //    auto end = std::chrono::high_resolution_clock::now();
    //    std::chrono::duration<double> elapsed_seconds = end - start;
    //    cout << "colorTwistRGB48_C -> " << elapsed_seconds.count() << "s" << endl;
    //
    //#if COLORTWISTLIB_HASIPP
    //    start = std::chrono::high_resolution_clock::now();
    //    for (int i = 0; i < Repeats; ++i)
    //    {
    //        colorTwistRGB48_IPP(upSrc.get(), Width, Height, Width * 3 * 2, upDstIpp.get(), Width * 3 * 2, twistMatrix);
    //    }
    //
    //    end = std::chrono::high_resolution_clock::now();
    //    elapsed_seconds = end - start;
    //    cout << "colorTwistRGB48_IPP -> " << elapsed_seconds.count() << "s" << endl;
    //#endif
    //
    //#if COLORTWISTLIB_HASAVX
    //    start = std::chrono::high_resolution_clock::now();
    //    for (int i = 0; i < Repeats; ++i)
    //    {
    //        colorTwistRGB48_AVX(upSrc.get(), Width, Height, Width * 3 * 2, upDstAvx.get(), Width * 3 * 2, twistMatrix);
    //    }
    //
    //    end = std::chrono::high_resolution_clock::now();
    //    elapsed_seconds = end - start;
    //    cout << "colorTwistRGB48_AVX -> " << elapsed_seconds.count() << "s" << endl;
    //#endif
    //
    //#if COLORTWISTLIB_HASIPP
    //    Compare("colorTwistRGB48- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize);
    //#endif
    //#if COLORTWISTLIB_HASAVX
    //    Compare("colorTwistRGB48- C vs AVX", upDstC.get(), upDstAvx.get(), bitmapSize);
    //#endif
}

void CompareUint16(const char* functionName, const uint16_t* ptr1, const uint16_t* ptr2, size_t length)
{
    //int r = memcmp(ptr1, ptr2, size);
    //cout << functionName << " -> " << ((r == 0) ? "OK" : "FAILURE") << endl;

    bool isOk = true;
    for (size_t i = 0; i < length; ++i)
    {
        auto a = *ptr1++;
        auto b = *ptr2++;
        if (a != b)
        {
            cout << "Offset " << i << " src: " << a << "  dst:" << b << endl;
            isOk = false;
        }
    }

    cout << functionName << " -> " << ((isOk) ? "OK" : "FAILURE") << endl;
}