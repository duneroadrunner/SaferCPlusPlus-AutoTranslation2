Mar 2022

This subdirectory contains a version of the LodePNG png encoder/decoder that has been converted to SaferCPlusPlus with the SaferCPlusPlus conversion assistance tool, and then partially re-optimized as demonstration of how to recover some of the original performance while maintaining (the newly attained) memory safety.

So again, by default, the conversion essentially performs a one-to-one replacement of unsafe elements in the original source, which can result in code with noticeably worse performance than the original. If needed, the translated code can be manually re-optimized. The assumption being that even in performance sensitive programs, usually only a small, concentrated portion of the total code actually affects performance. (In contrast to unsafe memory accesses, which generally could occur anywhere in the code.)

Most of the added overhead introduced in the conversion comes from the replacement of native pointers with safe "smart" pointers with run-time safety mechanisms. Where possible, we now endeavor to replace them with [scope pointers](https://github.com/duneroadrunner/SaferCPlusPlus/blob/master/README.md#scope-pointers) that attain their memory safety via compile-time restrictions with no extra run-time cost.

The problem is that in C (and C++) it is valid for a variable/object to "expire" or be deallocated while an (imminently "dangling") pointer still targets it, as long as the pointer isn't subsequently dereferenced. The only SaferCPlusPlus pointers that (safely) support this are [registered pointers](https://github.com/duneroadrunner/SaferCPlusPlus/blob/master/README.md#registered-pointers), so by default, native pointers are converted to registered pointers (and their target objects are converted to registered objects). But the (zero-overhead) scope pointers we desire cannot be obtained directly from registered pointers.

We can, however, obtain scope pointers from [norad pointers](https://github.com/duneroadrunner/SaferCPlusPlus/blob/master/README.md#norad-pointers). So we can try replacing the registered pointers and objects of interest with their norad counterparts. Note though that unlike with registered pointers, the expiration/deallocation of an object while it is the target of a norad pointer is not permitted (and would result in immediate program termination). So we have to ensure such a situation doesn't occur.

Here in our particular case, we're in luck. The original lodepng code is very good about this and doesn't seem to have any dangling pointers. So we can go ahead and convert all the registered pointers to norad pointers by simply replacing "Registered" with "Norad" in the code text. 

We'll demonstrate how to obtain (zero-overhead) scope pointers from norad pointers, but as with any optimization undertaking, the first step is to identify those sections of the code that are critical to performance. As this is just a demonstration, we won't bother with the rigorous profiling that would normally be done, but for example, it's readily apparent that in our example [`huffmanDecodeSymbol()`](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/440d2adb9f12b25a7bbe0be089ed8349d9d37669/examples/lodepng/lodepng_translated/src/lodepng.cpp#L944-L962) is a "hot" function:

```cpp
static unsigned huffmanDecodeSymbol(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp,
                                    mse::lh::TLHNullableAnyPointer<const HuffmanTree>  codetree, size_t inbitlength)
{
  unsigned int treepos = 0; unsigned int ct = 0/*auto-generated init val*/;
  for(;;)
  {
    if(*bp >= inbitlength) return (unsigned)(-1); /*error: end of input memory reached without endcode*/
    /*
    decode the symbol from the tree. The "readBitFromStream" code is inlined in
    the expression below because this is the biggest bottleneck while decoding
    */
    ct = codetree->tree2d[(treepos << 1) + READBIT(*bp, in)];
    ++(*bp);
    if(ct < codetree->numcodes) return ct; /*the symbol is decoded, return it*/
    else treepos = ct - codetree->numcodes; /*symbol not yet decoded, instead move tree position*/

    if(treepos >= codetree->numcodes) return (unsigned)(-1); /*error: it appeared outside the codetree*/
  }
}
```

Starting from the top, we observe that the first parameter, `in`, is a `TLHNullableAnyRandomAccessIterator<>`. `TLHNullableAnyRandomAccessIterator<>` is a "type-erased, polymorphic" iterator. That is, it's kind of like an `std::any` that's restricted to holding iterators (or null) and can be used like an iterator. This allows a caller to pass any type of (supported) iterator as the first parameter. The drawback is that any iterator operation (such as dereferencing or incrementing) incurs a level of indirection. (Essentially it calls the corresponding operator of the contained iterator as a virtual function/operator.) This indirection can be kind of expensive. We can avoid this penalty by, instead of using a polymorphic iterator type, making the function a function template and making the first function parameter type a template parameter. 

This similarly applies to the second and third function parameters. So an "optimized" version of the function might look something [like](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/7c4c9878819547e4337027691e2bec4560d63711/examples/lodepng/lodepng_partial_reoptimization1/src/lodepng.cpp#L948-L967):

```cpp
template<typename TIn, typename TBp, typename TCodeTree>
static unsigned huffmanDecodeSymbol(TIn in, TBp bp, TCodeTree codetree, size_t inbitlength)
{
  unsigned int treepos = 0; unsigned int ct = 0/*auto-generated init val*/;

    for (;;)
    {
        if (*bp >= inbitlength) return (unsigned)(-1); /*error: end of input memory reached without endcode*/
        /*
        decode the symbol from the tree. The "readBitFromStream" code is inlined in
        the expression below because this is the biggest bottleneck while decoding
        */
        ct = codetree->tree2d[(treepos << 1) + READBIT(*bp, in)];
        ++(*bp);
        if (ct < codetree->numcodes) return ct; /*the symbol is decoded, return it*/
        else treepos = ct - codetree->numcodes; /*symbol not yet decoded, instead move tree position*/

        if (treepos >= codetree->numcodes) return (unsigned)(-1); /*error: it appeared outside the codetree*/
    }
}
```

So now let's look at [`inflateHuffmanBlock()`](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/440d2adb9f12b25a7bbe0be089ed8349d9d37669/examples/lodepng/lodepng_translated/src/lodepng.cpp#L1128-L1220), one of the "hot" functions that calls the "hot" function we just templatized/optimized:

```cpp
static unsigned inflateHuffmanBlock(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp,
                                    mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  pos, size_t inlength, unsigned btype)
{
  unsigned error = 0;
  mse::TNoradObj<HuffmanTree > tree_ll; /*the huffman tree for literal and length codes*/
  mse::TNoradObj<HuffmanTree > tree_d; /*the huffman tree for distance codes*/
  size_t inbitlength = inlength * 8;

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  if(btype == 1) getTreeInflateFixed(&tree_ll, &tree_d);
  else if(btype == 2) error = getTreeInflateDynamic(&tree_ll, &tree_d, in, bp, inlength);

  while(!error) /*decode all symbols until end reached, breaks at end code*/
  {
    /*code_ll is literal, length or end code*/
    unsigned code_ll = huffmanDecodeSymbol(in, bp, &tree_ll, inbitlength);

    ...

  }

  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}
```

So again we can templatize the function and replace the polymorphic function parameter types with template parameters. But in the case of the first two function parameters, `out` and `in`, they are passed through a deeply nested call stack, and would only be passed as their original type if every function in the call stack is also templatized in the same way. While it's not egregiously burdensome to do so, here we will demonstrate an alternative method of obtaining those parameters as their original type. That alternative is to extract the element contained in the polymorhpic type via the [`mse::maybe_any_cast<>()`](https://github.com/duneroadrunner/SaferCPlusPlus/blob/master/README.md#anys) function, which is similar to the `std::any_cast<>()` function, but also works on the polymorphic iterator and pointer types we're dealing with.

So first let's see what the "re-optimized" function looks like, so we can then explain the changes made:

```cpp
template<typename TBp, typename TPos>
static unsigned inflateHuffmanBlock(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, TBp bp,
    TPos pos, size_t inlength, unsigned btype)
{
  unsigned error = 0;
  mse::TNoradObj<HuffmanTree > tree_ll; /*the huffman tree for literal and length codes*/
  mse::TNoradObj<HuffmanTree > tree_d; /*the huffman tree for distance codes*/
  size_t inbitlength = inlength * 8;

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  if(btype == 1) getTreeInflateFixed(&tree_ll, &tree_d);
  else if(btype == 2) error = getTreeInflateDynamic(&tree_ll, &tree_d, in, bp, inlength);

  auto loop1 = [&](auto in, auto out, auto bp, auto pos) {
      while (!error) /*decode all symbols until end reached, breaks at end code*/
      {
          /*code_ll is literal, length or end code*/
          unsigned code_ll = huffmanDecodeSymbol(in, bp, &tree_ll, inbitlength);

          ...

      }
  };

  mse::xscope_optional<mse::lh::TStrongVectorIterator<unsigned char> > maybe_in_sviter;
  auto maybe_in_sviter2 = mse::maybe_any_cast<mse::lh::TStrongVectorIterator<unsigned char> >(in);
  if (maybe_in_sviter2.has_value()) {
      maybe_in_sviter = maybe_in_sviter2.value();
  }
  else {
      auto maybe_in_sviter3 = mse::maybe_any_cast<mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char> > >(in);
      if (maybe_in_sviter3.has_value()) {
          maybe_in_sviter = maybe_in_sviter3.value();
      }
  }
  auto maybe_out_nnnptr = mse::maybe_any_cast<mse::TNoradNotNullPointer<ucvector> >(out);
  if (maybe_in_sviter.has_value() && maybe_out_nnnptr.has_value()) {
      auto in_sviter = maybe_in_sviter.value();
      auto out_xs_store = mse::make_xscope_strong_pointer_store(maybe_out_nnnptr.value());
      auto out_xsptr = out_xs_store.xscope_ptr();

      loop1(in_sviter, out_xsptr, bp, pos);
  }
  else {
      loop1(in, out, bp, pos);
  }

  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}
```

First, we put the loop in a lambda function with `auto` parameters, just so we can invoke it with different argument types depending on whether we successfully extract the contained elements from `inflateHuffmanBlock<>()`'s polymorphic function parameters.

Then we use `mse::maybe_any_cast<>()` to try to extract the contained elements as their original type. By locating the place in the code where it's originally declared, we see that the original type of the `in` parameter should be either `mse::lh::TStrongVectorIterator<unsigned char>` or `mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char> >` (depending on where the function is being called from). And the original type of the `out` parameter should be `mse::TNoradNotNullPointer<ucvector>`. We could use the `out` parameter as its recovered original norad pointer type. But we can actually do even better by obtaining a (zero-overhead) scope pointer from the norad pointer using the [`mse::make_xscope_strong_pointer_store<>()`](https://github.com/duneroadrunner/SaferCPlusPlus/blob/master/README.md#make_xscope_strong_pointer_store) function.

Finally, when we locate the declarations of the original sources of the `bp` and `pos` function parameters in the [`lodepng_inflatev()`](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/1c190a566646717742d619b69d751b0a7b8291d1/examples/lodepng/lodepng_translated/src/lodepng.cpp#L1250-L1278) function, we find:

```cpp
static unsigned lodepng_inflatev( ... )
{
  /*bit pointer in the "in" data, current byte is bp >> 3, current bit is bp & 0x7 (from lsb to msb of the byte)*/
  mse::TNoradObj<unsigned long > bp = 0;
  unsigned BFINAL = 0;
  mse::TNoradObj<unsigned long > pos = 0; /*byte position in the out buffer*/

  ...

    error = inflateHuffmanBlock(out, in, &bp, &pos, insize, BTYPE); /*compression, BTYPE 01 or 10*/

  ...
}
```

and note that the `&bp` and `&pos` arguments passed are norad pointers. While, as we just demonstrated, we can obtain (zero-overhead) scope pointers from norad pointers, it would be even more convenient to just pass scope pointers in the first place. We can do this by simply changing the `bp` and `pos` variables from norad objects to scope objects [like so](https://github.com/duneroadrunner/SaferCPlusPlus-AutoTranslation2/blob/7c4c9878819547e4337027691e2bec4560d63711/examples/lodepng/lodepng_partial_reoptimization1/src/lodepng.cpp#L1287-L1289):

```cpp
  mse::TXScopeObj<unsigned long > bp = 0;
  unsigned BFINAL = 0;
  mse::TXScopeObj<unsigned long > pos = 0; /*byte position in the out buffer*/
```

You can't always just change norad objects to scope objects as scope objects have different (more stringent) restrictions, and so attempting to do so may result in compile-time errors. But in this case it works.

So after removing usage of polymorphic pointer and iterator types from performance sensitive sections of the code, the program performance should be significantly closer to that of the (unsafe) original. But with iterators, there would still be the added bounds checking. Redundant bounds checking minimization techniques are generally more situation specific.


--------

LodePNG
-------

PNG encoder and decoder in C and C++.

Home page: http://lodev.org/lodepng/

Only two files are needed to allow your program to read and write PNG files: lodepng.cpp and lodepng.h.

For C, you can rename lodepng.cpp to lodepng.c and it'll work. C++ only adds extra API.

The other files in the project are just examples, unit tests, etc...
