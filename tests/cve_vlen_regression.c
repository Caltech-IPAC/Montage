/*
 * cve_vlen_regression.c: regression test for CVE-2026-115504.
 *
 * Unpatched 4.7.0 reads a variable-length string wider than CFITSIO's
 * 28800-byte internal buffer. The upstream fix checks the width before
 * reading. The caller's result buffers here are deliberately large enough.
 *
 * A variable-length string column gets three rows:
 *   row 1: 1000 bytes; must read back intact (the fix leaves normal strings
 *          alone);
 *   row 2: exactly 28800 bytes, the largest width the check allows; the read
 *          must succeed (status 0). Its contents are not compared;
 *   row 3: 28801 bytes; the read must fail with READ_ERROR and the fix's own
 *          "Out-of-bounds string read" message, not some other failure.
 * The corresponding upstream regression lives in tests/test_getcols.c.
 *
 * Exit status:
 *   0  patched: valid strings accepted, the oversized string rejected
 *   1  unpatched: the oversized string was not rejected by the fix
 *   2  setup failure: memory, or the test file could not be written/opened
 *   3  valid data mishandled: the short string was not read back intact,
 *      or the 28800-byte string was rejected
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"

#define TEST_FITS_FILE "cve_vlen_test.fits"
#define SHORT      1000
#define BOUNDARY   28800  /* CFITSIO's internal buffer size */
#define OVERSIZE   28801  /* one byte beyond it */
#define GUARD_MSG  "Out-of-bounds string read"

enum { PATCHED = 0, UNPATCHED = 1, SETUP_FAILURE = 2, VALID_MISHANDLED = 3 };

/* Return 1 if CFITSIO's error-message stack holds the fix's message. */
static int guard_message_seen(void)
{
    char msg[FLEN_ERRMSG];
    int  seen = 0;

    while (ffgmsg(msg))
        if (strstr(msg, GUARD_MSG))
            seen = 1;
    return seen;
}

int main(void)
{
    fitsfile *f = NULL;
    int       status = 0;
    int       close_status = 0;
    int       anynull = 0;
    int       result = SETUP_FAILURE;
    size_t    ii;

    char *ttype[] = { "VARSTR" };
    char *tform[] = { "PA" };  /* one variable-length string per row */

    unsigned char *data = malloc(OVERSIZE);
    char          *buf  = malloc(OVERSIZE + 1);
    char          *results[1];

    if (!data || !buf) {
        fprintf(stderr, "SETUP FAILURE: malloc\n");
        goto cleanup;
    }
    results[0] = buf;

    /* Write the three rows. */
    ffinit(&f, "!" TEST_FITS_FILE, &status);
    ffphps(f, BYTE_IMG, 0, NULL, &status);
    ffcrtb(f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL, &status);
    memset(data, 'C', SHORT);
    ffpclb(f, 1, 1, 1, (long)SHORT, data, &status);
    memset(data, 'B', BOUNDARY);
    ffpclb(f, 1, 2, 1, (long)BOUNDARY, data, &status);
    memset(data, 'A', OVERSIZE);
    ffpclb(f, 1, 3, 1, (long)OVERSIZE, data, &status);
    if (f)
        ffclos(f, &status);
    f = NULL;
    if (status != 0) {
        fprintf(stderr, "SETUP FAILURE: could not create the test FITS file (status=%d)\n", status);
        goto cleanup;
    }

    ffopen(&f, TEST_FITS_FILE, READONLY, &status);
    ffmahd(f, 2, NULL, &status);
    if (status != 0) {
        fprintf(stderr, "SETUP FAILURE: could not open the test FITS table (status=%d)\n", status);
        goto cleanup;
    }

    /* Row 1: a short string must read back intact. */
    memset(buf, 0, OVERSIZE + 1);
    ffgcvs(f, 1, 1, 1, 1, NULL, results, &anynull, &status);
    if (status != 0 || strlen(buf) != SHORT) {
        fprintf(stderr, "FAIL: a %d-byte string was not read back (status=%d, length=%lu)\n",
                SHORT, status, (unsigned long)strlen(buf));
        result = VALID_MISHANDLED;
        goto cleanup;
    }
    for (ii = 0; ii < SHORT; ii++) {
        if (buf[ii] != 'C') {
            fprintf(stderr, "FAIL: the %d-byte string came back altered at byte %lu\n",
                    SHORT, (unsigned long)ii);
            result = VALID_MISHANDLED;
            goto cleanup;
        }
    }
    puts("ok   a 1000-byte variable-length string reads back intact");

    /* Row 2: the largest allowed width must not be rejected. */
    ffgcvs(f, 1, 2, 1, 1, NULL, results, &anynull, &status);
    if (status != 0) {
        fprintf(stderr, "FAIL: a %d-byte string was rejected (status=%d)\n", BOUNDARY, status);
        result = VALID_MISHANDLED;
        goto cleanup;
    }
    puts("ok   a 28800-byte variable-length string is accepted");

    /* Row 3: the oversized string must be rejected by the fix. */
    ffcmsg();
    ffgcvs(f, 1, 3, 1, 1, NULL, results, &anynull, &status);
    if (status != READ_ERROR || !guard_message_seen()) {
        fprintf(stderr, "FAIL: a %d-byte string was not rejected by the CVE-2026-115504 "
                "check (status=%d, expected READ_ERROR %d with \"" GUARD_MSG "\")\n",
                OVERSIZE, status, READ_ERROR);
        result = UNPATCHED;
        goto cleanup;
    }
    puts("ok   a 28801-byte variable-length string is rejected (" GUARD_MSG ")");
    result = PATCHED;

cleanup:
    /* A close error is not reported: result already holds the outcome. */
    if (f)
        ffclos(f, &close_status);
    remove(TEST_FITS_FILE);
    free(data);
    free(buf);
    return result;
}
