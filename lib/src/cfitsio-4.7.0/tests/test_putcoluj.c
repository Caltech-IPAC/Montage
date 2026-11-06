/*
 * Tests for putcoluj.c - unsigned long (TULONG) write functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcoluj.fits"
#define test_path2 "test_putcoluj2.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned long data[] = { 0, 100, 1000, 10000, INT_MAX };
	unsigned long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffppruj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 1000);
	fail_if(result[3] != 10000);
	fail_if(result[4] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_primary_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned long nulval = 999999;
	unsigned long data[] = { 100, nulval, 200, nulval, 300 };
	unsigned long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_02(ffpnul, f, (long)nulval);
	call_06(ffppnuj, f, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_write_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 2 };
	unsigned long data[] = { 1, 2, 3, 4, 5, 6 };
	unsigned long result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_06(ffp2duj, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 6, NULL, result, &anynull);
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
	unsigned long data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned long result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3duj, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 8, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[7] != 8);
	call_01(ffclos, f);
}

static void
test_write_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	unsigned long data[] = { 10, 20, 30, 40 };
	unsigned long result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_07(ffpssuj, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 16, NULL, result, &anynull);
	fail_if(result[5] != 10);
	fail_if(result[6] != 20);
	fail_if(result[9] != 30);
	fail_if(result[10] != 40);
	call_01(ffclos, f);
}

static void
test_write_binary_table_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UCOL" };
	char *tform[] = { "1V" };
	unsigned long data[] = { 0, 1000000, UINT_MAX };
	unsigned long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000000);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UCOL" };
	char *tform[] = { "5V" };
	unsigned long nulval = 999999;
	unsigned long data[] = { 100, nulval, 200, nulval, 300 };
	unsigned long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	call_07(ffpcnuj, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_write_to_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { 0, 1000, INT_MAX };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned long data[] = { 0, 1000000, INT_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000000);
	fail_if(result[2] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned long data[] = { 0, 1000, 10000 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 1000.0f);
	fail_if(result[2] != 10000.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned long data[] = { 0, 1000, 10000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 1000.0);
	fail_if(result[2] != 10000.0);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned long data[] = { 0, 127, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
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
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned long data[] = { 0, 1000, SHRT_MAX };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != SHRT_MAX);
	call_01(ffclos, f);
}

static void
test_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned long data[] = { 256 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_short_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned long data[] = { SHRT_MAX + 1UL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_long_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { (unsigned long)INT_MAX + 1UL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F15.1" };
	unsigned long data[] = { 0, 1000, 1000000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 1000.0);
	fail_if(result[2] != 1000000.0);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned long pdata[] = { 100, 200 };
	unsigned long idata[] = { 1, 2, 3, 4 };
	float presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, LONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpuj, f, 1, 1, 2, pdata);
	call_05(ffppruj, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpe, f, 1, 1, 2, presult);
	fail_if(presult[0] != 100.0f);
	fail_if(presult[1] != 200.0f);
	call_01(ffclos, f);
}

static void
test_write_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_write_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UCOL" };
	char *tform[] = { "3V" };
	unsigned long data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	unsigned long result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 9, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
	fail_if(result[8] != 9);
	call_01(ffclos, f);
}

static void
test_write_variable_length_array(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARV" };
	char *tform[] = { "1PV" };
	unsigned long data1[] = { 100, 200, 300 };
	unsigned long data2[] = { 400, 500 };
	unsigned long result[5];
	int anynull;
	long nelem;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, data1);
	call_06(ffpcluj, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	fail_if(result[2] != 300);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_09(ffgcv, f, TULONG, 1, 2, 1, 2, NULL, &result[3], &anynull);
	fail_if(result[3] != 400);
	fail_if(result[4] != 500);
	call_01(ffclos, f);
}

static void
test_bad_btable_format(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	unsigned long data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_atable_format(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "A5" };
	unsigned long data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_ATABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	unsigned long data[32];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	ffp3duj(f, 1, 2, 4, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2 };
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 2, 2 };
	unsigned long data[4];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	ffpssuj(f, 1, 8, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_noncontiguous_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	unsigned long data[] = {
		1, 2, 99,
		3, 4, 99,
		99, 99, 99,
		5, 6, 99,
		7, 8, 99,
		99, 99, 99
	};
	unsigned long result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3duj, f, 1, 3, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 8, NULL, result, &anynull);
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
test_scaled_byte_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned long data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 500.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned long data[] = { 1000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, -500.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_short_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned long data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 50000.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_short_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned long data[] = { 100000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, -50000.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_long_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 3000000000.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_long_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { 1000000000UL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.0001, 0.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_longlong_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned long data[] = { 1000000000UL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-18, 0.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_float(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned long data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_scaled_double(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned long data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_scaled_ascii(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F15.2" };
	unsigned long data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_int_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { 0, 1000000, UINT_MAX };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 2147483648.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 3, NULL, result, &anynull);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned long data[] = { 0, 1000000, INT_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	call_01(ffclos, f);
}

static void
test_multirow_element_wrap(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UCOL" };
	char *tform[] = { "2V" };
	unsigned long data[] = { 1, 2, 3, 4 };
	unsigned long result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 4, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 2, NULL, &result[0], &anynull);
	call_09(ffgcv, f, TULONG, 1, 2, 1, 2, NULL, &result[2], &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 4);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "5B" };
	unsigned long data[] = { 100, 255, 256, 255, 50 };
	unsigned long nulval = 255;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	ffpcnuj(f, 1, 1, 1, 5, data, nulval, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_field_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NCOL" };
	char *tform[] = { "F4.1" };
	unsigned long data[] = { 1000000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_scaled_field_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NCOL" };
	char *tform[] = { "F4.1" };
	unsigned long data[] = { 1000000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpcluj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_primary_with_nulls_longlong_coords(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	ULONGLONG nulval = 999999;
	ULONGLONG data[] = { 100, nulval, 200, nulval, 300 };
	ULONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_02(ffpnul, f, (long)nulval);
	call_06(ffppnujj, f, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONGLONG, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_write_subsection_longlong_coords(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	ULONGLONG data[] = { 10, 20, 30, 40 };
	ULONGLONG result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_07(ffpssujj, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONGLONG, 1, 16, NULL, result, &anynull);
	fail_if(result[5] != 10);
	fail_if(result[6] != 20);
	fail_if(result[9] != 30);
	fail_if(result[10] != 40);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls_longlong_coords(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "5K" };
	ULONGLONG nulval = 999999;
	ULONGLONG data[] = { 100, nulval, 200, nulval, 300 };
	ULONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	call_07(ffpcnujj, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONGLONG, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_write_ulonglong_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	ULONGLONG data[] = { 0, 127, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 127);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_write_ulonglong_to_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { 0, 1000, SHRT_MAX };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != SHRT_MAX);
	call_01(ffclos, f);
}

static void
test_write_ulonglong_to_int_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	ULONGLONG data[] = { 0, 1000, INT_MAX };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TINT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_write_ulonglong_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	ULONGLONG data[] = { 0, 1000, 10000 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 1000.0f);
	fail_if(result[2] != 10000.0f);
	call_01(ffclos, f);
}

static void
test_write_ulonglong_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	ULONGLONG data[] = { 0, 1000, 10000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 1000.0);
	fail_if(result[2] != 10000.0);
	call_01(ffclos, f);
}

static void
test_write_ulonglong_to_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "20A" };
	ULONGLONG data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Writing ULONGLONG to binary table string column is not supported */
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to ASCII table column (ffu8fstr function).
 */
static void
test_write_ulonglong_to_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F15.1" };
	ULONGLONG data[] = { 0, 1000, 1000000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 1000.0);
	fail_if(result[2] != 1000000.0);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to ASCII table with scaling (ffu8fstr scaled path).
 */
static void
test_write_ulonglong_to_ascii_table_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F15.2" };
	ULONGLONG data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}


/*
 * Test ffpcnuj with variable length array (VLA) column.
 */
static void
test_write_column_with_nulls_vla(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARV" };
	char *tform[] = { "1PV" };
	unsigned long nulval = 999999;
	unsigned long data[] = { 100, nulval, 200 };
	unsigned long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	call_07(ffpcnuj, f, 1, 1, 1, 3, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	call_01(ffclos, f);
}


/*
 * Test ffpcnuj ending with null values (tests the final "else if (nbad)" path).
 */
static void
test_write_column_with_trailing_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VCOL" };
	char *tform[] = { "5V" };
	unsigned long nulval = 999999;
	unsigned long data[] = { 100, 200, nulval, nulval, nulval };
	unsigned long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	call_07(ffpcnuj, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 5, NULL, result, &anynull);
	/* Check only the non-null values */
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to longlong column with scaling.
 */
static void
test_write_ulonglong_to_longlong_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	ULONGLONG data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to float column with scaling.
 */
static void
test_write_ulonglong_to_float_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	ULONGLONG data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to double column with scaling.
 */
static void
test_write_ulonglong_to_double_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	ULONGLONG data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG byte overflow without scaling.
 */
static void
test_ulonglong_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	ULONGLONG data[] = { 256 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ULONGLONG short overflow without scaling.
 */
static void
test_ulonglong_short_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { SHRT_MAX + 1ULL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ULONGLONG int32 overflow without scaling.
 */
static void
test_ulonglong_int_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	ULONGLONG data[] = { (ULONGLONG)INT_MAX + 1ULL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ULONGLONG longlong overflow without scaling.
 */
static void
test_ulonglong_longlong_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	ULONGLONG data[] = { (ULONGLONG)LLONG_MAX + 1ULL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ULONGLONG byte overflow with scaling.
 */
static void
test_ulonglong_scaled_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	ULONGLONG data[] = { 1000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, -500.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG short overflow with scaling.
 */
static void
test_ulonglong_scaled_short_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { 100000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, -50000.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG int overflow with scaling.
 */
static void
test_ulonglong_scaled_int_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	ULONGLONG data[] = { 1000000000ULL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.0001, 0.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG longlong overflow with scaling.
 */
static void
test_ulonglong_scaled_longlong_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	ULONGLONG data[] = { 1000000000ULL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-18, 0.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to byte column with scaling.
 */
static void
test_write_ulonglong_to_byte_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	ULONGLONG data[] = { 100, 150, 200 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 150);
	fail_if(result[2] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to short column with scaling.
 */
static void
test_write_ulonglong_to_short_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { 100, 1000, 10000 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 10000);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to int column with scaling.
 */
static void
test_write_ulonglong_to_int_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	ULONGLONG data[] = { 100, 1000, 100000 };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TINT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 100000);
	call_01(ffclos, f);
}

/*
 * Test ULONGLONG to byte underflow with scaling.
 */
static void
test_ulonglong_scaled_byte_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	ULONGLONG data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 500.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to short underflow with scaling.
 */
static void
test_ulonglong_scaled_short_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 50000.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to int underflow with scaling.
 */
static void
test_ulonglong_scaled_int_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	ULONGLONG data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 3000000000.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to longlong underflow with scaling.
 */
static void
test_ulonglong_scaled_longlong_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	ULONGLONG data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 1e19);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ffpcnujj with variable length array (VLA) column.
 */
static void
test_write_column_with_nulls_vla_ulonglong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARK" };
	char *tform[] = { "1PK" };
	ULONGLONG nulval = 999999;
	ULONGLONG data[] = { 100, nulval, 200 };
	ULONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	call_07(ffpcnujj, f, 1, 1, 1, 3, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	call_01(ffclos, f);
}


/*
 * Test ffpcnujj ending with null values.
 */
static void
test_write_column_with_trailing_nulls_ulonglong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "5K" };
	ULONGLONG nulval = 999999;
	ULONGLONG data[] = { 100, 200, nulval, nulval, nulval };
	ULONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, (long)nulval);
	call_07(ffpcnujj, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONGLONG, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG ASCII field overflow.
 */
static void
test_ulonglong_ascii_field_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NCOL" };
	char *tform[] = { "F4.1" };
	ULONGLONG data[] = { 1000000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG ASCII scaled field overflow.
 */
static void
test_ulonglong_ascii_scaled_field_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NCOL" };
	char *tform[] = { "F4.1" };
	ULONGLONG data[] = { 1000000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpclujj(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG 3D array write with BAD_DIMEN error.
 */
static void
test_ulonglong_bad_dimen_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	ULONGLONG data[32];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	ffp3dujj(f, 1, 2, 4, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}


/*
 * Test unsigned long to byte with fractional scaling (triggers rounding).
 */
static void
test_ulong_to_byte_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned long data[] = { 100 };
	unsigned char result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* scale=0.5 means: stored_value = (input - 0) / 0.5 = input * 2 */
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpcluj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 1, NULL, result, &anynull);
	/* Raw value should be 200 */
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test unsigned long to short with fractional scaling (triggers rounding).
 */
static void
test_ulong_to_short_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned long data[] = { 100 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpcluj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test unsigned long to long with fractional scaling (triggers rounding).
 */
static void
test_ulong_to_long_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned long data[] = { 100 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpcluj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test unsigned long to longlong with fractional scaling (triggers rounding).
 */
static void
test_ulong_to_longlong_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned long data[] = { 100 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpcluj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to byte with fractional scaling.
 */
static void
test_ulonglong_to_byte_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	ULONGLONG data[] = { 100 };
	unsigned char result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to short with fractional scaling.
 */
static void
test_ulonglong_to_short_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { 100 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to int with fractional scaling.
 */
static void
test_ulonglong_to_int_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	ULONGLONG data[] = { 100 };
	int result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TINT, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test ULONGLONG to longlong with fractional scaling.
 */
static void
test_ulonglong_to_longlong_fractional_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	ULONGLONG data[] = { 100 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclujj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 200);
	call_01(ffclos, f);
}


/*
 * Test writing to unsigned int column with 2^31 offset.
 * This exercises the XOR masking path in ffu4fi4.
 */
static void
test_write_to_unsigned_int_column_full_range(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* Use BZERO=2147483648 to store unsigned values */
	unsigned long data[] = { 0, 2147483647, 4294967295UL };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TZERO=2147483648 allows storing 0..2^32-1 in a signed J column */
	call_04(fftscl, f, 1, 1.0, 2147483648.0);
	call_06(ffpcluj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}


/* Test ffpssujj - write ULONGLONG subsection to image */
static void
test_write_ulonglong_subsection(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 5, 5 };
	ULONGLONG data[16];
	ULONGLONG result[16];
	int anynull;

	for (int i = 0; i < 16; i += 1) {
		data[i] = (ULONGLONG)(i + 100);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_07(ffpssujj, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	/* Read back and verify */
	call_03(ffopen, &f, test_path, READONLY);
	long rfpixel[] = { 2, 2 };
	long rlpixel[] = { 5, 5 };
	long inc[] = { 1, 1 };
	call_08(ffgsv, f, TULONGLONG, rfpixel, rlpixel, inc, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 101);
	fail_if(result[15] != 115);
	call_01(ffclos, f);
}


/*
 * Test negative scaled value rounding. When TZERO > input value,
 * scaled result is negative. On read, TZERO is re-applied, so we read
 * back the original values. The coverage happens during the write
 * conversion
 */
static void
test_ulong_to_short_negative_scale(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	char *ttype[] = { "VAL" };
	char *tform[] = { "1I" };  /* 16-bit signed int column */
	unsigned long data[] = { 0, 10, 20, 30, 40 };
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 5, 1, ttype, tform, NULL, NULL, 0);
	/* Set TZERO=100 so stored values are negative during conversion */
	call_04(ffpkyj, f, "TZERO1", 100, "zero point");
	call_06(ffpcluj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	/* Read back - TZERO is re-applied, so values appear as written */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 10);
	fail_if(result[4] != 40);
	call_01(ffclos, f);
}


/*
 * Test negative scaled value rounding for LONGLONG. Same approach -
 * TZERO causes negative values during conversion
 */
static void
test_ulong_to_longlong_negative_scale(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	char *ttype[] = { "VAL" };
	char *tform[] = { "1K" };  /* 64-bit signed int column */
	unsigned long data[] = { 0, 10, 20, 30, 40 };
	LONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 5, 1, ttype, tform, NULL, NULL, 0);
	/* Set TZERO=100 so stored values are negative during conversion */
	call_04(ffpkyj, f, "TZERO1", 100, "zero point");
	call_06(ffpcluj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	/* Read back */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 10);
	fail_if(result[4] != 40);
	call_01(ffclos, f);
}


/* Test negative scaled ULONGLONG to short */
static void
test_ulonglong_to_short_negative_scale(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	char *ttype[] = { "VAL" };
	char *tform[] = { "1I" };
	ULONGLONG data[] = { 0, 10, 20, 30, 40 };
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 5, 1, ttype, tform, NULL, NULL, 0);
	call_04(ffpkyj, f, "TZERO1", 100, "zero point");
	call_06(ffpclujj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 10);
	call_01(ffclos, f);
}


/* Test negative scaled ULONGLONG to int */
static void
test_ulonglong_to_int_negative_scale(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	char *ttype[] = { "VAL" };
	char *tform[] = { "1J" };  /* 32-bit int */
	ULONGLONG data[] = { 0, 10, 20, 30, 40 };
	int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 5, 1, ttype, tform, NULL, NULL, 0);
	call_04(ffpkyj, f, "TZERO1", 100, "zero point");
	call_06(ffpclujj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TINT, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 10);
	call_01(ffclos, f);
}


/* Test negative scaled ULONGLONG to longlong */
static void
test_ulonglong_to_longlong_negative_scale(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	char *ttype[] = { "VAL" };
	char *tform[] = { "1K" };  /* 64-bit signed int */
	ULONGLONG data[] = { 0, 10, 20, 30, 40 };
	LONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 5, 1, ttype, tform, NULL, NULL, 0);
	call_04(ffpkyj, f, "TZERO1", 100, "zero point");
	call_06(ffpclujj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 10);
	call_01(ffclos, f);
}


/* Test ffp3dujj with ncols != naxis1 (non-contiguous path) */
static void
test_write_ulonglong_3d_noncontiguous(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	ULONGLONG data[50];  /* 5 cols * 5 rows * 2 planes */
	ULONGLONG result[32];
	int anynull;

	for (int i = 0; i < 50; i += 1) {
		data[i] = (ULONGLONG)(i + 100);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	/* Write with ncols>naxis1, nrows>naxis2 */
	call_08(ffp3dujj, f, 1, 5, 5, 4, 4, 2, data);
	call_01(ffclos, f);

	/* Read back and verify */
	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONGLONG, 1, 32, NULL, result, &anynull);
	/* First plane: data[0-3] then data[5-8] etc (skipping extra columns) */
	fail_if(result[0] != 100);
	fail_if(result[1] != 101);
	fail_if(result[4] != 105);  /* Start of second row in FITS image */
	call_01(ffclos, f);
}


/* Test that writing TULONGLONG to compressed image returns error */
static void
test_ulonglong_compressed_image_error(void)
{

	fitsfile *infptr, *outfptr;
	int status = 0;
	long naxes[2] = { 16, 16 };
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 16, 16 };
	ULONGLONG data[256];
	ULONGLONG nulval = 999;

	for (int i = 0; i < 256; i += 1) {
		data[i] = (ULONGLONG)i;
	}

	/* Create uncompressed image */
	call_02(ffinit, &infptr, "!" test_path);
	call_04(ffphps, infptr, LONG_IMG, 2, naxes);
	call_01(ffclos, infptr);

	/* Compress to new file */
	call_03(ffopen, &infptr, test_path, READONLY);
	status = 0;
	fits_create_file(&outfptr, "!" test_path2, &status);
	fail_if(status);
	status = 0;
	fits_set_compression_type(outfptr, RICE_1, &status);
	fail_if(status);
	status = 0;
	fits_img_compress(infptr, outfptr, &status);
	fail_if(status);
	call_01(ffclos, infptr);
	call_01(ffclos, outfptr);

	/* Try to write ULONGLONG to compressed - should fail */
	call_03(ffopen, &outfptr, test_path2, READWRITE);
	status = 0;
	fits_movabs_hdu(outfptr, 2, NULL, &status);
	fail_if(status);

	/* Test ffpprujj - should return DATA_COMPRESSION_ERR */
	status = 0;
	ffpprujj(outfptr, 1, 1, 256, data, &status);
	fail_if(status != DATA_COMPRESSION_ERR);

	/* Test ffppnujj - should return DATA_COMPRESSION_ERR */
	status = 0;
	ffppnujj(outfptr, 1, 1, 256, data, nulval, &status);
	fail_if(status != DATA_COMPRESSION_ERR);

	/* Test ffp2dujj - should return DATA_COMPRESSION_ERR */
	status = 0;
	ffp2dujj(outfptr, 1, 16, 16, 16, data, &status);
	fail_if(status != DATA_COMPRESSION_ERR);

	/* Test ffp3dujj - should return DATA_COMPRESSION_ERR */
	status = 0;
	ffp3dujj(outfptr, 1, 16, 16, 16, 16, 1, data, &status);
	fail_if(status != DATA_COMPRESSION_ERR);

	/* Test ffpssujj - should return DATA_COMPRESSION_ERR */
	status = 0;
	ffpssujj(outfptr, 1, 2, naxes, fpixel, lpixel, data, &status);
	fail_if(status != DATA_COMPRESSION_ERR);

	status = 0;
	ffclos(outfptr, &status);
	remove(test_path2);
}


/*
 * Writing TULONG to compressed images is not supported on 64-bit systems
 * where sizeof(long) == 8.  See imcompress.c:1901.
 */

int
main(void)
{
	test_write_primary_array();
	test_write_primary_with_nulls();
	test_write_2d_array();
	test_write_3d_array();
	test_write_subsection();
	test_write_binary_table_column();
	test_write_column_with_nulls();
	test_write_to_long_column();
	test_write_to_longlong_column();
	test_write_to_float_column();
	test_write_to_double_column();
	test_write_to_byte_column();
	test_write_to_short_column();
	test_byte_overflow();
	test_short_overflow();
	test_long_overflow();
	test_write_to_ascii_table();
	test_write_group_parameters();
	test_write_with_scaling();
	test_write_multirow();
	test_write_variable_length_array();
	test_bad_btable_format();
	test_bad_atable_format();
	test_bad_dimen_3d();
	test_bad_dimen_subsection();
	test_write_noncontiguous_3d();
	test_scaled_byte_underflow();
	test_scaled_byte_overflow();
	test_scaled_short_underflow();
	test_scaled_short_overflow();
	test_scaled_long_underflow();
	test_scaled_long_overflow();
	test_scaled_longlong_overflow();
	test_scaled_float();
	test_scaled_double();
	test_scaled_ascii();
	test_write_to_unsigned_int_column();
	test_write_to_unsigned_longlong();
	test_multirow_element_wrap();
	test_write_column_with_nulls_overflow();
	test_ascii_field_overflow();
	test_ascii_scaled_field_overflow();
	test_write_primary_with_nulls_longlong_coords();
	test_write_subsection_longlong_coords();
	test_write_column_with_nulls_longlong_coords();
	test_write_ulonglong_to_byte_column();
	test_write_ulonglong_to_short_column();
	test_write_ulonglong_to_int_column();
	test_write_ulonglong_to_float_column();
	test_write_ulonglong_to_double_column();
	test_write_ulonglong_to_string_column();
	test_write_ulonglong_to_ascii_table();
	test_write_ulonglong_to_ascii_table_scaled();
	test_write_column_with_nulls_vla();
	test_write_column_with_trailing_nulls();
	test_write_ulonglong_to_longlong_scaled();
	test_write_ulonglong_to_float_scaled();
	test_write_ulonglong_to_double_scaled();
	test_ulonglong_byte_overflow();
	test_ulonglong_short_overflow();
	test_ulonglong_int_overflow();
	test_ulonglong_longlong_overflow();
	test_ulonglong_scaled_byte_overflow();
	test_ulonglong_scaled_short_overflow();
	test_ulonglong_scaled_int_overflow();
	test_ulonglong_scaled_longlong_overflow();
	test_write_ulonglong_to_byte_scaled();
	test_write_ulonglong_to_short_scaled();
	test_write_ulonglong_to_int_scaled();
	test_ulonglong_scaled_byte_underflow();
	test_ulonglong_scaled_short_underflow();
	test_ulonglong_scaled_int_underflow();
	test_ulonglong_scaled_longlong_underflow();
	test_write_column_with_nulls_vla_ulonglong();
	test_write_column_with_trailing_nulls_ulonglong();
	test_ulonglong_ascii_field_overflow();
	test_ulonglong_ascii_scaled_field_overflow();
	test_ulonglong_bad_dimen_3d();
	test_ulong_to_byte_fractional_scale();
	test_ulong_to_short_fractional_scale();
	test_ulong_to_long_fractional_scale();
	test_ulong_to_longlong_fractional_scale();
	test_ulonglong_to_byte_fractional_scale();
	test_ulonglong_to_short_fractional_scale();
	test_ulonglong_to_int_fractional_scale();
	test_ulonglong_to_longlong_fractional_scale();
	test_write_to_unsigned_int_column_full_range();
	test_write_ulonglong_subsection();
	test_ulong_to_short_negative_scale();
	test_ulong_to_longlong_negative_scale();
	test_ulonglong_to_short_negative_scale();
	test_ulonglong_to_int_negative_scale();
	test_ulonglong_to_longlong_negative_scale();
	test_write_ulonglong_3d_noncontiguous();
	test_ulonglong_compressed_image_error();
	remove(test_path);

	return 0;
}
