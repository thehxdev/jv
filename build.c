#ifndef __linux__
    #error "At this point, only Linux is supported"
#endif

#if !defined(__GNUC__) && !defined(__clang__)
    #error "only GCC and Clang compilers are supported"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "jv_x86_64.c"
#include "main.c"
