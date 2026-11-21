/*
 * Linux CPU hog (nice'd busy loop).
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void lower_nice(void)
{
    errno = 0;
    if (nice(10) == -1 && errno != 0) {
        perror("nice");
    }
}

static void burn_cpu(void)
{
    volatile unsigned long sink = 0;

    for (;;) {
        sink++;
        if (sink == 0) {
            continue;
        }
    }
}

int main(void)
{
    lower_nice();

    printf("linux hog: burning CPU (pid=%d)\n", (int)getpid());
    fflush(stdout);

    burn_cpu();
    return EXIT_SUCCESS;
}
