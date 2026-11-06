/*
 * Tests for putcolj.c long and longlong write functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcolj.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	long data[] = { INT_MIN, -100, 0, 100, INT_MAX };
	long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != INT_MIN);
	fail_if(result[1] != -100);
	fail_if(result[2] != 0);
	fail_if(result[3] != 100);
	fail_if(result[4] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_primary_with_null(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	long data[] = { 10, 20, 30, 40, 50 };
	long result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_04(ffpkyj, f, "BLANK", 30, NULL);  /* Define null value first. */
	/* Write with null value 30. */
	call_06(ffppnj, f, 1, 1, 5, data, 30);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfj, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(nularray[2] != 1);  /* 30 = null. */
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
	long data[] = { 1, 2, 3, 4, 5, 6 };
	long result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_06(ffp2dj, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2dj, f, 1, 0, 3, 3, 2, result, &anynull);
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
	long data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	long result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3dj, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dj, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
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
	long data[] = { 99, 99, 99, 99 };
	long result[16];
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	/* Initialize with zeros. */
	memset(result, 0, sizeof result);
	call_05(ffpprj, f, 1, 1, 16, result);
	/* Write subset. */
	call_07(ffpssj, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 16, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[5] != 99);  /* (2,2). */
	fail_if(result[6] != 99);  /* (3,2). */
	fail_if(result[9] != 99);  /* (2,3). */
	fail_if(result[10] != 99); /* (3,3). */
	fail_if(result[15] != 0);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	long pdata[] = { 10, 20 };
	long idata[] = { 1, 2, 3, 4 };
	long presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, LONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpj, f, 1, 1, 2, pdata);
	call_05(ffpprj, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpj, f, 1, 1, 2, presult);
	fail_if(presult[0] != 10);
	fail_if(presult[1] != 20);
	call_01(ffclos, f);
}

static void
test_write_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	long data[] = { INT_MIN, 0, INT_MAX };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != INT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_column_with_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	long data[] = { 10, 20, 30 };
	long result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(ffpkyj, f, "TNULL1", 20, NULL);
	/* Write with null value 20. */
	call_07(ffpcnj, f, 1, 1, 1, 3, data, 20);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfj, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(nularray[1] != 1);  /* 20 = null. */
	fail_if(result[2] != 30);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_vector_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGVEC" };
	char *tform[] = { "5J" };
	long data[] = { 1, 2, 3, 4, 5 };
	long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	long data[] = { 0, 127, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
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
test_write_to_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	long data[] = { SHRT_MIN, 0, SHRT_MAX };
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
	fail_if(result[0] != SHRT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != SHRT_MAX);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	long data[] = { -1000, 0, 1000 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 1000.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	long data[] = { -10000, 0, 10000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -10000.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 10000.0);
	call_01(ffclos, f);
}

static void
test_write_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "I11" };
	long data[] = { -100, 0, 100 };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != 0);
	fail_if(result[2] != 100);
	call_01(ffclos, f);
}

static void
test_write_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1J" };
	long data[] = { 110 };  /* Will be stored as (110 - 10) / 2 = 50. */
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 110);
	call_01(ffclos, f);
}

/* LONGLONG (jj suffix) tests. */

static void
test_write_longlong_primary(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	LONGLONG data[] = { -1000000000LL, 0, 1000000000LL };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000000000LL);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000000000LL);
	call_01(ffclos, f);
}

static void
test_write_longlong_with_null(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	LONGLONG data[] = { 10, 20, 30, 40, 50 };
	LONGLONG result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_04(ffpkyj, f, "BLANK", 30, NULL);  /* Define null value first. */
	call_06(ffppnjj, f, 1, 1, 5, data, 30);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfjj, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(nularray[2] != 1);
	fail_if(result[4] != 50);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_longlong_2d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 2 };
	LONGLONG data[] = { 1, 2, 3, 4, 5, 6 };
	LONGLONG result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_06(ffp2djj, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2djj, f, 1, 0, 3, 3, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[5] != 6);
	call_01(ffclos, f);
}

static void
test_write_longlong_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	LONGLONG data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	LONGLONG result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	call_08(ffp3djj, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3djj, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[7] != 8);
	call_01(ffclos, f);
}

static void
test_write_longlong_subset(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	LONGLONG data[] = { 99, 99, 99, 99 };
	LONGLONG result[16];
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	memset(result, 0, sizeof result);
	call_05(ffpprjj, f, 1, 1, 16, result);
	call_07(ffpssjj, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 16, 0, result, &anynull);
	fail_if(result[5] != 99);
	fail_if(result[10] != 99);
	call_01(ffclos, f);
}

static void
test_write_longlong_group_params(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	LONGLONG pdata[] = { 100, 200 };
	LONGLONG idata[] = { 1, 2, 3, 4 };
	LONGLONG presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, LONGLONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpjj, f, 1, 1, 2, pdata);
	call_05(ffpprjj, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpjj, f, 1, 1, 2, presult);
	fail_if(presult[0] != 100);
	fail_if(presult[1] != 200);
	call_01(ffclos, f);
}

static void
test_write_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { LLONG_MIN, 0, LLONG_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != LLONG_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != LLONG_MAX);
	call_01(ffclos, f);
}

static void
test_write_longlong_column_with_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 10, 20, 30 };
	LONGLONG result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(ffpkyj, f, "TNULL1", 20, NULL);
	call_07(ffpcnjj, f, 1, 1, 1, 3, data, 20);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfjj, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(nularray[1] != 1);
	fail_if(result[2] != 30);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_longlong_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	LONGLONG data[] = { 0, 128, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvb, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 128);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_longlong_to_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	LONGLONG data[] = { SHRT_MIN, 0, SHRT_MAX };
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
	fail_if(result[0] != SHRT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != SHRT_MAX);
	call_01(ffclos, f);
}

static void
test_write_longlong_to_int_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	LONGLONG data[] = { INT_MIN, 0, INT_MAX };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != INT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_longlong_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	LONGLONG data[] = { -1000000, 0, 1000000 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000000.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 1000000.0f);
	call_01(ffclos, f);
}

static void
test_write_longlong_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	LONGLONG data[] = { -10000000000LL, 0, 10000000000LL };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -10000000000.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 10000000000.0);
	call_01(ffclos, f);
}

static void
test_write_longlong_to_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "20A" };
	LONGLONG data[] = { -9876543210LL, 0, 9876543210LL };

	/* Writing LONGLONG to string column is not supported. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_bad_col_num(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);

	ffpclj(f, 0, 1, 1, 1, data, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGVEC" };
	char *tform[] = { "3J" };
	long data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	long result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 9, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
	fail_if(result[8] != 9);
	call_01(ffclos, f);
}

static void
test_write_3d_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	/* Array with ncols=3, nrows=3, naxis3=2 = 18 elements */
	long data[18];
	long result[8];
	int anynull, i;

	for (i = 0; i < 18; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	/* Non-contiguous: ncols=3 > naxis1=2, nrows=3 > naxis2=2 */
	call_08(ffp3dj, f, 1, 3, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 8, 0, result, &anynull);
	/* Result[0..1] = data[0..1] = 1,2 */
	/* Result[2..3] = data[3..4] = 4,5 */
	/* Result[4..5] = data[12..13] = 13,14 */
	/* Result[6..7] = data[15..16] = 16,17 */
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 4);
	fail_if(result[3] != 5);
	call_01(ffclos, f);
}

static void
test_write_3d_bad_dimen(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	long data[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	/* ncols < naxis1 should fail */
	ffp3dj(f, 1, 2, 2, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_subsection_bad_naxis(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long fpixel[] = { 1 };
	long lpixel[] = { 5 };
	long data[] = { 1, 2, 3, 4, 5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	/* naxis=10 but image only has 1 dimension */
	ffpssj(f, 1, 10, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	long data[] = { 100, 200, 300 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	fail_if(result[2] != 300);
	call_01(ffclos, f);
}

static void
test_byte_overflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* 200 * 2 = 400 > 255 = UCHAR_MAX */
	long data[] = { 100, 200, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_byte_underflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TZERO=50, dvalue = (input-50)/1. Input=10 -> dvalue = -40 < 0 */
	long data[] = { 100, 10, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_short_overflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* 20000 * 2 = 40000 > 32767 */
	long data[] = { 100, 20000, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_short_underflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* -20000 * 2 = -40000 < -32768 */
	long data[] = { 100, -20000, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_long_overflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	long data[] = { 100, LONG_MAX, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	/* LONG_MAX / 0.5 > INT32_MAX, should overflow */
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_long_underflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	long data[] = { 100, LONG_MIN, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	/* LONG_MIN / 0.5 < INT32_MIN, should underflow */
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_overflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	long data[] = { 100, LONG_MAX, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 1.0e-10, 15, NULL);
	/* LONG_MAX / 1e-10 > LONGLONG_MAX, should overflow */
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_underflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	long data[] = { 100, LONG_MIN, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 1.0e19, 15, NULL);
	/* LONG_MIN - 1e19 < LONGLONG_MIN, should underflow */
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_subsection_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	long fpixel[] = { 2, 2, 1 };
	long lpixel[] = { 3, 3, 2 };
	long data[8];
	long result[32];
	int anynull, i;

	for (i = 0; i < 8; i += 1) {
		data[i] = i + 100;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_07(ffpssj, f, 1, 3, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 32, 0, result, &anynull);
	fail_if(result[5] != 100);
	call_01(ffclos, f);
}

static void
test_4d_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3, 2, 2 };
	long fpixel[] = { 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2 };
	long data[16];
	long result[36];
	int anynull, i;

	for (i = 0; i < 16; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 4, naxes);
	call_07(ffpssj, f, 1, 4, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 36, 0, result, &anynull);
	fail_if(result[0] != 1);
	call_01(ffclos, f);
}

static void
test_5d_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2, 2, 2 };
	long fpixel[] = { 1, 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2, 2 };
	long data[32];
	long result[32];
	int anynull, i;

	for (i = 0; i < 32; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 5, naxes);
	call_07(ffpssj, f, 1, 5, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 32, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[31] != 32);
	call_01(ffclos, f);
}

static void
test_write_nulls_at_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VAL" };
	char *tform[] = { "1J" };
	long data[] = { 10, 20, -9999, -9999 };
	long result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -9999);
	call_07(ffpcnj, f, 1, 1, 1, 4, data, -9999);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 4, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != -9999);
	fail_if(result[3] != -9999);
	call_01(ffclos, f);
}

static void
test_overflow_in_null_write(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	long data[] = { 100, 300, -999 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 123);
	ffpcnj(f, 1, 1, 1, 3, data, -999, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_negative_dvalue_short(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	long data[] = { 0 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvi, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_negative_dvalue_int32(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	long data[] = { 0 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	long data[] = { 100, -1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 9223372036854775808.0, 20, NULL);
	ffpclj(f, 1, 1, 1, 2, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_negative_dvalue_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	long data[] = { 0 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvjj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_ascii_table_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "I3" };
	long data[] = { 10000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_table_overflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "I3" };
	long data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.0001, 15, NULL);
	ffpclj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_vla_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PJ" };
	long data[] = { 10, 20, 30 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -999);
	call_07(ffpcnj, f, 1, 1, 1, 3, data, -999);
	call_01(ffclos, f);
}

static void
test_vla_with_overflow_null(void)
{
	/* Test VLA null handling where ffpclj returns overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PI" };  /* VLA of shorts */
	/* 40000 overflows short, which triggers overflow handling in VLA path */
	long data[] = { 100, 40000, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -999);
	/* Write should succeed but with overflow cleared */
	call_07(ffpcnj, f, 1, 1, 1, 3, data, -999);
	call_01(ffclos, f);
}


/*
 * Test writing negative long to unscaled byte column triggers NUM_OVERFLOW.
 */
static void
test_long_to_byte_unscaled_negative(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	long data[] = { -5, 100, -10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing long > 255 to unscaled byte column triggers NUM_OVERFLOW.
 */
static void
test_long_to_byte_unscaled_positive_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	long data[] = { 100, 300, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing long < SHRT_MIN to unscaled short column triggers NUM_OVERFLOW.
 * Hits ffi4fi2 underflow path.
 */
static void
test_long_to_short_unscaled_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	long data[] = { 100, -40000, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing long > SHRT_MAX to unscaled short column triggers NUM_OVERFLOW.
 * Hits ffi4fi2 overflow path.
 */
static void
test_long_to_short_unscaled_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	long data[] = { 100, 40000, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing negative longlong to unscaled byte column triggers NUM_OVERFLOW.
 * Hits ffi8fi1 underflow path.
 */
static void
test_longlong_to_byte_unscaled_negative(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	LONGLONG data[] = { 100, -5, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing longlong > 255 to unscaled byte column triggers NUM_OVERFLOW.
 * Hits ffi8fi1 overflow path.
 */
static void
test_longlong_to_byte_unscaled_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	LONGLONG data[] = { 100, 300, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test writing longlong < SHRT_MIN to unscaled short column.
 * Hits ffi8fi2 underflow path.
 */
static void
test_longlong_to_short_unscaled_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	LONGLONG data[] = { 100, -40000, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


static void
test_write_to_ascii_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STR" };
	char *tform[] = { "A10" };
	long data[] = { 42 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_ATABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_binary_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STR" };
	char *tform[] = { "10A" };
	long data[] = { 42 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_float_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	long data[] = { 100, 200, 300 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 100.0f);
	fail_if(result[1] != 200.0f);
	fail_if(result[2] != 300.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	long data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_ascii_table_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "F10.2" };
	long data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

/* Tests for LONGLONG functions */

static void
test_longlong_3d_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	LONGLONG data[18];
	LONGLONG result[8];
	int anynull, i;

	for (i = 0; i < 18; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	/* Non-contiguous: ncols=3 > naxis1=2, nrows=3 > naxis2=2 */
	call_08(ffp3djj, f, 1, 3, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 8, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 4);
	fail_if(result[3] != 5);
	call_01(ffclos, f);
}

static void
test_longlong_3d_bad_dimen(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	LONGLONG data[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	/* ncols < naxis1 should fail */
	ffp3djj(f, 1, 2, 2, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_subsection_bad_naxis(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long fpixel[] = { 1 };
	long lpixel[] = { 5 };
	LONGLONG data[] = { 1, 2, 3, 4, 5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	/* naxis=10 but image only has 1 dimension */
	ffpssjj(f, 1, 10, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_byte_overflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* 200 * 2 = 400 > 255 */
	LONGLONG data[] = { 100, 200, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_byte_underflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TZERO=50, dvalue = (input-50)/1. Input=10 -> dvalue = -40 < 0 */
	LONGLONG data[] = { 100, 10, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_short_overflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* 20000 * 2 = 40000 > 32767 */
	LONGLONG data[] = { 100, 20000, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_short_underflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2 */
	/* -20000 * 2 = -40000 < -32768 */
	LONGLONG data[] = { 100, -20000, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_int32_overflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With TSCAL=0.5, dvalue = input/0.5 */
	/* 2e9 * 2 = 4e9 > INT32_MAX */
	LONGLONG data[] = { 100, 2000000000LL, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_int32_underflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With TSCAL=0.5, dvalue = input/0.5 */
	/* -2e9 * 2 = -4e9 < INT32_MIN */
	LONGLONG data[] = { 100, -2000000000LL, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_unsigned_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 100, -1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 9223372036854775808.0, 20, NULL);
	ffpcljj(f, 1, 1, 1, 2, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_int32_unscaled_overflow(void)
{
	/* LONGLONG > INT32_MAX to J column without scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	LONGLONG data[] = { 100, 3000000000LL, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* No scaling - scale=1, zero=0 by default */
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_int32_unscaled_underflow(void)
{
	/* LONGLONG < INT32_MIN to J column without scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	LONGLONG data[] = { 100, -3000000000LL, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* No scaling - scale=1, zero=0 by default */
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_negative_dvalue_short(void)
{
	/* Test negative dvalue rounding for LONGLONG to short */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	LONGLONG data[] = { 0 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 makes dvalue = (0-100)/1 = -100 */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvi, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_longlong_negative_dvalue_int32(void)
{
	/* Test negative dvalue rounding for LONGLONG to INT32 */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	LONGLONG data[] = { 0 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 makes dvalue = (0-100)/1 = -100 */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_longlong_to_longlong_scaled_overflow(void)
{
	/* Test LONGLONG to LONGLONG with scaling that causes overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 100, LLONG_MAX / 2, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TSCAL=0.25 makes dvalue = input/0.25 = input*4 */
	/* LLONG_MAX/2 * 4 > LLONG_MAX */
	call_05(ffpkyd, f, "TSCAL1", 0.25, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_longlong_scaled_underflow(void)
{
	/* Test LONGLONG to LONGLONG with scaling that causes underflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 100, LLONG_MIN / 2, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TSCAL=0.25 makes dvalue = input/0.25 = input*4 */
	/* LLONG_MIN/2 * 4 < LLONG_MIN */
	call_05(ffpkyd, f, "TSCAL1", 0.25, 15, NULL);
	ffpcljj(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_negative_dvalue_longlong(void)
{
	/* Test negative dvalue rounding for LONGLONG to LONGLONG */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 0 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 makes dvalue = (0-100)/1 = -100 */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvjj, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != -100);
	call_01(ffclos, f);
}

static void
test_longlong_subsection_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	long fpixel[] = { 2, 2, 1 };
	long lpixel[] = { 3, 3, 2 };
	LONGLONG data[8];
	LONGLONG result[32];
	int anynull, i;

	for (i = 0; i < 8; i += 1) {
		data[i] = i + 100;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	call_07(ffpssjj, f, 1, 3, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 32, 0, result, &anynull);
	fail_if(result[5] != 100);
	call_01(ffclos, f);
}

static void
test_longlong_4d_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3, 2, 2 };
	long fpixel[] = { 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2 };
	LONGLONG data[16];
	LONGLONG result[36];
	int anynull, i;

	for (i = 0; i < 16; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 4, naxes);
	call_07(ffpssjj, f, 1, 4, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 36, 0, result, &anynull);
	fail_if(result[0] != 1);
	call_01(ffclos, f);
}

static void
test_longlong_5d_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2, 2, 2 };
	long fpixel[] = { 1, 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2, 2 };
	LONGLONG data[32];
	LONGLONG result[32];
	int anynull, i;

	for (i = 0; i < 32; i += 1) {
		data[i] = i + 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 5, naxes);
	call_07(ffpssjj, f, 1, 5, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 32, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[31] != 32);
	call_01(ffclos, f);
}

static void
test_longlong_nulls_at_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VAL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 10, 20, -9999, -9999 };
	LONGLONG result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -9999);
	call_07(ffpcnjj, f, 1, 1, 1, 4, data, -9999);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 4, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != -9999);
	fail_if(result[3] != -9999);
	call_01(ffclos, f);
}

static void
test_longlong_vla_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PK" };
	LONGLONG data[] = { 10, 20, 30 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -999);
	call_07(ffpcnjj, f, 1, 1, 1, 3, data, -999);
	call_01(ffclos, f);
}

static void
test_longlong_vla_with_overflow_null(void)
{
	/* Test VLA null handling where ffpcljj returns overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PI" };  /* VLA of ints */
	/* 3000000000 overflows int, which triggers overflow handling in VLA path */
	LONGLONG data[] = { 100, 3000000000LL, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -999);
	/* Write should succeed but with overflow cleared */
	call_07(ffpcnjj, f, 1, 1, 1, 3, data, -999);
	call_01(ffclos, f);
}

static void
test_longlong_vla_good_overflow(void)
{
	/* Test VLA null handling where writing good pixels causes overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VLA" };
	char *tform[] = { "1PI" };  /* VLA of ints */
	/*
	** Pattern: null, overflow-good, null - should hit the overflow path
	** when writing good pixels after null.
	*/
	LONGLONG data[] = { -999, 3000000000LL, -999 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -999);
	/* Write should proceed but set overflow flag */
	call_07(ffpcnjj, f, 1, 1, 1, 3, data, -999);
	call_01(ffclos, f);
}

static void
test_longlong_ascii_table_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "I3" };
	LONGLONG data[] = { 10000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_ascii_table_overflow_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "I3" };
	LONGLONG data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.0001, 15, NULL);
	ffpcljj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_ascii_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STR" };
	char *tform[] = { "A10" };
	LONGLONG data[] = { 42 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_ATABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_binary_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STR" };
	char *tform[] = { "10A" };
	LONGLONG data[] = { 42 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcljj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_longlong_to_float_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	LONGLONG data[] = { 100, 200, 300 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0.0f, result, &anynull);
	fail_if(result[0] != 100.0f);
	fail_if(result[1] != 200.0f);
	fail_if(result[2] != 300.0f);
	call_01(ffclos, f);
}

static void
test_longlong_to_double_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	LONGLONG data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_longlong_ascii_table_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUM" };
	char *tform[] = { "F10.2" };
	LONGLONG data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0.0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
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
	test_write_to_byte_column();
	test_write_to_short_column();
	test_write_to_float_column();
	test_write_to_double_column();
	test_write_ascii_table();
	test_write_with_scaling();
	test_write_longlong_primary();
	test_write_longlong_with_null();
	test_write_longlong_2d();
	test_write_longlong_3d();
	test_write_longlong_subset();
	test_write_longlong_group_params();
	test_write_longlong_column();
	test_write_longlong_column_with_null();
	test_write_longlong_to_byte_column();
	test_write_longlong_to_short_column();
	test_write_longlong_to_int_column();
	test_write_longlong_to_float_column();
	test_write_longlong_to_double_column();
	test_write_longlong_to_string_column();
	test_write_bad_col_num();
	test_write_multirow();
	test_write_3d_noncontiguous();
	test_write_3d_bad_dimen();
	test_subsection_bad_naxis();
	test_write_to_longlong_column_scaled();
	test_byte_overflow_with_scaling();
	test_byte_underflow_with_scaling();
	test_short_overflow_with_scaling();
	test_short_underflow_with_scaling();
	test_long_overflow_with_scaling();
	test_long_underflow_with_scaling();
	test_longlong_overflow_with_scaling();
	test_longlong_underflow_with_scaling();
	test_subsection_3d();
	test_4d_subsection();
	test_5d_subsection();
	test_write_nulls_at_end();
	test_overflow_in_null_write();
	test_negative_dvalue_short();
	test_negative_dvalue_int32();
	test_write_to_unsigned_longlong();
	test_negative_dvalue_longlong();
	test_ascii_table_overflow();
	test_ascii_table_overflow_scaled();
	test_vla_with_nulls();
	test_write_to_ascii_string_column();
	test_write_to_binary_string_column();
	test_write_to_float_column_scaled();
	test_write_to_double_column_scaled();
	test_write_ascii_table_scaled();
	test_longlong_3d_noncontiguous();
	test_longlong_3d_bad_dimen();
	test_longlong_subsection_bad_naxis();
	test_longlong_byte_overflow_scaled();
	test_longlong_byte_underflow_scaled();
	test_longlong_short_overflow_scaled();
	test_longlong_short_underflow_scaled();
	test_longlong_int32_overflow_scaled();
	test_longlong_int32_underflow_scaled();
	test_longlong_to_unsigned_longlong();
	test_longlong_to_int32_unscaled_overflow();
	test_longlong_to_int32_unscaled_underflow();
	test_longlong_negative_dvalue_short();
	test_longlong_negative_dvalue_int32();
	test_longlong_to_longlong_scaled_overflow();
	test_longlong_to_longlong_scaled_underflow();
	test_longlong_negative_dvalue_longlong();
	test_longlong_subsection_3d();
	test_longlong_4d_subsection();
	test_longlong_5d_subsection();
	test_longlong_nulls_at_end();
	test_longlong_vla_with_nulls();
	test_longlong_vla_with_overflow_null();
	test_longlong_vla_good_overflow();
	test_longlong_ascii_table_overflow();
	test_longlong_ascii_table_overflow_scaled();
	test_longlong_to_ascii_string_column();
	test_longlong_to_binary_string_column();
	test_longlong_to_float_column_scaled();
	test_longlong_to_double_column_scaled();
	test_longlong_ascii_table_scaled();
	test_vla_with_overflow_null();
	test_long_to_byte_unscaled_negative();
	test_long_to_byte_unscaled_positive_overflow();
	test_long_to_short_unscaled_underflow();
	test_long_to_short_unscaled_overflow();
	test_longlong_to_byte_unscaled_negative();
	test_longlong_to_byte_unscaled_overflow();
	test_longlong_to_short_unscaled_underflow();
	remove(test_path);
	return 0;
}
