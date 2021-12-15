As you have probably noticed, the entire brush folder is gone. I have rewritten everything from scratch in a single file Brush.hxx because the provided code template was unnecessarily convoluted and unwieldy to use.

The code compiles on my laptop (win10 + mingw-GCC11) and department machines (GCC10). It however might not compile with the default MacOS development setup because Apple ships their own "crippled" Clang which has not caught up with newer C++ standards.

If you have a problem understanding certain C++ constructs in my code, please consider referring to:
https://en.cppreference.com/w/cpp/language/constraints#Requires_expressions
https://en.cppreference.com/w/cpp/language/if#Constexpr_if
https://en.cppreference.com/w/cpp/language/function_template#Abbreviated_function_template
https://en.cppreference.com/w/cpp/language/structured_binding
