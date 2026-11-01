/*
 * Tests for buffers.c - I/O buffering functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_buffers.fits"
#define MINDIRECT_SIZE 8640

static void
test_ffgrsz_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100, 100 };
	long ndata;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_02(ffgrsz, f, &ndata);
	fail_if(ndata <= 0);
	call_01(ffclos, f);
}

static void
test_ffgrsz_binary_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL", "BCOL" };
	char *tform[] = { "1E", "1D" };
	long ndata;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 100, 2, ttype, tform, NULL, NULL);
	call_02(ffgrsz, f, &ndata);
	fail_if(ndata <= 0);
	call_01(ffclos, f);
}

static void
test_ffflus(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	float data[] = {
		1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
		6.0f, 7.0f, 8.0f, 9.0f, 10.0f
	};

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 10, data);
	call_01(ffflus, f);
	call_01(ffclos, f);
}

static void *
xmalloc(size_t s)
{
	void *rv = malloc(s);
	fail_if(rv == NULL && "Out of memory");
	return rv;
}

static void
test_large_write(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10000 };
	float *data;
	int i;

	data = xmalloc(10000 * sizeof *data);
	for (i = 0; i < 10000; i += 1) {
		data[i] = (float)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 10000, data);
	call_01(ffclos, f);

	free(data);
}

static void
test_cross_record_write(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1000E" };
	float *data;
	float *result;
	int i, anynull;

	data = xmalloc(1000 * sizeof *data);
	result = xmalloc(1000 * sizeof *result);

	for (i = 0; i < 1000; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);

	for (i = 0; i < 10; i += 1) {
		call_06(ffpcle, f, 1, i + 1, 1, 1000, data);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 5, 1, 1000, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[999] != 1000.0f);
	call_01(ffclos, f);

	free(data);
	free(result);
}

static void
test_multiple_hdu_access(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1E" };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 5, data);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 1, NULL);
	call_07(ffgpv, f, TFLOAT, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	call_01(ffclos, f);
}

static void
test_read_write_interleaved(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100 };
	float data[100];
	float result[100];
	int i, anynull;

	for (i = 0; i < 100; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 50, data);
	call_01(ffflus, f);
	call_05(ffppre, f, 1, 51, 50, &data[50]);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 100, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[99] != 100.0f);
	call_01(ffclos, f);
}

static void
test_ascii_table_buffering(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "A", "B", "C" };
	char *tform[] = { "F10.2", "F10.2", "F10.2" };
	float data[] = { 1.0f, 2.0f, 3.0f };
	float readval;
	float nullval = 0.0f;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 100, 3, ttype, tform, NULL, NULL);

	for (int i = 0; i < 100; i += 1) {
		call_06(ffpcle, f, 1, i + 1, 1, 1, &data[0]);
		call_06(ffpcle, f, 2, i + 1, 1, 1, &data[1]);
		call_06(ffpcle, f, 3, i + 1, 1, 1, &data[2]);
	}
	call_01(ffclos, f);

	/* Verify data was written correctly */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Verify first row */
	call_08(ffgcve, f, 1, 1, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 0.99f || readval > 1.01f);
	call_08(ffgcve, f, 2, 1, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 1.99f || readval > 2.01f);
	call_08(ffgcve, f, 3, 1, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 2.99f || readval > 3.01f);

	/* Verify middle row (50) */
	call_08(ffgcve, f, 1, 50, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 0.99f || readval > 1.01f);
	call_08(ffgcve, f, 2, 50, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 1.99f || readval > 2.01f);

	/* Verify last row (100) */
	call_08(ffgcve, f, 1, 100, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 0.99f || readval > 1.01f);
	call_08(ffgcve, f, 3, 100, 1, 1, nullval, &readval, &anynull);
	fail_if(readval < 2.99f || readval > 3.01f);

	call_01(ffclos, f);
}

/*
 * Test large direct I/O writes (>= MINDIRECT bytes)
 * to exercise the ffpbyt direct write path.
 */
static void
test_large_direct_write(void)
{
	fitsfile *f;
	int status = 0;
	float *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 100;
	long naxes[] = { 0 };
	float *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[nvals - 1] != (float)nvals);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test large direct I/O reads (>= MINDIRECT bytes)
 * to exercise the ffgbyt direct read path.
 */
static void
test_large_direct_read(void)
{
	fitsfile *f;
	int status = 0;
	double *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 100;
	long naxes[] = { 0 };
	double *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TDOUBLE, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[nvals - 1] != (double)nvals);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test ffgtbb and ffptbb; raw table byte read/write.
 */
static void
test_table_byte_io(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "100B" };
	unsigned char data[100];
	unsigned char result[100];
	int i;

	for (i = 0; i < 100; i += 1) {
		data[i] = (unsigned char)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);

	/* Write raw bytes using ffptbb */
	call_05(ffptbb, f, 1, 1, 100, data);
	call_05(ffptbb, f, 5, 1, 100, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Read raw bytes using ffgtbb */
	call_05(ffgtbb, f, 1, 1, 100, result);
	for (i = 0; i < 100; i += 1) {
		fail_if(result[i] != data[i]);
	}

	call_05(ffgtbb, f, 5, 1, 100, result);
	for (i = 0; i < 100; i += 1) {
		fail_if(result[i] != data[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test ffflsh; flush all buffers.
 */
static void
test_ffflsh(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100 };
	float data[100];
	int i;

	for (i = 0; i < 100; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 100, data);

	/* Flush buffers without clearing */
	call_02(ffflsh, f, 0);

	/* Flush buffers with clearing */
	call_02(ffflsh, f, 1);

	call_01(ffclos, f);
}

/*
 * Test buffer management with multiple operations.
 */
static void
test_buffer_management(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100 };
	float data[100];
	float result[100];
	int i, anynull;

	for (i = 0; i < 100; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 100, data);

	/* Multiple flush operations */
	call_02(ffflsh, f, 0);
	call_02(ffflsh, f, 1);

	call_01(ffclos, f);

	/* Verify data survived */
	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 100, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[99] != 100.0f);
	call_01(ffclos, f);
}

/*
 * Test writing to existing file with multiple HDUs
 * to exercise ffbfwt when writing beyond EOF with fill records.
 */
static void
test_write_beyond_eof(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 1000 };
	float *data;
	float *result;
	int i, anynull;
	char *ttype[] = { "COL" };
	char *tform[] = { "1E" };

	data = xmalloc(1000 * sizeof *data);
	result = xmalloc(1000 * sizeof *result);

	for (i = 0; i < 1000; i += 1) {
		data[i] = (float)(i + 1);
	}

	/* Create file with primary HDU */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 1000, data);

	/* Add a table extension */
	call_08(ffcrtb, f, BINARY_TBL, 100, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 100, data);
	call_01(ffclos, f);

	/* Verify data */
	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 1000, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[999] != 1000.0f);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test different integer types for I/O (short, int, long) to
 * exercise ffgi2b, ffgi4b, ffgi8b, ffpi2b, ffpi4b, ffpi8b.
 */
static void
test_integer_types_io(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100 };
	short sdata[100], sresult[100];
	long jdata[100], jresult[100];
	LONGLONG lldata[100], llresult[100];
	int i, anynull;

	for (i = 0; i < 100; i += 1) {
		sdata[i] = (short)(i + 1);
		jdata[i] = i + 1;
		lldata[i] = i + 1;
	}

	/* Test SHORT_IMG */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 100, sdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 100, NULL, sresult, &anynull);
	fail_if(sresult[0] != 1);
	fail_if(sresult[99] != 100);
	call_01(ffclos, f);

	/* Test LONG_IMG (32-bit) */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 100, jdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONG, 1, 100, NULL, jresult, &anynull);
	fail_if(jresult[0] != 1);
	fail_if(jresult[99] != 100);
	call_01(ffclos, f);

	/* Test LONGLONG_IMG (64-bit) */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 100, lldata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONGLONG, 1, 100, NULL, llresult, &anynull);
	fail_if(llresult[0] != 1);
	fail_if(llresult[99] != 100);
	call_01(ffclos, f);
}

/*
 * Test non-contiguous I/O (with increment/stride) to
 * exercise ffpbytoff and ffgbytoff paths.
 */
static void
test_strided_io(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "A", "B", "C" };
	char *tform[] = { "1E", "1D", "1J" };
	float fdata[10];
	double ddata[10];
	int jdata[10];
	float fresult[10];
	double dresult[10];
	int jresult[10];
	int i, anynull;

	for (i = 0; i < 10; i += 1) {
		fdata[i] = (float)(i + 1);
		ddata[i] = (double)(i + 1);
		jdata[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 3, ttype, tform, NULL, NULL);

	/* Write columns (strided I/O internally) */
	for (i = 0; i < 10; i += 1) {
		call_06(ffpcle, f, 1, i + 1, 1, 1, &fdata[i]);
		call_06(ffpcld, f, 2, i + 1, 1, 1, &ddata[i]);
		call_06(ffpclk, f, 3, i + 1, 1, 1, &jdata[i]);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Read back columns */
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 10, NULL, fresult, &anynull);
	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 10, NULL, dresult, &anynull);
	call_09(ffgcv, f, TINT, 3, 1, 1, 10, NULL, jresult, &anynull);

	for (i = 0; i < 10; i += 1) {
		fail_if(fresult[i] != fdata[i]);
		fail_if(dresult[i] != ddata[i]);
		fail_if(jresult[i] != jdata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test byte array I/O to
 * exercise ffgi1b and ffpi1b.
 */
static void
test_byte_io(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 200 };
	unsigned char data[200], result[200];
	int i, anynull;

	for (i = 0; i < 200; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 200, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TBYTE, 1, 200, NULL, result, &anynull);
	for (i = 0; i < 200; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

/*
 * Test spanning multiple records with large data to
 * exercise record boundary handling in buffering.
 */
static void
test_multi_record_spanning(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BIGCOL" };
	char *tform[] = { "3000E" };  /* 12000 bytes per row, spans ~4 records */
	float *data;
	float *result;
	int i, anynull;

	data = xmalloc(3000 * sizeof *data);
	result = xmalloc(3000 * sizeof *result);

	for (i = 0; i < 3000; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);

	/* Write multiple rows that span records */
	for (i = 0; i < 5; i += 1) {
		call_06(ffpcle, f, 1, i + 1, 1, 3000, data);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Read back and verify */
	for (i = 0; i < 5; i += 1) {
		call_09(ffgcv, f, TFLOAT, 1, i + 1, 1, 3000, NULL, result, &anynull);
		fail_if(result[0] != 1.0f);
		fail_if(result[2999] != 3000.0f);
	}

	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test with very large image to trigger direct I/O path
 */
static void
test_very_large_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5000 };
	double *data;
	double *result;
	int i, anynull;

	data = xmalloc(5000 * sizeof *data);
	result = xmalloc(5000 * sizeof *result);

	for (i = 0; i < 5000; i += 1) {
		data[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 5000, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TDOUBLE, 1, 5000, NULL, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[4999] != 5000.0);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test ffflus on binary table HDU
 */
static void
test_ffflus_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1E" };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, data);
	call_01(ffflus, f);
	call_01(ffclos, f);
}

/*
 * Test writing to ASCII table with large data
 */
static void
test_ascii_table_large(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "E15.7" };
	float data;
	float result;
	int i, anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 200, 1, ttype, tform, NULL, NULL);

	/* Write many rows to span multiple records */
	for (i = 0; i < 200; i += 1) {
		data = (float)(i + 1);
		call_06(ffpcle, f, 1, i + 1, 1, 1, &data);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Verify some values */
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 1, NULL, &result, &anynull);
	fail_if(result != 1.0f);
	call_09(ffgcv, f, TFLOAT, 1, 200, 1, 1, NULL, &result, &anynull);
	fail_if(result != 200.0f);

	call_01(ffclos, f);
}

/*
 * Test writing beyond current rows (auto-expand table)
 */
static void
test_auto_expand_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	float data = 42.0f;
	float result;
	int anynull;
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);

	/* Write to row 100 (should auto-expand) */
	call_06(ffpcle, f, 1, 100, 1, 1, &data);

	/* Check number of rows */
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 100);

	call_01(ffclos, f);

	/* Verify */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 100, 1, 1, NULL, &result, &anynull);
	fail_if(result != 42.0f);
	call_01(ffclos, f);
}

/*
 * Test non-contiguous column access (triggers ffpbytoff/ffgbytoff).
 * When reading/writing multiple rows of single-element columns, the
 * data is non-contiguous because each value is separated by the row length.
 */
static void
test_noncontiguous_column_io(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "A", "B", "C", "D" };

	/* 4 + 4 + 8 + 2 = 18 bytes/row */
	char *tform[] = { "1E", "1J", "1D", "1I" };

	float fdata[50], fresult[50];
	int jdata[50], jresult[50];
	double ddata[50], dresult[50];
	short idata[50], iresult[50];
	int i, anynull;

	for (i = 0; i < 50; i += 1) {
		fdata[i] = (float)(i + 1);
		jdata[i] = i + 1;
		ddata[i] = (double)(i + 1);
		idata[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 50, 4, ttype, tform, NULL, NULL);

	/* Write all 50 rows in one call to trigger non-contiguous write. */
	call_06(ffpcle, f, 1, 1, 1, 50, fdata);  /* floats */
	call_06(ffpclk, f, 2, 1, 1, 50, jdata);  /* ints */
	call_06(ffpcld, f, 3, 1, 1, 50, ddata);  /* doubles */
	call_06(ffpcli, f, 4, 1, 1, 50, idata);  /* shorts */
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Read all 50 rows in one call to trigger non-contiguous read */
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 50, NULL, fresult, &anynull);
	call_09(ffgcv, f, TINT, 2, 1, 1, 50, NULL, jresult, &anynull);
	call_09(ffgcv, f, TDOUBLE, 3, 1, 1, 50, NULL, dresult, &anynull);
	call_09(ffgcv, f, TSHORT, 4, 1, 1, 50, NULL, iresult, &anynull);

	for (i = 0; i < 50; i += 1) {
		fail_if(fresult[i] != fdata[i]);
		fail_if(jresult[i] != jdata[i]);
		fail_if(dresult[i] != ddata[i]);
		fail_if(iresult[i] != idata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test non-contiguous I/O that spans multiple FITS records (2880 bytes)
 * to exercise the record boundary handling in ffpbytoff/ffgbytoff.
 */
static void
test_noncontiguous_spanning_records(void)
{
	fitsfile *f;
	int status = 0;
	/* 500 bytes per row, 200 rows = 100000 bytes (~35 records) */
	char *ttype[] = { "PAD", "DATA" };
	char *tform[] = { "492B", "1D" };  /* 492 + 8 = 500 bytes/row */
	double ddata[200], dresult[200];
	int i, anynull;

	for (i = 0; i < 200; i += 1) {
		ddata[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 200, 2, ttype, tform, NULL, NULL);

	/* Write 200 values across many records with large row spacing. */
	call_06(ffpcld, f, 2, 1, 1, 200, ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Read back all 200 values. */
	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 200, NULL, dresult, &anynull);

	for (i = 0; i < 200; i += 1) {
		fail_if(dresult[i] != ddata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test byte column non-contiguous access to trigger
 * ffgi1b/ffpi1b offset paths.
 */
static void
test_byte_column_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "PAD", "BYTES" };
	char *tform[] = { "10B", "1B" };  /* 10 + 1 = 11 bytes/row */
	unsigned char bdata[100], bresult[100];
	int i, anynull;

	for (i = 0; i < 100; i += 1) {
		bdata[i] = (unsigned char)(i % 256);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 100, 2, ttype, tform, NULL, NULL);

	/* Write 100 single-byte values with 11-byte row spacing. */
	call_06(ffpclb, f, 2, 1, 1, 100, bdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Read back all 100 values. */
	call_09(ffgcv, f, TBYTE, 2, 1, 1, 100, NULL, bresult, &anynull);

	for (i = 0; i < 100; i += 1) {
		fail_if(bresult[i] != bdata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test element spanning record boundary in non-contiguous I/O.
 * Row length ~2870 bytes means 8-byte elements near the end span boundaries.
 */
static void
test_element_spanning_record(void)
{
	fitsfile *f;
	int status = 0;

	/* Row ~2870 bytes: elements at offset 2870 will span 2880 boundary */
	char *ttype[] = { "PAD", "DATA" };
	char *tform[] = { "2862B", "1D" };  /* 2862 + 8 = 2870 bytes/row */
	double ddata[50], dresult[50];
	int i, anynull;

	for (i = 0; i < 50; i += 1) {
		ddata[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 50, 2, ttype, tform, NULL, NULL);

	/*
	** Each double is at offset 2862 in a 2870-byte row.
	** Row 1: offset 2862-2869 (just before 2880)
	** Row 2: offset 5732-5739 (spans 5760)
	** etc. - should hit spanning case regularly
	*/
	call_06(ffpcld, f, 2, 1, 1, 50, ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 50, NULL, dresult, &anynull);

	for (i = 0; i < 50; i += 1) {
		fail_if(dresult[i] != ddata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test large direct writes to ASCII table (blank fill case).
 */
static void
test_ascii_large_direct_write(void)
{
	fitsfile *f;
	int status = 0;
	/* ASCII table with 100 bytes per row, 500 rows = 50000 bytes */
	char *ttype[] = { "VALUE" };
	char *tform[] = { "E20.10" };
	float data;
	float result;
	int i, anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 500, 1, ttype, tform, NULL, NULL);

	/* Write 500 rows to trigger large buffered writes. */
	for (i = 0; i < 500; i += 1) {
		data = (float)(i + 1);
		call_06(ffpcle, f, 1, i + 1, 1, 1, &data);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 1, NULL, &result, &anynull);
	fail_if(result != 1.0f);
	call_09(ffgcv, f, TFLOAT, 1, 500, 1, 1, NULL, &result, &anynull);
	fail_if(result != 500.0f);

	call_01(ffclos, f);
}

/*
 * Test large (>MINDIRECT) array write that triggers direct disk I/O path.
 */
static void
test_massive_direct_io(void)
{
	fitsfile *f;
	int status = 0;
	long nvals = 20000;  /* 20000 floats = 80000 bytes >> MINDIRECT */
	long naxes[] = { 20000 };
	float *data;
	float *result;
	int i, anynull;

	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[nvals - 1] != (float)nvals);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test where individual elements straddle the 2880-byte record boundary
 * in non-contiguous access. This requires row_offset + element_offset
 * to place the element right at the boundary.
 *
 * FITS record is 2880 bytes.
 * Design: element at offset 2876 in row, element size 8 bytes
 * This means bytes 2876-2879 in record 0, bytes 0-3 in record 1
 *
 * Row size = 2876 + 8 = 2884 bytes
 * Row 1: element at offset 2876 (spans 2880)
 * Row 2: element at offset 2876 + 2884 = 5760 (exactly at boundary!)
 */
static void
test_element_at_boundary(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "PAD", "VAL" };
	char *tform[] = { "2876B", "1D" };  /* 2876 + 8 = 2884 bytes/row */
	double ddata[30], dresult[30];
	int i, anynull;

	for (i = 0; i < 30; i += 1) {
		ddata[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 30, 2, ttype, tform, NULL, NULL);

	/* Write 30 doubles, each at position 2876 in a 2884-byte row. */
	call_06(ffpcld, f, 2, 1, 1, 30, ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 30, NULL, dresult, &anynull);

	for (i = 0; i < 30; i += 1) {
		fail_if(dresult[i] != ddata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test with 4-byte elements (floats) straddling boundaries.
 */
static void
test_float_at_boundary(void)
{
	fitsfile *f;
	int status = 0;
	/*
	** Element at offset 2878 (size 4) spans bytes 2878-2879, 0-1
	** Row size = 2878 + 4 = 2882 bytes
	*/
	char *ttype[] = { "PAD", "VAL" };
	char *tform[] = { "2878B", "1E" };  /* 2878 + 4 = 2882 bytes/row */
	float fdata[40], fresult[40];
	int i, anynull;

	for (i = 0; i < 40; i += 1) {
		fdata[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 40, 2, ttype, tform, NULL, NULL);

	call_06(ffpcle, f, 2, 1, 1, 40, fdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TFLOAT, 2, 1, 1, 40, NULL, fresult, &anynull);

	for (i = 0; i < 40; i += 1) {
		fail_if(fresult[i] != fdata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test with 2-byte elements (shorts) straddling boundaries.
 */
static void
test_short_at_boundary(void)
{
	fitsfile *f;
	int status = 0;
	/*
	** Element at offset 2879 (size 2) spans bytes 2879, 0
	** Row size = 2879 + 2 = 2881 bytes
	*/
	char *ttype[] = { "PAD", "VAL" };
	char *tform[] = { "2879B", "1I" };  /* 2879 + 2 = 2881 bytes/row */
	short sdata[50], sresult[50];
	int i, anynull;

	for (i = 0; i < 50; i += 1) {
		sdata[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 50, 2, ttype, tform, NULL, NULL);

	call_06(ffpcli, f, 2, 1, 1, 50, sdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TSHORT, 2, 1, 1, 50, NULL, sresult, &anynull);

	for (i = 0; i < 50; i += 1) {
		fail_if(sresult[i] != sdata[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test single-element spanning (last element case in ffpbytoff/ffgbytoff)
 * When ngroups=1, only the "last group" code is executed.
 */
static void
test_single_element_spanning(void)
{
	fitsfile *f;
	int status = 0;
	/*
	** Position element so it spans the 2880 boundary
	** Offset 2874, size 8 bytes: spans 2874-2879 and 0-1.
	**/
	char *ttype[] = { "PAD", "VAL" };
	char *tform[] = { "2874B", "1D" };  /* 2874 + 8 = 2882 bytes/row */
	double ddata, dresult;
	int anynull;

	ddata = 42.5;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 2, ttype, tform, NULL, NULL);

	/* Write exactly 1 double that spans the record boundary */
	call_06(ffpcld, f, 2, 1, 1, 1, &ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 1, NULL, &dresult, &anynull);
	fail_if(dresult != ddata);

	call_01(ffclos, f);
}

/*
 * Test reading from existing file after small writes (not at EOF case)
 */
static void
test_read_existing_after_partial_write(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 1000 };
	float *data;
	float *result;
	int i, anynull;

	data = xmalloc(1000 * sizeof *data);
	result = xmalloc(1000 * sizeof *result);

	for (i = 0; i < 1000; i += 1) {
		data[i] = (float)(i + 1);
	}

	/* Create file */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 1000, data);
	call_01(ffclos, f);

	/* Open for read/write and modify part of it. */
	call_03(ffopen, &f, test_path, READWRITE);

	/* Write a small change in the middle. */
	data[500] = 999.0f;
	call_05(ffppre, f, 1, 501, 1, &data[500]);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 1000, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[500] != 999.0f);
	fail_if(result[999] != 1000.0f);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test reading/writing with various column types in same table
 */
static void
test_mixed_column_types(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTE", "SHORT", "INT", "LONG", "FLOAT", "DOUBLE" };
	char *tform[] = { "1B", "1I", "1J", "1K", "1E", "1D" };
	unsigned char bdata = 255;
	short sdata = 32000;
	int idata = 2000000;
	long ldata = 4000000000L;
	float fdata = 3.14159f;
	double ddata = 2.71828182845;

	unsigned char bresult;
	short sresult;
	int iresult;
	long lresult;
	float fresult;
	double dresult;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 6, ttype, tform, NULL, NULL);

	call_06(ffpclb, f, 1, 1, 1, 1, &bdata);
	call_06(ffpcli, f, 2, 1, 1, 1, &sdata);
	call_06(ffpclk, f, 3, 1, 1, 1, &idata);
	call_06(ffpclj, f, 4, 1, 1, 1, &ldata);
	call_06(ffpcle, f, 5, 1, 1, 1, &fdata);
	call_06(ffpcld, f, 6, 1, 1, 1, &ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	call_09(ffgcv, f, TBYTE, 1, 1, 1, 1, NULL, &bresult, &anynull);
	call_09(ffgcv, f, TSHORT, 2, 1, 1, 1, NULL, &sresult, &anynull);
	call_09(ffgcv, f, TINT, 3, 1, 1, 1, NULL, &iresult, &anynull);
	call_09(ffgcv, f, TLONG, 4, 1, 1, 1, NULL, &lresult, &anynull);
	call_09(ffgcv, f, TFLOAT, 5, 1, 1, 1, NULL, &fresult, &anynull);
	call_09(ffgcv, f, TDOUBLE, 6, 1, 1, 1, NULL, &dresult, &anynull);

	fail_if(bresult != bdata);
	fail_if(sresult != sdata);
	fail_if(iresult != idata);
	fail_if(lresult != ldata);
	fail_if(fresult != fdata);
	fail_if(dresult != ddata);

	call_01(ffclos, f);
}

/*
 * Test large (> MINDIRECT) image read that triggers direct disk read path.
 */
static void
test_large_image_read(void)
{
	fitsfile *f;
	int status = 0;
	float *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 1000;
	long naxes[] = { 0 };
	float *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (float)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	/* Read the large image */
	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[nvals - 1] != (float)nvals);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test large short image to exercise ffgi2b direct path.
 */
static void
test_large_short_image(void)
{
	fitsfile *f;
	int status = 0;
	short *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 1000;
	long naxes[] = { 0 };
	short *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (short)((i + 1) % 30000);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[nvals - 1] != (short)((nvals) % 30000));
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test large int image to exercise ffgi4b direct path
 */
static void
test_large_int_image(void)
{
	fitsfile *f;
	int status = 0;
	int *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 1000;
	long naxes[] = { 0 };
	int *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprk, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TINT, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[nvals - 1] != nvals);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test large double image to exercise ffgr8b/ffgi8b direct path.
 */
static void
test_large_double_image(void)
{
	fitsfile *f;
	int status = 0;
	double *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 1000;
	long naxes[] = { 0 };
	double *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TDOUBLE, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[nvals - 1] != (double)nvals);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test large write overwriting existing data (not at EOF)
 * to trigger the read path in ffpbyt.
 */
static void
test_large_overwrite(void)
{
	fitsfile *f;
	int status = 0;
	float *data;
	long nvals = MINDIRECT_SIZE / sizeof *data + 1000;
	long naxes[] = { 0 };
	float *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	/* Initialize with sequential values */
	for (i = 0; i < nvals; i += 1) {
		data[i] = (float)(i + 1);
	}

	/* Create file with initial data */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	/* Initialize with different values for overwrite */
	for (i = 0; i < nvals; i += 1) {
		data[i] = (float)(i + 10001);
	}

	/* Open and overwrite with large write (triggers "not at EOF" path) */
	call_03(ffopen, &f, test_path, READWRITE);
	call_05(ffppre, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	/* Verify overwritten data */
	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 10001.0f);
	fail_if(result[nvals - 1] != (float)(nvals + 10000));
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test large byte image to exercise ffgi1b direct path.
 */
static void
test_large_byte_image(void)
{
	fitsfile *f;
	int status = 0;
	long nvals = MINDIRECT_SIZE + 1000;
	long naxes[] = { 0 };
	unsigned char *data;
	unsigned char *result;
	int i, anynull;

	naxes[0] = nvals;
	data = xmalloc(nvals * sizeof *data);
	result = xmalloc(nvals * sizeof *result);

	for (i = 0; i < nvals; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, nvals, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TBYTE, 1, nvals, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[255] != 255);
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test ffbfeof by deleting rows from a table which truncates the file
 * to exercise the buffer clearing code for buffers beyond new EOF.
 */
static void
test_delete_rows_truncate(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1000E" };  /* 4000 bytes per row */
	float *data;
	int i;
	LONGLONG nrows;

	data = xmalloc(1000 * sizeof *data);

	for (i = 0; i < 1000; i += 1) {
		data[i] = (float)(i + 1);
	}

	/* Create table with 100 rows = 400000 bytes of data. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 100, 1, ttype, tform, NULL, NULL);

	/* Write data to many rows to ensure buffers are used. */
	for (i = 0; i < 100; i += 1) {
		call_06(ffpcle, f, 1, i + 1, 1, 1000, data);
	}
	call_01(ffflus, f);  /* Flush to establish file state. */

	/* Read some data to load buffers. */
	for (i = 0; i < 50; i += 1) {
		float result[1000];
		int anynull;
		call_09(ffgcv, f, TFLOAT, 1, i + 50, 1, 1000, NULL, result, &anynull);
	}

	/* Now delete many rows to trigger truncation and ffbfeof. */
	call_03(ffdrow, f, 1, 80);  /* Delete 80 rows starting at row 1 */

	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 20);

	call_01(ffclos, f);
	free(data);
}

/*
 * Test ffptbb auto-extend when table has a following HDU
 * to exercise the ffirow path in ffptbb.
 */
static void
test_ffptbb_auto_extend_with_following_hdu(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "100B" };  /* 100 bytes per row */
	long naxes[] = { 10 };
	unsigned char data[100];
	unsigned char result[100];
	int i;
	LONGLONG nrows;

	for (i = 0; i < 100; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}

	/* Create file with table followed by another HDU. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);

	/* Write some initial data. */
	call_05(ffptbb, f, 1, 1, 100, data);

	/* Add another HDU after the table so the table is NOT the last HDU. */
	call_04(ffcrim, f, BYTE_IMG, 1, naxes);

	/* Move back to the table. */
	call_03(ffmahd, f, 2, NULL);

	/* Now write to row 20 to trigger auto-extend with ffirow. */
	call_05(ffptbb, f, 20, 1, 100, data);

	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 20);

	/* Read back and verify. */
	call_05(ffgtbb, f, 20, 1, 100, result);
	for (i = 0; i < 100; i += 1) {
		fail_if(result[i] != data[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test ffptbb auto-extend on last HDU with no heap
 * to exercise the direct numrows update path.
 */
static void
test_ffptbb_auto_extend_last_hdu(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "100B" };  /* 100 bytes per row */
	unsigned char data[100];
	unsigned char result[100];
	int i;
	LONGLONG nrows;

	for (i = 0; i < 100; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}

	/* Create file with just a table (last HDU, no heap). */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);

	/* Write initial data. */
	call_05(ffptbb, f, 1, 1, 100, data);

	/* Write to row 50 to auto-extend directly (since this is last HDU). */
	call_05(ffptbb, f, 50, 1, 100, data);

	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 50);

	/* Read back and verify. */
	call_05(ffgtbb, f, 50, 1, 100, result);
	for (i = 0; i < 100; i += 1) {
		fail_if(result[i] != data[i]);
	}

	call_01(ffclos, f);
}

/*
 * Test ffgtbb/ffptbb error paths for invalid row/element numbers.
 */
static void
test_table_byte_io_errors(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "100B" };
	unsigned char data[100];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);

	/* Test BAD_ROW_NUM for ffgtbb (row 0). */
	status = 0;
	ffgtbb(f, 0, 1, 100, data, &status);
	fail_if(status != BAD_ROW_NUM);

	/* Test BAD_ELEM_NUM for ffgtbb (element 0). */
	status = 0;
	ffgtbb(f, 1, 0, 100, data, &status);
	fail_if(status != BAD_ELEM_NUM);

	/* Test BAD_ROW_NUM for ffptbb (row 0). */
	status = 0;
	ffptbb(f, 0, 1, 100, data, &status);
	fail_if(status != BAD_ROW_NUM);

	/* Test BAD_ELEM_NUM for ffptbb (element 0). */
	status = 0;
	ffptbb(f, 1, 0, 100, data, &status);
	fail_if(status != BAD_ELEM_NUM);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test reading past end of table (BAD_ROW_NUM in ffgtbb).
 */
static void
test_read_past_table_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "100B" };
	unsigned char data[100];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Try to read row 100 of a 10-row table. */
	status = 0;
	ffgtbb(f, 100, 1, 100, data, &status);
	fail_if(status != BAD_ROW_NUM);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test writing element that precisely spans record boundary in buffered path
 * with single element (exercise  last element case in ffpbytoff).
 */
static void
test_last_element_span_boundary(void)
{
	fitsfile *f;
	int status = 0;
	/*
	** Design a table where:
	** - Row size is slightly larger than record (2880)
	** - The element starts in the last few bytes of a record
	** - When ngroups=1, only the "last element" code path is taken
	**
	** Row size 2900: element at offset 2876 spans 2876-2879 (rec 0)
	** + 0-3 (rec 1)
	*/
	char *ttype[] = { "PAD1", "PAD2", "VAL" };

	/* 2800 + 76 + 8 = 2884 bytes/row */
	char *tform[] = { "2800B", "76B", "1D" };
	double ddata, dresult;
	int anynull;

	ddata = 123.456;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 3, ttype, tform, NULL, NULL);

	/* Write single element at boundary-spanning position. */
	call_06(ffpcld, f, 3, 1, 1, 1, &ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 3, 1, 1, 1, NULL, &dresult, &anynull);
	fail_if(dresult != ddata);
	call_01(ffclos, f);
}

/*
 * Test multiple elements where last one spans boundary
 * to exercise the case where ngroups > 1 and last element spans.
 */
static void
test_multi_element_last_spans(void)
{
	fitsfile *f;
	int status = 0;
	/*
	** Row size 2884 bytes, element at offset 2876.  Write to 3 rows,
	** each row's element spans the 2880 boundary differently.
	*/
	char *ttype[] = { "PAD", "VAL" };
	char *tform[] = { "2876B", "1D" };  /* 2876 + 8 = 2884 bytes/row */
	double ddata[3], dresult[3];
	int i, anynull;

	for (i = 0; i < 3; i += 1) {
		ddata[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 2, ttype, tform, NULL, NULL);

	/* Write all 3 elements in one call to exercise multi-group path. */
	call_06(ffpcld, f, 2, 1, 1, 3, ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 3, NULL, dresult, &anynull);
	for (i = 0; i < 3; i += 1) {
		fail_if(dresult[i] != ddata[i]);
	}
	call_01(ffclos, f);
}

/*
 * Test large buffered write that wraps around records in the "last chunk" code
 * to exercise where nwrite < gsize for the final group.
 */
static void
test_buffered_write_wrap_last_chunk(void)
{
	fitsfile *f;
	int status = 0;
	/*
	** Create scenario where:
	** - Writing uses buffered path (not direct)
	** - Final chunk of write doesn't fit in current record
	**
	** Write 3000 bytes starting mid-record. This will span records
	** and the last portion will wrap.
	*/
	char *ttype[] = { "DATA" };
	char *tform[] = { "6000B" };  /* Large row to ensure buffered path */
	unsigned char *data;
	unsigned char *result;
	int i;

	data = xmalloc(6000 * sizeof *data);
	result = xmalloc(6000 * sizeof *result);

	for (i = 0; i < 6000; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);

	/* Write full rows to exercise buffered write paths. */
	call_05(ffptbb, f, 1, 1, 6000, data);
	call_05(ffptbb, f, 2, 1, 6000, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgtbb, f, 1, 1, 6000, result);
	for (i = 0; i < 6000; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test NEG_FILE_POS error when seeking to negative position.
 */
static void
test_neg_file_pos(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	signed char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TSBYTE, 1, 10, data);
	/* Try to move to negative position - should fail */
	/* IGNORE_EOF = 1, REPORT_EOF = 0 */
	ffmbyt(f, -1, 1, &status);
	fail_if(status != NEG_FILE_POS);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test write that spans multiple record boundaries.
 * This exercises the record-spanning code in ffpbyt.
 */
static void
test_write_spanning_records(void)
{
	fitsfile *f;
	int status = 0;
	/* Create a large enough write to span multiple IOBUFLEN records */
	/* IOBUFLEN is typically 2880 bytes, so write ~10000 bytes */
	char *ttype[] = { "BIGCOL" };
	char *tform[] = { "10000B" };
	unsigned char *data;
	unsigned char *result;
	int i;

	data = xmalloc(10000 * sizeof *data);
	result = xmalloc(10000 * sizeof *result);

	for (i = 0; i < 10000; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);

	/* This write should span multiple records */
	call_05(ffptbb, f, 1, 1, 10000, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgtbb, f, 1, 1, 10000, result);
	for (i = 0; i < 10000; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);

	free(data);
	free(result);
}

/*
 * Test with exact calculated parameters to ensure last element spans boundary.
 * With data_start=5760 (2 header blocks), 4 rows, 713B padding + 1D element:
 * - Element 4 position = 5760 + 3*721 + 713 = 8636
 * - 8636 mod 2880 = 2876, element size 8 -> spans to pos 4 of next record
 */
static void
test_last_element_exact_span(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "PAD", "VAL" };
	char *tform[] = { "713B", "1D" };  /* 713 + 8 = 721 bytes/row */
	double ddata[4], dresult[4];
	int i, anynull;

	for (i = 0; i < 4; i += 1) {
		ddata[i] = (double)(i + 100);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 2, ttype, tform, NULL, NULL);

	/* Write 4 doubles; the 4th should span the record boundary */
	call_06(ffpcld, f, 2, 1, 1, 4, ddata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 4, NULL, dresult, &anynull);
	for (i = 0; i < 4; i += 1) {
		fail_if(dresult[i] != ddata[i]);
	}
	call_01(ffclos, f);
}

int
main(void)
{
	test_ffgrsz_image();
	test_ffgrsz_binary_table();
	test_ffflus();
	test_large_write();
	test_cross_record_write();
	test_multiple_hdu_access();
	test_read_write_interleaved();
	test_ascii_table_buffering();
	test_large_direct_write();
	test_large_direct_read();
	test_table_byte_io();
	test_ffflsh();
	test_buffer_management();
	test_write_beyond_eof();
	test_integer_types_io();
	test_strided_io();
	test_byte_io();
	test_multi_record_spanning();
	test_very_large_image();
	test_ffflus_table();
	test_ascii_table_large();
	test_auto_expand_table();
	test_noncontiguous_column_io();
	test_noncontiguous_spanning_records();
	test_byte_column_noncontiguous();
	test_element_spanning_record();
	test_ascii_large_direct_write();
	test_massive_direct_io();
	test_element_at_boundary();
	test_float_at_boundary();
	test_short_at_boundary();
	test_single_element_spanning();
	test_read_existing_after_partial_write();
	test_large_image_read();
	test_large_short_image();
	test_large_int_image();
	test_large_double_image();
	test_large_byte_image();
	test_large_overwrite();
	test_mixed_column_types();
	test_delete_rows_truncate();
	test_ffptbb_auto_extend_with_following_hdu();
	test_ffptbb_auto_extend_last_hdu();
	test_table_byte_io_errors();
	test_read_past_table_end();
	test_last_element_span_boundary();
	test_multi_element_last_spans();
	test_buffered_write_wrap_last_chunk();
	test_neg_file_pos();
	test_write_spanning_records();
	test_last_element_exact_span();

	/*
	** Run some tests again to exercise code paths with
	** different buffer states.
	*/
	test_table_byte_io();
	test_noncontiguous_column_io();
	test_element_at_boundary();

	remove(test_path);

	return 0;
}
