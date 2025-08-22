#ifndef __linux__
    #error "only Linux is supported"
#endif

#if !defined(__GNUC__) && !defined(__clang__)
    #error "only GCC and Clang compilers are supported"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "arena.h"
#include "mempool.h"

#include "arena.c"
#include "mempool.c"
#include "jv_x86_64.c"
#include "main.c"
