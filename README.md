# colortwist

This is a little finger exercise in SIMD-optimization. The example we try to optimize is an image processing operation known as "colortwist". It is a matrix-multication of an RGB-triple and a given matrix.
So we have 

<img src="https://render.githubusercontent.com/render/math?math=R = t_11 * r  %2B t_12 * g  %2B t_13 * b  %2B t_14">
<img src="https://render.githubusercontent.com/render/math?math=G = t_21 * r  %2B t_22 * g  %2B t_23 * b  %2B t_24">
<img src="https://render.githubusercontent.com/render/math?math=B = t_31 * r  %2B t_32 * g  %2B t_33 * b  %2B t_34">