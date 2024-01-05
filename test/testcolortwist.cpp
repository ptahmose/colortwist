#include <iostream>
#include <memory>
#include <chrono>
#include <colortwist.h>
#include <random>

using namespace std;
using namespace colortwist;

static void TestBgr48(int repeats);
static void TestBgr24(int repeats);
static void CompareUint16(const char* functionName, const uint16_t* ptr1, const uint16_t* ptr2, size_t length, uint8_t maxDiff);
static void CompareUint8(const char* functionName, const uint8_t* ptr1, const uint8_t* ptr2, size_t length, uint8_t maxDiff);

int main(int argc, char** argv)
{
    int repeats = 100;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-r" || arg == "--repeats") && i + 1 < argc)
        {
            repeats = std::atoi(argv[++i]);
            if (repeats <= 0)
            {
                std::cerr << "Error: The number of repeats must be a positive integer." << endl;
                return 1;
            }
        }
    }

    std::cout << "Repeats: " << repeats << endl;

    TestBgr24(repeats);
    TestBgr48(repeats);

    return 0;
}

void FillWithRandom(void* p, size_t size)
{
    uint8_t* dst = static_cast<uint8_t*>(p);
    std::random_device rd;
    std::mt19937 gen(rd()); //seed for rd(Mersenne twister)
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    uint32_t bits = 0;
    for (size_t i = 0; i < size; ++i)
    {
        if ((i % 4) == 0)
        {
            bits = dist(gen);
        }

        *dst++ = static_cast<uint8_t>(bits);
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

    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < repeats; ++i)
    {
        colorTwistRGB48(type, pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    const size_t dataSize = height * strideSrc;
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed_seconds = end - start;
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

    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < repeats; ++i)
    {
        colorTwistRGB24(type, pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    const size_t dataSize = height * strideSrc;
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed_seconds = end - start;
    cout << name << " -> " << elapsed_seconds.count() << "s, " << (repeats * dataSize / elapsed_seconds.count()) / 1e6 << "MB/s" << endl;
}

void TestBgr48(int repeats)
{
    //const int Repeats = 1;// 100;
    const int Width = 2049;
    const int Height = 2048;

    const size_t StrideSrc = Width * 3 * 2;
    const size_t StrideDst = Width * 3 * 2;

    const size_t bitmapSize = Width * Height * 3 * sizeof(uint16_t);
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upSrc(static_cast<uint16_t*>(malloc(bitmapSize)), [](uint16_t* p) -> void { free(p); });
    FillWithRandom(upSrc.get(), bitmapSize);
    std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstC(static_cast<uint16_t*>(malloc(bitmapSize)), [](uint16_t* p) -> void { free(p); });

    TestBgr48("colorTwistRGB48_C", ImplementationType::PlainC, repeats, Width, Height, upSrc.get(), StrideSrc, upDstC.get(), StrideDst);

    if (isOperationalRgb48(ImplementationType::IPP))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstIpp(static_cast<uint16_t*>(malloc(bitmapSize)), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_IPP", ImplementationType::IPP, repeats, Width, Height, upSrc.get(), StrideSrc, upDstIpp.get(), StrideDst);
        CompareUint16("colorTwistRGB48- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize / 2, 1);
    }

    if (isOperationalRgb48(ImplementationType::X64_AVX))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstAvx(static_cast<uint16_t*>(malloc(bitmapSize)), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_AVX", ImplementationType::X64_AVX, repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs AVX", upDstC.get(), upDstAvx.get(), bitmapSize / 2, 1);
    }

    if (isOperationalRgb24(ImplementationType::X86_SSE))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstSse(static_cast<uint16_t*>(malloc(bitmapSize)), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_SSE", ImplementationType::X86_SSE, repeats, Width, Height, upSrc.get(), StrideSrc, upDstSse.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs SSE", upDstC.get(), upDstSse.get(), bitmapSize / 2, 1);
    }

    if (isOperationalRgb48(ImplementationType::ARM_NEON))
    {
        std::unique_ptr<uint16_t, void (*)(uint16_t*)> upDstNeon2(static_cast<uint16_t*>(malloc(bitmapSize)), [](uint16_t* p) -> void { free(p); });
        TestBgr48("colorTwistRGB48_NEON", ImplementationType::ARM_NEON, repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon2.get(), StrideDst);
        CompareUint16("colorTwistRGB48: C vs NEON", upDstC.get(), upDstNeon2.get(), bitmapSize / 2, 1);
    }
}

void TestBgr24(int repeats)
{
    const int Width = 2048;
    const int Height = 2048;

    const size_t StrideSrc = Width * 3;
    const size_t StrideDst = Width * 3;

    const size_t bitmapSize = Width * Height * 3 * sizeof(uint8_t);
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upSrc(static_cast<uint8_t*>(malloc(bitmapSize)), [](uint8_t* p) -> void { free(p); });
    FillWithRandom(upSrc.get(), bitmapSize);
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstC(static_cast<uint8_t*>(malloc(bitmapSize)), [](uint8_t* p) -> void { free(p); });

    TestBgr24("colorTwistRGB24_C", ImplementationType::PlainC, repeats, Width, Height, upSrc.get(), StrideSrc, upDstC.get(), StrideDst);

    if (isOperationalRgb24(ImplementationType::IPP))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstIpp(static_cast<uint8_t*>(malloc(bitmapSize)), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_IPP", ImplementationType::IPP, repeats, Width, Height, upSrc.get(), StrideSrc, upDstIpp.get(), StrideDst);
        CompareUint8("colorTwistRGB24- C vs IPP", upDstC.get(), upDstIpp.get(), bitmapSize, 1);
    }

    if (isOperationalRgb24(ImplementationType::X64_AVX))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstAvx(static_cast<uint8_t*>(malloc(bitmapSize)), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_AVX", ImplementationType::X64_AVX, repeats, Width, Height, upSrc.get(), StrideSrc, upDstAvx.get(), StrideDst);
        CompareUint8("colorTwistRGB24: C vs AVX", upDstC.get(), upDstAvx.get(), bitmapSize, 1);
    }

    if (isOperationalRgb24(ImplementationType::X86_SSE))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstSse(static_cast<uint8_t*>(malloc(bitmapSize)), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_SSE", ImplementationType::X86_SSE, repeats, Width, Height, upSrc.get(), StrideSrc, upDstSse.get(), StrideDst);
        CompareUint8("colorTwistRGB24: C vs SSE", upDstC.get(), upDstSse.get(), bitmapSize, 1);
    }

    if (isOperationalRgb24(ImplementationType::ARM_NEON))
    {
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> upDstNeon2(static_cast<uint8_t*>(malloc(bitmapSize)), [](uint8_t* p) -> void { free(p); });
        TestBgr24("colorTwistRGB24_NEON", ImplementationType::ARM_NEON, repeats, Width, Height, upSrc.get(), StrideSrc, upDstNeon2.get(), StrideDst);
        CompareUint8("colorTwistRGB24: C vs NEON", upDstC.get(), upDstNeon2.get(), bitmapSize, 1);
    }
}

void CompareUint16(const char* functionName, const uint16_t* ptr1, const uint16_t* ptr2, size_t length, uint8_t maxDiff)
{
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
    bool isOk = true;
    for (size_t i = 0; i < length; ++i)
    {
        auto a = *ptr1++;
        auto b = *ptr2++;
        if (abs(a - b) > maxDiff)
        {
            cout << "Offset " << i << " src: " << static_cast<int>(a) << "  dst:" << static_cast<int>(b) << endl;
            isOk = false;
        }
    }

    cout << functionName << " -> " << ((isOk) ? "OK" : "FAILURE") << endl;
}
