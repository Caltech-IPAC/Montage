/*
 * Tests for putcole.c - float precision write functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcole.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { -1.5e30f, -1.0f, 0.0f, 1.0f, 1.5e30f };
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != data[0]);
	fail_if(result[2] != 0.0f);
	fail_if(result[4] != data[4]);
	call_01(ffclos, f);
}

static void
test_write_primary_with_null(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { 1.0f, 2.0f, -999.0f, 4.0f, 5.0f };
	float nulval = -999.0f;
	float result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_06(ffppne, f, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpfe, f, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[1] != 2.0f);
	fail_if(nularray[2] != 1);  /* Should be null. */
	fail_if(result[3] != 4.0f);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3 };
	float data[] = {
		1.1f, 2.2f, 3.3f,
		4.4f, 5.5f, 6.6f,
		7.7f, 8.8f, 9.9f
	};
	float result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_06(ffp2de, f, 1, 3, 3, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 9, 0, result, &anynull);
	fail_if(fabsf(result[0] - 1.1f) > 1e-5f);
	fail_if(fabsf(result[8] - 9.9f) > 1e-5f);
	call_01(ffclos, f);
}

static void
test_write_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
	float result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 3, naxes);
	call_08(ffp3de, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 8, 0, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[7] != 8.0f);
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
	float data[] = { 10.5f, 20.5f, 30.5f, 40.5f };
	float result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_07(ffpsse, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 16, 0, result, &anynull);
	fail_if(fabsf(result[5] - 10.5f) > 1e-5f);
	fail_if(fabsf(result[6] - 20.5f) > 1e-5f);
	fail_if(fabsf(result[9] - 30.5f) > 1e-5f);
	fail_if(fabsf(result[10] - 40.5f) > 1e-5f);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long pcount = 2;
	float params[] = { 3.14159f, 2.71828f };
	float result[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, FLOAT_IMG, 1, naxes, pcount, 1, TRUE);
	call_05(ffpgpe, f, 1, 1, 2, params);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpe, f, 1, 1, 2, result);
	fail_if(fabsf(result[0] - 3.14159f) > 1e-5f);
	fail_if(fabsf(result[1] - 2.71828f) > 1e-5f);
	call_01(ffclos, f);
}

static void
test_write_binary_table_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { -1e30f, 0.0f, 1e30f };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[0] != data[0]);
	fail_if(result[1] != 0.0f);
	fail_if(result[2] != data[2]);
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { 1.0f, -999.0f, 3.0f, -999.0f, 5.0f };
	float nulval = -999.0f;
	float result[5];
	char nularray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_07(ffpcne, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfe, f, 1, 1, 1, 5, result, nularray, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(nularray[1] != 1);
	fail_if(result[2] != 3.0f);
	fail_if(nularray[3] != 1);
	fail_if(result[4] != 5.0f);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_write_to_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	float data[] = { 0.0f, 127.0f, 255.0f };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
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
	float data[] = { -32768.0f, 0.0f, 32767.0f };
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
	fail_if(result[0] != -32768);
	fail_if(result[1] != 0);
	fail_if(result[2] != 32767);
	call_01(ffclos, f);
}

static void
test_write_to_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	float data[] = { -1000000.0f, 0.0f, 1000000.0f };
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
	fail_if(result[0] != -1000000);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1000000);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	float data[] = { -1.5f, 0.0f, 1.5f };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(fabs(result[0] - (-1.5)) > 1e-6);
	fail_if(result[1] != 0.0);
	fail_if(fabs(result[2] - 1.5) > 1e-6);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	float data[] = { -1e10f, 0.0f, 1e10f };
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
	fail_if(result[1] != 0);
	call_01(ffclos, f);
}

static void
test_write_to_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "E15.7" };
	float data[] = { -1.23456e10f, 0.0f, 9.87654e-5f };
	float result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(result[1] != 0.0f);
	call_01(ffclos, f);
}

static void
test_write_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1E" };
	float data[] = { 100.0f };  /* Stored as (100 - 10) / 2 = 45. */
	float result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_06(ffpcle, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 100.0f);
	call_01(ffclos, f);
}

static void
test_write_complex_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CPLXCOL" };
	char *tform[] = { "1C" };  /* Single precision complex. */
	float data[] = { 1.5f, 2.5f, 3.5f, 4.5f };  /* Two complex numbers. */
	float result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclc, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 4, 0, result, &anynull);
	fail_if(result[0] != 1.5f);
	fail_if(result[1] != 2.5f);
	fail_if(result[2] != 3.5f);
	fail_if(result[3] != 4.5f);
	call_01(ffclos, f);
}

static void
test_write_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLTVEC" };
	char *tform[] = { "3E" };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
	float result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 9, 0, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[4] != 5.0f);
	fail_if(result[8] != 9.0f);
	call_01(ffclos, f);
}

static void
test_overflow_to_byte(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	float data[] = { -1.0f, 256.0f };  /* Both overflow. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	ffpcle(f, 1, 1, 1, 2, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_overflow_to_short(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	float data[] = { -40000.0f, 40000.0f };  /* Both overflow. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	ffpcle(f, 1, 1, 1, 2, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_overflow_to_long(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	float data[] = { -3e9f, 3e9f };  /* Both overflow 32-bit. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	ffpcle(f, 1, 1, 1, 2, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_overflow_to_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	float data[] = { 1e19f };  /* Overflows LONGLONG_MAX. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_overflow_to_byte(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	/* With scale=0.1, 100/0.1 = 1000 > 255. */
	float data[] = { 100.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.1, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_overflow_to_short(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	/* With scale=0.001, 100/0.001 = 100000 > SHRT_MAX. */
	float data[] = { 100.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_overflow_to_long(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	/* With scale=0.0000001, 1000/0.0000001 = 1e10 > INT32_MAX. */
	float data[] = { 1000.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.0000001, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_overflow_to_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	/* With scale=1e-15, 100/1e-15 = 1e17, still fits. Try larger. */
	/* With scale=1e-18, 1e10/1e-18 = 1e28 > LONGLONG_MAX. */
	float data[] = { 1e10f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-18, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	float data[32];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 3, naxes);
	/* ncols=2 < naxis1=4, should fail with BAD_DIMEN. */
	ffp3de(f, 1, 2, 4, 4, 4, 2, data, &status);
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
	float data[4];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	/* naxis=8 > 7, should fail with BAD_DIMEN. */
	ffpsse(f, 1, 8, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_variable_length_array(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PE" };
	float data1[] = { 1.1f, 2.2f, 3.3f };
	float data2[] = { 10.0f, 20.0f };
	float result[3];
	long nelem;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, data1);
	call_06(ffpcle, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_08(ffgcve, f, 1, 1, 1, 3, 0, result, &anynull);
	fail_if(fabsf(result[0] - 1.1f) > 1e-5f);
	fail_if(fabsf(result[2] - 3.3f) > 1e-5f);
	call_01(ffclos, f);
}

static void
test_write_varlen_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PE" };
	float data[] = { 1.0f, -999.0f, 3.0f };
	float nulval = -999.0f;
	float result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_07(ffpcne, f, 1, 1, 1, 3, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfe, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(nularray[1] != 1);
	fail_if(result[2] != 3.0f);
	call_01(ffclos, f);
}

static void
test_write_nan_values(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float nan_val = NAN;
	float data[] = { 1.0f, nan_val, 3.0f };
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
	fail_if(result[0] != 1.0f);
	fail_if(nularray[1] != 1);  /* NaN should be flagged as null. */
	fail_if(result[2] != 3.0f);
	call_01(ffclos, f);
}

static void
test_write_2d_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 3 };
	/* Input array is 4 columns wide but we only write 2 columns. */
	float data[] = {
		1.0f, 2.0f, 99.0f, 99.0f,
		3.0f, 4.0f, 99.0f, 99.0f,
		5.0f, 6.0f, 99.0f, 99.0f
	};
	float result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	/* ncols=4, naxis1=2, naxis2=3. */
	call_06(ffp2de, f, 1, 4, 2, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 6, 0, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[1] != 2.0f);
	fail_if(result[2] != 3.0f);
	fail_if(result[3] != 4.0f);
	fail_if(result[4] != 5.0f);
	fail_if(result[5] != 6.0f);
	call_01(ffclos, f);
}

static void
test_write_3d_noncontiguous(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	/* nrows=3 > naxis2=2, so there's padding between planes. */
	float data[] = {
		1.0f, 2.0f,
		3.0f, 4.0f,
		99.0f, 99.0f,  /* Padding row. */
		5.0f, 6.0f,
		7.0f, 8.0f,
		99.0f, 99.0f   /* Padding row. */
	};
	float result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 3, naxes);
	/* ncols=2, nrows=3, naxis1=2, naxis2=2, naxis3=2. */
	call_08(ffp3de, f, 1, 2, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 8, 0, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[3] != 4.0f);
	fail_if(result[4] != 5.0f);
	fail_if(result[7] != 8.0f);
	call_01(ffclos, f);
}

static void
test_write_7d_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2, 1, 1, 1, 1 };
	long fpixel[] = { 1, 1, 1, 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 1, 1, 1, 1 };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
	float result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 7, naxes);
	call_07(ffpsse, f, 1, 7, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpve, f, 1, 1, 8, 0, result, &anynull);
	for (int i = 0; i < 8; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULLCOL" };
	char *tform[] = { "1K" };
	float data[] = { 0.0f, 1e10f, 1e15f };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Set TZERO to 2^63 to make it unsigned longlong. */
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	call_06(ffpcle, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 3, 0, result, &anynull);
	call_01(ffclos, f);
}

static void
test_unsigned_longlong_negative_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULLCOL" };
	char *tform[] = { "1K" };
	float data[] = { -1.0f };  /* Negative overflows unsigned. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_table_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F6.2" };
	/* Value 12345.67 needs more than 6 characters. */
	float data[] = { 12345.67f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcle(f, 1, 1, 1, 1, data, &status);
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
	char *tform[] = { "1L" };  /* Logical column. */
	float data[] = { 1.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_nulls_at_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { 1.0f, 2.0f, -999.0f, -999.0f };
	float nulval = -999.0f;
	float result[4];
	char nularray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_07(ffpcne, f, 1, 1, 1, 4, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfe, f, 1, 1, 1, 4, result, nularray, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[1] != 2.0f);
	fail_if(nularray[2] != 1);
	fail_if(nularray[3] != 1);
	call_01(ffclos, f);
}

static void
test_all_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { -999.0f, -999.0f, -999.0f };
	float nulval = -999.0f;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_07(ffpcne, f, 1, 1, 1, 3, data, nulval);
	call_01(ffclos, f);
}

static void
test_large_vector(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BIGCOL" };
	char *tform[] = { "10000E" };
	float *data;
	float *result;
	int anynull;
	int i;

	data = malloc(10000 * sizeof *data);
	result = malloc(10000 * sizeof *result);
	fail_if(data == NULL || result == NULL);

	for (i = 0; i < 10000; i += 1) {
		data[i] = i * 0.001f;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 10000, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 10000, 0, result, &anynull);
	for (i = 0; i < 10000; i += 1) {
		fail_if(fabsf(result[i] - data[i]) > 1e-6f);
	}
	call_01(ffclos, f);

	free(data);
	free(result);
}

/* Test ffpcne with complex column (repeat *= 2) */
static void
test_pcne_complex_column(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CPLXCOL" };
	char *tform[] = { "3C" };  /* 3 float complex values. */
	float data[] = { 1.0f, 2.0f, -999.0f, -999.0f, 5.0f, 6.0f };
	float nulval = -999.0f;
	float result[6];
	char nularray[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_07(ffpcne, f, 1, 1, 1, 6, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfe, f, 1, 1, 1, 6, result, nularray, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(nularray[2] != 1);
	fail_if(result[4] != 5.0f);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

/* Test underflow with scaling to byte column */
static void
test_scaled_underflow_to_byte(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	float data[] = { -10.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.1, 10.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/* Test normal scaled conversion to byte */
static void
test_scaled_normal_to_byte(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	float data[] = { 50.0f };
	unsigned int result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 0.0);
	call_06(ffpcle, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvuk, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(result[0] != 100);
	call_01(ffclos, f);
}

/* Test underflow with scaling to short */
static void
test_scaled_underflow_to_short(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	float data[] = { -100.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/* Test normal scaled conversion to short */
static void
test_scaled_normal_to_short(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	float data[] = { 50.0f, -50.0f };
	short result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.1, 0.0);
	call_06(ffpcle, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvi, f, 1, 1, 1, 2, 0, result, &anynull);
	fail_if(result[0] != 500);
	fail_if(result[1] != -500);
	call_01(ffclos, f);
}

/* Test underflow with scaling to long */
static void
test_scaled_underflow_to_long(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	float data[] = { -1000.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.0000001, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/* Test normal scaled conversion to long */
static void
test_scaled_normal_to_long(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	float data[] = { 100.0f, -100.0f };
	long result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.01, 0.0);
	call_06(ffpcle, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 2, 0, result, &anynull);
	fail_if(result[0] != 10000);
	fail_if(result[1] != -10000);
	call_01(ffclos, f);
}

/* Test underflow with scaling to longlong */
static void
test_scaled_underflow_to_longlong(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	float data[] = { -100.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-18, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/* Test normal scaled conversion to longlong */
static void
test_scaled_normal_to_longlong(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	float data[] = { 100.0f, -100.0f };
	LONGLONG result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	call_06(ffpcle, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvjj, f, 1, 1, 1, 2, 0, result, &anynull);
	fail_if(result[0] != 100000LL);
	fail_if(result[1] != -100000LL);
	call_01(ffclos, f);
}

/* Test scaled conversion to double */
static void
test_scaled_write_to_double_column(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	float data[] = { 100.0f };
	double result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 10.0);
	call_06(ffpcle, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvd, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(fabs(result[0] - 45.0) > 0.01);
	call_01(ffclos, f);
}

/* Test scaled conversion to ASCII */
static void
test_scaled_ascii_column(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "E15.6" };
	float data[] = { 100.0f };
	float result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 10.0);
	call_06(ffpcle, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcve, f, 1, 1, 1, 1, 0, result, &anynull);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

/*
 * Test unsigned longlong positive overflow. Input > 2*DLONGLONG_MAX
 * overflows
 */
static void
test_unsigned_longlong_positive_overflow(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULLCOL" };
	char *tform[] = { "1K" };
	float data[] = { 2e19f };  /* > 2 * 9.2e18. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/* Test scaled ASCII table overflow */
static void
test_scaled_ascii_overflow(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F6.2" };
	/* With scale=0.01, stored = 100/0.01 = 10000, which needs >6 chars. */
	float data[] = { 100.0f };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.01, 0.0);
	ffpcle(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

/* Test unscaled LONGLONG underflow */
static void
test_unscaled_longlong_underflow(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	float data[] = { -1e19f };  /* < LONGLONG_MIN. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* No scaling - scale=1, zero=0. */
	ffpcle(f, 1, 1, 1, 1, data, &status);
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
	test_write_column_with_nulls();
	test_write_to_byte_column();
	test_write_to_short_column();
	test_write_to_long_column();
	test_write_to_double_column();
	test_write_to_longlong_column();
	test_write_to_ascii_table();
	test_write_with_scaling();
	test_write_complex_column();
	test_write_multirow();
	test_overflow_to_byte();
	test_overflow_to_short();
	test_overflow_to_long();
	test_overflow_to_longlong();
	test_scaled_overflow_to_byte();
	test_scaled_overflow_to_short();
	test_scaled_overflow_to_long();
	test_scaled_overflow_to_longlong();
	test_bad_dimen_3d();
	test_bad_dimen_subsection();
	test_write_variable_length_array();
	test_write_varlen_with_nulls();
	test_write_nan_values();
	test_write_2d_noncontiguous();
	test_write_3d_noncontiguous();
	test_write_7d_subsection();
	test_write_to_unsigned_longlong_column();
	test_unsigned_longlong_negative_overflow();
	test_ascii_table_overflow();
	test_bad_btable_format();
	test_nulls_at_end();
	test_all_nulls();
	test_large_vector();
	test_pcne_complex_column();
	test_scaled_underflow_to_byte();
	test_scaled_normal_to_byte();
	test_scaled_underflow_to_short();
	test_scaled_normal_to_short();
	test_scaled_underflow_to_long();
	test_scaled_normal_to_long();
	test_scaled_underflow_to_longlong();
	test_scaled_normal_to_longlong();
	test_scaled_write_to_double_column();
	test_scaled_ascii_column();
	test_unsigned_longlong_positive_overflow();
	test_scaled_ascii_overflow();
	test_unscaled_longlong_underflow();
	remove(test_path);

	return 0;
}
