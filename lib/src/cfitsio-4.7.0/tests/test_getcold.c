/*
 * Tests for getcold.c - double precision read functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcold.fits"

static void
test_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	double data[] = { -1.0e100, -1.0, 0.0, 1.0, 1.0e100 };
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 5, 0.0, result, &anynull);
	fail_if(result[0] != data[0]);
	fail_if(result[2] != 0.0);
	fail_if(result[4] != data[4]);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_primary_with_null_value(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	double data[] = { 1.0, 2.0, -999.0, 4.0, 5.0 };
	double result[5];
	int anynull;

	/* For floating point images, IEEE NaN is the null value. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Read with null substitution value. */
	call_07(ffgpvd, f, 1, 1, 5, -123.0, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[2] != -999.0);  /* No BLANK in floating point. */
	call_01(ffclos, f);
}

static void
test_read_primary_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	double data[] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
	double result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfd, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10.0);
	fail_if(result[4] != 50.0);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 3 };  /* 4 cols x 3 rows. */
	double data[12];
	double result[12];
	int anynull;
	int i;

	for (i = 0; i < 12; i += 1) {
		data[i] = (double)i * 100.5;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_05(ffpprd, f, 1, 1, 12, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2dd, f, 1, 0.0, 4, 4, 3, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[5] != 5 * 100.5);
	fail_if(result[11] != 11 * 100.5);
	call_01(ffclos, f);
}

static void
test_read_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };  /* 2x2x2 = 8 elements. */
	double data[8];
	double result[8];
	int anynull;
	int i;

	for (i = 0; i < 8; i += 1) {
		data[i] = (double)(i + 1) * 0.5;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 3, naxes);
	call_05(ffpprd, f, 1, 1, 8, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dd, f, 1, 0.0, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != 0.5);
	fail_if(result[7] != 4.0);
	call_01(ffclos, f);
}

static void
test_read_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	double data[16];
	double result[4];
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (double)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_05(ffpprd, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvd, f, 1, 2, naxes, fpixel, lpixel, inc, 0.0, result, &anynull);
	fail_if(result[0] != 5.0);
	fail_if(result[1] != 6.0);
	fail_if(result[2] != 9.0);
	fail_if(result[3] != 10.0);
	call_01(ffclos, f);
}

static void
test_read_subsection_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	double data[16];
	double result[4];
	char nularray[4];
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 2, 2 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (double)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_05(ffpprd, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfd, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 1.0);
	fail_if(result[2] != 4.0);
	fail_if(result[3] != 5.0);
	call_01(ffclos, f);
}

static void
test_read_subsection_with_increment(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 6, 6 };
	double data[36];
	double result[4];
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 5, 5 };
	long inc[] = { 2, 2 };  /* Skip every other pixel. */
	int anynull;
	int i;

	for (i = 0; i < 36; i += 1) {
		data[i] = (double)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_05(ffpprd, f, 1, 1, 36, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvd, f, 1, 2, naxes, fpixel, lpixel, inc, 0.0, result, &anynull);
	/* With increment 2, reading [1,3,5] x [1,3,5]. */
	/* Indices: (0,0)=0, (2,0)=2, (4,0)=4, (0,2)=12, ... */
	fail_if(result[0] != 0.0);   /* (1,1) -> index 0. */
	fail_if(result[1] != 2.0);   /* (3,1) -> index 2. */
	fail_if(result[2] != 4.0);   /* (5,1) -> index 4. */
	fail_if(result[3] != 12.0);  /* (1,3) -> index 12. */
	call_01(ffclos, f);
}

static void
test_read_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double data[] = { -1.0e100, 0.0, 1.0e100 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != -1.0e100);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 1.0e100);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double data[] = { 10.0, 20.0, 30.0 };
	double result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfd, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 10.0);
	fail_if(result[1] != 20.0);
	fail_if(result[2] != 30.0);
	call_01(ffclos, f);
}

static void
test_read_vector_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLVEC" };
	char *tform[] = { "5D" };  /* 5 doubles per row. */
	double data[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 5, 0.0, result, &anynull);
	fail_if(result[0] != 1.1);
	fail_if(result[4] != 5.5);
	call_01(ffclos, f);
}

static void
test_read_from_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 0, 127, 255 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 127.0);
	fail_if(result[2] != 255.0);
	call_01(ffclos, f);
}

static void
test_read_from_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short data[] = { -32768, 0, 32767 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != -32768.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 32767.0);
	call_01(ffclos, f);
}

static void
test_read_from_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	long data[] = { -1000000, 0, 1000000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != -1000000.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 1000000.0);
	call_01(ffclos, f);
}

static void
test_read_from_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { -1.23e20f, 0.0f, 1.23e20f };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	/* Float to double conversion preserves the float value. */
	fail_if(fabs(result[0] - (double)data[0]) > 1.0e10);
	fail_if(result[1] != 0.0);
	fail_if(fabs(result[2] - (double)data[2]) > 1.0e10);
	call_01(ffclos, f);
}

static void
test_read_from_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { -1000000000LL, 0, 1000000000LL };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != -1000000000.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 1000000000.0);
	call_01(ffclos, f);
}

static void
test_read_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1D" };
	double data[] = { 50.0 };
	double result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 1, data);
	/* Set scaling: result = data * 2.0 + 10.0 = 110.0. */
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 1, 0.0, result, &anynull);
	fail_if(result[0] != 110.0);
	call_01(ffclos, f);
}

static void
test_read_bad_col_num(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvd(f, 0, 1, 1, 1, 0.0, result, &anynull, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "D25.17" };  /* ASCII format for double. */
	double data[] = { -1.23456789012345e10, 0.0, 1.23456789012345e10 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(fabs(result[0] - data[0]) > 1.0);
	fail_if(result[1] != 0.0);
	fail_if(fabs(result[2] - data[2]) > 1.0);
	call_01(ffclos, f);
}

static void
test_read_complex_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CMPLXCOL" };
	char *tform[] = { "1M" };  /* Double precision complex. */
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };  /* 3 complex nums. */
	double result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclm, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvm, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[1] != 2.0);
	fail_if(result[4] != 5.0);
	fail_if(result[5] != 6.0);
	call_01(ffclos, f);
}

static void
test_read_complex_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CMPLXCOL" };
	char *tform[] = { "1M" };  /* Double precision complex. */
	double data[] = { 1.0, 2.0, 3.0, 4.0 };  /* 2 complex nums. */
	double result[4];
	char nularray[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclm, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfm, f, 1, 1, 1, 2, result, nularray, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[1] != 2.0);
	fail_if(result[2] != 3.0);
	fail_if(result[3] != 4.0);
	call_01(ffclos, f);
}

static void
test_read_from_integer_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	long data[] = { -1000, -1, 0, 1, 1000 };
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 5, 0.0, result, &anynull);
	fail_if(result[0] != -1000.0);
	fail_if(result[2] != 0.0);
	fail_if(result[4] != 1000.0);
	call_01(ffclos, f);
}

static void
test_read_from_short_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { -32768, -100, 0, 100, 32767 };
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 5, 0.0, result, &anynull);
	fail_if(result[0] != -32768.0);
	fail_if(result[2] != 0.0);
	fail_if(result[4] != 32767.0);
	call_01(ffclos, f);
}

static void
test_read_from_float_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { -1.5e30f, -1.0f, 0.0f, 1.0f, 1.5e30f };
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 5, 0.0, result, &anynull);
	fail_if(fabs(result[0] - (double)data[0]) > 1.0e20);
	fail_if(result[2] != 0.0);
	fail_if(fabs(result[4] - (double)data[4]) > 1.0e20);
	call_01(ffclos, f);
}

static void
test_read_with_image_scaling(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	long data[] = { 10, 20, 30 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 3, data);
	/* Set scaling: result = data * 0.5 + 100.0. */
	call_05(ffpkyd, f, "BSCALE", 0.5, 15, NULL);
	call_05(ffpkyd, f, "BZERO", 100.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 105.0);  /* 10 * 0.5 + 100. */
	fail_if(result[1] != 110.0);  /* 20 * 0.5 + 100. */
	fail_if(result[2] != 115.0);  /* 30 * 0.5 + 100. */
	call_01(ffclos, f);
}

static void
test_read_group_parameter(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	double pdata[] = { 1.1, 2.2 };
	double result[2];

	/* Create random groups with 2 parameters. */
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, DOUBLE_IMG, 1, naxes, 2, 1, 1);
	call_05(ffpgpd, f, 1, 1, 2, pdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpd, f, 1, 1, 2, result);
	fail_if(result[0] != 1.1);
	fail_if(result[1] != 2.2);
	call_01(ffclos, f);
}

static void
test_read_multiple_rows(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double result[5];
	int anynull;
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	for (i = 0; i < 5; i += 1) {
		call_06(ffpcld, f, 1, i + 1, 1, 1, &data[i]);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 5, 0.0, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[2] != 3.0);
	fail_if(result[4] != 5.0);
	call_01(ffclos, f);
}

static void
test_read_large_values(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	double data[] = { DBL_MIN, 0.0, DBL_MAX };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != DBL_MIN);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != DBL_MAX);
	call_01(ffclos, f);
}

static void
test_read_special_values(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2 };
	double data[] = { HUGE_VAL, -HUGE_VAL };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 2, 0.0, result, &anynull);
	/* HUGE_VAL typically stored as infinity. */
	fail_if(result[0] != HUGE_VAL);
	fail_if(result[1] != -HUGE_VAL);
	call_01(ffclos, f);
}

static void
test_read_byte_image_as_double(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned char data[] = { 0, 128, 255 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 128.0);
	fail_if(result[2] != 255.0);
	call_01(ffclos, f);
}

static void
test_read_longlong_image_as_double(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	LONGLONG data[] = { -1000000000000LL, 0, 1000000000000LL };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != -1000000000000.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 1000000000000.0);
	call_01(ffclos, f);
}

static void
test_read_bad_dimension(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	double data[100];
	double result[4];
	int anynull;
	int i;

	for (i = 0; i < 100; i += 1) {
		data[i] = (double)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_05(ffpprd, f, 1, 1, 100, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Try to read with ncols < naxis1 - should fail with BAD_DIMEN. */
	ffg3dd(f, 1, 0.0, 5, 5, 10, 10, 1, result, &anynull, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_reverse_order(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5, 5 };
	double data[25];
	double result[9];
	long fpixel[] = { 4, 4 };  /* Start at (4,4). */
	long lpixel[] = { 2, 2 };  /* End at (2,2) - reverse! */
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 25; i += 1) {
		data[i] = (double)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_05(ffpprd, f, 1, 1, 25, data);
	call_01(ffclos, f);

	/* Reading reverse order on image - exercises dir=-1 code path.   */
	/* The result is more complex - just verify it works without error. */
	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvd, f, 1, 2, naxes, fpixel, lpixel, inc, 0.0, result, &anynull);
	/* Just verify no error and we got some data. */
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_table_subsection(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECTOR" };
	char *tform[] = { "6D" };  /* 6 doubles per row. */
	double data1[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
	double data2[] = { 10.0, 20.0, 30.0, 40.0, 50.0, 60.0 };
	double data3[] = { 100.0, 200.0, 300.0, 400.0, 500.0, 600.0 };
	/* 2 elements from each of 3 rows. */
	double result[6];
	long naxes[] = { 6 };
	long blc[] = { 2, 1 };  /* Start at element 2, row 1. */
	long trc[] = { 3, 3 };  /* End at element 3, row 3. */
	long inc[] = { 1, 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 6, data1);
	call_06(ffpcld, f, 1, 2, 1, 6, data2);
	call_06(ffpcld, f, 1, 3, 1, 6, data3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Read subsection from table: elements 2-3 from rows 1-3. */
	call_10(ffgsvd, f, 1, 1, naxes, blc, trc, inc, 0.0, result, &anynull);
	/* Row 1: elem 2,3 = 2.0, 3.0. Row 2: 20.0, 30.0. Row 3: 200.0, 300.0. */
	fail_if(result[0] != 2.0);
	fail_if(result[1] != 3.0);
	fail_if(result[2] != 20.0);
	fail_if(result[3] != 30.0);
	fail_if(result[4] != 200.0);
	fail_if(result[5] != 300.0);
	call_01(ffclos, f);
}

static void
test_read_unsigned_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULONGCOL" };
	char *tform[] = { "1K" };  /* Longlong column. */
	LONGLONG data[] = { 0, 1, 9223372036854775807LL };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	/* Set TZERO to 9223372036854775808 to treat as unsigned. */
	call_05(ffpkyd, f, "TZERO1", 9223372036854775808., 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	/* With TZERO=9223372036854775808:                                   */
	/* 0 -> 9223372036854775808, 1 -> 9223372036854775809, etc.          */
	/* But stored internally as signed, so reading uses XOR.             */
	fail_if(result[0] < 9.0e18);
	fail_if(result[1] < 9.0e18);
	fail_if(result[2] < 1.8e19);
	call_01(ffclos, f);
}

static void
test_read_byte_with_null_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 0, 50, 100, 200, 255 };
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 5, data);
	/* Set TNULL, TSCAL, TZERO. */
	call_04(ffpkyj, f, "TNULL1", 255, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 5, -999.0, result, &anynull);
	/* Values: 0*2+100=100, 50*2+100=200, 100*2+100=300, 200*2+100=500, 255=null=-999. */
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	fail_if(result[3] != 500.0);
	fail_if(result[4] != -999.0);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_short_with_null_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short data[] = { -100, 0, 100, -32768 };
	double result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 4, data);
	/* Set TNULL, TSCAL, TZERO. */
	call_04(ffpkyj, f, "TNULL1", -32768, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 4, -999.0, result, &anynull);
	/* Values: -100*0.5+50=0, 0*0.5+50=50, 100*0.5+50=100, -32768=null=-999. */
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 50.0);
	fail_if(result[2] != 100.0);
	fail_if(result[3] != -999.0);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_int_with_null_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300, -2147483647 };
	double result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 4, data);
	/* Set TNULL, TSCAL, TZERO. */
	call_04(ffpkyj, f, "TNULL1", -2147483647, NULL);
	call_05(ffpkyd, f, "TSCAL1", 3.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 1000.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 4, -999.0, result, &anynull);
	/* Values: 100*3+1000=1300, 200*3+1000=1600, 300*3+1000=1900, null=-999. */
	fail_if(result[0] != 1300.0);
	fail_if(result[1] != 1600.0);
	fail_if(result[2] != 1900.0);
	fail_if(result[3] != -999.0);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_longlong_with_null_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 1000, 2000, 3000, -9223372036854775807LL };
	double result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 4, data);
	/* Set TNULL, TSCAL, TZERO. */
	call_04(ffpkyj, f, "TNULL1", -9223372036854775807LL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 5000.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 4, -999.0, result, &anynull);
	/* Values: 1000*2+5000=7000, 2000*2+5000=9000, 3000*2+5000=11000, null=-999. */
	fail_if(result[0] != 7000.0);
	fail_if(result[1] != 9000.0);
	fail_if(result[2] != 11000.0);
	fail_if(result[3] != -999.0);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_float_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { 1.0f, 2.0f, 3.0f };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	/* Set TSCAL, TZERO. */
	call_05(ffpkyd, f, "TSCAL1", 10.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	/* Values: 1*10+100=110, 2*10+100=120, 3*10+100=130. */
	fail_if(fabs(result[0] - 110.0) > 0.001);
	fail_if(fabs(result[1] - 120.0) > 0.001);
	fail_if(fabs(result[2] - 130.0) > 0.001);
	call_01(ffclos, f);
}

static void
test_read_double_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	double data[] = { 10.0, 20.0, 30.0 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 3, data);
	/* Set BSCALE, BZERO. */
	call_05(ffpkyd, f, "BSCALE", 0.1, 15, NULL);
	call_05(ffpkyd, f, "BZERO", 1000.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvd, f, 1, 1, 3, 0.0, result, &anynull);
	/* Values: 10*0.1+1000=1001, 20*0.1+1000=1002, 30*0.1+1000=1003. */
	fail_if(fabs(result[0] - 1001.0) > 0.001);
	fail_if(fabs(result[1] - 1002.0) > 0.001);
	fail_if(fabs(result[2] - 1003.0) > 0.001);
	call_01(ffclos, f);
}

static void
test_read_byte_with_null_noscaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 10, 20, 30, 255 };
	double result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 4, data);
	call_04(ffpkyj, f, "TNULL1", 255, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfd, f, 1, 1, 1, 4, result, nularray, &anynull);
	fail_if(result[0] != 10.0);
	fail_if(result[1] != 20.0);
	fail_if(result[2] != 30.0);
	fail_if(nularray[3] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_short_with_null_noscaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short data[] = { 100, 200, 300, -32768 };
	double result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 4, data);
	call_04(ffpkyj, f, "TNULL1", -32768, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfd, f, 1, 1, 1, 4, result, nularray, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	fail_if(nularray[3] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_int_with_null_noscaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1000, 2000, 3000, -2147483647 };
	double result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 4, data);
	call_04(ffpkyj, f, "TNULL1", -2147483647, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfd, f, 1, 1, 1, 4, result, nularray, &anynull);
	fail_if(result[0] != 1000.0);
	fail_if(result[1] != 2000.0);
	fail_if(result[2] != 3000.0);
	fail_if(nularray[3] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_longlong_with_null_noscaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 10000, 20000, 30000, -9223372036854775807LL };
	double result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 4, data);
	call_04(ffpkyj, f, "TNULL1", -9223372036854775807LL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfd, f, 1, 1, 1, 4, result, nularray, &anynull);
	fail_if(result[0] != 10000.0);
	fail_if(result[1] != 20000.0);
	fail_if(result[2] != 30000.0);
	fail_if(nularray[3] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_unsigned_longlong_with_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULONGCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 0, 1, -1 };  /* -1 will be null. */
	double result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	/* Set TZERO for unsigned and TNULL. */
	call_05(ffpkyd, f, "TZERO1", 9223372036854775808., 15, NULL);
	call_04(ffpkyj, f, "TNULL1", -1, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfd, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(nularray[2] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_naxis_boundary(void)
{
	fitsfile *f;
	int status = 0;
	double result[1];
	long naxes[] = { 1 };
	long blc[] = { 1 };
	long trc[] = { 1 };
	long inc[] = { 1 };
	int anynull;

	/* Test with naxis out of range. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* naxis = 0 should fail. */
	ffgsvd(f, 1, 0, naxes, blc, trc, inc, 0.0, result, &anynull, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	/* naxis = 10 should fail. */
	ffgsvd(f, 1, 10, naxes, blc, trc, inc, 0.0, result, &anynull, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_ascii_with_exponent(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "E15.7" };  /* ASCII exponential format. */
	double data[] = { 1.2345678e10, -9.87654e-5 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 2, 0.0, result, &anynull);
	fail_if(fabs(result[0] - data[0]) / data[0] > 1e-6);
	fail_if(fabs(result[1] - data[1]) / fabs(data[1]) > 1e-4);
	call_01(ffclos, f);
}

static void
test_read_ascii_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "F10.2" };  /* ASCII fixed format. */
	double data[] = { 100.0, 200.0, 300.0 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	/* Set TSCAL, TZERO. */
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	/* Values are scaled: (read_value) * 2 + 50. */
	/* But the data is written raw, so result = data * TSCAL + TZERO. */
	fail_if(fabs(result[0] - 250.0) > 0.01);  /* 100 * 2 + 50 = 250. */
	fail_if(fabs(result[1] - 450.0) > 0.01);  /* 200 * 2 + 50 = 450. */
	fail_if(fabs(result[2] - 650.0) > 0.01);  /* 300 * 2 + 50 = 650. */
	call_01(ffclos, f);
}

static void
test_read_single_element_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALAR" };
	char *tform[] = { "1D" };  /* Single element per row. */
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double result[5];
	long naxes[] = { 1 };
	long blc[] = { 1, 1 };
	long trc[] = { 1, 5 };
	long inc[] = { 1, 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Read all 5 rows of single-element column. */
	call_10(ffgsvd, f, 1, 1, naxes, blc, trc, inc, 0.0, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[2] != 3.0);
	fail_if(result[4] != 5.0);
	call_01(ffclos, f);
}

static void
test_read_negative_elemincre(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	double data[] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
	double result[5];
	long fpixel[] = { 10 };
	long lpixel[] = { 1 };
	long inc[] = { 2 };  /* Skip every other. */
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 10, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Read from 10 to 1 with step 2 - should read 9,7,5,3,1. */
	call_10(ffgsvd, f, 1, 1, naxes, fpixel, lpixel, inc, 0.0, result, &anynull);
	fail_if(result[0] != 9.0);
	fail_if(result[1] != 7.0);
	fail_if(result[2] != 5.0);
	fail_if(result[3] != 3.0);
	fail_if(result[4] != 1.0);
	call_01(ffclos, f);
}

static void
test_read_colnum_zero(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double result[5];
	long blc[] = { 1 };
	long trc[] = { 5 };
	long inc[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* colnum=0 on IMAGE_HDU means use default row 1. */
	call_10(ffgsvd, f, 0, 1, naxes, blc, trc, inc, 0.0, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[4] != 5.0);
	call_01(ffclos, f);
}

static void
test_read_3d_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3, 2 };  /* 3x3x2 = 18 elements. */
	double data[18];
	double result[32];  /* 4x4x2 = 32, larger than FITS image. */
	int anynull;
	int i;

	for (i = 0; i < 18; i += 1) {
		data[i] = (double)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 3, naxes);
	call_05(ffpprd, f, 1, 1, 18, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Read into array larger than FITS image: ncols=4 > naxis1=3. */
	call_10(ffg3dd, f, 1, 0.0, 4, 4, 3, 3, 2, result, &anynull);
	/* Data should be at start of each row. */
	fail_if(result[0] != 1.0);   /* First element. */
	fail_if(result[1] != 2.0);
	fail_if(result[2] != 3.0);
	/* result[3] is padding (beyond naxis1=3). */
	fail_if(result[4] != 4.0);   /* Second row of first plane. */
	call_01(ffclos, f);
}

static void
test_read_table_bad_range(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECTOR" };
	char *tform[] = { "5D" };
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double result[1];
	long naxes[] = { 5 };
	long blc[] = { 3, 1 };  /* Start at elem 3. */
	long trc[] = { 1, 1 };  /* End at elem 1 - reverse! */
	long inc[] = { 1, 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* On tables, reverse range should fail with BAD_PIX_NUM. */
	ffgsvd(f, 1, 1, naxes, blc, trc, inc, 0.0, result, &anynull, &status);
	fail_if(status != BAD_PIX_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_table_bad_range_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECTOR" };
	char *tform[] = { "5D" };
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double result[1];
	char nularray[1];
	long naxes[] = { 5 };
	long blc[] = { 3, 1 };  /* Start at elem 3. */
	long trc[] = { 1, 1 };  /* End at elem 1 - reverse! */
	long inc[] = { 1, 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* On tables, reverse range should fail with BAD_PIX_NUM. */
	ffgsfd(f, 1, 1, naxes, blc, trc, inc, result, nularray, &anynull, &status);
	fail_if(status != BAD_PIX_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_subsection_flags_naxis_boundary(void)
{
	fitsfile *f;
	int status = 0;
	double result[1];
	char nularray[1];
	long naxes[] = { 1 };
	long blc[] = { 1 };
	long trc[] = { 1 };
	long inc[] = { 1 };
	int anynull;

	/* Test with naxis out of range for ffgsfd. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* naxis = 0 should fail. */
	ffgsfd(f, 1, 0, naxes, blc, trc, inc, result, nularray, &anynull, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	/* naxis = 10 should fail. */
	ffgsfd(f, 1, 10, naxes, blc, trc, inc, result, nularray, &anynull, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

int
main(void)
{
	test_read_primary_array();
	test_read_primary_with_null_value();
	test_read_primary_with_null_flags();
	test_read_2d_array();
	test_read_3d_array();
	test_read_subsection();
	test_read_subsection_with_null_flags();
	test_read_subsection_with_increment();
	test_read_column();
	test_read_column_with_null_flags();
	test_read_vector_column();
	test_read_from_byte_column();
	test_read_from_short_column();
	test_read_from_long_column();
	test_read_from_float_column();
	test_read_from_longlong_column();
	test_read_with_scaling();
	test_read_bad_col_num();
	test_read_ascii_table();
	test_read_complex_column();
	test_read_complex_column_with_null_flags();
	test_read_from_integer_image();
	test_read_from_short_image();
	test_read_from_float_image();
	test_read_with_image_scaling();
	test_read_group_parameter();
	test_read_multiple_rows();
	test_read_large_values();
	test_read_special_values();
	test_read_byte_image_as_double();
	test_read_longlong_image_as_double();
	test_read_bad_dimension();
	test_read_reverse_order();
	test_read_table_subsection();
	test_read_unsigned_longlong();
	test_read_byte_with_null_scaling();
	test_read_short_with_null_scaling();
	test_read_int_with_null_scaling();
	test_read_longlong_with_null_scaling();
	test_read_float_with_scaling();
	test_read_double_with_scaling();
	test_read_byte_with_null_noscaling();
	test_read_short_with_null_noscaling();
	test_read_int_with_null_noscaling();
	test_read_longlong_with_null_noscaling();
	test_read_unsigned_longlong_with_null();
	test_read_naxis_boundary();
	test_read_ascii_with_exponent();
	test_read_ascii_with_scaling();
	test_read_single_element_column();
	test_read_negative_elemincre();
	test_read_colnum_zero();
	test_read_3d_noncontiguous();
	test_read_table_bad_range();
	test_read_table_bad_range_flags();
	test_read_subsection_flags_naxis_boundary();
	return 0;
}
