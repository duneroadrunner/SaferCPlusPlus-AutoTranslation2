
#include "mselegacyhelpers.h"
/*
LodePNG version 20161127

Copyright (c) 2005-2016 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

/*
The manual and changelog are in the header file "lodepng.h"
Rename this file to lodepng.cpp to use it for C++, or to lodepng.c to use it for C.
*/

#include "lodepng.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1310) /*Visual Studio: A few warning types are not desired here.*/
#pragma warning( disable : 4244 ) /*implicit conversions: not warned by gcc -Wall -Wextra and requires too much casts*/
#pragma warning( disable : 4996 ) /*VS does not like fopen, but fopen_s is not standard C so unusable here*/
#endif /*_MSC_VER */

mse::lh::TLHNullableAnyRandomAccessIterator<const char>  LODEPNG_VERSION_STRING = "20161127";

/*
This source file is built up in the following large parts. The code sections
with the "LODEPNG_COMPILE_" #defines divide this up further in an intermixed way.
-Tools for C and common code for PNG and Zlib
-C Code for Zlib (huffman, deflate, ...)
-C Code for PNG (file format chunks, adam7, PNG filters, color conversions, ...)
-The C++ wrapper around all of the above
*/

/*The malloc, realloc and free functions defined here with "lodepng_" in front
of the name, so that you can easily change them to others related to your
platform if needed. Everything else in the code calls these. Pass
-DLODEPNG_NO_COMPILE_ALLOCATORS to the compiler, or comment out
#define LODEPNG_COMPILE_ALLOCATORS in the header, to disable the ones here and
define them in your own project's source files without needing to change
lodepng source code. Don't forget to remove "static" if you copypaste them
from here.*/

#ifdef LODEPNG_COMPILE_ALLOCATORS
static mse::lh::void_star_replacement   lodepng_malloc(size_t size)
{
  return malloc(size);
}

static mse::lh::void_star_replacement   lodepng_realloc(mse::lh::void_star_replacement   ptr, size_t new_size)
{
  return realloc(ptr, new_size);
}

static void lodepng_free(mse::lh::void_star_replacement   ptr)
{
  mse::lh::free(ptr);
}
#else /*LODEPNG_COMPILE_ALLOCATORS*/
void* lodepng_malloc(size_t size);
void* lodepng_realloc(void* ptr, size_t new_size);
void lodepng_free(void* ptr);
#endif /*LODEPNG_COMPILE_ALLOCATORS*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // Tools for C, and common code for PNG and Zlib.                       // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

/*
Often in case of an error a value is assigned to a variable and then it breaks
out of a loop (to go to the cleanup phase of a function). This macro does that.
It makes the error handling code shorter and more readable.

Example: if(!uivector_resizev(&frequencies_ll, 286, 0)) ERROR_BREAK(83);
*/
#define CERROR_BREAK(errorvar, code)\
{\
  errorvar = code;\
  break;\
}

/*version of CERROR_BREAK that assumes the common case where the error variable is named "error"*/
#define ERROR_BREAK(code) CERROR_BREAK(error, code)

/*Set error var to the error code, and return it.*/
#define CERROR_RETURN_ERROR(errorvar, code)\
{\
  errorvar = code;\
  return code;\
}

/*Try the code, if it returns error, also return the error.*/
#define CERROR_TRY_RETURN(call)\
{\
  unsigned error = call;\
  if(error) return error;\
}

/*Set error var to the error code, and return from the void function.*/
#define CERROR_RETURN(errorvar, code)\
{\
  errorvar = code;\
  return;\
}

/*
About uivector, ucvector and string:
-All of them wrap dynamic arrays or text strings in a similar way.
-LodePNG was originally written in C++. The vectors replace the std::vectors that were used in the C++ version.
-The string tools are made to avoid problems with compilers that declare things like strncat as deprecated.
-They're not used in the interface, only internally in this file as static functions.
-As with many other structs in this file, the init and cleanup functions serve as ctor and dtor.
*/

#ifdef LODEPNG_COMPILE_ZLIB
/*dynamic vector of unsigned ints*/
typedef struct uivector
{
  mse::lh::TStrongVectorIterator<unsigned int>  data;
  unsigned long size = 0/*auto-generated init val*/; /*size in number of unsigned longs*/
  unsigned long allocsize = 0/*auto-generated init val*/; /*allocated size in bytes*/
} uivector;

static void uivector_cleanup(mse::lh::void_star_replacement   p)
{
  ((mse::lh::TLHNullableAnyPointer<uivector> const &)(p))->size = ((mse::lh::TLHNullableAnyPointer<uivector> const &)(p))->allocsize = 0;
  mse::lh::free(((mse::lh::TLHNullableAnyPointer<uivector> const &)(p))->data);
  ((mse::lh::TLHNullableAnyPointer<uivector> const &)(p))->data = NULL;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_reserve(mse::lh::TXScopeLHNullableAnyPointer<uivector>  p, size_t allocsize)
{
  if(allocsize > p->allocsize)
  {
    size_t newsize = (allocsize > p->allocsize * 2) ? allocsize : (allocsize * 3 / 2);
    mse::lh::void_star_replacement   data = mse::lh::reallocate(p->data, (newsize));
    if(data)
    {
      p->allocsize = newsize;
      p->data = (mse::lh::TStrongVectorIterator<unsigned int> const &)(data);
    }
    else return 0; /*error: not enough memory*/
  }
  return 1;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_resize(mse::lh::TXScopeLHNullableAnyPointer<uivector>  p, size_t size)
{
  if(!uivector_reserve(p, size * sizeof(unsigned))) return 0;
  p->size = size;
  return 1; /*success*/
}

/*resize and give all new elements the value*/
static unsigned uivector_resizev(mse::lh::TLHNullableAnyPointer<uivector>  p, size_t size, unsigned value)
{
  unsigned long oldsize = p->size; unsigned long i = 0/*auto-generated init val*/;
  if(!uivector_resize(p, size)) return 0;
  for(i = oldsize; i < size; ++i) p->data[i] = value;
  return 1;
}

static void uivector_init(mse::lh::TXScopeLHNullableAnyPointer<uivector>  p)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
}

#ifdef LODEPNG_COMPILE_ENCODER
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_push_back(mse::lh::TLHNullableAnyPointer<uivector>  p, unsigned c)
{
  if(!uivector_resize(p, p->size + 1)) return 0;
  p->data[p->size - 1] = c;
  return 1;
}
#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_ZLIB*/

/* /////////////////////////////////////////////////////////////////////////// */

/*dynamic vector of unsigned chars*/
typedef struct ucvector
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > data;
  mse::TNoradObj<unsigned long > size = 0/*auto-generated init val*/; /*used size*/
  unsigned long allocsize = 0/*auto-generated init val*/; /*allocated size*/
} ucvector;

/*returns 1 if success, 0 if failure ==> nothing done*/
template<typename TUCVectorPointer>
static unsigned ucvector_reserve(TUCVectorPointer p, size_t allocsize)
{
  if(allocsize > p->allocsize)
  {
    size_t newsize = (allocsize > p->allocsize * 2) ? allocsize : (allocsize * 3 / 2);
    mse::lh::void_star_replacement   data = mse::lh::reallocate(p->data, (newsize));
    if(data)
    {
      p->allocsize = newsize;
      p->data = (mse::lh::TStrongVectorIterator<unsigned char> const &)(data);
    }
    else return 0; /*error: not enough memory*/
  }
  return 1;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
template<typename TUCVectorPointer>
static unsigned ucvector_resize(TUCVectorPointer p, size_t size)
{
  if(!ucvector_reserve(p, size * sizeof(unsigned char))) return 0;
  p->size = size;
  return 1; /*success*/
}

#ifdef LODEPNG_COMPILE_PNG

static void ucvector_cleanup(mse::lh::void_star_replacement   p)
{
  ((mse::lh::TLHNullableAnyPointer<ucvector> const &)(p))->size = ((mse::lh::TLHNullableAnyPointer<ucvector> const &)(p))->allocsize = 0;
  mse::lh::free(((mse::lh::TLHNullableAnyPointer<ucvector> const &)(p))->data);
  ((mse::lh::TLHNullableAnyPointer<ucvector> const &)(p))->data = NULL;
}

static void ucvector_init(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  p)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
}
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ZLIB
/*you can both convert from vector to buffer&size and vica versa. If you use
init_buffer to take over a buffer and size, it is not needed to use cleanup*/
static void ucvector_init_buffer(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  p, mse::lh::TStrongVectorIterator<unsigned char>  buffer, size_t size)
{
  p->data = buffer;
  p->allocsize = p->size = size;
}
#endif /*LODEPNG_COMPILE_ZLIB*/

#if (defined(LODEPNG_COMPILE_PNG) && defined(LODEPNG_COMPILE_ANCILLARY_CHUNKS)) || defined(LODEPNG_COMPILE_ENCODER)
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned ucvector_push_back(mse::lh::TLHNullableAnyPointer<ucvector>  p, unsigned char c)
{
  if(!ucvector_resize(p, p->size + 1)) return 0;
  p->data[p->size - 1] = c;
  return 1;
}
#endif /*defined(LODEPNG_COMPILE_PNG) || defined(LODEPNG_COMPILE_ENCODER)*/


/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_PNG
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned string_resize(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<char> >  out, size_t size)
{
  mse::lh::TStrongVectorIterator<char>  data = mse::lh::reallocate(*out, (size + 1));
  if(data)
  {
    data[size] = 0; /*null termination char*/
    *out = data;
  }
  return data != 0;
}

/*init a {char*, size_t} pair for use as string*/
static void string_init(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<char> >  out)
{
  *out = NULL;
  string_resize(out, 0);
}

/*free the above pair again*/
static void string_cleanup(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<char> >  out)
{
  mse::lh::free(*out);
  *out = NULL;
}

static void string_set(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<char> >  out, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  in)
{
  unsigned long insize = strlen(mse::us::lh::make_raw_pointer_from(in)); unsigned long i = 0/*auto-generated init val*/;
  if(string_resize(out, insize))
  {
    for(i = 0; i != insize; ++i)
    {
      (*out)[i] = in[i];
    }
  }
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#endif /*LODEPNG_COMPILE_PNG*/

/* ////////////////////////////////////////////////////////////////////////// */

template<typename TBuffer>
unsigned lodepng_read32bitInt(TBuffer buffer)
{
  return (unsigned)((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]);
}

#if defined(LODEPNG_COMPILE_PNG) || defined(LODEPNG_COMPILE_ENCODER)
/*buffer must have at least 4 allocated bytes available*/
static void lodepng_set32bitInt(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  buffer, unsigned value)
{
  buffer[0] = (unsigned char)((value >> 24) & 0xff);
  buffer[1] = (unsigned char)((value >> 16) & 0xff);
  buffer[2] = (unsigned char)((value >>  8) & 0xff);
  buffer[3] = (unsigned char)((value      ) & 0xff);
}
#endif /*defined(LODEPNG_COMPILE_PNG) || defined(LODEPNG_COMPILE_ENCODER)*/

#ifdef LODEPNG_COMPILE_ENCODER
static void lodepng_add32bitInt(mse::lh::TLHNullableAnyPointer<ucvector>  buffer, unsigned value)
{
  ucvector_resize(buffer, buffer->size + 4); /*todo: give error if resize failed*/
  lodepng_set32bitInt(((buffer->data) + (buffer->size - 4)), value);
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / File IO                                                                / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DISK

/* returns negative value on error. This should be pure C compatible, so no fstat. */
static long lodepng_filesize(mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename)
{
  FILE*  file = nullptr/*auto-generated init val*/;
  long size = 0/*auto-generated init val*/;
  file = fopen(mse::us::lh::make_raw_pointer_from(filename), "rb");
  if(!file) return -1;

  if(fseek(mse::us::lh::make_raw_pointer_from(file), 0, SEEK_END) != 0)
  {
    fclose(mse::us::lh::make_raw_pointer_from(file));
    return -1;
  }

  size = ftell(mse::us::lh::make_raw_pointer_from(file));
  /* It may give LONG_MAX as directory size, this is invalid for us. */
  if(size == LONG_MAX) size = -1;

  fclose(mse::us::lh::make_raw_pointer_from(file));
  return size;
}

/* load file into buffer that already has the correct allocated size. Returns error code.*/
static unsigned lodepng_buffer_file(mse::lh::TXScopeLHNullableAnyRandomAccessIterator<unsigned char>  out, size_t size, mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename)
{
  FILE*  file = nullptr/*auto-generated init val*/;
  unsigned long readsize = 0/*auto-generated init val*/;
  file = fopen(mse::us::lh::make_raw_pointer_from(filename), "rb");
  if(!file) return 78;

  readsize = mse::lh::fread(mse::us::lh::make_raw_pointer_from(out), 1, size, mse::us::lh::make_raw_pointer_from(file));
  fclose(mse::us::lh::make_raw_pointer_from(file));

  if (readsize != size) return 78;
  return 0;
}

unsigned lodepng_load_file(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename)
{
  long size = lodepng_filesize(filename);
  if (size < 0) return 78;
  *outsize = (size_t)size;

  mse::lh::allocate(*out, ((size_t)size));
  if(!(*out) && size > 0) return 83; /*the above malloc failed*/

  return lodepng_buffer_file(*out, (size_t)size, filename);
}

/*write given buffer to the file, overwriting the file, it doesn't append to it.*/
unsigned lodepng_save_file(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  buffer, size_t buffersize, mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename)
{
  FILE*  file = nullptr/*auto-generated init val*/;
  file = fopen(mse::us::lh::make_raw_pointer_from(filename), "wb" );
  if(!file) return 79;
  mse::lh::fwrite(mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<char>>(buffer) , 1 , buffersize, mse::us::lh::make_raw_pointer_from(file));
  fclose(mse::us::lh::make_raw_pointer_from(file));
  return 0;
}

#endif /*LODEPNG_COMPILE_DISK*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of common code and tools. Begin of Zlib related code.            // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ZLIB
#ifdef LODEPNG_COMPILE_ENCODER
/*TODO: this ignores potential out of memory errors*/
#define addBitToStream(/*size_t**/ bitpointer, /*ucvector**/ bitstream, /*unsigned char*/ bit)\
{\
  /*add a new byte at the end*/\
  if(((*bitpointer) & 7) == 0) ucvector_push_back(bitstream, (unsigned char)0);\
  /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/\
  (bitstream->data[bitstream->size - 1]) |= (bit << ((*bitpointer) & 0x7));\
  ++(*bitpointer);\
}

static void addBitsToStream(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyPointer<ucvector>  bitstream, unsigned value, size_t nbits)
{
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0; i != nbits; ++i) addBitToStream(bitpointer, bitstream, (unsigned char)((value >> i) & 1));
}

static void addBitsToStreamReversed(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyPointer<ucvector>  bitstream, unsigned value, size_t nbits)
{
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0; i != nbits; ++i) addBitToStream(bitpointer, bitstream, (unsigned char)((value >> (nbits - 1 - i)) & 1));
}
#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_DECODER

#define READBIT(bitpointer, bitstream) ((bitstream[bitpointer >> 3] >> (bitpointer & 0x7)) & (unsigned char)1)

static unsigned char readBitFromStream(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  bitstream)
{
  unsigned char result = (unsigned char)(READBIT(*bitpointer, bitstream));
  ++(*bitpointer);
  return result;
}

template<typename TBitPointer, typename TBitStream>
static unsigned readBitsFromStream(TBitPointer bitpointer, TBitStream bitstream, size_t nbits)
{
  unsigned int result = 0; unsigned int i = 0/*auto-generated init val*/;
  for(i = 0; i != nbits; ++i)
  {
    result += ((unsigned)READBIT(*bitpointer, bitstream)) << i;
    ++(*bitpointer);
  }
  return result;
}
#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflate - Huffman                                                      / */
/* ////////////////////////////////////////////////////////////////////////// */

#define FIRST_LENGTH_CODE_INDEX 257
#define LAST_LENGTH_CODE_INDEX 285
/*256 literals, the end code, some length codes, and 2 unused codes*/
#define NUM_DEFLATE_CODE_SYMBOLS 288
/*the distance codes have their own symbols, 30 used, 2 unused*/
#define NUM_DISTANCE_SYMBOLS 32
/*the code length codes. 0-15: code lengths, 16: copy previous 3-6 times, 17: 3-10 zeros, 18: 11-138 zeros*/
#define NUM_CODE_LENGTH_CODES 19

/*the base lengths represented by codes 257-285*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 29> LENGTHBASE = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
     67, 83, 99, 115, 131, 163, 195, 227, 258};

/*the extra bits used by codes 257-285 (added to base length)*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 29> LENGTHEXTRA = {0, 0, 0, 0, 0, 0, 0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
      4,  4,  4,   4,   5,   5,   5,   5,   0};

/*the base backwards distances (the bits of distance codes appear after length codes and use their own huffman tree)*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 30> DISTANCEBASE = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
     769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

/*the extra bits of backwards distances (added to base)*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 30> DISTANCEEXTRA = {0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,   6,   6,   7,   7,   8,
       8,    9,    9,   10,   10,   11,   11,   12,    12,    13,    13};

/*the order in which "code length alphabet code lengths" are stored, out of this
the huffman tree of the dynamic huffman tree lengths is generated*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, NUM_CODE_LENGTH_CODES> CLCL_ORDER = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* ////////////////////////////////////////////////////////////////////////// */

/*
Huffman tree struct, containing multiple representations of the tree
*/
typedef struct HuffmanTree
{
  mse::lh::TStrongVectorIterator<unsigned int>  tree2d;
  mse::lh::TStrongVectorIterator<unsigned int>  tree1d;
  mse::lh::TStrongVectorIterator<unsigned int>  lengths; /*the lengths of the codes of the 1d-tree*/
  unsigned int maxbitlen = 0/*auto-generated init val*/; /*maximum number of bits a single code can get*/
  unsigned int numcodes = 0/*auto-generated init val*/; /*number of symbols in the alphabet = number of codes*/
} HuffmanTree;

/*function used for debug purposes to draw the tree in ascii art with C++*/
/*
static void HuffmanTree_draw(HuffmanTree* tree)
{
  std::cout << "tree. length: " << tree->numcodes << " maxbitlen: " << tree->maxbitlen << std::endl;
  for(size_t i = 0; i != tree->tree1d.size; ++i)
  {
    if(tree->lengths.data[i])
      std::cout << i << " " << tree->tree1d.data[i] << " " << tree->lengths.data[i] << std::endl;
  }
  std::cout << std::endl;
}*/

static void HuffmanTree_init(mse::lh::TXScopeLHNullableAnyPointer<HuffmanTree>  tree)
{
  tree->tree2d = 0;
  tree->tree1d = 0;
  tree->lengths = 0;
}

static void HuffmanTree_cleanup(mse::lh::TXScopeLHNullableAnyPointer<HuffmanTree>  tree)
{
  mse::lh::free(tree->tree2d);
  mse::lh::free(tree->tree1d);
  mse::lh::free(tree->lengths);
}

/*the tree representation used by the decoder. return value is error*/
static unsigned HuffmanTree_make2DTree(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree)
{
  unsigned nodefilled = 0; /*up to which node it is filled*/
  unsigned treepos = 0; /*position in the tree (1 of the numcodes columns)*/
  unsigned int n = 0/*auto-generated init val*/; unsigned int i = 0/*auto-generated init val*/;

  mse::lh::allocate(tree->tree2d, (tree->numcodes * 2 * sizeof(unsigned)));
  if(!tree->tree2d) return 83; /*alloc fail*/

  /*
  convert tree1d[] to tree2d[][]. In the 2D array, a value of 32767 means
  uninited, a value >= numcodes is an address to another bit, a value < numcodes
  is a code. The 2 rows are the 2 possible bit values (0 or 1), there are as
  many columns as codes - 1.
  A good huffman tree has N * 2 - 1 nodes, of which N - 1 are internal nodes.
  Here, the internal nodes are stored (what their 0 and 1 option point to).
  There is only memory for such good tree currently, if there are more nodes
  (due to too long length codes), error 55 will happen
  */
  for(n = 0; n < tree->numcodes * 2; ++n)
  {
    tree->tree2d[n] = 32767; /*32767 here means the tree2d isn't filled there yet*/
  }

  for(n = 0; n < tree->numcodes; ++n) /*the codes*/
  {
    for(i = 0; i != tree->lengths[n]; ++i) /*the bits for this code*/
    {
      unsigned char bit = (unsigned char)((tree->tree1d[n] >> (tree->lengths[n] - i - 1)) & 1);
      /*oversubscribed, see comment in lodepng_error_text*/
      if(treepos > 2147483647 || treepos + 2 > tree->numcodes) return 55;
      if(tree->tree2d[2 * treepos + bit] == 32767) /*not yet filled in*/
      {
        if(i + 1 == tree->lengths[n]) /*last bit*/
        {
          tree->tree2d[2 * treepos + bit] = n; /*put the current code in it*/
          treepos = 0;
        }
        else
        {
          /*put address of the next step in here, first that address has to be found of course
          (it's just nodefilled + 1)...*/
          ++nodefilled;
          /*addresses encoded with numcodes added to it*/
          tree->tree2d[2 * treepos + bit] = nodefilled + tree->numcodes;
          treepos = nodefilled;
        }
      }
      else treepos = tree->tree2d[2 * treepos + bit] - tree->numcodes;
    }
  }

  for(n = 0; n < tree->numcodes * 2; ++n)
  {
    if(tree->tree2d[n] == 32767) tree->tree2d[n] = 0; /*remove possible remaining 32767's*/
  }

  return 0;
}

/*
Second step for the ...makeFromLengths and ...makeFromFrequencies functions.
numcodes, lengths and maxbitlen must already be filled in correctly. return
value is error.
*/
static unsigned HuffmanTree_makeFromLengths2(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree)
{
  mse::TNoradObj<uivector > blcount;
  mse::TNoradObj<uivector > nextcode;
  unsigned error = 0;
  unsigned int bits = 0/*auto-generated init val*/; unsigned int n = 0/*auto-generated init val*/;

  uivector_init(&blcount);
  uivector_init(&nextcode);

  mse::lh::allocate(tree->tree1d, (tree->numcodes * sizeof(unsigned)));
  if(!tree->tree1d) error = 83; /*alloc fail*/

  if(!uivector_resizev(&blcount, tree->maxbitlen + 1, 0)
  || !uivector_resizev(&nextcode, tree->maxbitlen + 1, 0))
    error = 83; /*alloc fail*/

  if(!error)
  {
    /*step 1: count number of instances of each code length*/
    for(bits = 0; bits != tree->numcodes; ++bits) ++blcount.data[tree->lengths[bits]];
    /*step 2: generate the nextcode values*/
    for(bits = 1; bits <= tree->maxbitlen; ++bits)
    {
      nextcode.data[bits] = (nextcode.data[bits - 1] + blcount.data[bits - 1]) << 1;
    }
    /*step 3: generate all the codes*/
    for(n = 0; n != tree->numcodes; ++n)
    {
      if(tree->lengths[n] != 0) tree->tree1d[n] = nextcode.data[tree->lengths[n]]++;
    }
  }

  uivector_cleanup(&blcount);
  uivector_cleanup(&nextcode);

  if(!error) return HuffmanTree_make2DTree(tree);
  else return error;
}

/*
given the code lengths (as stored in the PNG file), generate the tree as defined
by Deflate. maxbitlen is the maximum bits that a code in the tree can have.
return value is error.
*/
static unsigned HuffmanTree_makeFromLengths(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned int>  bitlen,
                                            size_t numcodes, unsigned maxbitlen)
{
  unsigned int i = 0/*auto-generated init val*/;
  mse::lh::allocate(tree->lengths, (numcodes * sizeof(unsigned)));
  if(!tree->lengths) return 83; /*alloc fail*/
  for(i = 0; i != numcodes; ++i) tree->lengths[i] = bitlen[i];
  tree->numcodes = (unsigned)numcodes; /*number of symbols*/
  tree->maxbitlen = maxbitlen;
  return HuffmanTree_makeFromLengths2(tree);
}

#ifdef LODEPNG_COMPILE_ENCODER

/*BPM: Boundary Package Merge, see "A Fast and Space-Economical Algorithm for Length-Limited Coding",
Jyrki Katajainen, Alistair Moffat, Andrew Turpin, 1995.*/

/*chain node for boundary package merge*/
typedef struct BPMNode
{
  int weight = 0/*auto-generated init val*/; /*the sum of all weights in this chain*/
  unsigned int index = 0/*auto-generated init val*/; /*index of this leaf node (called "count" in the paper)*/
  mse::lh::TLHNullableAnyPointer<BPMNode>  tail; /*the next nodes in this chain (null if last)*/
  int in_use = 0/*auto-generated init val*/;
} BPMNode;

/*lists of chains*/
typedef struct BPMLists
{
  /*memory pool*/
  unsigned int memsize = 0/*auto-generated init val*/;
  mse::lh::TStrongVectorIterator<BPMNode>  memory;
  unsigned int numfree = 0/*auto-generated init val*/;
  unsigned int nextfree = 0/*auto-generated init val*/;
  mse::lh::TStrongVectorIterator<mse::lh::TLHNullableAnyPointer<BPMNode> >  freelist;
  /*two heads of lookahead chains per list*/
  unsigned int listsize = 0/*auto-generated init val*/;
  mse::lh::TStrongVectorIterator<mse::lh::TLHNullableAnyPointer<BPMNode> >  chains0;
  mse::lh::TStrongVectorIterator<mse::lh::TLHNullableAnyPointer<BPMNode> >  chains1;
} BPMLists;

/*creates a new chain node with the given parameters, from the memory in the lists */
mse::lh::TLHNullableAnyPointer<BPMNode>  bpmnode_create(mse::lh::TLHNullableAnyPointer<BPMLists>  lists, int weight, unsigned index, mse::lh::TLHNullableAnyPointer<BPMNode>  tail)
{
  unsigned int i = 0/*auto-generated init val*/;
  mse::lh::TLHNullableAnyPointer<BPMNode>  result = nullptr/*auto-generated init val*/;

  /*memory full, so garbage collect*/
  if(lists->nextfree >= lists->numfree)
  {
    /*mark only those that are in use*/
    for(i = 0; i != lists->memsize; ++i) lists->memory[i].in_use = 0;
    for(i = 0; i != lists->listsize; ++i)
    {
      mse::lh::TLHNullableAnyPointer<BPMNode>  node = nullptr/*auto-generated init val*/;
      for(node = lists->chains0[i]; node != 0; node = node->tail) node->in_use = 1;
      for(node = lists->chains1[i]; node != 0; node = node->tail) node->in_use = 1;
    }
    /*collect those that are free*/
    lists->numfree = 0;
    for(i = 0; i != lists->memsize; ++i)
    {
      if(!lists->memory[i].in_use) lists->freelist[lists->numfree++] = ((lists->memory) + (i));
    }
    lists->nextfree = 0;
  }

  result = lists->freelist[lists->nextfree++];
  result->weight = weight;
  result->index = index;
  result->tail = tail;
  return result;
}

/*sort the leaves with stable mergesort*/
static void bpmnode_sort(mse::lh::TLHNullableAnyRandomAccessIterator<BPMNode>  leaves, size_t num)
{
  mse::lh::TStrongVectorIterator<BPMNode>  mem = mse::lh::allocate_dyn_array1<mse::lh::TStrongVectorIterator<BPMNode> >((sizeof(*leaves) * num));
  unsigned long width = 0/*auto-generated init val*/; unsigned long counter = 0;
  for(width = 1; width < num; width *= 2)
  {
    mse::lh::TLHNullableAnyRandomAccessIterator<BPMNode>  a = (counter & 1) ? mse::lh::TLHNullableAnyRandomAccessIterator<BPMNode >(mem) : leaves;
    mse::lh::TLHNullableAnyRandomAccessIterator<BPMNode>  b = (counter & 1) ? leaves : mse::lh::TLHNullableAnyRandomAccessIterator<BPMNode >(mem);
    unsigned long p = 0/*auto-generated init val*/;
    for(p = 0; p < num; p += 2 * width)
    {
      size_t q = (p + width > num) ? num : (p + width);
      size_t r = (p + 2 * width > num) ? num : (p + 2 * width);
      unsigned long i = p; unsigned long j = q; unsigned long k = 0/*auto-generated init val*/;
      for(k = p; k < r; k++)
      {
        if(i < q && (j >= r || a[i].weight <= a[j].weight)) b[k] = a[i++];
        else b[k] = a[j++];
      }
    }
    counter++;
  }
  if(counter & 1) mse::lh::memcpy(leaves, mem, sizeof(*leaves) * num);
  mse::lh::free(mem);
}

/*Boundary Package Merge step, numpresent is the amount of leaves, and c is the current chain.*/
static void boundaryPM(mse::lh::TLHNullableAnyPointer<BPMLists>  lists, mse::lh::TLHNullableAnyRandomAccessIterator<BPMNode>  leaves, size_t numpresent, int c, int num)
{
  unsigned lastindex = lists->chains1[c]->index;

  if(c == 0)
  {
    if(lastindex >= numpresent) return;
    lists->chains0[c] = lists->chains1[c];
    lists->chains1[c] = bpmnode_create(lists, leaves[lastindex].weight, lastindex + 1, 0);
  }
  else
  {
    /*sum of the weights of the head nodes of the previous lookahead chains.*/
    int sum = lists->chains0[c - 1]->weight + lists->chains1[c - 1]->weight;
    lists->chains0[c] = lists->chains1[c];
    if(lastindex < numpresent && sum > leaves[lastindex].weight)
    {
      lists->chains1[c] = bpmnode_create(lists, leaves[lastindex].weight, lastindex + 1, lists->chains1[c]->tail);
      return;
    }
    lists->chains1[c] = bpmnode_create(lists, sum, lastindex, lists->chains1[c - 1]);
    /*in the end we are only interested in the chain of the last list, so no
    need to recurse if we're at the last one (this gives measurable speedup)*/
    if(num + 1 < (int)(2 * numpresent - 2))
    {
      boundaryPM(lists, leaves, numpresent, c - 1, num);
      boundaryPM(lists, leaves, numpresent, c - 1, num);
    }
  }
}

unsigned lodepng_huffman_code_lengths(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned int>  lengths, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned int>  frequencies,
                                      size_t numcodes, unsigned maxbitlen)
{
  unsigned error = 0;
  unsigned int i = 0/*auto-generated init val*/;
  size_t numpresent = 0; /*number of symbols with non-zero frequency*/
  mse::lh::TStrongVectorIterator<BPMNode>  leaves = nullptr/*auto-generated init val*/; /*the symbols, only those with > 0 frequency*/

  if(numcodes == 0) return 80; /*error: a tree of 0 symbols is not supposed to be made*/
  if((1u << maxbitlen) < numcodes) return 80; /*error: represent all symbols*/

  mse::lh::allocate(leaves, (numcodes * sizeof(*leaves)));
  if(!leaves) return 83; /*alloc fail*/

  for(i = 0; i != numcodes; ++i)
  {
    if(frequencies[i] > 0)
    {
      leaves[numpresent].weight = (int)frequencies[i];
      leaves[numpresent].index = i;
      ++numpresent;
    }
  }

  for(i = 0; i != numcodes; ++i) lengths[i] = 0;

  /*ensure at least two present symbols. There should be at least one symbol
  according to RFC 1951 section 3.2.7. Some decoders incorrectly require two. To
  make these work as well ensure there are at least two symbols. The
  Package-Merge code below also doesn't work correctly if there's only one
  symbol, it'd give it the theoritical 0 bits but in practice zlib wants 1 bit*/
  if(numpresent == 0)
  {
    lengths[0] = lengths[1] = 1; /*note that for RFC 1951 section 3.2.7, only lengths[0] = 1 is needed*/
  }
  else if(numpresent == 1)
  {
    lengths[leaves[0].index] = 1;
    lengths[leaves[0].index == 0 ? 1 : 0] = 1;
  }
  else
  {
    mse::TNoradObj<BPMLists > lists;
    mse::lh::TLHNullableAnyPointer<BPMNode>  node = nullptr/*auto-generated init val*/;

    bpmnode_sort(leaves, numpresent);

    lists.listsize = maxbitlen;
    lists.memsize = 2 * maxbitlen * (maxbitlen + 1);
    lists.nextfree = 0;
    lists.numfree = lists.memsize;
    mse::lh::allocate(lists.memory, (lists.memsize * sizeof(*lists.memory)));
    mse::lh::allocate(lists.freelist, (lists.memsize * sizeof(BPMNode*)) / sizeof(BPMNode *) * sizeof(mse::lh::TLHNullableAnyPointer<BPMNode> ));
    mse::lh::allocate(lists.chains0, (lists.listsize * sizeof(BPMNode*)) / sizeof(BPMNode *) * sizeof(mse::lh::TLHNullableAnyPointer<BPMNode> ));
    mse::lh::allocate(lists.chains1, (lists.listsize * sizeof(BPMNode*)) / sizeof(BPMNode *) * sizeof(mse::lh::TLHNullableAnyPointer<BPMNode> ));
    if(!lists.memory || !lists.freelist || !lists.chains0 || !lists.chains1) error = 83; /*alloc fail*/

    if(!error)
    {
      for(i = 0; i != lists.memsize; ++i) lists.freelist[i] = ((lists.memory) + (i));

      bpmnode_create(&lists, leaves[0].weight, 1, 0);
      bpmnode_create(&lists, leaves[1].weight, 2, 0);

      for(i = 0; i != lists.listsize; ++i)
      {
        lists.chains0[i] = ((lists.memory) + (0));
        lists.chains1[i] = ((lists.memory) + (1));
      }

      /*each boundaryPM call adds one chain to the last list, and we need 2 * numpresent - 2 chains.*/
      for(i = 2; i != 2 * numpresent - 2; ++i) boundaryPM(&lists, leaves, numpresent, (int)maxbitlen - 1, (int)i);

      for(node = lists.chains1[maxbitlen - 1]; node; node = node->tail)
      {
        for(i = 0; i != node->index; ++i) ++lengths[leaves[i].index];
      }
    }

    mse::lh::free(lists.memory);
    mse::lh::free(lists.freelist);
    mse::lh::free(lists.chains0);
    mse::lh::free(lists.chains1);
  }

  mse::lh::free(leaves);
  return error;
}

/*Create the Huffman tree given the symbol frequencies*/
static unsigned HuffmanTree_makeFromFrequencies(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned int>  frequencies,
                                                size_t mincodes, size_t numcodes, unsigned maxbitlen)
{
  unsigned error = 0;
  while(!frequencies[numcodes - 1] && numcodes > mincodes) --numcodes; /*trim zeroes*/
  tree->maxbitlen = maxbitlen;
  tree->numcodes = (unsigned)numcodes; /*number of symbols*/
  tree->lengths = mse::lh::reallocate(tree->lengths, (numcodes * sizeof(unsigned)));
  if(!tree->lengths) return 83; /*alloc fail*/
  /*initialize all lengths to 0*/
  mse::lh::memset(tree->lengths, 0, numcodes * sizeof(unsigned));

  error = lodepng_huffman_code_lengths(tree->lengths, frequencies, numcodes, maxbitlen);
  if(!error) error = HuffmanTree_makeFromLengths2(tree);
  return error;
}

static unsigned HuffmanTree_getCode(mse::lh::TLHNullableAnyPointer<const HuffmanTree>  tree, unsigned index)
{
  return tree->tree1d[index];
}

static unsigned HuffmanTree_getLength(mse::lh::TLHNullableAnyPointer<const HuffmanTree>  tree, unsigned index)
{
  return tree->lengths[index];
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/*get the literal and length code tree of a deflated block with fixed tree, as per the deflate specification*/
static unsigned generateFixedLitLenTree(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree)
{
  unsigned int i = 0/*auto-generated init val*/; unsigned int error = 0;
  mse::lh::TStrongVectorIterator<unsigned int>  bitlen = mse::lh::allocate_dyn_array1<mse::lh::TStrongVectorIterator<unsigned int> >((NUM_DEFLATE_CODE_SYMBOLS * sizeof(unsigned)));
  if(!bitlen) return 83; /*alloc fail*/

  /*288 possible codes: 0-255=literals, 256=endcode, 257-285=lengthcodes, 286-287=unused*/
  for(i =   0; i <= 143; ++i) bitlen[i] = 8;
  for(i = 144; i <= 255; ++i) bitlen[i] = 9;
  for(i = 256; i <= 279; ++i) bitlen[i] = 7;
  for(i = 280; i <= 287; ++i) bitlen[i] = 8;

  error = HuffmanTree_makeFromLengths(tree, bitlen, NUM_DEFLATE_CODE_SYMBOLS, 15);

  mse::lh::free(bitlen);
  return error;
}

/*get the distance code tree of a deflated block with fixed tree, as specified in the deflate specification*/
static unsigned generateFixedDistanceTree(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree)
{
  unsigned int i = 0/*auto-generated init val*/; unsigned int error = 0;
  mse::lh::TStrongVectorIterator<unsigned int>  bitlen = mse::lh::allocate_dyn_array1<mse::lh::TStrongVectorIterator<unsigned int> >((NUM_DISTANCE_SYMBOLS * sizeof(unsigned)));
  if(!bitlen) return 83; /*alloc fail*/

  /*there are 32 distance codes, but 30-31 are unused*/
  for(i = 0; i != NUM_DISTANCE_SYMBOLS; ++i) bitlen[i] = 5;
  error = HuffmanTree_makeFromLengths(tree, bitlen, NUM_DISTANCE_SYMBOLS, 15);

  mse::lh::free(bitlen);
  return error;
}

#ifdef LODEPNG_COMPILE_DECODER

/*
returns the code, or (unsigned)(-1) if error happened
inbitlength is the length of the complete buffer, in bits (so its byte length times 8)
*/
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
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / Inflator (Decompressor)                                                / */
/* ////////////////////////////////////////////////////////////////////////// */

/*get the tree of a deflated block with fixed tree, as specified in the deflate specification*/
static void getTreeInflateFixed(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree_ll, mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree_d)
{
  /*TODO: check for out of memory errors*/
  generateFixedLitLenTree(tree_ll);
  generateFixedDistanceTree(tree_d);
}

/*get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree*/
static unsigned getTreeInflateDynamic(mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree_ll, mse::lh::TLHNullableAnyPointer<HuffmanTree>  tree_d,
                                      mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp, size_t inlength)
{
  /*make sure that length values that aren't filled in will be 0, or a wrong tree will be generated*/
  unsigned error = 0;
  unsigned int n = 0/*auto-generated init val*/; unsigned int HLIT = 0/*auto-generated init val*/; unsigned int HDIST = 0/*auto-generated init val*/; unsigned int HCLEN = 0/*auto-generated init val*/; unsigned int i = 0/*auto-generated init val*/;
  size_t inbitlength = inlength * 8;

  /*see comments in deflateDynamic for explanation of the context and these variables, it is analogous*/
  mse::lh::TStrongVectorIterator<unsigned int>  bitlen_ll = nullptr; /*lit,len code lengths*/
  mse::lh::TStrongVectorIterator<unsigned int>  bitlen_d = nullptr; /*dist code lengths*/
  /*code length code lengths ("clcl"), the bit lengths of the huffman tree used to compress bitlen_ll and bitlen_d*/
  mse::lh::TStrongVectorIterator<unsigned int>  bitlen_cl = nullptr;
  mse::TNoradObj<HuffmanTree > tree_cl; /*the code tree for code length codes (the huffman tree for compressed huffman trees)*/

  if((*bp) + 14 > (inlength << 3)) return 49; /*error: the bit pointer is or will go past the memory*/

  /*number of literal/length codes + 257. Unlike the spec, the value 257 is added to it here already*/
  HLIT =  readBitsFromStream(bp, in, 5) + 257;
  /*number of distance codes. Unlike the spec, the value 1 is added to it here already*/
  HDIST = readBitsFromStream(bp, in, 5) + 1;
  /*number of code length codes. Unlike the spec, the value 4 is added to it here already*/
  HCLEN = readBitsFromStream(bp, in, 4) + 4;

  if((*bp) + HCLEN * 3 > (inlength << 3)) return 50; /*error: the bit pointer is or will go past the memory*/

  HuffmanTree_init(&tree_cl);

  while(!error)
  {
    /*read the code length codes out of 3 * (amount of code length codes) bits*/

    mse::lh::allocate(bitlen_cl, (NUM_CODE_LENGTH_CODES * sizeof(unsigned)));
    if(!bitlen_cl) ERROR_BREAK(83 /*alloc fail*/);

    for(i = 0; i != NUM_CODE_LENGTH_CODES; ++i)
    {
      if(i < HCLEN) bitlen_cl[CLCL_ORDER[i]] = readBitsFromStream(bp, in, 3);
      else bitlen_cl[CLCL_ORDER[i]] = 0; /*if not, it must stay 0*/
    }

    error = HuffmanTree_makeFromLengths(&tree_cl, bitlen_cl, NUM_CODE_LENGTH_CODES, 7);
    if(error) break;

    /*now we can use this tree to read the lengths for the tree that this function will return*/
    mse::lh::allocate(bitlen_ll, (NUM_DEFLATE_CODE_SYMBOLS * sizeof(unsigned)));
    mse::lh::allocate(bitlen_d, (NUM_DISTANCE_SYMBOLS * sizeof(unsigned)));
    if(!bitlen_ll || !bitlen_d) ERROR_BREAK(83 /*alloc fail*/);
    for(i = 0; i != NUM_DEFLATE_CODE_SYMBOLS; ++i) bitlen_ll[i] = 0;
    for(i = 0; i != NUM_DISTANCE_SYMBOLS; ++i) bitlen_d[i] = 0;

    /*i is the current symbol we're reading in the part that contains the code lengths of lit/len and dist codes*/
    i = 0;
    while(i < HLIT + HDIST)
    {
      unsigned code = huffmanDecodeSymbol(in, bp, &tree_cl, inbitlength);
      if(code <= 15) /*a length code*/
      {
        if(i < HLIT) bitlen_ll[i] = code;
        else bitlen_d[i - HLIT] = code;
        ++i;
      }
      else if(code == 16) /*repeat previous*/
      {
        unsigned replength = 3; /*read in the 2 bits that indicate repeat length (3-6)*/
        unsigned int value = 0/*auto-generated init val*/; /*set value to the previous code*/

        if(i == 0) ERROR_BREAK(54); /*can't repeat previous if i is 0*/

        if((*bp + 2) > inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/
        replength += readBitsFromStream(bp, in, 2);

        if(i < HLIT + 1) value = bitlen_ll[i - 1];
        else value = bitlen_d[i - HLIT - 1];
        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; ++n)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(13); /*error: i is larger than the amount of codes*/
          if(i < HLIT) bitlen_ll[i] = value;
          else bitlen_d[i - HLIT] = value;
          ++i;
        }
      }
      else if(code == 17) /*repeat "0" 3-10 times*/
      {
        unsigned replength = 3; /*read in the bits that indicate repeat length*/
        if((*bp + 3) > inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/
        replength += readBitsFromStream(bp, in, 3);

        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; ++n)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(14); /*error: i is larger than the amount of codes*/

          if(i < HLIT) bitlen_ll[i] = 0;
          else bitlen_d[i - HLIT] = 0;
          ++i;
        }
      }
      else if(code == 18) /*repeat "0" 11-138 times*/
      {
        unsigned replength = 11; /*read in the bits that indicate repeat length*/
        if((*bp + 7) > inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/
        replength += readBitsFromStream(bp, in, 7);

        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; ++n)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(15); /*error: i is larger than the amount of codes*/

          if(i < HLIT) bitlen_ll[i] = 0;
          else bitlen_d[i - HLIT] = 0;
          ++i;
        }
      }
      else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
      {
        if(code == (unsigned)(-1))
        {
          /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
          (10=no endcode, 11=wrong jump outside of tree)*/
          error = (*bp) > inbitlength ? 10 : 11;
        }
        else error = 16; /*unexisting code, this can never happen*/
        break;
      }
    }
    if(error) break;

    if(bitlen_ll[256] == 0) ERROR_BREAK(64); /*the length of the end code 256 must be larger than 0*/

    /*now we've finally got HLIT and HDIST, so generate the code trees, and the function is done*/
    error = HuffmanTree_makeFromLengths(tree_ll, bitlen_ll, NUM_DEFLATE_CODE_SYMBOLS, 15);
    if(error) break;
    error = HuffmanTree_makeFromLengths(tree_d, bitlen_d, NUM_DISTANCE_SYMBOLS, 15);

    break; /*end of error-while*/
  }

  mse::lh::free(bitlen_cl);
  mse::lh::free(bitlen_ll);
  mse::lh::free(bitlen_d);
  HuffmanTree_cleanup(&tree_cl);

  return error;
}

/*inflate a block with dynamic of fixed Huffman tree*/
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
          if (code_ll <= 255) /*literal symbol*/
          {
              /*ucvector_push_back would do the same, but for some reason the two lines below run 10% faster*/
              if (!ucvector_resize(out, (*pos) + 1)) ERROR_BREAK(83 /*alloc fail*/);
              out->data[*pos] = (unsigned char)code_ll;
              ++(*pos);
          }
          else if (code_ll >= FIRST_LENGTH_CODE_INDEX && code_ll <= LAST_LENGTH_CODE_INDEX) /*length code*/
          {
              unsigned int code_d = 0/*auto-generated init val*/; unsigned int distance = 0/*auto-generated init val*/;
              unsigned int numextrabits_l = 0/*auto-generated init val*/; unsigned int numextrabits_d = 0/*auto-generated init val*/; /*extra bits for length and distance*/
              unsigned long start = 0/*auto-generated init val*/; unsigned long forward = 0/*auto-generated init val*/; unsigned long backward = 0/*auto-generated init val*/; unsigned long length = 0/*auto-generated init val*/;

              /*part 1: get length base*/
              length = LENGTHBASE[code_ll - FIRST_LENGTH_CODE_INDEX];

              /*part 2: get extra bits and add the value of that to length*/
              numextrabits_l = LENGTHEXTRA[code_ll - FIRST_LENGTH_CODE_INDEX];
              if ((*bp + numextrabits_l) > inbitlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/
              length += readBitsFromStream(bp, in, numextrabits_l);

              /*part 3: get distance code*/
              code_d = huffmanDecodeSymbol(in, bp, &tree_d, inbitlength);
              if (code_d > 29)
              {
                  if (code_ll == (unsigned)(-1)) /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
                  {
                      /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
                      (10=no endcode, 11=wrong jump outside of tree)*/
                      error = (*bp) > inlength * 8 ? 10 : 11;
                  }
                  else error = 18; /*error: invalid distance code (30-31 are never used)*/
                  break;
              }
              distance = DISTANCEBASE[code_d];

              /*part 4: get extra bits from distance*/
              numextrabits_d = DISTANCEEXTRA[code_d];
              if ((*bp + numextrabits_d) > inbitlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/
              distance += readBitsFromStream(bp, in, numextrabits_d);

              /*part 5: fill in all the out[n] values based on the length and dist*/
              start = (*pos);
              if (distance > start) ERROR_BREAK(52); /*too long backward distance*/
              backward = start - distance;

              if (!ucvector_resize(out, (*pos) + length)) ERROR_BREAK(83 /*alloc fail*/);
              if (distance < length) {
                  for (forward = 0; forward < length; ++forward)
                  {
                      out->data[(*pos)++] = out->data[backward++];
                  }
              }
              else {
                  mse::lh::memcpy(out->data + *pos, out->data + backward, length);
                  *pos += length;
              }
          }
          else if (code_ll == 256)
          {
              break; /*end code, break the loop*/
          }
          else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
          {
              /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
              (10=no endcode, 11=wrong jump outside of tree)*/
              error = ((*bp) > inlength * 8) ? 10 : 11;
              break;
          }
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

static unsigned inflateNoCompression(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  pos, size_t inlength)
{
  unsigned long p = 0/*auto-generated init val*/;
  unsigned int LEN = 0/*auto-generated init val*/; unsigned int NLEN = 0/*auto-generated init val*/; unsigned int n = 0/*auto-generated init val*/; unsigned int error = 0;

  /*go to first boundary of byte*/
  while(((*bp) & 0x7) != 0) ++(*bp);
  p = (*bp) / 8; /*byte position*/

  /*read LEN (2 bytes) and NLEN (2 bytes)*/
  if(p + 4 >= inlength) return 52; /*error, bit pointer will jump past memory*/
  LEN = in[p] + 256u * in[p + 1]; p += 2;
  NLEN = in[p] + 256u * in[p + 1]; p += 2;

  /*check if 16-bit NLEN is really the one's complement of LEN*/
  if(LEN + NLEN != 65535) return 21; /*error: NLEN is not one's complement of LEN*/

  if(!ucvector_resize(out, (*pos) + LEN)) return 83; /*alloc fail*/

  /*read the literal data: LEN bytes are now stored in the out buffer*/
  if(p + LEN > inlength) return 23; /*error: reading outside of in buffer*/
  for(n = 0; n < LEN; ++n) out->data[(*pos)++] = in[p++];

  (*bp) = p * 8;

  return error;
}

static unsigned lodepng_inflatev(mse::lh::TLHNullableAnyPointer<ucvector>  out,
                                 mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                                 mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  settings)
{
  /*bit pointer in the "in" data, current byte is bp >> 3, current bit is bp & 0x7 (from lsb to msb of the byte)*/
  mse::TXScopeObj<unsigned long > bp = 0;
  unsigned BFINAL = 0;
  mse::TXScopeObj<unsigned long > pos = 0; /*byte position in the out buffer*/
  unsigned error = 0;

  (void)settings;

  while(!BFINAL)
  {
    unsigned int BTYPE = 0/*auto-generated init val*/;
    if(bp + 2 >= insize * 8) return 52; /*error, bit pointer will jump past memory*/
    BFINAL = readBitFromStream(&bp, in);
    BTYPE = 1u * readBitFromStream(&bp, in);
    BTYPE += 2u * readBitFromStream(&bp, in);

    if(BTYPE == 3) return 20; /*error: invalid BTYPE*/
    else if(BTYPE == 0) error = inflateNoCompression(out, in, &bp, &pos, insize); /*no compression*/
    else error = inflateHuffmanBlock(out, in, &bp, &pos, insize, BTYPE); /*compression, BTYPE 01 or 10*/

    if(error) return error;
  }

  return error;
}

unsigned lodepng_inflate(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize,
                         mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                         mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  settings)
{
  unsigned int error = 0/*auto-generated init val*/;
  mse::TNoradObj<ucvector > v;
  ucvector_init_buffer(&v, *out, *outsize);
  error = lodepng_inflatev(&v, in, insize, settings);
  *out = v.data;
  *outsize = v.size;
  return error;
}

static unsigned inflate(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize,
                        mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                        mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  settings)
{
  if(settings->custom_inflate)
  {
    return settings->custom_inflate(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_inflate(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflator (Compressor)                                                  / */
/* ////////////////////////////////////////////////////////////////////////// */

static const size_t MAX_SUPPORTED_DEFLATE_LENGTH = 258;

/*bitlen is the size in bits of the code*/
static void addHuffmanSymbol(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp, mse::lh::TLHNullableAnyPointer<ucvector>  compressed, unsigned code, unsigned bitlen)
{
  addBitsToStreamReversed(bp, compressed, code, bitlen);
}

/*search the index in the array, that has the largest value smaller than or equal to the given value,
given array must be sorted (if no value is smaller, it returns the size of the given array)*/
static size_t searchCodeIndex(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned int>  array, size_t array_size, size_t value)
{
  /*binary search (only small gain over linear). TODO: use CPU log2 instruction for getting symbols instead*/
  size_t left = 1;
  size_t right = array_size - 1;

  while(left <= right) {
    size_t mid = (left + right) >> 1;
    if (array[mid] >= value) right = mid - 1;
    else left = mid + 1;
  }
  if(left >= array_size || array[left] > value) left--;
  return left;
}

static void addLengthDistance(mse::lh::TLHNullableAnyPointer<uivector>  values, size_t length, size_t distance)
{
  /*values in encoded vector are those used by deflate:
  0-255: literal bytes
  256: end
  257-285: length/distance pair (length code, followed by extra length bits, distance code, extra distance bits)
  286-287: invalid*/

  unsigned length_code = (unsigned)searchCodeIndex(LENGTHBASE, 29, length);
  unsigned extra_length = (unsigned)(length - LENGTHBASE[length_code]);
  unsigned dist_code = (unsigned)searchCodeIndex(DISTANCEBASE, 30, distance);
  unsigned extra_distance = (unsigned)(distance - DISTANCEBASE[dist_code]);

  uivector_push_back(values, length_code + FIRST_LENGTH_CODE_INDEX);
  uivector_push_back(values, extra_length);
  uivector_push_back(values, dist_code);
  uivector_push_back(values, extra_distance);
}

/*3 bytes of data get encoded into two bytes. The hash cannot use more than 3
bytes as input because 3 is the minimum match length for deflate*/
static const unsigned HASH_NUM_VALUES = 65536;
static const unsigned HASH_BIT_MASK = 65535; /*HASH_NUM_VALUES - 1, but C90 does not like that as initializer*/

typedef struct Hash
{
  mse::lh::TStrongVectorIterator<int>  head; /*hash value to head circular pos - can be outdated if went around window*/
  /*circular pos to prev circular pos*/
  mse::lh::TStrongVectorIterator<unsigned short>  chain;
  mse::lh::TStrongVectorIterator<int>  val; /*circular pos to hash value*/

  /*TODO: do this not only for zeros but for any repeated byte. However for PNG
  it's always going to be the zeros that dominate, so not important for PNG*/
  mse::lh::TStrongVectorIterator<int>  headz; /*similar to head, but for chainz*/
  mse::lh::TStrongVectorIterator<unsigned short>  chainz; /*those with same amount of zeros*/
  mse::lh::TStrongVectorIterator<unsigned short>  zeros; /*length of zeros streak, used as a second hash chain*/
} Hash;

static unsigned hash_init(mse::lh::TLHNullableAnyPointer<Hash>  hash, unsigned windowsize)
{
  unsigned int i = 0/*auto-generated init val*/;
  mse::lh::allocate(hash->head, (sizeof(int) * HASH_NUM_VALUES));
  mse::lh::allocate(hash->val, (sizeof(int) * windowsize));
  mse::lh::allocate(hash->chain, (sizeof(unsigned short) * windowsize));

  mse::lh::allocate(hash->zeros, (sizeof(unsigned short) * windowsize));
  mse::lh::allocate(hash->headz, (sizeof(int) * (MAX_SUPPORTED_DEFLATE_LENGTH + 1)));
  mse::lh::allocate(hash->chainz, (sizeof(unsigned short) * windowsize));

  if(!hash->head || !hash->chain || !hash->val  || !hash->headz|| !hash->chainz || !hash->zeros)
  {
    return 83; /*alloc fail*/
  }

  /*initialize hash table*/
  for(i = 0; i != HASH_NUM_VALUES; ++i) hash->head[i] = -1;
  for(i = 0; i != windowsize; ++i) hash->val[i] = -1;
  for(i = 0; i != windowsize; ++i) hash->chain[i] = i; /*same value as index indicates uninitialized*/

  for(i = 0; i <= MAX_SUPPORTED_DEFLATE_LENGTH; ++i) hash->headz[i] = -1;
  for(i = 0; i != windowsize; ++i) hash->chainz[i] = i; /*same value as index indicates uninitialized*/

  return 0;
}

static void hash_cleanup(mse::lh::TXScopeLHNullableAnyPointer<Hash>  hash)
{
  mse::lh::free(hash->head);
  mse::lh::free(hash->val);
  mse::lh::free(hash->chain);

  mse::lh::free(hash->zeros);
  mse::lh::free(hash->headz);
  mse::lh::free(hash->chainz);
}



static unsigned getHash(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t size, size_t pos)
{
  unsigned result = 0;
  if(pos + 2 < size)
  {
    /*A simple shift and xor hash is used. Since the data of PNGs is dominated
    by zeroes due to the filters, a better hash does not have a significant
    effect on speed in traversing the chain, and causes more time spend on
    calculating the hash.*/
    result ^= (unsigned)(data[pos + 0] << 0u);
    result ^= (unsigned)(data[pos + 1] << 4u);
    result ^= (unsigned)(data[pos + 2] << 8u);
  } else {
    unsigned long amount = 0/*auto-generated init val*/; unsigned long i = 0/*auto-generated init val*/;
    if(pos >= size) return 0;
    amount = size - pos;
    for(i = 0; i != amount; ++i) result ^= (unsigned)(data[pos + i] << (i * 8u));
  }
  return result & HASH_BIT_MASK;
}

static unsigned countZeros(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t size, size_t pos)
{
  mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  start = data + pos;
  mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  end = start + MAX_SUPPORTED_DEFLATE_LENGTH;
  if(end > data + size) end = data + size;
  data = start;
  while(data != end && *data == 0) ++data;
  /*subtracting two addresses returned as 32-bit number (max value is MAX_SUPPORTED_DEFLATE_LENGTH)*/
  return (unsigned)(data - start);
}

/*wpos = pos & (windowsize - 1)*/
static void updateHashChain(mse::lh::TLHNullableAnyPointer<Hash>  hash, size_t wpos, unsigned hashval, unsigned short numzeros)
{
  hash->val[wpos] = (int)hashval;
  if(hash->head[hashval] != -1) hash->chain[wpos] = hash->head[hashval];
  hash->head[hashval] = wpos;

  hash->zeros[wpos] = numzeros;
  if(hash->headz[numzeros] != -1) hash->chainz[wpos] = hash->headz[numzeros];
  hash->headz[numzeros] = wpos;
}

/*
LZ77-encode the data. Return value is error code. The input are raw bytes, the output
is in the form of unsigned integers with codes representing for example literal bytes, or
length/distance pairs.
It uses a hash table technique to let it encode faster. When doing LZ77 encoding, a
sliding window (of windowsize) is used, and all past bytes in that window can be used as
the "dictionary". A brute force search through all possible distances would be slow, and
this hash technique is one out of several ways to speed this up.
*/
static unsigned encodeLZ77(mse::lh::TLHNullableAnyPointer<uivector>  out, mse::lh::TLHNullableAnyPointer<Hash>  hash,
                           mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t inpos, size_t insize, unsigned windowsize,
                           unsigned minmatch, unsigned nicematch, unsigned lazymatching)
{
  unsigned long pos = 0/*auto-generated init val*/;
  unsigned int i = 0/*auto-generated init val*/; unsigned int error = 0;
  /*for large window lengths, assume the user wants no compression loss. Otherwise, max hash chain length speedup.*/
  unsigned maxchainlength = windowsize >= 8192 ? windowsize : windowsize / 8;
  unsigned maxlazymatch = windowsize >= 8192 ? MAX_SUPPORTED_DEFLATE_LENGTH : 64;

  unsigned usezeros = 1; /*not sure if setting it to false for windowsize < 8192 is better or worse*/
  unsigned numzeros = 0;

  unsigned int offset = 0/*auto-generated init val*/; /*the offset represents the distance in LZ77 terminology*/
  unsigned int length = 0/*auto-generated init val*/;
  unsigned lazy = 0;
  unsigned lazylength = 0, lazyoffset = 0;
  unsigned int hashval = 0/*auto-generated init val*/;
  unsigned int current_offset = 0/*auto-generated init val*/; unsigned int current_length = 0/*auto-generated init val*/;
  unsigned int prev_offset = 0/*auto-generated init val*/;
  mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  lastptr = nullptr/*auto-generated init val*/; mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  foreptr = nullptr/*auto-generated init val*/; mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  backptr = nullptr/*auto-generated init val*/;
  unsigned int hashpos = 0/*auto-generated init val*/;

  if(windowsize == 0 || windowsize > 32768) return 60; /*error: windowsize smaller/larger than allowed*/
  if((windowsize & (windowsize - 1)) != 0) return 90; /*error: must be power of two*/

  if(nicematch > MAX_SUPPORTED_DEFLATE_LENGTH) nicematch = MAX_SUPPORTED_DEFLATE_LENGTH;

  for(pos = inpos; pos < insize; ++pos)
  {
    size_t wpos = pos & (windowsize - 1); /*position for in 'circular' hash buffers*/
    unsigned chainlength = 0;

    hashval = getHash(in, insize, pos);

    if(usezeros && hashval == 0)
    {
      if(numzeros == 0) numzeros = countZeros(in, insize, pos);
      else if(pos + numzeros > insize || in[pos + numzeros - 1] != 0) --numzeros;
    }
    else
    {
      numzeros = 0;
    }

    updateHashChain(hash, wpos, hashval, numzeros);

    /*the length and offset found for the current position*/
    length = 0;
    offset = 0;

    hashpos = hash->chain[wpos];

    lastptr = ((in) + (insize < pos + MAX_SUPPORTED_DEFLATE_LENGTH ? insize : pos + MAX_SUPPORTED_DEFLATE_LENGTH));

    /*search for the longest string*/
    prev_offset = 0;
    for(;;)
    {
      if(chainlength++ >= maxchainlength) break;
      current_offset = hashpos <= wpos ? wpos - hashpos : wpos - hashpos + windowsize;

      if(current_offset < prev_offset) break; /*stop when went completely around the circular buffer*/
      prev_offset = current_offset;
      if(current_offset > 0)
      {
        /*test the next characters*/
        foreptr = ((in) + (pos));
        backptr = ((in) + (pos - current_offset));

        /*common case in PNGs is lots of zeros. Quickly skip over them as a speedup*/
        if(numzeros >= 3)
        {
          unsigned skip = hash->zeros[hashpos];
          if(skip > numzeros) skip = numzeros;
          backptr += skip;
          foreptr += skip;
        }

        while(foreptr != lastptr && *backptr == *foreptr) /*maximum supported length by deflate is max length*/
        {
          ++backptr;
          ++foreptr;
        }
        current_length = (unsigned)(foreptr - ((in) + (pos)));

        if(current_length > length)
        {
          length = current_length; /*the longest length*/
          offset = current_offset; /*the offset that is related to this longest length*/
          /*jump out once a length of max length is found (speed gain). This also jumps
          out if length is MAX_SUPPORTED_DEFLATE_LENGTH*/
          if(current_length >= nicematch) break;
        }
      }

      if(hashpos == hash->chain[hashpos]) break;

      if(numzeros >= 3 && length > numzeros)
      {
        hashpos = hash->chainz[hashpos];
        if(hash->zeros[hashpos] != numzeros) break;
      }
      else
      {
        hashpos = hash->chain[hashpos];
        /*outdated hash value, happens if particular value was not encountered in whole last window*/
        if(hash->val[hashpos] != (int)hashval) break;
      }
    }

    if(lazymatching)
    {
      if(!lazy && length >= 3 && length <= maxlazymatch && length < MAX_SUPPORTED_DEFLATE_LENGTH)
      {
        lazy = 1;
        lazylength = length;
        lazyoffset = offset;
        continue; /*try the next byte*/
      }
      if(lazy)
      {
        lazy = 0;
        if(pos == 0) ERROR_BREAK(81);
        if(length > lazylength + 1)
        {
          /*push the previous character as literal*/
          if(!uivector_push_back(out, in[pos - 1])) ERROR_BREAK(83 /*alloc fail*/);
        }
        else
        {
          length = lazylength;
          offset = lazyoffset;
          hash->head[hashval] = -1; /*the same hashchain update will be done, this ensures no wrong alteration*/
          hash->headz[numzeros] = -1; /*idem*/
          --pos;
        }
      }
    }
    if(length >= 3 && offset > windowsize) ERROR_BREAK(86 /*too big (or overflown negative) offset*/);

    /*encode it as length/distance pair or literal value*/
    if(length < 3) /*only lengths of 3 or higher are supported as length/distance pair*/
    {
      if(!uivector_push_back(out, in[pos])) ERROR_BREAK(83 /*alloc fail*/);
    }
    else if(length < minmatch || (length == 3 && offset > 4096))
    {
      /*compensate for the fact that longer offsets have more extra bits, a
      length of only 3 may be not worth it then*/
      if(!uivector_push_back(out, in[pos])) ERROR_BREAK(83 /*alloc fail*/);
    }
    else
    {
      addLengthDistance(out, length, offset);
      for(i = 1; i < length; ++i)
      {
        ++pos;
        wpos = pos & (windowsize - 1);
        hashval = getHash(in, insize, pos);
        if(usezeros && hashval == 0)
        {
          if(numzeros == 0) numzeros = countZeros(in, insize, pos);
          else if(pos + numzeros > insize || in[pos + numzeros - 1] != 0) --numzeros;
        }
        else
        {
          numzeros = 0;
        }
        updateHashChain(hash, wpos, hashval, numzeros);
      }
    }
  } /*end of the loop through each character of input*/

  return error;
}

/* /////////////////////////////////////////////////////////////////////////// */

static unsigned deflateNoCompression(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t datasize)
{
  /*non compressed deflate block data: 1 bit BFINAL,2 bits BTYPE,(5 bits): it jumps to start of next byte,
  2 bytes LEN, 2 bytes NLEN, LEN bytes literal DATA*/

  unsigned long i = 0/*auto-generated init val*/; unsigned long j = 0/*auto-generated init val*/; unsigned long numdeflateblocks = (datasize + 65534) / 65535;
  unsigned datapos = 0;
  for(i = 0; i != numdeflateblocks; ++i)
  {
    unsigned int BFINAL = 0/*auto-generated init val*/; unsigned int BTYPE = 0/*auto-generated init val*/; unsigned int LEN = 0/*auto-generated init val*/; unsigned int NLEN = 0/*auto-generated init val*/;
    unsigned char firstbyte = 0/*auto-generated init val*/;

    BFINAL = (i == numdeflateblocks - 1);
    BTYPE = 0;

    firstbyte = (unsigned char)(BFINAL + ((BTYPE & 1) << 1) + ((BTYPE & 2) << 1));
    ucvector_push_back(out, firstbyte);

    LEN = 65535;
    if(datasize - datapos < 65535) LEN = (unsigned)datasize - datapos;
    NLEN = 65535 - LEN;

    ucvector_push_back(out, (unsigned char)(LEN & 255));
    ucvector_push_back(out, (unsigned char)(LEN >> 8));
    ucvector_push_back(out, (unsigned char)(NLEN & 255));
    ucvector_push_back(out, (unsigned char)(NLEN >> 8));

    /*Decompressed data*/
    for(j = 0; j < 65535 && datapos < datasize; ++j)
    {
      ucvector_push_back(out, data[datapos++]);
    }
  }

  return 0;
}

/*
write the lz77-encoded data, which has lit, len and dist codes, to compressed stream using huffman trees.
tree_ll: the tree for lit and len codes.
tree_d: the tree for distance codes.
*/
static void writeLZ77data(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp, mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyPointer<const uivector>  lz77_encoded,
                          mse::lh::TLHNullableAnyPointer<const HuffmanTree>  tree_ll, mse::lh::TLHNullableAnyPointer<const HuffmanTree>  tree_d)
{
  size_t i = 0;
  for(i = 0; i != lz77_encoded->size; ++i)
  {
    unsigned val = lz77_encoded->data[i];
    addHuffmanSymbol(bp, out, HuffmanTree_getCode(tree_ll, val), HuffmanTree_getLength(tree_ll, val));
    if(val > 256) /*for a length code, 3 more things have to be added*/
    {
      unsigned length_index = val - FIRST_LENGTH_CODE_INDEX;
      unsigned n_length_extra_bits = LENGTHEXTRA[length_index];
      unsigned length_extra_bits = lz77_encoded->data[++i];

      unsigned distance_code = lz77_encoded->data[++i];

      unsigned distance_index = distance_code;
      unsigned n_distance_extra_bits = DISTANCEEXTRA[distance_index];
      unsigned distance_extra_bits = lz77_encoded->data[++i];

      addBitsToStream(bp, out, length_extra_bits, n_length_extra_bits);
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(tree_d, distance_code),
                       HuffmanTree_getLength(tree_d, distance_code));
      addBitsToStream(bp, out, distance_extra_bits, n_distance_extra_bits);
    }
  }
}

/*Deflate for a block of type "dynamic", that is, with freely, optimally, created huffman trees*/
static unsigned deflateDynamic(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp, mse::lh::TLHNullableAnyPointer<Hash>  hash,
                               mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t datapos, size_t dataend,
                               mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings, unsigned final)
{
  unsigned error = 0;

  /*
  A block is compressed as follows: The PNG data is lz77 encoded, resulting in
  literal bytes and length/distance pairs. This is then huffman compressed with
  two huffman trees. One huffman tree is used for the lit and len values ("ll"),
  another huffman tree is used for the dist values ("d"). These two trees are
  stored using their code lengths, and to compress even more these code lengths
  are also run-length encoded and huffman compressed. This gives a huffman tree
  of code lengths "cl". The code lenghts used to describe this third tree are
  the code length code lengths ("clcl").
  */

  /*The lz77 encoded data, represented with integers since there will also be length and distance codes in it*/
  mse::TNoradObj<uivector > lz77_encoded;
  mse::TNoradObj<HuffmanTree > tree_ll; /*tree for lit,len values*/
  mse::TNoradObj<HuffmanTree > tree_d; /*tree for distance codes*/
  mse::TNoradObj<HuffmanTree > tree_cl; /*tree for encoding the code lengths representing tree_ll and tree_d*/
  mse::TNoradObj<uivector > frequencies_ll; /*frequency of lit,len codes*/
  mse::TNoradObj<uivector > frequencies_d; /*frequency of dist codes*/
  mse::TNoradObj<uivector > frequencies_cl; /*frequency of code length codes*/
  mse::TNoradObj<uivector > bitlen_lld; /*lit,len,dist code lenghts (int bits), literally (without repeat codes).*/
  mse::TNoradObj<uivector > bitlen_lld_e; /*bitlen_lld encoded with repeat codes (this is a rudemtary run length compression)*/
  /*bitlen_cl is the code length code lengths ("clcl"). The bit lengths of codes to represent tree_cl
  (these are written as is in the file, it would be crazy to compress these using yet another huffman
  tree that needs to be represented by yet another set of code lengths)*/
  mse::TNoradObj<uivector > bitlen_cl;
  size_t datasize = dataend - datapos;

  /*
  Due to the huffman compression of huffman tree representations ("two levels"), there are some anologies:
  bitlen_lld is to tree_cl what data is to tree_ll and tree_d.
  bitlen_lld_e is to bitlen_lld what lz77_encoded is to data.
  bitlen_cl is to bitlen_lld_e what bitlen_lld is to lz77_encoded.
  */

  unsigned BFINAL = final;
  unsigned long numcodes_ll = 0/*auto-generated init val*/; unsigned long numcodes_d = 0/*auto-generated init val*/; unsigned long i = 0/*auto-generated init val*/;
  unsigned int HLIT = 0/*auto-generated init val*/; unsigned int HDIST = 0/*auto-generated init val*/; unsigned int HCLEN = 0/*auto-generated init val*/;

  uivector_init(&lz77_encoded);
  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);
  HuffmanTree_init(&tree_cl);
  uivector_init(&frequencies_ll);
  uivector_init(&frequencies_d);
  uivector_init(&frequencies_cl);
  uivector_init(&bitlen_lld);
  uivector_init(&bitlen_lld_e);
  uivector_init(&bitlen_cl);

  /*This while loop never loops due to a break at the end, it is here to
  allow breaking out of it to the cleanup phase on error conditions.*/
  while(!error)
  {
    if(settings->use_lz77)
    {
      error = encodeLZ77(&lz77_encoded, hash, data, datapos, dataend, settings->windowsize,
                         settings->minmatch, settings->nicematch, settings->lazymatching);
      if(error) break;
    }
    else
    {
      if(!uivector_resize(&lz77_encoded, datasize)) ERROR_BREAK(83 /*alloc fail*/);
      for(i = datapos; i < dataend; ++i) lz77_encoded.data[i - datapos] = data[i]; /*no LZ77, but still will be Huffman compressed*/
    }

    if(!uivector_resizev(&frequencies_ll, 286, 0)) ERROR_BREAK(83 /*alloc fail*/);
    if(!uivector_resizev(&frequencies_d, 30, 0)) ERROR_BREAK(83 /*alloc fail*/);

    /*Count the frequencies of lit, len and dist codes*/
    for(i = 0; i != lz77_encoded.size; ++i)
    {
      unsigned symbol = lz77_encoded.data[i];
      ++frequencies_ll.data[symbol];
      if(symbol > 256)
      {
        unsigned dist = lz77_encoded.data[i + 2];
        ++frequencies_d.data[dist];
        i += 3;
      }
    }
    frequencies_ll.data[256] = 1; /*there will be exactly 1 end code, at the end of the block*/

    /*Make both huffman trees, one for the lit and len codes, one for the dist codes*/
    error = HuffmanTree_makeFromFrequencies(&tree_ll, frequencies_ll.data, 257, frequencies_ll.size, 15);
    if(error) break;
    /*2, not 1, is chosen for mincodes: some buggy PNG decoders require at least 2 symbols in the dist tree*/
    error = HuffmanTree_makeFromFrequencies(&tree_d, frequencies_d.data, 2, frequencies_d.size, 15);
    if(error) break;

    numcodes_ll = tree_ll.numcodes; if(numcodes_ll > 286) numcodes_ll = 286;
    numcodes_d = tree_d.numcodes; if(numcodes_d > 30) numcodes_d = 30;
    /*store the code lengths of both generated trees in bitlen_lld*/
    for(i = 0; i != numcodes_ll; ++i) uivector_push_back(&bitlen_lld, HuffmanTree_getLength(&tree_ll, (unsigned)i));
    for(i = 0; i != numcodes_d; ++i) uivector_push_back(&bitlen_lld, HuffmanTree_getLength(&tree_d, (unsigned)i));

    /*run-length compress bitlen_ldd into bitlen_lld_e by using repeat codes 16 (copy length 3-6 times),
    17 (3-10 zeroes), 18 (11-138 zeroes)*/
    for(i = 0; i != (unsigned)bitlen_lld.size; ++i)
    {
      unsigned j = 0; /*amount of repititions*/
      while(i + j + 1 < (unsigned)bitlen_lld.size && bitlen_lld.data[i + j + 1] == bitlen_lld.data[i]) ++j;

      if(bitlen_lld.data[i] == 0 && j >= 2) /*repeat code for zeroes*/
      {
        ++j; /*include the first zero*/
        if(j <= 10) /*repeat code 17 supports max 10 zeroes*/
        {
          uivector_push_back(&bitlen_lld_e, 17);
          uivector_push_back(&bitlen_lld_e, j - 3);
        }
        else /*repeat code 18 supports max 138 zeroes*/
        {
          if(j > 138) j = 138;
          uivector_push_back(&bitlen_lld_e, 18);
          uivector_push_back(&bitlen_lld_e, j - 11);
        }
        i += (j - 1);
      }
      else if(j >= 3) /*repeat code for value other than zero*/
      {
        unsigned long k = 0/*auto-generated init val*/;
        unsigned num = j / 6, rest = j % 6;
        uivector_push_back(&bitlen_lld_e, bitlen_lld.data[i]);
        for(k = 0; k < num; ++k)
        {
          uivector_push_back(&bitlen_lld_e, 16);
          uivector_push_back(&bitlen_lld_e, 6 - 3);
        }
        if(rest >= 3)
        {
          uivector_push_back(&bitlen_lld_e, 16);
          uivector_push_back(&bitlen_lld_e, rest - 3);
        }
        else j -= rest;
        i += j;
      }
      else /*too short to benefit from repeat code*/
      {
        uivector_push_back(&bitlen_lld_e, bitlen_lld.data[i]);
      }
    }

    /*generate tree_cl, the huffmantree of huffmantrees*/

    if(!uivector_resizev(&frequencies_cl, NUM_CODE_LENGTH_CODES, 0)) ERROR_BREAK(83 /*alloc fail*/);
    for(i = 0; i != bitlen_lld_e.size; ++i)
    {
      ++frequencies_cl.data[bitlen_lld_e.data[i]];
      /*after a repeat code come the bits that specify the number of repetitions,
      those don't need to be in the frequencies_cl calculation*/
      if(bitlen_lld_e.data[i] >= 16) ++i;
    }

    error = HuffmanTree_makeFromFrequencies(&tree_cl, frequencies_cl.data,
                                            frequencies_cl.size, frequencies_cl.size, 7);
    if(error) break;

    if(!uivector_resize(&bitlen_cl, tree_cl.numcodes)) ERROR_BREAK(83 /*alloc fail*/);
    for(i = 0; i != tree_cl.numcodes; ++i)
    {
      /*lenghts of code length tree is in the order as specified by deflate*/
      bitlen_cl.data[i] = HuffmanTree_getLength(&tree_cl, CLCL_ORDER[i]);
    }
    while(bitlen_cl.data[bitlen_cl.size - 1] == 0 && bitlen_cl.size > 4)
    {
      /*remove zeros at the end, but minimum size must be 4*/
      if(!uivector_resize(&bitlen_cl, bitlen_cl.size - 1)) ERROR_BREAK(83 /*alloc fail*/);
    }
    if(error) break;

    /*
    Write everything into the output

    After the BFINAL and BTYPE, the dynamic block consists out of the following:
    - 5 bits HLIT, 5 bits HDIST, 4 bits HCLEN
    - (HCLEN+4)*3 bits code lengths of code length alphabet
    - HLIT + 257 code lenghts of lit/length alphabet (encoded using the code length
      alphabet, + possible repetition codes 16, 17, 18)
    - HDIST + 1 code lengths of distance alphabet (encoded using the code length
      alphabet, + possible repetition codes 16, 17, 18)
    - compressed data
    - 256 (end code)
    */

    /*Write block type*/
    addBitToStream(bp, out, BFINAL);
    addBitToStream(bp, out, 0); /*first bit of BTYPE "dynamic"*/
    addBitToStream(bp, out, 1); /*second bit of BTYPE "dynamic"*/

    /*write the HLIT, HDIST and HCLEN values*/
    HLIT = (unsigned)(numcodes_ll - 257);
    HDIST = (unsigned)(numcodes_d - 1);
    HCLEN = (unsigned)bitlen_cl.size - 4;
    /*trim zeroes for HCLEN. HLIT and HDIST were already trimmed at tree creation*/
    while(!bitlen_cl.data[HCLEN + 4 - 1] && HCLEN > 0) --HCLEN;
    addBitsToStream(bp, out, HLIT, 5);
    addBitsToStream(bp, out, HDIST, 5);
    addBitsToStream(bp, out, HCLEN, 4);

    /*write the code lenghts of the code length alphabet*/
    for(i = 0; i != HCLEN + 4; ++i) addBitsToStream(bp, out, bitlen_cl.data[i], 3);

    /*write the lenghts of the lit/len AND the dist alphabet*/
    for(i = 0; i != bitlen_lld_e.size; ++i)
    {
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_cl, bitlen_lld_e.data[i]),
                       HuffmanTree_getLength(&tree_cl, bitlen_lld_e.data[i]));
      /*extra bits of repeat codes*/
      if(bitlen_lld_e.data[i] == 16) addBitsToStream(bp, out, bitlen_lld_e.data[++i], 2);
      else if(bitlen_lld_e.data[i] == 17) addBitsToStream(bp, out, bitlen_lld_e.data[++i], 3);
      else if(bitlen_lld_e.data[i] == 18) addBitsToStream(bp, out, bitlen_lld_e.data[++i], 7);
    }

    /*write the compressed data symbols*/
    writeLZ77data(bp, out, &lz77_encoded, &tree_ll, &tree_d);
    /*error: the length of the end code 256 must be larger than 0*/
    if(HuffmanTree_getLength(&tree_ll, 256) == 0) ERROR_BREAK(64);

    /*write the end code*/
    addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_ll, 256), HuffmanTree_getLength(&tree_ll, 256));

    break; /*end of error-while*/
  }

  /*cleanup*/
  uivector_cleanup(&lz77_encoded);
  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);
  HuffmanTree_cleanup(&tree_cl);
  uivector_cleanup(&frequencies_ll);
  uivector_cleanup(&frequencies_d);
  uivector_cleanup(&frequencies_cl);
  uivector_cleanup(&bitlen_lld_e);
  uivector_cleanup(&bitlen_lld);
  uivector_cleanup(&bitlen_cl);

  return error;
}

static unsigned deflateFixed(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bp, mse::lh::TLHNullableAnyPointer<Hash>  hash,
                             mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data,
                             size_t datapos, size_t dataend,
                             mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings, unsigned final)
{
  mse::TNoradObj<HuffmanTree > tree_ll; /*tree for literal values and length codes*/
  mse::TNoradObj<HuffmanTree > tree_d; /*tree for distance codes*/

  unsigned BFINAL = final;
  unsigned error = 0;
  unsigned long i = 0/*auto-generated init val*/;

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  generateFixedLitLenTree(&tree_ll);
  generateFixedDistanceTree(&tree_d);

  addBitToStream(bp, out, BFINAL);
  addBitToStream(bp, out, 1); /*first bit of BTYPE*/
  addBitToStream(bp, out, 0); /*second bit of BTYPE*/

  if(settings->use_lz77) /*LZ77 encoded*/
  {
    mse::TNoradObj<uivector > lz77_encoded;
    uivector_init(&lz77_encoded);
    error = encodeLZ77(&lz77_encoded, hash, data, datapos, dataend, settings->windowsize,
                       settings->minmatch, settings->nicematch, settings->lazymatching);
    if(!error) writeLZ77data(bp, out, &lz77_encoded, &tree_ll, &tree_d);
    uivector_cleanup(&lz77_encoded);
  }
  else /*no LZ77, but still will be Huffman compressed*/
  {
    for(i = datapos; i < dataend; ++i)
    {
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_ll, data[i]), HuffmanTree_getLength(&tree_ll, data[i]));
    }
  }
  /*add END code*/
  if(!error) addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_ll, 256), HuffmanTree_getLength(&tree_ll, 256));

  /*cleanup*/
  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}

static unsigned lodepng_deflatev(mse::lh::TLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                                 mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings)
{
  unsigned error = 0;
  unsigned long i = 0/*auto-generated init val*/; unsigned long blocksize = 0/*auto-generated init val*/; unsigned long numdeflateblocks = 0/*auto-generated init val*/;
  mse::TNoradObj<unsigned long > bp = 0; /*the bit pointer*/
  mse::TNoradObj<Hash > hash;

  if(settings->btype > 2) return 61;
  else if(settings->btype == 0) return deflateNoCompression(out, in, insize);
  else if(settings->btype == 1) blocksize = insize;
  else /*if(settings->btype == 2)*/
  {
    /*on PNGs, deflate blocks of 65-262k seem to give most dense encoding*/
    blocksize = insize / 8 + 8;
    if(blocksize < 65536) blocksize = 65536;
    if(blocksize > 262144) blocksize = 262144;
  }

  numdeflateblocks = (insize + blocksize - 1) / blocksize;
  if(numdeflateblocks == 0) numdeflateblocks = 1;

  error = hash_init(&hash, settings->windowsize);
  if(error) return error;

  for(i = 0; i != numdeflateblocks && !error; ++i)
  {
    unsigned final = (i == numdeflateblocks - 1);
    size_t start = i * blocksize;
    size_t end = start + blocksize;
    if(end > insize) end = insize;

    if(settings->btype == 1) error = deflateFixed(out, &bp, &hash, in, start, end, settings, final);
    else if(settings->btype == 2) error = deflateDynamic(out, &bp, &hash, in, start, end, settings, final);
  }

  hash_cleanup(&hash);

  return error;
}

unsigned lodepng_deflate(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize,
                         mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                         mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings)
{
  unsigned int error = 0/*auto-generated init val*/;
  mse::TNoradObj<ucvector > v;
  ucvector_init_buffer(&v, *out, *outsize);
  error = lodepng_deflatev(&v, in, insize, settings);
  *out = v.data;
  *outsize = v.size;
  return error;
}

static unsigned deflate(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize,
                        mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                        mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings)
{
  if(settings->custom_deflate)
  {
    return settings->custom_deflate(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_deflate(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / Adler32                                                                  */
/* ////////////////////////////////////////////////////////////////////////// */

template<typename TData>
static unsigned update_adler32(unsigned adler, TData data, unsigned len)
{
   unsigned s1 = adler & 0xffff;
   unsigned s2 = (adler >> 16) & 0xffff;

  while(len > 0)
  {
    /*at least 5550 sums can be done before the sums overflow, saving a lot of module divisions*/
    unsigned amount = len > 5550 ? 5550 : len;
    len -= amount;
    while(amount > 0)
    {
      if (false) {
          s1 += (*data++);
      }
      else {
          s1 += (*data);
          ++data;
      }
      s2 += s1;
      --amount;
    }
    s1 %= 65521;
    s2 %= 65521;
  }

  return (s2 << 16) | s1;
}

/*Return the adler32 of the bytes data[0..len-1]*/
template<typename TData>
static unsigned adler32(TData data, unsigned len)
{
  return update_adler32(1L, data, len);
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Zlib                                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DECODER

unsigned lodepng_zlib_decompress(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                                 size_t insize, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  settings)
{
  unsigned error = 0;
  unsigned int CM = 0/*auto-generated init val*/; unsigned int CINFO = 0/*auto-generated init val*/; unsigned int FDICT = 0/*auto-generated init val*/;

  if(insize < 2) return 53; /*error, size of zlib data too small*/
  /*read information from zlib header*/
  if((in[0] * 256 + in[1]) % 31 != 0)
  {
    /*error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way*/
    return 24;
  }

  CM = in[0] & 15;
  CINFO = (in[0] >> 4) & 15;
  /*FCHECK = in[1] & 31;*/ /*FCHECK is already tested above*/
  FDICT = (in[1] >> 5) & 1;
  /*FLEVEL = (in[1] >> 6) & 3;*/ /*FLEVEL is not used here*/

  if(CM != 8 || CINFO > 7)
  {
    /*error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec*/
    return 25;
  }
  if(FDICT != 0)
  {
    /*error: the specification of PNG says about the zlib stream:
      "The additional flags shall not specify a preset dictionary."*/
    return 26;
  }

  auto do1 = [&](auto in, auto out, auto outsize) -> unsigned {
      error = inflate(out, outsize, in + 2, insize - 2, settings);
      if (error) return error;

      if (!settings->ignore_adler32)
      {
          unsigned ADLER32 = lodepng_read32bitInt(((in)+(insize - 4)));
          unsigned checksum = adler32(*out, (unsigned)(*outsize));
          if (checksum != ADLER32) return 58; /*error, adler checksum not correct, data must be corrupted*/
      }

      return 0; /*no error*/
  };

  auto maybe_in_sviter = mse::maybe_any_cast<mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char> >>(in);
  auto maybe_out_nnnptr = mse::maybe_any_cast<mse::TNoradNotNullPointer<mse::lh::TStrongVectorIterator<unsigned char> >>(out);
  auto maybe_outsize_nnnptr = mse::maybe_any_cast<mse::TNoradNotNullPointer<mse::TInt<unsigned long> >>(outsize);
  if (maybe_in_sviter.has_value() && maybe_out_nnnptr.has_value() && maybe_outsize_nnnptr.has_value()) {
      auto in_sviter = maybe_in_sviter.value();
      auto out_xs_store = mse::make_xscope_strong_pointer_store(maybe_out_nnnptr.value());
      auto out_xsptr = out_xs_store.xscope_ptr();
      auto outsize_xs_store = mse::make_xscope_strong_pointer_store(maybe_outsize_nnnptr.value());
      auto outsize_xsptr = outsize_xs_store.xscope_ptr();

      return do1(in_sviter, out_xsptr, outsize_xsptr);
  }
  return do1(in, out, outsize);
}

static unsigned zlib_decompress(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                                size_t insize, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  settings)
{
  if(settings->custom_zlib)
  {
    return settings->custom_zlib(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_zlib_decompress(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

unsigned lodepng_zlib_compress(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                               size_t insize, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings)
{
  /*initially, *out must be NULL and outsize 0, if you just give some random *out
  that's pointing to a non allocated buffer, this'll crash*/
  mse::TNoradObj<ucvector > outv;
  unsigned long i = 0/*auto-generated init val*/;
  unsigned int error = 0/*auto-generated init val*/;
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > deflatedata = nullptr;
  mse::TNoradObj<unsigned long > deflatesize = 0;

  /*zlib data: 1 byte CMF (CM+CINFO), 1 byte FLG, deflate data, 4 byte ADLER32 checksum of the Decompressed data*/
  unsigned CMF = 120; /*0b01111000: CM 8, CINFO 7. With CINFO 7, any window size up to 32768 can be used.*/
  unsigned FLEVEL = 0;
  unsigned FDICT = 0;
  unsigned CMFFLG = 256 * CMF + FDICT * 32 + FLEVEL * 64;
  unsigned FCHECK = 31 - CMFFLG % 31;
  CMFFLG += FCHECK;

  /*ucvector-controlled version of the output buffer, for dynamic array*/
  ucvector_init_buffer(&outv, *out, *outsize);

  ucvector_push_back(&outv, (unsigned char)(CMFFLG >> 8));
  ucvector_push_back(&outv, (unsigned char)(CMFFLG & 255));

  error = deflate(&deflatedata, &deflatesize, in, insize, settings);

  if(!error)
  {
    unsigned ADLER32 = adler32(in, (unsigned)insize);
    for(i = 0; i != deflatesize; ++i) ucvector_push_back(&outv, deflatedata[i]);
    mse::lh::free(deflatedata);
    lodepng_add32bitInt(&outv, ADLER32);
  }

  *out = outv.data;
  *outsize = outv.size;

  return error;
}

/* compress using the default or custom zlib function */
static unsigned zlib_compress(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                              size_t insize, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGCompressSettings>  settings)
{
  if(settings->custom_zlib)
  {
    return settings->custom_zlib(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_zlib_compress(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_ENCODER*/

#else /*no LODEPNG_COMPILE_ZLIB*/

#ifdef LODEPNG_COMPILE_DECODER
static unsigned zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                size_t insize, const LodePNGDecompressSettings* settings)
{
  if(!settings->custom_zlib) return 87; /*no custom zlib function provided */
  return settings->custom_zlib(out, outsize, in, insize, settings);
}
#endif /*LODEPNG_COMPILE_DECODER*/
#ifdef LODEPNG_COMPILE_ENCODER
static unsigned zlib_compress(unsigned char** out, size_t* outsize, const unsigned char* in,
                              size_t insize, const LodePNGCompressSettings* settings)
{
  if(!settings->custom_zlib) return 87; /*no custom zlib function provided */
  return settings->custom_zlib(out, outsize, in, insize, settings);
}
#endif /*LODEPNG_COMPILE_ENCODER*/

#endif /*LODEPNG_COMPILE_ZLIB*/

/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ENCODER

/*this is a good tradeoff between speed and compression ratio*/
#define DEFAULT_WINDOWSIZE 2048

void lodepng_compress_settings_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGCompressSettings>  settings)
{
  /*compress with dynamic huffman tree (not in the mathematical sense, just not the predefined one)*/
  settings->btype = 2;
  settings->use_lz77 = 1;
  settings->windowsize = DEFAULT_WINDOWSIZE;
  settings->minmatch = 3;
  settings->nicematch = 128;
  settings->lazymatching = 1;

  settings->custom_zlib = 0;
  settings->custom_deflate = 0;
  settings->custom_context = 0;
}

const LodePNGCompressSettings lodepng_default_compress_settings = {2, 1, DEFAULT_WINDOWSIZE, 3, 128, 1, 0, 0, 0};


#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_DECODER

void lodepng_decompress_settings_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGDecompressSettings>  settings)
{
  settings->ignore_adler32 = 0;

  settings->custom_zlib = 0;
  settings->custom_inflate = 0;
  settings->custom_context = 0;
}

const LodePNGDecompressSettings lodepng_default_decompress_settings = {0, 0, 0, 0};

#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of Zlib related code. Begin of PNG related code.                 // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_PNG

/* ////////////////////////////////////////////////////////////////////////// */
/* / CRC32                                                                  / */
/* ////////////////////////////////////////////////////////////////////////// */


#ifndef LODEPNG_NO_COMPILE_CRC
/* CRC polynomial: 0xedb88320 */
thread_local static mse::lh::TNativeArrayReplacement<unsigned int, 256> lodepng_crc32_table = std::array<unsigned int, 256 > {
           0u, 1996959894u, 3993919788u, 2567524794u,  124634137u, 1886057615u, 3915621685u, 2657392035u,
   249268274u, 2044508324u, 3772115230u, 2547177864u,  162941995u, 2125561021u, 3887607047u, 2428444049u,
   498536548u, 1789927666u, 4089016648u, 2227061214u,  450548861u, 1843258603u, 4107580753u, 2211677639u,
   325883990u, 1684777152u, 4251122042u, 2321926636u,  335633487u, 1661365465u, 4195302755u, 2366115317u,
   997073096u, 1281953886u, 3579855332u, 2724688242u, 1006888145u, 1258607687u, 3524101629u, 2768942443u,
   901097722u, 1119000684u, 3686517206u, 2898065728u,  853044451u, 1172266101u, 3705015759u, 2882616665u,
   651767980u, 1373503546u, 3369554304u, 3218104598u,  565507253u, 1454621731u, 3485111705u, 3099436303u,
   671266974u, 1594198024u, 3322730930u, 2970347812u,  795835527u, 1483230225u, 3244367275u, 3060149565u,
  1994146192u,   31158534u, 2563907772u, 4023717930u, 1907459465u,  112637215u, 2680153253u, 3904427059u,
  2013776290u,  251722036u, 2517215374u, 3775830040u, 2137656763u,  141376813u, 2439277719u, 3865271297u,
  1802195444u,  476864866u, 2238001368u, 4066508878u, 1812370925u,  453092731u, 2181625025u, 4111451223u,
  1706088902u,  314042704u, 2344532202u, 4240017532u, 1658658271u,  366619977u, 2362670323u, 4224994405u,
  1303535960u,  984961486u, 2747007092u, 3569037538u, 1256170817u, 1037604311u, 2765210733u, 3554079995u,
  1131014506u,  879679996u, 2909243462u, 3663771856u, 1141124467u,  855842277u, 2852801631u, 3708648649u,
  1342533948u,  654459306u, 3188396048u, 3373015174u, 1466479909u,  544179635u, 3110523913u, 3462522015u,
  1591671054u,  702138776u, 2966460450u, 3352799412u, 1504918807u,  783551873u, 3082640443u, 3233442989u,
  3988292384u, 2596254646u,   62317068u, 1957810842u, 3939845945u, 2647816111u,   81470997u, 1943803523u,
  3814918930u, 2489596804u,  225274430u, 2053790376u, 3826175755u, 2466906013u,  167816743u, 2097651377u,
  4027552580u, 2265490386u,  503444072u, 1762050814u, 4150417245u, 2154129355u,  426522225u, 1852507879u,
  4275313526u, 2312317920u,  282753626u, 1742555852u, 4189708143u, 2394877945u,  397917763u, 1622183637u,
  3604390888u, 2714866558u,  953729732u, 1340076626u, 3518719985u, 2797360999u, 1068828381u, 1219638859u,
  3624741850u, 2936675148u,  906185462u, 1090812512u, 3747672003u, 2825379669u,  829329135u, 1181335161u,
  3412177804u, 3160834842u,  628085408u, 1382605366u, 3423369109u, 3138078467u,  570562233u, 1426400815u,
  3317316542u, 2998733608u,  733239954u, 1555261956u, 3268935591u, 3050360625u,  752459403u, 1541320221u,
  2607071920u, 3965973030u, 1969922972u,   40735498u, 2617837225u, 3943577151u, 1913087877u,   83908371u,
  2512341634u, 3803740692u, 2075208622u,  213261112u, 2463272603u, 3855990285u, 2094854071u,  198958881u,
  2262029012u, 4057260610u, 1759359992u,  534414190u, 2176718541u, 4139329115u, 1873836001u,  414664567u,
  2282248934u, 4279200368u, 1711684554u,  285281116u, 2405801727u, 4167216745u, 1634467795u,  376229701u,
  2685067896u, 3608007406u, 1308918612u,  956543938u, 2808555105u, 3495958263u, 1231636301u, 1047427035u,
  2932959818u, 3654703836u, 1088359270u,  936918000u, 2847714899u, 3736837829u, 1202900863u,  817233897u,
  3183342108u, 3401237130u, 1404277552u,  615818150u, 3134207493u, 3453421203u, 1423857449u,  601450431u,
  3009837614u, 3294710456u, 1567103746u,  711928724u, 3020668471u, 3272380065u, 1510334235u,  755167117u
};

/*Return the CRC of the bytes buf[0..len-1].*/
unsigned lodepng_crc32(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t length)
{
  unsigned r = 0xffffffffu;
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0; i < length; ++i)
  {
    r = lodepng_crc32_table[(r ^ data[i]) & 0xff] ^ (r >> 8);
  }
  return r ^ 0xffffffffu;
}
#else /* !LODEPNG_NO_COMPILE_CRC */
unsigned lodepng_crc32(const unsigned char* data, size_t length);
#endif /* !LODEPNG_NO_COMPILE_CRC */

/* ////////////////////////////////////////////////////////////////////////// */
/* / Reading and writing single bits and bytes from/to stream for LodePNG   / */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned char readBitFromReversedStream(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  bitstream)
{
  unsigned char result = (unsigned char)((bitstream[(*bitpointer) >> 3] >> (7 - ((*bitpointer) & 0x7))) & 1);
  ++(*bitpointer);
  return result;
}

static unsigned readBitsFromReversedStream(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  bitstream, size_t nbits)
{
  unsigned result = 0;
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0 ; i < nbits; ++i)
  {
    result <<= 1;
    result |= (unsigned)readBitFromReversedStream(bitpointer, bitstream);
  }
  return result;
}

#ifdef LODEPNG_COMPILE_DECODER
static void setBitOfReversedStream0(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  bitstream, unsigned char bit)
{
  /*the current bit in bitstream must be 0 for this to work*/
  if(bit)
  {
    /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/
    bitstream[(*bitpointer) >> 3] |= (bit << (7 - ((*bitpointer) & 0x7)));
  }
  ++(*bitpointer);
}
#endif /*LODEPNG_COMPILE_DECODER*/

static void setBitOfReversedStream(mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  bitpointer, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  bitstream, unsigned char bit)
{
  /*the current bit in bitstream may be 0 or 1 for this to work*/
  if(bit == 0) bitstream[(*bitpointer) >> 3] &=  (unsigned char)(~(1 << (7 - ((*bitpointer) & 0x7))));
  else         bitstream[(*bitpointer) >> 3] |=  (1 << (7 - ((*bitpointer) & 0x7)));
  ++(*bitpointer);
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG chunks                                                             / */
/* ////////////////////////////////////////////////////////////////////////// */

unsigned lodepng_chunk_length(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  return lodepng_read32bitInt(((chunk) + (0)));
}

void lodepng_chunk_type(mse::lh::TLHNullableAnyRandomAccessIterator<char>  type, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  unsigned int i = 0/*auto-generated init val*/;
  for(i = 0; i != 4; ++i) type[i] = (char)chunk[4 + i];
  type[4] = 0; /*null termination char*/
}

unsigned char lodepng_chunk_type_equals(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  type)
{
  if(strlen(mse::us::lh::make_raw_pointer_from(type)) != 4) return 0;
  return (chunk[4] == type[0] && chunk[5] == type[1] && chunk[6] == type[2] && chunk[7] == type[3]);
}

unsigned char lodepng_chunk_ancillary(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  return((chunk[4] & 32) != 0);
}

unsigned char lodepng_chunk_private(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  return((chunk[6] & 32) != 0);
}

unsigned char lodepng_chunk_safetocopy(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  return((chunk[7] & 32) != 0);
}

mse::lh::TLHNullableAnyPointer<unsigned char>  lodepng_chunk_data(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  chunk)
{
  return ((chunk) + (8));
}

mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  lodepng_chunk_data_const(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  return ((chunk) + (8));
}

unsigned lodepng_chunk_check_crc(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  unsigned length = lodepng_chunk_length(chunk);
  unsigned CRC = lodepng_read32bitInt(((chunk) + (length + 8)));
  /*the CRC is taken of the data and the 4 chunk type letters, not the length*/
  unsigned checksum = lodepng_crc32(((chunk) + (4)), length + 4);
  if(CRC != checksum) return 1;
  else return 0;
}

void lodepng_chunk_generate_crc(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  chunk)
{
  unsigned length = lodepng_chunk_length(chunk);
  unsigned CRC = lodepng_crc32(((chunk) + (4)), length + 4);
  lodepng_set32bitInt(chunk + 8 + length, CRC);
}

mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  lodepng_chunk_next(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  chunk)
{
  unsigned total_chunk_length = lodepng_chunk_length(chunk) + 12;
  return ((chunk) + (total_chunk_length));
}

mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  lodepng_chunk_next_const(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  unsigned total_chunk_length = lodepng_chunk_length(chunk) + 12;
  return ((chunk) + (total_chunk_length));
}

unsigned lodepng_chunk_append(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outlength, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk)
{
  unsigned int i = 0/*auto-generated init val*/;
  unsigned total_chunk_length = lodepng_chunk_length(chunk) + 12;
  mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  chunk_start = nullptr/*auto-generated init val*/; mse::lh::TStrongVectorIterator<unsigned char>  new_buffer = nullptr/*auto-generated init val*/;
  size_t new_length = (*outlength) + total_chunk_length;
  if(new_length < total_chunk_length || new_length < (*outlength)) return 77; /*integer overflow happened*/

  new_buffer = mse::lh::reallocate(*out, (new_length));
  if(!new_buffer) return 83; /*alloc fail*/
  (*out) = new_buffer;
  (*outlength) = new_length;
  chunk_start = (((*out)) + (new_length - total_chunk_length));

  for(i = 0; i != total_chunk_length; ++i) chunk_start[i] = chunk[i];

  return 0;
}

unsigned lodepng_chunk_create(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outlength, unsigned length,
                              mse::lh::TLHNullableAnyRandomAccessIterator<const char>  type, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data)
{
  unsigned int i = 0/*auto-generated init val*/;
  mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  chunk = nullptr/*auto-generated init val*/; mse::lh::TStrongVectorIterator<unsigned char>  new_buffer = nullptr/*auto-generated init val*/;
  size_t new_length = (*outlength) + length + 12;
  if(new_length < length + 12 || new_length < (*outlength)) return 77; /*integer overflow happened*/
  new_buffer = mse::lh::reallocate(*out, (new_length));
  if(!new_buffer) return 83; /*alloc fail*/
  (*out) = new_buffer;
  (*outlength) = new_length;
  chunk = (((*out)) + ((*outlength) - length - 12));

  /*1: length*/
  lodepng_set32bitInt(chunk, (unsigned)length);

  /*2: chunk name (4 letters)*/
  chunk[4] = (unsigned char)type[0];
  chunk[5] = (unsigned char)type[1];
  chunk[6] = (unsigned char)type[2];
  chunk[7] = (unsigned char)type[3];

  /*3: the data*/
  for(i = 0; i != length; ++i) chunk[8 + i] = data[i];

  /*4: CRC (of the chunkname characters and the data)*/
  lodepng_chunk_generate_crc(chunk);

  return 0;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Color types and such                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

/*return type is a LodePNG error code*/
static unsigned checkColorValidity(LodePNGColorType colortype, unsigned bd) /*bd = bitdepth*/
{
  switch(colortype)
  {
    case 0: if(!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; break; /*grey*/
    case 2: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*RGB*/
    case 3: if(!(bd == 1 || bd == 2 || bd == 4 || bd == 8            )) return 37; break; /*palette*/
    case 4: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*grey + alpha*/
    case 6: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*RGBA*/
    default: return 31;
  }
  return 0; /*allowed color type / bits combination*/
}

static unsigned getNumColorChannels(LodePNGColorType colortype)
{
  switch(colortype)
  {
    case 0: return 1; /*grey*/
    case 2: return 3; /*RGB*/
    case 3: return 1; /*palette*/
    case 4: return 2; /*grey + alpha*/
    case 6: return 4; /*RGBA*/
  }
  return 0; /*unexisting color type*/
}

static unsigned lodepng_get_bpp_lct(LodePNGColorType colortype, unsigned bitdepth)
{
  /*bits per pixel is amount of channels * bits per channel*/
  return getNumColorChannels(colortype) * bitdepth;
}

/* ////////////////////////////////////////////////////////////////////////// */

void lodepng_color_mode_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGColorMode>  info)
{
  info->key_defined = 0;
  info->key_r = info->key_g = info->key_b = 0;
  info->colortype = LCT_RGBA;
  info->bitdepth = 8;
  info->palette = 0;
  info->palettesize = 0;
}

void lodepng_color_mode_cleanup(mse::lh::TXScopeLHNullableAnyPointer<LodePNGColorMode>  info)
{
  lodepng_palette_clear(info);
}

unsigned lodepng_color_mode_copy(mse::lh::TLHNullableAnyPointer<LodePNGColorMode>  dest, mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  source)
{
  unsigned long i = 0/*auto-generated init val*/;
  lodepng_color_mode_cleanup(dest);
  *dest = *source;
  if(source->palette)
  {
    mse::lh::allocate(dest->palette, (1024));
    if(!dest->palette && source->palettesize) return 83; /*alloc fail*/
    for(i = 0; i != source->palettesize * 4; ++i) dest->palette[i] = source->palette[i];
  }
  return 0;
}

static int lodepng_color_mode_equal(mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  a, mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  b)
{
  unsigned long i = 0/*auto-generated init val*/;
  if(a->colortype != b->colortype) return 0;
  if(a->bitdepth != b->bitdepth) return 0;
  if(a->key_defined != b->key_defined) return 0;
  if(a->key_defined)
  {
    if(a->key_r != b->key_r) return 0;
    if(a->key_g != b->key_g) return 0;
    if(a->key_b != b->key_b) return 0;
  }
  /*if one of the palette sizes is 0, then we consider it to be the same as the
  other: it means that e.g. the palette was not given by the user and should be
  considered the same as the palette inside the PNG.*/
  if(1/*a->palettesize != 0 && b->palettesize != 0*/) {
    if(a->palettesize != b->palettesize) return 0;
    for(i = 0; i != a->palettesize * 4; ++i)
    {
      if(a->palette[i] != b->palette[i]) return 0;
    }
  }
  return 1;
}

void lodepng_palette_clear(mse::lh::TXScopeLHNullableAnyPointer<LodePNGColorMode>  info)
{
  if(info->palette) mse::lh::free(info->palette);
  info->palette = 0;
  info->palettesize = 0;
}

unsigned lodepng_palette_add(mse::lh::TLHNullableAnyPointer<LodePNGColorMode>  info,
                             unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  mse::lh::TStrongVectorIterator<unsigned char>  data = nullptr/*auto-generated init val*/;
  /*the same resize technique as C++ std::vectors is used, and here it's made so that for a palette with
  the max of 256 colors, it'll have the exact alloc size*/
  if(!info->palette) /*allocate palette if empty*/
  {
    /*room for 256 colors with 4 bytes each*/
    data = mse::lh::reallocate(info->palette, (1024));
    if(!data) return 83; /*alloc fail*/
    else info->palette = data;
  }
  info->palette[4 * info->palettesize + 0] = r;
  info->palette[4 * info->palettesize + 1] = g;
  info->palette[4 * info->palettesize + 2] = b;
  info->palette[4 * info->palettesize + 3] = a;
  ++info->palettesize;
  return 0;
}

unsigned lodepng_get_bpp(mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  /*calculate bits per pixel out of colortype and bitdepth*/
  return lodepng_get_bpp_lct(info->colortype, info->bitdepth);
}

unsigned lodepng_get_channels(mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  return getNumColorChannels(info->colortype);
}

unsigned lodepng_is_greyscale_type(mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  return info->colortype == LCT_GREY || info->colortype == LCT_GREY_ALPHA;
}

unsigned lodepng_is_alpha_type(mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  return (info->colortype & 4) != 0; /*4 or 6*/
}

unsigned lodepng_is_palette_type(mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  return info->colortype == LCT_PALETTE;
}

unsigned lodepng_has_palette_alpha(mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0; i != info->palettesize; ++i)
  {
    if(info->palette[i * 4 + 3] < 255) return 1;
  }
  return 0;
}

unsigned lodepng_can_have_alpha(mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  return info->key_defined
      || lodepng_is_alpha_type(info)
      || lodepng_has_palette_alpha(info);
}

size_t lodepng_get_raw_size(unsigned w, unsigned h, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  color)
{
  /*will not overflow for any color type if roughly w * h < 268435455*/
  size_t bpp = lodepng_get_bpp(color);
  size_t n = w * h;
  return ((n / 8) * bpp) + ((n & 7) * bpp + 7) / 8;
}

size_t lodepng_get_raw_size_lct(unsigned w, unsigned h, LodePNGColorType colortype, unsigned bitdepth)
{
  /*will not overflow for any color type if roughly w * h < 268435455*/
  size_t bpp = lodepng_get_bpp_lct(colortype, bitdepth);
  size_t n = w * h;
  return ((n / 8) * bpp) + ((n & 7) * bpp + 7) / 8;
}


#ifdef LODEPNG_COMPILE_PNG
#ifdef LODEPNG_COMPILE_DECODER
/*in an idat chunk, each scanline is a multiple of 8 bits, unlike the lodepng output buffer*/
static size_t lodepng_get_raw_size_idat(unsigned w, unsigned h, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  color)
{
  /*will not overflow for any color type if roughly w * h < 268435455*/
  size_t bpp = lodepng_get_bpp(color);
  size_t line = ((w / 8) * bpp) + ((w & 7) * bpp + 7) / 8;
  return h * line;
}
#endif /*LODEPNG_COMPILE_DECODER*/
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS

static void LodePNGUnknownChunks_init(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  unsigned int i = 0/*auto-generated init val*/;
  for(i = 0; i != 3; ++i) info->unknown_chunks_data[i] = 0;
  for(i = 0; i != 3; ++i) info->unknown_chunks_size[i] = 0;
}

static void LodePNGUnknownChunks_cleanup(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  unsigned int i = 0/*auto-generated init val*/;
  for(i = 0; i != 3; ++i) mse::lh::free(info->unknown_chunks_data[i]);
}

static unsigned LodePNGUnknownChunks_copy(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  dest, mse::lh::TLHNullableAnyPointer<const LodePNGInfo>  src)
{
  unsigned int i = 0/*auto-generated init val*/;

  LodePNGUnknownChunks_cleanup(dest);

  for(i = 0; i != 3; ++i)
  {
    unsigned long j = 0/*auto-generated init val*/;
    dest->unknown_chunks_size[i] = src->unknown_chunks_size[i];
    mse::lh::allocate(dest->unknown_chunks_data[i], (src->unknown_chunks_size[i]));
    if(!dest->unknown_chunks_data[i] && dest->unknown_chunks_size[i]) return 83; /*alloc fail*/
    for(j = 0; j < src->unknown_chunks_size[i]; ++j)
    {
      dest->unknown_chunks_data[i][j] = src->unknown_chunks_data[i][j];
    }
  }

  return 0;
}

/******************************************************************************/

static void LodePNGText_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  info)
{
  info->text_num = 0;
  info->text_keys = NULL;
  info->text_strings = NULL;
}

static void LodePNGText_cleanup(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0; i != info->text_num; ++i)
  {
    string_cleanup(((info->text_keys) + (i)));
    string_cleanup(((info->text_strings) + (i)));
  }
  mse::lh::free(info->text_keys);
  mse::lh::free(info->text_strings);
}

static unsigned LodePNGText_copy(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  dest, mse::lh::TLHNullableAnyPointer<const LodePNGInfo>  source)
{
  size_t i = 0;
  dest->text_keys = 0;
  dest->text_strings = 0;
  dest->text_num = 0;
  for(i = 0; i != source->text_num; ++i)
  {
    CERROR_TRY_RETURN(lodepng_add_text(dest, source->text_keys[i], source->text_strings[i]));
  }
  return 0;
}

void lodepng_clear_text(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  LodePNGText_cleanup(info);
}

unsigned lodepng_add_text(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  key, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  str)
{
  mse::lh::TStrongVectorIterator<mse::lh::TStrongVectorIterator<char> >  new_keys = mse::lh::reallocate(info->text_keys, (sizeof(char*) * (info->text_num + 1)) / sizeof(char *) * sizeof(mse::lh::TLHNullableAnyRandomAccessIterator<char> ));
  mse::lh::TStrongVectorIterator<mse::lh::TStrongVectorIterator<char> >  new_strings = mse::lh::reallocate(info->text_strings, (sizeof(char*) * (info->text_num + 1)) / sizeof(char *) * sizeof(mse::lh::TLHNullableAnyRandomAccessIterator<char> ));
  if(!new_keys || !new_strings)
  {
    mse::lh::free(new_keys);
    mse::lh::free(new_strings);
    return 83; /*alloc fail*/
  }

  ++info->text_num;
  info->text_keys = new_keys;
  info->text_strings = new_strings;

  string_init(((info->text_keys) + (info->text_num - 1)));
  string_set(((info->text_keys) + (info->text_num - 1)), key);

  string_init(((info->text_strings) + (info->text_num - 1)));
  string_set(((info->text_strings) + (info->text_num - 1)), str);

  return 0;
}

/******************************************************************************/

static void LodePNGIText_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  info)
{
  info->itext_num = 0;
  info->itext_keys = NULL;
  info->itext_langtags = NULL;
  info->itext_transkeys = NULL;
  info->itext_strings = NULL;
}

static void LodePNGIText_cleanup(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  unsigned long i = 0/*auto-generated init val*/;
  for(i = 0; i != info->itext_num; ++i)
  {
    string_cleanup(((info->itext_keys) + (i)));
    string_cleanup(((info->itext_langtags) + (i)));
    string_cleanup(((info->itext_transkeys) + (i)));
    string_cleanup(((info->itext_strings) + (i)));
  }
  mse::lh::free(info->itext_keys);
  mse::lh::free(info->itext_langtags);
  mse::lh::free(info->itext_transkeys);
  mse::lh::free(info->itext_strings);
}

static unsigned LodePNGIText_copy(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  dest, mse::lh::TLHNullableAnyPointer<const LodePNGInfo>  source)
{
  size_t i = 0;
  dest->itext_keys = 0;
  dest->itext_langtags = 0;
  dest->itext_transkeys = 0;
  dest->itext_strings = 0;
  dest->itext_num = 0;
  for(i = 0; i != source->itext_num; ++i)
  {
    CERROR_TRY_RETURN(lodepng_add_itext(dest, source->itext_keys[i], source->itext_langtags[i],
                                        source->itext_transkeys[i], source->itext_strings[i]));
  }
  return 0;
}

void lodepng_clear_itext(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  LodePNGIText_cleanup(info);
}

unsigned lodepng_add_itext(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  key, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  langtag,
                           mse::lh::TLHNullableAnyRandomAccessIterator<const char>  transkey, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  str)
{
  mse::lh::TStrongVectorIterator<mse::lh::TStrongVectorIterator<char> >  new_keys = mse::lh::reallocate(info->itext_keys, (sizeof(char*) * (info->itext_num + 1)) / sizeof(char *) * sizeof(mse::lh::TLHNullableAnyRandomAccessIterator<char> ));
  mse::lh::TStrongVectorIterator<mse::lh::TStrongVectorIterator<char> >  new_langtags = mse::lh::reallocate(info->itext_langtags, (sizeof(char*) * (info->itext_num + 1)) / sizeof(char *) * sizeof(mse::lh::TLHNullableAnyRandomAccessIterator<char> ));
  mse::lh::TStrongVectorIterator<mse::lh::TStrongVectorIterator<char> >  new_transkeys = mse::lh::reallocate(info->itext_transkeys, (sizeof(char*) * (info->itext_num + 1)) / sizeof(char *) * sizeof(mse::lh::TLHNullableAnyRandomAccessIterator<char> ));
  mse::lh::TStrongVectorIterator<mse::lh::TStrongVectorIterator<char> >  new_strings = mse::lh::reallocate(info->itext_strings, (sizeof(char*) * (info->itext_num + 1)) / sizeof(char *) * sizeof(mse::lh::TLHNullableAnyRandomAccessIterator<char> ));
  if(!new_keys || !new_langtags || !new_transkeys || !new_strings)
  {
    mse::lh::free(new_keys);
    mse::lh::free(new_langtags);
    mse::lh::free(new_transkeys);
    mse::lh::free(new_strings);
    return 83; /*alloc fail*/
  }

  ++info->itext_num;
  info->itext_keys = new_keys;
  info->itext_langtags = new_langtags;
  info->itext_transkeys = new_transkeys;
  info->itext_strings = new_strings;

  string_init(((info->itext_keys) + (info->itext_num - 1)));
  string_set(((info->itext_keys) + (info->itext_num - 1)), key);

  string_init(((info->itext_langtags) + (info->itext_num - 1)));
  string_set(((info->itext_langtags) + (info->itext_num - 1)), langtag);

  string_init(((info->itext_transkeys) + (info->itext_num - 1)));
  string_set(((info->itext_transkeys) + (info->itext_num - 1)), transkey);

  string_init(((info->itext_strings) + (info->itext_num - 1)));
  string_set(((info->itext_strings) + (info->itext_num - 1)), str);

  return 0;
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

void lodepng_info_init(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  lodepng_color_mode_init(&info->color);
  info->interlace_method = 0;
  info->compression_method = 0;
  info->filter_method = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  info->background_defined = 0;
  info->background_r = info->background_g = info->background_b = 0;

  LodePNGText_init(info);
  LodePNGIText_init(info);

  info->time_defined = 0;
  info->phys_defined = 0;

  LodePNGUnknownChunks_init(info);
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

void lodepng_info_cleanup(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info)
{
  lodepng_color_mode_cleanup(&info->color);
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  LodePNGText_cleanup(info);
  LodePNGIText_cleanup(info);

  LodePNGUnknownChunks_cleanup(info);
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

unsigned lodepng_info_copy(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  dest, mse::lh::TLHNullableAnyPointer<const LodePNGInfo>  source)
{
  lodepng_info_cleanup(dest);
  *dest = *source;
  lodepng_color_mode_init(&dest->color);
  CERROR_TRY_RETURN(lodepng_color_mode_copy(&dest->color, &source->color));

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  CERROR_TRY_RETURN(LodePNGText_copy(dest, source));
  CERROR_TRY_RETURN(LodePNGIText_copy(dest, source));

  LodePNGUnknownChunks_init(dest);
  CERROR_TRY_RETURN(LodePNGUnknownChunks_copy(dest, source));
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
  return 0;
}

void lodepng_info_swap(mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  a, mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  b)
{
  LodePNGInfo temp = *a;
  *a = *b;
  *b = temp;
}

/* ////////////////////////////////////////////////////////////////////////// */

/*index: bitgroup index, bits: bitgroup size(1, 2 or 4), in: bitgroup value, out: octet array to add bits to*/
static void addColorBits(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, size_t index, unsigned bits, unsigned in)
{
  unsigned m = bits == 1 ? 7 : bits == 2 ? 3 : 1; /*8 / bits - 1*/
  /*p = the partial index in the byte, e.g. with 4 palettebits it is 0 for first half or 1 for second half*/
  unsigned p = index & m;
  in &= (1u << bits) - 1u; /*filter out any other bits of the input value*/
  in = in << (bits * (m - p));
  if(p == 0) out[index * bits / 8] = in;
  else out[index * bits / 8] |= in;
}

typedef struct ColorTree ColorTree;

/*
One node of a color tree
This is the data structure used to count the number of unique colors and to get a palette
index for a color. It's like an octree, but because the alpha channel is used too, each
node has 16 instead of 8 children.
*/
struct ColorTree
{
  mse::lh::TNativeArrayReplacement<mse::lh::TLHNullableAnyPointer<ColorTree> , 16>  children; /*up to 16 pointers to ColorTree of next level*/
  int index = 0/*auto-generated init val*/; /*the payload. Only has a meaningful value if this is in the last level*/
};

static void color_tree_init(mse::lh::TLHNullableAnyPointer<ColorTree>  tree)
{
  int i = 0/*auto-generated init val*/;
  for(i = 0; i != 16; ++i) tree->children[i] = 0;
  tree->index = -1;
}

static void color_tree_cleanup(mse::lh::TLHNullableAnyPointer<ColorTree>  tree)
{
  int i = 0/*auto-generated init val*/;
  for(i = 0; i != 16; ++i)
  {
    if(tree->children[i])
    {
      color_tree_cleanup(tree->children[i]);
      mse::lh::free(tree->children[i]);
    }
  }
}

/*returns -1 if color not present, its index otherwise*/
static int color_tree_get(mse::lh::TLHNullableAnyPointer<ColorTree>  tree, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  int bit = 0;
  for(bit = 0; bit < 8; ++bit)
  {
    int i = 8 * ((r >> bit) & 1) + 4 * ((g >> bit) & 1) + 2 * ((b >> bit) & 1) + 1 * ((a >> bit) & 1);
    if(!tree->children[i]) return -1;
    else tree = tree->children[i];
  }
  return tree ? tree->index : -1;
}

#ifdef LODEPNG_COMPILE_ENCODER
static int color_tree_has(mse::lh::TLHNullableAnyPointer<ColorTree>  tree, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  return color_tree_get(tree, r, g, b, a) >= 0;
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/*color is not allowed to already exist.
Index should be >= 0 (it's signed to be compatible with using -1 for "doesn't exist")*/
static void color_tree_add(mse::lh::TLHNullableAnyPointer<ColorTree>  tree,
                           unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned index)
{
  int bit = 0/*auto-generated init val*/;
  for(bit = 0; bit < 8; ++bit)
  {
    int i = 8 * ((r >> bit) & 1) + 4 * ((g >> bit) & 1) + 2 * ((b >> bit) & 1) + 1 * ((a >> bit) & 1);
    if(!tree->children[i])
    {
      mse::lh::allocate(tree->children[i], (sizeof(ColorTree)) / sizeof(struct ColorTree) * sizeof(ColorTree));
      color_tree_init(tree->children[i]);
    }
    tree = tree->children[i];
  }
  tree->index = (int)index;
}

/*put a pixel, given its RGBA color, into image of any color type*/
static unsigned rgba8ToPixel(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, size_t i,
                             mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  mode, mse::lh::TLHNullableAnyPointer<ColorTree>  tree /*for palette*/,
                             unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  if(mode->colortype == LCT_GREY)
  {
    unsigned char grey = r; /*((unsigned short)r + g + b) / 3*/;
    if(mode->bitdepth == 8) out[i] = grey;
    else if(mode->bitdepth == 16) out[i * 2 + 0] = out[i * 2 + 1] = grey;
    else
    {
      /*take the most significant bits of grey*/
      grey = (grey >> (8 - mode->bitdepth)) & ((1 << mode->bitdepth) - 1);
      addColorBits(out, i, mode->bitdepth, grey);
    }
  }
  else if(mode->colortype == LCT_RGB)
  {
    if(mode->bitdepth == 8)
    {
      out[i * 3 + 0] = r;
      out[i * 3 + 1] = g;
      out[i * 3 + 2] = b;
    }
    else
    {
      out[i * 6 + 0] = out[i * 6 + 1] = r;
      out[i * 6 + 2] = out[i * 6 + 3] = g;
      out[i * 6 + 4] = out[i * 6 + 5] = b;
    }
  }
  else if(mode->colortype == LCT_PALETTE)
  {
    int index = color_tree_get(tree, r, g, b, a);
    if(index < 0) return 82; /*color not in palette*/
    if(mode->bitdepth == 8) out[i] = index;
    else addColorBits(out, i, mode->bitdepth, (unsigned)index);
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    unsigned char grey = r; /*((unsigned short)r + g + b) / 3*/;
    if(mode->bitdepth == 8)
    {
      out[i * 2 + 0] = grey;
      out[i * 2 + 1] = a;
    }
    else if(mode->bitdepth == 16)
    {
      out[i * 4 + 0] = out[i * 4 + 1] = grey;
      out[i * 4 + 2] = out[i * 4 + 3] = a;
    }
  }
  else if(mode->colortype == LCT_RGBA)
  {
    if(mode->bitdepth == 8)
    {
      out[i * 4 + 0] = r;
      out[i * 4 + 1] = g;
      out[i * 4 + 2] = b;
      out[i * 4 + 3] = a;
    }
    else
    {
      out[i * 8 + 0] = out[i * 8 + 1] = r;
      out[i * 8 + 2] = out[i * 8 + 3] = g;
      out[i * 8 + 4] = out[i * 8 + 5] = b;
      out[i * 8 + 6] = out[i * 8 + 7] = a;
    }
  }

  return 0; /*no error*/
}

/*put a pixel, given its RGBA16 color, into image of any color 16-bitdepth type*/
static void rgba16ToPixel(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, size_t i,
                         mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  mode,
                         unsigned short r, unsigned short g, unsigned short b, unsigned short a)
{
  if(mode->colortype == LCT_GREY)
  {
    unsigned short grey = r; /*((unsigned)r + g + b) / 3*/;
    out[i * 2 + 0] = (grey >> 8) & 255;
    out[i * 2 + 1] = grey & 255;
  }
  else if(mode->colortype == LCT_RGB)
  {
    out[i * 6 + 0] = (r >> 8) & 255;
    out[i * 6 + 1] = r & 255;
    out[i * 6 + 2] = (g >> 8) & 255;
    out[i * 6 + 3] = g & 255;
    out[i * 6 + 4] = (b >> 8) & 255;
    out[i * 6 + 5] = b & 255;
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    unsigned short grey = r; /*((unsigned)r + g + b) / 3*/;
    out[i * 4 + 0] = (grey >> 8) & 255;
    out[i * 4 + 1] = grey & 255;
    out[i * 4 + 2] = (a >> 8) & 255;
    out[i * 4 + 3] = a & 255;
  }
  else if(mode->colortype == LCT_RGBA)
  {
    out[i * 8 + 0] = (r >> 8) & 255;
    out[i * 8 + 1] = r & 255;
    out[i * 8 + 2] = (g >> 8) & 255;
    out[i * 8 + 3] = g & 255;
    out[i * 8 + 4] = (b >> 8) & 255;
    out[i * 8 + 5] = b & 255;
    out[i * 8 + 6] = (a >> 8) & 255;
    out[i * 8 + 7] = a & 255;
  }
}

/*Get RGBA8 color of pixel with index i (y * width + x) from the raw image with given color type.*/
static void getPixelColorRGBA8(mse::lh::TXScopeLHNullableAnyPointer<unsigned char>  r, mse::lh::TXScopeLHNullableAnyPointer<unsigned char>  g,
                               mse::lh::TXScopeLHNullableAnyPointer<unsigned char>  b, mse::lh::TXScopeLHNullableAnyPointer<unsigned char>  a,
                               mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t i,
                               mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  mode)
{
  if(mode->colortype == LCT_GREY)
  {
    if(mode->bitdepth == 8)
    {
      *r = *g = *b = in[i];
      if(mode->key_defined && *r == mode->key_r) *a = 0;
      else *a = 255;
    }
    else if(mode->bitdepth == 16)
    {
      *r = *g = *b = in[i * 2 + 0];
      if(mode->key_defined && 256U * in[i * 2 + 0] + in[i * 2 + 1] == mode->key_r) *a = 0;
      else *a = 255;
    }
    else
    {
      unsigned highest = ((1U << mode->bitdepth) - 1U); /*highest possible value for this bit depth*/
      mse::TNoradObj<unsigned long > j = i * mode->bitdepth;
      unsigned value = readBitsFromReversedStream(&j, in, mode->bitdepth);
      *r = *g = *b = (value * 255) / highest;
      if(mode->key_defined && value == mode->key_r) *a = 0;
      else *a = 255;
    }
  }
  else if(mode->colortype == LCT_RGB)
  {
    if(mode->bitdepth == 8)
    {
      *r = in[i * 3 + 0]; *g = in[i * 3 + 1]; *b = in[i * 3 + 2];
      if(mode->key_defined && *r == mode->key_r && *g == mode->key_g && *b == mode->key_b) *a = 0;
      else *a = 255;
    }
    else
    {
      *r = in[i * 6 + 0];
      *g = in[i * 6 + 2];
      *b = in[i * 6 + 4];
      if(mode->key_defined && 256U * in[i * 6 + 0] + in[i * 6 + 1] == mode->key_r
         && 256U * in[i * 6 + 2] + in[i * 6 + 3] == mode->key_g
         && 256U * in[i * 6 + 4] + in[i * 6 + 5] == mode->key_b) *a = 0;
      else *a = 255;
    }
  }
  else if(mode->colortype == LCT_PALETTE)
  {
    unsigned int index = 0/*auto-generated init val*/;
    if(mode->bitdepth == 8) index = in[i];
    else
    {
      mse::TNoradObj<unsigned long > j = i * mode->bitdepth;
      index = readBitsFromReversedStream(&j, in, mode->bitdepth);
    }

    if(index >= mode->palettesize)
    {
      /*This is an error according to the PNG spec, but common PNG decoders make it black instead.
      Done here too, slightly faster due to no error handling needed.*/
      *r = *g = *b = 0;
      *a = 255;
    }
    else
    {
      *r = mode->palette[index * 4 + 0];
      *g = mode->palette[index * 4 + 1];
      *b = mode->palette[index * 4 + 2];
      *a = mode->palette[index * 4 + 3];
    }
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    if(mode->bitdepth == 8)
    {
      *r = *g = *b = in[i * 2 + 0];
      *a = in[i * 2 + 1];
    }
    else
    {
      *r = *g = *b = in[i * 4 + 0];
      *a = in[i * 4 + 2];
    }
  }
  else if(mode->colortype == LCT_RGBA)
  {
    if(mode->bitdepth == 8)
    {
      *r = in[i * 4 + 0];
      *g = in[i * 4 + 1];
      *b = in[i * 4 + 2];
      *a = in[i * 4 + 3];
    }
    else
    {
      *r = in[i * 8 + 0];
      *g = in[i * 8 + 2];
      *b = in[i * 8 + 4];
      *a = in[i * 8 + 6];
    }
  }
}

/*Similar to getPixelColorRGBA8, but with all the for loops inside of the color
mode test cases, optimized to convert the colors much faster, when converting
to RGBA or RGB with 8 bit per cannel. buffer must be RGBA or RGB output with
enough memory, if has_alpha is true the output is RGBA. mode has the color mode
of the input buffer.*/
static void getPixelColorsRGBA8(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  buffer, size_t numpixels,
                                unsigned has_alpha, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                                mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  mode)
{
  unsigned num_channels = has_alpha ? 4 : 3;
  unsigned long i = 0/*auto-generated init val*/;
  if(mode->colortype == LCT_GREY)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i];
        if(has_alpha) buffer[3] = mode->key_defined && in[i] == mode->key_r ? 0 : 255;
      }
    }
    else if(mode->bitdepth == 16)
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i * 2];
        if(has_alpha) buffer[3] = mode->key_defined && 256U * in[i * 2 + 0] + in[i * 2 + 1] == mode->key_r ? 0 : 255;
      }
    }
    else
    {
      unsigned highest = ((1U << mode->bitdepth) - 1U); /*highest possible value for this bit depth*/
      mse::TNoradObj<unsigned long > j = 0;
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        unsigned value = readBitsFromReversedStream(&j, in, mode->bitdepth);
        buffer[0] = buffer[1] = buffer[2] = (value * 255) / highest;
        if(has_alpha) buffer[3] = mode->key_defined && value == mode->key_r ? 0 : 255;
      }
    }
  }
  else if(mode->colortype == LCT_RGB)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = in[i * 3 + 0];
        buffer[1] = in[i * 3 + 1];
        buffer[2] = in[i * 3 + 2];
        if(has_alpha) buffer[3] = mode->key_defined && buffer[0] == mode->key_r
           && buffer[1]== mode->key_g && buffer[2] == mode->key_b ? 0 : 255;
      }
    }
    else
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = in[i * 6 + 0];
        buffer[1] = in[i * 6 + 2];
        buffer[2] = in[i * 6 + 4];
        if(has_alpha) buffer[3] = mode->key_defined
           && 256U * in[i * 6 + 0] + in[i * 6 + 1] == mode->key_r
           && 256U * in[i * 6 + 2] + in[i * 6 + 3] == mode->key_g
           && 256U * in[i * 6 + 4] + in[i * 6 + 5] == mode->key_b ? 0 : 255;
      }
    }
  }
  else if(mode->colortype == LCT_PALETTE)
  {
    unsigned int index = 0/*auto-generated init val*/;
    mse::TNoradObj<unsigned long > j = 0;
    for(i = 0; i != numpixels; ++i, buffer += num_channels)
    {
      if(mode->bitdepth == 8) index = in[i];
      else index = readBitsFromReversedStream(&j, in, mode->bitdepth);

      if(index >= mode->palettesize)
      {
        /*This is an error according to the PNG spec, but most PNG decoders make it black instead.
        Done here too, slightly faster due to no error handling needed.*/
        buffer[0] = buffer[1] = buffer[2] = 0;
        if(has_alpha) buffer[3] = 255;
      }
      else
      {
        buffer[0] = mode->palette[index * 4 + 0];
        buffer[1] = mode->palette[index * 4 + 1];
        buffer[2] = mode->palette[index * 4 + 2];
        if(has_alpha) buffer[3] = mode->palette[index * 4 + 3];
      }
    }
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i * 2 + 0];
        if(has_alpha) buffer[3] = in[i * 2 + 1];
      }
    }
    else
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i * 4 + 0];
        if(has_alpha) buffer[3] = in[i * 4 + 2];
      }
    }
  }
  else if(mode->colortype == LCT_RGBA)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = in[i * 4 + 0];
        buffer[1] = in[i * 4 + 1];
        buffer[2] = in[i * 4 + 2];
        if(has_alpha) buffer[3] = in[i * 4 + 3];
      }
    }
    else
    {
      for(i = 0; i != numpixels; ++i, buffer += num_channels)
      {
        buffer[0] = in[i * 8 + 0];
        buffer[1] = in[i * 8 + 2];
        buffer[2] = in[i * 8 + 4];
        if(has_alpha) buffer[3] = in[i * 8 + 6];
      }
    }
  }
}

/*Get RGBA16 color of pixel with index i (y * width + x) from the raw image with
given color type, but the given color type must be 16-bit itself.*/
static void getPixelColorRGBA16(mse::lh::TXScopeLHNullableAnyPointer<unsigned short>  r, mse::lh::TXScopeLHNullableAnyPointer<unsigned short>  g, mse::lh::TXScopeLHNullableAnyPointer<unsigned short>  b, mse::lh::TXScopeLHNullableAnyPointer<unsigned short>  a,
                                mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t i, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  mode)
{
  if(mode->colortype == LCT_GREY)
  {
    *r = *g = *b = 256 * in[i * 2 + 0] + in[i * 2 + 1];
    if(mode->key_defined && 256U * in[i * 2 + 0] + in[i * 2 + 1] == mode->key_r) *a = 0;
    else *a = 65535;
  }
  else if(mode->colortype == LCT_RGB)
  {
    *r = 256u * in[i * 6 + 0] + in[i * 6 + 1];
    *g = 256u * in[i * 6 + 2] + in[i * 6 + 3];
    *b = 256u * in[i * 6 + 4] + in[i * 6 + 5];
    if(mode->key_defined
       && 256u * in[i * 6 + 0] + in[i * 6 + 1] == mode->key_r
       && 256u * in[i * 6 + 2] + in[i * 6 + 3] == mode->key_g
       && 256u * in[i * 6 + 4] + in[i * 6 + 5] == mode->key_b) *a = 0;
    else *a = 65535;
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    *r = *g = *b = 256u * in[i * 4 + 0] + in[i * 4 + 1];
    *a = 256u * in[i * 4 + 2] + in[i * 4 + 3];
  }
  else if(mode->colortype == LCT_RGBA)
  {
    *r = 256u * in[i * 8 + 0] + in[i * 8 + 1];
    *g = 256u * in[i * 8 + 2] + in[i * 8 + 3];
    *b = 256u * in[i * 8 + 4] + in[i * 8 + 5];
    *a = 256u * in[i * 8 + 6] + in[i * 8 + 7];
  }
}

unsigned lodepng_convert(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                         mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  mode_out, mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  mode_in,
                         unsigned w, unsigned h)
{
  unsigned long i = 0/*auto-generated init val*/;
  mse::TNoradObj<ColorTree > tree;
  size_t numpixels = w * h;

  if(lodepng_color_mode_equal(mode_out, mode_in))
  {
    size_t numbytes = lodepng_get_raw_size(w, h, mode_in);
    for(i = 0; i != numbytes; ++i) out[i] = in[i];
    return 0;
  }

  if(mode_out->colortype == LCT_PALETTE)
  {
    size_t palettesize = mode_out->palettesize;
    mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  palette = mode_out->palette;
    size_t palsize = 1u << mode_out->bitdepth;
    /*if the user specified output palette but did not give the values, assume
    they want the values of the input color type (assuming that one is palette).
    Note that we never create a new palette ourselves.*/
    if(palettesize == 0)
    {
      palettesize = mode_in->palettesize;
      palette = mode_in->palette;
    }
    if(palettesize < palsize) palsize = palettesize;
    color_tree_init(&tree);
    for(i = 0; i != palsize; ++i)
    {
      mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  p = ((palette) + (i * 4));
      color_tree_add(&tree, p[0], p[1], p[2], p[3], i);
    }
  }

  if(mode_in->bitdepth == 16 && mode_out->bitdepth == 16)
  {
    for(i = 0; i != numpixels; ++i)
    {
      mse::TNoradObj<unsigned short > r = 0; mse::TNoradObj<unsigned short > g = 0; mse::TNoradObj<unsigned short > b = 0; mse::TNoradObj<unsigned short > a = 0;
      getPixelColorRGBA16(&r, &g, &b, &a, in, i, mode_in);
      rgba16ToPixel(out, i, mode_out, r, g, b, a);
    }
  }
  else if(mode_out->bitdepth == 8 && mode_out->colortype == LCT_RGBA)
  {
    getPixelColorsRGBA8(out, numpixels, 1, in, mode_in);
  }
  else if(mode_out->bitdepth == 8 && mode_out->colortype == LCT_RGB)
  {
    getPixelColorsRGBA8(out, numpixels, 0, in, mode_in);
  }
  else
  {
    mse::TNoradObj<unsigned char > r = 0; mse::TNoradObj<unsigned char > g = 0; mse::TNoradObj<unsigned char > b = 0; mse::TNoradObj<unsigned char > a = 0;
    for(i = 0; i != numpixels; ++i)
    {
      getPixelColorRGBA8(&r, &g, &b, &a, in, i, mode_in);
      CERROR_TRY_RETURN(rgba8ToPixel(out, i, mode_out, &tree, r, g, b, a));
    }
  }

  if(mode_out->colortype == LCT_PALETTE)
  {
    color_tree_cleanup(&tree);
  }

  return 0; /*no error*/
}

#ifdef LODEPNG_COMPILE_ENCODER

void lodepng_color_profile_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGColorProfile>  profile)
{
  profile->colored = 0;
  profile->key = 0;
  profile->key_r = profile->key_g = profile->key_b = 0;
  profile->alpha = 0;
  profile->numcolors = 0;
  profile->bits = 1;
}

/*function used for debug purposes with C++*/
/*void printColorProfile(LodePNGColorProfile* p)
{
  std::cout << "colored: " << (int)p->colored << ", ";
  std::cout << "key: " << (int)p->key << ", ";
  std::cout << "key_r: " << (int)p->key_r << ", ";
  std::cout << "key_g: " << (int)p->key_g << ", ";
  std::cout << "key_b: " << (int)p->key_b << ", ";
  std::cout << "alpha: " << (int)p->alpha << ", ";
  std::cout << "numcolors: " << (int)p->numcolors << ", ";
  std::cout << "bits: " << (int)p->bits << std::endl;
}*/

/*Returns how many bits needed to represent given value (max 8 bit)*/
static unsigned getValueRequiredBits(unsigned char value)
{
  if(value == 0 || value == 255) return 1;
  /*The scaling of 2-bit and 4-bit values uses multiples of 85 and 17*/
  if(value % 17 == 0) return value % 85 == 0 ? 2 : 4;
  return 8;
}

/*profile must already have been inited with mode.
It's ok to set some parameters of profile to done already.*/
unsigned lodepng_get_color_profile(mse::lh::TXScopeLHNullableAnyPointer<LodePNGColorProfile>  profile,
                                   mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h,
                                   mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  mode)
{
  unsigned error = 0;
  unsigned long i = 0/*auto-generated init val*/;
  mse::TNoradObj<ColorTree > tree;
  size_t numpixels = w * h;

  unsigned colored_done = lodepng_is_greyscale_type(mode) ? 1 : 0;
  unsigned alpha_done = lodepng_can_have_alpha(mode) ? 0 : 1;
  unsigned numcolors_done = 0;
  unsigned bpp = lodepng_get_bpp(mode);
  unsigned bits_done = bpp == 1 ? 1 : 0;
  unsigned maxnumcolors = 257;
  unsigned sixteen = 0;
  if(bpp <= 8) maxnumcolors = bpp == 1 ? 2 : (bpp == 2 ? 4 : (bpp == 4 ? 16 : 256));

  color_tree_init(&tree);

  /*Check if the 16-bit input is truly 16-bit*/
  if(mode->bitdepth == 16)
  {
    mse::TNoradObj<unsigned short > r = 0/*auto-generated init val*/; mse::TNoradObj<unsigned short > g = 0/*auto-generated init val*/; mse::TNoradObj<unsigned short > b = 0/*auto-generated init val*/; mse::TNoradObj<unsigned short > a = 0/*auto-generated init val*/;
    for(i = 0; i != numpixels; ++i)
    {
      getPixelColorRGBA16(&r, &g, &b, &a, in, i, mode);
      if((r & 255) != ((r >> 8) & 255) || (g & 255) != ((g >> 8) & 255) ||
         (b & 255) != ((b >> 8) & 255) || (a & 255) != ((a >> 8) & 255)) /*first and second byte differ*/
      {
        sixteen = 1;
        break;
      }
    }
  }

  if(sixteen)
  {
    mse::TNoradObj<unsigned short > r = 0; mse::TNoradObj<unsigned short > g = 0; mse::TNoradObj<unsigned short > b = 0; mse::TNoradObj<unsigned short > a = 0;
    profile->bits = 16;
    bits_done = numcolors_done = 1; /*counting colors no longer useful, palette doesn't support 16-bit*/

    for(i = 0; i != numpixels; ++i)
    {
      getPixelColorRGBA16(&r, &g, &b, &a, in, i, mode);

      if(!colored_done && (r != g || r != b))
      {
        profile->colored = 1;
        colored_done = 1;
      }

      if(!alpha_done)
      {
        unsigned matchkey = (r == profile->key_r && g == profile->key_g && b == profile->key_b);
        if(a != 65535 && (a != 0 || (profile->key && !matchkey)))
        {
          profile->alpha = 1;
          profile->key = 0;
          alpha_done = 1;
        }
        else if(a == 0 && !profile->alpha && !profile->key)
        {
          profile->key = 1;
          profile->key_r = r;
          profile->key_g = g;
          profile->key_b = b;
        }
        else if(a == 65535 && profile->key && matchkey)
        {
          /* Color key cannot be used if an opaque pixel also has that RGB color. */
          profile->alpha = 1;
          profile->key = 0;
          alpha_done = 1;
        }
      }
      if(alpha_done && numcolors_done && colored_done && bits_done) break;
    }

    if(profile->key && !profile->alpha)
    {
      for(i = 0; i != numpixels; ++i)
      {
        getPixelColorRGBA16(&r, &g, &b, &a, in, i, mode);
        if(a != 0 && r == profile->key_r && g == profile->key_g && b == profile->key_b)
        {
          /* Color key cannot be used if an opaque pixel also has that RGB color. */
          profile->alpha = 1;
          profile->key = 0;
          alpha_done = 1;
        }
      }
    }
  }
  else /* < 16-bit */
  {
    mse::TNoradObj<unsigned char > r = 0; mse::TNoradObj<unsigned char > g = 0; mse::TNoradObj<unsigned char > b = 0; mse::TNoradObj<unsigned char > a = 0;
    for(i = 0; i != numpixels; ++i)
    {
      getPixelColorRGBA8(&r, &g, &b, &a, in, i, mode);

      if(!bits_done && profile->bits < 8)
      {
        /*only r is checked, < 8 bits is only relevant for greyscale*/
        unsigned bits = getValueRequiredBits(r);
        if(bits > profile->bits) profile->bits = bits;
      }
      bits_done = (profile->bits >= bpp);

      if(!colored_done && (r != g || r != b))
      {
        profile->colored = 1;
        colored_done = 1;
        if(profile->bits < 8) profile->bits = 8; /*PNG has no colored modes with less than 8-bit per channel*/
      }

      if(!alpha_done)
      {
        unsigned matchkey = (r == profile->key_r && g == profile->key_g && b == profile->key_b);
        if(a != 255 && (a != 0 || (profile->key && !matchkey)))
        {
          profile->alpha = 1;
          profile->key = 0;
          alpha_done = 1;
          if(profile->bits < 8) profile->bits = 8; /*PNG has no alphachannel modes with less than 8-bit per channel*/
        }
        else if(a == 0 && !profile->alpha && !profile->key)
        {
          profile->key = 1;
          profile->key_r = r;
          profile->key_g = g;
          profile->key_b = b;
        }
        else if(a == 255 && profile->key && matchkey)
        {
          /* Color key cannot be used if an opaque pixel also has that RGB color. */
          profile->alpha = 1;
          profile->key = 0;
          alpha_done = 1;
          if(profile->bits < 8) profile->bits = 8; /*PNG has no alphachannel modes with less than 8-bit per channel*/
        }
      }

      if(!numcolors_done)
      {
        if(!color_tree_has(&tree, r, g, b, a))
        {
          color_tree_add(&tree, r, g, b, a, profile->numcolors);
          if(profile->numcolors < 256)
          {
            mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  p = profile->palette;
            unsigned n = profile->numcolors;
            p[n * 4 + 0] = r;
            p[n * 4 + 1] = g;
            p[n * 4 + 2] = b;
            p[n * 4 + 3] = a;
          }
          ++profile->numcolors;
          numcolors_done = profile->numcolors >= maxnumcolors;
        }
      }

      if(alpha_done && numcolors_done && colored_done && bits_done) break;
    }

    if(profile->key && !profile->alpha)
    {
      for(i = 0; i != numpixels; ++i)
      {
        getPixelColorRGBA8(&r, &g, &b, &a, in, i, mode);
        if(a != 0 && r == profile->key_r && g == profile->key_g && b == profile->key_b)
        {
          /* Color key cannot be used if an opaque pixel also has that RGB color. */
          profile->alpha = 1;
          profile->key = 0;
          alpha_done = 1;
          if(profile->bits < 8) profile->bits = 8; /*PNG has no alphachannel modes with less than 8-bit per channel*/
        }
      }
    }

    /*make the profile's key always 16-bit for consistency - repeat each byte twice*/
    profile->key_r += (profile->key_r << 8);
    profile->key_g += (profile->key_g << 8);
    profile->key_b += (profile->key_b << 8);
  }

  color_tree_cleanup(&tree);
  return error;
}

/*Automatically chooses color type that gives smallest amount of bits in the
output image, e.g. grey if there are only greyscale pixels, palette if there
are less than 256 colors, ...
Updates values of mode with a potentially smaller color model. mode_out should
contain the user chosen color model, but will be overwritten with the new chosen one.*/
unsigned lodepng_auto_choose_color(mse::lh::TLHNullableAnyPointer<LodePNGColorMode>  mode_out,
                                   mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h,
                                   mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  mode_in)
{
  mse::TNoradObj<LodePNGColorProfile > prof;
  unsigned error = 0;
  unsigned int i = 0/*auto-generated init val*/; unsigned int n = 0/*auto-generated init val*/; unsigned int palettebits = 0/*auto-generated init val*/; unsigned int palette_ok = 0/*auto-generated init val*/;

  lodepng_color_profile_init(&prof);
  error = lodepng_get_color_profile(&prof, image, w, h, mode_in);
  if(error) return error;
  mode_out->key_defined = 0;

  if(prof.key && w * h <= 16)
  {
    prof.alpha = 1; /*too few pixels to justify tRNS chunk overhead*/
    prof.key = 0;
    if(prof.bits < 8) prof.bits = 8; /*PNG has no alphachannel modes with less than 8-bit per channel*/
  }
  n = prof.numcolors;
  palettebits = n <= 2 ? 1 : (n <= 4 ? 2 : (n <= 16 ? 4 : 8));
  palette_ok = n <= 256 && prof.bits <= 8;
  if(w * h < n * 2) palette_ok = 0; /*don't add palette overhead if image has only a few pixels*/
  if(!prof.colored && prof.bits <= palettebits) palette_ok = 0; /*grey is less overhead*/

  if(palette_ok)
  {
    mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  p = prof.palette;
    lodepng_palette_clear(mode_out); /*remove potential earlier palette*/
    for(i = 0; i != prof.numcolors; ++i)
    {
      error = lodepng_palette_add(mode_out, p[i * 4 + 0], p[i * 4 + 1], p[i * 4 + 2], p[i * 4 + 3]);
      if(error) break;
    }

    mode_out->colortype = LCT_PALETTE;
    mode_out->bitdepth = palettebits;

    if(mode_in->colortype == LCT_PALETTE && mode_in->palettesize >= mode_out->palettesize
        && mode_in->bitdepth == mode_out->bitdepth)
    {
      /*If input should have same palette colors, keep original to preserve its order and prevent conversion*/
      lodepng_color_mode_cleanup(mode_out);
      lodepng_color_mode_copy(mode_out, mode_in);
    }
  }
  else /*8-bit or 16-bit per channel*/
  {
    mode_out->bitdepth = prof.bits;
    mode_out->colortype = prof.alpha ? (prof.colored ? LCT_RGBA : LCT_GREY_ALPHA)
                                     : (prof.colored ? LCT_RGB : LCT_GREY);

    if(prof.key)
    {
      unsigned mask = (1u << mode_out->bitdepth) - 1u; /*profile always uses 16-bit, mask converts it*/
      mode_out->key_r = prof.key_r & mask;
      mode_out->key_g = prof.key_g & mask;
      mode_out->key_b = prof.key_b & mask;
      mode_out->key_defined = 1;
    }
  }

  return error;
}

#endif /* #ifdef LODEPNG_COMPILE_ENCODER */

/*
Paeth predicter, used by PNG filter type 4
The parameters are of type short, but should come from unsigned chars, the shorts
are only needed to make the paeth calculation correct.
*/
static unsigned char paethPredictor(short a, short b, short c)
{
  short pa = abs(b - c);
  short pb = abs(a - c);
  short pc = abs(a + b - c - c);

  if(pc < pa && pc < pb) return (unsigned char)c;
  else if(pb < pa) return (unsigned char)b;
  else return (unsigned char)a;
}

/*shared values used by multiple Adam7 related functions*/

thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 7> ADAM7_IX = { 0, 4, 0, 2, 0, 1, 0 }; /*x start values*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 7> ADAM7_IY = { 0, 0, 4, 0, 2, 0, 1 }; /*y start values*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 7> ADAM7_DX = { 8, 8, 4, 4, 2, 2, 1 }; /*x delta values*/
thread_local static mse::lh::TNativeArrayReplacement<const unsigned int, 7> ADAM7_DY = { 8, 8, 8, 4, 4, 2, 2 }; /*y delta values*/

/*
Outputs various dimensions and positions in the image related to the Adam7 reduced images.
passw: output containing the width of the 7 passes
passh: output containing the height of the 7 passes
filter_passstart: output containing the index of the start and end of each
 reduced image with filter bytes
padded_passstart output containing the index of the start and end of each
 reduced image when without filter bytes but with padded scanlines
passstart: output containing the index of the start and end of each reduced
 image without padding between scanlines, but still padding between the images
w, h: width and height of non-interlaced image
bpp: bits per pixel
"padded" is only relevant if bpp is less than 8 and a scanline or image does not
 end at a full byte
*/
static void Adam7_getpassvalues(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned int>  passw, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned int>  passh, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned long>  filter_passstart,
                                mse::lh::TLHNullableAnyRandomAccessIterator<unsigned long>  padded_passstart, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned long>  passstart, unsigned w, unsigned h, unsigned bpp)
{
  /*the passstart values have 8 values: the 8th one indicates the byte after the end of the 7th (= last) pass*/
  unsigned int i = 0/*auto-generated init val*/;

  /*calculate width and height in pixels of each pass*/
  for(i = 0; i != 7; ++i)
  {
    passw[i] = (w + ADAM7_DX[i] - ADAM7_IX[i] - 1) / ADAM7_DX[i];
    passh[i] = (h + ADAM7_DY[i] - ADAM7_IY[i] - 1) / ADAM7_DY[i];
    if(passw[i] == 0) passh[i] = 0;
    if(passh[i] == 0) passw[i] = 0;
  }

  filter_passstart[0] = padded_passstart[0] = passstart[0] = 0;
  for(i = 0; i != 7; ++i)
  {
    /*if passw[i] is 0, it's 0 bytes, not 1 (no filtertype-byte)*/
    filter_passstart[i + 1] = filter_passstart[i]
                            + ((passw[i] && passh[i]) ? passh[i] * (1 + (passw[i] * bpp + 7) / 8) : 0);
    /*bits padded if needed to fill full byte at end of each scanline*/
    padded_passstart[i + 1] = padded_passstart[i] + passh[i] * ((passw[i] * bpp + 7) / 8);
    /*only padded at end of reduced image*/
    passstart[i + 1] = passstart[i] + (passh[i] * passw[i] * bpp + 7) / 8;
  }
}

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Decoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*read the information from the header and store it in the LodePNGInfo. return value is error*/
unsigned lodepng_inspect(mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TXScopeLHNullableAnyPointer<LodePNGState>  state,
                         mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize)
{
  mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info = &state->info_png;
  if(insize == 0 || in == 0)
  {
    CERROR_RETURN_ERROR(state->error, 48); /*error: the given data is empty*/
  }
  if(insize < 33)
  {
    CERROR_RETURN_ERROR(state->error, 27); /*error: the data length is smaller than the length of a PNG header*/
  }

  /*when decoding a new PNG image, make sure all parameters created after previous decoding are reset*/
  lodepng_info_cleanup(info);
  lodepng_info_init(info);

  if(in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71
     || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10)
  {
    CERROR_RETURN_ERROR(state->error, 28); /*error: the first 8 bytes are not the correct PNG signature*/
  }
  if(lodepng_chunk_length(in + 8) != 13)
  {
    CERROR_RETURN_ERROR(state->error, 94); /*error: header size must be 13 bytes*/
  }
  if(!lodepng_chunk_type_equals(in + 8, "IHDR"))
  {
    CERROR_RETURN_ERROR(state->error, 29); /*error: it doesn't start with a IHDR chunk!*/
  }

  /*read the values given in the header*/
  *w = lodepng_read32bitInt(((in) + (16)));
  *h = lodepng_read32bitInt(((in) + (20)));
  info->color.bitdepth = in[24];
  info->color.colortype = (LodePNGColorType)in[25];
  info->compression_method = in[26];
  info->filter_method = in[27];
  info->interlace_method = in[28];

  if(*w == 0 || *h == 0)
  {
    CERROR_RETURN_ERROR(state->error, 93);
  }

  if(!state->decoder.ignore_crc)
  {
    unsigned CRC = lodepng_read32bitInt(((in) + (29)));
    unsigned checksum = lodepng_crc32(((in) + (12)), 17);
    if(CRC != checksum)
    {
      CERROR_RETURN_ERROR(state->error, 57); /*invalid CRC*/
    }
  }

  /*error: only compression method 0 is allowed in the specification*/
  if(info->compression_method != 0) CERROR_RETURN_ERROR(state->error, 32);
  /*error: only filter method 0 is allowed in the specification*/
  if(info->filter_method != 0) CERROR_RETURN_ERROR(state->error, 33);
  /*error: only interlace methods 0 and 1 exist in the specification*/
  if(info->interlace_method > 1) CERROR_RETURN_ERROR(state->error, 34);

  state->error = checkColorValidity(info->color.colortype, info->color.bitdepth);
  return state->error;
}

static unsigned unfilterScanline(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  recon, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  scanline, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  precon,
                                 size_t bytewidth, unsigned char filterType, size_t length)
{
  /*
  For PNG filter method 0
  unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte,
  the filter works byte per byte (bytewidth = 1)
  precon is the previous unfiltered scanline, recon the result, scanline the current one
  the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
  recon and scanline MAY be the same memory address! precon must be disjoint.
  */

  unsigned long i = 0/*auto-generated init val*/;
  switch(filterType)
  {
    case 0:
      for(i = 0; i != length; ++i) recon[i] = scanline[i];
      break;
    case 1:
      for(i = 0; i != bytewidth; ++i) recon[i] = scanline[i];
      for(i = bytewidth; i < length; ++i) recon[i] = scanline[i] + recon[i - bytewidth];
      break;
    case 2:
      if(precon)
      {
        for(i = 0; i != length; ++i) recon[i] = scanline[i] + precon[i];
      }
      else
      {
        for(i = 0; i != length; ++i) recon[i] = scanline[i];
      }
      break;
    case 3:
      if(precon)
      {
        for(i = 0; i != bytewidth; ++i) recon[i] = scanline[i] + (precon[i] >> 1);
        for(i = bytewidth; i < length; ++i) recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) >> 1);
      }
      else
      {
        for(i = 0; i != bytewidth; ++i) recon[i] = scanline[i];
        for(i = bytewidth; i < length; ++i) recon[i] = scanline[i] + (recon[i - bytewidth] >> 1);
      }
      break;
    case 4:
      if(precon)
      {
        for(i = 0; i != bytewidth; ++i)
        {
          recon[i] = (scanline[i] + precon[i]); /*paethPredictor(0, precon[i], 0) is always precon[i]*/
        }
        for(i = bytewidth; i < length; ++i)
        {
          recon[i] = (scanline[i] + paethPredictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]));
        }
      }
      else
      {
        for(i = 0; i != bytewidth; ++i)
        {
          recon[i] = scanline[i];
        }
        for(i = bytewidth; i < length; ++i)
        {
          /*paethPredictor(recon[i - bytewidth], 0, 0) is always recon[i - bytewidth]*/
          recon[i] = (scanline[i] + recon[i - bytewidth]);
        }
      }
      break;
    default: return 36; /*error: unexisting filter type given*/
  }
  return 0;
}

static unsigned unfilter(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h, unsigned bpp)
{
  /*
  For PNG filter method 0
  this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 seven times)
  out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
  w and h are image dimensions or dimensions of reduced image, bpp is bits per pixel
  in and out are allowed to be the same memory address (but aren't the same size since in has the extra filter bytes)
  */

  unsigned int y = 0/*auto-generated init val*/;
  mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  prevline = nullptr;

  /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
  size_t bytewidth = (bpp + 7) / 8;
  size_t linebytes = (w * bpp + 7) / 8;

  for(y = 0; y < h; ++y)
  {
    size_t outindex = linebytes * y;
    size_t inindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
    unsigned char filterType = in[inindex];

    CERROR_TRY_RETURN(unfilterScanline(((out) + (outindex)), ((in) + (inindex + 1)), prevline, bytewidth, filterType, linebytes));

    prevline = ((out) + (outindex));
  }

  return 0;
}

/*
in: Adam7 interlaced image, with no padding bits between scanlines, but between
 reduced images so that each reduced image starts at a byte.
out: the same pixels, but re-ordered so that they're now a non-interlaced image with size w*h
bpp: bits per pixel
out has the following size in bits: w * h * bpp.
in is possibly bigger due to padding bits between reduced images.
out must be big enough AND must be 0 everywhere if bpp < 8 in the current implementation
(because that's likely a little bit faster)
NOTE: comments about padding bits are only relevant if bpp < 8
*/
static void Adam7_deinterlace(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h, unsigned bpp)
{
  mse::lh::TNativeArrayReplacement<unsigned int, 7> passw; mse::lh::TNativeArrayReplacement<unsigned int, 7> passh;
  mse::lh::TNativeArrayReplacement<unsigned long, 8> filter_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> padded_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> passstart;
  unsigned int i = 0/*auto-generated init val*/;

  Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

  if(bpp >= 8)
  {
    for(i = 0; i != 7; ++i)
    {
      unsigned int x = 0/*auto-generated init val*/; unsigned int y = 0/*auto-generated init val*/; unsigned int b = 0/*auto-generated init val*/;
      size_t bytewidth = bpp / 8;
      for(y = 0; y < passh[i]; ++y)
      for(x = 0; x < passw[i]; ++x)
      {
        size_t pixelinstart = passstart[i] + (y * passw[i] + x) * bytewidth;
        size_t pixeloutstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
        for(b = 0; b < bytewidth; ++b)
        {
          out[pixeloutstart + b] = in[pixelinstart + b];
        }
      }
    }
  }
  else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
  {
    for(i = 0; i != 7; ++i)
    {
      unsigned int x = 0/*auto-generated init val*/; unsigned int y = 0/*auto-generated init val*/; unsigned int b = 0/*auto-generated init val*/;
      unsigned ilinebits = bpp * passw[i];
      unsigned olinebits = bpp * w;
      mse::TNoradObj<unsigned long > obp = 0/*auto-generated init val*/; mse::TNoradObj<unsigned long > ibp = 0/*auto-generated init val*/; /*bit pointers (for out and in buffer)*/
      for(y = 0; y < passh[i]; ++y)
      for(x = 0; x < passw[i]; ++x)
      {
        ibp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
        obp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
        for(b = 0; b < bpp; ++b)
        {
          unsigned char bit = readBitFromReversedStream(&ibp, in);
          /*note that this function assumes the out buffer is completely 0, use setBitOfReversedStream otherwise*/
          setBitOfReversedStream0(&obp, out, bit);
        }
      }
    }
  }
}

static void removePaddingBits(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                              size_t olinebits, size_t ilinebits, unsigned h)
{
  /*
  After filtering there are still padding bits if scanlines have non multiple of 8 bit amounts. They need
  to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers
  for the Adam7 code, the color convert code and the output to the user.
  in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must
  have >= ilinebits*h bits, out must have >= olinebits*h bits, olinebits must be <= ilinebits
  also used to move bits after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
  only useful if (ilinebits - olinebits) is a value in the range 1..7
  */
  unsigned int y = 0/*auto-generated init val*/;
  size_t diff = ilinebits - olinebits;
  mse::TNoradObj<unsigned long > ibp = 0; mse::TNoradObj<unsigned long > obp = 0; /*input and output bit pointers*/
  for(y = 0; y < h; ++y)
  {
    unsigned long x = 0/*auto-generated init val*/;
    for(x = 0; x < olinebits; ++x)
    {
      unsigned char bit = readBitFromReversedStream(&ibp, in);
      setBitOfReversedStream(&obp, out, bit);
    }
    ibp += diff;
  }
}

/*out must be buffer big enough to contain full image, and in must contain the full decompressed data from
the IDAT chunks (with filter index bytes and possible padding bits)
return value is error*/
static unsigned postProcessScanlines(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  in,
                                     unsigned w, unsigned h, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGInfo>  info_png)
{
  /*
  This function converts the filtered-padded-interlaced data into pure 2D image buffer with the PNG's colortype.
  Steps:
  *) if no Adam7: 1) unfilter 2) remove padding bits (= posible extra bits per scanline if bpp < 8)
  *) if adam7: 1) 7x unfilter 2) 7x remove padding bits 3) Adam7_deinterlace
  NOTE: the in buffer will be overwritten with intermediate data!
  */
  unsigned bpp = lodepng_get_bpp(&info_png->color);
  if(bpp == 0) return 31; /*error: invalid colortype*/

  if(info_png->interlace_method == 0)
  {
    if(bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
    {
      CERROR_TRY_RETURN(unfilter(in, in, w, h, bpp));
      removePaddingBits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
    }
    /*we can immediately filter into the out buffer, no other steps needed*/
    else CERROR_TRY_RETURN(unfilter(out, in, w, h, bpp));
  }
  else /*interlace_method is 1 (Adam7)*/
  {
    mse::lh::TNativeArrayReplacement<unsigned int, 7> passw; mse::lh::TNativeArrayReplacement<unsigned int, 7> passh; mse::lh::TNativeArrayReplacement<unsigned long, 8> filter_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> padded_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> passstart;
    unsigned int i = 0/*auto-generated init val*/;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

    for(i = 0; i != 7; ++i)
    {
      CERROR_TRY_RETURN(unfilter(((in) + (padded_passstart[i])), ((in) + (filter_passstart[i])), passw[i], passh[i], bpp));
      /*TODO: possible efficiency improvement: if in this reduced image the bits fit nicely in 1 scanline,
      move bytes instead of bits or move not at all*/
      if(bpp < 8)
      {
        /*remove padding bits in scanlines; after this there still may be padding
        bits between the different reduced images: each reduced image still starts nicely at a byte*/
        removePaddingBits(((in) + (passstart[i])), ((in) + (padded_passstart[i])), passw[i] * bpp,
                          ((passw[i] * bpp + 7) / 8) * 8, passh[i]);
      }
    }

    Adam7_deinterlace(out, in, w, h, bpp);
  }

  return 0;
}

static unsigned readChunk_PLTE(mse::lh::TLHNullableAnyPointer<LodePNGColorMode>  color, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  unsigned int pos = 0; unsigned int i = 0/*auto-generated init val*/;
  if(color->palette) mse::lh::free(color->palette);
  color->palettesize = chunkLength / 3;
  mse::lh::allocate(color->palette, (4 * color->palettesize));
  if(!color->palette && color->palettesize)
  {
    color->palettesize = 0;
    return 83; /*alloc fail*/
  }
  if(color->palettesize > 256) return 38; /*error: palette too big*/

  for(i = 0; i != color->palettesize; ++i)
  {
    color->palette[4 * i + 0] = data[pos++]; /*R*/
    color->palette[4 * i + 1] = data[pos++]; /*G*/
    color->palette[4 * i + 2] = data[pos++]; /*B*/
    color->palette[4 * i + 3] = 255; /*alpha*/
  }

  return 0; /* OK */
}

static unsigned readChunk_tRNS(mse::lh::TLHNullableAnyPointer<LodePNGColorMode>  color, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  unsigned int i = 0/*auto-generated init val*/;
  if(color->colortype == LCT_PALETTE)
  {
    /*error: more alpha values given than there are palette entries*/
    if(chunkLength > color->palettesize) return 38;

    for(i = 0; i != chunkLength; ++i) color->palette[4 * i + 3] = data[i];
  }
  else if(color->colortype == LCT_GREY)
  {
    /*error: this chunk must be 2 bytes for greyscale image*/
    if(chunkLength != 2) return 30;

    color->key_defined = 1;
    color->key_r = color->key_g = color->key_b = 256u * data[0] + data[1];
  }
  else if(color->colortype == LCT_RGB)
  {
    /*error: this chunk must be 6 bytes for RGB image*/
    if(chunkLength != 6) return 41;

    color->key_defined = 1;
    color->key_r = 256u * data[0] + data[1];
    color->key_g = 256u * data[2] + data[3];
    color->key_b = 256u * data[4] + data[5];
  }
  else return 42; /*error: tRNS chunk not allowed for other color models*/

  return 0; /* OK */
}


#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*background color chunk (bKGD)*/
static unsigned readChunk_bKGD(mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  if(info->color.colortype == LCT_PALETTE)
  {
    /*error: this chunk must be 1 byte for indexed color image*/
    if(chunkLength != 1) return 43;

    info->background_defined = 1;
    info->background_r = info->background_g = info->background_b = data[0];
  }
  else if(info->color.colortype == LCT_GREY || info->color.colortype == LCT_GREY_ALPHA)
  {
    /*error: this chunk must be 2 bytes for greyscale image*/
    if(chunkLength != 2) return 44;

    info->background_defined = 1;
    info->background_r = info->background_g = info->background_b = 256u * data[0] + data[1];
  }
  else if(info->color.colortype == LCT_RGB || info->color.colortype == LCT_RGBA)
  {
    /*error: this chunk must be 6 bytes for greyscale image*/
    if(chunkLength != 6) return 45;

    info->background_defined = 1;
    info->background_r = 256u * data[0] + data[1];
    info->background_g = 256u * data[2] + data[3];
    info->background_b = 256u * data[4] + data[5];
  }

  return 0; /* OK */
}

/*text chunk (tEXt)*/
static unsigned readChunk_tEXt(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  unsigned error = 0;
  mse::lh::TStrongVectorIterator<char>  key = nullptr; mse::lh::TStrongVectorIterator<char>  str = nullptr;
  unsigned int i = 0/*auto-generated init val*/;

  while(!error) /*not really a while loop, only used to break on error*/
  {
    unsigned int length = 0/*auto-generated init val*/; unsigned int string2_begin = 0/*auto-generated init val*/;

    length = 0;
    while(length < chunkLength && data[length] != 0) ++length;
    /*even though it's not allowed by the standard, no error is thrown if
    there's no null termination char, if the text is empty*/
    if(length < 1 || length > 79) CERROR_BREAK(error, 89); /*keyword too short or long*/

    mse::lh::allocate(key, (length + 1));
    if(!key) CERROR_BREAK(error, 83); /*alloc fail*/

    key[length] = 0;
    for(i = 0; i != length; ++i) key[i] = (char)data[i];

    string2_begin = length + 1; /*skip keyword null terminator*/

    length = chunkLength < string2_begin ? 0 : chunkLength - string2_begin;
    mse::lh::allocate(str, (length + 1));
    if(!str) CERROR_BREAK(error, 83); /*alloc fail*/

    str[length] = 0;
    for(i = 0; i != length; ++i) str[i] = (char)data[string2_begin + i];

    error = lodepng_add_text(info, key, str);

    break;
  }

  mse::lh::free(key);
  mse::lh::free(str);

  return error;
}

/*compressed text chunk (zTXt)*/
static unsigned readChunk_zTXt(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  zlibsettings,
                               mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  unsigned error = 0;
  unsigned int i = 0/*auto-generated init val*/;

  unsigned int length = 0/*auto-generated init val*/; unsigned int string2_begin = 0/*auto-generated init val*/;
  mse::lh::TStrongVectorIterator<char>  key = nullptr;
  mse::TNoradObj<ucvector > decoded;

  ucvector_init(&decoded);

  while(!error) /*not really a while loop, only used to break on error*/
  {
    for(length = 0; length < chunkLength && data[length] != 0; ++length) ;
    if(length + 2 >= chunkLength) CERROR_BREAK(error, 75); /*no null termination, corrupt?*/
    if(length < 1 || length > 79) CERROR_BREAK(error, 89); /*keyword too short or long*/

    mse::lh::allocate(key, (length + 1));
    if(!key) CERROR_BREAK(error, 83); /*alloc fail*/

    key[length] = 0;
    for(i = 0; i != length; ++i) key[i] = (char)data[i];

    if(data[length + 1] != 0) CERROR_BREAK(error, 72); /*the 0 byte indicating compression must be 0*/

    string2_begin = length + 2;
    if(string2_begin > chunkLength) CERROR_BREAK(error, 75); /*no null termination, corrupt?*/

    length = chunkLength - string2_begin;
    /*will fail if zlib error, e.g. if length is too small*/
    error = zlib_decompress(&decoded.data, &decoded.size,
                            mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>>((&data[string2_begin])),
                            length, zlibsettings);
    if(error) break;
    ucvector_push_back(&decoded, 0);

    error = lodepng_add_text(info, key, mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<const char>>(decoded.data));

    break;
  }

  mse::lh::free(key);
  ucvector_cleanup(&decoded);

  return error;
}

/*international text chunk (iTXt)*/
static unsigned readChunk_iTXt(mse::lh::TLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGDecompressSettings>  zlibsettings,
                               mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  unsigned error = 0;
  unsigned int i = 0/*auto-generated init val*/;

  unsigned int length = 0/*auto-generated init val*/; unsigned int begin = 0/*auto-generated init val*/; unsigned int compressed = 0/*auto-generated init val*/;
  mse::lh::TStrongVectorIterator<char>  key = nullptr; mse::lh::TStrongVectorIterator<char>  langtag = nullptr; mse::lh::TStrongVectorIterator<char>  transkey = nullptr;
  mse::TNoradObj<ucvector > decoded;
  ucvector_init(&decoded);

  while(!error) /*not really a while loop, only used to break on error*/
  {
    /*Quick check if the chunk length isn't too small. Even without check
    it'd still fail with other error checks below if it's too short. This just gives a different error code.*/
    if(chunkLength < 5) CERROR_BREAK(error, 30); /*iTXt chunk too short*/

    /*read the key*/
    for(length = 0; length < chunkLength && data[length] != 0; ++length) ;
    if(length + 3 >= chunkLength) CERROR_BREAK(error, 75); /*no null termination char, corrupt?*/
    if(length < 1 || length > 79) CERROR_BREAK(error, 89); /*keyword too short or long*/

    mse::lh::allocate(key, (length + 1));
    if(!key) CERROR_BREAK(error, 83); /*alloc fail*/

    key[length] = 0;
    for(i = 0; i != length; ++i) key[i] = (char)data[i];

    /*read the compression method*/
    compressed = data[length + 1];
    if(data[length + 2] != 0) CERROR_BREAK(error, 72); /*the 0 byte indicating compression must be 0*/

    /*even though it's not allowed by the standard, no error is thrown if
    there's no null termination char, if the text is empty for the next 3 texts*/

    /*read the langtag*/
    begin = length + 3;
    length = 0;
    for(i = begin; i < chunkLength && data[i] != 0; ++i) ++length;

    mse::lh::allocate(langtag, (length + 1));
    if(!langtag) CERROR_BREAK(error, 83); /*alloc fail*/

    langtag[length] = 0;
    for(i = 0; i != length; ++i) langtag[i] = (char)data[begin + i];

    /*read the transkey*/
    begin += length + 1;
    length = 0;
    for(i = begin; i < chunkLength && data[i] != 0; ++i) ++length;

    mse::lh::allocate(transkey, (length + 1));
    if(!transkey) CERROR_BREAK(error, 83); /*alloc fail*/

    transkey[length] = 0;
    for(i = 0; i != length; ++i) transkey[i] = (char)data[begin + i];

    /*read the actual text*/
    begin += length + 1;

    length = chunkLength < begin ? 0 : chunkLength - begin;

    if(compressed)
    {
      /*will fail if zlib error, e.g. if length is too small*/
      error = zlib_decompress(&decoded.data, &decoded.size,
                              mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>>((&data[begin])),
                              length, zlibsettings);
      if(error) break;
      if(decoded.allocsize < decoded.size) decoded.allocsize = decoded.size;
      ucvector_push_back(&decoded, 0);
    }
    else
    {
      if(!ucvector_resize(&decoded, length + 1)) CERROR_BREAK(error, 83 /*alloc fail*/);

      decoded.data[length] = 0;
      for(i = 0; i != length; ++i) decoded.data[i] = data[begin + i];
    }

    error = lodepng_add_itext(info, key, langtag, transkey, mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<const char>>(decoded.data));

    break;
  }

  mse::lh::free(key);
  mse::lh::free(langtag);
  mse::lh::free(transkey);
  ucvector_cleanup(&decoded);

  return error;
}

static unsigned readChunk_tIME(mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  if(chunkLength != 7) return 73; /*invalid tIME chunk size*/

  info->time_defined = 1;
  info->time.year = 256u * data[0] + data[1];
  info->time.month = data[2];
  info->time.day = data[3];
  info->time.hour = data[4];
  info->time.minute = data[5];
  info->time.second = data[6];

  return 0; /* OK */
}

static unsigned readChunk_pHYs(mse::lh::TXScopeLHNullableAnyPointer<LodePNGInfo>  info, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t chunkLength)
{
  if(chunkLength != 9) return 74; /*invalid pHYs chunk size*/

  info->phys_defined = 1;
  info->phys_x = 16777216u * data[0] + 65536u * data[1] + 256u * data[2] + data[3];
  info->phys_y = 16777216u * data[4] + 65536u * data[5] + 256u * data[6] + data[7];
  info->phys_unit = data[8];

  return 0; /* OK */
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

/*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/
static void decodeGeneric(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h,
                          mse::lh::TLHNullableAnyPointer<LodePNGState>  state,
                          mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize)
{
  unsigned char IEND = 0;
  mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  chunk = nullptr/*auto-generated init val*/;
  unsigned long i = 0/*auto-generated init val*/;
  mse::TNoradObj<ucvector > idat; /*the data from idat chunks*/
  mse::TNoradObj<ucvector > scanlines;
  unsigned long predict = 0/*auto-generated init val*/;
  unsigned long numpixels = 0/*auto-generated init val*/;
  size_t outsize = 0;

  /*for unknown chunk order*/
  unsigned unknown = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  unsigned critical_pos = 1; /*1 = after IHDR, 2 = after PLTE, 3 = after IDAT*/
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

  /*provide some proper output values if error will happen*/
  *out = 0;

  state->error = lodepng_inspect(w, h, state, in, insize); /*reads header and resets other parameters in state->info_png*/
  if(state->error) return;

  numpixels = *w * *h;

  /*multiplication overflow*/
  if(*h != 0 && numpixels / *h != *w) CERROR_RETURN(state->error, 92);
  /*multiplication overflow possible further below. Allows up to 2^31-1 pixel
  bytes with 16-bit RGBA, the rest is room for filter bytes.*/
  if(numpixels > 268435455) CERROR_RETURN(state->error, 92);

  ucvector_init(&idat);
  chunk = ((in) + (33)); /*first byte of the first chunk after the header*/

  /*loop through the chunks, ignoring unknown chunks and stopping at IEND chunk.
  IDAT data is put at the start of the in buffer*/
  while(!IEND && !state->error)
  {
    unsigned int chunkLength = 0/*auto-generated init val*/;
    mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data = nullptr/*auto-generated init val*/; /*the data in the chunk*/

    /*error: size of the in buffer too small to contain next chunk*/
    if((size_t)((chunk - in) + 12) > insize || chunk < in) CERROR_BREAK(state->error, 30);

    /*length of the data of the chunk, excluding the length bytes, chunk type and CRC bytes*/
    chunkLength = lodepng_chunk_length(chunk);
    /*error: chunk length larger than the max PNG chunk size*/
    if(chunkLength > 2147483647) CERROR_BREAK(state->error, 63);

    if((size_t)((chunk - in) + chunkLength + 12) > insize || (chunk + chunkLength + 12) < in)
    {
      CERROR_BREAK(state->error, 64); /*error: size of the in buffer too small to contain next chunk*/
    }

    data = lodepng_chunk_data_const(chunk);

    /*IDAT chunk, containing compressed image data*/
    if(lodepng_chunk_type_equals(chunk, "IDAT"))
    {
      size_t oldsize = idat.size;
      if(!ucvector_resize(&idat, oldsize + chunkLength)) CERROR_BREAK(state->error, 83 /*alloc fail*/);
      for(i = 0; i != chunkLength; ++i) idat.data[oldsize + i] = data[i];
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
      critical_pos = 3;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    }
    /*IEND chunk*/
    else if(lodepng_chunk_type_equals(chunk, "IEND"))
    {
      IEND = 1;
    }
    /*palette chunk (PLTE)*/
    else if(lodepng_chunk_type_equals(chunk, "PLTE"))
    {
      state->error = readChunk_PLTE(&state->info_png.color, data, chunkLength);
      if(state->error) break;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
      critical_pos = 2;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    }
    /*palette transparency chunk (tRNS)*/
    else if(lodepng_chunk_type_equals(chunk, "tRNS"))
    {
      state->error = readChunk_tRNS(&state->info_png.color, data, chunkLength);
      if(state->error) break;
    }
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*background color chunk (bKGD)*/
    else if(lodepng_chunk_type_equals(chunk, "bKGD"))
    {
      state->error = readChunk_bKGD(&state->info_png, data, chunkLength);
      if(state->error) break;
    }
    /*text chunk (tEXt)*/
    else if(lodepng_chunk_type_equals(chunk, "tEXt"))
    {
      if(state->decoder.read_text_chunks)
      {
        state->error = readChunk_tEXt(&state->info_png, data, chunkLength);
        if(state->error) break;
      }
    }
    /*compressed text chunk (zTXt)*/
    else if(lodepng_chunk_type_equals(chunk, "zTXt"))
    {
      if(state->decoder.read_text_chunks)
      {
        state->error = readChunk_zTXt(&state->info_png, &state->decoder.zlibsettings, data, chunkLength);
        if(state->error) break;
      }
    }
    /*international text chunk (iTXt)*/
    else if(lodepng_chunk_type_equals(chunk, "iTXt"))
    {
      if(state->decoder.read_text_chunks)
      {
        state->error = readChunk_iTXt(&state->info_png, &state->decoder.zlibsettings, data, chunkLength);
        if(state->error) break;
      }
    }
    else if(lodepng_chunk_type_equals(chunk, "tIME"))
    {
      state->error = readChunk_tIME(&state->info_png, data, chunkLength);
      if(state->error) break;
    }
    else if(lodepng_chunk_type_equals(chunk, "pHYs"))
    {
      state->error = readChunk_pHYs(&state->info_png, data, chunkLength);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    else /*it's not an implemented chunk type, so ignore it: skip over the data*/
    {
      /*error: unknown critical chunk (5th bit of first byte of chunk type is 0)*/
      if(!lodepng_chunk_ancillary(chunk)) CERROR_BREAK(state->error, 69);

      unknown = 1;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
      if(state->decoder.remember_unknown_chunks)
      {
        state->error = lodepng_chunk_append(((state->info_png.unknown_chunks_data) + (critical_pos - 1)),
                                            ((state->info_png.unknown_chunks_size) + (critical_pos - 1)), chunk);
        if(state->error) break;
      }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    }

    if(!state->decoder.ignore_crc && !unknown) /*check CRC if wanted, only on known chunk types*/
    {
      if(lodepng_chunk_check_crc(chunk)) CERROR_BREAK(state->error, 57); /*invalid CRC*/
    }

    if(!IEND) chunk = lodepng_chunk_next_const(chunk);
  }

  ucvector_init(&scanlines);
  /*predict output size, to allocate exact size for output buffer to avoid more dynamic allocation.
  If the decompressed size does not match the prediction, the image must be corrupt.*/
  if(state->info_png.interlace_method == 0)
  {
    /*The extra *h is added because this are the filter bytes every scanline starts with*/
    predict = lodepng_get_raw_size_idat(*w, *h, &state->info_png.color) + *h;
  }
  else
  {
    /*Adam-7 interlaced: predicted size is the sum of the 7 sub-images sizes*/
    mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  color = &state->info_png.color;
    predict = 0;
    predict += lodepng_get_raw_size_idat((*w + 7) >> 3, (*h + 7) >> 3, color) + ((*h + 7) >> 3);
    if(*w > 4) predict += lodepng_get_raw_size_idat((*w + 3) >> 3, (*h + 7) >> 3, color) + ((*h + 7) >> 3);
    predict += lodepng_get_raw_size_idat((*w + 3) >> 2, (*h + 3) >> 3, color) + ((*h + 3) >> 3);
    if(*w > 2) predict += lodepng_get_raw_size_idat((*w + 1) >> 2, (*h + 3) >> 2, color) + ((*h + 3) >> 2);
    predict += lodepng_get_raw_size_idat((*w + 1) >> 1, (*h + 1) >> 2, color) + ((*h + 1) >> 2);
    if(*w > 1) predict += lodepng_get_raw_size_idat((*w + 0) >> 1, (*h + 1) >> 1, color) + ((*h + 1) >> 1);
    predict += lodepng_get_raw_size_idat((*w + 0), (*h + 0) >> 1, color) + ((*h + 0) >> 1);
  }
  if(!state->error && !ucvector_reserve(&scanlines, predict)) state->error = 83; /*alloc fail*/
  if(!state->error)
  {
    state->error = zlib_decompress(&scanlines.data, &scanlines.size, idat.data,
                                   idat.size, &state->decoder.zlibsettings);
    if(!state->error && scanlines.size != predict) state->error = 91; /*decompressed size doesn't match prediction*/
  }
  ucvector_cleanup(&idat);

  if(!state->error)
  {
    outsize = lodepng_get_raw_size(*w, *h, &state->info_png.color);
    mse::lh::allocate(*out, (outsize));
    if(!*out) state->error = 83; /*alloc fail*/
  }
  if(!state->error)
  {
    for(i = 0; i < outsize; i++) (*out)[i] = 0;
    state->error = postProcessScanlines(*out, scanlines.data, *w, *h, &state->info_png);
  }
  ucvector_cleanup(&scanlines);
}

unsigned lodepng_decode(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h,
                        mse::lh::TLHNullableAnyPointer<LodePNGState>  state,
                        mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize)
{
  *out = 0;
  decodeGeneric(out, w, h, state, in, insize);
  if(state->error) return state->error;
  if(!state->decoder.color_convert || lodepng_color_mode_equal(&state->info_raw, &state->info_png.color))
  {
    /*same color type, no copying or converting of data needed*/
    /*store the info_png color settings on the info_raw so that the info_raw still reflects what colortype
    the raw image has to the end user*/
    if(!state->decoder.color_convert)
    {
      state->error = lodepng_color_mode_copy(&state->info_raw, &state->info_png.color);
      if(state->error) return state->error;
    }
  }
  else
  {
    /*color conversion needed; sort of copy of the data*/
    mse::lh::TStrongVectorIterator<unsigned char>  data = *out;
    unsigned long outsize = 0/*auto-generated init val*/;

    /*TODO: check if this works according to the statement in the documentation: "The converter can convert
    from greyscale input color type, to 8-bit greyscale or greyscale with alpha"*/
    if(!(state->info_raw.colortype == LCT_RGB || state->info_raw.colortype == LCT_RGBA)
       && !(state->info_raw.bitdepth == 8))
    {
      return 56; /*unsupported color mode conversion*/
    }

    outsize = lodepng_get_raw_size(*w, *h, &state->info_raw);
    mse::lh::allocate(*out, (outsize));
    if(!(*out))
    {
      state->error = 83; /*alloc fail*/
    }
    else state->error = lodepng_convert(*out, data, &state->info_raw,
                                        &state->info_png.color, *w, *h);
    mse::lh::free(data);
  }
  return state->error;
}

unsigned lodepng_decode_memory(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                               size_t insize, LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned int error = 0/*auto-generated init val*/;
  mse::TNoradObj<LodePNGState > state;
  lodepng_state_init(&state);
  state.info_raw.colortype = colortype;
  state.info_raw.bitdepth = bitdepth;
  error = lodepng_decode(out, w, h, &state, in, insize);
  lodepng_state_cleanup(&state);
  return error;
}

unsigned lodepng_decode32(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize)
{
  return lodepng_decode_memory(out, w, h, in, insize, LCT_RGBA, 8);
}

unsigned lodepng_decode24(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize)
{
  return lodepng_decode_memory(out, w, h, in, insize, LCT_RGB, 8);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned lodepng_decode_file(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename,
                             LodePNGColorType colortype, unsigned bitdepth)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr;
  mse::TNoradObj<unsigned long > buffersize = 0/*auto-generated init val*/;
  unsigned int error = 0/*auto-generated init val*/;
  error = lodepng_load_file(&buffer, &buffersize, filename);
  if(!error) error = lodepng_decode_memory(out, w, h, buffer, buffersize, colortype, bitdepth);
  mse::lh::free(buffer);
  return error;
}

unsigned lodepng_decode32_file(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename)
{
  return lodepng_decode_file(out, w, h, filename, LCT_RGBA, 8);
}

unsigned lodepng_decode24_file(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  w, mse::lh::TXScopeLHNullableAnyPointer<unsigned int>  h, mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename)
{
  return lodepng_decode_file(out, w, h, filename, LCT_RGB, 8);
}
#endif /*LODEPNG_COMPILE_DISK*/

void lodepng_decoder_settings_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGDecoderSettings>  settings)
{
  settings->color_convert = 1;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  settings->read_text_chunks = 1;
  settings->remember_unknown_chunks = 0;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
  settings->ignore_crc = 0;
  lodepng_decompress_settings_init(&settings->zlibsettings);
}

#endif /*LODEPNG_COMPILE_DECODER*/

#if defined(LODEPNG_COMPILE_DECODER) || defined(LODEPNG_COMPILE_ENCODER)

void lodepng_state_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGState>  state)
{
#ifdef LODEPNG_COMPILE_DECODER
  lodepng_decoder_settings_init(&state->decoder);
#endif /*LODEPNG_COMPILE_DECODER*/
#ifdef LODEPNG_COMPILE_ENCODER
  lodepng_encoder_settings_init(&state->encoder);
#endif /*LODEPNG_COMPILE_ENCODER*/
  lodepng_color_mode_init(&state->info_raw);
  lodepng_info_init(&state->info_png);
  state->error = 1;
}

void lodepng_state_cleanup(mse::lh::TXScopeLHNullableAnyPointer<LodePNGState>  state)
{
  lodepng_color_mode_cleanup(&state->info_raw);
  lodepng_info_cleanup(&state->info_png);
}

void lodepng_state_copy(mse::lh::TXScopeLHNullableAnyPointer<LodePNGState>  dest, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGState>  source)
{
  lodepng_state_cleanup(dest);
  *dest = *source;
  lodepng_color_mode_init(&dest->info_raw);
  lodepng_info_init(&dest->info_png);
  dest->error = lodepng_color_mode_copy(&dest->info_raw, &source->info_raw); if(dest->error) return;
  dest->error = lodepng_info_copy(&dest->info_png, &source->info_png); if(dest->error) return;
}

#endif /* defined(LODEPNG_COMPILE_DECODER) || defined(LODEPNG_COMPILE_ENCODER) */

#ifdef LODEPNG_COMPILE_ENCODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Encoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*chunkName must be string of 4 characters*/
static unsigned addChunk(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  chunkName, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t length)
{
  CERROR_TRY_RETURN(lodepng_chunk_create(&out->data, &out->size, (unsigned)length, chunkName, data));
  out->allocsize = out->size; /*fix the allocsize again*/
  return 0;
}

static void writeSignature(mse::lh::TLHNullableAnyPointer<ucvector>  out)
{
  /*8 bytes PNG signature, aka the magic bytes*/
  ucvector_push_back(out, 137);
  ucvector_push_back(out, 80);
  ucvector_push_back(out, 78);
  ucvector_push_back(out, 71);
  ucvector_push_back(out, 13);
  ucvector_push_back(out, 10);
  ucvector_push_back(out, 26);
  ucvector_push_back(out, 10);
}

static unsigned addChunk_IHDR(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, unsigned w, unsigned h,
                              LodePNGColorType colortype, unsigned bitdepth, unsigned interlace_method)
{
  unsigned error = 0;
  mse::TNoradObj<ucvector > header;
  ucvector_init(&header);

  lodepng_add32bitInt(&header, w); /*width*/
  lodepng_add32bitInt(&header, h); /*height*/
  ucvector_push_back(&header, (unsigned char)bitdepth); /*bit depth*/
  ucvector_push_back(&header, (unsigned char)colortype); /*color type*/
  ucvector_push_back(&header, 0); /*compression method*/
  ucvector_push_back(&header, 0); /*filter method*/
  ucvector_push_back(&header, interlace_method); /*interlace method*/

  error = addChunk(out, "IHDR", header.data, header.size);
  ucvector_cleanup(&header);

  return error;
}

static unsigned addChunk_PLTE(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  unsigned error = 0;
  unsigned long i = 0/*auto-generated init val*/;
  mse::TNoradObj<ucvector > PLTE;
  ucvector_init(&PLTE);
  for(i = 0; i != info->palettesize * 4; ++i)
  {
    /*add all channels except alpha channel*/
    if(i % 4 != 3) ucvector_push_back(&PLTE, info->palette[i]);
  }
  error = addChunk(out, "PLTE", PLTE.data, PLTE.size);
  ucvector_cleanup(&PLTE);

  return error;
}

static unsigned addChunk_tRNS(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyPointer<const LodePNGColorMode>  info)
{
  unsigned error = 0;
  unsigned long i = 0/*auto-generated init val*/;
  mse::TNoradObj<ucvector > tRNS;
  ucvector_init(&tRNS);
  if(info->colortype == LCT_PALETTE)
  {
    size_t amount = info->palettesize;
    /*the tail of palette values that all have 255 as alpha, does not have to be encoded*/
    for(i = info->palettesize; i != 0; --i)
    {
      if(info->palette[4 * (i - 1) + 3] == 255) --amount;
      else break;
    }
    /*add only alpha channel*/
    for(i = 0; i != amount; ++i) ucvector_push_back(&tRNS, info->palette[4 * i + 3]);
  }
  else if(info->colortype == LCT_GREY)
  {
    if(info->key_defined)
    {
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r >> 8));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r & 255));
    }
  }
  else if(info->colortype == LCT_RGB)
  {
    if(info->key_defined)
    {
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r >> 8));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r & 255));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_g >> 8));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_g & 255));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_b >> 8));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_b & 255));
    }
  }

  error = addChunk(out, "tRNS", tRNS.data, tRNS.size);
  ucvector_cleanup(&tRNS);

  return error;
}

static unsigned addChunk_IDAT(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  data, size_t datasize,
                              mse::lh::TXScopeLHNullableAnyPointer<LodePNGCompressSettings>  zlibsettings)
{
  mse::TNoradObj<ucvector > zlibdata;
  unsigned error = 0;

  /*compress with the Zlib compressor*/
  ucvector_init(&zlibdata);
  error = zlib_compress(&zlibdata.data, &zlibdata.size, data, datasize, zlibsettings);
  if(!error) error = addChunk(out, "IDAT", zlibdata.data, zlibdata.size);
  ucvector_cleanup(&zlibdata);

  return error;
}

static unsigned addChunk_IEND(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out)
{
  unsigned error = 0;
  error = addChunk(out, "IEND", 0, 0);
  return error;
}

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS

static unsigned addChunk_tEXt(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  keyword, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  textstring)
{
  unsigned error = 0;
  unsigned long i = 0/*auto-generated init val*/;
  mse::TNoradObj<ucvector > text;
  ucvector_init(&text);
  for(i = 0; keyword[i] != 0; ++i) ucvector_push_back(&text, (unsigned char)keyword[i]);
  if(i < 1 || i > 79) return 89; /*error: invalid keyword size*/
  ucvector_push_back(&text, 0); /*0 termination char*/
  for(i = 0; textstring[i] != 0; ++i) ucvector_push_back(&text, (unsigned char)textstring[i]);
  error = addChunk(out, "tEXt", text.data, text.size);
  ucvector_cleanup(&text);

  return error;
}

static unsigned addChunk_zTXt(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  keyword, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  textstring,
                              mse::lh::TXScopeLHNullableAnyPointer<LodePNGCompressSettings>  zlibsettings)
{
  unsigned error = 0;
  mse::TNoradObj<ucvector > data; mse::TNoradObj<ucvector > compressed;
  unsigned long i = 0/*auto-generated init val*/; unsigned long textsize = strlen(mse::us::lh::make_raw_pointer_from(textstring));

  ucvector_init(&data);
  ucvector_init(&compressed);
  for(i = 0; keyword[i] != 0; ++i) ucvector_push_back(&data, (unsigned char)keyword[i]);
  if(i < 1 || i > 79) return 89; /*error: invalid keyword size*/
  ucvector_push_back(&data, 0); /*0 termination char*/
  ucvector_push_back(&data, 0); /*compression method: 0*/

  error = zlib_compress(&compressed.data, &compressed.size,
                        mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>>(textstring), textsize, zlibsettings);
  if(!error)
  {
    for(i = 0; i != compressed.size; ++i) ucvector_push_back(&data, compressed.data[i]);
    error = addChunk(out, "zTXt", data.data, data.size);
  }

  ucvector_cleanup(&compressed);
  ucvector_cleanup(&data);
  return error;
}

static unsigned addChunk_iTXt(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, unsigned compressed, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  keyword, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  langtag,
                              mse::lh::TLHNullableAnyRandomAccessIterator<const char>  transkey, mse::lh::TLHNullableAnyRandomAccessIterator<const char>  textstring, mse::lh::TXScopeLHNullableAnyPointer<LodePNGCompressSettings>  zlibsettings)
{
  unsigned error = 0;
  mse::TNoradObj<ucvector > data;
  unsigned long i = 0/*auto-generated init val*/; unsigned long textsize = strlen(mse::us::lh::make_raw_pointer_from(textstring));

  ucvector_init(&data);

  for(i = 0; keyword[i] != 0; ++i) ucvector_push_back(&data, (unsigned char)keyword[i]);
  if(i < 1 || i > 79) return 89; /*error: invalid keyword size*/
  ucvector_push_back(&data, 0); /*null termination char*/
  ucvector_push_back(&data, compressed ? 1 : 0); /*compression flag*/
  ucvector_push_back(&data, 0); /*compression method*/
  for(i = 0; langtag[i] != 0; ++i) ucvector_push_back(&data, (unsigned char)langtag[i]);
  ucvector_push_back(&data, 0); /*null termination char*/
  for(i = 0; transkey[i] != 0; ++i) ucvector_push_back(&data, (unsigned char)transkey[i]);
  ucvector_push_back(&data, 0); /*null termination char*/

  if(compressed)
  {
    mse::TNoradObj<ucvector > compressed_data;
    ucvector_init(&compressed_data);
    error = zlib_compress(&compressed_data.data, &compressed_data.size,
                          mse::us::lh::unsafe_cast<mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>>(textstring), textsize, zlibsettings);
    if(!error)
    {
      for(i = 0; i != compressed_data.size; ++i) ucvector_push_back(&data, compressed_data.data[i]);
    }
    ucvector_cleanup(&compressed_data);
  }
  else /*not compressed*/
  {
    for(i = 0; textstring[i] != 0; ++i) ucvector_push_back(&data, (unsigned char)textstring[i]);
  }

  if(!error) error = addChunk(out, "iTXt", data.data, data.size);
  ucvector_cleanup(&data);
  return error;
}

static unsigned addChunk_bKGD(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGInfo>  info)
{
  unsigned error = 0;
  mse::TNoradObj<ucvector > bKGD;
  ucvector_init(&bKGD);
  if(info->color.colortype == LCT_GREY || info->color.colortype == LCT_GREY_ALPHA)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r >> 8));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r & 255));
  }
  else if(info->color.colortype == LCT_RGB || info->color.colortype == LCT_RGBA)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r >> 8));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r & 255));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_g >> 8));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_g & 255));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_b >> 8));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_b & 255));
  }
  else if(info->color.colortype == LCT_PALETTE)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r & 255)); /*palette index*/
  }

  error = addChunk(out, "bKGD", bKGD.data, bKGD.size);
  ucvector_cleanup(&bKGD);

  return error;
}

static unsigned addChunk_tIME(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGTime>  time)
{
  unsigned error = 0;
  mse::lh::TStrongVectorIterator<unsigned char>  data = mse::lh::allocate_dyn_array1<mse::lh::TStrongVectorIterator<unsigned char> >((7));
  if(!data) return 83; /*alloc fail*/
  data[0] = (unsigned char)(time->year >> 8);
  data[1] = (unsigned char)(time->year & 255);
  data[2] = (unsigned char)time->month;
  data[3] = (unsigned char)time->day;
  data[4] = (unsigned char)time->hour;
  data[5] = (unsigned char)time->minute;
  data[6] = (unsigned char)time->second;
  error = addChunk(out, "tIME", data, 7);
  mse::lh::free(data);
  return error;
}

static unsigned addChunk_pHYs(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TXScopeLHNullableAnyPointer<const LodePNGInfo>  info)
{
  unsigned error = 0;
  mse::TNoradObj<ucvector > data;
  ucvector_init(&data);

  lodepng_add32bitInt(&data, info->phys_x);
  lodepng_add32bitInt(&data, info->phys_y);
  ucvector_push_back(&data, info->phys_unit);

  error = addChunk(out, "pHYs", data.data, data.size);
  ucvector_cleanup(&data);

  return error;
}

#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

static void filterScanline(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  scanline, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  prevline,
                           size_t length, size_t bytewidth, unsigned char filterType)
{
  unsigned long i = 0/*auto-generated init val*/;
  switch(filterType)
  {
    case 0: /*None*/
      for(i = 0; i != length; ++i) out[i] = scanline[i];
      break;
    case 1: /*Sub*/
      for(i = 0; i != bytewidth; ++i) out[i] = scanline[i];
      for(i = bytewidth; i < length; ++i) out[i] = scanline[i] - scanline[i - bytewidth];
      break;
    case 2: /*Up*/
      if(prevline)
      {
        for(i = 0; i != length; ++i) out[i] = scanline[i] - prevline[i];
      }
      else
      {
        for(i = 0; i != length; ++i) out[i] = scanline[i];
      }
      break;
    case 3: /*Average*/
      if(prevline)
      {
        for(i = 0; i != bytewidth; ++i) out[i] = scanline[i] - (prevline[i] >> 1);
        for(i = bytewidth; i < length; ++i) out[i] = scanline[i] - ((scanline[i - bytewidth] + prevline[i]) >> 1);
      }
      else
      {
        for(i = 0; i != bytewidth; ++i) out[i] = scanline[i];
        for(i = bytewidth; i < length; ++i) out[i] = scanline[i] - (scanline[i - bytewidth] >> 1);
      }
      break;
    case 4: /*Paeth*/
      if(prevline)
      {
        /*paethPredictor(0, prevline[i], 0) is always prevline[i]*/
        for(i = 0; i != bytewidth; ++i) out[i] = (scanline[i] - prevline[i]);
        for(i = bytewidth; i < length; ++i)
        {
          out[i] = (scanline[i] - paethPredictor(scanline[i - bytewidth], prevline[i], prevline[i - bytewidth]));
        }
      }
      else
      {
        for(i = 0; i != bytewidth; ++i) out[i] = scanline[i];
        /*paethPredictor(scanline[i - bytewidth], 0, 0) is always scanline[i - bytewidth]*/
        for(i = bytewidth; i < length; ++i) out[i] = (scanline[i] - scanline[i - bytewidth]);
      }
      break;
    default: return; /*unexisting filter type given*/
  }
}

/* log2 approximation. A slight bit faster than std::log. */
static float flog2(float f)
{
  float result = 0;
  while(f > 32) { result += 4; f /= 16; }
  while(f > 2) { ++result; f /= 2; }
  return result + 1.442695f * (f * f * f / 3 - 3 * f * f / 2 + 3 * f - 1.83333f);
}

static unsigned filter(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h,
                       mse::lh::TXScopeLHNullableAnyPointer<const LodePNGColorMode>  info, mse::lh::TLHNullableAnyPointer<const LodePNGEncoderSettings>  settings)
{
  /*
  For PNG filter method 0
  out must be a buffer with as size: h + (w * h * bpp + 7) / 8, because there are
  the scanlines with 1 extra byte per scanline
  */

  unsigned bpp = lodepng_get_bpp(info);
  /*the width of a scanline in bytes, not including the filter type*/
  size_t linebytes = (w * bpp + 7) / 8;
  /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
  size_t bytewidth = (bpp + 7) / 8;
  mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  prevline = nullptr;
  unsigned int x = 0/*auto-generated init val*/; unsigned int y = 0/*auto-generated init val*/;
  unsigned error = 0;
  LodePNGFilterStrategy strategy = settings->filter_strategy;

  /*
  There is a heuristic called the minimum sum of absolute differences heuristic, suggested by the PNG standard:
   *  If the image type is Palette, or the bit depth is smaller than 8, then do not filter the image (i.e.
      use fixed filtering, with the filter None).
   * (The other case) If the image type is Grayscale or RGB (with or without Alpha), and the bit depth is
     not smaller than 8, then use adaptive filtering heuristic as follows: independently for each row, apply
     all five filters and select the filter that produces the smallest sum of absolute values per row.
  This heuristic is used if filter strategy is LFS_MINSUM and filter_palette_zero is true.

  If filter_palette_zero is true and filter_strategy is not LFS_MINSUM, the above heuristic is followed,
  but for "the other case", whatever strategy filter_strategy is set to instead of the minimum sum
  heuristic is used.
  */
  if(settings->filter_palette_zero &&
     (info->colortype == LCT_PALETTE || info->bitdepth < 8)) strategy = LFS_ZERO;

  if(bpp == 0) return 31; /*error: invalid color type*/

  if(strategy == LFS_ZERO)
  {
    for(y = 0; y != h; ++y)
    {
      size_t outindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
      size_t inindex = linebytes * y;
      out[outindex] = 0; /*filter type byte*/
      filterScanline(((out) + (outindex + 1)), ((in) + (inindex)), prevline, linebytes, bytewidth, 0);
      prevline = ((in) + (inindex));
    }
  }
  else if(strategy == LFS_MINSUM)
  {
    /*adaptive filtering*/
    mse::lh::TNativeArrayReplacement<unsigned long, 5> sum;
    mse::lh::TNativeArrayReplacement<mse::lh::TStrongVectorIterator<unsigned char> , 5>  attempt; /*five filtering attempts, one for each filter type*/
    size_t smallest = 0;
    unsigned char type = 0/*auto-generated init val*/; unsigned char bestType = 0;

    for(type = 0; type != 5; ++type)
    {
      mse::lh::allocate(attempt[type], (linebytes));
      if(!attempt[type]) return 83; /*alloc fail*/
    }

    if(!error)
    {
      for(y = 0; y != h; ++y)
      {
        /*try the 5 filter types*/
        for(type = 0; type != 5; ++type)
        {
          filterScanline(attempt[type], ((in) + (y * linebytes)), prevline, linebytes, bytewidth, type);

          /*calculate the sum of the result*/
          sum[type] = 0;
          if(type == 0)
          {
            for(x = 0; x != linebytes; ++x) sum[type] += (unsigned char)(attempt[type][x]);
          }
          else
          {
            for(x = 0; x != linebytes; ++x)
            {
              /*For differences, each byte should be treated as signed, values above 127 are negative
              (converted to signed char). Filtertype 0 isn't a difference though, so use unsigned there.
              This means filtertype 0 is almost never chosen, but that is justified.*/
              unsigned char s = attempt[type][x];
              sum[type] += s < 128 ? s : (255U - s);
            }
          }

          /*check if this is smallest sum (or if type == 0 it's the first case so always store the values)*/
          if(type == 0 || sum[type] < smallest)
          {
            bestType = type;
            smallest = sum[type];
          }
        }

        prevline = ((in) + (y * linebytes));

        /*now fill the out values*/
        out[y * (linebytes + 1)] = bestType; /*the first byte of a scanline will be the filter type*/
        for(x = 0; x != linebytes; ++x) out[y * (linebytes + 1) + 1 + x] = attempt[bestType][x];
      }
    }

    for(type = 0; type != 5; ++type) mse::lh::free(attempt[type]);
  }
  else if(strategy == LFS_ENTROPY)
  {
    mse::lh::TNativeArrayReplacement<float, 5> sum;
    mse::lh::TNativeArrayReplacement<mse::lh::TStrongVectorIterator<unsigned char> , 5>  attempt; /*five filtering attempts, one for each filter type*/
    float smallest = 0;
    unsigned int type = 0/*auto-generated init val*/; unsigned int bestType = 0;
    mse::lh::TNativeArrayReplacement<unsigned int, 256> count;

    for(type = 0; type != 5; ++type)
    {
      mse::lh::allocate(attempt[type], (linebytes));
      if(!attempt[type]) return 83; /*alloc fail*/
    }

    for(y = 0; y != h; ++y)
    {
      /*try the 5 filter types*/
      for(type = 0; type != 5; ++type)
      {
        filterScanline(attempt[type], ((in) + (y * linebytes)), prevline, linebytes, bytewidth, type);
        for(x = 0; x != 256; ++x) count[x] = 0;
        for(x = 0; x != linebytes; ++x) ++count[attempt[type][x]];
        ++count[type]; /*the filter type itself is part of the scanline*/
        sum[type] = 0;
        for(x = 0; x != 256; ++x)
        {
          float p = count[x] / (float)(linebytes + 1);
          sum[type] += count[x] == 0 ? 0 : flog2(1 / p) * p;
        }
        /*check if this is smallest sum (or if type == 0 it's the first case so always store the values)*/
        if(type == 0 || sum[type] < smallest)
        {
          bestType = type;
          smallest = sum[type];
        }
      }

      prevline = ((in) + (y * linebytes));

      /*now fill the out values*/
      out[y * (linebytes + 1)] = bestType; /*the first byte of a scanline will be the filter type*/
      for(x = 0; x != linebytes; ++x) out[y * (linebytes + 1) + 1 + x] = attempt[bestType][x];
    }

    for(type = 0; type != 5; ++type) mse::lh::free(attempt[type]);
  }
  else if(strategy == LFS_PREDEFINED)
  {
    for(y = 0; y != h; ++y)
    {
      size_t outindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
      size_t inindex = linebytes * y;
      unsigned char type = settings->predefined_filters[y];
      out[outindex] = type; /*filter type byte*/
      filterScanline(((out) + (outindex + 1)), ((in) + (inindex)), prevline, linebytes, bytewidth, type);
      prevline = ((in) + (inindex));
    }
  }
  else if(strategy == LFS_BRUTE_FORCE)
  {
    /*brute force filter chooser.
    deflate the scanline after every filter attempt to see which one deflates best.
    This is very slow and gives only slightly smaller, sometimes even larger, result*/
    mse::lh::TNativeArrayReplacement<unsigned long, 5> size;
    mse::lh::TNativeArrayReplacement<mse::lh::TStrongVectorIterator<unsigned char> , 5>  attempt; /*five filtering attempts, one for each filter type*/
    size_t smallest = 0;
    unsigned type = 0, bestType = 0;
    mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > dummy = nullptr/*auto-generated init val*/;
    mse::TNoradObj<LodePNGCompressSettings > zlibsettings = settings->zlibsettings;
    /*use fixed tree on the attempts so that the tree is not adapted to the filtertype on purpose,
    to simulate the true case where the tree is the same for the whole image. Sometimes it gives
    better result with dynamic tree anyway. Using the fixed tree sometimes gives worse, but in rare
    cases better compression. It does make this a bit less slow, so it's worth doing this.*/
    zlibsettings.btype = 1;
    /*a custom encoder likely doesn't read the btype setting and is optimized for complete PNG
    images only, so disable it*/
    zlibsettings.custom_zlib = 0;
    zlibsettings.custom_deflate = 0;
    for(type = 0; type != 5; ++type)
    {
      mse::lh::allocate(attempt[type], (linebytes));
      if(!attempt[type]) return 83; /*alloc fail*/
    }
    for(y = 0; y != h; ++y) /*try the 5 filter types*/
    {
      for(type = 0; type != 5; ++type)
      {
        unsigned testsize = linebytes;
        /*if(testsize > 8) testsize /= 8;*/ /*it already works good enough by testing a part of the row*/

        filterScanline(attempt[type], ((in) + (y * linebytes)), prevline, linebytes, bytewidth, type);
        size[type] = 0;
        dummy = 0;
        zlib_compress(&dummy, ((size) + (type)), attempt[type], testsize, &zlibsettings);
        mse::lh::free(dummy);
        /*check if this is smallest size (or if type == 0 it's the first case so always store the values)*/
        if(type == 0 || size[type] < smallest)
        {
          bestType = type;
          smallest = size[type];
        }
      }
      prevline = ((in) + (y * linebytes));
      out[y * (linebytes + 1)] = bestType; /*the first byte of a scanline will be the filter type*/
      for(x = 0; x != linebytes; ++x) out[y * (linebytes + 1) + 1 + x] = attempt[bestType][x];
    }
    for(type = 0; type != 5; ++type) mse::lh::free(attempt[type]);
  }
  else return 88; /* unknown filter strategy */

  return error;
}

static void addPaddingBits(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                           size_t olinebits, size_t ilinebits, unsigned h)
{
  /*The opposite of the removePaddingBits function
  olinebits must be >= ilinebits*/
  unsigned int y = 0/*auto-generated init val*/;
  size_t diff = olinebits - ilinebits;
  mse::TNoradObj<unsigned long > obp = 0; mse::TNoradObj<unsigned long > ibp = 0; /*bit pointers*/
  for(y = 0; y != h; ++y)
  {
    unsigned long x = 0/*auto-generated init val*/;
    for(x = 0; x < ilinebits; ++x)
    {
      unsigned char bit = readBitFromReversedStream(&ibp, in);
      setBitOfReversedStream(&obp, out, bit);
    }
    /*obp += diff; --> no, fill in some value in the padding bits too, to avoid
    "Use of uninitialised value of size ###" warning from valgrind*/
    for(x = 0; x != diff; ++x) setBitOfReversedStream(&obp, out, 0);
  }
}

/*
in: non-interlaced image with size w*h
out: the same pixels, but re-ordered according to PNG's Adam7 interlacing, with
 no padding bits between scanlines, but between reduced images so that each
 reduced image starts at a byte.
bpp: bits per pixel
there are no padding bits, not between scanlines, not between reduced images
in has the following size in bits: w * h * bpp.
out is possibly bigger due to padding bits between reduced images
NOTE: comments about padding bits are only relevant if bpp < 8
*/
static void Adam7_interlace(mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h, unsigned bpp)
{
  mse::lh::TNativeArrayReplacement<unsigned int, 7> passw; mse::lh::TNativeArrayReplacement<unsigned int, 7> passh;
  mse::lh::TNativeArrayReplacement<unsigned long, 8> filter_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> padded_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> passstart;
  unsigned int i = 0/*auto-generated init val*/;

  Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

  if(bpp >= 8)
  {
    for(i = 0; i != 7; ++i)
    {
      unsigned int x = 0/*auto-generated init val*/; unsigned int y = 0/*auto-generated init val*/; unsigned int b = 0/*auto-generated init val*/;
      size_t bytewidth = bpp / 8;
      for(y = 0; y < passh[i]; ++y)
      for(x = 0; x < passw[i]; ++x)
      {
        size_t pixelinstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
        size_t pixeloutstart = passstart[i] + (y * passw[i] + x) * bytewidth;
        for(b = 0; b < bytewidth; ++b)
        {
          out[pixeloutstart + b] = in[pixelinstart + b];
        }
      }
    }
  }
  else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
  {
    for(i = 0; i != 7; ++i)
    {
      unsigned int x = 0/*auto-generated init val*/; unsigned int y = 0/*auto-generated init val*/; unsigned int b = 0/*auto-generated init val*/;
      unsigned ilinebits = bpp * passw[i];
      unsigned olinebits = bpp * w;
      mse::TNoradObj<unsigned long > obp = 0/*auto-generated init val*/; mse::TNoradObj<unsigned long > ibp = 0/*auto-generated init val*/; /*bit pointers (for out and in buffer)*/
      for(y = 0; y < passh[i]; ++y)
      for(x = 0; x < passw[i]; ++x)
      {
        ibp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
        obp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
        for(b = 0; b < bpp; ++b)
        {
          unsigned char bit = readBitFromReversedStream(&ibp, in);
          setBitOfReversedStream(&obp, out, bit);
        }
      }
    }
  }
}

/*out must be buffer big enough to contain uncompressed IDAT chunk data, and in must contain the full image.
return value is error**/
static unsigned preProcessScanlines(mse::lh::TLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                                    unsigned w, unsigned h,
                                    mse::lh::TXScopeLHNullableAnyPointer<const LodePNGInfo>  info_png, mse::lh::TLHNullableAnyPointer<const LodePNGEncoderSettings>  settings)
{
  /*
  This function converts the pure 2D image with the PNG's colortype, into filtered-padded-interlaced data. Steps:
  *) if no Adam7: 1) add padding bits (= posible extra bits per scanline if bpp < 8) 2) filter
  *) if adam7: 1) Adam7_interlace 2) 7x add padding bits 3) 7x filter
  */
  unsigned bpp = lodepng_get_bpp(&info_png->color);
  unsigned error = 0;

  if(info_png->interlace_method == 0)
  {
    *outsize = h + (h * ((w * bpp + 7) / 8)); /*image size plus an extra byte per scanline + possible padding bits*/
    mse::lh::allocate(*out, (*outsize));
    if(!(*out) && (*outsize)) error = 83; /*alloc fail*/

    if(!error)
    {
      /*non multiple of 8 bits per scanline, padding bits needed per scanline*/
      if(bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
      {
        mse::lh::TStrongVectorIterator<unsigned char>  padded = mse::lh::allocate_dyn_array1<mse::lh::TStrongVectorIterator<unsigned char> >((h * ((w * bpp + 7) / 8)));
        if(!padded) error = 83; /*alloc fail*/
        if(!error)
        {
          addPaddingBits(padded, in, ((w * bpp + 7) / 8) * 8, w * bpp, h);
          error = filter(*out, padded, w, h, &info_png->color, settings);
        }
        mse::lh::free(padded);
      }
      else
      {
        /*we can immediately filter into the out buffer, no other steps needed*/
        error = filter(*out, in, w, h, &info_png->color, settings);
      }
    }
  }
  else /*interlace_method is 1 (Adam7)*/
  {
    mse::lh::TNativeArrayReplacement<unsigned int, 7> passw; mse::lh::TNativeArrayReplacement<unsigned int, 7> passh;
    mse::lh::TNativeArrayReplacement<unsigned long, 8> filter_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> padded_passstart; mse::lh::TNativeArrayReplacement<unsigned long, 8> passstart;
    mse::lh::TStrongVectorIterator<unsigned char>  adam7 = nullptr/*auto-generated init val*/;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

    *outsize = filter_passstart[7]; /*image size plus an extra byte per scanline + possible padding bits*/
    mse::lh::allocate(*out, (*outsize));
    if(!(*out)) error = 83; /*alloc fail*/

    mse::lh::allocate(adam7, (passstart[7]));
    if(!adam7 && passstart[7]) error = 83; /*alloc fail*/

    if(!error)
    {
      unsigned int i = 0/*auto-generated init val*/;

      Adam7_interlace(adam7, in, w, h, bpp);
      for(i = 0; i != 7; ++i)
      {
        if(bpp < 8)
        {
          mse::lh::TStrongVectorIterator<unsigned char>  padded = mse::lh::allocate_dyn_array1<mse::lh::TStrongVectorIterator<unsigned char> >((padded_passstart[i + 1] - padded_passstart[i]));
          if(!padded) ERROR_BREAK(83); /*alloc fail*/
          addPaddingBits(padded, ((adam7) + (passstart[i])),
                         ((passw[i] * bpp + 7) / 8) * 8, passw[i] * bpp, passh[i]);
          error = filter((((*out)) + (filter_passstart[i])), padded,
                         passw[i], passh[i], &info_png->color, settings);
          mse::lh::free(padded);
        }
        else
        {
          error = filter((((*out)) + (filter_passstart[i])), ((adam7) + (padded_passstart[i])),
                         passw[i], passh[i], &info_png->color, settings);
        }

        if(error) break;
      }
    }

    mse::lh::free(adam7);
  }

  return error;
}

/*
palette must have 4 * palettesize bytes allocated, and given in format RGBARGBARGBARGBA...
returns 0 if the palette is opaque,
returns 1 if the palette has a single color with alpha 0 ==> color key
returns 2 if the palette is semi-translucent.
*/
static unsigned getPaletteTranslucency(mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  palette, size_t palettesize)
{
  unsigned long i = 0/*auto-generated init val*/;
  unsigned key = 0;
  unsigned r = 0, g = 0, b = 0; /*the value of the color with alpha 0, so long as color keying is possible*/
  for(i = 0; i != palettesize; ++i)
  {
    if(!key && palette[4 * i + 3] == 0)
    {
      r = palette[4 * i + 0]; g = palette[4 * i + 1]; b = palette[4 * i + 2];
      key = 1;
      i = (size_t)(-1); /*restart from beginning, to detect earlier opaque colors with key's value*/
    }
    else if(palette[4 * i + 3] != 255) return 2;
    /*when key, no opaque RGB may have key's RGB*/
    else if(key && r == palette[i * 4 + 0] && g == palette[i * 4 + 1] && b == palette[i * 4 + 2]) return 2;
  }
  return key;
}

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
static unsigned addUnknownChunks(mse::lh::TXScopeLHNullableAnyPointer<ucvector>  out, mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  data, size_t datasize)
{
  mse::lh::TLHNullableAnyRandomAccessIterator<unsigned char>  inchunk = data;
  while((size_t)(inchunk - data) < datasize)
  {
    CERROR_TRY_RETURN(lodepng_chunk_append(&out->data, &out->size, inchunk));
    out->allocsize = out->size; /*fix the allocsize again*/
    inchunk = lodepng_chunk_next(inchunk);
  }
  return 0;
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

unsigned lodepng_encode(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize,
                        mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h,
                        mse::lh::TXScopeLHNullableAnyPointer<LodePNGState>  state)
{
  mse::TNoradObj<LodePNGInfo > info;
  mse::TNoradObj<ucvector > outv;
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > data = nullptr; /*uncompressed version of the IDAT chunk data*/
  mse::TNoradObj<unsigned long > datasize = 0;

  /*provide some proper output values if error will happen*/
  *out = 0;
  *outsize = 0;
  state->error = 0;

  lodepng_info_init(&info);
  lodepng_info_copy(&info, &state->info_png);

  if((info.color.colortype == LCT_PALETTE || state->encoder.force_palette)
      && (info.color.palettesize == 0 || info.color.palettesize > 256))
  {
    state->error = 68; /*invalid palette size, it is only allowed to be 1-256*/
    return state->error;
  }

  if(state->encoder.auto_convert)
  {
    state->error = lodepng_auto_choose_color(&info.color, image, w, h, &state->info_raw);
  }
  if(state->error) return state->error;

  if(state->encoder.zlibsettings.btype > 2)
  {
    CERROR_RETURN_ERROR(state->error, 61); /*error: unexisting btype*/
  }
  if(state->info_png.interlace_method > 1)
  {
    CERROR_RETURN_ERROR(state->error, 71); /*error: unexisting interlace mode*/
  }

  state->error = checkColorValidity(info.color.colortype, info.color.bitdepth);
  if(state->error) return state->error; /*error: unexisting color type given*/
  state->error = checkColorValidity(state->info_raw.colortype, state->info_raw.bitdepth);
  if(state->error) return state->error; /*error: unexisting color type given*/

  if(!lodepng_color_mode_equal(&state->info_raw, &info.color))
  {
    mse::lh::TStrongVectorIterator<unsigned char>  converted = nullptr/*auto-generated init val*/;
    size_t size = (w * h * (size_t)lodepng_get_bpp(&info.color) + 7) / 8;

    mse::lh::allocate(converted, (size));
    if(!converted && size) state->error = 83; /*alloc fail*/
    if(!state->error)
    {
      state->error = lodepng_convert(converted, image, &info.color, &state->info_raw, w, h);
    }
    if(!state->error) preProcessScanlines(&data, &datasize, converted, w, h, &info, &state->encoder);
    mse::lh::free(converted);
  }
  else preProcessScanlines(&data, &datasize, image, w, h, &info, &state->encoder);

  ucvector_init(&outv);
  while(!state->error) /*while only executed once, to break on error*/
  {
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    unsigned long i = 0/*auto-generated init val*/;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*write signature and chunks*/
    writeSignature(&outv);
    /*IHDR*/
    addChunk_IHDR(&outv, w, h, info.color.colortype, info.color.bitdepth, info.interlace_method);
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*unknown chunks between IHDR and PLTE*/
    if(info.unknown_chunks_data[0])
    {
      state->error = addUnknownChunks(&outv, info.unknown_chunks_data[0], info.unknown_chunks_size[0]);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*PLTE*/
    if(info.color.colortype == LCT_PALETTE)
    {
      addChunk_PLTE(&outv, &info.color);
    }
    if(state->encoder.force_palette && (info.color.colortype == LCT_RGB || info.color.colortype == LCT_RGBA))
    {
      addChunk_PLTE(&outv, &info.color);
    }
    /*tRNS*/
    if(info.color.colortype == LCT_PALETTE && getPaletteTranslucency(info.color.palette, info.color.palettesize) != 0)
    {
      addChunk_tRNS(&outv, &info.color);
    }
    if((info.color.colortype == LCT_GREY || info.color.colortype == LCT_RGB) && info.color.key_defined)
    {
      addChunk_tRNS(&outv, &info.color);
    }
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*bKGD (must come between PLTE and the IDAt chunks*/
    if(info.background_defined) addChunk_bKGD(&outv, &info);
    /*pHYs (must come before the IDAT chunks)*/
    if(info.phys_defined) addChunk_pHYs(&outv, &info);

    /*unknown chunks between PLTE and IDAT*/
    if(info.unknown_chunks_data[1])
    {
      state->error = addUnknownChunks(&outv, info.unknown_chunks_data[1], info.unknown_chunks_size[1]);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*IDAT (multiple IDAT chunks must be consecutive)*/
    state->error = addChunk_IDAT(&outv, data, datasize, &state->encoder.zlibsettings);
    if(state->error) break;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*tIME*/
    if(info.time_defined) addChunk_tIME(&outv, &info.time);
    /*tEXt and/or zTXt*/
    for(i = 0; i != info.text_num; ++i)
    {
      if(strlen(mse::us::lh::make_raw_pointer_from(info.text_keys[i])) > 79)
      {
        state->error = 66; /*text chunk too large*/
        break;
      }
      if(strlen(mse::us::lh::make_raw_pointer_from(info.text_keys[i])) < 1)
      {
        state->error = 67; /*text chunk too small*/
        break;
      }
      if(state->encoder.text_compression)
      {
        addChunk_zTXt(&outv, info.text_keys[i], info.text_strings[i], &state->encoder.zlibsettings);
      }
      else
      {
        addChunk_tEXt(&outv, info.text_keys[i], info.text_strings[i]);
      }
    }
    /*LodePNG version id in text chunk*/
    if(state->encoder.add_id)
    {
      unsigned alread_added_id_text = 0;
      for(i = 0; i != info.text_num; ++i)
      {
        if(!strcmp(mse::us::lh::make_raw_pointer_from(info.text_keys[i]), "LodePNG"))
        {
          alread_added_id_text = 1;
          break;
        }
      }
      if(alread_added_id_text == 0)
      {
        addChunk_tEXt(&outv, "LodePNG", LODEPNG_VERSION_STRING); /*it's shorter as tEXt than as zTXt chunk*/
      }
    }
    /*iTXt*/
    for(i = 0; i != info.itext_num; ++i)
    {
      if(strlen(mse::us::lh::make_raw_pointer_from(info.itext_keys[i])) > 79)
      {
        state->error = 66; /*text chunk too large*/
        break;
      }
      if(strlen(mse::us::lh::make_raw_pointer_from(info.itext_keys[i])) < 1)
      {
        state->error = 67; /*text chunk too small*/
        break;
      }
      addChunk_iTXt(&outv, state->encoder.text_compression,
                    info.itext_keys[i], info.itext_langtags[i], info.itext_transkeys[i], info.itext_strings[i],
                    &state->encoder.zlibsettings);
    }

    /*unknown chunks between IDAT and IEND*/
    if(info.unknown_chunks_data[2])
    {
      state->error = addUnknownChunks(&outv, info.unknown_chunks_data[2], info.unknown_chunks_size[2]);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    addChunk_IEND(&outv);

    break; /*this isn't really a while loop; no error happened so break out now!*/
  }

  lodepng_info_cleanup(&info);
  mse::lh::free(data);
  /*instead of cleaning the vector up, give it to the output*/
  *out = outv.data;
  *outsize = outv.size;

  return state->error;
}

unsigned lodepng_encode_memory(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image,
                               unsigned w, unsigned h, LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned int error = 0/*auto-generated init val*/;
  mse::TNoradObj<LodePNGState > state;
  lodepng_state_init(&state);
  state.info_raw.colortype = colortype;
  state.info_raw.bitdepth = bitdepth;
  state.info_png.color.colortype = colortype;
  state.info_png.color.bitdepth = bitdepth;
  lodepng_encode(out, outsize, image, w, h, &state);
  error = state.error;
  lodepng_state_cleanup(&state);
  return error;
}

unsigned lodepng_encode32(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h)
{
  return lodepng_encode_memory(out, outsize, image, w, h, LCT_RGBA, 8);
}

unsigned lodepng_encode24(mse::lh::TXScopeLHNullableAnyPointer<mse::lh::TStrongVectorIterator<unsigned char> >  out, mse::lh::TXScopeLHNullableAnyPointer<unsigned long>  outsize, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h)
{
  return lodepng_encode_memory(out, outsize, image, w, h, LCT_RGB, 8);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned lodepng_encode_file(mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h,
                             LodePNGColorType colortype, unsigned bitdepth)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr/*auto-generated init val*/;
  mse::TNoradObj<unsigned long > buffersize = 0/*auto-generated init val*/;
  unsigned error = lodepng_encode_memory(&buffer, &buffersize, image, w, h, colortype, bitdepth);
  if(!error) error = lodepng_save_file(buffer, buffersize, filename);
  mse::lh::free(buffer);
  return error;
}

unsigned lodepng_encode32_file(mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h)
{
  return lodepng_encode_file(filename, image, w, h, LCT_RGBA, 8);
}

unsigned lodepng_encode24_file(mse::lh::TXScopeLHNullableAnyRandomAccessIterator<const char>  filename, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  image, unsigned w, unsigned h)
{
  return lodepng_encode_file(filename, image, w, h, LCT_RGB, 8);
}
#endif /*LODEPNG_COMPILE_DISK*/

void lodepng_encoder_settings_init(mse::lh::TXScopeLHNullableAnyPointer<LodePNGEncoderSettings>  settings)
{
  lodepng_compress_settings_init(&settings->zlibsettings);
  settings->filter_palette_zero = 1;
  settings->filter_strategy = LFS_MINSUM;
  settings->auto_convert = 1;
  settings->force_palette = 0;
  settings->predefined_filters = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  settings->add_id = 0;
  settings->text_compression = 1;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ERROR_TEXT
/*
This returns the description of a numerical error code in English. This is also
the documentation of all the error codes.
*/
mse::lh::TLHNullableAnyRandomAccessIterator<const char>  lodepng_error_text(unsigned code)
{
  switch(code)
  {
    case 0: return "no error, everything went ok";
    case 1: return "nothing done yet"; /*the Encoder/Decoder has done nothing yet, error checking makes no sense yet*/
    case 10: return "end of input memory reached without huffman end code"; /*while huffman decoding*/
    case 11: return "error in code tree made it jump outside of huffman tree"; /*while huffman decoding*/
    case 13: return "problem while processing dynamic deflate block";
    case 14: return "problem while processing dynamic deflate block";
    case 15: return "problem while processing dynamic deflate block";
    case 16: return "unexisting code while processing dynamic deflate block";
    case 17: return "end of out buffer memory reached while inflating";
    case 18: return "invalid distance code while inflating";
    case 19: return "end of out buffer memory reached while inflating";
    case 20: return "invalid deflate block BTYPE encountered while decoding";
    case 21: return "NLEN is not ones complement of LEN in a deflate block";
     /*end of out buffer memory reached while inflating:
     This can happen if the inflated deflate data is longer than the amount of bytes required to fill up
     all the pixels of the image, given the color depth and image dimensions. Something that doesn't
     happen in a normal, well encoded, PNG image.*/
    case 22: return "end of out buffer memory reached while inflating";
    case 23: return "end of in buffer memory reached while inflating";
    case 24: return "invalid FCHECK in zlib header";
    case 25: return "invalid compression method in zlib header";
    case 26: return "FDICT encountered in zlib header while it's not used for PNG";
    case 27: return "PNG file is smaller than a PNG header";
    /*Checks the magic file header, the first 8 bytes of the PNG file*/
    case 28: return "incorrect PNG signature, it's no PNG or corrupted";
    case 29: return "first chunk is not the header chunk";
    case 30: return "chunk length too large, chunk broken off at end of file";
    case 31: return "illegal PNG color type or bpp";
    case 32: return "illegal PNG compression method";
    case 33: return "illegal PNG filter method";
    case 34: return "illegal PNG interlace method";
    case 35: return "chunk length of a chunk is too large or the chunk too small";
    case 36: return "illegal PNG filter type encountered";
    case 37: return "illegal bit depth for this color type given";
    case 38: return "the palette is too big"; /*more than 256 colors*/
    case 39: return "more palette alpha values given in tRNS chunk than there are colors in the palette";
    case 40: return "tRNS chunk has wrong size for greyscale image";
    case 41: return "tRNS chunk has wrong size for RGB image";
    case 42: return "tRNS chunk appeared while it was not allowed for this color type";
    case 43: return "bKGD chunk has wrong size for palette image";
    case 44: return "bKGD chunk has wrong size for greyscale image";
    case 45: return "bKGD chunk has wrong size for RGB image";
    case 48: return "empty input buffer given to decoder. Maybe caused by non-existing file?";
    case 49: return "jumped past memory while generating dynamic huffman tree";
    case 50: return "jumped past memory while generating dynamic huffman tree";
    case 51: return "jumped past memory while inflating huffman block";
    case 52: return "jumped past memory while inflating";
    case 53: return "size of zlib data too small";
    case 54: return "repeat symbol in tree while there was no value symbol yet";
    /*jumped past tree while generating huffman tree, this could be when the
    tree will have more leaves than symbols after generating it out of the
    given lenghts. They call this an oversubscribed dynamic bit lengths tree in zlib.*/
    case 55: return "jumped past tree while generating huffman tree";
    case 56: return "given output image colortype or bitdepth not supported for color conversion";
    case 57: return "invalid CRC encountered (checking CRC can be disabled)";
    case 58: return "invalid ADLER32 encountered (checking ADLER32 can be disabled)";
    case 59: return "requested color conversion not supported";
    case 60: return "invalid window size given in the settings of the encoder (must be 0-32768)";
    case 61: return "invalid BTYPE given in the settings of the encoder (only 0, 1 and 2 are allowed)";
    /*LodePNG leaves the choice of RGB to greyscale conversion formula to the user.*/
    case 62: return "conversion from color to greyscale not supported";
    case 63: return "length of a chunk too long, max allowed for PNG is 2147483647 bytes per chunk"; /*(2^31-1)*/
    /*this would result in the inability of a deflated block to ever contain an end code. It must be at least 1.*/
    case 64: return "the length of the END symbol 256 in the Huffman tree is 0";
    case 66: return "the length of a text chunk keyword given to the encoder is longer than the maximum of 79 bytes";
    case 67: return "the length of a text chunk keyword given to the encoder is smaller than the minimum of 1 byte";
    case 68: return "tried to encode a PLTE chunk with a palette that has less than 1 or more than 256 colors";
    case 69: return "unknown chunk type with 'critical' flag encountered by the decoder";
    case 71: return "unexisting interlace mode given to encoder (must be 0 or 1)";
    case 72: return "while decoding, unexisting compression method encountering in zTXt or iTXt chunk (it must be 0)";
    case 73: return "invalid tIME chunk size";
    case 74: return "invalid pHYs chunk size";
    /*length could be wrong, or data chopped off*/
    case 75: return "no null termination char found while decoding text chunk";
    case 76: return "iTXt chunk too short to contain required bytes";
    case 77: return "integer overflow in buffer size";
    case 78: return "failed to open file for reading"; /*file doesn't exist or couldn't be opened for reading*/
    case 79: return "failed to open file for writing";
    case 80: return "tried creating a tree of 0 symbols";
    case 81: return "lazy matching at pos 0 is impossible";
    case 82: return "color conversion to palette requested while a color isn't in palette";
    case 83: return "memory allocation failed";
    case 84: return "given image too small to contain all pixels to be encoded";
    case 86: return "impossible offset in lz77 encoding (internal bug)";
    case 87: return "must provide custom zlib function pointer if LODEPNG_COMPILE_ZLIB is not defined";
    case 88: return "invalid filter strategy given for LodePNGEncoderSettings.filter_strategy";
    case 89: return "text chunk keyword too short or long: must have size 1-79";
    /*the windowsize in the LodePNGCompressSettings. Requiring POT(==> & instead of %) makes encoding 12% faster.*/
    case 90: return "windowsize must be a power of two";
    case 91: return "invalid decompressed idat size";
    case 92: return "too many pixels, not supported";
    case 93: return "zero width or height is invalid";
    case 94: return "header chunk must have a size of 13 bytes";
  }
  return "unknown error code";
}
#endif /*LODEPNG_COMPILE_ERROR_TEXT*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // C++ Wrapper                                                          // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_CPP
namespace lodepng
{

#ifdef LODEPNG_COMPILE_DISK
unsigned load_file(mse::mstd::vector<unsigned char>&  buffer, const mse::mstd::string&  filename)
{
  long size = lodepng_filesize(filename.c_str());
  if(size < 0) return 78;
  buffer.resize((size_t)size);
  return size == 0 ? 0 : lodepng_buffer_file(mse::lh::address_of_array_element_replacement(buffer, 0), (size_t)size, filename.c_str());
}

/*write given buffer to the file, overwriting the file, it doesn't append to it.*/
unsigned save_file(const mse::mstd::vector<unsigned char>&  buffer, const mse::mstd::string&  filename)
{
  return lodepng_save_file(buffer.empty() ? 0 : mse::lh::address_of_array_element_replacement(buffer, 0), buffer.size(), filename.c_str());
}
#endif /* LODEPNG_COMPILE_DISK */

#ifdef LODEPNG_COMPILE_ZLIB
#ifdef LODEPNG_COMPILE_DECODER
unsigned decompress(mse::mstd::vector<unsigned char>&  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                    const mse::TNoradObj<LodePNGDecompressSettings >&  settings)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr;
  mse::TNoradObj<unsigned long > buffersize = 0;
  unsigned error = zlib_decompress(&buffer, &buffersize, in, insize, &settings);
  if(buffer)
  {
    out.insert(out.end(), ((buffer) + (0)), ((buffer) + (buffersize)));
    mse::lh::free(buffer);
  }
  return error;
}

unsigned decompress(mse::mstd::vector<unsigned char>&  out, const mse::mstd::vector<unsigned char>&  in,
                    const LodePNGDecompressSettings& settings)
{
  return decompress(out, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), in.size(), settings);
}
#endif /* LODEPNG_COMPILE_DECODER */

#ifdef LODEPNG_COMPILE_ENCODER
unsigned compress(mse::mstd::vector<unsigned char>&  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize,
                  const mse::TNoradObj<LodePNGCompressSettings >&  settings)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr;
  mse::TNoradObj<unsigned long > buffersize = 0;
  unsigned error = zlib_compress(&buffer, &buffersize, in, insize, &settings);
  if(buffer)
  {
    out.insert(out.end(), ((buffer) + (0)), ((buffer) + (buffersize)));
    mse::lh::free(buffer);
  }
  return error;
}

unsigned compress(mse::mstd::vector<unsigned char>&  out, const mse::mstd::vector<unsigned char>&  in,
                  const LodePNGCompressSettings& settings)
{
  return compress(out, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), in.size(), settings);
}
#endif /* LODEPNG_COMPILE_ENCODER */
#endif /* LODEPNG_COMPILE_ZLIB */


#ifdef LODEPNG_COMPILE_PNG

State::State()
{
  lodepng_state_init(this);
}

State::State(const mse::TNoradObj<class lodepng::State >&  other)
{
  lodepng_state_init(this);
  lodepng_state_copy(this, &other);
}

State::~State()
{
  lodepng_state_cleanup(this);
}

State& State::operator=(const mse::TNoradObj<class lodepng::State >&  other)
{
  lodepng_state_copy(this, &other);
  return *this;
}

#ifdef LODEPNG_COMPILE_DECODER

unsigned decode(mse::mstd::vector<unsigned char>&  out, mse::TNoradObj<unsigned int >&  w, mse::TNoradObj<unsigned int >&  h, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in,
                size_t insize, LodePNGColorType colortype, unsigned bitdepth)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr/*auto-generated init val*/;
  unsigned error = lodepng_decode_memory(&buffer, &w, &h, in, insize, colortype, bitdepth);
  if(buffer && !error)
  {
    State state;
    state.info_raw.colortype = colortype;
    state.info_raw.bitdepth = bitdepth;
    size_t buffersize = lodepng_get_raw_size(w, h, &state.info_raw);
    out.insert(out.end(), ((buffer) + (0)), ((buffer) + (buffersize)));
    mse::lh::free(buffer);
  }
  return error;
}

unsigned decode(mse::mstd::vector<unsigned char>&  out, mse::TNoradObj<unsigned int >&  w, mse::TNoradObj<unsigned int >&  h,
                const mse::mstd::vector<unsigned char>&  in, LodePNGColorType colortype, unsigned bitdepth)
{
  return decode(out, w, h, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), (unsigned)in.size(), colortype, bitdepth);
}

unsigned decode(mse::mstd::vector<unsigned char>&  out, mse::TNoradObj<unsigned int >&  w, mse::TNoradObj<unsigned int >&  h,
                mse::TNoradObj<class lodepng::State >&  state,
                mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, size_t insize)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr;
  unsigned error = lodepng_decode(&buffer, &w, &h, &state, in, insize);
  if(buffer && !error)
  {
    size_t buffersize = lodepng_get_raw_size(w, h, &state.info_raw);
    out.insert(out.end(), ((buffer) + (0)), ((buffer) + (buffersize)));
  }
  mse::lh::free(buffer);
  return error;
}

unsigned decode(mse::mstd::vector<unsigned char>&  out, mse::TNoradObj<unsigned int >&  w, mse::TNoradObj<unsigned int >&  h,
                mse::TNoradObj<class lodepng::State >&  state,
                const mse::mstd::vector<unsigned char>&  in)
{
  return decode(out, w, h, state, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), in.size());
}

#ifdef LODEPNG_COMPILE_DISK
unsigned decode(mse::mstd::vector<unsigned char>&  out, mse::TNoradObj<unsigned int >&  w, mse::TNoradObj<unsigned int >&  h, const mse::mstd::string&  filename,
                LodePNGColorType colortype, unsigned bitdepth)
{
  mse::mstd::vector<unsigned char> buffer;
  unsigned error = load_file(buffer, filename);
  if(error) return error;
  return decode(out, w, h, buffer, colortype, bitdepth);
}
#endif /* LODEPNG_COMPILE_DECODER */
#endif /* LODEPNG_COMPILE_DISK */

#ifdef LODEPNG_COMPILE_ENCODER
unsigned encode(mse::mstd::vector<unsigned char>&  out, mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr/*auto-generated init val*/;
  mse::TNoradObj<unsigned long > buffersize = 0/*auto-generated init val*/;
  unsigned error = lodepng_encode_memory(&buffer, &buffersize, in, w, h, colortype, bitdepth);
  if(buffer)
  {
    out.insert(out.end(), ((buffer) + (0)), ((buffer) + (buffersize)));
    mse::lh::free(buffer);
  }
  return error;
}

unsigned encode(mse::mstd::vector<unsigned char>&  out,
                const mse::mstd::vector<unsigned char>&  in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  if(lodepng_get_raw_size_lct(w, h, colortype, bitdepth) > in.size()) return 84;
  return encode(out, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), w, h, colortype, bitdepth);
}

unsigned encode(mse::mstd::vector<unsigned char>&  out,
                mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h,
                mse::TNoradObj<class lodepng::State >&  state)
{
  mse::TNoradObj<mse::lh::TStrongVectorIterator<unsigned char>  > buffer = nullptr/*auto-generated init val*/;
  mse::TNoradObj<unsigned long > buffersize = 0/*auto-generated init val*/;
  unsigned error = lodepng_encode(&buffer, &buffersize, in, w, h, &state);
  if(buffer)
  {
    out.insert(out.end(), ((buffer) + (0)), ((buffer) + (buffersize)));
    mse::lh::free(buffer);
  }
  return error;
}

unsigned encode(mse::mstd::vector<unsigned char>&  out,
                const mse::mstd::vector<unsigned char>&  in, unsigned w, unsigned h,
                mse::TNoradObj<class lodepng::State >&  state)
{
  if(lodepng_get_raw_size(w, h, &state.info_raw) > in.size()) return 84;
  return encode(out, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), w, h, state);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned encode(const mse::mstd::string&  filename,
                mse::lh::TLHNullableAnyRandomAccessIterator<const unsigned char>  in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  mse::mstd::vector<unsigned char> buffer;
  unsigned error = encode(buffer, in, w, h, colortype, bitdepth);
  if(!error) error = save_file(buffer, filename);
  return error;
}

unsigned encode(const mse::mstd::string&  filename,
                const mse::mstd::vector<unsigned char>&  in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  if(lodepng_get_raw_size_lct(w, h, colortype, bitdepth) > in.size()) return 84;
  return encode(filename, in.empty() ? 0 : mse::lh::address_of_array_element_replacement(in, 0), w, h, colortype, bitdepth);
}
#endif /* LODEPNG_COMPILE_DISK */
#endif /* LODEPNG_COMPILE_ENCODER */
#endif /* LODEPNG_COMPILE_PNG */
} /* namespace lodepng */
#endif /*LODEPNG_COMPILE_CPP*/
