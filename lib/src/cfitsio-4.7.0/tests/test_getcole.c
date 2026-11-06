/*
 * Tests for getcole.c - float precision read functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcole.fits"

static void
test_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { -1.0e30f, -1.0f, 0.0f, 1.0f, 1.0e30f };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 5, 0.0f, result, &anynull);
	fail_if(result[0] != data[0]);
	fail_if(result[2] != 0.0f);
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
	float data[] = { 1.0f, 2.0f, -999.0f, 4.0f, 5.0f };
	float result[5];
	int anynull;

	/* For floating point images, IEEE NaN is the null value. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Read with null substitution value. */
	call_07(ffgpve, f, 1, 1, 5, -123.0f, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[2] != -999.0f);  /* No BLANK in floating point. */
	call_01(ffclos, f);
}

static void
test_read_primary_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };
	float result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfe, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10.0f);
	fail_if(result[4] != 50.0f);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 3 };  /* 4 cols x 3 rows. */
	float data[12];
	float result[12];
	int anynull;
	int i;

	for (i = 0; i < 12; i += 1) {
		data[i] = (float)i * 10.5f;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_05(ffppre, f, 1, 1, 12, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2de, f, 1, 0.0f, 4, 4, 3, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[5] != 5 * 10.5f);
	fail_if(result[11] != 11 * 10.5f);
	call_01(ffclos, f);
}

static void
test_read_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };  /* 2x2x2 = 8 elements. */
	float data[8];
	float result[8];
	int anynull;
	int i;

	for (i = 0; i < 8; i += 1) {
		data[i] = (float)(i + 1) * 0.5f;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 3, naxes);
	call_05(ffppre, f, 1, 1, 8, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3de, f, 1, 0.0f, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != 0.5f);
	fail_if(result[7] != 4.0f);
	call_01(ffclos, f);
}

static void
test_read_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	float data[16];
	float result[4];
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (float)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_05(ffppre, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsve, f, 1, 2, naxes, fpixel, lpixel, inc, 0.0f, result, &anynull);
	fail_if(result[0] != 5.0f);
	fail_if(result[1] != 6.0f);
	fail_if(result[2] != 9.0f);
	fail_if(result[3] != 10.0f);
	call_01(ffclos, f);
}

static void
test_read_subsection_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	float data[16];
	float result[4];
	char nularray[4];
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 2, 2 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (float)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_05(ffppre, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfe, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 1.0f);
	fail_if(result[2] != 4.0f);
	fail_if(result[3] != 5.0f);
	call_01(ffclos, f);
}

static void
test_read_subsection_with_increment(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 6, 6 };
	float data[36];
	float result[4];
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 5, 5 };
	long inc[] = { 2, 2 };  /* Skip every other pixel. */
	int anynull;
	int i;

	for (i = 0; i < 36; i += 1) {
		data[i] = (float)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_05(ffppre, f, 1, 1, 36, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsve, f, 1, 2, naxes, fpixel, lpixel, inc, 0.0f, result, &anynull);
	/* With increment 2, reading [1,3,5] x [1,3,5]. */
	fail_if(result[0] != 0.0f);   /* (1,1) -> index 0. */
	fail_if(result[1] != 2.0f);   /* (3,1) -> index 2. */
	fail_if(result[2] != 4.0f);   /* (5,1) -> index 4. */
	fail_if(result[3] != 12.0f);  /* (1,3) -> index 12. */
	call_01(ffclos, f);
}

static void
test_read_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTCOL" };
	char *tform[] = { "1E" };
	float data[] = { -1.0e30f, 0.0f, 1.0e30f };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != -1.0e30f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 1.0e30f);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTCOL" };
	char *tform[] = { "1E" };
	float data[] = { 10.0f, 20.0f, 30.0f };
	float result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfe, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 10.0f);
	fail_if(result[1] != 20.0f);
	fail_if(result[2] != 30.0f);
	call_01(ffclos, f);
}

static void
test_read_vector_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTVEC" };
	char *tform[] = { "5E" };  /* 5 floats per row. */
	float data[] = { 1.1f, 2.2f, 3.3f, 4.4f, 5.5f };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 5, 0.0f, result, &anynull);
	fail_if(result[0] != 1.1f);
	fail_if(result[4] != 5.5f);
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
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 127.0f);
	fail_if(result[2] != 255.0f);
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
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != -32768.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 32767.0f);
	call_01(ffclos, f);
}

static void
test_read_from_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	long data[] = { -100000, 0, 100000 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != -100000.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 100000.0f);
	call_01(ffclos, f);
}

static void
test_read_from_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double data[] = { -1.5e10, 0.0, 1.5e10 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	/* Double to float truncates but preserves approximate value. */
	fail_if(fabsf(result[0] - (float)data[0]) > 1.0e5f);
	fail_if(result[1] != 0.0f);
	fail_if(fabsf(result[2] - (float)data[2]) > 1.0e5f);
	call_01(ffclos, f);
}

static void
test_read_from_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { -1000000LL, 0, 1000000LL };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != -1000000.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 1000000.0f);
	call_01(ffclos, f);
}

static void
test_read_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1E" };
	float data[] = { 50.0f };
	float result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 1, data);
	/* Set scaling: result = data * 2.0 + 10.0 = 110.0. */
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 1, 0.0f, result, &anynull);
	fail_if(result[0] != 110.0f);
	call_01(ffclos, f);
}

static void
test_read_bad_col_num(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTCOL" };
	char *tform[] = { "1E" };
	float result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcve(f, 0, 1, 1, 1, 0.0f, result, &anynull, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTCOL" };
	char *tform[] = { "E15.7" };  /* ASCII format for float. */
	float data[] = { -1.234567e5f, 0.0f, 1.234567e5f };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(fabsf(result[0] - data[0]) > 1.0f);
	fail_if(result[1] != 0.0f);
	fail_if(fabsf(result[2] - data[2]) > 1.0f);
	call_01(ffclos, f);
}

static void
test_read_complex_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CMPLXCOL" };
	char *tform[] = { "1C" };  /* Single precision complex. */
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };  /* 3 complex. */
	float result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclc, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvc, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[1] != 2.0f);
	fail_if(result[4] != 5.0f);
	fail_if(result[5] != 6.0f);
	call_01(ffclos, f);
}

static void
test_read_complex_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CMPLXCOL" };
	char *tform[] = { "1C" };  /* Single precision complex. */
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };  /* 2 complex nums. */
	float result[4];
	char nularray[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclc, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfc, f, 1, 1, 1, 2, result, nularray, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[1] != 2.0f);
	fail_if(result[2] != 3.0f);
	fail_if(result[3] != 4.0f);
	call_01(ffclos, f);
}

static void
test_read_from_integer_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	long data[] = { -1000, -1, 0, 1, 1000 };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 5, 0.0f, result, &anynull);
	fail_if(result[0] != -1000.0f);
	fail_if(result[2] != 0.0f);
	fail_if(result[4] != 1000.0f);
	call_01(ffclos, f);
}

static void
test_read_from_short_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { -32768, -100, 0, 100, 32767 };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 5, 0.0f, result, &anynull);
	fail_if(result[0] != -32768.0f);
	fail_if(result[2] != 0.0f);
	fail_if(result[4] != 32767.0f);
	call_01(ffclos, f);
}

static void
test_read_from_double_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	double data[] = { -1.5e10, -1.0, 0.0, 1.0, 1.5e10 };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 5, 0.0f, result, &anynull);
	/* Double to float may lose precision but maintains approximate value. */
	fail_if(fabsf(result[0] - (float)data[0]) > 1.0e5f);
	fail_if(result[2] != 0.0f);
	fail_if(fabsf(result[4] - (float)data[4]) > 1.0e5f);
	call_01(ffclos, f);
}

static void
test_read_with_image_scaling(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	long data[] = { 10, 20, 30 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 3, data);
	/* Set scaling: result = data * 0.5 + 100.0. */
	call_05(ffpkyd, f, "BSCALE", 0.5, 15, NULL);
	call_05(ffpkyd, f, "BZERO", 100.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 105.0f);  /* 10 * 0.5 + 100. */
	fail_if(result[1] != 110.0f);  /* 20 * 0.5 + 100. */
	fail_if(result[2] != 115.0f);  /* 30 * 0.5 + 100. */
	call_01(ffclos, f);
}

static void
test_read_group_parameter(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	float pdata[] = { 1.1f, 2.2f };
	float result[2];

	/* Create random groups with 2 parameters. */
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, FLOAT_IMG, 1, naxes, 2, 1, 1);
	call_05(ffpgpe, f, 1, 1, 2, pdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpe, f, 1, 1, 2, result);
	fail_if(result[0] != 1.1f);
	fail_if(result[1] != 2.2f);
	call_01(ffclos, f);
}

static void
test_read_multiple_rows(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTCOL" };
	char *tform[] = { "1E" };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	float result[5];
	int anynull;
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	for (i = 0; i < 5; i += 1) {
		call_06(ffpcle, f, 1, i + 1, 1, 1, &data[i]);
	}
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 5, 0.0f, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[2] != 3.0f);
	fail_if(result[4] != 5.0f);
	call_01(ffclos, f);
}

static void
test_read_large_values(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	float data[] = { FLT_MIN, 0.0f, FLT_MAX };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != FLT_MIN);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != FLT_MAX);
	call_01(ffclos, f);
}

static void
test_read_byte_image_as_float(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned char data[] = { 0, 128, 255 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 128.0f);
	fail_if(result[2] != 255.0f);
	call_01(ffclos, f);
}

static void
test_read_longlong_image_as_float(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	LONGLONG data[] = { -1000000LL, 0, 1000000LL };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != -1000000.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 1000000.0f);
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
	test_read_from_double_column();
	test_read_from_longlong_column();
	test_read_with_scaling();
	test_read_bad_col_num();
	test_read_ascii_table();
	test_read_complex_column();
	test_read_complex_column_with_null_flags();
	test_read_from_integer_image();
	test_read_from_short_image();
	test_read_from_double_image();
	test_read_with_image_scaling();
	test_read_group_parameter();
	test_read_multiple_rows();
	test_read_large_values();
	test_read_byte_image_as_float();
	test_read_longlong_image_as_float();
	return 0;
}
