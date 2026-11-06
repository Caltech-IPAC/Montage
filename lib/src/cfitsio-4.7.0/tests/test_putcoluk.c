/*
 * Tests for putcoluk.c - unsigned int write functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcoluk.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned int data[] = { 0, 100, 1000, 10000, 100000 };
	unsigned int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppruk, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 1000);
	fail_if(result[3] != 10000);
	fail_if(result[4] != 100000);
	call_01(ffclos, f);
}

static void
test_write_primary_with_null(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned int data[] = { 10, 20, 999, 40, 50 };
	unsigned int result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_04(ffpkyj, f, "BLANK", 999, NULL);
	call_06(ffppnuk, f, 1, 1, 5, data, 999);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfuk, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(nularray[2] != 1);
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
	unsigned int data[] = { 1, 2, 3, 4, 5, 6 };
	unsigned int result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 2, naxes);
	call_06(ffp2duk, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2duk, f, 1, 0, 3, 3, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 4);
	fail_if(result[4] != 5);
	fail_if(result[5] != 6);
	call_01(ffclos, f);
}

static void
test_write_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	unsigned int data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned int result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 3, naxes);
	call_08(ffp3duk, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3duk, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
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
	unsigned int data[] = { 100, 200, 300, 400 };
	unsigned int result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 2, naxes);
	call_07(ffpssuk, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 16, 0, result, &anynull);
	fail_if(result[5] != 100);
	fail_if(result[6] != 200);
	fail_if(result[9] != 300);
	fail_if(result[10] != 400);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned int pdata[] = { 10, 20 };
	unsigned int idata[] = { 1, 2, 3, 4 };
	unsigned int presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, ULONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpuk, f, 1, 1, 2, pdata);
	call_05(ffppruk, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpuk, f, 1, 1, 2, presult);
	fail_if(presult[0] != 10);
	fail_if(presult[1] != 20);
	call_01(ffclos, f);
}

static void
test_write_binary_table_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UKCOL" };
	char *tform[] = { "1V" };
	unsigned int data[] = { 0, 1000000000U, 3000000000U };
	unsigned int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvuk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000000000U);
	fail_if(result[2] != 3000000000U);
	call_01(ffclos, f);
}

static void
test_write_column_with_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "5K" };
	unsigned int data[] = { 10, 20, 999, 40, 50 };
	unsigned int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 999);
	call_07(ffpcnuk, f, 1, 1, 1, 5, data, 999);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvuk, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned int data[] = { 0, 128, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
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
test_write_to_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned int data[] = { 0, 1000, 32767 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 32767);
	call_01(ffclos, f);
}

static void
test_write_to_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned int data[] = { 0, 100000, INT_MAX };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100000);
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
	unsigned int data[] = { 0, 1000000000U, UINT_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000000000LL);
	fail_if(result[2] != UINT_MAX);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned int data[] = { 0, 1000, 1000000 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 1000.0f);
	fail_if(result[2] != 1000000.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned int data[] = { 0, 1000, 1000000 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 1000.0);
	fail_if(result[2] != 1000000.0);
	call_01(ffclos, f);
}

static void
test_write_to_string_column_fails(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "15A" };
	unsigned int data[] = { 0, 12345, 4000000000U };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	ffpcluk(f, 1, 1, 1, 3, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "I15" };
	unsigned int data[] = { 0, 1000, 1000000 };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 1000000);
	call_01(ffclos, f);
}

static void
test_write_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UKCOL" };
	char *tform[] = { "3V" };
	unsigned int data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	unsigned int result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvuk, f, 1, 1, 1, 9, 0, result, &anynull);
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
	char *ttype[] = { "VARUK" };
	char *tform[] = { "1PV" };
	unsigned int data1[] = { 10, 20, 30 };
	unsigned int data2[] = { 40, 50 };
	unsigned int result[5];
	int anynull;
	long nelem;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data1);
	call_06(ffpcluk, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_08(ffgcvuk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_08(ffgcvuk, f, 1, 2, 1, 2, 0, &result[3], &anynull);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	call_01(ffclos, f);
}

static void
test_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned int data[] = { 256 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluk(f, 1, 1, 1, 1, data, &status);
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
	unsigned int data[] = { 32768 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluk(f, 1, 1, 1, 1, data, &status);
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
	unsigned int data[] = { 2147483648U };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcluk(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_bad_status(void)
{
	fitsfile *f;
	int status = 1;
	unsigned int data[] = { 1 };
	ffppruk(f, 1, 1, 1, data, &status);
	fail_if(status != 1);
	ffppnuk(f, 1, 1, 1, data, 0, &status);
	fail_if(status != 1);
	ffpcluk(f, 1, 1, 1, 1, data, &status);
	fail_if(status != 1);
	ffpcnuk(f, 1, 1, 1, 1, data, 0, &status);
	fail_if(status != 1);
}

static void
test_write_null_column_no_tnull(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UKCOL" };
	char *tform[] = { "5V" };
	unsigned int data[] = { 10, 20, 999, 40, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcnuk(f, 1, 1, 1, 5, data, 999, &status);
	fail_if(status != NO_NULL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_large_values(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned int data[] = { 0, UINT_MAX / 2, UINT_MAX };
	unsigned int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppruk, f, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != UINT_MAX / 2);
	fail_if(result[2] != UINT_MAX);
	call_01(ffclos, f);
}

static void
test_write_3d_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3, 2 };
	/* Input array is 4x4x2 but we write to 3x3x2 image */
	unsigned int data[32];
	unsigned int result[18];
	int anynull, i;

	for (i = 0; i < 32; i += 1) {
		data[i] = (unsigned int)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 3, naxes);
	/* ncols=4, nrows=4 but naxis1=3, naxis2=3 */
	call_08(ffp3duk, f, 1, 4, 4, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 18, 0, result, &anynull);
	/* First plane: rows 0,1,2 of input array, first 3 columns each */
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 5);  /* row 1 starts at index 4 in input */
	call_01(ffclos, f);
}

static void
test_write_3d_bad_dimen(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5, 5, 2 };
	unsigned int data[50];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 3, naxes);
	/* ncols < naxis1 should fail */
	ffp3duk(f, 1, 3, 3, 5, 5, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_subsection_bad_naxis(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	long fpixel[] = { 1 };
	long lpixel[] = { 4 };
	unsigned int data[] = { 1, 2, 3, 4 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	/* naxis = 0 should fail */
	ffpssuk(f, 1, 0, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	/* naxis = 8 should fail */
	ffpssuk(f, 1, 8, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_long_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_to_float_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_to_double_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_to_ascii_table_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F15.2" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_vla_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARUK" };
	char *tform[] = { "1PV" };
	unsigned int data[] = { 10, 20, 30 };
	unsigned int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Write variable length array (exercises tcode < 0 path in ffpcnuk) */
	/* Note: TNULL doesn't work for VLAs, so just test the write path */
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvuk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	call_01(ffclos, f);
}

static void
test_write_nulls_at_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VAL" };
	char *tform[] = { "1J" };
	/* Data where last values are equal to null value */
	unsigned int data[] = { 10, 20, 999, 999 };
	long result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 999);
	/* Data ends with null values - tests final nbad path in ffpcnuk */
	call_07(ffpcnuk, f, 1, 1, 1, 4, data, 999);
	call_01(ffclos, f);

	/* Just verify the file was written - null handling code path exercised */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 4, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	/* Null values are written as TNULL value */
	fail_if(result[2] != 999);
	fail_if(result[3] != 999);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UKCOL" };
	char *tform[] = { "1K" };
	unsigned int data[] = { 0, 1000000000U, UINT_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Set TZERO to make it unsigned long long column */
	call_04(ffpkyj, f, "TZERO1", 9223372036854775807LL, NULL);
	call_04(ffpkyj, f, "TSCAL1", 1, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Disable scaling to read raw values */
	call_04(fftscl, f, 1, 1.0, 0.0);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	/* Values are XOR'd with 0x8000000000000000 */
	call_01(ffclos, f);
}

static void
test_write_to_long_negative_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned int data[] = { 0, 10, 40 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 so values become negative when scaled */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	/* TZERO is applied on read, so values appear as original */
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 10.0);
	fail_if(result[2] != 40.0);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_negative_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned int data[] = { 0, 10, 40 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 so values become negative when scaled */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 10.0);
	fail_if(result[2] != 40.0);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_to_short_column_scaled(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned int data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 50.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column_negative_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TZERO=100, stored = true - 100, so true must be >= 100 */
	unsigned int data[] = { 100, 150, 200 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 causes scaled values to shift */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 150.0);
	fail_if(result[2] != 200.0);
	call_01(ffclos, f);
}

static void
test_write_to_short_column_negative_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned int data[] = { 0, 50, 100 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* TZERO=100 causes negative scaled values */
	call_05(ffpkyd, f, "TZERO1", 100.0, 15, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 50.0);
	fail_if(result[2] != 100.0);
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
	unsigned int data[8];
	unsigned int result[32];
	int anynull, i;

	for (i = 0; i < 8; i += 1) {
		data[i] = (unsigned int)(i + 100);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 3, naxes);
	call_07(ffpssuk, f, 1, 3, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 32, 0, result, &anynull);
	/* Check some values at expected positions */
	fail_if(result[5] != 100);  /* fpixel[1,1,0] -> position 5 */
	call_01(ffclos, f);
}

static void
test_overflow_in_null_write(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* Value 300 will overflow byte column, 999 is null marker */
	unsigned int data[] = { 100, 300, 999 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 123);
	/* 300 overflows byte - tests overflow path in ffpcnuk */
	ffpcnuk(f, 1, 1, 1, 3, data, 999, &status);
	/* Expect NUM_OVERFLOW because 300 > 255 */
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_multirow_spanning_rows(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VAL" };
	char *tform[] = { "3J" };
	/* Data that spans multiple rows */
	unsigned int data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
	long result[12];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	/* Write all 12 values starting at row 1, elem 1 */
	call_06(ffpcluk, f, 1, 1, 1, 12, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 12, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[3] != 4);  /* Start of row 2 */
	fail_if(result[11] != 12);
	call_01(ffclos, f);
}

static void
test_byte_overflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2, so 200 -> 400 overflows */
	unsigned int data[] = { 50, 200, 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	/* 200 / 0.5 = 400 > 255, should overflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
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
	/* With TSCAL=0.5, dvalue = input/0.5 = input*2, so 20000 -> 40000 overflows */
	unsigned int data[] = { 1000, 20000, 5000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	/* 20000 / 0.5 = 40000 > 32767, should overflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
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
	/* With TZERO=300, dvalue = (input - 300) which is negative for small values */
	unsigned int data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 300.0, 15, NULL);
	/* 100 - 300 = -200 < 0, should underflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
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
	unsigned int data[16];
	unsigned int result[36];
	int anynull, i;

	for (i = 0; i < 16; i += 1) {
		data[i] = (unsigned int)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 4, naxes);
	call_07(ffpssuk, f, 1, 4, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 36, 0, result, &anynull);
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
	unsigned int data[32];
	unsigned int result[32];
	int anynull, i;

	for (i = 0; i < 32; i += 1) {
		data[i] = (unsigned int)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 5, naxes);
	call_07(ffpssuk, f, 1, 5, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvuk, f, 1, 1, 32, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[31] != 32);
	call_01(ffclos, f);
}

static void
test_short_underflow_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With TZERO=50000, dvalue = (0 - 50000) = -50000 < -32768 */
	unsigned int data[] = { 0, 100, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 50000.0, 15, NULL);
	/* 0 - 50000 = -50000 < -32768, should underflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
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
	/* With TZERO=3e9, dvalue = (0 - 3e9) = -3e9 < -2147483648 */
	unsigned int data[] = { 0, 100, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 3.0e9, 15, NULL);
	/* 0 - 3e9 = -3e9 < -2147483648, should underflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
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
	/* With TSCAL=0.5, dvalue = 3e9/0.5 = 6e9 > 2147483647 */
	unsigned int data[] = { 100, 3000000000U, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 0.5, 15, NULL);
	/* 3e9 / 0.5 = 6e9 > 2147483647, should overflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
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
	/* With TZERO=1e19, dvalue = (0 - 1e19) < LONGLONG_MIN */
	unsigned int data[] = { 0, 100, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TZERO1", 1.0e19, 15, NULL);
	/* 0 - 1e19 < LONGLONG_MIN, should underflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
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
	/* With TSCAL=1e-10, dvalue = 4e9/1e-10 = 4e19 > LONGLONG_MAX */
	unsigned int data[] = { 100, 4000000000U, 200 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 1.0e-10, 15, NULL);
	/* 4e9 / 1e-10 = 4e19 > LONGLONG_MAX, should overflow */
	ffpcluk(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

int
main(void)
{
	test_write_primary_array();
	test_write_primary_with_null();
	test_write_2d_array();
	test_write_3d_array();
	test_write_subsection();
	test_write_group_parameters();
	test_write_binary_table_column();
	test_write_column_with_null();
	test_write_to_byte_column();
	test_write_to_short_column();
	test_write_to_long_column();
	test_write_to_longlong_column();
	test_write_to_float_column();
	test_write_to_double_column();
	test_write_to_string_column_fails();
	test_write_to_ascii_table();
	test_write_multirow();
	test_write_variable_length_array();
	test_byte_overflow();
	test_short_overflow();
	test_long_overflow();
	test_write_with_scaling();
	test_bad_status();
	test_write_null_column_no_tnull();
	test_write_large_values();
	test_write_3d_noncontiguous();
	test_write_3d_bad_dimen();
	test_subsection_bad_naxis();
	test_write_to_long_column_scaled();
	test_write_to_longlong_column_scaled();
	test_write_to_float_column_scaled();
	test_write_to_double_column_scaled();
	test_write_to_ascii_table_scaled();
	test_write_vla_with_nulls();
	test_write_nulls_at_end();
	test_write_to_unsigned_longlong();
	test_write_to_long_negative_scale();
	test_write_to_longlong_negative_scale();
	test_write_to_byte_column_scaled();
	test_write_to_short_column_scaled();
	test_write_to_byte_column_negative_scale();
	test_write_to_short_column_negative_scale();
	test_subsection_3d();
	test_overflow_in_null_write();
	test_multirow_spanning_rows();
	test_byte_overflow_with_scaling();
	test_short_overflow_with_scaling();
	test_byte_underflow_with_scaling();
	test_4d_subsection();
	test_5d_subsection();
	test_short_underflow_with_scaling();
	test_long_underflow_with_scaling();
	test_long_overflow_with_scaling();
	test_longlong_underflow_with_scaling();
	test_longlong_overflow_with_scaling();
	remove(test_path);

	return 0;
}
