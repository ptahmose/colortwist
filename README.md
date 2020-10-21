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

Intel Core i7-8700K @ 3.70GHz, msvc 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1226.24             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1720.98  |
| colorTwistRGB48_AVX    | 3291.02             |
| colorTwistRGB48_AVX2   | 4297.74             |
| colorTwistRGB48_AVX3   | 5175.55             |

Intel Core i7-8700K @ 3.70GHz, gcc 10.2.0, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1453.32             |
| colorTwistRGB48_AVX    | 3334.76             |
| colorTwistRGB48_AVX2   | 4315.26             |
| colorTwistRGB48_AVX3   | 5011.39             |


Intel Core i7-8700K @ 3.70GHz, cl 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1226.24             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1720.98  |
| colorTwistRGB48_AVX    | 3291.02             |
| colorTwistRGB48_AVX2   | 4297.74             |
| colorTwistRGB48_AVX3   | 5175.55             |

Raspberry Pi 4, gcc 8.3.0-6, 32bit : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 366.99              |
| colorTwistRGB48_NEON   | 410.42              |
| colorTwistRGB48_NEON2  | 940.71              |

AMD Ryzen 7 4700U, msvc 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 1202.02             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 1836.64  |
| colorTwistRGB48_AVX    | 3810.53             |
| colorTwistRGB48_AVX2   | 4970.00             |
| colorTwistRGB48_AVX3   | 5572.38             |

Intel m3, msvc 19.27.29112, x64 : 2048x2048 RGB48 bitmap

| version                | performance in MB/s |
| ---------------------- | ------------------- |
| colorTwistRGB48_C      | 650.32             |
| [colorTwistRGB48_IPP](https://software.intel.com/content/www/us/en/develop/documentation/ipp-dev-reference/top/volume-2-image-processing/image-color-conversion/color-twist.html)  | 935.62  |
| colorTwistRGB48_AVX    | 1769.30             |
| colorTwistRGB48_AVX2   | 2370.21             |
| colorTwistRGB48_AVX3   | 2943.29             |
