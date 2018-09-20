
/*
 * RELOC_FUNC(type, err_ret, libc_func_name, prototype, args, ...args_decl)
 */

RELOC_FUNC(int, -1, creat, (const char *, mode_t),
           (path, mode), const char *path, mode_t mode)

RELOC_FUNC(int, -1, stat, (const char *, struct stat *),
           (path, st), const char *path, struct stat *st)

RELOC_FUNC(int, -1, lstat, (const char *, struct stat *),
           (path, st), const char *path, struct stat *st)

RELOC_FUNC(void *, NULL, dlopen, (const char *, int),
           (path, flags), const char *path, int flags)
