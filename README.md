# colortwist

This is a little finger exercise in SIMD-optimization. The example we try to optimize is an image processing operation known as "color twist". It is a matrix-multication of an RGB-triple and a given matrix.
So, if we have the source pixel values r, g, b, then the destination pixels R, G, B are calculated as 

<img src="https://render.githubusercontent.com/render/math?math=R = t_11 \cdot r %2B t_12 \cdot g %2B t_13 \cdot b %2B t_14">
<img src="https://render.githubusercontent.com/render/math?math=G = t_21 \cdot r %2B t_22 \cdot g %2B t_23 \cdot b %2B t_24">
<img src="https://render.githubusercontent.com/render/math?math=B = t_31 \cdot r %2B t_32 \cdot g %2B t_33 \cdot b %2B t_34">

where 

<img src="https://render.githubusercontent.com/render/math?math=T=\begin{pmatrix}t_11 %26 t_12 %26 t_13 %26 t_14\\t_21 %26 t_22 %26 t_23 %26 t_24\\t_31 %26 t_32 %26 t_33 %26 t_34 \end{pmatrix} ">

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

Raspberry Pi 3b, gcc 8.3.0-6, 32bit : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 181.37              |
| colorTwistRGB48_NEON   | 170.11              |
| colorTwistRGB48_NEON2  | 347.69              |
| colorTwistRGB24_C      | 48.45               |
| colorTwistRGB24_NEON2  | 222.63              |

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

