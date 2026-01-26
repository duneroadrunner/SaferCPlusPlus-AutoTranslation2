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

Note that a number of `struct` definitions near the beginning of *main.c* remain untranslated due to their participation in the [declaration of `union` members](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/master/README.md#unions).

