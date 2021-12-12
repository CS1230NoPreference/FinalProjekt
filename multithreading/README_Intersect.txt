The code is structured in a mostly (non-pure) functional way, similarly to Shape and Brush. I have explained in Brush and Shape that such design results in concise and highly flexible code, and therefore preferred over traditional OOP design. The code of the entire Intersect assignment is self-contained in a single file: Ray.hxx

There’re 4 major sections of the code, ViewPlane, Illuminations, Ray, ImplicitFunctions, each has been grouped into their own namespace.

ViewPlane::ConfigureProjectorFromScreenSpaceIntoWorldSpace is a higher order function, it takes a camera object, canvas height and width as arguments, and returns a projector function that projects any (x, y) coordinates on the view plane into the world space.

Illuminations::Diffuse is the only lighting function used for this assignment (ambient light is trivial, and therefore does not have a dedicated function). It is invoked by Canvas2D::renderImage::IlluminationModel which describes the simplified illumination model in the handout.

Ray::Trace is the main raytracing function, it takes an implicit function, the object transformation matrix, an eye point, a ray direction vector and an illumination model as arguments, and returns a tuple of { t, raytraced color }. The implicit function accepts an object space eye point, object space ray direction, and returns a tuple of { t, object space surface normal }, t == Ray::NoIntersection indicates no intersection. The illumination model is any (potentially stateful) callable object that takes a surface position and surface normal as arguments and returns the lit color.

The rest of the code (implicit functions) is pretty self-explanatory and you should be able to understand at first glance. If you have a problem understanding certain C++ constructs in my code, or if the code does not compile for you, please consider referring to the Brush and Shape readme.

Compiling in release mode is HIGHLY RECOMMENDED! It turns on -O3 optimization which results in hundreds of times faster rendering.

