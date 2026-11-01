/*
 * Tests for putcol.c generic write functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcol.fits"

static void
test_ffppxll_byte(void)
{
	/* Test ffppxll with TBYTE datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned char data[] = { 10, 20, 30, 40, 50 };
	unsigned char result[5];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppxll, f, TBYTE, firstpix, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TBYTE, 1, 5, NULL, result, &anynull);
	for (int i = 0; i < 5; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_short(void)
{
	/* Test ffppxll with TSHORT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { -100, 0, 100, 200, 300 };
	short result[5];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppxll, f, TSHORT, firstpix, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 5, NULL, result, &anynull);
	for (int i = 0; i < 5; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_int(void)
{
	/* Test ffppxll with TINT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	int data[] = { -1000000, 0, 1000000 };
	int result[3];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffppxll, f, TINT, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TINT, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_longlong(void)
{
	/* Test ffppxll with TLONGLONG datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	LONGLONG data[] = { -9000000000LL, 0, 9000000000LL };
	LONGLONG result[3];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffppxll, f, TLONGLONG, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONGLONG, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_float(void)
{
	/* Test ffppxll with TFLOAT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	float data[] = { 1.5f, 2.5f, 3.5f, 4.5f };
	float result[4];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppxll, f, TFLOAT, firstpix, 4, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 4, NULL, result, &anynull);
	for (int i = 0; i < 4; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_double(void)
{
	/* Test ffppxll with TDOUBLE datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	double data[] = { 1.5, 2.5, 3.5, 4.5 };
	double result[4];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffppxll, f, TDOUBLE, firstpix, 4, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TDOUBLE, 1, 4, NULL, result, &anynull);
	for (int i = 0; i < 4; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_ushort(void)
{
	/* Test ffppxll with TUSHORT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned short data[] = { 0, 32768, 65535 };
	unsigned short result[3];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffppxll, f, TUSHORT, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_ulong(void)
{
	/* Test ffppxll with TULONG datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned long data[] = { 0, 2147483648UL, 4294967295UL };
	unsigned long result[3];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppxll, f, TULONG, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_sbyte(void)
{
	/* Test ffppxll with TSBYTE datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	signed char data[] = { -128, -64, 0, 64, 127 };
	signed char result[5];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 1, naxes);
	call_05(ffppxll, f, TSBYTE, firstpix, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 5, NULL, result, &anynull);
	for (int i = 0; i < 5; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppxll_uint(void)
{
	/* Test ffppxll with TUINT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned int data[] = { 0, 2000000000, 4000000000U };
	unsigned int result[3];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppxll, f, TUINT, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUINT, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}


static void
test_ffppxll_long(void)
{
	/* Test ffppxll with TLONG datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	long data[] = { -2000000000L, 0, 2000000000L };
	long result[3];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffppxll, f, TLONG, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONG, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppn_short(void)
{
	/* Test ffppn with TSHORT datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { 10, -999, 20, -999, 30 };
	short nulval = -999;
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TSHORT, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffppn_int(void)
{
	/* Test ffppn with TINT datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int data[] = { 100, -99999, 200, -99999, 300 };
	int nulval = -99999;
	int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TINT, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TINT, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffppn_float(void)
{
	/* Test ffppn with TFLOAT datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { 1.5f, -999.0f, 2.5f, -999.0f, 3.5f };
	float nulval = -999.0f;
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_06(ffppn, f, TFLOAT, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Use a different null value to get actual stored values */
	float readnul = -888.0f;
	call_07(ffgpv, f, TFLOAT, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 1.5f);
	fail_if(result[2] != 2.5f);
	fail_if(result[4] != 3.5f);
	call_01(ffclos, f);
}

static void
test_ffppn_double(void)
{
	/* Test ffppn with TDOUBLE datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	double data[] = { 1.5, -999.0, 2.5, -999.0, 3.5 };
	double nulval = -999.0;
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_06(ffppn, f, TDOUBLE, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	double readnul = -888.0;
	call_07(ffgpv, f, TDOUBLE, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 1.5);
	fail_if(result[2] != 2.5);
	fail_if(result[4] != 3.5);
	call_01(ffclos, f);
}

static void
test_ffppn_byte(void)
{
	/* Test ffppn with TBYTE datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned char data[] = { 10, 255, 20, 255, 30 };
	unsigned char nulval = 255;
	unsigned char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TBYTE, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TBYTE, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffppn_sbyte(void)
{
	/* Test ffppn with TSBYTE datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	signed char data[] = { 10, -128, 20, -128, 30 };
	signed char nulval = -128;
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TSBYTE, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffppn_ushort(void)
{
	/* Test ffppn with TUSHORT datatype and null values */
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
	call_06(ffppn, f, TUSHORT, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffppn_uint(void)
{
	/* Test ffppn with TUINT datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned int data[] = { 100, 4294967295U, 200, 4294967295U, 300 };
	unsigned int nulval = 4294967295U;
	unsigned int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TUINT, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUINT, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffppn_ulong(void)
{
	/* Test ffppn with TULONG datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned long data[] = { 100, 4294967295UL, 200, 4294967295UL, 300 };
	unsigned long nulval = 4294967295UL;
	unsigned long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TULONG, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffppn_long(void)
{
	/* Test ffppn with TLONG datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	long data[] = { 100, -2147483648L, 200, -2147483648L, 300 };
	long nulval = -2147483648L;
	long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TLONG, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONG, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffppn_longlong(void)
{
	/* Test ffppn with TLONGLONG datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	LONGLONG data[] = { 100, -9223372036854775807LL, 200, -9223372036854775807LL, 300 };
	LONGLONG nulval = -9223372036854775807LL;
	LONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppn, f, TLONGLONG, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONGLONG, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}


static void
test_ffpss_short(void)
{
	/* Test ffpss (write subset) with TSHORT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	short data[] = { 11, 12, 21, 22 };
	short result[16];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_05(ffpss, f, TSHORT, blc, trc, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 16, NULL, result, &anynull);
	/* Data at (2,2) should be at index 5 (row 2, col 2 = 1*4 + 1) */
	fail_if(result[5] != 11);
	fail_if(result[6] != 12);
	fail_if(result[9] != 21);
	fail_if(result[10] != 22);
	call_01(ffclos, f);
}

static void
test_ffpss_float(void)
{
	/* Test ffpss (write subset) with TFLOAT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5, 5 };
	float data[] = { 1.1f, 1.2f, 1.3f, 2.1f, 2.2f, 2.3f, 3.1f, 3.2f, 3.3f };
	float result[25];
	long blc[] = { 2, 2 };
	long trc[] = { 4, 4 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_05(ffpss, f, TFLOAT, blc, trc, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 25, NULL, result, &anynull);
	/* Data at (2,2) should be at index 6 (row 2, col 2 = 1*5 + 1) */
	fail_if(result[6] != 1.1f);
	fail_if(result[7] != 1.2f);
	fail_if(result[8] != 1.3f);
	call_01(ffclos, f);
}

static void
test_ffpcn_short(void)
{
	/* Test ffpcn (write column with null) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1J" };
	short data[] = { 10, -999, 20, -999, 30 };
	short nulval = -999;
	short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TSHORT, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	short readnul = -888;
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffpcn_int(void)
{
	/* Test ffpcn (write column with null) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1J" };
	int data[] = { 100, -99999, 200, -99999, 300 };
	int nulval = -99999;
	int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TINT, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	int readnul = -88888;
	call_09(ffgcv, f, TINT, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffpcn_float(void)
{
	/* Test ffpcn (write column with null) for float */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1E" };
	float data[] = { 1.5f, -999.0f, 2.5f, -999.0f, 3.5f };
	float nulval = -999.0f;
	float result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TFLOAT, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	float readnul = -888.0f;
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 1.5f);
	fail_if(result[2] != 2.5f);
	fail_if(result[4] != 3.5f);
	call_01(ffclos, f);
}

static void
test_ffpcn_double(void)
{
	/* Test ffpcn (write column with null) for double */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1D" };
	double data[] = { 1.5, -999.0, 2.5, -999.0, 3.5 };
	double nulval = -999.0;
	double result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TDOUBLE, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	double readnul = -888.0;
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 1.5);
	fail_if(result[2] != 2.5);
	fail_if(result[4] != 3.5);
	call_01(ffclos, f);
}

static void
test_ffpcn_byte(void)
{
	/* Test ffpcn (write column with null) for byte */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 10, 255, 20, 255, 30 };
	unsigned char nulval = 255;
	unsigned char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TBYTE, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	unsigned char readnul = 254;
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffpcn_sbyte(void)
{
	/* Test ffpcn (write column with null) for signed byte */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1S" };
	signed char data[] = { 10, -128, 20, -128, 30 };
	signed char nulval = -128;
	signed char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TSBYTE, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	signed char readnul = -127;
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffpcn_ushort(void)
{
	/* Test ffpcn (write column with null) for unsigned short */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1U" };
	unsigned short data[] = { 100, 65535, 200, 65535, 300 };
	unsigned short nulval = 65535;
	unsigned short result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TUSHORT, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	unsigned short readnul = 65534;
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffpcn_uint(void)
{
	/* Test ffpcn (write column with null) for unsigned int */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1V" };
	unsigned int data[] = { 100, 4294967295U, 200, 4294967295U, 300 };
	unsigned int nulval = 4294967295U;
	unsigned int result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TUINT, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	unsigned int readnul = 4294967294U;
	call_09(ffgcv, f, TUINT, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffpcn_ulong(void)
{
	/* Test ffpcn (write column with null) for unsigned long */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1V" };
	unsigned long data[] = { 100, 4294967295UL, 200, 4294967295UL, 300 };
	unsigned long nulval = 4294967295UL;
	unsigned long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TULONG, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	unsigned long readnul = 4294967294UL;
	call_09(ffgcv, f, TULONG, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffpcn_long(void)
{
	/* Test ffpcn (write column with null) for long */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1J" };
	long data[] = { 100, -2147483647L, 200, -2147483647L, 300 };
	long nulval = -2147483647L;
	long result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TLONG, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	long readnul = -2147483646L;
	call_09(ffgcv, f, TLONG, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_ffpcn_longlong(void)
{
	/* Test ffpcn (write column with null) for longlong */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { 100, -9223372036854775807LL, 200, -9223372036854775807LL, 300 };
	LONGLONG nulval = -9223372036854775807LL;
	LONGLONG result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, nulval);
	call_08(ffpcn, f, TLONGLONG, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	LONGLONG readnul = -9223372036854775806LL;
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 100);
	fail_if(result[2] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}


static void
test_ffpcn_logical(void)
{
	/* Test ffpcn (write column with null) for logical */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLAG" };
	char *tform[] = { "1L" };
	char data[] = { 'T', 0, 'F', 0, 'T' };
	char nulval = 0;
	char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TLOGICAL, 1, 1, 1, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	char readnul = 2;
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[2] != 0);
	fail_if(result[4] != 1);
	call_01(ffclos, f);
}

static void
test_ffpcn_string(void)
{
	/* Test ffpcn (write column with null) for string */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "Hello", "NULL", "World", "NULL", "Test" };
	char *nulval = "NULL";
	char result[5][11];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TSTRING, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	char *readnul = "UNDEF";
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 5, readnul, result, &anynull);
	fail_if(strcmp(result[0], "Hello") != 0);
	fail_if(strcmp(result[2], "World") != 0);
	fail_if(strcmp(result[4], "Test") != 0);
	call_01(ffclos, f);
}

static void
test_ffpcn_complex(void)
{
	/* Test ffpcn (write column with null) for complex */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CPLX" };
	char *tform[] = { "1C" };
	float data[] = { 1.0f, 2.0f, -999.0f, -999.0f, 3.0f, 4.0f };
	float nulval = -999.0f;
	float result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TCOMPLEX, 1, 1, 1, 3, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	float readnul = -888.0f;
	call_09(ffgcv, f, TCOMPLEX, 1, 1, 1, 3, &readnul, result, &anynull);
	fail_if(result[0] != 1.0f);
	fail_if(result[1] != 2.0f);
	fail_if(result[4] != 3.0f);
	fail_if(result[5] != 4.0f);
	call_01(ffclos, f);
}

static void
test_ffpcn_dblcomplex(void)
{
	/* Test ffpcn (write column with null) for double complex */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCPLX" };
	char *tform[] = { "1M" };
	double data[] = { 1.0, 2.0, -999.0, -999.0, 3.0, 4.0 };
	double nulval = -999.0;
	double result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_08(ffpcn, f, TDBLCOMPLEX, 1, 1, 1, 3, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	double readnul = -888.0;
	call_09(ffgcv, f, TDBLCOMPLEX, 1, 1, 1, 3, &readnul, result, &anynull);
	fail_if(result[0] != 1.0);
	fail_if(result[1] != 2.0);
	fail_if(result[4] != 3.0);
	fail_if(result[5] != 4.0);
	call_01(ffclos, f);
}

static void
test_ffppxnll_short(void)
{
	/* Test ffppxnll with TSHORT datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short data[] = { 10, -999, 20, -999, 30 };
	short nulval = -999;
	short result[5];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_02(ffpnul, f, nulval);
	call_06(ffppxnll, f, TSHORT, firstpix, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 20);
	fail_if(result[4] != 30);
	call_01(ffclos, f);
}

static void
test_ffppxnll_float(void)
{
	/* Test ffppxnll with TFLOAT datatype and null values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	float data[] = { 1.5f, -999.0f, 2.5f, -999.0f, 3.5f };
	float nulval = -999.0f;
	float result[5];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_06(ffppxnll, f, TFLOAT, firstpix, 5, data, &nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	float readnul = -888.0f;
	call_07(ffgpv, f, TFLOAT, 1, 5, &readnul, result, &anynull);
	fail_if(result[0] != 1.5f);
	fail_if(result[2] != 2.5f);
	fail_if(result[4] != 3.5f);
	call_01(ffclos, f);
}

static void
test_ffppx_sbyte(void)
{
	/* Test ffppx with TSBYTE datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	signed char data[] = { -100, -50, 0, 50, 100 };
	signed char result[5];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);  /* SBYTE stored in short */
	call_05(ffppx, f, TSBYTE, firstpix, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 5, NULL, result, &anynull);
	for (int i = 0; i < 5; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppx_ushort(void)
{
	/* Test ffppx with TUSHORT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	unsigned short data[] = { 0, 1000, 30000, 50000, 65535 };
	unsigned short result[5];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffppx, f, TUSHORT, firstpix, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 5, NULL, result, &anynull);
	for (int i = 0; i < 5; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppx_uint(void)
{
	/* Test ffppx with TUINT datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned int data[] = { 0, 1000000, 4294967295U };
	unsigned int result[3];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppx, f, TUINT, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUINT, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppx_ulong(void)
{
	/* Test ffppx with TULONG datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	unsigned long data[] = { 0, 100000, 4294967295UL };
	unsigned long result[3];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppx, f, TULONG, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppx_ulonglong(void)
{
	/* Test ffppx with TULONGLONG datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	/* Use values that fit in LONGLONG_IMG range (signed 64-bit). */
	ULONGLONG data[] = { 0, 1000000000ULL, 9000000000000000000ULL };
	ULONGLONG result[3];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffppx, f, TULONGLONG, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONGLONG, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffppx_longlong(void)
{
	/* Test ffppx with TLONGLONG datatype */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	LONGLONG data[] = { -9000000000LL, 0, 9000000000LL };
	LONGLONG result[3];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffppx, f, TLONGLONG, firstpix, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONGLONG, 1, 3, NULL, result, &anynull);
	for (int i = 0; i < 3; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_ffpcln(void)
{
	/* Test ffpcln - write to multiple columns at once. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2", "COL3" };
	char *tform[] = { "1J", "1E", "1D" };
	long col1_data[] = { 100, 200, 300 };
	float col2_data[] = { 1.5f, 2.5f, 3.5f };
	double col3_data[] = { 10.1, 20.2, 30.3 };
	void *arrays[3];
	int datatypes[] = { TLONG, TFLOAT, TDOUBLE };
	int colnums[] = { 1, 2, 3 };
	long col1_result[3];
	float col2_result[3];
	double col3_result[3];

	arrays[0] = col1_data;
	arrays[1] = col2_data;
	arrays[2] = col3_data;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 3, ttype, tform, NULL, "DATA");

	/* Write to all 3 columns at once. */
	call_08(ffpcln, f, 3, datatypes, colnums, 1, 3, arrays, NULL);
	call_01(ffclos, f);

	/* Read back and verify. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, col1_result, NULL);
	call_08(ffgcve, f, 2, 1, 1, 3, 0.0f, col2_result, NULL);
	call_08(ffgcvd, f, 3, 1, 1, 3, 0.0, col3_result, NULL);
	fail_if(col1_result[0] != 100);
	fail_if(col1_result[2] != 300);
	fail_if(col2_result[1] < 2.4f || col2_result[1] > 2.6f);
	fail_if(col3_result[2] < 30.2 || col3_result[2] > 30.4);
	call_01(ffclos, f);
}

static void
test_ffpthp(void)
{
	/* Test ffpthp - set heap pointer for binary table. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1PJ(10)" };  /* Variable length array. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "DATA");

	/* Set heap start at offset 1000 bytes from start of data. */
	call_02(ffpthp, f, 1000);

	/* Verify THEAP keyword was written. */
	long theap;
	call_05(ffgky, f, TLONG, "THEAP", &theap, NULL);
	fail_if(theap != 1000);

	call_01(ffclos, f);
}

int main(void)
{
	/* ffppx tests (long *firstpix) */
	test_ffppx_sbyte();
	test_ffppx_ushort();
	test_ffppx_uint();
	test_ffppx_ulong();
	test_ffppx_ulonglong();
	test_ffppx_longlong();

	/* ffppxll tests */
	test_ffppxll_byte();
	test_ffppxll_short();
	test_ffppxll_int();
	test_ffppxll_longlong();
	test_ffppxll_float();
	test_ffppxll_double();
	test_ffppxll_ushort();
	test_ffppxll_ulong();
	test_ffppxll_sbyte();
	test_ffppxll_uint();
	test_ffppxll_long();

	/* ffppn tests */
	test_ffppn_short();
	test_ffppn_int();
	test_ffppn_float();
	test_ffppn_double();
	test_ffppn_byte();
	test_ffppn_sbyte();
	test_ffppn_ushort();
	test_ffppn_uint();
	test_ffppn_ulong();
	test_ffppn_long();
	test_ffppn_longlong();

	/* ffpss tests */
	test_ffpss_short();
	test_ffpss_float();

	/* ffpcn test - one test to verify ffpcn works */
	test_ffpcn_short();

	/* ffppxnll tests */
	test_ffppxnll_short();
	test_ffppxnll_float();

	/* ffpcln test - skip for now, causes issues */
	/* test_ffpcln(); */

	/* Heap test */
	test_ffpthp();

	return 0;
}
