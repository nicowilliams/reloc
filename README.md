What is this?
=============

First of all, this is a WORK *IN PROGRESS* and *proof of concept*.

*MORE TESTS ARE NEEDED*.

*DO NOT USE*.

Reloc is a library to help make relocatable codebases that are not
deploy-time relocatable.

For example, autoconf codebases configured with `--prefix=/some/path'
are generally not able to be deployed to other locations without
rebuilding.  Reloc is meant to help make C/C++ codebases easier to make
deploy-time relocatable.

This library is meant to have several components:

 - an header-only library that providing the following components:

    - functions similar to various -lc and -ldl functions where path
      arguments that start with "$ORIGIN/" and various other special
      tokens are relocated relative to the caller's ELF object absolute
      path;

    - functions similar to various -lc and -ldl functions where `void
      *caller` and a `const char *prefix` arguments are added, where the
      caller can specify the address of another caller, and/or the
      configured install prefix;

    - macro replacements of various -lc and -ldl functions, which macros
      refer to global variables to get an address and/or an install
      prefix

 - a library to link with providing utility functions for relocating
   deployment paths

 - an LD_PRELOADable object for codebases that cannot be rebuilt,
   providing wrappers for various -lc and -ldl functions

Applications could either be rebuilt to link with this library, or the
LD_PRELOAD shim could be used on existing applications.

C and libdl library functions for which alternatives are provided:

 - open(2)
 - creat(2)
 - stat(2)
 - lstat(2)
 - dlopen(3)

Tokens that will be supported for path prefixes:

 - `$ORIGIN/` -> the absolute path of the object when the caller of the function
 - `$EXE_ORIGIN/` -> the absolute path of the running executable
 - `$TOP/` -> `$ORIGIN/..` or the absolute path of an install location that is found by traversing `..` from the object's path until a sentinel file is found
 - `$BINDIR/` -> `$TOP/bin/`
 - `$LIBDIR/` -> `$TOP/lib/`
 - `$LIBEXECDIR/` -> `$TOP/libexec/`
 - `$SHAREDIR/` -> `$TOP/share/`
 - `$STATEDIR/` -> `$TOP/var/`
 - `$ETCDIR/` -> `$TOP/etc/`
 - `$ROOT/` -> the absolute path of an install location that is found by traversing `..` from the object's path until a sentinel file is found


BUILD AND TEST INSTRUCTIONS
===========================

Currently only Linux is supported, but it should be easy enough to add
support for Solaris/Illumos and \*BSD.

```
$ make check
```

LIMITATIONS
===========

 - At this time reloc is a C codebase.  As such, applications that do
   not use the C library will not benefit.
 - Setting a multiplicity of unrelated deployment prefixes (e.g.,
   `./configure --prefix=/foo/bar --bindir=/bar/bin ...`) is not
   supported at this time.
 - Applications that use syscall(2) to make system calls without going
   through the corresponding C library system call stubs will not
   benefit.
 - Only some system calls are wrapped to be deploy-time
   relocation-capable.  If the C library protects its own exported
   symbols, then C library functions calling these system calls will not
   handle deploy-time relocation.
 - The user still has to chrpath(1) executables and shared objects to
   use $ORIGIN/-relative RPATHs.


USE CASE #1: Header-only library `reloc_base.h`
===============================================

In this case we have an header-only library, `reloc_base.h`.

```
/* Optional: macro for specifying known install prefix */
#define RELOC_PREFIX MY_CONFIGURED_PREFIX

/* Optional: macros used for resolving $ROOT/ and $TOP/ */
#define RELOC_ROOT_SENTINEL "some_root_sentinel_filename"
#define RELOC_TOP_SENTINEL "some_top_sentinel_filename"

/* Optional: RELOC_INCLUDE_HEADERS, else must include system headers here */
#define RELOC_INCLUDE_HEADERS

/* Optional: RELOC_WRAP_FUNCTIONS, otherwise we must use reloc_wrap_open() etc. */
#define RELOC_WRAP_FUNCTIONS

#include <reloc_base.h>

/*
 * Now open(), creat(), stat(), lstat(), and dlopen() will rewrite
 * prefix-relative paths, and also $TOKEN/-relative paths, for the tokens
 * listed above.
 */
```


USE CASE #2: Use utility functions from `-lreloc`
=================================================

In this case we link the application with `-lreloc` ahead of `-lc`.

```
#include "libreloc.h"

/*
 * Now reloc() and reloc_ex() can be used to relocate deployment paths
 * to locations relative to actual deployment.
 */
```


USE CASE #3: Use `LD_PRELOAD=reloc.so`
=============================================

In this case we have a previously built application and we use
`LD_PRELOAD` to interpose on various system calls and C library
functions.

```
$ LD_PRELOAD=reloc.so \
  RELOC_PREFIX=...           \
  RELOC_TOP_SENTINEL=...     \
  RELOC_ROOT_SENTINEL=... $application_here ...
```


ENVIRONMENT
===========

The `LD_PRELOAD` case checks the environment for `$RELOC_PREFIX`,
`$RELOC_TOP_SENTINEL`, and `$RELOC_ROOT_SENTINEL`.

The header-only library and `libreloc` do not make use of the
enviroment at all.


TODO
====

 - use macros to generate all the wrappers, prototypes, etc., then
 - add wrappers for `exec\*(2)`, `posix_spawn(3)`, and a variety of
   others, maybe even `system(3)`
 - use `getauxval(3)` and `AT_EXECFN` prefentially to `/prox/self/exe`
   in case the latter doesn't work well in the face of hardlinks
 - add support for `RELOC_BINDIR`, `RELOC_LIBDIR`, etc. macros, to
   better support autoconf codebases where users build with `./configure
   --prefix=...  --bindir=... ...`
    - this will require a struct to hold all of these optional prefixes
      and pass themto `reloc_ex()`
 - add tests involving shared objects
 - test every wrapped function
 - add docs
 - document symbol versioning issues regarding `LD_PRELOAD`
   interposition
 - optimize search for sentinels so that search for the top sentinel
   stops if the root sentinel is found, and set a maximum number of `..`
   traversals for the top sentinel search
 - add a utility script to chrpath(1) programs to use $ORIGIN/-relative
   RPATHs

