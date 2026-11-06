/*
 * Tests for getcolk.c - signed int read functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcolk.fits"

static void
test_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int data[] = { -1000, -100, 0, 100, 1000 };
	int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprk, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvk, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != -1000);
	fail_if(result[1] != -100);
	fail_if(result[2] != 0);
	fail_if(result[3] != 100);
	fail_if(result[4] != 1000);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_primary_no_nulls(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int data[] = { INT_MIN, -1, 0, 1, INT_MAX };
	int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprk, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvk, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != INT_MIN);
	fail_if(result[1] != -1);
	fail_if(result[2] != 0);
	fail_if(result[3] != 1);
	fail_if(result[4] != INT_MAX);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_primary_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int data[] = { 10, 20, 30, 40, 50 };
	int result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprk, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfk, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 2 };
	int data[] = { 1, 2, 3, 4, 5, 6 };
	int result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_06(ffp2dk, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2dk, f, 1, 0, 3, 3, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 4);
	fail_if(result[4] != 5);
	fail_if(result[5] != 6);
	call_01(ffclos, f);
}

static void
test_read_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	int data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	int result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3dk, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dk, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
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
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	int result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_05(ffpprk, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvk, f, 1, 2, naxes, fpixel, lpixel, inc, 0, result, &anynull);
	fail_if(result[0] != 6);
	fail_if(result[1] != 7);
	fail_if(result[2] != 10);
	fail_if(result[3] != 11);
	call_01(ffclos, f);
}

static void
test_read_binary_table_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1J" };
	int data[] = { INT_MIN, 0, INT_MAX };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclk, f, 1, 1, 1, 3, data);
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
test_read_column_no_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "5J" };
	int data[] = { -100, -50, 0, 50, 100 };
	int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclk, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != -50);
	fail_if(result[2] != 0);
	fail_if(result[3] != 50);
	fail_if(result[4] != 100);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "5J" };
	int data[] = { 10, 20, 30, 40, 50 };
	int result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclk, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfk, f, 1, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_from_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 0, 128, 255 };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 128);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

static void
test_read_from_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { SHRT_MIN, 0, SHRT_MAX };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != SHRT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != SHRT_MAX);
	call_01(ffclos, f);
}

static void
test_read_from_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	float data[] = { -1000.0f, 0.0f, 1000.0f };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000);
	call_01(ffclos, f);
}

static void
test_read_from_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	double data[] = { -1000000.0, 0.0, 1000000.0 };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000000);
	call_01(ffclos, f);
}

static void
test_read_from_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "I15" };
	long data[] = { -1000000, 0, 1000000 };
	int result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000000);
	call_01(ffclos, f);
}

static void
test_read_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "3J" };
	int data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	int result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclk, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvk, f, 1, 1, 1, 9, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[4] != 5);
	fail_if(result[8] != 9);
	call_01(ffclos, f);
}

static void
test_read_variable_length_array(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARK" };
	char *tform[] = { "1PJ" };
	int data1[] = { 10, 20, 30 };
	int data2[] = { 40, 50 };
	int result[5];
	int anynull;
	long nelem;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclk, f, 1, 1, 1, 3, data1);
	call_06(ffpclk, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_08(ffgcvk, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_08(ffgcvk, f, 1, 2, 1, 2, 0, &result[3], &anynull);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	call_01(ffclos, f);
}

static void
test_longlong_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { (LONGLONG)INT_MAX + 1 };
	int result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvk(f, 1, 1, 1, 1, 0, result, &anynull, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_double_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	double data[] = { 1e15 };
	int result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvk(f, 1, 1, 1, 1, 0, result, &anynull, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_noncontiguous_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	int data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	int result[18];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3dk, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dk, f, 1, 0, 3, 3, 2, 2, 2, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[3] != 3);
	fail_if(result[4] != 4);
	fail_if(result[9] != 5);
	fail_if(result[10] != 6);
	fail_if(result[12] != 7);
	fail_if(result[13] != 8);
	call_01(ffclos, f);
}

static void
test_bad_dimen_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	int result[32];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffg3dk(f, 1, 0, 2, 2, 4, 4, 2, result, &anynull, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	int pdata[] = { 10, 20 };
	int idata[] = { 1, 2, 3, 4 };
	int presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, LONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpk, f, 1, 1, 2, pdata);
	call_05(ffpprk, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpk, f, 1, 1, 2, presult);
	fail_if(presult[0] != 10);
	fail_if(presult[1] != 20);
	call_01(ffclos, f);
}

static void
test_read_subsection_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	int result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_05(ffpprk, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfk, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
	fail_if(result[0] != 6);
	fail_if(result[1] != 7);
	fail_if(result[2] != 10);
	fail_if(result[3] != 11);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

int
main(void)
{
	test_read_primary_array();
	test_read_primary_no_nulls();
	test_read_primary_with_null_flags();
	test_read_2d_array();
	test_read_3d_array();
	test_read_subsection();
	test_read_binary_table_column();
	test_read_column_no_nulls();
	test_read_column_with_null_flags();
	test_read_from_byte_column();
	test_read_from_short_column();
	test_read_from_float_column();
	test_read_from_double_column();
	test_read_from_ascii_table();
	test_read_multirow();
	test_read_variable_length_array();
	test_longlong_overflow();
	test_double_overflow();
	test_read_noncontiguous_3d();
	test_bad_dimen_3d();
	test_read_group_parameters();
	test_read_subsection_with_null_flags();
	remove(test_path);

	return 0;
}
