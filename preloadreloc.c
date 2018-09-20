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
#include <errno.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef open
#error "TBD"
#endif
#ifdef creat
#error "TBD"
#endif
#ifdef stat
#error "TBD"
#endif
#ifdef lstat
#error "TBD"
#endif
#ifdef dlopen
#error "TBD"
#endif

static int
real_open(const char *path, int oflags, ...)
{
    static int (*next)(const char *, int, ...) = NULL;
    va_list ap;
    mode_t mode = 0;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "open")) == NULL)
        abort();
    va_start(ap, oflags);
    if ((oflags & O_CREAT))
       mode = va_arg(ap, mode_t);
    va_end(ap);
    if ((oflags & O_CREAT))
      return next(path, oflags, mode);
    return next(path, oflags);
}

static int
real_creat(const char *path, mode_t mode)
{
    static int (*next)(const char *, mode_t) = NULL;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "creat")) == NULL)
        abort();
    return next(path, mode);
}

static int
real_stat(const char *path, struct stat *st)
{
    static int (*next)(const char *, struct stat *) = NULL;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "stat")) == NULL)
        abort();
    return next(path, st);
}

#ifdef __linux__
static int
real___xstat(int ver, const char *path, struct stat *st)
{
    static int (*next)(int, const char *, struct stat *) = NULL;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "__xstat")) == NULL)
        abort();
    return next(ver, path, st);
}
#endif

static int
real_lstat(const char *path, struct stat *st)
{
    static int (*next)(const char *, struct stat *) = NULL;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "lstat")) == NULL)
        abort();
    return next(path, st);
}

#ifdef __linux__
static int
real___lxstat(int ver, const char *path, struct stat *st)
{
    static int (*next)(int, const char *, struct stat *) = NULL;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "__lxstat")) == NULL)
        abort();
    return next(ver, path, st);
}
#endif

static void *
real_dlopen(const char *path, int flags)
{
    static void * (*next)(const char *, int) = NULL;

    if (next == NULL && (next = dlsym(RTLD_NEXT, "dlopen")) == NULL)
        abort();
    return next(path, flags);
}

#ifdef RELOC_BUILD_LIBRARY
#undef RELOC_BUILD_LIBRARY
#endif

#define RELOC_BUILD_LDPRELOAD

#include "reloc_base.h"
#include "symvers.h"

pthread_once_t once = PTHREAD_ONCE_INIT;
static const char *prefix;
static const char *top_sentinel;
static const char *root_sentinel;

static void
reloc_init(void)
{
    prefix = secure_getenv("RELOC_PREFIX");
    top_sentinel = secure_getenv("RELOC_TOP_SENTINEL");
    root_sentinel = secure_getenv("RELOC_ROOT_SENTINEL");
}

int
reloc_preload_open(const char *path, int oflags, ...)
{
    char buf[PATH_MAX + 1];
    va_list ap;
    mode_t mode = 0;

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return -1;

    if (!(oflags & O_CREAT))
        return real_open(buf, oflags);
    va_start(ap, oflags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
    return real_open(buf, oflags, mode);
}
asm(".symver reloc_preload_open," sym_open);

int
reloc_preload_creat(const char *path, mode_t mode)
{
    char buf[PATH_MAX + 1];

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return -1;
    return real_creat(buf, mode);
}
asm(".symver reloc_preload_creat," sym_creat);

int
reloc_preload_stat(const char *path, struct stat *st)
{
    char buf[PATH_MAX + 1];

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return -1;
    return real_stat(buf, st);
}
asm(".symver reloc_preload_stat," sym_stat);

#ifdef __linux__
int
reloc_preload___xstat(int ver, const char *path, struct stat *st)
{
    char buf[PATH_MAX + 1];

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return -1;
    if (ver == _STAT_VER)
        return real___xstat(ver, buf, st); /* oh well */
    return stat(buf, st);
}
asm(".symver reloc_preload___xstat," sym___xstat);
#endif

int
reloc_preload_lstat(const char *path, struct stat *st)
{
    char buf[PATH_MAX + 1];

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return -1;
    return real_lstat(buf, st);
}
asm(".symver reloc_preload_lstat," sym_lstat);

#ifdef __linux__
int
reloc_preload___lxstat(int ver, const char *path, struct stat *st)
{
    char buf[PATH_MAX + 1];

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return -1;
    if (ver == _STAT_VER)
        return real___lxstat(ver, buf, st); /* oh well */
    return lstat(buf, st);
}
asm(".symver reloc_preload___lxstat," sym___lxstat);
#endif

void *
reloc_preload_dlopen(const char *path, int flags)
{
    char buf[PATH_MAX + 1];

    pthread_once(&once, reloc_init);
    if ((path = reloc_ex(buf, sizeof(buf), RELOC_CALLER_ADDRESS, prefix,
                         top_sentinel, root_sentinel, path)) == NULL)
        return NULL;
    return real_dlopen(buf, flags);
}
asm(".symver reloc_preload_dlopen," sym_dlopen);
