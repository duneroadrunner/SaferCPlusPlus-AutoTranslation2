Feb 2021

This subdirectory contains versions of [LodePNG](https://github.com/lvandeve/lodepng), an open source png encoder/decoder, before and after conversion from C (and a little bit of C++) to [SaferCPlusPlus](https://github.com/duneroadrunner/SaferCPlusPlus). The conversion was done using the [scpptool](https://github.com/duneroadrunner/scpptool) tool.

The LodePNG project contains several example programs demonstrating the use of the library. Here we choose just one to convert. Conversion of the others would be similar. The example we'll use is contained in the example_sdl.cpp file. 

So here we'll explain the steps used to do the conversion. The first step is to make a copy of the project in a new directory. Next we verify that we can build the program with the compiler used by the conversion tool (scpptool). The version of scpptool we'll be using uses llvm8. The example uses libSDL so make sure you have it installed. (Otherwise you can use the example_decode.cpp example instead.) From the src/examples subdirectory where the example_sdl.cpp file is located we would execute a command like: 

    {the llvm8 directory}/bin/clang++ example_sdl.cpp ../lodepng.cpp -I.. -lSDL -std=c++17 -I'{the llvm8 directory}/lib/clang/8.0.0/include'

and hopefully get no errors. Now we can verify that the program we just compiled works:

    ./a.out ../../pngtest.png

The program should display a little example graphic.

Next we need to prepare the project for conversion. First we need to add a subdirectory named "msetl" containing the [SaferCPlusPlus](https://github.com/duneroadrunner/SaferCPlusPlus) header files to the src subdirectory of our LodePNG project. 

Then we need to go through the project source files and identify any elements in the code that should not be converted to a safe replacement. For example, our program uses the SDL library to display the graphics on the screen. But the SDL library has an interface that requires use of unsafe elements like raw pointers and the `SDL_Event` type. So we annotate the use of those elements with "check suppression" directives from the SaferCPlusPlus library. For example, line 55 in the example_sdl.cpp file:

```cpp
    SDL_Surface* scr = SDL_SetVideoMode(w / jump, h / jump, 32, SDL_HWSURFACE);
```

declares a technically unsafe raw pointer variable that would by default get converted to a safe pointer type. But the SDL interface necessitates the use of this (unsafe) raw pointer, so we'll add a "[check suppression](https://github.com/duneroadrunner/scpptool#local-suppression-of-the-checks)" prefix like so:

```cpp
    MSE_LH_SUPPRESS_CHECK_IN_XSCOPE SDL_Surface* scr = SDL_SetVideoMode(w / jump, h / jump, 32, SDL_HWSURFACE);
```

This will prevent the raw pointer declaration from being modified during the conversion process. It will also direct scpptool not to complain about the code being unsafe when used in its (default) static safety verification mode. The check suppression directive is an element provided by the SaferCPlusPlus library, so any file that uses it needs to include the necessary header file. So we add an include directive:

```cpp
    #include "mselegacyhelpers.h"
```

at the top of the file. There are a few other places in that file that also need check suppression annotations. You can see them all by looking for the check supression directives in the [converted file](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/master/examples/lodepng/lodepng_translated/src/examples/example_sdl.cpp). Missing any required annotations will result in compile errors in the converted code. But you can just add the missing annotations then.

So it turns out that example_sdl.cpp is the only file in our example needing such annotations. Next its time to do the conversion. Before executing a conversion we always make a(nother) backup copy of the source files in case we need to tweak the files and repeat the conversion (which is likely). The format of the conversion command is 

    {scpptool_executable_directory}/scpptool -ConvertToSCPP -ScopeTypePointerFunctionParameters {source files} -- {compiler options} -std=c++17 -I'{the llvm8 directory}/lib/clang/8.0.0/include'

So, from the source directory of the new copy of the project, we first convert the "SDL" example with a command like 

    {scpptool_executable_directory}/scpptool -ConvertToSCPP -ScopeTypePointerFunctionParameters lodepng.cpp example_sdl.cpp -- -I.. -I../msetl -lSDL -std=c++17 -I'{the llvm8 directory}/lib/clang/8.0.0/include'

This command will spew a whole bunch of stuff to the screen, including what may look like error messages. This is just the output of a "static analysis" pass that is executed as part of the conversion process. At the end you may get a message that says "merge: warning: conflicts during merge". While this indicates the possibility of an invalid conversion, its not abnormal and by default the conversion tool attempts to resolve "its merge conflicts" automatically (with a heuristic).

So this conversion command results in three files being modified - lodepng.cpp, example_sdl.cpp and the header file they include, lodepng.h. The converted files now require two additional compiler options to build: `-DMSE_SCOPEPOINTER_DISABLED` and `-DMSE_CHAR_STAR_EXEMPTED`. `-DMSE_SCOPEPOINTER_DISABLED` directs the SaferCPlusPlus library to just use raw pointers as scope pointers. This may make it easier to interface the converted code with other (possibly unsafe) code not using the SaferCPlusPlus library. At the time of writing, the converter does not convert `char*`s as they are assumed to be null-terminated strings and safe, compatible counterparts for the C standard library string functions have not been implemented yet. `-DMSE_CHAR_STAR_EXEMPTED` directs the SaferCPlusPlus library to tolerate `char*`s even in situations where they aren't verified to be safe.

So the new compile command looks like:

    {the llvm8 directory}/bin/clang++ example_sdl.cpp ../lodepng.cpp -I.. -I../msetl -DMSE_SCOPEPOINTER_DISABLED -DMSE_CHAR_STAR_EXEMPTED -lSDL -std=c++17 -I'{the llvm8 directory}/lib/clang/8.0.0/include'

But attempting to compile the converted source files results in some compile errors. This is because there are a few items that the converter tool is unable to properly address. These items need to be addressed manually. One of those items is the use of `void*`. For example, line 144 of the lodepng.cpp file:

```cpp
    static void uivector_cleanup(void* p)
```

uses `void*` as the parameter type. Fortunately in this case the type doesn't actually need to be `void*`, and we can go back to our backup copy and just replace the `void` part with the actual used type. So we change the line to

```cpp
    static void uivector_cleanup(uivector* p)
```

The other uses of `void*` are similar. The another thing not addressed by the converter is certain "hard" casts. Line 146 of the lodepng.cpp file:

```cpp
      ((uivector*)p)->size = ((uivector*)p)->allocsize = 0;
```

contains hard casts of pointer paramter `p`. It turns out that this is just because `p` was originally declared as a `void*`. But we just changed that, so this hard cast is now redundant, so we can just get rid of it and the other similar ones.

So now (after, as always, making a(nother) backup) we can execute the conversion and try to compile again. But we still get one last compile error. A "no matching function for call" error that occurs around lines 112 and 113 of the converted example_sdl.cpp:

```cpp
    unsigned int w = 0/*auto-generated init val*/; unsigned int h = 0/*auto-generated init val*/;
    unsigned error = lodepng::decode(image, w, h, buffer); //decode the png
```

We can look at the declaration of the `lodepng::decode()` function on line 214 of the converted lodepng.h file: 

```cpp
    unsigned decode(mse::mstd::vector<unsigned char>&  out, mse::TRegisteredObj<unsigned int >&  w, mse::TRegisteredObj<unsigned int >&  h, const mse::mstd::vector<unsigned char>&  in, LodePNGColorType colortype = LCT_RGBA, unsigned bitdepth = 8);
```

where we see that the `w` and `h` parameters are of type `mse::TRegisteredObj<unsigned int >&` where the `w` and `h` arguments passed are (still) of type `unsigned int`. It's a shortcoming of the converter that these arguments weren't automatically converted to the type of the reference parameters.

The reason for the shortcoming is that the conversion is (currently) done one "translation unit" (i.e. source file) at a time, but the lodepng.h header file is included in more than one source file. When converting the lodepng.cpp source file, the converter determines that, for safety reasons, the parameters need to be converted from `unsigned int&` to `mse::TRegisteredObj<unsigned int >&`. But when converting the example_sdl.cpp source file, the converter doesn't have visibility into (the safety issue in) the lodepng.cpp source file and doesn't see any compelling safety reason in its own source file to require the conversion. Note that in this case it's only an issue because of the (non-const native) reference, which is a C++ construct. If the source had been pure C code, presumably those references would have been pointers and there would have been no issue.

So anyway, we can just manually change the declaration of the argument variables on line 112 of the converted example_sdl.cpp file to match the declaration of the function parameters like so:

```cpp
    mse::TRegisteredObj<unsigned int > w = 0/*auto-generated init val*/; mse::TRegisteredObj<unsigned int > h = 0/*auto-generated init val*/;
    unsigned error = lodepng::decode(image, w, h, buffer); //decode the png
```

Now the code should compile and work. 


