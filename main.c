#include "sup.h"

int main(int argc, char **argv) {
    /*if (geteuid() != 0) {
        fprintf(stderr, "Error: program has not been run with sufficient priveleges.\n"
                        "Either run as root or with sudo.\n");
        return 1;
    } */
    if (argc < 3) {
        fprintf(stderr, "Error: invalid number of command-line arguments.\n");
        return 1;
    }
    const char *usb;
    const char *iso;
    uint64_t bsize = GIGABYTE;
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
               "Verbose: true\n",
               usb, iso, bsize, verify ? "true" : "false", zmem ? "true" : "false");
    }
    struct stat buff = {0};
    int fdu = open(usb, O_WRONLY);
    if (fdu == -1) {
        fprintf(stderr, "Error: device file \"%s\" could not be opened.\n", usb);
        return 1;
    }
    int fdi = open(iso, O_WRONLY);
    if (fdi == -1) {
        close(fdu);
        fprintf(stderr, "Error: .iso file \"%s\" could not be opened.\n", usb);
        return 1;
    }
    char *buffer = malloc(bsize*sizeof(char));
    if (!buffer) {
        fprintf(stderr, "Error: could not allocate buffer.\n");
        return 1;
    }

    free(buffer);
    close(fdu);
    close(fdi);
    return 0;
}
