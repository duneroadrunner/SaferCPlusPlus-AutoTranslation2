Nov 2021

# SaferCPlusPlus-AutoTranslation2

Here we describe using the [scpptool](https://github.com/duneroadrunner/scpptool) tool to assist in the conversion of native C code to [SaferCPlusPlus](https://github.com/duneroadrunner/SaferCPlusPlus) (a memory-safe subset of C++).

To demonstrate how to use it, this repository contains an [example](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/tree/master/examples/lodepng) of the conversion tool being applied to an open source png encoder/decoder written in C.

Note that, by default, the conversion tool doesn't necessarily produce (performance) optimal SaferCPlusPlus code. Instead it uses SaferCPlusPlus elements that map directly to the (unsafe) native elements they are replacing. In fact, if you add the `-ConvertMode Dual` command line option, rather than being replaced with corresponding SaferCPlusPlus elements directly, the unsafe native elements are replaced with macros that allow you to use a compile-time directive to "disable" the SaferCPlusPlus elements and "restore" the original (unsafe) implementation. That is, adding `-DMSE_LEGACYHELPERS_DISABLED` to the compile options of the converted code, should make it essentially equivalent to the original (unsafe) code, generating the same, or nearly the same, machine code as the (unconverted) original code.
