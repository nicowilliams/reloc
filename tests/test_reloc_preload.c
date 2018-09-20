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

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define require(e) \
    ((e) ? printf("PASS %d\n", (int)__LINE__) : (printf("FAIL %d\n", (int)__LINE__), failures++))

int
main(void)
{
    struct stat st;
    int failures = 0;

    require(open(MY_CONFIGURED_PREFIX "/bin/foobin", O_RDONLY) != -1);
    require(stat(MY_CONFIGURED_PREFIX "/bin/foobin", &st) != -1);
    require(open("$ROOT/test01/bin/foobin", O_RDONLY) != -1);
    require(open("$ORIGIN/../share/footxt", O_RDONLY) != -1);
    require(open("$TOP/share/footxt", O_RDONLY) != -1);
    require(open("$BINDIR/foobin", O_RDONLY) != -1);
    require(open("$LIBDIR/foolib", O_RDONLY) != -1);
    require(open("$LIBEXECDIR/foo", O_RDONLY) != -1);
    require(open("$SHAREDIR/footxt", O_RDONLY) != -1);
    return failures;
}
