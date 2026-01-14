#!/bin/bash
set -x

./scpptool-master/src/scpptool -ConvertC2ValidCpp -SuppressPrompts $@ -- -DGNULIB_NAMESPACE=gnulib -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"./wget-build/wgetrc\" -DLOCALEDIR=\"./wget-build/share/locale\" -I. -I./wget-1.25.0/src  -I./wget-1.25.0/lib  -I./wget-build/src  -I./wget-build/lib      -DHAVE_LIBSSL -DHAVE_STRLCPY -DO_BINARY=0 -DO_TEXT=0 -DO_SEARCH=O_RDONLY -c > scppt_conv_out1.txt

clang++ -DGNULIB_NAMESPACE=gnulib -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"./wget-build/wgetrc\" -DLOCALEDIR=\"./wget-build/share/locale\" -I. -I./wget-1.25.0/src  -I./wget-1.25.0/lib  -I./wget-build/src  -I./wget-build/lib      -DHAVE_LIBSSL -DHAVE_STRLCPY -DO_BINARY=0 -DO_TEXT=0 -DO_SEARCH=O_RDONLY -std=c++20 -c -x c++ $@

