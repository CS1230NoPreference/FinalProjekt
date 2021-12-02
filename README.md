# CSCI1230 Final Project: Fractal Generation

Team: No Preference

Members: Nathan Plano, Nick Huang, Richard Tang (rtang26)

We want to delver deeper into the world of fractals! Using ray-marching
for rendering, we plan on procedurally generating shapes from 3D
Mandelbulbs to large-scale fractal generated terrains.

## High Level Plan

We plan on implementing our project in three steps:

-   Raymarching with C++ code on a CPU
    -   If enough time, we may attempt a migration to the GPU
-   Generate 3D Mandelbulb Fractals
-   Generate Fractal Terrains

Our final UI layout will resemble something like this: ![UI
Layout](UILayout.png "UI Layout")

## Technical Details

Our project will consist of four parts, the first three of which are
strictly graphics related:

### Ray Marching

Ray marching will be used to render different scenes based on their
different distance fields. We will implement a CPU ray marcher that will
be used to render different fractal scenes. This ray marcher will use
inspiration from the ray marching lab. However, it will be on the CPU
instead of the GPU (we may attempt a GPU raymarcher if we meet all the
other goals). We would allow the user to toggle certain parameters of
the ray marcher, such as soft/harsh shadows.

References:

-   [Raymarching
    Lab](https://github.com/cs123tas/labs/blob/master/lab10/lab10.md)

### Mandelbulb Fractals

Once we’ve successfully implemented a ray marcher and tested on basic
shapes (sphere, cylinder, etc), we will generate Mandelbulb fractals by
changing the ray marcher’s distance estimator to a Mandelbulb DE
(pseudocode in the blog post, and the two example Shadertoy
implementations). Ideally, we would allow the user to customize the
Mandelbulb based on certain parameters (TODO: determine which).

References:

-   [Distance Estimated 3D
    Fractals](http://blog.hvidtfeldts.net/index.php/2011/09/distance-estimated-3d-fractals-v-the-mandelbulb-different-de-approximations/)
-   [Shadertoy Mandelbulb #1](https://www.shadertoy.com/view/WsySDR)
-   [Shadertoy Mandelbulb #2](http://shadertoy.com/view/Mdy3Dw)

### Fractal Terrains

Finally, we will implement the fractal terrains in a manner similar to
the Mandelbulb fractals; the primary complication will be determining
which distance estimator to use with the ray marcher. Like the
Mandelbulb, ideally we would allow users to customize the scene/terrain
based on certain parameters. Additionally, we may modify the ray marcher
to allow for more complicated scenes (note in the attached Shadertoy
that the raymarcher code contains scene-specific aspects, such as tree
generation?) \[TODO: determine fractal terrain DEs\]

References:

-   [Fractal Terrain
    Raymarching](https://www.iquilezles.org/www/articles/terrainmarching/terrainmarching.htm)
-   [Fractal Terrain
    Noise](https://www.iquilezles.org/www/articles/morenoise/morenoise.htm)
-   [Shadertoy Rainforest](https://www.shadertoy.com/view/4ttSWf)

### Video Generation

With both of our fractal generations, the ray-marched images have great
potential to be dynamic; that is, given the nature of fractals, they
evolve over time. Using tools like ffmpeg, we would give users the
ability to generate a variable-length video of a scene, and watch it
grow.

## Division of Labor

TBD
