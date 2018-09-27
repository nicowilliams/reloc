
CFLAGS=-g -ggdb3 -O0 -Wall -Werror

all:	libreloc.so reloc.so syms                          	\
	tests/root/test01/bin/test_includelib              	\
	tests/root/test01/bin/test_lib				\
	tests/root/test01/bin/test_preload

check: all
	@tests/root/test01/bin/test_includelib
	@tests/root/test01/bin/test_lib
	@LD_PRELOAD=$$PWD/reloc.so				\
	    RELOC_PREFIX=$$PWD/tests/originalroot/test01	\
	    RELOC_TOP_SENTINEL=config/top.sentinel		\
	    RELOC_ROOT_SENTINEL=root.sentinel			\
	    tests/root/test01/bin/test_preload

# `syms' is a utility whose only purpose is to have nm(1) applied to it
# so as to discover C library symbol versions on Linux so that we can
# make the preloadable object advertise the same versions, otherwise
# interposition does not happen.
syms: syms.c Makefile
	$(CC) $(CFLAGS) -o $@ $< -lc -ldl

symvers: syms Makefile
	nm $< | egrep ' U .*(open|creat|stat|lstat|dlopen)' | sed -e 's/^.* U //' > $@

# `symvers.h' is a header used by preloadreloc.c so as to use the
# correct versioned symbols.
#
# Note: glibc has __xstat() and __lxstat() functions and symbols, and
#       stat() and lstat() are macros that call them, respectively.
#
#       In that case we still export stat() and lstat(), but versioned
#       at RELOC_1.0.
symvers.h: symvers Makefile
	( \
	echo "#ifndef RELOC_SYMVERS_H";                                          \
	echo "#define RELOC_SYMVERS_H";                                          \
	echo "#define sym_open \"`grep open $<|grep -v dlopen`\"";               \
	echo "#define sym_creat \"`grep creat $<`\"";                            \
	grep -w stat $< >/dev/null &&                                            \
	    echo "#define sym_stat  \"`grep stat $<`\"";                         \
	grep -w stat $< >/dev/null ||                                            \
	    echo "#define sym_stat  \"stat@@RELOC_1.0\"";                        \
	grep -w __xstat $< >/dev/null &&                                         \
	    echo "#define sym___xstat  \"`grep __xstat $<`\"";                   \
	grep -w lstat $< >/dev/null &&                                           \
	    echo "#define sym_lstat  \"`grep lstat $<`\"";                       \
	grep -w lstat $< >/dev/null ||                                           \
	    echo "#define sym_lstat  \"lstat@@RELOC_1.0\"";                      \
	grep -w __lxstat $< >/dev/null &&                                        \
	    echo "#define sym___lxstat  \"`grep __lxstat $<`\"";                 \
	echo "#define sym_dlopen \"`grep dlopen $<`\"";                          \
	echo "#endif";                                                           \
	) > $@

reloc-mapfile: symvers Makefile
	sed -e 's/^.*[@]//' $< | sort -u | while read vers; do          \
	    echo "RELOC_1.0 {";                                         \
	    echo "global:";                                             \
	    grep -w stat $< >/dev/null || echo "stat;";                 \
	    grep -w lstat $< >/dev/null || echo "lstat;";               \
	    echo "};";                                                  \
	    echo "$$vers {";                                            \
	    echo "global:";                                             \
	    grep "[@]$$vers" $<|cut -d'@' -f1 | while read sym; do      \
		echo "$$sym;";                                          \
	    done;                                                       \
	    grep -w stat $< >/dev/null || echo "stat;";                 \
	    grep -w lstat $< >/dev/null || echo "lstat;";               \
	    echo "local: *;";                                           \
	    echo "} RELOC_1.0;";                                        \
	done > $@

libreloc.so: libreloc.c reloc_base.h Makefile
	$(CC) $(CFLAGS) -I. -fPIC -shared -o $@ $< -lc -ldl -lpthread

reloc.so: preloadreloc.c symvers.h reloc_base.h reloc-mapfile Makefile
	$(CC) $(CFLAGS) -I. -fPIC -shared -o $@ $< -Wl,--version-script=reloc-mapfile -Wl,-z,interpose -lc -ldl -lpthread

tests/root/test01/bin/test_includelib: tests/test_reloc_includelib.c reloc_base.h Makefile
	$(CC) $(CFLAGS) -I. -DMY_CONFIGURED_PREFIX=\"$$PWD/tests/originalroot/test01\" -o $@ $< -lc -ldl

tests/root/test01/bin/test_lib: tests/test_reloc_lib.c libreloc.so reloc_base.h Makefile
	$(CC) $(CFLAGS) -I. -DMY_CONFIGURED_PREFIX=\"$$PWD/tests/originalroot/test01\" -o $@ $< -L$$PWD -Wl,-rpath,$$PWD -lreloc -lc -ldl

tests/root/test01/bin/test_preload: tests/test_reloc_preload.c reloc.so reloc_base.h Makefile
	$(CC) $(CFLAGS) -I. -DMY_CONFIGURED_PREFIX=\"$$PWD/tests/originalroot/test01\" -o $@ $< -lc -ldl

clean:
	rm -f *.o *.so
	rm -f tests/root/test01/bin/test_includelib
	rm -f tests/root/test01/bin/test_lib
	rm -f tests/root/test01/bin/test_preload
	rm -f syms symvers symvers.h reloc-mapfile
