#define _FILE_OFFSET_BITS 64 // just a precaution

#include "sup.h"

char *buffer = NULL;
char *vbuff = NULL;
int fdu;
int fdi;

void cleanup(void) {
    if (buffer)
        free(buffer);
    if (vbuff)
        free(vbuff);
    close(fdu);
    close(fdi);
}

int main(int argc, char **argv) {
    if (geteuid() != 0) {
        fprintf(stderr, "Error: program has not been run with sufficient priveleges.\n"
                        "Either run as root or with sudo.\n");
        return 1;
    }
    if (argc < 3) {
        fprintf(stderr, "Error: invalid number of command-line arguments.\n");
        return 1;
    }
    const char *usb;
    const char *iso;
    uint64_t bsize = MEGABYTE;
    bool verify = false;
    bool verbose = false;
    bool zmem = false;
    parse_argv(argc, argv, &usb, &iso, &bsize, &verify, &verbose, &zmem);
    if (!bsize) {
        fprintf(stderr, "Error: buffer size cannot be zero.\n");
        return 1;
    }
    if (verbose) {
        printf("Path to device file: %s\n"
               ".iso file: %s\n"
               "Buffer size: %" PRIu64 " bytes\n"
               "Verify flash: %s\n"
               "Zero-out empty bytes: %s\n"
               "Verbose: true\n---------------\n",
               usb, iso, bsize, verify ? "true" : "false", zmem ? "true" : "false");
    }
    struct stat buff = {0};
    if (stat(iso, &buff) == -1) {
        fprintf(stderr, "Error: could not obtain file information for \"%s\".\n", iso);
        return 1;
    }
    if (!S_ISREG(buff.st_mode)) {
        fprintf(stderr, "Error: file \"%s\" is not a regular file.\n", iso);
        return 1;
    }
    if (verify)
        fdu = open(usb, O_RDWR | O_TRUNC);
    else
        fdu = open(usb, O_WRONLY | O_TRUNC);
    if (fdu == -1) {
        fprintf(stderr, "Error: device file \"%s\" could not be opened.\n", usb);
        return 1;
    }
    fdi = open(iso, O_RDONLY);
    if (fdi == -1) {
        close(fdu);
        fprintf(stderr, "Error: .iso file \"%s\" could not be opened.\n", iso);
        return 1;
    }
    if (atexit(&cleanup)) {
        fprintf(stderr, "Error: could not register cleanup function.\n");
        return 1;
    }
    uint64_t padded = bsize;
    // uint64_t remainder;
    if (verify) {
        uint64_t remainder = bsize % sizeof(uint64_t);
        buffer = malloc((padded = bsize + (remainder ? sizeof(uint64_t) - remainder : 0))*sizeof(char));
        // char *ptr = buffer + bsize;
        // char i = remainder = padded - bsize;
        // while (i --> 0)
        //     *ptr++ = 0;
    } else
        buffer = malloc(bsize*sizeof(char));
    // printf("bsize: %" PRIu64 ", padded: %" PRIu64 "\n", bsize, padded);
    if (!buffer) {
        buffer = NULL;
        fprintf(stderr, "Error: could not allocate buffer.\n");
        return 1;
    }
    uint64_t rem = buff.st_size;
    ssize_t bread;
    ssize_t bwritten;
    do {
        if ((bread = read(fdi, buffer, bsize)) == -1) {
            fprintf(stderr, "Error: \"read\" error from \"%s\".\n", iso);
            return 1;
        }
        // printf("Read: %zu\n", (size_t) bread);
        rem -= bread;
        bwritten = 0;
        do {
            if ((bwritten += write(fdu, buffer + bwritten, bread - bwritten)) == -1) {
                fprintf(stderr, "Error: could not write \"%s\" to \"%s\".\n", iso, usb);
                return 1;
            }
        } while (bwritten != bread);
        if (verbose) {
            printf("%.1Lf%% complete\r", ((buff.st_size - rem)/((long double) buff.st_size))*100);
            fflush(stdout);
        }
    } while (rem);
    if (verbose)
        printf("\nDone!\n--------------");
    if (zmem) {

    }
    if (verify) {
        if (verbose)
            printf("Verifying flash now...\n");
        vbuff = malloc(padded*sizeof(char));
        if (!vbuff) {
            fprintf(stderr, "Error: could not allocate verification buffer. Verification failed.\n");
            return 1;
        }
        if (lseek(fdu, 0, SEEK_SET) == -1) {
            fprintf(stderr, "Error: could not seek to beginning of \"%s\" device file.\n", usb);
            return 1;
        }
        if (lseek(fdi, 0, SEEK_SET) == -1) {
            fprintf(stderr, "Error: could not seek to beginning of \"%s\".\n", iso);
            return 1;
        }
        /* if (close(fdu) == -1) {
            fprintf(stderr, "Error: could not close \"%s\" device file.\n", usb);
            return 1;
        } */
        /* if ((fdu = open(usb, O_RDONLY) == -1)) {
            fprintf(stderr, "Error: could not re-open \"%s\" device file for reading.\n", usb);
            return 1;
        } */
        rem = buff.st_size;
        ssize_t bread2;
        uint64_t *p1;
        uint64_t *p2;
        char *c1;
        char *c2;
        int64_t remb;
        ssize_t res;
        do {
            if ((bread = read(fdu, vbuff, bsize)) == -1) {
                fprintf(stderr, "Error: could not read from \"%s\" during verification.\n", usb);
                return 1;
            }
            bread2 = 0;
            do {
                if ((res = read(fdi, buffer + bread2, bread - bread2)) == -1) {
                    fprintf(stderr, "Error: could not read from \"%s\" during verification.\n", iso);
                    return 1;
                }
                bread2 += res;
            } while (bread2 != bread);
            remb = bread;
            p1 = (uint64_t*) vbuff;
            p2 = (uint64_t*) buffer;
            if (bread % sizeof(uint64_t)) {
                while (remb > sizeof(uint64_t)) {
                    if (*p1++ != *p2++) {
                        fprintf(stderr, "Non-identical byte/s found. Please re-attempt flash.\n");
                        return 1;
                    }
                    remb -= sizeof(uint64_t);
                    printf("stuck here.\n");
                }
                c1 = (char*) p1;
                c2 = (char*) p2;
                while (remb --> 0) {
                    if (*c1++ != *c2++) {
                        fprintf(stderr, "Non-identical byte/s found. Please re-attempt flash.\n");
                        return 1;
                    }
                    printf("stucker herer 2\n");
                }
            } else {
                while (remb) {
                    if (*p1++ != *p2++) {
                        fprintf(stderr, "Non-identical byte/s found. Please re-attempt flash.\n");
                        return 1;
                    }
                    remb -= sizeof(uint64_t);
                    printf("stuck end.\n");
                }
            }
            rem -= bread;
            if (verbose) {
                printf("%.1Lf%% complete\r", ((buff.st_size - rem)/((long double) buff.st_size))*100);
                fflush(stdout);
            }
            printf("rem: %zu\n", (size_t) rem);
        } while (rem);
        printf("Verification completed. Flash was successful!\n");
    }
    return 0;
}
