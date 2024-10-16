# colortwist

This is a little finger exercise in SIMD-optimization. The example we try to optimize is an image processing operation known as "color twist". It is a matrix-multication of an RGB-triple and a given matrix.
So, if we have the source pixel values r, g, b, then the destination pixels R, G, B are calculated as 

$$
R = t_{11} \cdot r + t_{12} \cdot g + t_{13} \cdot b + t_{14}
$$

$$
G = t_{21} \cdot r + t_{22} \cdot g + t_{23} \cdot b + t_{24}
$$

$$
B = t_{31} \cdot r + t_{32} \cdot g + t_{33} \cdot b + t_{34}
$$

where 

$$
\begin{equation}
T = \begin{pmatrix}
    t_{11} & t_{12} & t_{13} & t_{14} \\
    t_{21} & t_{22} & t_{23} & t_{24} \\
    t_{31} & t_{32} & t_{33} & t_{34}
\end{pmatrix}
\end{equation}
$$

is the color twist matrix.

There are SIMD-optimized versions leveraging [AVX-instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions) (for x86) and [Neon-instructions](https://en.wikipedia.org/wiki/ARM_architecture#Advanced_SIMD_(NEON)) (for ARM).

# How fast is it?

Here are some numbers (1MB = 1,000,000 bytes):

[Intel Core i7-8700K @ 3.70GHz](https://ark.intel.com/content/www/us/en/ark/products/126684/intel-core-i7-8700k-processor-12m-cache-up-to-4-70-ghz.html), msvc 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1226.24             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1720.98  |
| colorTwistRGB48_AVX    | 3291.02             |
| colorTwistRGB48_AVX2   | 4297.74             |
| colorTwistRGB48_AVX3   | 5175.55             |
| colorTwistRGB24_C      | 318.01              |
| colorTwistRGB24_IPP    | 892.93              |
| colorTwistRGB24_AVX3   | 2966.55             |

[Intel Core i7-8700K @ 3.70GHz](https://ark.intel.com/content/www/us/en/ark/products/126684/intel-core-i7-8700k-processor-12m-cache-up-to-4-70-ghz.html), Intel icl 19.1.3.311, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1313.22             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1809.26  |
| colorTwistRGB48_AVX    | 3354.22             |
| colorTwistRGB48_AVX2   | 4452.38             |
| colorTwistRGB48_AVX3   | 5207.50             |
| colorTwistRGB24_C      | 340.01              |
| colorTwistRGB24_IPP    | 922.14              |
| colorTwistRGB24_AVX3   | 3180.88             |

Intel Core i7-8700K @ 3.70GHz, gcc 10.2.0, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1453.32             |
| colorTwistRGB48_AVX    | 3334.76             |
| colorTwistRGB48_AVX2   | 4315.26             |
| colorTwistRGB48_AVX3   | 5011.39             |
| colorTwistRGB24_C      | 328.43              |
| colorTwistRGB24_AVX3   | 2998.93             |

Raspberry Pi 4, gcc 8.3.0-6, 32bit : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 366.99              |
| colorTwistRGB48_NEON   | 410.42              |
| colorTwistRGB48_NEON2  | 1009.3              |
| colorTwistRGB24_C      | 96.14               |
| colorTwistRGB24_NEON2  | 480.69              |

Raspberry Pi 4, gcc 10.3.0, 64bit : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 439.89              |
| colorTwistRGB48_NEON   | 417.84              |
| colorTwistRGB48_NEON2  | 942.86              |
| colorTwistRGB24_C      | 120.64              |
| colorTwistRGB24_NEON2  | 462.34              |

Raspberry Pi 3b, gcc 8.3.0-6, 32bit : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 208.69              |
| colorTwistRGB48_NEON   | 187.42              |
| colorTwistRGB48_NEON2  | 435.30              |
| colorTwistRGB24_C      | 63.30               |
| colorTwistRGB24_NEON2  | 285.971             |

AMD Ryzen 7 4700U, msvc 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1202.02             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1836.64  |
| colorTwistRGB48_AVX    | 3810.53             |
| colorTwistRGB48_AVX2   | 4970.00             |
| colorTwistRGB48_AVX3   | 5572.38             |
| colorTwistRGB24_C      | 297.13              |
| colorTwistRGB24_IPP    | 920.24              |
| colorTwistRGB24_AVX3   | 2548.42             |

AMD Ryzen 7 4700U,  Intel icl 19.1.3.311, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1355.20             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1862.04  |
| colorTwistRGB48_AVX    | 3881.71             |
| colorTwistRGB48_AVX2   | 5150.99             |
| colorTwistRGB48_AVX3   | 5736.88             |
| colorTwistRGB24_C      | 287.26              |
| colorTwistRGB24_IPP    | 952.50              |
| colorTwistRGB24_AVX3   | 2508.71             |

[Intel Core m3-6Y30 @ 0.9GHz](https://ark.intel.com/content/www/us/en/ark/products/88198/intel-core-m3-6y30-processor-4m-cache-up-to-2-20-ghz.html), msvc 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 650.32              |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 935.62  |
| colorTwistRGB48_AVX    | 1769.30             |
| colorTwistRGB48_AVX2   | 2370.21             |
| colorTwistRGB48_AVX3   | 2943.29             |
| colorTwistRGB24_C      | 157.44              |
| colorTwistRGB24_IPP    | 466.74              |
| colorTwistRGB24_AVX3   | 1793.66             |

Samsung Galaxy Book Go ([Snapdragon 7c Gen2 @ 2.55GHz](https://www.qualcomm.com/products/snapdragon-7c-gen-2-compute-platform)), msvc 19.29.30133.0, ARM64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 827.08              |
| colorTwistRGB48_NEON   | 1252.92             |
| colorTwistRGB48_NEON2  | 2758.96             |
| colorTwistRGB24_C      | 222.86              |
| colorTwistRGB24_NEON2  | 1201.04             |

Samsung Galaxy Book Go ([Snapdragon 7c Gen2 @ 2.55GHz](https://www.qualcomm.com/products/snapdragon-7c-gen-2-compute-platform)), msvc 19.29.30133.0, ARM : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 583.61              |
| colorTwistRGB48_NEON   | 1241.57             |
| colorTwistRGB48_NEON2  | 2581.25             |
| colorTwistRGB24_C      | 208.34              |
| colorTwistRGB24_NEON2  | 1336.50             |

[AMD Ryzen 5 1600X](https://www.amd.com/en/products/cpu/amd-ryzen-5-1600x), gcc 7.5.0, Linux x64  : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1344.27             |
| colorTwistRGB48_AVX    | 3186.97             |
| colorTwistRGB48_AVX2   | 3445.82             |
| colorTwistRGB48_AVX3   | 4008.93             |
| colorTwistRGB24_C      | 281.88              |
| colorTwistRGB24_AVX3   | 2014.70             |

[Intel Core i9-12900K @ 3.20GHz](https://ark.intel.com/content/www/us/en/ark/products/134599/intel-core-i912900k-processor-30m-cache-up-to-5-20-ghz.html), msvc 19.31.31106.2, x64 : 2048x2048 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1928.36             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 2203.86  |
| colorTwistRGB48_AVX    | 4058.93             |
| colorTwistRGB48_AVX2   | 8323.63             |
| colorTwistRGB48_AVX3   | 9091.08             |
| colorTwistRGB24_C      | 440.10              |
| colorTwistRGB24_IPP    | 1116.93             |
| colorTwistRGB24_AVX3   | 5441.54             |

[MacBook Pro M2](https://www.apple.com/macbook-pro-13/specs/), clang-1400.0.29.202, arm64 : 2048x2048 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 5301.62             |
| colorTwistRGB48_NEON   | 7226.25             |
| colorTwistRGB48_NEON2  | 9188.08             |
| colorTwistRGB24_C      | 2863.86             |
| colorTwistRGB24_NEON2  | 4584.36             |

[Lenovo Slim 7x](https://www.lenovo.com/us/en/p/laptops/yoga/yoga-slim-series/yoga-slim-7x-gen-9-14-inch-snapdragon/len101y0049), MSVC 19.41.34123.0, arm64 : 2048x2048 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1733.64             |
| colorTwistRGB48_NEON   | 10817.1             |
| colorTwistRGB24_C      | 353.98              |
| colorTwistRGB24_NEON2  | 5184.21             |



# Building

For Windows-on-ARM, run the following:

```
mkdir build
cmake .. -G "Visual Studio 16 2019" -A ARM64
cmake  --build . --config Release
```

For a 32-bit build, use

```
mkdir build
cmake .. -G "Visual Studio 16 2019" -A ARM
cmake  --build . --config Release
```

For x86-Windows, use for x64-build:

```
mkdir build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake  --build . --config Release
```

For x86-build, run

```
mkdir build
cmake .. -G "Visual Studio 16 2019" -A Win32
cmake  --build . --config Release
```

