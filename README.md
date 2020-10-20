# colortwist

This is a little finger exercise in SIMD-optimization. The example we try to optimize is an image processing operation known as "colortwist". It is a matrix-multication of an RGB-triple and a given matrix.
So, if we have the source pixel values r, g, b, then the destination pixels R, G, B are calculated as 

<img src="https://render.githubusercontent.com/render/math?math=R = t_11 \cdot r  %2B t_12 \cdot g  %2B t_13 \cdot b  %2B t_14">
<img src="https://render.githubusercontent.com/render/math?math=G = t_21  \cdot r %2B t_22 \cdot g  %2B t_23 \cdot b  %2B t_24">
<img src="https://render.githubusercontent.com/render/math?math=B = t_31  \cdot r %2B t_32 \cdot g  %2B t_33 \cdot b  %2B t_34">

where 

<img src="https://render.githubusercontent.com/render/math?math=T=\begin{pmatrix}t_11 %26 t_12 %26 t_13 %26 t_14\\t_21 %26 t_22 %26 t_23 %26 t_24\\t_31 %26 t_32 %26 t_33 %26 t_34 \end{pmatrix} ">

is the color twist matrix.