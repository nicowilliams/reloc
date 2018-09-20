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

#ifndef RELOC_H
#define RELOC_H

#if defined(RELOC_BUILD_LIBRARY) || defined(RELOC_INCLUDE_HEADERS)
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#endif /* RELOC_BUILD_LIBRARY || RELOC_INCLUDE_HEADERS */

#ifndef RELOC_BUILD_LDPRELOAD
#define real_open open
#define real_creat creat
#define real_stat stat
#define real_lstat lstat
#define real_dlopen dlopen
#endif /* !RELOC_BUILD_LDPRELOAD */

#ifndef RELOC_BUILD_LIBRARY
/*
 * When #including this file into an application, we want to make everything
 * static inline.
 *
 * All RELOC_EXPORT functions are public APIs.
 */
#define RELOC_EXPORT static inline

#else /* RELOC_BUILD_LIBRARY */

/*
 * When building libreloc, we don't want to make exported functions static
 * inline.
 */
#define RELOC_EXPORT
#endif /* RELOC_BUILD_LIBRARY */

#define RELOC_CALLER_ADDRESS __builtin_extract_return_addr(__builtin_return_address(0))

#define RELOC_ERET(e, v) ((errno = (e)), (v))

/*
 * When including this header as a header-only library, or when building
 * libreloc.so, the application can define:
 *
 *  - RELOC_PREFIX -> original build-time install prefix
 *  - RELOC_TOP_SENTINEL -> name of a file whose existence denotes "root
 *                          of as-deployed codebase"
 *  - RELOC_ROOT_SENTINEL -> name of a file whose existence denotes "root
 *                           of deployed universe"
 */
#ifndef RELOC_PREFIX
#define RELOC_PREFIX ((const char *)NULL)
#endif

#ifndef RELOC_TOP_SENTINEL
#define RELOC_TOP_SENTINEL ((const char *)NULL)
#endif

#ifndef RELOC_ROOT_SENTINEL
#define RELOC_ROOT_SENTINEL ((const char *)NULL)
#endif

#define RELOC_DEF_SENTINEL(s, def) ((s) == NULL ? (def) : (s))

static inline int
addr2dirname(char *buf, size_t bufsz, void *addr)
{
    Dl_info dli;
    size_t len;
    char *p;
    int save_errno = errno;

    memset(&dli, 0, sizeof(dli));

    buf[0] = '\0';
    /* Shared object abs path must start with a / and have one other / */
    if (addr == NULL)
        return RELOC_ERET(ENOENT, -1);
    if (dladdr(addr, &dli)) {
        int ret;
        /*
         * Ay!  dladdr(3) applied to addresses from the executable is
         * apt to fail.
         *
         * We'll assume that all dladdr(3) failures are due to this and
         * not ENOMEM or some other such error, and we'll fallback on
         * /proc.
         */
        if ((ret = readlink("/proc/self/exe", buf, bufsz)) < 0)
            return ret;
        buf[ret] = '\0';
        if ((p = strrchr(buf, '/')) == buf || p == NULL)
            return RELOC_ERET(ENOENT, -1);
        len = ((uintptr_t)(++p)) - (uintptr_t)buf;
    } else {
        if ((p = strrchr(dli.dli_fname, '/')) == dli.dli_fname || p == NULL)
            return RELOC_ERET(ENOENT, -1);
        /* len = strlen(dirname(dli.dli_fname)), including the trailing / */
        len = ((uintptr_t)(++p)) - (uintptr_t)dli.dli_fname;
        if (len > bufsz - 1 || len > INT_MAX /* impossible */)
            return RELOC_ERET(ENAMETOOLONG, -1);
        (void) memcpy(buf, dli.dli_fname, len);
    }
    if (buf[0] != '/')
        return RELOC_ERET(ENOENT, -1);
    buf[len] = '\0';
    errno = save_errno;
    return (int)len;
}

enum reloc_token {
    RELOC_INVAL = 0,
    RELOC_ROOT = 1,
    RELOC_ORIGIN = 2,
    RELOC_TOP = 3,
    RELOC_BINDIR = 4,
    RELOC_LIBDIR = 5,
    RELOC_LIBEXECDIR = 6,
    RELOC_SHAREDIR = 7,
    RELOC_LOCALSTATEDIR = 8,
    RELOC_ETCDIR = 9,
    RELOC_EXE_ORIGIN = 10,
};

static inline enum reloc_token
which_token(const char *path)
{
    if (path[0] != '$')
        return RELOC_INVAL;
    switch (path[1]) {
    case 'R':
        return strncmp(path, "$ROOT/", sizeof("$ROOT/") - 1) == 0 ?
            RELOC_ROOT : RELOC_INVAL;
    case 'T':
        return strncmp(path, "$TOP/", sizeof("$TOP/") - 1) == 0 ?
            RELOC_TOP : RELOC_INVAL;
    case 'O':
        return strncmp(path, "$ORIGIN/", sizeof("$ORIGIN/") - 1) == 0 ?
            RELOC_ORIGIN : RELOC_INVAL;
    case 'E':
        if (strncmp(path, "$EXE_ORIGIN/", sizeof("$EXE_ORIGIN/") - 1) == 0)
            return RELOC_EXE_ORIGIN;
        return strncmp(path, "$ETCDIR/", sizeof("$ETCDIR/") - 1) == 0 ?
            RELOC_ETCDIR : RELOC_INVAL;
    case 'B':
        return strncmp(path, "$BINDIR/", sizeof("$BINDIR/") - 1) == 0 ?
            RELOC_BINDIR : RELOC_INVAL;
    case 'L':
        if (strncmp(path, "$LIBDIR/", sizeof("$LIBDIR/") - 1) == 0)
            return RELOC_LIBDIR;
        if (strncmp(path, "$LIBEXECDIR/", sizeof("$LIBEXECDIR/") - 1) == 0)
            return RELOC_LIBEXECDIR;
        return strncmp(path, "$LOCALSTATEDIR/", sizeof("$LOCALSTATEDIR/") - 1) == 0 ?
            RELOC_LOCALSTATEDIR : RELOC_INVAL;
    case 'S':
        return strncmp(path, "$SHAREDIR/", sizeof("$SHAREDIR/") - 1) == 0 ?
            RELOC_SHAREDIR : RELOC_INVAL;
    default: return RELOC_INVAL;
    }
}

static inline int
reloc_find_sentinel(char *buf, size_t bufsz, const char *sentinel, int def_parents)
{
    char *p, *prev;
    int save_errno = errno;
    int distance = 0;
    int fd = -1;

    if (buf[0] != '/')
        return -1;
    if (sentinel) {
        while (buf[strlen(buf) - 1] == '/')
            buf[strlen(buf) - 1] = '\0'; /* eat trailing slashes */

        for (prev = NULL, p = buf + strlen(buf); buf[0] != '\0'; distance++) {
            struct stat st;

            if (fd != -1) {
                (void) close(fd);
                fd = -1;
            }

            if ((fd = real_open(buf, O_RDONLY)) == -1)
                break;
            if (fstatat(fd, sentinel, &st, 0) == 0) {
                (void) close(fd);
                errno = save_errno;
                if (buf[strlen(buf) - 1] != '/') {
                    if (strlen(buf) + 2 > bufsz)
                        return RELOC_ERET(ENAMETOOLONG, -1);
                    buf[strlen(buf) + 1] = '\0';
                    buf[strlen(buf)] = '/';
                }
                return distance;
            }
            if (errno != ENOENT)
                break;

            p = strrchr(buf, '/');
            if (prev)
                *prev = '/';
            prev = p;
            *p = '\0';
        }
        if (fd != -1)
            (void) close(fd);
        if (prev)
            *prev = '/';
    }

    /* Fallback on some number of .. traversals */
    if (def_parents > 0) {
        for (prev = NULL, distance = 0; def_parents-- > 0; distance++) {
            if ((p = strrchr(buf, '/')) == buf)
                break;
            if (prev)
                *prev = '/';
            prev = p;
            *p = '\0';
        }
        if (def_parents == 0)
            return distance;
    }
    if (prev)
        *prev = '/';
    errno = save_errno;
    return -1;
}

/**
 * Adjust @path to be relative to the deployed location of the
 * application.
 *
 * @param buf   Buffer into which to write relocated path
 * @param bufsz Size of `buffer`
 * @param addr  Address of caller that supplied `path` to the caller of this function
 * @param prefix Build-time installation PREFIX
 * @param top_sentinel  Name of a file (or directory) whose existence denotes "top of codebase as deployed"
 * @param root_sentinel Name of a file (or directory) whose existence denotes "top of deployed universe"
 * @param path  Path to relocate
 *
 * @return Returns NULL on failure (and sets errno), else returns buf.
 */
/* XXX Replace prefix arg with struct that has prefixes */
RELOC_EXPORT const char *
reloc_ex(char *buf, size_t bufsz, void *addr, const char *prefix,
         const char *top_sentinel, const char *root_sentinel,
         const char *path)
{
    enum reloc_token which;
    size_t pathlen, buflen;
    const char *ret = buf;
    ssize_t bytes;
    char *p;

    addr = addr ? addr : RELOC_CALLER_ADDRESS;
    prefix = prefix ? prefix : RELOC_PREFIX;
    if (prefix != NULL && *prefix == '\0')
        prefix = NULL;

    /*
     * For all cases here the pattern is:
     *
     * 1. increment path so it points to a suffix of the original
     * 2. set pathlen to the length of that suffix
     * 3. goto finish
     *
     * In particular, pathlen MUST be set before jumping to finish.
     */

    if (prefix != NULL && prefix[0] != '/')
        prefix = NULL;

    buf[0] = '\0';
    if ((path[0] != '/' && path[0] != '$') ||
        (prefix == NULL && path[0] != '$')) {

        /*
         * Not an absolute path nor a path that starts with $TOKEN/,
         * Or no prefix given and path does not start with $TOKEN/.
         */
        pathlen = strlen(path);
        goto finish;
    }

    if (prefix != NULL && path[0] == '/') {
        size_t prefixlen;

        /* Absolute path and prefix given: check if path starts with it */
        if ((pathlen = strlen(path)) < (prefixlen = strlen(prefix)) ||
            strncmp(path, prefix, prefixlen) != 0 ||
            (prefix[prefixlen - 1] != '/' && path[prefixlen] != '/'))
            goto finish; /* No, path does not start with prefix */

        /* Yes, path does start with prefix */
        if ((bytes = addr2dirname(buf, bufsz, addr)) < 0)
            return NULL;
        buflen = bytes;

        if (reloc_find_sentinel(buf, bufsz,
                                RELOC_DEF_SENTINEL(top_sentinel,
                                                   RELOC_TOP_SENTINEL),
                                1) < 0)
            return RELOC_ERET(ENOENT, NULL);

        buflen = strlen(buf);
        buf += buflen, bufsz -= buflen;
        path += prefixlen, pathlen -= prefixlen;
        while (path[0] == '/') path++, pathlen--;
        goto finish;
    } else if (path[0] != '$') {
        /* Relative path or prefix not given */
        pathlen = strlen(path);
        goto finish;
    } /* else continue below, unindented */

    if ((which = which_token(path)) == RELOC_INVAL)
        return RELOC_ERET(ENOENT, NULL);

    /* First the one case that doesn't require a call to addr2dirname() */
    if (which == RELOC_EXE_ORIGIN) {
        path += sizeof("$EXE_ORIGIN/") - 1, pathlen = strlen(path);
        if ((bytes = readlink("/proc/self/exe", buf, bufsz)) < 0)
            return NULL;
        buflen = bytes;

        if (buflen > bufsz - 1)
            return RELOC_ERET(ENAMETOOLONG, NULL);
        if ((p = strrchr(buf, '/')) == NULL)
            return RELOC_ERET(EINVAL, NULL);
        *(++p) = '\0';
        buflen = p - buf;
        buf += buflen, bufsz -= buflen;
        goto finish;
    }

    if ((bytes = addr2dirname(buf, bufsz, addr)) < 0)
        return NULL; /* errno set by addr2dirname() */
    buflen = bytes;

    if (which == RELOC_ORIGIN) {
        path += sizeof("$ORIGIN/") - 1, pathlen = strlen(path);
        buf += buflen, bufsz -= buflen;
        goto finish;
    }

    /* All other cases are $ROOT, $TOP, or $*DIR */

    /* Start by working out $TOP */
    if (reloc_find_sentinel(buf, bufsz,
                            RELOC_DEF_SENTINEL(top_sentinel,
                                               RELOC_TOP_SENTINEL),
                            1) < 0)
        return RELOC_ERET(ENOENT, NULL);

    if (which == RELOC_TOP) {
        path += sizeof("$TOP/") - 1, pathlen = strlen(path);
        buflen = strlen(buf);
        buf += buflen, bufsz -= buflen;
        goto finish;
    } else if (which == RELOC_ROOT) {
        path += sizeof("$ROOT/") - 1, pathlen = strlen(path);
        if (reloc_find_sentinel(buf, bufsz,
                                RELOC_DEF_SENTINEL(root_sentinel,
                                                   RELOC_ROOT_SENTINEL), -1) < 0)
            return RELOC_ERET(ENOENT, NULL);
        buflen = strlen(buf);
        buf += buflen, bufsz -= buflen;
        goto finish;
    } else {
        const char *suffix;
        size_t suffixlen;

        /* All other cases are $*DIR */
        switch (which) {
        case RELOC_BINDIR:
            path += sizeof("$BINDIR/") - 1, suffix = "bin/", suffixlen = sizeof("bin/") - 1;
            break;
        case RELOC_LIBDIR:
            path += sizeof("$LIBDIR/") - 1, suffix = "lib/", suffixlen = sizeof("lib/") - 1;
            break;
        case RELOC_LIBEXECDIR:
            path += sizeof("$LIBEXECDIR/") - 1, suffix = "libexec/", suffixlen = sizeof("libexec/") - 1;
            break;
        case RELOC_SHAREDIR:
            path += sizeof("$SHAREDIR/") - 1, suffix = "share/", suffixlen = sizeof("share/") - 1;
            break;
        case RELOC_LOCALSTATEDIR:
            path += sizeof("$LOCALSTATEDIR/") - 1, suffix = "var/", suffixlen = sizeof("var/") - 1;
            break;
        case RELOC_ETCDIR:
            path += sizeof("$ETCDIR/") - 1, suffix = "etc/", suffixlen = sizeof("etc/") - 1;
            break;
        default: return RELOC_ERET(EINVAL, NULL);
        }

        if (suffixlen + 1 > bufsz)
            return RELOC_ERET(ENAMETOOLONG, NULL);
        buflen = strlen(buf);
        buf += buflen, bufsz -= buflen;
        (void) memcpy(buf, suffix, suffixlen);
        buf += suffixlen, bufsz -= suffixlen;
        goto finish;
    }

finish:
    buf[0] = '\0';
    if ((pathlen = strlen(path)) > bufsz - 1)
        return RELOC_ERET(ENAMETOOLONG, NULL);
    (void) memcpy(buf, path, pathlen + 1);
    return ret;
}

/**
 * Adjust @path to be relative to the deployed location of the
 * application.
 *
 * @param buf   Buffer into which to write relocated path
 * @param bufsz Size of `buffer`
 * @param addr  Address of caller that supplied `path` to the caller of this function
 * @param top_sentinel  Name of a file (or directory) whose existence denotes "top of codebase as deployed"
 * @param root_sentinel Name of a file (or directory) whose existence denotes "top of deployed universe"
 * @param path  Path to relocate
 *
 * @return Returns NULL on failure (and sets errno), else returns buf.
 */
RELOC_EXPORT const char *
reloc(char *buf, size_t bufsz, void *addr,
      const char *path)
{
    return reloc_ex(buf, bufsz, addr ? addr : RELOC_CALLER_ADDRESS,
                    RELOC_PREFIX, RELOC_TOP_SENTINEL,
                    RELOC_ROOT_SENTINEL, path);
}

#ifndef RELOC_BUILD_LIBRARY
/**
 * Open a file, as if by open(2), with the path rewritten to account for
 * deploy-time relocation of the caller's ELF object or executable.
 *
 * @param path    Path to relocate
 * @param oflags  Open flags
 * @param ...     If O_CREAT is set in `oflags`, then a mode_t must be * supplied
 *
 * @return Returns -1 on failure (and sets errno), or a file descriptor number
 */
RELOC_EXPORT int
reloc_wrap_open(const char *path, int oflags, ...)
{
    va_list ap;
    char buf[PATH_MAX + 1];
    mode_t mode;

    if ((path = reloc(buf, sizeof(buf), RELOC_CALLER_ADDRESS, path)) == NULL)
        return -1;
    if (!(oflags & O_CREAT))
        return open(path, oflags);
    va_start(ap, oflags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
    return open(path, oflags, mode);
}

#define RELOC_FUNC(ret_type, err_ret, libc_func_name, prototype, args, ...) \
ret_type reloc_wrap_ ## libc_func_name(__VA_ARGS__) {                       \
    char buf[PATH_MAX + 1];                                                 \
    if ((path = reloc(buf, sizeof(buf), RELOC_CALLER_ADDRESS,               \
                      path)) == NULL)                                       \
        return (err_ret);                                                   \
    return libc_func_name args;                                             \
}

#include <reloc_funcs.inc>
#undef RELOC_FUNC

#ifndef RELOC_BUILD_LDPRELOAD

/* We're being #included into an application */

#ifdef RELOC_WRAP_FUNCTIONS
/* Rename C library functions for which we have wrappers */
#ifdef open
#undef open
#endif
#define open(p,o,...)   reloc_wrap_creat((p),(o),__VA_ARGS__)
#ifdef creat
#undef creat
#endif
#define creat(p,m)      reloc_wrap_creat((p),(m))
#ifdef stat
#undef stat
#endif
#define stat(p,s)       reloc_wrap_stat((p),(s))
#ifdef lstat
#undef lstat
#endif
#define lstat(p,s)      reloc_wrap_lstat((p),(s))
#ifdef dlopen
#undef dlopen
#endif
#define dlopen(p,f)     reloc_wrap_dlopen((p),(f))
#endif /* RELOC_WRAP_FUNCTIONS */

/* Unpollute namespace where we can */

#undef RELOC_CALLER_ADDRESS
#undef RELOC_ROOT_SENTINEL
#undef RELOC_TOP_SENTINEL
#undef RELOC_DEF_SENTINEL
#undef RELOC_EXPORT
#undef RELOC_PREFIX
#undef RELOC_ERET
#endif /* !RELOC_BUILD_LDPRELOAD */
#endif /* !RELOC_BUILD_LIBRARY */

#endif /* RELOC_H */
