#!/bin/bash
set -x

././scpptool-master/src/scpptool -ConvertToSCPP -AddPrecedingIncludeConfigDotHDirective -ModifiablePath ./wget-1.25.0/src -ModifiablePath ./wget-build/src/version.c -SuppressPrompts $@ -- -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"./wget-build/wgetrc\" -DLOCALEDIR=\"./wget-build/share/locale\" -I. -I./wget-1.25.0/src -I./msetl  -I./wget-1.25.0/lib  -I./wget-build/src  -I./wget-build/lib      -DHAVE_LIBSSL -DHAVE_STRLCPY -DO_BINARY=0 -std=c++23 -c -x c++ > scppt_conv_out1.txt

clang++ -DHAVE_CONFIG_H -DSYSTEM_WGETRC=\"./wget-build/wgetrc\" -DLOCALEDIR=\"./wget-build/share/locale\" -I. -I./wget-1.25.0/src -I./msetl  -I./wget-1.25.0/lib  -I./wget-build/src  -I./wget-build/lib      -DHAVE_LIBSSL -DHAVE_STRLCPY -DO_BINARY=0 -std=c++23 -g -c -x c++ $@

