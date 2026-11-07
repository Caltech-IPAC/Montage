/* The bundled CFITSIO 4.7.0 picks the Unix compress decoder when ".Z" appears
   anywhere in a path (mem_uncompress2mem in drvrmem.c), so it cannot open a
   gzip file in a directory such as data.Zdir. The same file in a directory
   without ".Z" opens. This pins that behaviour. When a CFITSIO upgrade fixes
   it, the second open succeeds, this test fails, and it should be deleted.
   Run in an empty directory whose path has no ".Z" and no ".bz2". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fitsio.h"

static int open_gzip(const char *cwd, const char *dir)
{
    char path[4096];
    fitsfile *fptr = NULL;
    int status = 0, close_status = 0;

    snprintf(path, sizeof path, "%s/%s/img.fits.gz", cwd, dir);
    if (fits_open_file(&fptr, path, READONLY, &status)) {
        fits_clear_errmsg();
        return 0;
    }
    fits_close_file(fptr, &close_status);
    return 1;
}

int main(void)
{
    char cwd[4096];
    fitsfile *fptr = NULL;
    long naxes[2] = { 8, 8 };
    float pixels[64];
    int status = 0, i;

    if (getcwd(cwd, sizeof cwd) == NULL) {
        fprintf(stderr, "FAIL: cannot get the working directory\n");
        return 1;
    }
    if (strstr(cwd, ".Z") || strstr(cwd, ".bz2")) {
        fprintf(stderr, "FAIL: the working directory %s contains .Z or .bz2\n", cwd);
        return 1;
    }

    for (i = 0; i < 64; i++)
        pixels[i] = (float)i;
    if (fits_create_file(&fptr, "raw.fits", &status)
            || fits_create_img(fptr, FLOAT_IMG, 2, naxes, &status)
            || fits_write_img(fptr, TFLOAT, 1, 64, pixels, &status)
            || fits_close_file(fptr, &status)) {
        fits_report_error(stderr, status);
        return 1;
    }

    if (mkdir("dataZdir", 0777) || mkdir("data.Zdir", 0777)
            || system("gzip -c raw.fits > dataZdir/img.fits.gz")
            || system("gzip -c raw.fits > data.Zdir/img.fits.gz")) {
        fprintf(stderr, "FAIL: cannot set up the test directories\n");
        return 1;
    }

    if (!open_gzip(cwd, "dataZdir")) {
        fprintf(stderr, "FAIL: CFITSIO cannot open a gzip file in a path without .Z\n");
        return 1;
    }
    if (open_gzip(cwd, "data.Zdir")) {
        fprintf(stderr, "FAIL: CFITSIO now opens a gzip file in a path with .Z, "
                        "so delete tests/cfitsio_dotz_path.c\n");
        return 1;
    }

    printf("PASS: bundled CFITSIO still cannot open a gzip file in a path with .Z\n");
    return 0;
}
