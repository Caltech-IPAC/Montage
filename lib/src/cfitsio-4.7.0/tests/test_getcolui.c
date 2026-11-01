/*
 * Tests for getcolui.c - unsigned short (TUSHORT) read functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcolui.fits"

static void
test_read_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned short data[] = { 0, 100, 1000, 10000, USHRT_MAX };
	unsigned short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffpprui, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvui, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 1000);
	fail_if(result[3] != 10000);
	fail_if(result[4] != USHRT_MAX);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_primary_no_nulls(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned short data[] = { 10, 20, 30, 40, 50 };
	unsigned short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffpprui, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpvui, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_primary_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned short data[] = { 10, 20, 30, 40, 50 };
	unsigned short result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffpprui, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfui, f, 1, 1, 5, result, nularray, &anynull);
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
	unsigned short data[] = { 1, 2, 3, 4, 5, 6 };
	unsigned short result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 2, naxes);
	call_06(ffp2dui, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffg2dui, f, 1, 0, 3, 3, 2, result, &anynull);
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
	unsigned short data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned short result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 3, naxes);
	call_08(ffp3dui, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dui, f, 1, 0, 2, 2, 2, 2, 2, result, &anynull);
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
	unsigned short data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	unsigned short result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 2, naxes);
	call_05(ffpprui, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsvui, f, 1, 2, naxes, fpixel, lpixel, inc, 0, result, &anynull);
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
	char *ttype[] = { "USCOL" };
	char *tform[] = { "1U" };
	unsigned short data[] = { 0, 32768, USHRT_MAX };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 32768);
	fail_if(result[2] != USHRT_MAX);
	call_01(ffclos, f);
}

static void
test_read_column_no_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "USCOL" };
	char *tform[] = { "5U" };
	unsigned short data[] = { 10, 20, 30, 40, 50 };
	unsigned short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "USCOL" };
	char *tform[] = { "5U" };
	unsigned short data[] = { 10, 20, 30, 40, 50 };
	unsigned short result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfui, f, 1, 1, 1, 5, result, nularray, &anynull);
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
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
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
	short data[] = { 0, 1000, 32767 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 32767);
	call_01(ffclos, f);
}

static void
test_read_from_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	long data[] = { 0, 10000, 60000 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 10000);
	fail_if(result[2] != 60000);
	call_01(ffclos, f);
}

static void
test_read_from_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	float data[] = { 0.0f, 1000.0f, 50000.0f };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 50000);
	call_01(ffclos, f);
}

static void
test_read_from_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	double data[] = { 0.0, 1000.0, 50000.0 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 50000);
	call_01(ffclos, f);
}

static void
test_read_from_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F12.1" };
	double data[] = { 0.0, 1000.0, 50000.0 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 50000);
	call_01(ffclos, f);
}

static void
test_read_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "USCOL" };
	char *tform[] = { "3U" };
	unsigned short data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	unsigned short result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 9, 0, result, &anynull);
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
	char *ttype[] = { "VARUS" };
	char *tform[] = { "1PU" };
	unsigned short data1[] = { 10, 20, 30 };
	unsigned short data2[] = { 40, 50 };
	unsigned short result[5];
	int anynull;
	long nelem;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data1);
	call_06(ffpclui, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_08(ffgcvui, f, 1, 2, 1, 2, 0, &result[3], &anynull);
	fail_if(result[3] != 40);
	fail_if(result[4] != 50);
	call_01(ffclos, f);
}

static void
test_short_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { -1 };
	unsigned short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvui(f, 1, 1, 1, 1, 0, result, &anynull, &status);
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
	long data[] = { 100000 };
	unsigned short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvui(f, 1, 1, 1, 1, 0, result, &anynull, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_float_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	float data[] = { -1.0f };
	unsigned short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvui(f, 1, 1, 1, 1, 0, result, &anynull, &status);
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
	double data[] = { -1.0 };
	unsigned short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvui(f, 1, 1, 1, 1, 0, result, &anynull, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_from_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 0, 1000, 65535 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvui, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 1000);
	fail_if(result[2] != 65535);
	call_01(ffclos, f);
}

static void
test_longlong_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { -1 };
	unsigned short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvui(f, 1, 1, 1, 1, 0, result, &anynull, &status);
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
	unsigned short data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned short result[18];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 3, naxes);
	call_08(ffp3dui, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffg3dui, f, 1, 0, 3, 3, 2, 2, 2, result, &anynull);
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
	unsigned short result[32];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 3, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffg3dui(f, 1, 0, 2, 2, 4, 4, 2, result, &anynull, &status);
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
	unsigned short pdata[] = { 10, 20 };
	unsigned short idata[] = { 1, 2, 3, 4 };
	unsigned short presult[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, USHORT_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpui, f, 1, 1, 2, pdata);
	call_05(ffpprui, f, 1, 1, 4, idata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpui, f, 1, 1, 2, presult);
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
	unsigned short data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	unsigned short result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 2, naxes);
	call_05(ffpprui, f, 1, 1, 16, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_10(ffgsfui, f, 1, 2, naxes, fpixel, lpixel, inc, result, nularray, &anynull);
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
	test_read_from_long_column();
	test_read_from_longlong_column();
	test_read_from_float_column();
	test_read_from_double_column();
	test_read_from_ascii_table();
	test_read_multirow();
	test_read_variable_length_array();
	test_short_overflow();
	test_long_overflow();
	test_longlong_overflow();
	test_float_overflow();
	test_double_overflow();
	test_read_noncontiguous_3d();
	test_bad_dimen_3d();
	test_read_group_parameters();
	test_read_subsection_with_null_flags();
	remove(test_path);

	return 0;
}
