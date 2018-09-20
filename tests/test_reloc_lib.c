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

#include <sys/auxv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "libreloc.h"

int
main(void)
{
    struct stat st;
    const char *exe_path, *path;
    ssize_t bytes;
    char exe_origin[PATH_MAX];
    char buf[PATH_MAX];

    if ((exe_path = (char *)(uintptr_t)getauxval(AT_EXECFN)) == NULL) {
        bytes = readlink("/proc/self/exe", exe_origin, sizeof(exe_origin));
        if (bytes < 0 || bytes > sizeof(exe_origin) - 1)
            err(1, "could not determine path to test executable");
        exe_path = exe_origin;
    }

    if ((path = reloc(buf, sizeof(buf), NULL, "$ORIGIN/../bin/foobin")) == NULL)
        err(1, "could not resolve $ORIGIN/../bin/foobin");
    if (stat(path, &st) == -1)
        err(1, "resolved $ORIGIN/../bin/foobin to %s, but it does not exist\n", path);
    printf("PASS %d\n", __LINE__);
    return 0;
}

