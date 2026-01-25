# safe(r) unsafehttp

[unsafehttp](https://github.com/GSGBen/unsafehttp) seems to have been an unapologetically unsafe toy http server written in C. This subdirectory contains the results of (unapologetically) making it somewhat safer by auto-translating the source to the SaferCPlusPlus safe subset of C++. 

The `src` subdirectory contains the original C source files. The `src_with_const_correctness_fixes` subdirectory contains a copy of those source files with a couple of const-correctness violations (that are allowed by the C compiler but not the C++ compiler) fixed. `at_src_converted_to_valid_cpp` contains the results of using scpptool to auto-convert the C code to a subset of C that will also compile as C++. 

And finally, the `at_src_converted_to_safe_subset` subdirectory contains the results of using scpptool to auto-convert the code to an essentially memory-safe subset of C++.

With only two source files, the scpptool command (executed from the directory where the source files are located) to convert the (const-correctness-fixed) code so that it will compile as C++ is just:

    {scpptool src directory}/scpptool -ConvertC2ValidCpp main.c ht.c -- -I.
 
And the scpptool command to translate the (now valid C++) code to the safe subset of C++ is:

    {scpptool src directory}/scpptool -ConvertToSCPP main.c ht.c -- -I. -x c++

We use the `-x c++` compiler option because the source files have `.c` filename extensions (which is not inappropriate when the command is invoked as at that point they are still valid C), but we want them to be interpreted as C++. 

The resulting code can be compiled with:

    clang++ -I. -I./msetl -std=c++20 -g -x c++ main.c ht.c

where `./msetl` is the directory (that needs to be added) that contains the SaferCPlusPlus library header files. At this point, rather than using the `-x c++` option, it'd probably be more appropriate to change the file extensions from `.c` to `.cpp`. (At the time of writing, the scpptool does not do it for you.) 

Then, to run the http server:

    ./a.out --content-path ../content --port 8080

Then connect to the server at `http://localhost:8080`.

### Beware `union`s and the posix socket API

This example actually serves to highlight the distinction between C code practices that simply lack safety enforcement, and those that are *intrinsically* unsafe.

So a bunch of the code that interacts with the posix socket API actually remains unconverted to the safe subset. (And because this web server is so minimal, that ends up being a not-insignificant portion of the whole code base.) The posix socket API is intrinsically unsafe in a kind of interesting and rather egregious way in that it involves the code using it to engage in unsafe type-punning via C `union`s.

`union`s, somewhat uniquely, propagate their unsafety "virally". That is, it's not just that the `union` itself is intrinsically unsafe (and so excluded from translation to the safe subset), but it also results in the definition of any type that is used in any member of any union to also be excluded from translation to the safe subset, even if the type would otherwise be eligible.

The scpptool auto-translator assumes that any `union` might potentially be used for type-punning, which is intrinsically unsafe. It's important to avoid changing the bit-representation of any element involved in type-punning. But this means that even if an object itself does not participate in any `union`, if (any part of) its type is involved in (any part of) the type of any member of any union, then the object's type may remain unsafe even after the auto-translation.

Now, if you have `union`s in your code that you know are not used for type-punning, then you can make them eligible for conversion to the safe subset by simply replacing the `union` keyword with `struct`. (It might be a little less memory-efficient, but if you really are not doing any sort of type-punning, then the behavior should remain the same, right?) But alas, like we said, the `union`s in this project involved with the posix socket API, are engaged in type-punning as part of that API.


