Jan 2026

This subdirectory contains some files needed for the conversion of wget version 1.25.0 to the SaferCPlusPlus safe subset of C++. The conversion is done using the [scpptool](https://github.com/duneroadrunner/scpptool). 

First, let's note that in the case of wget (and presumably it would be the same for the other GNU utility programs), the auto-converted C++ code will compile under clang++ but not g++. It turns out that the GNU utilities are available on so many platforms, not because the code is portable, but rather because the code has been *ported* to (and maintained for) so many platforms. This is reflected in the sources being riddled with a lot of code contained within preprocessor directives like `#if defined __clang__`, or `#if __GLIBC__ + (__GLIBC_MINOR__ >= 16) > 2`, or whatnot.

scpptool's auto-translation features rely on the clang API, and only consider elements in the AST (abstract syntax tree) that the clang API provides. So any code that is exclusive to platforms other than clang (or the version and configuration of clang used by scpptool), will not be auto-translated.

A related problem we ran into was the fact that while we could build the original wget source just fine when compiling as it C code, the sources have some code contained within `#ifdef __cplusplus` preprocessor directives, and as far as we can tell, some of that code is just broken, preventing successful compilation as C++. At least with the build/compiler configuration we were using. So we ended up just commenting out some of that code by hand.

Also note that wget (and presumably the other GNU utility programs), seem to replace significant portions of the C standard library with their own version. Much of this code is platform-specific. So for various reasons it's a bit problematic and somewhat pointless to convert this part of the code, so we generally leave it as is, or not use it all and just use the actual standard library elements themselves. Note that when calling (the actual) standard library functions, often the auto-converter will substitute the call with one that invokes a safe implementation of the function provided by the SaferCPlusPlus library. 

### One line that required hand modification

The one line of actual working code that we had to modify by hand was line 222 of `src/wget.h`:

    memset (basevar_new + sizevar_old * sizeof (type), 0, (DR_newsize - sizevar_old) * sizeof (type)); \

This line occurs in the definition body of a macro that starts with:

    #define DO_REALLOC(basevar, sizevar, needed_size, type) do {    \

The problem is that `basevar_new` is of type `void*` and has pointer arithmetic applied to it. The first issue is that C++ doesn't allow pointer arithmetic on `void*` pointers. When the C-to-C++ auto-converter encounters `void*` pointer arithmetic, by default it adds a cast to `char*`. And when subsequently converted to the memory-safe subset of C++, the `void*`s and `char*`s will be converted to their corresponding (safe) smart pointers. But the cast from the smart version of `void*` to the smart version of `char*` will throw a run-time exception if the value stored in the smart version of `void*` didn't actually come from a smart version of `char*`.

And unfortunately, this is sometimes the case in wget. So instead of (sometimes incorrectly) assuming that the `void` pointer value is actually a `char` pointer value, we hand-modified the original code to explicitly cast the void pointer to the correct type. So we change the `memset()` line to:

    memset (((BASEVAR_NEW_CAST_TYPE_FOR_POINTER_ARITHMETIC(basevar))basevar_new) + sizevar_old * sizeof (type) / BASEVAR_NEW_TARGET_SIZE_FOR_POINTER_ARITHMETIC(basevar), 0, (DR_newsize - sizevar_old) * sizeof (type)); \

where `basevar` is the first macro argument and corresponds to a pointer variable that has the same type as the intrinsic type of the value stored in the `basevar_new` `void` pointer, and `BASEVAR_NEW_CAST_TYPE_FOR_POINTER_ARITHMETIC()` and `BASEVAR_NEW_TARGET_SIZE_FOR_POINTER_ARITHMETIC()` are just helper macros that provide the appropriate cast type and target object size.

### Exposed wget bugs

Notably, the auto-translation process exposed a "benign-by-luck" buffer overflow bug that is allowed by the C compiler, but had to be fixed in order to compile as C++. (By hand-modifying wget's `src/init.h` file.) It was related to the fact that when initializing a (native) `char` array with a string literal, C++ requires that the array be large enough to hold the entire string including the null terminator, while C doesn't mind if the null terminator doesn't fit.

It also exposed at least one `const`-correctness violation, laundered through `void *` resulting in a run-time exception. (Which can be triggered by specifying wget options to mirror a specific subdirectory like so: `./at_wget --mirror --no-parent --include-directories=/example_subdirectory https://example.com/`.) Specifically, a `void *`  that was assigned a `char const***` value is subsequently cast to a `char ***`. (More specifically, the `place` parameter of the `cmd_directory_vector()` function that was previously assigned the address of the `includes` field of a `struct options`, is cast to a `char ***` at the beginning of that function.) The auto-translated code does not allow such `const`-correctness violations.

### Instructions for carrying out the auto-translation

These are the steps used to execute the auto-translation on an Ubuntu (24.04 x64) system (though it should be possible to get it to work on any system in which wget and clang are supported):

- create a directory for the project
- `sudo apt update`
- `sudo apt upgrade`
- `sudo apt install clang`
- `sudo apt install rcs` (for the system merge command)
- download the [SaferCPlusPlus-AutoTranslation2 zip file](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/archive/refs/heads/master.zip) and extract it
- open a terminal with `SaferCPlusPlus-AutoTranslation2-master/examples/wget-1.25.0` as the current directory
- in that directory there should be a wget-1.25.0 tarball, which is just a copy of the one from the [wget site](https://ftp.gnu.org/gnu/wget/)
- extract that tarball then execute `\cp -a -f ./hand_modified/wget-1.25.0/. ./wget-1.25.0/` (make sure not to omit any periods in that copy command)
	- this will overwrite a few of the wget source files with our "hand-modified" versions
	- make a `wget-build` subdirectory (in the `examples/wget-1.25.0` directory) that is sibling to the (nested) `wget-1.25.0` and `hand_modified` directories
	- install wget prerequisites
		- `sudo apt install libgnutls28-dev`
		- `sudo apt install libssl-dev`
	- from the `wget-build` directory, run `../wget-1.25.0/configure --disable-iri --with-ssl=openssl` (the idn2 library that iri depends on doesn't seem to work in clang++ due to the placement of __attribute__() in an unsupported position)
	- then run `make` (from the `wget-build` directory)
	- the produced (unsafe) wget executable should be in the `wget-1.25.0/src` directory
	- delete (or rename) the (generated) `string.h` and `fcntl.h` files from the `wget-build/lib` directory. (These files seem to be alternative implementations of standard headers but seem to cause errors when compiled as C++ due to conflicting exception specifiers.)
- download the [SaferCPlusPlus zip file](https://github.com/duneroadrunner/SaferCPlusPlus/archive/refs/heads/master.zip) to the `examples/wget-1.25.0` directory and extract it
	- copy the `include` subdirectory to the `examples/wget-1.25.0` directory and rename it to `msetl`
- download the [scpptool zip file](https://github.com/duneroadrunner/scpptool/archive/refs/heads/master.zip) to the `examples/wget-1.25.0` directory and extract it
	- build scpptool by following the [build instructions](https://github.com/duneroadrunner/scpptool#how-to-build)
- note: The next two scripts will invoke the scpptool conversion features which will attempt to modify in-place all specified source files *and any directly or indirectly included files*. So you generally want to be careful not to invoke the scpptool conversion features with root privileges or with write permission to any installed libraries to ensure that scpptool doesn't inadvertently modify system or installed library header files.
- from the `examples/wget-1.25.0` directory, run `at_c2validcpp_all1.sh` 
- from the `examples/wget-1.25.0` directory, run `at_convert2scpp_all1.sh` 
- from the `examples/wget-1.25.0` directory, run `at_compile_and_link1.sh` 
- the generated "safe" executable will be named `at_wget`

- you might test it with something like the following example: 

    ./at_wget https://www.wikipedia.org/portal/wikipedia.org/assets/img/Wikipedia-logo-v2.png

The following are rather boring videos of the above instructions being carried out in real- and elapsed- time. There probably isn't any reason to watch them unless you are looking for some visual clarification of the instructions.

part 1:

https://github.com/user-attachments/assets/088f1661-0773-4f2a-a1fd-3f6391b5aa68

part 2:

https://github.com/user-attachments/assets/f90480a2-a460-4b06-b863-12e5354a7568

part 3:

https://github.com/user-attachments/assets/b15719fb-722a-47b3-a1fe-ea0ef348fdfd


Note that, not having a very good understanding of the existing wget build process, the conversion and "compile_and_link" scripts were created in rudimentary fashion by simply looking for source files referred to in the existing makefile and using a process of trial-and-error to prune those files that didn't seem to be necessary (at least on our platform). All this to say that, while they serve to demonstrate simply how the auto-conversion can be done, they don't necessarily serve as a recommendation for how the build system should end up. 

The `at_c2validcpp_all1.sh` script calls another script which invokes the scpptool feature that converts (1990s-style) C source that doesn't qualify as valid C++ to a subset of C that (hopefully) compiles as C++. The actual scpptool command used is:

    ./scpptool-master/src/scpptool -ConvertC2ValidCpp -SuppressPrompts $@ -- -DGNULIB_NAMESPACE=gnulib -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"./wget-build/wgetrc\" -DLOCALEDIR=\"./wget-build/share/locale\" -I. -I./wget-1.25.0/src  -I./wget-1.25.0/lib  -I./wget-build/src  -I./wget-build/lib      -DHAVE_LIBSSL -DHAVE_STRLCPY -DO_BINARY=0 -DO_TEXT=0 -DO_SEARCH=O_RDONLY -c > scppt_conv_out1.txt

The options after the `--` double-dashes are just the wget-specific compiler options. 

And the `at_convert2scpp_all1.sh` script calls another script which invokes the scpptool feature that converts C++ source files to the (essentially) memory-safe subset of C++. The actual scpptool command used here is:

    ./scpptool-master/src/scpptool -ConvertToSCPP -AddPrecedingIncludeConfigDotHDirective -ModifiablePath ./wget-1.25.0/src -ModifiablePath ./wget-build/src/version.c -SuppressPrompts $@ -- -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"./wget-build/wgetrc\" -DLOCALEDIR=\"./wget-build/share/locale\" -I. -I./wget-1.25.0/src -I./msetl  -I./wget-1.25.0/lib  -I./wget-build/src  -I./wget-build/lib      -DHAVE_LIBSSL -DHAVE_STRLCPY -DO_BINARY=0 -DO_TEXT=0 -DO_SEARCH=O_RDONLY -std=c++23 -c -x c++ > scppt_conv_out1.txt

Again, the options after the `--` double-dashes are mostly the wget-specific compiler options. The `-x c++` is used because the source files still have their original `.c` file extensions, but we want them to be interpreted as C++ files as the `-ConvertToSCPP` option only supports converting from valid C++ source.

The `-AddPrecedingIncludeConfigDotHDirective` option indicates that `#include "config.h"` should be inserted before any other include directives, including the include directives for the SaferCPlusPlus library. It's (potentially) needed for wget and other gnu utilities because they use (the generated, platform-specific) `config.h` to replace standard system functions and interfaces with their own versions.

Also note that the second `-ModifiablePath` option is used to specify just a single file.

The `$@` is just the bash symbol that represents all of the arguments passed to the script. In this case it will be all of the source filenames. Unlike the C++ compiler, which can generally be used to compile the source files one at a time before linking, using scpptool to try to convert the source files one at a time generally won't work. Or to be more precise, any files that share one or more common include files generally need to be specified together in one conversion operation. 

### Conversion and build times

A the time of writing, the conversion process takes a rather long time. (On the order of double digit minutes.) And compiling and linking the converted code isn't fast either. At the current stage of development, not much attention has yet been directed at conversion or build times, so there is presumably plenty of room for improvement. With respect to the conversion, it shouldn't be an inherently slow process, but in our particular implementation, there is an issue where, in the case of nested macro invocations, the clang library we use does not seem to reliably report all of the nested macros that an element given as a macro argument participates in. So, for the moment at least, we resort to an (unoptimized) strategy for finding any unreported nested macros that adds cost to the processing of any element in a macro, and increasing cost correlated to the macro nesting depth. 

It turns out that wget uses a lot of nested macros, but in a very uneven distribution among the source files, which can be observed in the highly varying conversion times of different source files of comparable size. In particular the `src\utils.c` file uses a some very nested macros in its `number_to_string()` function, and that file alone makes up a good chunk of the total conversion time. Also, in our conversion scripts, after each file is converted it is then compiled as a test to make sure there are no errors. This might help to more quickly diagnose any conversion issues, but it's technically not necessary and could be dispensed with if, for example, one wanted to include the auto-translation as part of a regular build process.

While currently not as slow as the conversion process, compiling the converted code will take significantly longer than the pre-converted code. Part of it is simply that the safe replacement elements use a lot of SFINAE meta-programming in their implementation (in order to maintain compatibility back to C++14). But in the case of converted legacy code, a bigger factor is probably the conversion of void pointers. The safe subset of C++ being converted to enforces type safety. So the element we use to replace void pointers (namely, `lh::void_star_replacement`), has to also hold run-time type information about the value it holds in order to ensure that any casts from that value are safe and valid. But certain values can be validly cast to a number of different types. For example, if the void pointer holds an `int *` value, it can also be validly cast to an `int const*`. Or an `int const* const`. 

But the conversion can replace a pointer type with any number of replacement types, depending on how the pointer is used. For example it may use a different (smart pointer) type depending on whether or not the pointer is used as an array or buffer iterator. Also depending on whether the pointer points to heap-allocated memory, stack-allocated memory or potentially either/both. And these are not the only criteria. This results in a "combinatorial explosion" of the number of types an `lh::void_star_replacement` value could potentially be validly cast to. And the library generates (at compile-time) a code path to handle each possible valid cast. This results in the void pointer replacement elements incurring a modest run-time cost and a comparatively more substantial compile-time cost. "Modernization" of the converted code could eliminate these costs where run-time polymorphism (of void pointers) is replaced with compile-time polymorphism (of templates or whatever).

