/*
 * Copyright (c) 2018, Cryptonector LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define RELOC_PREFIX MY_CONFIGURED_PREFIX
#define RELOC_ROOT_SENTINEL "root.sentinel"
#define RELOC_TOP_SENTINEL "config/top.sentinel"
#define RELOC_INCLUDE_HEADERS
#define RELOC_WRAP_FUNCTIONS
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include "reloc_base.h"

/*
 * Now open(), creat(), stat(), lstat(), and dlopen() will rewrite
 * prefix-relative paths, and also $TOKEN/-relative paths, for the tokens
 * listed above.
 */

#define require(r, e) \
    ((e == r) ? printf("PASS %d\n", (int)__LINE__) : (printf("FAIL %d\n", (int)__LINE__), failures++))

int
main(void)
{
    struct stat st;
    int failures = 0;

    require(0, stat(MY_CONFIGURED_PREFIX "/bin/foobin", &st));
    require(0, stat("$ROOT/test01/bin/foobin", &st));
    require(0, stat("$ORIGIN/../share/footxt", &st));
    require(0, stat("$TOP/share/footxt", &st));
    require(0, stat("$BINDIR/foobin", &st));
    require(0, stat("$LIBDIR/foolib", &st));
    require(0, stat("$LIBEXECDIR/foo", &st));
    require(0, stat("$SHAREDIR/footxt", &st));
}

