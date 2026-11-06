/*
 * Tests for getcoli.c signed short read functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcoli.fits"

static void
test_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { SHRT_MIN, -100, 0, 100, SHRT_MAX };
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvi, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != SHRT_MIN);
	fail_if(result[1] != -100);
	fail_if(result[2] != 0);
	fail_if(result[3] != 100);
	fail_if(result[4] != SHRT_MAX);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_primary_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { 10, 20, 30, 40, 50 };
	short result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 5, data);
	call_04(ffpkyj, f, "BLANK", 30, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfi, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(nularray[2] != 1);  /* Value 30 = BLANK. */
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 3 };  /* 4 cols x 3 rows. */
	short data[12];
	short result[12];
	int anynull;
	int i;

	for (i = 0; i < 12; i += 1) {
		data[i] = (short)(i * 100);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_05(ffppri, f, 1, 1, 12, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2di, f, 1, 0, 4, 4, 3, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[5] != 500);
	fail_if(result[11] != 1100);
	call_01(ffclos, f);
}

static void
test_read_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };  /* 2x2x2 = 8 elements. */
	short data[8];
	short result[8];
	int anynull;
	int i;

	for (i = 0; i < 8; i += 1) {
		data[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 3, naxes);
	call_05(ffppri, f, 1, 1, 8, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3di, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[7] != 8);
	call_01(ffclos, f);
}

static void
test_read_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	short data[16];
	short result[4];
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (short)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_05(ffppri, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvi, f, 1, 2, naxes, fpixel, lpixel, inc, 0, result, &anynull);
	fail_if(result[0] != 5);
	fail_if(result[1] != 6);
	fail_if(result[2] != 9);
	fail_if(result[3] != 10);
	call_01(ffclos, f);
}

static void
test_read_subsection_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	short data[16];
	short result[4];
	char nularray[4];
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 2, 2 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (short)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_05(ffppri, f, 1, 1, 16, data);
	call_04(ffpkyj, f, "BLANK", 0, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfi, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
	fail_if(nularray[0] != 1);  /* 0 = BLANK. */
	fail_if(result[1] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short data[] = { SHRT_MIN, 0, SHRT_MAX };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != SHRT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != SHRT_MAX);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short data[] = { 10, 20, 30 };
	short result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_04(ffpkyj, f, "TNULL1", 20, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfi, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(nularray[1] != 1);  /* 20 = TNULL1. */
	fail_if(result[2] != 30);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_vector_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTVEC" };
	char *tform[] = { "5I" };  /* 5 shorts per row. */
	short data[] = { 1, 2, 3, 4, 5 };
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
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
test_read_from_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	long data[] = { -1000, 0, 1000 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000);
	call_01(ffclos, f);
}

static void
test_read_from_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { -100.5f, 0.0f, 100.5f };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != 0);
	fail_if(result[2] != 100);
	call_01(ffclos, f);
}

static void
test_read_from_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double data[] = { -500.0, 0.0, 500.0 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -500);
	fail_if(result[1] != 0);
	fail_if(result[2] != 500);
	call_01(ffclos, f);
}

static void
test_read_from_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { -1000, 0, 1000 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000);
	call_01(ffclos, f);
}

static void
test_read_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1I" };
	short data[] = { 50 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	/* Set scaling: result = data * 2.0 + 10.0 = 110. */
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 110);
	call_01(ffclos, f);
}

static void
test_read_bad_col_num(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvi(f, 0, 1, 1, 1, 0, result, &anynull, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "I6" };
	short data[] = { -100, 0, 100 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != 0);
	fail_if(result[2] != 100);
	call_01(ffclos, f);
}

static void
test_read_with_null_value(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { 10, 20, -999, 40, 50 };
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 5, data);
	call_04(ffpkyj, f, "BLANK", -999, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Read with null substitution value of -1. */
	call_07(ffgpvi, f, 1, 1, 5, -1, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != -1);  /* Null replaced with -1. */
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

int
main(void)
{
	test_read_primary_array();
	test_read_primary_with_null_flags();
	test_read_2d_array();
	test_read_3d_array();
	test_read_subsection();
	test_read_subsection_with_null_flags();
	test_read_column();
	test_read_column_with_null_flags();
	test_read_vector_column();
	test_read_from_byte_column();
	test_read_from_long_column();
	test_read_from_float_column();
	test_read_from_double_column();
	test_read_from_longlong_column();
	test_read_with_scaling();
	test_read_bad_col_num();
	test_read_ascii_table();
	test_read_with_null_value();
	return 0;
}
