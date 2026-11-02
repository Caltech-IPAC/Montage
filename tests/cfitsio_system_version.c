/*
 * cfitsio_system_version.c — used by `make check-system-cfitsio`.
 *
 * The selected CFITSIO headers and the linked library must be the same
 * version (any version; the bundled tests pin 4.7.0). Prints the version and
 * exits 0 if they agree; prints both and exits 1 if they differ.
 */
#include <stdio.h>
#include "fitsio.h"

#ifndef CFITSIO_MICRO
#define CFITSIO_MICRO 0  /* older headers (3.49 and earlier) have none */
#endif

int main(void)
{
    float linked = 0.0f;
    int   lib, hdr;

    /* ffvers returns major + minor/100 + micro/10000 (fitscore.c). */
    ffvers(&linked);
    lib = (int)(linked * 10000.0f + 0.5f);
    hdr = CFITSIO_MAJOR * 10000 + CFITSIO_MINOR * 100 + CFITSIO_MICRO;

    if (lib != hdr) {
        printf("headers %d.%d.%d, library %d.%d.%d\n",
               hdr / 10000, hdr / 100 % 100, hdr % 100,
               lib / 10000, lib / 100 % 100, lib % 100);
        return 1;
    }
    printf("%d.%d.%d\n", lib / 10000, lib / 100 % 100, lib % 100);
    return 0;
}
