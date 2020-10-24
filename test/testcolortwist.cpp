#include <iostream>
#include <memory>
#include <chrono>
#include <cstring>
#include <colortwist.h>
#include <random>

using namespace std;
using namespace colortwist;

static void TestBgr48();
static void TestBgr24();
static void CompareUint16(const char* functionName, const uint16_t* ptr1, const uint16_t* ptr2, size_t length, uint8_t maxDiff);
static void CompareUint8(const char* functionName, const uint8_t* ptr1, const uint8_t* ptr2, size_t length, uint8_t maxDiff);
//static void Test();

int main(int argc, char** argv)
{
    //Test();
    TestBgr24();
    TestBgr48();
    //bool b = colorTwistRGB24_C(nullptr, 10, 11, 12, nullptr, 13, nullptr);
    return 0;
}

/*void Test()
{
    size_t bitmapSize = 32 * 3;
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upSrc((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });
    for (size_t i = 0; i < bitmapSize; ++i)
    {
        upSrc.get()[i] = (uint8_t)(1 + i);
    }
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDst((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstC((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });

    static const float twistMatrix[4 * 3] =
    {
        1, 2, 3, 4,
        5, 6, 7, 8,
        1.1f, 1.2f, 1.3f, 1.4f
    };

    colorTwistRGB24(ImplementationType::PlainC, upSrc.get(), 32, 1, 32 * 3, upDstC.get(), 32 * 1, twistMatrix);
    colorTwistRGB24(ImplementationType::ARM_NEON2, upSrc.get(), 32, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
    //colorTwistRGB24(ImplementationType::X64_AVX3, upSrc.get(), 32, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
}*/


//void Test()
//{
//    size_t bitmapSize = 32 * 3;
//    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upSrc((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
//    for (size_t i = 0; i < bitmapSize / 2; ++i)
//    {
//        upSrc.get()[i] = (uint16_t)(1 + i);
//    }
//    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDst((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
//    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstC((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
//
//    static const float twistMatrix[4 * 3] =
//    {
//        1, 2, 3, 4,
//        5, 6, 7, 8,
//        1.1f, 1.2f, 1.3f, 1.4f
//    };
//
//    colorTwistRGB48(ImplementationType::PlainC, upSrc.get(), 16, 1, 32 * 3, upDstC.get(), 32 * 3, twistMatrix);
//    //colorTwistRGB48(ImplementationType::ARM_NEON2, upSrc.get(), 16, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
//    colorTwistRGB48(ImplementationType::X64_AVX3, upSrc.get(), 16, 1, 32 * 3, upDst.get(), 32 * 3, twistMatrix);
//}

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
     /*uint8_t* dst = (uint8_t*)p;
     for (size_t i = 0; i < size; ++i)
     {
         *dst++ = (uint8_t)i;
     }*/
    uint8_t* dst = (uint8_t*)p;
    std::random_device rd;
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    uint32_t bits = 0;
    for (size_t i = 0; i < size; ++i)
    {
        if ((i % 4) == 0)
        {
            bits = dist(rd);
        }

        *dst++ = (uint8_t)bits;
        bits >>= 8;
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

    size_t dataSize = height * strideSrc;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << name << " -> " << elapsed_seconds.count() << "s, " << (repeats * dataSize / elapsed_seconds.count()) / 1e6 << "MB/s" << endl;
}

static void TestBgr24(const string& name, ImplementationType type, int repeats, int width, int height, const void* pSrc, int strideSrc, void* pDst, int strideDst)
{
    static const float twistMatrix[4 * 3] =
    {
        0.1f, 0.2f, 0.3f, 0.4f,
        0.5f, 0.6f, 0.7f, 0.8f,
        1.1f, 1.2f, 1.3f, 1.4f
    };

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < repeats; ++i)
    {
        colorTwistRGB24(type, pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    size_t dataSize = height * strideSrc;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << name << " -> " << elapsed_seconds.count() << "s, " << (repeats * dataSize / elapsed_seconds.count()) / 1e6 << "MB/s" << endl;
}

void TestBgr48()
{
    const int Repeats =  100;
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
        CompareUint16("colorTwistRGB48- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize / 2, 1);
    }

    if (isAvailable(ImplementationType::X64_AVX))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX", ImplementationType::X64_AVX, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX", upDstC.get(), upDstAvx.get(), bitmapSize / 2, 1);
    }

    if (isAvailable(ImplementationType::X64_AVX2))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx2((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX2", ImplementationType::X64_AVX2, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx2.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX2", upDstC.get(), upDstAvx2.get(), bitmapSize / 2, 1);
    }

    if (isAvailable(ImplementationType::X64_AVX3))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx3((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX3", ImplementationType::X64_AVX3, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx3.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX3", upDstC.get(), upDstAvx3.get(), bitmapSize / 2, 1);
    }

    if (isAvailable(ImplementationType::ARM_NEON))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstNeon((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_NEON", ImplementationType::ARM_NEON, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs NEON", upDstC.get(), upDstNeon.get(), bitmapSize / 2, 1);
    }

    if (isAvailable(ImplementationType::ARM_NEON2))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstNeon2((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_NEON2", ImplementationType::ARM_NEON2, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon2.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs NEON2", upDstC.get(), upDstNeon2.get(), bitmapSize / 2, 1);
    }
}

void TestBgr24()
{
    const int Repeats =  50;
    const int Width = 2048;
    const int Height = 2048;

    const size_t StrideSrc = Width * 3;
    const size_t StrideDst = Width * 3;

    size_t bitmapSize = Width * Height * 3 * sizeof(uint8_t);
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upSrc((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });
    FillWithRandom(upSrc.get(), bitmapSize);
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstC((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });

    TestBgr24("colorTwistRGB24_C", ImplementationType::PlainC, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstC.get(), StrideDst);

    if (isAvailable(ImplementationType::IPP))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstIpp((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_IPP", ImplementationType::IPP, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstIpp.get(), StrideDst);
        CompareUint8("colorTwistRGB24- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize, 1);
    }

    /* if (isAvailable(ImplementationType::X64_AVX))
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
     }*/

    if (isAvailable(ImplementationType::X64_AVX3))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstAvx3((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_AVX3", ImplementationType::X64_AVX3, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx3.get(), StrideDst);
        CompareUint8("colorTwistRGB24: C vs AVX3", upDstC.get(), upDstAvx3.get(), bitmapSize, 1);
    }

    /* if (isAvailable(ImplementationType::ARM_NEON))
     {
         std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstNeon((uint16_t*)malloc(bitmapSize), [](uint16_t* p) -> void { free(p); });
         TestBgr48("colorTwistRGB48_NEON", ImplementationType::ARM_NEON, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon.get(), StrideDst);
         CompareUint16("colorTwistRGB48: C vs NEON", upDstC.get(), upDstNeon.get(), bitmapSize / 2);
     }*/

    if (isAvailable(ImplementationType::ARM_NEON2))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstNeon2((uint8_t*)malloc(bitmapSize), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_NEON2", ImplementationType::ARM_NEON2, Repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon2.get(), StrideDst);
        CompareUint8("colorTwistRGB24: C vs NEON2", upDstC.get(), upDstNeon2.get(), bitmapSize, 1);
    }
}

void CompareUint16(const char* functionName, const uint16_t* ptr1, const uint16_t* ptr2, size_t length, uint8_t maxDiff)
{
    //int r = memcmp(ptr1, ptr2, size);
    //cout << functionName << " -> " << ((r == 0) ? "OK" : "FAILURE") << endl;

    bool isOk = true;
    for (size_t i = 0; i < length; ++i)
    {
        auto a = *ptr1++;
        auto b = *ptr2++;
        if (abs(a - b) > maxDiff)
        {
            cout << "Offset " << i << " src: " << a << "  dst:" << b << endl;
            isOk = false;
        }
    }

    cout << functionName << " -> " << ((isOk) ? "OK" : "FAILURE") << endl;
}

void CompareUint8(const char* functionName, const uint8_t* ptr1, const uint8_t* ptr2, size_t length, uint8_t maxDiff)
{
    //int r = memcmp(ptr1, ptr2, size);
    //cout << functionName << " -> " << ((r == 0) ? "OK" : "FAILURE") << endl;

    bool isOk = true;
    for (size_t i = 0; i < length; ++i)
    {
        auto a = *ptr1++;
        auto b = *ptr2++;
        if (abs(a - b) > maxDiff)
        {
            cout << "Offset " << i << " src: " << (int)a << "  dst:" << (int)b << endl;
            isOk = false;
        }
    }

    cout << functionName << " -> " << ((isOk) ? "OK" : "FAILURE") << endl;
}
