/*
 * Tests to exercise edithdu.c - HDU copying/editing functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_edithdu.fits"
#define test_path2 "test_edithdu2.fits"
#define test_path3 "test_edithdu3.fits"

static void
test_cpfl_copy_all(void)
{
	/* Test ffcpfl - copy all HDUs from input to output. */
	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300 };
	int numhdus;

	/* Create input file with primary + 2 extensions. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, in, BINARY_TBL, 3, 1, ttype, tform, NULL, "EXT1");
	call_06(ffpclj, in, 1, 1, 1, 3, data);
	call_08(ffcrtb, in, BINARY_TBL, 2, 1, ttype, tform, NULL, "EXT2");
	call_01(ffclos, in);

	/* Create empty output file. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	/* Open both and copy all HDUs using ffcpfl. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	/* Move to HDU 2 in input, then copy previous + current + following. */
	call_03(ffmahd, in, 2, NULL);
	call_05(ffcpfl, in, out, 1, 1, 1);  /* copy all */
	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify output has 3 HDUs. */
	call_03(ffopen, &out, test_path2, READONLY);
	call_02(ffthdu, out, &numhdus);
	fail_if(numhdus != 4);  /* original empty + 3 copied */
	call_01(ffclos, out);
}

static void
test_cpfl_copy_current_only(void)
{
	/* Test ffcpfl - copy only current HDU. */
	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300 };
	int numhdus;
	char extname[FLEN_VALUE];

	/* Create input file with primary + 2 extensions. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, in, BINARY_TBL, 3, 1, ttype, tform, NULL, "FIRST");
	call_06(ffpclj, in, 1, 1, 1, 3, data);
	call_08(ffcrtb, in, BINARY_TBL, 2, 1, ttype, tform, NULL, "SECOND");
	call_01(ffclos, in);

	/* Create empty output file. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	/* Open both and copy only current HDU. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	/* Move to HDU 2 (FIRST extension), copy only current. */
	call_03(ffmahd, in, 2, NULL);
	call_05(ffcpfl, in, out, 0, 1, 0);  /* only current */
	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify output has 2 HDUs (original + 1 copied). */
	call_03(ffopen, &out, test_path2, READONLY);
	call_02(ffthdu, out, &numhdus);
	fail_if(numhdus != 2);
	/* Check it's the FIRST extension. */
	call_03(ffmahd, out, 2, NULL);
	call_05(ffgky, out, TSTRING, "EXTNAME", extname, NULL);
	fail_if(strcmp(extname, "FIRST") != 0);
	call_01(ffclos, out);
}

static void
test_cpfl_copy_following(void)
{
	/* Test ffcpfl - copy following HDUs only. */
	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	int numhdus;
	char extname[FLEN_VALUE];

	/* Create input file with primary + 3 extensions. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, in, BINARY_TBL, 2, 1, ttype, tform, NULL, "A");
	call_08(ffcrtb, in, BINARY_TBL, 2, 1, ttype, tform, NULL, "B");
	call_08(ffcrtb, in, BINARY_TBL, 2, 1, ttype, tform, NULL, "C");
	call_01(ffclos, in);

	/* Create empty output file. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	/* Copy only following HDUs from HDU 2 (extension A). */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	call_03(ffmahd, in, 2, NULL);  /* Move to extension A */
	call_05(ffcpfl, in, out, 0, 0, 1);  /* only following */
	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify output has 3 HDUs (original + B + C). */
	call_03(ffopen, &out, test_path2, READONLY);
	call_02(ffthdu, out, &numhdus);
	fail_if(numhdus != 3);
	/* Check extensions are B and C. */
	call_03(ffmahd, out, 2, NULL);
	call_05(ffgky, out, TSTRING, "EXTNAME", extname, NULL);
	fail_if(strcmp(extname, "B") != 0);
	call_03(ffmahd, out, 3, NULL);
	call_05(ffgky, out, TSTRING, "EXTNAME", extname, NULL);
	fail_if(strcmp(extname, "C") != 0);
	call_01(ffclos, out);
}

static void
test_cpht_copy_table_subset(void)
{
	/* Test ffcpht - copy table structure with limited row range. */
	fitsfile *in, *out;
	int status = 0;
	char *ttype[] = { "COL1", "COL2" };
	char *tform[] = { "1J", "1E" };
	long jdata[] = { 10, 20, 30, 40, 50 };
	float edata[] = { 1.1f, 2.2f, 3.3f, 4.4f, 5.5f };
	long jresult[3];
	float eresult[3];
	LONGLONG nrows;

	/* Create input table with 5 rows. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, in, BINARY_TBL, 5, 2, ttype, tform, NULL, "DATA");
	call_06(ffpclj, in, 1, 1, 1, 5, jdata);
	call_06(ffpcle, in, 2, 1, 1, 5, edata);
	call_01(ffclos, in);

	/* Create empty output file. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	/* Copy rows 2-4 from input to output. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	call_03(ffmahd, in, 2, NULL);  /* Move to table */
	call_04(ffcpht, in, out, 2LL, 3LL);  /* Copy rows 2-4 (3 rows). */
	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify output table has 3 rows with correct data. */
	call_03(ffopen, &out, test_path2, READONLY);
	call_03(ffmahd, out, 2, NULL);
	call_02(ffgnrwll, out, &nrows);
	fail_if(nrows != 3);
	call_08(ffgcvj, out, 1, 1, 1, 3, 0, jresult, NULL);
	call_08(ffgcve, out, 2, 1, 1, 3, 0, eresult, NULL);
	fail_if(jresult[0] != 20);  /* Original row 2. */
	fail_if(jresult[1] != 30);
	fail_if(jresult[2] != 40);
	fail_if(eresult[0] < 2.1f || eresult[0] > 2.3f);
	fail_if(eresult[1] < 3.2f || eresult[1] > 3.4f);
	fail_if(eresult[2] < 4.3f || eresult[2] > 4.5f);
	call_01(ffclos, out);
}

static void
test_wrhdu_write_to_stream(void)
{
	/* Test ffwrhdu - write HDU to output stream. */
	fitsfile *f, *f2;
	int status = 0;
	long naxes[] = { 10, 10 };
	short data[100];
	short result[100];
	FILE *stream;
	int i;

	/* Create FITS file with image data. */
	for (i = 0; i < 100; i += 1) {
		data[i] = (short)(i * 2);
	}
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_05(ffppri, f, 1, 1, 100, data);
	call_01(ffclos, f);

	/* Open and write HDU to a stream (file). */
	call_03(ffopen, &f, test_path, READONLY);
	stream = fopen(test_path2, "wb");
	fail_if(stream == NULL);
	call_02(ffwrhdu, f, stream);
	fclose(stream);
	call_01(ffclos, f);

	/* Verify the stream output is a valid FITS file. */
	call_03(ffopen, &f2, test_path2, READONLY);
	call_07(ffgpvi, f2, 1, 1, 100, 0, result, NULL);
	fail_if(result[0] != 0);
	fail_if(result[50] != 100);
	fail_if(result[99] != 198);
	call_01(ffclos, f2);
}

/* Test that functions return early when status > 0 */
static void
test_error_status_returns(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	status = 1;
	ffcopy(f, f, 0, &status);
	fail_if(status != 1);

	ffcphd(f, f, &status);
	fail_if(status != 1);

	ffcpdt(f, f, &status);
	fail_if(status != 1);

	ffwrhdu(f, NULL, &status);
	fail_if(status != 1);

	ffiimg(f, 16, 1, naxes, &status);
	fail_if(status != 1);

	ffitab(f, 10, 1, 1, NULL, NULL, NULL, NULL, NULL, &status);
	fail_if(status != 1);

	ffibin(f, 1, 1, NULL, NULL, NULL, NULL, 0, &status);
	fail_if(status != 1);

	ffdhdu(f, NULL, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, f);
}

/* Test SAME_FILE error when copying to same file pointer */
static void
test_same_file_error(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	/* Try to copy HDU to same file - should return SAME_FILE error */
	ffcopy(f, f, 0, &status);
	fail_if(status != SAME_FILE);
	status = 0;

	ffcphd(f, f, &status);
	fail_if(status != SAME_FILE);
	status = 0;

	ffcpdt(f, f, &status);
	fail_if(status != SAME_FILE);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffiimg - insert image extension */
static void
test_insert_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	int numhdus;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffiimg, f, SHORT_IMG, 2, naxes);
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 2);
	call_01(ffclos, f);
}

/* Test ffitab - insert ASCII table */
static void
test_insert_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "F10.2" };
	char *tunit[] = { "m" };
	int numhdus;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_09(ffitab, f, 80, 1, 1, ttype, NULL, tform, tunit, "TEST");
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 2);
	call_01(ffclos, f);
}

/* Test ffibin - insert binary table */
static void
test_insert_binary_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2" };
	char *tform[] = { "1J", "1E" };
	int numhdus;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffibin, f, 5, 2, ttype, tform, NULL, "TABLE", 0);
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 2);
	call_01(ffclos, f);
}

/* Test ffdhdu - delete current HDU */
static void
test_delete_hdu(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	int numhdus;
	int hdutype;

	/* Create file with 3 HDUs. */
	call_02(ffinit, &f, "!" test_path3);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, "TABLE1");
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, "TABLE2");
	call_01(ffclos, f);

	/* Reopen and verify 3 HDUs. */
	call_03(ffopen, &f, test_path3, READWRITE);
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 3);

	/* Delete HDU 2 (TABLE1). */
	call_03(ffmahd, f, 2, NULL);
	call_02(ffdhdu, f, &hdutype);

	/* Verify only 2 HDUs remain. */
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 2);
	call_01(ffclos, f);
}

/* Test BAD_NAXIS error with too many dimensions */
static void
test_bad_naxis(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[200];
	int i;

	for (i = 0; i < 200; i += 1) {
		naxes[i] = 2;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert image with 200 dimensions - should fail */
	ffiimg(f, SHORT_IMG, 200, naxes, &status);
	fail_if(status != BAD_NAXIS);
	status = 0;

	call_01(ffclos, f);
}

/* Test BAD_BITPIX error with invalid bitpix value in ffiimgll */
static void
test_bad_bitpix(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert image with invalid bitpix - should fail */
	ffiimg(f, 99, 1, naxes, &status);
	fail_if(status != BAD_BITPIX);
	status = 0;

	call_01(ffclos, f);
}

/* Test BAD_NAXES error with negative axis size in ffiimgll */
static void
test_bad_naxes(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { -10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert image with negative axis - should fail */
	ffiimg(f, SHORT_IMG, 1, naxes, &status);
	fail_if(status != BAD_NAXES);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffiimg with 64-bit bitpix values in ffiimgll */
static void
test_insert_64bit_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int hdunum;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Insert 64-bit integer image */
	call_04(ffiimg, f, LONGLONG_IMG, 1, naxes);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 2);

	/* Move to end and insert 64-bit float image */
	call_03(ffmahd, f, 2, NULL);
	call_04(ffiimg, f, DOUBLE_IMG, 1, naxes);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 3);

	call_01(ffclos, f);
}

/* Test ffcopy with morekeys parameter to reserve header space */
static void
test_copy_with_morekeys(void)
{
	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 10 };

	/* Create input file */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	call_01(ffclos, in);

	/* Create output file */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	/* Copy with morekeys=10 to reserve header space */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	call_03(ffcopy, in, out, 10);
	call_01(ffclos, in);
	call_01(ffclos, out);
}

/*
 * Test copying HDU data within the same file.
 * Need to open the file twice with different fitsfile pointers.
 */
static void
test_copy_within_same_file(void)
{
	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 100 };  /* Need data to copy. */
	unsigned char data[100];
	int numhdus;
	int i;

	for (i = 0; i < 100; i += 1) {
		data[i] = (unsigned char)i;
	}

	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	call_05(ffpprb, in, 1, 1, 100, data);
	call_01(ffclos, in);

	/* Open the same file twice - this creates two fitsfile pointers
	** but they share the same underlying file (Fptr). */
	call_03(ffopen, &in, test_path, READWRITE);
	call_03(ffopen, &out, test_path, READWRITE);
	call_03(ffmahd, in, 1, NULL);
	/* Copy from in to out within same file. */
	call_03(ffcopy, in, out, 0);
	call_02(ffthdu, out, &numhdus);
	fail_if(numhdus != 2);
	call_01(ffclos, in);
	call_01(ffclos, out);
}

/* Test ffitab with negative width */
static void
test_itab_negative_width(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "F10.2" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert ASCII table with negative width. */
	ffitab(f, -10, 1, 1, ttype, NULL, tform, NULL, NULL, &status);
	fail_if(status != NEG_WIDTH);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffitab with negative rows */
static void
test_itab_negative_rows(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "F10.2" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert ASCII table with negative rows. */
	ffitab(f, 80, -5, 1, ttype, NULL, tform, NULL, NULL, &status);
	fail_if(status != NEG_ROWS);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffitab with bad tfields */
static void
test_itab_bad_tfields(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try with negative tfields. */
	ffitab(f, 80, 1, -5, NULL, NULL, NULL, NULL, NULL, &status);
	fail_if(status != BAD_TFIELDS);
	status = 0;

	/* Try with too many fields (> 999). */
	ffitab(f, 80, 1, 1000, NULL, NULL, NULL, NULL, NULL, &status);
	fail_if(status != BAD_TFIELDS);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffibin with negative rows */
static void
test_ibin_negative_rows(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert binary table with negative rows. */
	ffibin(f, -5, 1, ttype, tform, NULL, NULL, 0, &status);
	fail_if(status != NEG_ROWS);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffibin with bad tfields */
static void
test_ibin_bad_tfields(void)
{

	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try with negative tfields. */
	ffibin(f, 1, -5, NULL, NULL, NULL, NULL, 0, &status);
	fail_if(status != BAD_TFIELDS);
	status = 0;

	/* Try with too many fields (> 999). */
	ffibin(f, 1, 1000, NULL, NULL, NULL, NULL, 0, &status);
	fail_if(status != BAD_TFIELDS);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffiimg with BYTE_IMG bitpix */
static void
test_insert_byte_image(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int hdunum;
	int bitpix;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 0, NULL);

	/* Insert 8-bit image. */
	call_04(ffiimg, f, BYTE_IMG, 1, naxes);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 2);
	call_05(ffgky, f, TINT, "BITPIX", &bitpix, NULL);
	fail_if(bitpix != 8);

	call_01(ffclos, f);
}

/* Test ffiimg with naxis=0 (null image) */
static void
test_insert_naxis_zero(void)
{

	fitsfile *f;
	int status = 0;
	int hdunum;
	int naxis;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Insert image with naxis=0 (null image). */
	call_04(ffiimg, f, SHORT_IMG, 0, NULL);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 2);
	call_05(ffgky, f, TINT, "NAXIS", &naxis, NULL);
	fail_if(naxis != 0);

	call_01(ffclos, f);
}

/*
 * Test ffcopy with source having large empty header space (ffwend
 * path)
 */
static void
test_copy_with_large_header_space(void)
{

	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 10 };
	int numhdus;
	int i;

	/* Create input file with lots of reserved header space (>= 35 keys). */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	/* Reserve 100 additional keywords - this will create multiple
	** empty blocks in the header. */
	call_02(ffhdef, in, 100);
	call_01(ffclos, in);

	/* Create output file. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	/* Copy with morekeys=0 to use the preserved space path. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	call_03(ffcopy, in, out, 0);  /* morekeys=0 triggers space preservation */
	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify copy succeeded. */
	call_03(ffopen, &out, test_path2, READONLY);
	call_02(ffthdu, out, &numhdus);
	fail_if(numhdus < 2);
	call_01(ffclos, out);
}

/* Test ffiimg with negative naxis */
static void
test_negative_naxis(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert image with negative naxis. */
	ffiimg(f, SHORT_IMG, -1, naxes, &status);
	fail_if(status != BAD_NAXIS);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffiimg with naxis > 999 */
static void
test_naxis_over_limit(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[1000];
	int i;

	for (i = 0; i < 1000; i += 1) {
		naxes[i] = 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to insert image with naxis=1000 (> 999 limit). */
	ffiimg(f, SHORT_IMG, 1000, naxes, &status);
	fail_if(status != BAD_NAXIS);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffcpfl with same file (should fail) */
static void
test_cpfl_same_file_error(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	/* Try to copy to same file - should return SAME_FILE. */
	ffcpfl(f, f, 1, 1, 1, &status);
	fail_if(status != SAME_FILE);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffcpfl with initial error status */
static void
test_cpfl_status_error(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	/* Call ffcpfl with status already set. */
	status = 1;
	ffcpfl(f, f, 1, 1, 1, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, f);
}

/* Test ffiimg on readonly file */
static void
test_insert_image_readonly(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	/* Create a file. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_01(ffclos, f);

	/* Open as readonly and try to insert image. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 1, NULL);  /* Move to primary */
	ffiimg(f, SHORT_IMG, 1, naxes, &status);
	fail_if(status != READONLY_FILE);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffitab on readonly file */
static void
test_insert_ascii_table_readonly(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "F10.2" };

	/* Create a file with an extension. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_01(ffclos, f);

	/* Open as readonly and try to insert ASCII table. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 1, NULL);  /* Move to primary */
	ffitab(f, 80, 1, 1, ttype, NULL, tform, NULL, NULL, &status);
	fail_if(status != READONLY_FILE);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffibin on readonly file */
static void
test_insert_binary_table_readonly(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	/* Create a file with an extension. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_01(ffclos, f);

	/* Open as readonly and try to insert binary table. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 1, NULL);  /* Move to primary */
	ffibin(f, 1, 1, ttype, tform, NULL, NULL, 0, &status);
	fail_if(status != READONLY_FILE);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffcpht with status already set */
static void
test_cpht_status_error(void)
{

	fitsfile *in, *out;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 1, naxes);
	call_01(ffclos, in);

	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_01(ffclos, out);

	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);

	/* Call ffcpht with status already set. */
	status = 1;
	ffcpht(in, out, 1LL, 1LL, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, in);
	call_01(ffclos, out);
}

/*
 * Test copying a table to an empty output file (create dummy primary
 * for table)
 */
static void
test_copy_table_to_empty_primary(void)
{

	fitsfile *in, *out;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	/* Create input file with a binary table. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, in, BINARY_TBL, 3, 1, ttype, tform, NULL, "DATA");
	call_01(ffclos, in);

	/* Create brand new output file using stream driver to avoid
	** automatic primary array creation. Use a memory file instead. */
	call_02(ffinit, &out, "mem://test_empty");
	/* The output is now at HDU position 0 (primary) with empty header. */

	/* Open input and copy table header to output primary position. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffmahd, in, 2, NULL);  /* Move to table. */
	call_02(ffcphd, in, out);  /* Copy table header to output. */
	call_01(ffclos, in);
	call_01(ffclos, out);
}

/* Test ffiimgll with status already set */
static void
test_iimgll_status_error(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	LONGLONG llnaxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Call ffiimgll with status already set. */
	status = 1;
	ffiimgll(f, SHORT_IMG, 1, llnaxes, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, f);
}

/* Test ffdhdu on last HDU making primary empty */
static void
test_delete_last_hdu(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int numhdus;
	int naxis;

	/* Create file with primary image (non-empty). */
	call_02(ffinit, &f, "!" test_path3);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	/* Reopen and check. */
	call_03(ffopen, &f, test_path3, READWRITE);
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 1);

	/* Move to HDU 1 (primary) and delete it. */
	call_03(ffmahd, f, 1, NULL);
	ffdhdu(f, NULL, &status);
	/* After deleting the only HDU, should get a minimal primary. */
	fail_if(status != 0);

	/* Check that we still have 1 HDU but it's empty. */
	call_02(ffthdu, f, &numhdus);
	fail_if(numhdus != 1);
	call_05(ffgky, f, TINT, "NAXIS", &naxis, NULL);
	fail_if(naxis != 0);  /* Primary should now be empty. */

	call_01(ffclos, f);
}


int
main(void)
{
	test_cpfl_copy_all();
	test_cpfl_copy_current_only();
	test_cpfl_copy_following();
	test_cpht_copy_table_subset();
	test_wrhdu_write_to_stream();
	test_error_status_returns();
	test_same_file_error();
	test_insert_image();
	test_insert_ascii_table();
	test_insert_binary_table();
	test_delete_hdu();
	test_bad_naxis();
	test_bad_bitpix();
	test_bad_naxes();
	test_insert_64bit_image();
	test_copy_with_morekeys();
	test_copy_within_same_file();
	test_itab_negative_width();
	test_itab_negative_rows();
	test_itab_bad_tfields();
	test_ibin_negative_rows();
	test_ibin_bad_tfields();
	test_insert_byte_image();
	test_insert_naxis_zero();
	test_copy_with_large_header_space();
	test_negative_naxis();
	test_naxis_over_limit();
	test_cpfl_same_file_error();
	test_cpfl_status_error();
	test_insert_image_readonly();
	test_insert_ascii_table_readonly();
	test_insert_binary_table_readonly();
	test_cpht_status_error();
	test_copy_table_to_empty_primary();
	test_iimgll_status_error();
	test_delete_last_hdu();
	remove(test_path);
	remove(test_path2);
	remove(test_path3);
	return 0;
}
