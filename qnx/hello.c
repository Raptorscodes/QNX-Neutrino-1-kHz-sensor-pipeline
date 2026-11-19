/*
 * Stage 1 — toolchain check only.
 */

#include <stdio.h>
#include <sys/utsname.h>

int main(void)
{
    struct utsname info;

    printf("hello from QNX pipeline stage 1\n");

    if (uname(&info) != 0) {
        perror("uname");
        return 1;
    }

    printf("sysname=%s release=%s machine=%s\n",
           info.sysname, info.release, info.machine);
    return 0;
}
