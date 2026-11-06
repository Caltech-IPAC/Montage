/*
 * Tests for getcolj.c - signed long read functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcolj.fits"

static void
test_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	/* FITS LONG_IMG is 32-bit signed, use INT_MIN/MAX for portable tests */
	long data[] = { INT_MIN, -1000, 0, 1000, INT_MAX };
	long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvj, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != INT_MIN);
	fail_if(result[1] != -1000);
	fail_if(result[2] != 0);
	fail_if(result[3] != 1000);
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
	long data[] = { -100, -50, 0, 50, 100 };
	long result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfj, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != -50);
	fail_if(result[2] != 0);
	fail_if(result[3] != 50);
	fail_if(result[4] != 100);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 2 };
	long data[] = { -3, -2, -1, 1, 2, 3 };
	long result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_06(ffp2dj, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2dj, f, 1, 0, 3, 3, 2, result, &anynull);
	fail_if(result[0] != -3);
	fail_if(result[1] != -2);
	fail_if(result[2] != -1);
	fail_if(result[3] != 1);
	fail_if(result[4] != 2);
	fail_if(result[5] != 3);
	call_01(ffclos, f);
}

static void
test_read_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	long data[] = { -4, -3, -2, -1, 1, 2, 3, 4 };
	long result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3dj, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dj, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != -4);
	fail_if(result[7] != 4);
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
	long data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	long result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_05(ffpprj, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvj, f, 1, 2, naxes, fpixel, lpixel, inc, 0, result, &anynull);
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
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* FITS 'J' column is 32-bit signed, use INT_MIN/MAX */
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
test_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "5J" };
	long data[] = { -100, -50, 0, 50, 100 };
	long result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfj, f, 1, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != -50);
	fail_if(result[2] != 0);
	fail_if(result[3] != 50);
	fail_if(result[4] != 100);
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
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
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
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
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
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
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
	char *ttype[] = { "JCOL" };
	char *tform[] = { "3J" };
	long data[] = { -4, -3, -2, -1, 0, 1, 2, 3, 4 };
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
	fail_if(result[0] != -4);
	fail_if(result[4] != 0);
	fail_if(result[8] != 4);
	call_01(ffclos, f);
}

static void
test_read_variable_length_array(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARJ" };
	char *tform[] = { "1PJ" };
	long data1[] = { -30, -20, -10 };
	long data2[] = { 10, 20 };
	long result[5];
	int anynull;
	long nelem;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data1);
	call_06(ffpclj, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -30);
	fail_if(result[1] != -20);
	fail_if(result[2] != -10);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_08(ffgcvj, f, 1, 2, 1, 2, 0, &result[3], &anynull);
	fail_if(result[3] != 10);
	fail_if(result[4] != 20);
	call_01(ffclos, f);
}

static void
test_read_noncontiguous_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	long data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	long result[18];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_08(ffp3dj, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dj, f, 1, 0, 3, 3, 2, 2, 2, result, &anynull);
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
	long result[32];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffg3dj(f, 1, 0, 2, 2, 4, 4, 2, result, &anynull, &status);
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
	long pdata[] = { -10, 20 };
	long idata[] = { 1, 2, 3, 4 };
	long presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, LONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpj, f, 1, 1, 2, pdata);
	call_05(ffpprj, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpj, f, 1, 1, 2, presult);
	fail_if(presult[0] != -10);
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
	long data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	long result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_05(ffpprj, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfj, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
	fail_if(result[0] != 6);
	fail_if(result[1] != 7);
	fail_if(result[2] != 10);
	fail_if(result[3] != 11);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

/* JJ (LONGLONG) function tests */

static void
test_jj_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	LONGLONG data[] = { LLONG_MIN, -1000, 0, 1000, LLONG_MAX };
	LONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvjj, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != LLONG_MIN);
	fail_if(result[1] != -1000);
	fail_if(result[2] != 0);
	fail_if(result[3] != 1000);
	fail_if(result[4] != LLONG_MAX);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_jj_read_primary_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	LONGLONG data[] = { -100, -50, 0, 50, 100 };
	LONGLONG result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfjj, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != -50);
	fail_if(result[2] != 0);
	fail_if(result[3] != 50);
	fail_if(result[4] != 100);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_jj_read_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 2 };
	LONGLONG data[] = { -3, -2, -1, 1, 2, 3 };
	LONGLONG result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_06(ffp2djj, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2djj, f, 1, 0, 3, 3, 2, result, &anynull);
	fail_if(result[0] != -3);
	fail_if(result[1] != -2);
	fail_if(result[2] != -1);
	fail_if(result[3] != 1);
	fail_if(result[4] != 2);
	fail_if(result[5] != 3);
	call_01(ffclos, f);
}

static void
test_jj_read_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	LONGLONG data[] = { -4, -3, -2, -1, 1, 2, 3, 4 };
	LONGLONG result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 3, naxes);
	call_08(ffp3djj, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3djj, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
	fail_if(result[0] != -4);
	fail_if(result[7] != 4);
	call_01(ffclos, f);
}

static void
test_jj_read_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	LONGLONG data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	LONGLONG result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_05(ffpprjj, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvjj, f, 1, 2, naxes, fpixel, lpixel, inc, 0, result, &anynull);
	fail_if(result[0] != 6);
	fail_if(result[1] != 7);
	fail_if(result[2] != 10);
	fail_if(result[3] != 11);
	call_01(ffclos, f);
}

static void
test_jj_read_subsection_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	long fpixel[] = { 2, 2 };
	long lpixel[] = { 3, 3 };
	long inc[] = { 1, 1 };
	LONGLONG data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	LONGLONG result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_05(ffpprjj, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfjj, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
	fail_if(result[0] != 6);
	fail_if(result[1] != 7);
	fail_if(result[2] != 10);
	fail_if(result[3] != 11);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_jj_read_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	LONGLONG pdata[] = { -10, 20 };
	LONGLONG idata[] = { 1, 2, 3, 4 };
	LONGLONG presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, LONGLONG_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpjj, f, 1, 1, 2, pdata);
	call_05(ffpprjj, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpjj, f, 1, 1, 2, presult);
	fail_if(presult[0] != -10);
	fail_if(presult[1] != 20);
	call_01(ffclos, f);
}

static void
test_jj_read_binary_table_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
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
test_jj_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "5K" };
	LONGLONG data[] = { -100, -50, 0, 50, 100 };
	LONGLONG result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfjj, f, 1, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != -100);
	fail_if(result[1] != -50);
	fail_if(result[2] != 0);
	fail_if(result[3] != 50);
	fail_if(result[4] != 100);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_jj_read_from_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
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
test_jj_read_from_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { SHRT_MIN, 0, SHRT_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != SHRT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != SHRT_MAX);
	call_01(ffclos, f);
}

static void
test_jj_read_from_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	long data[] = { INT_MIN, 0, INT_MAX };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != INT_MIN);
	fail_if(result[1] != 0);
	fail_if(result[2] != INT_MAX);
	call_01(ffclos, f);
}

static void
test_jj_read_from_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	float data[] = { -1000.0f, 0.0f, 1000.0f };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000);
	call_01(ffclos, f);
}

static void
test_jj_read_from_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	double data[] = { -1000000.0, 0.0, 1000000.0 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000000);
	call_01(ffclos, f);
}

static void
test_jj_read_from_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "I20" };
	LONGLONG data[] = { -1000000000LL, 0, 1000000000LL };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != -1000000000LL);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000000000LL);
	call_01(ffclos, f);
}

int
main(void)
{
	/* J (long) tests */
	test_read_primary_array();
	test_read_primary_with_null_flags();
	test_read_2d_array();
	test_read_3d_array();
	test_read_subsection();
	test_read_binary_table_column();
	test_read_column_with_null_flags();
	test_read_from_byte_column();
	test_read_from_short_column();
	test_read_from_float_column();
	test_read_from_double_column();
	test_read_from_ascii_table();
	test_read_multirow();
	test_read_variable_length_array();
	test_read_noncontiguous_3d();
	test_bad_dimen_3d();
	test_read_group_parameters();
	test_read_subsection_with_null_flags();

	/* JJ (LONGLONG) tests */
	test_jj_read_primary_array();
	test_jj_read_primary_with_null_flags();
	test_jj_read_2d_array();
	test_jj_read_3d_array();
	test_jj_read_subsection();
	test_jj_read_subsection_with_null_flags();
	test_jj_read_group_parameters();
	test_jj_read_binary_table_column();
	test_jj_read_column_with_null_flags();
	test_jj_read_from_byte_column();
	test_jj_read_from_short_column();
	test_jj_read_from_long_column();
	test_jj_read_from_float_column();
	test_jj_read_from_double_column();
	test_jj_read_from_ascii_table();

	remove(test_path);

	return 0;
}
