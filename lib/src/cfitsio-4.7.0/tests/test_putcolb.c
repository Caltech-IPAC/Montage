/*
 * Tests for putcolb.c - byte (unsigned char) write functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcolb.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned char data[] = { 0, 50, 100, 200, 255 };
	unsigned char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvb, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 50);
	fail_if(result[2] != 100);
	fail_if(result[3] != 200);
	fail_if(result[4] != 255);
	call_01(ffclos, f);
}

static void
test_write_primary_with_null(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned char data[] = { 10, 20, 30, 40, 50 };
	unsigned char result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_04(ffpkyj, f, "BLANK", 30, NULL);  /* Define null value first */
	call_06(ffppnb, f, 1, 1, 5, data, 30);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfb, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(nularray[2] != 1);  /* 30 = null */
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 2 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6 };
	unsigned char result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 2, naxes);
	call_06(ffp2db, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2db, f, 1, 0, 3, 3, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[5] != 6);
	call_01(ffclos, f);
}

static void
test_write_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned char result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 3, naxes);
	call_08(ffp3db, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3db, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[7] != 8);
	call_01(ffclos, f);
}

static void
test_write_subset(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	unsigned char data[] = { 99, 99, 99, 99 };
	unsigned char result[16];
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 2, naxes);
	/* Initialize with zeros */
	memset(result, 0, sizeof result);
	call_05(ffpprb, f, 1, 1, 16, result);
	/* Write subset */
	call_07(ffpssb, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvb, f, 1, 1, 16, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[5] != 99);  /* (2,2) */
	fail_if(result[6] != 99);  /* (3,2) */
	fail_if(result[9] != 99);  /* (2,3) */
	fail_if(result[10] != 99); /* (3,3) */
	fail_if(result[15] != 0);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned char pdata[] = { 10, 20 };
	unsigned char idata[] = { 1, 2, 3, 4 };
	unsigned char presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, BYTE_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpb, f, 1, 1, 2, pdata);
	call_05(ffpprb, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpb, f, 1, 1, 2, presult);
	fail_if(presult[0] != 10);
	fail_if(presult[1] != 20);
	call_01(ffclos, f);
}

static void
test_write_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 0, 127, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 127);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_column_with_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 10, 20, 30 };
	unsigned char result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(ffpkyj, f, "TNULL1", 20, NULL);
	call_07(ffpcnb, f, 1, 1, 1, 3, data, 20);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfb, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(nularray[1] != 1);  /* 20 = null */
	fail_if(result[2] != 30);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_vector_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTEVEC" };
	char *tform[] = { "5B" };
	unsigned char data[] = { 1, 2, 3, 4, 5 };
	unsigned char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
	call_01(ffclos, f);
}

static void
test_write_to_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	unsigned char data[] = { 0, 127, 255 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 127);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_to_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	unsigned char data[] = { 0, 100, 255 };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	unsigned char data[] = { 0, 100, 255 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 100.0f);
	fail_if(result[2] != 255.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	unsigned char data[] = { 0, 100, 255 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 100.0);
	fail_if(result[2] != 255.0);
	call_01(ffclos, f);
}

static void
test_write_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "I4" };
	unsigned char data[] = { 0, 100, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 110 };  /* Will be stored as (110 - 10) / 2 = 50 */
	unsigned char result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 110);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	unsigned char data[] = { 0, 128, 255 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 128);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_bad_col_num(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);

	ffpclb(f, 0, 1, 1, 1, data, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTEVEC" };
	char *tform[] = { "3B" };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	unsigned char result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 9, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
	fail_if(result[8] != 9);
	call_01(ffclos, f);
}

static void
test_write_3d_noncontiguous(void)
{
	/* Test 3D non-contiguous write path */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	/* Array is 3x3x2 but we only write 2x2x2 */
	unsigned char data[] = {
		1, 2, 0, 3, 4, 0, 0, 0, 0,
		5, 6, 0, 7, 8, 0, 0, 0, 0
	};
	unsigned char result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 3, naxes);
	call_08(ffp3db, f, 1, 3, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvb, f, 1, 1, 8, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 4);
	fail_if(result[4] != 5);
	fail_if(result[5] != 6);
	fail_if(result[6] != 7);
	fail_if(result[7] != 8);
	call_01(ffclos, f);
}

static void
test_write_3d_bad_dimen(void)
{
	/* Test BAD_DIMEN error for 3D write with too small dimensions */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 3, naxes);
	/* ncols=2 < naxis1=4, should fail */
	ffp3db(f, 1, 2, 2, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_subsection_bad_naxis(void)
{
	/* Test bad naxis in subset write */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long fpix[] = { 1 };
	long lpix[] = { 5 };
	unsigned char data[] = { 1, 2, 3, 4, 5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	/* naxis=0 should fail */
	ffpssb(f, 1, 0, naxes, fpix, lpix, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_short_overflow_with_scaling(void)
{
	/* Test writing byte that overflows short when scaled */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TSCAL=0.001, dvalue = input/0.001 = input*1000 */
	/* 255 * 1000 = 255000 > 32767 */
	unsigned char data[] = { 1, 255, 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.001, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_short_underflow_with_scaling(void)
{
	/* Test writing byte that underflows short when scaled */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TZERO=100000, dvalue = (input - 100000)/1 < -32768 */
	unsigned char data[] = { 1, 2, 3 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 100000.0, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_int32_overflow_with_scaling(void)
{
	/* Test writing byte that overflows int32 when scaled */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With TSCAL=0.00000001, dvalue = input/0.00000001 = input*1e8 */
	/* 255 * 1e8 > INT32_MAX */
	unsigned char data[] = { 1, 255, 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.00000001, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_int32_underflow_with_scaling(void)
{
	/* Test writing byte that underflows int32 when scaled */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With TZERO=1e10, dvalue = (input - 1e10)/1 < INT32_MIN */
	unsigned char data[] = { 1, 2, 3 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 1e10, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_longlong(void)
{
	/* Test writing byte to unsigned longlong column (TZERO = 2^63) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UKCOL" };
	char *tform[] = { "1K" };
	unsigned char data[] = { 0, 128, 255 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 9223372036854775808.0, 20, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	call_01(ffclos, f);
}

static void
test_longlong_overflow_with_scaling(void)
{
	/* Test writing byte that overflows longlong when scaled */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With TSCAL=1e-19, dvalue = input/1e-19 = input*1e19 > LLONG_MAX */
	unsigned char data[] = { 1, 255, 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 1e-19, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_underflow_with_scaling(void)
{
	/* Test writing byte that underflows longlong when scaled */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With TZERO=1e19, dvalue = (input - 1e19)/1 < LLONG_MIN */
	unsigned char data[] = { 1, 2, 3 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 1e19, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_float_column_scaled(void)
{
	/* Test writing byte to float column with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned char data[] = { 100, 200, 255 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 100.0f);
	fail_if(result[1] != 200.0f);
	fail_if(result[2] != 255.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column_scaled(void)
{
	/* Test writing byte to double column with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned char data[] = { 100, 200, 255 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 255.0);
	call_01(ffclos, f);
}

static void
test_write_ascii_table_scaled(void)
{
	/* Test writing byte to ASCII table with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "F10.2" };
	unsigned char data[] = { 100, 200, 255 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 255.0);
	call_01(ffclos, f);
}

static void
test_ascii_table_overflow_scaled(void)
{
	/* Test ASCII table overflow with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "I3" };  /* Only 3 chars width */
	unsigned char data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* With TSCAL=0.001, dvalue = 1/0.001 = 1000, needs 4 digits */
	call_05(ffpkyd, f, "TSCAL1", 0.001, 15, NULL);
	ffpclb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_negative_dvalue_short(void)
{
	/* Test negative dvalue rounding for byte to short */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned char data[] = { 0 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 makes dvalue = (0-100)/1 = -100 */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvi, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_positive_dvalue_short(void)
{
	/* Test positive dvalue rounding for byte to short with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned char data[] = { 100 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TSCAL=0.5 makes dvalue = 100/0.5 = 200 (positive) */
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvi, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}

static void
test_negative_dvalue_int32(void)
{
	/* Test negative dvalue rounding for byte to int32 */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned char data[] = { 0 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 makes dvalue = (0-100)/1 = -100 */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_positive_dvalue_int32(void)
{
	/* Test positive dvalue rounding for byte to int32 with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned char data[] = { 100 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TSCAL=0.5 makes dvalue = 100/0.5 = 200 (positive) */
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}

static void
test_negative_dvalue_longlong(void)
{
	/* Test negative dvalue rounding for byte to longlong */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned char data[] = { 0 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 makes dvalue = (0-100)/1 = -100 */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvjj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_positive_dvalue_longlong(void)
{
	/* Test positive dvalue rounding for byte to longlong with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned char data[] = { 100 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TSCAL=0.5 makes dvalue = 100/0.5 = 200 (positive, triggers +0.5 rounding) */
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvjj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}

static void
test_vla_with_nulls(void)
{
	/* Test VLA write with nulls */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PB" };
	unsigned char data[] = { 10, 20, 30 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 255);
	call_07(ffpcnb, f, 1, 1, 1, 3, data, 255);
	call_01(ffclos, f);
}

static void
test_vla_with_overflow_null(void)
{
	/* Test VLA null handling where ffpclb returns overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PI" };  /* VLA of short */
	/* 40000 overflows short, triggers overflow in VLA path */
	unsigned char data[] = { 100, 200, 255 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 255);
	call_05(ffpkyd, f, "TSCAL1", 0.001, 15, NULL);  /* 200/0.001 = 200000 > 32767 */
	call_07(ffpcnb, f, 1, 1, 1, 3, data, 255);
	call_01(ffclos, f);
}

static void
test_vla_good_overflow(void)
{
	/* Test VLA write where writing good pixels causes overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PI" };  /* VLA of short */
	/* Pattern: null, overflow-good, null */
	unsigned char data[] = { 255, 200, 255 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 255);
	call_05(ffpkyd, f, "TSCAL1", 0.001, 15, NULL);  /* 200/0.001 = 200000 > 32767 */
	call_07(ffpcnb, f, 1, 1, 1, 3, data, 255);
	call_01(ffclos, f);
}

static void
test_byte_to_byte_overflow_with_scaling(void)
{
	/* Test writing byte to byte column with scaling that causes overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* 255 * 2 = 510 > 255 */
	unsigned char data[] = { 100, 255, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_byte_to_byte_underflow_with_scaling(void)
{
	/* Test writing byte to byte column with scaling that causes underflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TZERO=1000, dvalue = (input-1000)/1 < 0 */
	unsigned char data[] = { 0, 1, 2 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 1000.0, 15, NULL);
	ffpclb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_subsection_3d(void)
{
	/* Test 3D subsection write */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10, 5 };
	long fpixel[] = { 2, 3, 1 };
	long lpixel[] = { 4, 5, 2 };
	unsigned char data[] = {
		1, 2, 3,  4, 5, 6,   7, 8, 9,
		10, 11, 12, 13, 14, 15, 16, 17, 18
	};
	unsigned char result[18];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 3, naxes);
	call_07(ffpssb, f, 1, 3, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvb, f, 1, 22, 3, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	call_01(ffclos, f);
}

static void
test_4d_subsection(void)
{
	/* Test 4D subsection write */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 4, 2 };
	long fpixel[] = { 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2 };
	unsigned char data[] = {
		1, 2, 3, 4, 5, 6, 7, 8,
		9, 10, 11, 12, 13, 14, 15, 16
	};

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 4, naxes);
	call_07(ffpssb, f, 1, 4, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);
}

static void
test_5d_subsection(void)
{
	/* Test 5D subsection write */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3, 3, 3, 2 };
	long fpixel[] = { 1, 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2, 2 };
	unsigned char data[32];
	int i;

	for (i = 0; i < 32; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 5, naxes);
	call_07(ffpssb, f, 1, 5, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);
}

static void
test_write_to_logical_column(void)
{
	/* Test BAD_BTABLE_FORMAT error - logical columns not supported */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLAG" };
	char *tform[] = { "1L" };
	unsigned char data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing float data to unscaled byte column (scale=1, zero=0).
 * This hits the memcpy path in ffr4fi1.
 */
static void
test_float_to_byte_unscaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	float data[] = { 10.0f, 20.0f, 100.0f };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 100);
	call_01(ffclos, f);
}


/*
 * Test ASCII table overflow - large byte values that exceed field width.
 * This hits ffi1fstr overflow.
 */
static void
test_ascii_table_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "I2" };  /* 2-character field - 255 needs 3 chars */
	unsigned char data[] = { 255 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing bytes to fixed-width A (string) column.
 * This hits the raw byte write path.
 */
static void
test_byte_to_string_column_raw(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STR" };
	char *tform[] = { "5A" };
	unsigned char data[] = { 'H', 'E', 'L', 'L', 'O' };
	char result[6] = {0};
	char *results[1] = { result };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 1, "", results, &anynull);
	fail_if(strcmp(result, "HELLO") != 0);
	call_01(ffclos, f);
}


/*
 * Test writing bytes to ASCII table A (string) column.
 * This hits the TSTRING case with strchr(tform,'A').
 */
static void
test_byte_to_ascii_A_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STR" };
	char *tform[] = { "A5" };
	unsigned char data[] = { 'H', 'E', 'L', 'L', 'O' };
	char result[6] = {0};
	char *results[1] = { result };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 1, "", results, &anynull);
	fail_if(strcmp(result, "HELLO") != 0);
	call_01(ffclos, f);
}


/*
 * Test VLA write where overflow is caught and cleared in null-handling path.
 */
static void
test_vla_byte_overflow_cleared(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PB" };  /* VLA of byte */
	/* With TSCAL=0.5, 200/0.5 = 400 > 255, causes overflow */
	unsigned char data[] = { 50, 200, 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 100);  /* 100 is the null value */
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	call_01(ffclos, f);

	/* Reopen to pick up TSCAL */
	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* This should overflow on 200 but continue for null handling */
	ffpcnb(f, 1, 1, 1, 3, data, 100, &status);
	/* Overflow is caught and cleared in VLA path, so final status is 0 */
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test fixed-length array null handling where writing good pixels overflows.
 * When good pixels precede null, the good pixel write can overflow.
 */
static void
test_fixed_array_null_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TSCAL=0.5: 200/0.5=400 (overflow!), 100=null, 50/0.5=100 OK */
	/* Data order matters: overflow value BEFORE null triggers the path */
	unsigned char data[] = { 200, 100, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	call_01(ffclos, f);

	/* Reopen to pick up TSCAL */
	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Write with 100 as null - 200 before null will overflow on write */
	ffpcnb(f, 1, 1, 1, 3, data, 100, &status);
	/* Overflow should be caught and cleared */
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ffpextn - write raw bytes to extension data area.
 */
static void
test_ffpextn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 0xAA, 0xBB, 0xCC, 0xDD };
	unsigned char result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 10, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	ffpextn(f, 0, 4, data, &status);
	fail_if(status != 0);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvb, f, 1, 1, 4, 0, result, &anynull);
	fail_if(result[0] != 0xAA);
	fail_if(result[1] != 0xBB);
	fail_if(result[2] != 0xCC);
	fail_if(result[3] != 0xDD);
	call_01(ffclos, f);
}


int
main(void)
{
	test_write_primary_array();
	test_write_primary_with_null();
	test_write_2d_array();
	test_write_3d_array();
	test_write_subset();
	test_write_group_parameters();
	test_write_column();
	test_write_column_with_null();
	test_write_vector_column();
	test_write_to_short_column();
	test_write_to_long_column();
	test_write_to_float_column();
	test_write_to_double_column();
	test_write_ascii_table();
	test_write_with_scaling();
	test_write_to_longlong_column();
	test_write_bad_col_num();
	test_write_multirow();
	test_write_3d_noncontiguous();
	test_write_3d_bad_dimen();
	test_subsection_bad_naxis();
	test_short_overflow_with_scaling();
	test_short_underflow_with_scaling();
	test_int32_overflow_with_scaling();
	test_int32_underflow_with_scaling();
	test_write_to_unsigned_longlong();
	test_longlong_overflow_with_scaling();
	test_longlong_underflow_with_scaling();
	test_write_to_float_column_scaled();
	test_write_to_double_column_scaled();
	test_write_ascii_table_scaled();
	test_ascii_table_overflow_scaled();
	test_negative_dvalue_short();
	test_positive_dvalue_short();
	test_negative_dvalue_int32();
	test_positive_dvalue_int32();
	test_negative_dvalue_longlong();
	test_positive_dvalue_longlong();
	test_vla_with_nulls();
	test_vla_with_overflow_null();
	test_vla_good_overflow();
	test_byte_to_byte_overflow_with_scaling();
	test_byte_to_byte_underflow_with_scaling();
	test_subsection_3d();
	test_4d_subsection();
	test_5d_subsection();
	test_write_to_logical_column();
	test_float_to_byte_unscaled();
	test_ascii_table_overflow();
	test_byte_to_string_column_raw();
	test_byte_to_ascii_A_column();
	test_vla_byte_overflow_cleared();
	test_fixed_array_null_overflow();
	test_ffpextn();
	remove(test_path);
	return 0;
}
