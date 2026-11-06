/* Bundled CFITSIO headers and linked library must both be version 4.7.0. */
#include <math.h>
#include <stdio.h>
#include "fitsio.h"

int main(void)
{
    float linked_version = 0.0f;
    float header_version = (float)CFITSIO_MAJOR
                         + (float)CFITSIO_MINOR * 0.01f
                         + (float)CFITSIO_MICRO * 0.0001f;

    /* ffvers uses major + minor/100 + micro/10000 (fitscore.c). */
    ffvers(&linked_version);
    if (fabsf(linked_version - header_version) >= 0.00005f) {
        fprintf(stderr, "FAIL: CFITSIO headers %d.%d.%d, linked library %.6f\n",
                CFITSIO_MAJOR, CFITSIO_MINOR, CFITSIO_MICRO,
                (double)linked_version);
        return 1;
    }

    if (CFITSIO_MAJOR != 4 || CFITSIO_MINOR != 7 || CFITSIO_MICRO != 0) {
        fprintf(stderr, "FAIL: expected bundled CFITSIO 4.7.0, got %d.%d.%d\n",
                CFITSIO_MAJOR, CFITSIO_MINOR, CFITSIO_MICRO);
        return 1;
    }

    printf("PASS: bundled CFITSIO 4.7.0 headers and linked library agree\n");
    return 0;
}
