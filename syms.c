
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int
main(void)
{
    printf("open: %p\n", open);
    printf("creat: %p\n", creat);
    printf("stat: %p\n", stat);
    printf("lstat: %p\n", lstat);
    printf("dlopen: %p\n", dlopen);
    return 0;
}
