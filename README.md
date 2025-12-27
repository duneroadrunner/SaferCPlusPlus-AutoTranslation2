Dec 2025

# SaferCPlusPlus-AutoTranslation2

Here we describe using the [scpptool](https://github.com/duneroadrunner/scpptool) tool to assist in the conversion of native C code to [SaferCPlusPlus](https://github.com/duneroadrunner/SaferCPlusPlus) (a memory-safe subset of C++).

To demonstrate how to use it, this repository contains an [example](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/tree/master/examples/lodepng) of the conversion tool being applied to an open source png encoder/decoder written in C.

Note that, by default, the conversion tool doesn't necessarily produce (performance) optimal SaferCPlusPlus code. Instead it uses SaferCPlusPlus elements that map directly to the (unsafe) native elements they are replacing. In fact, if you add the `-ConvertMode Dual` command line option, rather than being replaced with corresponding SaferCPlusPlus elements directly, the unsafe native elements are replaced with macros that allow you to use a compile-time directive to "disable" the SaferCPlusPlus elements and "restore" the original (unsafe) implementation. That is, adding `-DMSE_LEGACYHELPERS_DISABLED` to the compile options of the converted code, should make it essentially equivalent to the original (unsafe) code, generating the same, or nearly the same, machine code as the (unconverted) original code.

While performance-optimal "idiomatic" SaferCPlusPlus code relies significantly on static analysis and enforcement of compile-time restrictions to achieve high-performance memory-safe code, this auto-conversion feature produces code that instead relies mainly on run-time mechanisms to enforce its memory safety. And while the SaferCPlusPlus library provides memory-safe implementations of (the most commonly used) C++ standard library elements, the interface and behavior of those elements don't map precisely to corresponding traditional C elements. For example, `std::array<>` (and its corresponding safe implementation in the SaferCPlusPlus library, `mse::mstd::array<>`), cannot really act as a direct drop-in replacement for native C arrays as, for example, they don't mimic the array-to-poiner decay behavior of the latter.

So the SaferCPlusPlus library provides another set of memory-safe elements, in the `mse::lh` namespace, that are intentended to map more directly to the behavior of their corresponding (unsafe) C elements. For direct replacement of native C arrays, the library provides `lh::TNativeArrayReplacement<>` which, for example, coordinates with the other safe replacement elements to emulate array-to-pointer decay behavior. 

### Conversion of pointers

But of course the main task in the effort to safen C code is to address pointers and the standard memory allocation and deallocation functions. This auto-conversion tool replaces native pointers with non-owning "smart" pointers and iterators. The SaferCPlusPlus library has idiomatic non-owning smart pointers and iterators, but again, their behavior does not quite match that of native pointers in precise detail, so the library provides another set of more compatible non-owning smart pointers and iterators. 

The tool uses "whole translation unit" analysis to determine whether or not a pointer is being used as an array/buffer iterator and will replace it with a smart pointer or iterator as appropriate. 

It also determines whether a pointer is used to target heap-allocated items (only), or non-heap-allocated items (i.e. automatic, global or static variables), or potentially either at various times. Pointers that target only heap items are replaced with a smart pointer or iterator whose safety mechanism is optimized for that usage scenario. 

The analysis also determines whether variables are ever (potentially) the target of a pointer. If so, the type is wrapped (in the declaration) with the `mse::TRegisteredObj<>` "transparent" template, which adds a mechanism that allows the object to cooperate with (smart) pointers targeting it to ensure that those smart pointers are aware when/if they become dangling. The `mse::TRegisteredObj<>` wrapper also overrides the object's `&` (addressof) operator to yield an appropriate smart pointer.

Pointers that are determined to (potentially) target either heap-allocated items or non-heap-allocated at various times are replaced with a sort of "polymorphic type-erased" pointer or iterator that is essentially a pointer/iterator version of `std::any`. That is, it will faithfully store the value of whatever pointer is assigned to it, retaining the source value in its original form and type, thus preserving any safety mechanism the source has.

In exchange for this degree of generality, these polymorphic "any" pointers and iterators incur the cost of an additional level of indirection.

#### table of pointer/iterator types that result from auto-conversion of a native pointer
type 									| description 
----------------------------------------|------------------------------------------------------------------
`mse::TRefCountingPointer<>` 			| (shared-owning) pointer to heap-allocated item
`mse::TRegisteredPointer<>` 			| (non-owning) pointer (usually) to non-heap-allocated item
`mse::lh::TLHNullableAnyPointer<>` 		| pointer to either heap- or non-heap-allocated item
`mse::lh::TStrongVectorIterator<>` 		| iterator to heap-allocated buffer
`mse::lh::TNativeArrayReplacement<>` 	| native array replacement that decays to a (smart) iterator or pointer
`mse::lh::TLHNullableAnyIterator<>` 	| iterator to either heap- or non-heap-allocated buffer or array

(The names of these "smart" replacement pointer/iterator types may strike some as egregiously verbose. This may be kind of semi-intentional, as an indication the solution is not yet complete or polished enough to be appropriate for the casual user (for whom memory safety might not be the utmost priority). It is expected that ultimately more concise aliases for these elements will be used.)

As for `malloc()` and company, legacy "untyped" memory allocation is not really compatible with the type of safety we're trying to achieve. So the tool's code analysis will (attempt to) determine the type of the pointer(/iterator) that any allocation is ultimately assigned to and, when necessary, provide that type to the safe replacement allocation function.

#### table of allocation functions that result from auto-conversion
alloc/dealloc function 								| description 
----------------------------------------------------|------------------------------------------------------------------
`mse::lh::allocate<type>(void)` 					| heap-allocate object of specified type
`mse::lh::allocate_dyn_array1<type>(num_bytes)` 	| heap-allocate buffer of specified type and size
`mse::lh::reallocate<>(iter, num_bytes)` 			| realloc (with type deduced from iterator argument)
`mse::lh::free<>(ptr)` 								| free (pointer or iterator)

These allocation functions generally just call the library's safe equivalent of `std::make_shared<>()`. If the allocated item is a buffer, then the returned iterator will be a shared owner of the library's safe equivalent of an `std::vector<>`. The reallocation function basically just calls the underlying vector's `resize()` method. The `lh::free(ptr)` function takes its argument by non-`const` reference, and generally just sets the argument to null, which may or may not immediately deallocate the item depending on whether there are still any other shared owners of the item out there.

### Type-safe `void*`

Our version of memory-safety includes type-safety. As such, we have to deal with the ubiquitous use of `void*` in legacy C codebases. `void*`s generally get converted to `mse::lh::void_star_replacement`. `mse::lh::void_star_replacement`, somewhat like `mse::lh::TLHNullableAnyPointer<>`, is basically an `std::any` that is restricted to holding pointers/iterators. But `mse::lh::void_star_replacement` has an `operator T const()` member cast operator defined. That cast operator will (at run-time) check to make sure that target cast type is compatible with the type of the stored value.

### Interfacing with unsafe APIs

For each potentially unsafe code element, the auto-converter makes an assesment as to whether the element is eligible to be converted to a safe counterpart (or, indeed, modified at all). The assessment may be based on any of several criteria, including whether the code is explicitly designated as ineligible for conversion, or implicitly judged as such based on its location or status as part of a "system" interface. 

In order to maximize the amount of code that can be converted, the auto-converter will inject (unsafe) conversion code to allow for interactions between safe and unsafe elements. These (legacy-compatibility/"FFI") conversions are readily identifiable as "unsafe" operations, often just by virtue of being instigated by elements in the `mse::us` namespace. 

The most common conversions will be (fairly straightforward conversions) between "safe" (smart) pointers and (unsafe) raw pointers. But some conversions involve more elaborate mechanisms, such as those used for pointer-to-pointers and function pointers. Pointer-to-pointer conversions can involve making a temporary copy of the target pointer(s) and subsequently reflecting any changes to the temporary copy back to the original. 

Converting between "safe" and legacy function pointers generally involves "on-the-fly" generation of wrapper (lambda) functions with safe or (legacy) unsafe interfaces as need.

### Marking sections of code, files and/or directories you want to exempt from auto-conversion

As mentioned, the auto-converter will deem some code to be implicitly ineligible for conversion based on the location of the code or whether it is identified as part of a "system" interface, or otherwise-recognized ineligible interface. But you can also explicitly specify code that you don't want the auto-converter to modify.

First, any code in an `extern "C" {}` block will be considered ineligible for conversion. Also, any code that is under a ["check-suppression"](https://github.com/duneroadrunner/scpptool/blob/master/README.md#local-suppression-of-the-checks) [directive](https://github.com/duneroadrunner/SaferCPlusPlus/blob/55022fd7c572510de8be49d5121edea3459394bf/include/mselegacyhelpers.h#L248-L249) won't be modified. 

And when invoking the auto-converter you can, and this is very much recommended, use the `-ModifiablePaths` and `-UnModifiablePaths` command-line options to specify paths where the contained source code should be considered "modifiable" or "unmodifiable". In particular, if you specify any modifiable paths, then any other paths will be considered unmodifiable by default.

### Missing safe implementations of standard library elements

The auto-converter will convert many of the commonly used standard library calls to safe implementations provided in the SaferCPlusPlus library. For example, `memcpy()` calls will be replaced with calls to `lh::memcpy<>()`. (A function template that verifies the type safety of its arguments and bounds safety of its operation.) But as of the time of writing, many standard library elements do not yet have safe replacement implementations. Notably, this includes `printf()` and friends. For those, the auto-converter will just treat them as "unmodifiable" system calls and interface with them as described above.

### Auto-conversion of C to valid C++

One of the key design features of C++ was backward compatability with C. While largely compatible (at least with 1990s era C), there are significant incompatibilities that make it likely that legacy C code bases that didn't adopt a policy of C++ compatibility will generate compiler errors when compiled as C++. So scpptool has another feature that helps convert (1990s era) C code to valid C++ (while remaining valid C). Most of the required changes are relatively minor and straightforward, some differences, such as C++'s extra restrictions on the use of `goto`, are a little more involved (and in the case of `goto`s, may involve a little code restructuring).

### Potential usage scenarios

The main intended use case would probably be the one where you're starting with a legacy C code base and are intending to end up with (more) modern code largely restricted to the scpptool-enforced safe subset of C++. In that scenario, auto-conversion of the code base might be just one step in the process. Separartely, performance-sensitive parts of the code may need to be manually converted to be more efficient and idiomatic of the scpptool-enforced safe subset. 

The less performance-sensitive parts of the code can, if desired, be modernized with (presumably) less urgency. But one of the benefits of the "one-for-one" replacement technique that the auto-converter uses by default, is that, because each (safe) replacement element is designed to replicate the (valid) behavior of the original element, the opportunities for the introduction of new unintended behavior (that deviates from the original behavior) are limited. All the original algorithms, loops, branch conditions, etc. are preserved. This arguably results in the converted code, to some degree, inheriting the "well-tested" or "battle-proven" "correctness" of the original code. 

Manually modernizing such code potentially sacrifices some the value of the testing (and field testing) of the original code, so one might imagine (with the urgency of potential memory vulnerabilities addressed via auto-conversion) the advantage of having the flexibility to postpone such modernization until such time that adequate testing of the new rewritten code can be done.

Note though, that while the auto-conversion may be less prone to introducing new unintended behavior, it may also be prone to raising exceptions in cases where the original code may have "worked just fine", but doesn't meet the standards of "correctness" of the scpptool-enforced subset. For example, we've encountered code that assigns an `char const**` to a `void*` and then casts that `void*` value to an `char**`. While this code "worked" in practice, it is technically a `const`-correctness violation and the auto-converted code throws an exception (or user-specified custom behavior, if any) upon the `const`-correctness violating cast attempt. And one may not find out about it until they encounter it at run-time. One can of course, insert exception handling code to deal with such potential unforseen exceptions with some degree of "gracefullness".

Though not the primary intended use case, in theory the auto-conversion could be used as simply a build step, allowing one to maintain their codebase in legacy form and, for certain build targets, produce a (more) memory-safe executable. 

Note though, that not all legacy code can be auto-converted. It has to be, in some sense, at least "reasonable". While arguably some might want to use this as a "disciplining" measure to enforce some level of "reasonableness" on the legacy code, any code section that fails to auto-convert can generally be (manually) marked/annotated as "unsafe" and not to be converted. The auto-converter can generally accommodate such annotated code.
