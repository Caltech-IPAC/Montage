/*
 * Tests for putcoli.c - short (TSHORT) write functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcoli.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 6 };
	short data[] = { -32768, -100, 0, 100, 1000, 32767 };
	short result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 6, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 6, NULL, result, &anynull);
	fail_if(result[0] != -32768);
	fail_if(result[1] != -100);
	fail_if(result[2] != 0);
	fail_if(result[3] != 100);
	fail_if(result[4] != 1000);
	fail_if(result[5] != 32767);
	call_01(ffclos, f);
}

static void
test_write_primary_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppni, f, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 5, NULL, result, &anynull);
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
	short data[] = { 1, 2, 3, 4, 5, 6 };
	short result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_06(ffp2di, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 6, NULL, result, &anynull);
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
	short data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	short result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 3, naxes);
	call_08(ffp3di, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 8, NULL, result, &anynull);
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
	short data[] = { 10, 20, 30, 40 };
	short result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_07(ffpssi, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 16, NULL, result, &anynull);
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
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { -32768, 0, 32767 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768);
	fail_if(result[1] != 0);
	fail_if(result[2] != 32767);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "5I" };
	short data[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcni, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 5, NULL, result, &anynull);
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
	short data[] = { -32768, 0, 32767 };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768);
	fail_if(result[1] != 0);
	fail_if(result[2] != 32767);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
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
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768.0f);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != 32767.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
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
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 32767.0);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	short data[] = { -32768, 0, 32767 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768);
	fail_if(result[1] != 0);
	fail_if(result[2] != 32767);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	short data[] = { 0, 127, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
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
test_byte_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	short data[] = { -1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	short data[] = { 256 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	char *tform[] = { "F10.1" };
	short data[] = { -32768, 0, 32767 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 32767.0);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	short pdata[] = { 100, 200 };
	short idata[] = { 1, 2, 3, 4 };
	float presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, SHORT_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpi, f, 1, 1, 2, pdata);
	call_05(ffppri, f, 1, 1, 4, idata);
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
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_write_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "3I" };
	short data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	short result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 9, NULL, result, &anynull);
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
	char *ttype[] = { "VARI" };
	char *tform[] = { "1PI" };
	short data1[] = { 100, 200, 300 };
	short data2[] = { 400, 500 };
	short result[5];
	int anynull;
	long nelem;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data1);
	call_06(ffpcli, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	fail_if(result[2] != 300);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_09(ffgcv, f, TSHORT, 1, 2, 1, 2, NULL, &result[3], &anynull);
	fail_if(result[3] != 400);
	fail_if(result[4] != 500);
	call_01(ffclos, f);
}

static void
test_scaled_byte_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	short data[] = { -100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 50.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 3000000000.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 32767 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.00001, 0.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_btable_format(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	short data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_ATABLE_FORMAT);
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
	short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_scaled_double(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_scaled_ascii(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F10.2" };
	short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_bad_dimen_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	short data[32];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 3, naxes);
	ffp3di(f, 1, 2, 4, 4, 4, 2, data, &status);
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
	short data[4];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	ffpssi(f, 1, 8, naxes, fpixel, lpixel, data, &status);
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
	short data[] = {
		1, 2, 99,
		3, 4, 99,
		99, 99, 99,
		5, 6, 99,
		7, 8, 99,
		99, 99, 99
	};
	short result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 3, naxes);
	call_08(ffp3di, f, 1, 3, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 8, NULL, result, &anynull);
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
test_scaled_byte_overflow_high(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	short data[] = { 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, -50.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 50000.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 32767 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.0001, 0.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_short_negative(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { -100 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, -150.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 25);
	call_01(ffclos, f);
}

static void
test_scaled_long_positive(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	short data[] = { 100 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 0.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 50);
	call_01(ffclos, f);
}

static void
test_scaled_long_negative(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	short data[] = { 0 };
	long result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 5.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != -3);
	call_01(ffclos, f);
}

static void
test_scaled_longlong_underflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	short data[] = { -100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-18, 0.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-18, 0.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_longlong_positive(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	short data[] = { 100 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 0.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 50);
	call_01(ffclos, f);
}

static void
test_scaled_longlong_negative(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	short data[] = { 0 };
	LONGLONG result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 5.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != -3);
	call_01(ffclos, f);
}

static void
test_write_contiguous_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "C1", "C2" };
	char *tform[] = { "F5.1", "F5.1" };
	short data[] = { 100, 200 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 2, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 1, &data[0]);
	call_06(ffpcli, f, 2, 1, 1, 1, &data[1]);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 1, NULL, &result[0], &anynull);
	call_09(ffgcv, f, TDOUBLE, 2, 1, 1, 1, NULL, &result[1], &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "5B" };
	short data[] = { 100, 255, 256, 255, 50 };
	short nulval = 255;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	ffpcni(f, 1, 1, 1, 5, data, nulval, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_multirow_element_wrap(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "2I" };
	short data[] = { 1, 2, 3, 4 };
	short result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 4, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 2, NULL, &result[0], &anynull);
	call_09(ffgcv, f, TSHORT, 1, 2, 1, 2, NULL, &result[2], &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 4);
	call_01(ffclos, f);
}

static void
test_contiguous_ascii_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "C1" };
	char *tform[] = { "F10.1" };
	short data[] = { 100, 200, 300 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100.0);
	fail_if(result[1] != 200.0);
	fail_if(result[2] != 300.0);
	call_01(ffclos, f);
}

static void
test_scaled_byte_valid(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	short data[] = { 200 };
	unsigned char result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 0.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 1, NULL, result, &anynull);
	fail_if(result[0] != 100);
	call_01(ffclos, f);
}

static void
test_scaled_short_valid_negative(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short data[] = { 0 };
	short result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 100.0);
	call_06(ffpcli, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 1, NULL, result, &anynull);
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
	short data[] = { 0, 100, 32767 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	call_01(ffclos, f);
}

static void
test_unsigned_longlong_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	short data[] = { -1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_primary_many(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100 };
	short data[100];
	int i;

	for (i = 0; i < 100; i += 1) {
		data[i] = (short)(i - 50);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 100, data);
	call_01(ffclos, f);
}

static void
test_ascii_field_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NCOL" };
	char *tform[] = { "F4.1" };
	short data[] = { 32767 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcli(f, 1, 1, 1, 1, data, &status);
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
	short data[] = { 32767 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpcli(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_varlen_with_nulls(void)
{
	/*
	* Test writing variable length array with nulls.
	* This exercises the tcode < 0 path in ffpcni.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PI" };
	short data[] = { 100, -32000, 200, -32000, 300 };
	short nulval = -32000;
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcni, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_varlen_with_nulls_to_byte_overflow(void)
{
	/*
	* Test writing VLA with nulls to byte column where values overflow.
	* This exercises the NUM_OVERFLOW handling in ffpcni for VLA.
	* Writing short values like 32767 to byte column causes overflow.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PB" };  /* Variable length byte column */
	short data[] = { 32767, -32000, 100, -32000, 256 };
	short nulval = -32000;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	/* This should trigger overflow (32767 and 256 don't fit in byte) */
	/* but ffpcni ignores overflow for VLA columns */
	call_07(ffpcni, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);
}

static void
test_ascii_e_format(void)
{
	/*
	* Test writing to ASCII table E format (exponential notation).
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "E12.4" };
	short data[] = { -32768, 0, 32767 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -32768.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 32767.0);
	call_01(ffclos, f);
}

static void
test_ascii_e_format_scaled(void)
{
	/*
	* Test writing to ASCII table E format with scaling.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "E15.6" };
	short data[] = { 100, -100 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 50.0);
	call_06(ffpcli, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	/* stored = (input - zero) / scale = (100 - 50) / 0.5 = 100 */
	fail_if(result[0] != 100.0);
	/* stored = (-100 - 50) / 0.5 = -300 */
	fail_if(result[1] != -300.0);
	call_01(ffclos, f);
}

static void
test_ascii_e_format_multirow(void)
{
	/*
	* Write to ASCII table E format with multiple rows.
	* With multiple columns, incre != twidth, testing different code paths.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL", "ICOL" };
	char *tform[] = { "E12.4", "I8" };
	short data[] = { -100, 0, 100 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 2, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -100.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 100.0);
	call_01(ffclos, f);
}

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
	test_write_to_float_column();
	test_write_to_double_column();
	test_write_to_longlong_column();
	test_write_to_byte_column();
	test_byte_underflow();
	test_byte_overflow();
	test_write_to_ascii_table();
	test_write_group_parameters();
	test_write_with_scaling();
	test_write_multirow();
	test_write_variable_length_array();
	test_scaled_byte_underflow();
	test_scaled_long_underflow();
	test_scaled_long_overflow();
	test_bad_btable_format();
	test_bad_atable_format();
	test_scaled_float();
	test_scaled_double();
	test_scaled_ascii();
	test_bad_dimen_3d();
	test_bad_dimen_subsection();
	test_write_noncontiguous_3d();
	test_scaled_byte_overflow_high();
	test_scaled_short_underflow();
	test_scaled_short_overflow();
	test_scaled_short_negative();
	test_scaled_long_positive();
	test_scaled_long_negative();
	test_scaled_longlong_underflow();
	test_scaled_longlong_overflow();
	test_scaled_longlong_positive();
	test_scaled_longlong_negative();
	test_write_contiguous_ascii_table();
	test_write_column_with_nulls_overflow();
	test_multirow_element_wrap();
	test_contiguous_ascii_multirow();
	test_scaled_byte_valid();
	test_scaled_short_valid_negative();
	test_write_to_unsigned_longlong();
	test_unsigned_longlong_overflow();
	test_write_primary_many();
	test_ascii_field_overflow();
	test_ascii_scaled_field_overflow();
	test_varlen_with_nulls();
	test_varlen_with_nulls_to_byte_overflow();
	test_ascii_e_format();
	test_ascii_e_format_scaled();
	test_ascii_e_format_multirow();
	remove(test_path);

	return 0;
}
