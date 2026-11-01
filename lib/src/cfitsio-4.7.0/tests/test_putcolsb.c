/*
 * Tests for putcolsb.c signed byte write functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcolsb.fits"

static void
test_write_primary_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	signed char data[] = { -128, -64, -1, 0, 1, 64, 100, 127, -100, -50 };
	signed char result[10];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 1, naxes);
	call_05(ffpprsb, f, 1, 1, 10, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 10, NULL, result, &anynull);
	for (int i = 0; i < 10; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_write_primary_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	signed char data[] = { 10, -99, 20, -99, 30 };
	signed char nulval = -99;
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppnsb, f, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_write_2d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3 };
	signed char data[] = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9
	};
	signed char result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 2, naxes);
	call_06(ffp2dsb, f, 1, 3, 3, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 9, NULL, result, &anynull);
	for (int i = 0; i < 9; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_write_3d_array(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	signed char data[] = { -1, -2, -3, -4, -5, -6, -7, -8 };
	signed char result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 3, naxes);
	call_08(ffp3dsb, f, 1, 2, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 8, NULL, result, &anynull);
	for (int i = 0; i < 8; i += 1) {
		fail_if(result[i] != data[i]);
	}
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
	signed char data[] = { 10, 20, 30, 40 };
	signed char result[16];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 2, naxes);
	call_07(ffpsssb, f, 1, 2, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 16, NULL, result, &anynull);
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
	char *ttype[] = { "SBCOL" };
	char *tform[] = { "5S" };
	signed char data[] = { -10, -20, -30, -40, -50 };
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 5, NULL, result, &anynull);
	for (int i = 0; i < 5; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_write_column_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SBCOL" };
	char *tform[] = { "5S" };
	signed char data[] = { 1, -99, 3, -99, 5 };
	signed char nulval = -99;
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcnsb, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[2] != 3);
	fail_if(result[4] != 5);
	call_01(ffclos, f);
}

static void
test_write_to_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	signed char data[] = { -128, 0, 127 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -128);
	fail_if(result[1] != 0);
	fail_if(result[2] != 127);
	call_01(ffclos, f);
}

static void
test_write_to_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LONGCOL" };
	char *tform[] = { "1J" };
	signed char data[] = { -50, 50 };
	long result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -50);
	fail_if(result[1] != 50);
	call_01(ffclos, f);
}

static void
test_write_to_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	signed char data[] = { -100, 100 };
	float result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -100.0f);
	fail_if(result[1] != 100.0f);
	call_01(ffclos, f);
}

static void
test_write_to_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	signed char data[] = { -127, 127 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -127.0);
	fail_if(result[1] != 127.0);
	call_01(ffclos, f);
}

static void
test_write_to_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	signed char data[] = { -128, 0, 127 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -128);
	fail_if(result[1] != 0);
	fail_if(result[2] != 127);
	call_01(ffclos, f);
}

static void
test_write_to_ascii_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F8.2" };
	signed char data[] = { -99, 0, 50 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -99.0);
	fail_if(result[1] != 0.0);
	fail_if(result[2] != 50.0);
	call_01(ffclos, f);
}

static void
test_write_group_parameters(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long pcount = 2;
	signed char params[] = { -10, 20 };
	signed char result[2];

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, TRUE, SHORT_IMG, 1, naxes, pcount, 1, TRUE);
	call_05(ffpgpsb, f, 1, 1, 2, params);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffggpsb, f, 1, 1, 2, result);
	fail_if(result[0] != -10);
	fail_if(result[1] != 20);
	call_01(ffclos, f);
}

static void
test_write_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALEDCOL" };
	char *tform[] = { "1I" };
	signed char data[] = { -100, 0, 100 };
	short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 10.0);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -55);
	fail_if(result[1] != -5);
	fail_if(result[2] != 45);
	call_01(ffclos, f);
}

static void
test_write_noncontiguous_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	signed char data[] = {
		1, 2, 99, 99,
		3, 4, 99, 99,
		5, 6, 99, 99,
		7, 8, 99, 99
	};
	signed char result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 3, naxes);
	call_08(ffp3dsb, f, 1, 4, 2, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 8, NULL, result, &anynull);
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
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "2S" };
	signed char data[] = { 1, 2, 3, 4, 5, 6 };
	signed char result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 6, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 6, NULL, result, &anynull);
	for (int i = 0; i < 6; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_write_to_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STRCOL" };
	char *tform[] = { "4A" };
	signed char data[] = { 'T', 'E', 'S', 'T' };
	char result[5] = {0};
	char *rptr = result;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 4, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "TEST") != 0);
	call_01(ffclos, f);
}

static void
test_write_scaled_float(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FCOL" };
	char *tform[] = { "1E" };
	signed char data[] = { 10, 20 };
	float result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 5.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != 10.0f);
	fail_if(result[1] != 30.0f);
	call_01(ffclos, f);
}

static void
test_write_scaled_double(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	signed char data[] = { -50, 50 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 0.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -25.0);
	fail_if(result[1] != 25.0);
	call_01(ffclos, f);
}

static void
test_write_scaled_ascii(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F10.2" };
	signed char data[] = { 10, 20 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.1, 100.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -900.0);
	fail_if(result[1] != -800.0);
	call_01(ffclos, f);
}

static void
test_write_variable_length_array(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PS" };
	signed char data1[] = { 1, 2, 3 };
	signed char data2[] = { 10, 20 };
	signed char result[3];
	long nelem;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data1);
	call_06(ffpclsb, f, 1, 2, 1, 2, data2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_05(ffgdes, f, 1, 1, &nelem, NULL);
	fail_if(nelem != 3);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	call_01(ffclos, f);
}

static void
test_write_varlen_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PS" };
	signed char data[] = { 1, -99, 3, -99, 5 };
	signed char nulval = -99;
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcnsb, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[2] != 3);
	fail_if(result[4] != 5);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UBCOL" };
	char *tform[] = { "1B" };
	signed char data[] = { 0, 50, 127 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 50);
	fail_if(result[2] != 127);
	call_01(ffclos, f);
}

static void
test_write_scaled_short(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCOL" };
	char *tform[] = { "1I" };
	signed char data[] = { -100, 100 };
	short result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 50.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -300);
	fail_if(result[1] != 100);
	call_01(ffclos, f);
}

static void
test_write_scaled_long(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LCOL" };
	char *tform[] = { "1J" };
	signed char data[] = { -50, 50 };
	long result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.25, 25.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -300);
	fail_if(result[1] != 100);
	call_01(ffclos, f);
}

static void
test_write_scaled_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	signed char data[] = { -100, 100 };
	LONGLONG result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 0.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -50);
	fail_if(result[1] != 50);
	call_01(ffclos, f);
}

static void
test_negative_to_unsigned_byte(void)
{
	/*
	* Writing negative signed bytes to unsigned byte column causes overflow.
	* The negative values are clamped to 0, positive values pass through.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UBCOL" };
	char *tform[] = { "1B" };
	signed char data[] = { -50, -1, 50 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Writing negative values to unsigned column returns NUM_OVERFLOW */
	ffpclsb(f, 1, 1, 1, 3, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	/* Negative values clamped to 0, positive stored correctly */
	fail_if(result[0] != 0);
	fail_if(result[1] != 0);
	fail_if(result[2] != 50);
	call_01(ffclos, f);
}

static void
test_overflow_scaled_short(void)
{
	/* Test overflow in scaled short conversion */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCOL" };
	char *tform[] = { "1I" };
	/* With scale=0.001, value 127 becomes 127000 which exceeds SHRT_MAX */
	signed char data[] = { 127 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_overflow_scaled_long(void)
{
	/* Test overflow in scaled long conversion */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LCOL" };
	char *tform[] = { "1J" };
	/* With scale=0.00000001, value 127 becomes huge, exceeds INT32_MAX */
	signed char data[] = { 127 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.00000001, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_overflow_scaled_longlong(void)
{
	/* Test overflow in scaled longlong conversion */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* Test underflow: with scale=0.00000001 and zero=1e18, -128 underflows */
	signed char data[] = { -128 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.0000000001, 1e18);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "USCOL" };
	char *tform[] = { "1U" };
	signed char data[] = { 0, 50, 127 };
	unsigned short result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 50);
	fail_if(result[2] != 127);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULCOL" };
	char *tform[] = { "1V" };
	signed char data[] = { 0, 100, 127 };
	unsigned long result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != 0);
	fail_if(result[1] != 100);
	fail_if(result[2] != 127);
	call_01(ffclos, f);
}

static void
test_write_large_vector(void)
{
	/* Write a large vector that spans multiple I/O buffer operations */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BIGCOL" };
	char *tform[] = { "10000S" };
	signed char *data;
	signed char *result;
	int anynull;
	int i;

	data = malloc(10000 * sizeof *data);
	result = malloc(10000 * sizeof *result);
	fail_if(data == NULL || result == NULL);

	for (i = 0; i < 10000; i += 1) {
		data[i] = (signed char)(i % 256 - 128);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 10000, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 10000, NULL, result, &anynull);
	for (i = 0; i < 10000; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);

	free(data);
	free(result);
}

static void
test_write_row_by_row(void)
{
	/* Write data row by row to exercise different code paths */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "3S" };
	signed char data1[] = { 1, 2, 3 };
	signed char data2[] = { 10, 20, 30 };
	signed char data3[] = { -1, -2, -3 };
	signed char result[9];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data1);
	call_06(ffpclsb, f, 1, 2, 1, 3, data2);
	call_06(ffpclsb, f, 1, 3, 1, 3, data3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 9, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[3] != 10);
	fail_if(result[6] != -1);
	call_01(ffclos, f);
}

static void
test_write_partial_row(void)
{
	/* Write partial elements within a row */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "10S" };
	signed char data[] = { 50, 60, 70 };
	signed char result[10];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Write to elements 4-6 only */
	call_06(ffpclsb, f, 1, 1, 4, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 10, NULL, result, &anynull);
	fail_if(result[3] != 50);
	fail_if(result[4] != 60);
	fail_if(result[5] != 70);
	call_01(ffclos, f);
}

static void
test_write_nulls_at_end(void)
{
	/* Write data with null values at the end to test nbad path */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "5S" };
	signed char data[] = { 1, 2, 3, -99, -99 };
	signed char nulval = -99;
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcnsb, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 5, NULL, result, &anynull);
	/* Non-null values should be read correctly */
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	call_01(ffclos, f);
}

static void
test_write_all_nulls(void)
{
	/* Write all null values - exercises the ffpclu code path */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "3S" };
	signed char data[] = { -99, -99, -99 };
	signed char nulval = -99;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_07(ffpcnsb, f, 1, 1, 1, 3, data, nulval);
	call_01(ffclos, f);
}

static void
test_write_scaled_unsigned_byte(void)
{
	/* Write to unsigned byte column with scaling */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	signed char data[] = { 50 };
	unsigned char result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* With BZERO=-128 (default for signed byte), value is stored as 50+128=178 */
	call_04(fftscl, f, 1, 1.0, -128.0);
	call_06(ffpclsb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 1, NULL, result, &anynull);
	/* Read back raw value which is 50+128=178 */
	fail_if(result[0] != 178);
	call_01(ffclos, f);
}

static void
test_write_7d_subsection(void)
{
	/* Write a 7D subsection (maximum dimensions) */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2, 1, 1, 1, 1 };
	long fpixel[] = { 1, 1, 1, 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 1, 1, 1, 1 };
	signed char data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	signed char result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 7, naxes);
	call_07(ffpsssb, f, 1, 7, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 8, NULL, result, &anynull);
	for (int i = 0; i < 8; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_write_4d_subsection(void)
{
	/* Write a 4D subsection */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3, 3, 2, 2 };
	long fpixel[] = { 1, 1, 1, 1 };
	long lpixel[] = { 2, 2, 2, 2 };
	signed char data[16];
	signed char result[36];
	int anynull;

	for (int i = 0; i < 16; i += 1) {
		data[i] = (signed char)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 4, naxes);
	call_07(ffpsssb, f, 1, 4, naxes, fpixel, lpixel, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 36, NULL, result, &anynull);
	/* Check corner values */
	fail_if(result[0] != 1);
	call_01(ffclos, f);
}

static void
test_write_scaled_with_zero_only(void)
{
	/* Test scaling with zero offset only (scale=1) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "1I" };
	signed char data[] = { 10, -10 };
	short result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 100.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 2, NULL, result, &anynull);
	/* stored = (input - zero) / scale = (10 - 100) / 1 = -90 */
	fail_if(result[0] != -90);
	fail_if(result[1] != -110);
	call_01(ffclos, f);
}

static void
test_write_underflow_scaled_short(void)
{
	/* Test underflow in scaled short conversion */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCOL" };
	char *tform[] = { "1I" };
	/* With scale=0.001 and zero=0, value -128 becomes -128000 < SHRT_MIN */
	signed char data[] = { -128 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_3d(void)
{
	/* Test BAD_DIMEN error when ncols < naxis1 in ffp3dsb */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4, 2 };
	signed char data[32];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 3, naxes);
	/* ncols=2 < naxis1=4, should fail with BAD_DIMEN */
	ffp3dsb(f, 1, 2, 4, 4, 4, 2, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_dimen_subsection(void)
{
	/* Test BAD_DIMEN error when naxis > 7 in ffpsssb */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2 };
	long fpixel[] = { 1, 1 };
	long lpixel[] = { 2, 2 };
	signed char data[4];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 2, naxes);
	/* naxis=8 > 7, should fail with BAD_DIMEN */
	ffpsssb(f, 1, 8, naxes, fpixel, lpixel, data, &status);
	fail_if(status != BAD_DIMEN);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_unsigned_byte_general(void)
{
	/* Test scaled unsigned byte with non-trivial scale/zero */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	signed char data[] = { 10, 50, 100 };
	unsigned char result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* With scale=2.0 and zero=10, stored = (input - 10) / 2 */
	call_04(fftscl, f, 1, 2.0, 10.0);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, result, &anynull);
	/* stored = (10-10)/2=0, (50-10)/2=20, (100-10)/2=45 */
	fail_if(result[0] != 0);
	fail_if(result[1] != 20);
	fail_if(result[2] != 45);
	call_01(ffclos, f);
}

static void
test_underflow_scaled_long(void)
{
	/* Test underflow in scaled long conversion */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LCOL" };
	char *tform[] = { "1J" };
	/* With scale=0.00000001 and zero=0, -128 becomes -12800000000 < INT32_MIN */
	signed char data[] = { -128 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.00000001, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_table_narrow_column(void)
{
	/* Test ASCII table with column that overflows, triggering NUM_OVERFLOW */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F4.1" };
	signed char data[] = { 127, -128 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	/* F4.1 is too narrow for 127.0 and -128.0 (need 5-6 chars) */
	ffpclsb(f, 1, 1, 1, 2, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_overflow_longlong_max(void)
{
	/* Test overflow to LONGLONG_MAX in scaled longlong */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	/* With scale=1e-17, 127/1e-17 = 1.27e19 > LONGLONG_MAX (9.2e18) */
	signed char data[] = { 127 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1e-17, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_2d_noncontiguous(void)
{
	/* Test 2D array where ncols > naxis1 (non-contiguous) */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 3 };
	/* Input array is 4 columns wide but we only write 2 columns */
	signed char data[] = {
		1, 2, 99, 99,
		3, 4, 99, 99,
		5, 6, 99, 99
	};
	signed char result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 2, naxes);
	/* ncols=4, naxis1=2, naxis2=3 */
	call_06(ffp2dsb, f, 1, 4, 2, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 6, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 4);
	fail_if(result[4] != 5);
	fail_if(result[5] != 6);
	call_01(ffclos, f);
}

static void
test_3d_noncontiguous_planes(void)
{
	/* Test 3D array with non-contiguous planes */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2, 2, 2 };
	/* nrows=3 > naxis2=2, so there's padding between planes */
	signed char data[] = {
		1, 2,
		3, 4,
		99, 99,  /* padding row */
		5, 6,
		7, 8,
		99, 99   /* padding row */
	};
	signed char result[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 3, naxes);
	/* ncols=2, nrows=3, naxis1=2, naxis2=2, naxis3=2 */
	call_08(ffp3dsb, f, 1, 2, 3, 2, 2, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 8, NULL, result, &anynull);
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
test_write_bit_column(void)
{
	/* Write to a bit column (X type) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	signed char data[] = { 0x55 };  /* 01010101 in binary */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
}

static void
test_write_complex_column(void)
{
	/* Write to a complex column (C type = 2 floats) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CPLXCOL" };
	char *tform[] = { "1C" };
	signed char data[] = { 10, 20 };  /* real=10, imag=20 */
	float result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != 10.0f);
	fail_if(result[1] != 20.0f);
	call_01(ffclos, f);
}

static void
test_write_double_complex_column(void)
{
	/* Write to a double complex column (M type = 2 doubles) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCPLXCOL" };
	char *tform[] = { "1M" };
	signed char data[] = { -50, 50 };  /* real=-50, imag=50 */
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != -50.0);
	fail_if(result[1] != 50.0);
	call_01(ffclos, f);
}

static void
test_write_to_unsigned_longlong_column(void)
{
	/*
	* Write to unsigned longlong column. The column has TZERO=2^63
	* which triggers the special XOR path in ffs1fi8.
	* Negative values should cause overflow.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULLCOL" };
	char *tform[] = { "1K" };
	signed char data[] = { 0, 50, 127 };
	LONGLONG result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Set TZERO to 2^63 to make it unsigned longlong */
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, result, &anynull);
	call_01(ffclos, f);
}

static void
test_unsigned_longlong_negative_overflow(void)
{
	/*
	* Writing negative signed byte to unsigned longlong column
	* should trigger overflow in the special XOR path.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ULLCOL" };
	char *tform[] = { "1K" };
	signed char data[] = { -1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 9223372036854775808.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_ubyte_underflow(void)
{
	/*
	* Test underflow in scaled unsigned byte (ffs1fi1).
	* With scale and zero, value becomes < 0.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With scale=1 and zero=200, stored = (input - 200) = negative */
	signed char data[] = { 0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 1.0, 200.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_ubyte_overflow(void)
{
	/*
	* Test overflow in scaled unsigned byte (ffs1fi1).
	* With scale and zero, value becomes > 255.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	/* With scale=0.1 and zero=0, stored = 127/0.1 = 1270 > 255 */
	signed char data[] = { 127 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.1, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_ascii_overflow(void)
{
	/*
	* Test overflow in ASCII table with scaled value that
	* produces a string too wide for the column.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL" };
	char *tform[] = { "F6.1" };
	/* With scale=0.001 and zero=0, 127/0.001 = 127000 needs 8 chars */
	signed char data[] = { 127 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_bad_btable_format(void)
{
	/*
	* Test writing to a binary table column with unsupported format.
	* Logical (L) columns cannot accept numeric data.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	signed char data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclsb(f, 1, 1, 1, 1, data, &status);
	fail_if(status != BAD_BTABLE_FORMAT);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_table_integer_format(void)
{
	/*
	* Test writing to an ASCII table column with integer (I) format.
	* This exercises the TSTRING path in ffpclsb for non-f/E formats.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "I6" };
	signed char data[] = { 42, -100 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	fail_if(result[0] != 42.0);
	fail_if(result[1] != -100.0);
	call_01(ffclos, f);
}

static void
test_fixed_col_overflow_with_nulls(void)
{
	/*
	* Test overflow tracking in fixed-length column with nulls.
	* For fixed-length columns, ffpcnsb tracks overflow separately.
	* Negative values overflow when written to unsigned byte column.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "5B" };
	/* -50 overflows to unsigned byte */
	signed char data[] = { -50, -99, -100, 50, 60 };
	signed char nulval = -99;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	ffpcnsb(f, 1, 1, 1, 5, data, nulval, &status);
	/* Should get NUM_OVERFLOW due to negative values going to unsigned */
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_scaled_col_overflow_with_nulls(void)
{
	/*
	* Test overflow tracking in scaled column with nulls in ffpcnsb.
	* The good values overflow during scaling, setting the overflow flag.
	* Ending with null values causes the final overflow check path to run.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCOL" };
	char *tform[] = { "5I" };
	/* With scale=0.001, 127/0.001 = 127000 overflows short (SHRT_MAX=32767) */
	/* Sequence: 127 (overflow), -99 (null), 127 (overflow), -99 (null) */
	/* The overflowing 127 in the middle sets the overflow flag. */
	/* Ending with nulls means nbad > 0, triggering final overflow check. */
	signed char data[] = { 127, -99, 127, -99 };
	signed char nulval = -99;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_04(fftscl, f, 1, 0.001, 0.0);
	ffpcnsb(f, 1, 1, 1, 4, data, nulval, &status);
	fail_if(status != NUM_OVERFLOW);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_string_column_multirow(void)
{
	/*
	* Write raw bytes to ASCII table A format column with multiple rows.
	* With multiple columns, incre (row width) != twidth (column width),
	* triggering the ffpbytoff path in ffpclsb.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STRCOL", "NUMCOL" };
	char *tform[] = { "A4", "I8" };
	signed char data[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
	char result[5] = {0};
	char *rptr = result;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 2, ttype, tform, NULL, NULL);
	/* Write 4 bytes per row, 2 rows */
	call_06(ffpclsb, f, 1, 1, 1, 4, data);
	call_06(ffpclsb, f, 1, 2, 1, 4, &data[4]);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "ABCD") != 0);
	call_09(ffgcv, f, TSTRING, 1, 2, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "EFGH") != 0);
	call_01(ffclos, f);
}

static void
test_ascii_e_format_column(void)
{
	/*
	* Write to ASCII table with E format (exponential notation).
	* This exercises the ffs1fstr path for 'E' format.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "E12.4" };
	signed char data[] = { -100, 50, 127 };
	double result[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, result, &anynull);
	fail_if(result[0] != -100.0);
	fail_if(result[1] != 50.0);
	fail_if(result[2] != 127.0);
	call_01(ffclos, f);
}

static void
test_ascii_e_format_scaled(void)
{
	/*
	* Write to ASCII table E format with scaling.
	* Tests the scaled path in ffs1fstr.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "E15.6" };
	signed char data[] = { 10, -10 };
	double result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 0.5, 5.0);
	call_06(ffpclsb, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 2, NULL, result, &anynull);
	/* stored = (input - zero) / scale = (10 - 5) / 0.5 = 10 */
	fail_if(result[0] != 10.0);
	/* stored = (-10 - 5) / 0.5 = -30 */
	fail_if(result[1] != -30.0);
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
	test_write_to_ascii_table();
	test_write_group_parameters();
	test_write_with_scaling();
	test_write_noncontiguous_3d();
	test_write_multirow();
	test_write_to_string_column();
	test_write_scaled_float();
	test_write_scaled_double();
	test_write_scaled_ascii();
	test_write_variable_length_array();
	test_write_varlen_with_nulls();
	test_write_to_unsigned_byte_column();
	test_write_scaled_short();
	test_write_scaled_long();
	test_write_scaled_longlong();
	test_negative_to_unsigned_byte();
	test_overflow_scaled_short();
	test_overflow_scaled_long();
	test_overflow_scaled_longlong();
	test_write_to_unsigned_short_column();
	test_write_to_unsigned_long_column();
	test_write_large_vector();
	test_write_row_by_row();
	test_write_partial_row();
	test_write_nulls_at_end();
	test_write_all_nulls();
	test_write_scaled_unsigned_byte();
	test_write_7d_subsection();
	test_write_4d_subsection();
	test_write_scaled_with_zero_only();
	test_write_underflow_scaled_short();
	test_bad_dimen_3d();
	test_bad_dimen_subsection();
	test_scaled_unsigned_byte_general();
	test_underflow_scaled_long();
	test_ascii_table_narrow_column();
	test_overflow_longlong_max();
	test_2d_noncontiguous();
	test_3d_noncontiguous_planes();
	test_write_bit_column();
	test_write_complex_column();
	test_write_double_complex_column();
	test_write_to_unsigned_longlong_column();
	test_unsigned_longlong_negative_overflow();
	test_scaled_ubyte_underflow();
	test_scaled_ubyte_overflow();
	test_scaled_ascii_overflow();
	test_bad_btable_format();
	test_ascii_table_integer_format();
	test_fixed_col_overflow_with_nulls();
	test_scaled_col_overflow_with_nulls();
	test_ascii_string_column_multirow();
	test_ascii_e_format_column();
	test_ascii_e_format_scaled();
	remove(test_path);

	return 0;
}
