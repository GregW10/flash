#ifndef GREGETCH_H
#define GREGETCH_H

#if !defined(__linux__) && !defined(__APPLE__)
#error "Cannot compile on current OS.\n"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <errno.h>

#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1099511627776

typedef enum boolean {
    false, true
} bool;

uint64_t to_uint(const char *str, const char **endptr) { // if endptr, leaves it pointing to first non-converted char
    if (!str || !*str || *str < 48 || *str > 57) {
        if (endptr)
            *endptr = str;
        return -1;
    }
    uint64_t res = 0;
    goto start;
    while (*str) {
        if (*str < 48 || *str > 57) {
            if (endptr)
                *endptr = str;
            return res;
        }
        res *= 10;
        start:
        res += *str++ - 48;
    }
    if (endptr)
        *endptr = str;
    return res;
}

void parse_argv(int argc,
                char **argv,
                const char **usb,
                const char **iso,
                uint64_t *bsize,
                bool *verify,
                bool *verbose,
                bool *zmem) {
    --argc; // skip executable name
    ++argv;
    *usb = NULL;
    *iso = NULL;
    const char *ptr;
    while (argc --> 0) {
        if (**argv != '-') {
            if (!*usb)
                *usb = *argv;
            else if (!*iso)
                *iso = *argv;
            else {
                fprintf(stderr, "Error: unrecognised argument \"%s\".\n", *argv);
                exit(1);
            }
            ++argv;
            continue;
        }
        ptr = *argv + 1;
        if (!*ptr) {
            fprintf(stderr, "Error: invalid argument \"%s\".\n", ptr); // just a hyphen
            exit(1);
        }
        do {
            if (*ptr == 'v')
                *verbose = true;
            else if (*ptr == 'c')
                *verify = true;
            else if (*ptr == 'z')
                *zmem = true;
            else if (*ptr == 'b') {
                if (*(ptr + 1) || !argc) {
                    fprintf(stderr, "Error: -b flag must be followed by buffer size.\n");
                    exit(1);
                }
                *bsize = to_uint(*++argv, &ptr);
                if (ptr == *argv) {
                    fprintf(stderr, "Error: invalid buffer size \"%s\".\n", *argv);
                    exit(1);
                }
                if (*ptr) {
                    if (*ptr == 'K')
                        *bsize *= KILOBYTE;
                    else if (*ptr == 'M')
                        *bsize *= MEGABYTE;
                    else if (*ptr == 'G')
                        *bsize *= GIGABYTE;
                    else {
                        fprintf(stderr, "Error: unsupported buffer size suffix \"%c\".\n", *ptr);
                        exit(1);
                    }
                }
                --argc;
                break;
            }
        } while (*++ptr);
        ++argv;
    }
}

void zero_mem(void *mem, uint64_t count) {
    uint64_t rem;
    char *p = mem;
    if ((rem = count % sizeof(uint64_t))) {
        rem = sizeof(uint64_t) - rem;
        count -= rem;
        while (rem --> 0)
            *p++ = 0;
    }
    uint64_t *ptr = (uint64_t*) p;
    count /= 8;
    while (count --> 0)
        *ptr++ = 0;
}
#endif
