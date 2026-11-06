/*
 * Tests for putcolui.c - unsigned short (TUSHORT) write functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcolui.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 6 };
	unsigned short data[] = { 0, 100, 1000, 10000, 50000, 65535 };
	unsigned short result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffpprui, f, 1, 1, 6, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 6, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 1000);
	fail_if(result[3] != 10000);
	fail_if(result[4] != 50000);
	fail_if(result[5] != 65535);
	call_01(ffclos, f);
}

static void
test_write_primary_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned short data[] = { 100, 65535, 200, 65535, 300 };
	unsigned short nulval = 65535;
	unsigned short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppnui, f, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 5, NULL, result, &anynull);
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
	unsigned short data[] = { 1, 2, 3, 4, 5, 6 };
	unsigned short result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 2, naxes);
	call_06(ffp2dui, f, 1, 3, 3, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 6, NULL, result, &anynull);
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
	unsigned short data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned short result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 3, naxes);
	call_08(ffp3dui, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 8, NULL, result, &anynull);
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
	unsigned short data[] = { 10, 20, 30, 40 };
	unsigned short result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 2, naxes);
	call_07(ffpssui, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 16, NULL, result, &anynull);
	/* Check that the 2x2 subsection was written at positions [1][1] to [2][2] */
	fail_if(result[5] != 10);  /* [1][1] = index 4+1 = 5 */
	fail_if(result[6] != 20);  /* [2][1] = index 4+2 = 6 */
	fail_if(result[9] != 30);  /* [1][2] = index 8+1 = 9 */
	fail_if(result[10] != 40); /* [2][2] = index 8+2 = 10 */
	call_01(ffclos, f);
}

static void
test_write_binary_table_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UICOL" };
	char *tform[] = { "1U" };
	unsigned short data[] = { 0, 32768, 65535 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 32768);
	fail_if(result[2] != 65535);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UICOL" };
	char *tform[] = { "5U" };
	unsigned short data[] = { 100, 0, 200, 0, 300 };
	unsigned short nulval = 0;
	unsigned short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 1);
	call_07(ffpcnui, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

/*
 * Write unsigned short to signed short column with TZERO=32768. This
 * exercises the XOR path in ffu2fi2
 */
static void
test_write_to_short_column(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned short data[] = { 0, 32768, 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Set TZERO=32768 to store unsigned values in signed column */
	call_04(fftscl, f, 1, 1.0, 32768.0);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_write_to_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	unsigned short data[] = { 0, 32768, 65535 };
	long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 32768);
	fail_if(result[2] != 65535);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned short data[] = { 0, 32768, 65535 };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0f);
	fail_if(result[1] != 32768.0f);
	fail_if(result[2] != 65535.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned short data[] = { 0, 32768, 65535 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 32768.0);
	fail_if(result[2] != 65535.0);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	unsigned short data[] = { 0, 32768, 65535 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 32768);
	fail_if(result[2] != 65535);
	call_01(ffclos, f);
}

/* Write unsigned short to byte column - values > 255 should overflow */
static void
test_write_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned short data[] = { 0, 127, 255 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 127);
	fail_if(result[2] != 255);
	call_01(ffclos, f);
}

/* Write unsigned short > 255 to byte column - should overflow */
static void
test_byte_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned short data[] = { 256 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclui(f, 1, 1, 1, 1, data, &status);
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
	unsigned short data[] = { 0, 32768, 65535 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0.0);
	fail_if(result[1] != 32768.0);
	fail_if(result[2] != 65535.0);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned short pdata[] = { 100, 200 };
	unsigned short idata[] = { 1, 2, 3, 4 };
	float presult[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, USHORT_IMG, 1, naxes, 2, 1, TRUE);
	call_05(ffpgpui, f, 1, 1, 2, pdata);
	call_05(ffpprui, f, 1, 1, 4, idata);
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
	/* Test scaled write path - verify write succeeds */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* scale=2.0, zero=50 => stored = (input - 50) / 2 */
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

/* Test 3D write where array dimensions differ from FITS dimensions */
static void
test_write_noncontiguous_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	/* Input array is 3x3x2 but we only write 2x2x2 */
	unsigned short data[] = {
		1, 2, 99,
		3, 4, 99,
		99, 99, 99,
		5, 6, 99,
		7, 8, 99,
		99, 99, 99
	};
	unsigned short result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 3, naxes);
	call_08(ffp3dui, f, 1, 3, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 8, NULL, result, &anynull);
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
test_write_multirow(void)
{
	/* Write data that spans multiple rows */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UICOL" };
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
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 9, NULL, result, &anynull);
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
	char *ttype[] = { "VARUI" };
	char *tform[] = { "1PU" };
	unsigned short data1[] = { 100, 200, 300 };
	unsigned short data2[] = { 400, 500 };
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
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	fail_if(result[2] != 300);
	call_05(ffgdes, f, 1, 2, &nelem, NULL);
	fail_if(nelem != 2);
	call_09(ffgcv, f, TUSHORT, 1, 2, 1, 2, NULL, &result[3], &anynull);
	fail_if(result[3] != 400);
	fail_if(result[4] != 500);
	call_01(ffclos, f);
}

static void
test_short_overflow(void)
{
	/* Write to signed short without TZERO - values > SHRT_MAX overflow */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	unsigned short data[] = { 32768 };  /* > SHRT_MAX */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* No TZERO, so 32768 > SHRT_MAX causes overflow */
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Write to unsigned longlong column with TZERO=2^63. This exercises
 * the XOR path in ffu2fi8
 */
static void
test_write_to_unsigned_longlong_column(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULLCOL" };
	char *tform[] = { "1K" };
	unsigned short data[] = { 0, 32768, 65535 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	call_01(ffclos, f);
}

static void
test_scaled_byte_underflow(void)
{
	/* With scale and zero, value becomes < 0 */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With scale=1 and zero=100, stored = (0 - 100) / 1 = -100 < 0 */
	unsigned short data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 100.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_byte_overflow(void)
{
	/* With scale and zero, value becomes > 255 */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With scale=0.1 and zero=0, stored = 65535/0.1 = 655350 > 255 */
	unsigned short data[] = { 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.1, 0.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_short_underflow(void)
{
	/* Scaled value underflows signed short */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With scale=1 and zero=100000, stored = (0 - 100000) = -100000 < SHRT_MIN */
	unsigned short data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 100000.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_short_overflow(void)
{
	/* Scaled value overflows signed short */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With scale=0.5 and zero=0, stored = 65535/0.5 = 131070 > SHRT_MAX */
	unsigned short data[] = { 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_long_underflow(void)
{
	/* Scaled value underflows INT32 */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With scale=1 and zero=3000000000, stored = (0 - 3e9) < INT_MIN */
	unsigned short data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 3000000000.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_long_overflow(void)
{
	/* Scaled value overflows INT32 */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With scale=0.00001 and zero=0, stored = 65535/0.00001 = 6.5e9 > INT_MAX */
	unsigned short data[] = { 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.00001, 0.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_longlong_underflow(void)
{
	/* Scaled value underflows LONGLONG */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With scale=1e-17 and zero=0, stored = (0 - 1e19) / 1e-17 underflows */
	unsigned short data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-17, 1e19);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_longlong_overflow(void)
{
	/* Scaled value overflows LONGLONG */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With scale=1e-17 and zero=0, stored = 65535/1e-17 = 6.5e21 > LONGLONG_MAX */
	unsigned short data[] = { 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-17, 0.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_ascii_overflow(void)
{
	/* Scaled ASCII value overflows column width */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F6.1" };
	/* With scale=0.001, 65535/0.001 = 65535000 needs 10 chars */
	unsigned short data[] = { 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_3d(void)
{
	/* Test BAD_DIMEN error when ncols < naxis1 in ffp3dui */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	unsigned short data[32];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 3, naxes);
	/* ncols=2 < naxis1=4, should fail */
	ffp3dui(f, 1, 2, 4, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_subsection(void)
{
	/* Test BAD_DIMEN error when naxis > 7 in ffpssui */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2 };
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 2, 2 };
	unsigned short data[4];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 2, naxes);
	/* naxis=8 > 7, should fail */
	ffpssui(f, 1, 8, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_btable_format(void)
{
	/* Test writing to binary table column with unsupported format */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	unsigned short data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_float(void)
{
	/* Test scaled float conversion - exercises ffu2fr4 with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	unsigned short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_scaled_double(void)
{
	/* Test scaled double conversion - exercises ffu2fr8 with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	unsigned short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

static void
test_scaled_ascii(void)
{
	/* Test scaled ASCII conversion - exercises ffu2fstr with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F10.2" };
	unsigned short data[] = { 100, 200, 300 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 50.0);
	call_06(ffpclui, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
}

/*
 * Test overflow tracking in ffpcnui with fixed-length column. Good
 * values that overflow should set the overflow flag
 */
static void
test_overflow_with_nulls(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "5I" };
	/* 65535 > SHRT_MAX when no TZERO, so it overflows */
	/* Sequence: 65535 (overflow), 0 (null), 65535 (overflow), 0 (null) */
	unsigned short data[] = { 65535, 0, 65535, 0 };
	unsigned short nulval = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	ffpcnui(f, 1, 1, 1, 4, data, nulval, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test successful scaled conversion to byte. Covers ffu2fi1 path
 * where dvalue is within valid range
 */
static void
test_scaled_byte_valid(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With scale=0.5 and zero=0, stored = (50 - 0) / 0.5 = 100, valid byte */
	unsigned short data[] = { 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclui, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

/*
 * Test scaled conversion to short with negative dvalue in valid
 * range. Covers ffu2fi2 negative rounding path
 */
static void
test_scaled_short_negative_valid(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	/* With scale=1 and zero=200, stored = (100 - 200) / 1 = -100, valid */
	unsigned short data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 200.0);
	call_06(ffpclui, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

/*
 * Test successful scaled conversion to long (int32). Covers ffu2fi4
 * path where dvalue is within valid range
 */
static void
test_scaled_long_valid(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With scale=0.5 and zero=0, stored = (1000 - 0) / 0.5 = 2000, valid */
	unsigned short data[] = { 1000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclui, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

/*
 * Test successful scaled conversion to longlong (int64). Covers
 * ffu2fi8 path where dvalue is within valid range
 */
static void
test_scaled_longlong_valid(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With scale=0.5 and zero=0, stored = (1000 - 0) / 0.5 = 2000, valid */
	unsigned short data[] = { 1000 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpclui, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

/*
 * Test ASCII table overflow with unscaled value. Covers ffu2fstr
 * overflow check in the scale=1, zero=0 path
 */
static void
test_ascii_unscaled_overflow(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FCOL" };
	char *tform[] = { "F4.1" };  /* Very narrow field */
	/* 65535 won't fit in 4 chars with decimal */
	unsigned short data[] = { 65535 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test scaled conversion to long with negative dvalue in valid range.
 * Covers ffu2fi4 negative rounding path
 */
static void
test_scaled_long_negative_valid(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	/* With scale=1 and zero=200, stored = (100 - 200) / 1 = -100, valid */
	unsigned short data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 200.0);
	call_06(ffpclui, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

/*
 * Test scaled conversion to longlong with negative dvalue in valid
 * range. Covers ffu2fi8 negative rounding path
 */
static void
test_scaled_longlong_negative_valid(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With scale=1 and zero=200, stored = (100 - 200) / 1 = -100, valid */
	unsigned short data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 200.0);
	call_06(ffpclui, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

/*
 * Test ffpcnui with a variable length array column. Covers tcode < 0
 * path in ffpcnui
 */
static void
test_variable_length_array_with_nulls(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARUI" };
	char *tform[] = { "1PU" };  /* Variable length unsigned short */
	/* Mix of good values and null marker */
	unsigned short data[] = { 100, 0, 200, 0, 300 };
	unsigned short nulval = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcnui, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);
}

/*
 * Test writing unsigned short to ASCII table string column. Covers
 * BAD_ATABLE_FORMAT return in ffpclui
 */
static void
test_bad_atable_format(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "A5" };  /* String format, not numeric */
	unsigned short data[] = { 100 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclui(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_ATABLE_FORMAT);
	status = 0;
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
	test_write_to_short_column();
	test_write_to_long_column();
	test_write_to_float_column();
	test_write_to_double_column();
	test_write_to_longlong_column();
	test_write_to_byte_column();
	test_byte_overflow();
	test_write_to_ascii_table();
	test_write_group_parameters();
	test_write_with_scaling();
	test_write_noncontiguous_3d();
	test_write_multirow();
	test_write_variable_length_array();
	test_short_overflow();
	test_write_to_unsigned_longlong_column();
	test_scaled_byte_underflow();
	test_scaled_byte_overflow();
	test_scaled_short_underflow();
	test_scaled_short_overflow();
	test_scaled_long_underflow();
	test_scaled_long_overflow();
	test_scaled_longlong_underflow();
	test_scaled_longlong_overflow();
	test_scaled_ascii_overflow();
	test_bad_dimen_3d();
	test_bad_dimen_subsection();
	test_bad_btable_format();
	test_scaled_float();
	test_scaled_double();
	test_scaled_ascii();
	test_overflow_with_nulls();
	test_scaled_byte_valid();
	test_scaled_short_negative_valid();
	test_scaled_long_valid();
	test_scaled_longlong_valid();
	test_ascii_unscaled_overflow();
	test_scaled_long_negative_valid();
	test_scaled_longlong_negative_valid();
	test_variable_length_array_with_nulls();
	test_bad_atable_format();
	remove(test_path);

	return 0;
}
