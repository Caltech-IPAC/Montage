/*
 * Tests for getcol.c - generic column/pixel read functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcol.fits"

/*
 * Test ffgpv with various datatypes - read from primary array
 */
static void
test_ffgpv_short(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 6 };
	short wdata[] = { -32768, -100, 0, 100, 1000, 32767 };
	short rdata[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 6, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSHORT, 1, 6, NULL, rdata, &anynull);
	fail_if(rdata[0] != -32768);
	fail_if(rdata[1] != -100);
	fail_if(rdata[2] != 0);
	fail_if(rdata[3] != 100);
	fail_if(rdata[4] != 1000);
	fail_if(rdata[5] != 32767);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_ffgpv_long(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	long wdata[] = { -2000000000L, -100, 100, 2000000000L };
	long rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONG, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != -2000000000L);
	fail_if(rdata[1] != -100);
	fail_if(rdata[2] != 100);
	fail_if(rdata[3] != 2000000000L);
	call_01(ffclos, f);
}

static void
test_ffgpv_float(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	float wdata[] = { -1.5f, 0.0f, 1.5f, 3.14159f };
	float rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 4, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - (-1.5f)) > 1e-6);
	fail_if(fabs(rdata[1] - 0.0f) > 1e-6);
	fail_if(fabs(rdata[2] - 1.5f) > 1e-6);
	fail_if(fabs(rdata[3] - 3.14159f) > 1e-5);
	call_01(ffclos, f);
}

static void
test_ffgpv_double(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	double wdata[] = { -1.5, 0.0, 1.5, 3.14159265358979 };
	double rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TDOUBLE, 1, 4, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - (-1.5)) > 1e-10);
	fail_if(fabs(rdata[1] - 0.0) > 1e-10);
	fail_if(fabs(rdata[2] - 1.5) > 1e-10);
	fail_if(fabs(rdata[3] - 3.14159265358979) > 1e-10);
	call_01(ffclos, f);
}

static void
test_ffgpv_byte(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned char wdata[] = { 0, 100, 200, 255 };
	unsigned char rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TBYTE, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 100);
	fail_if(rdata[2] != 200);
	fail_if(rdata[3] != 255);
	call_01(ffclos, f);
}

static void
test_ffgpv_sbyte(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	signed char wdata[] = { -128, -1, 0, 127 };
	signed char rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SBYTE_IMG, 1, naxes);
	call_05(ffpprsb, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TSBYTE, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != -128);
	fail_if(rdata[1] != -1);
	fail_if(rdata[2] != 0);
	fail_if(rdata[3] != 127);
	call_01(ffclos, f);
}

static void
test_ffgpv_ushort(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned short wdata[] = { 0, 100, 30000, 65535 };
	unsigned short rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_05(ffpprui, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUSHORT, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 100);
	fail_if(rdata[2] != 30000);
	fail_if(rdata[3] != 65535);
	call_01(ffclos, f);
}

static void
test_ffgpv_uint(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned int wdata[] = { 0, 100, 3000000000U, 4000000000U };
	unsigned int rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppruk, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TUINT, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 100);
	fail_if(rdata[2] != 3000000000U);
	fail_if(rdata[3] != 4000000000U);
	call_01(ffclos, f);
}

static void
test_ffgpv_int(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	int wdata[] = { -2000000000, -100, 100, 2000000000 };
	int rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprk, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TINT, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != -2000000000);
	fail_if(rdata[1] != -100);
	fail_if(rdata[2] != 100);
	fail_if(rdata[3] != 2000000000);
	call_01(ffclos, f);
}

static void
test_ffgpv_ulong(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned long wdata[] = { 0, 100, 3000000000UL, 4000000000UL };
	unsigned long rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_05(ffppruj, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TULONG, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 100);
	fail_if(rdata[2] != 3000000000UL);
	fail_if(rdata[3] != 4000000000UL);
	call_01(ffclos, f);
}

static void
test_ffgpv_longlong(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	LONGLONG wdata[] = { -9000000000000LL, -100, 100, 9000000000000LL };
	LONGLONG rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_05(ffpprjj, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TLONGLONG, 1, 4, NULL, rdata, &anynull);
	fail_if(rdata[0] != -9000000000000LL);
	fail_if(rdata[1] != -100);
	fail_if(rdata[2] != 100);
	fail_if(rdata[3] != 9000000000000LL);
	call_01(ffclos, f);
}

/*
 * Test ffgpv with null value substitution
 */
static void
test_ffgpv_with_nulval(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short wdata[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short rdata[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	/* For images, write BLANK keyword to define null value */
	call_04(ffukyj, f, "BLANK", nulval, "null value");
	call_05(ffppri, f, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	short replace = -999;
	call_07(ffgpv, f, TSHORT, 1, 5, &replace, rdata, &anynull);
	fail_if(rdata[0] != 100);
	fail_if(rdata[1] != -999);
	fail_if(rdata[2] != 200);
	fail_if(rdata[3] != -999);
	fail_if(rdata[4] != 300);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

/*
 * Test ffgpf - read with null flagging
 */
static void
test_ffgpf_short(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short wdata[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short rdata[5];
	char nullarray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	/* For images, write BLANK keyword to define null value */
	call_04(ffukyj, f, "BLANK", nulval, "null value");
	call_05(ffppri, f, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpf, f, TSHORT, 1, 5, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 100);
	fail_if(rdata[2] != 200);
	fail_if(rdata[4] != 300);
	fail_if(nullarray[0] != 0);
	fail_if(nullarray[1] != 1);
	fail_if(nullarray[2] != 0);
	fail_if(nullarray[3] != 1);
	fail_if(nullarray[4] != 0);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_ffgpf_float(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	float wdata[] = { 1.0f, 2.0f, 3.0f, 4.0f };
	float rdata[4];
	char nullarray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppre, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpf, f, TFLOAT, 1, 4, rdata, nullarray, &anynull);
	fail_if(fabs(rdata[0] - 1.0f) > 1e-6);
	fail_if(fabs(rdata[1] - 2.0f) > 1e-6);
	fail_if(nullarray[0] != 0);
	fail_if(nullarray[1] != 0);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_ffgpf_double(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	double wdata[] = { 1.0, 2.0, 3.0, 4.0 };
	double rdata[4];
	char nullarray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffpprd, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpf, f, TDOUBLE, 1, 4, rdata, nullarray, &anynull);
	fail_if(fabs(rdata[0] - 1.0) > 1e-10);
	fail_if(fabs(rdata[3] - 4.0) > 1e-10);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_ffgpf_byte(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	unsigned char wdata[] = { 0, 100, 200, 255 };
	unsigned char rdata[4];
	char nullarray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpf, f, TBYTE, 1, 4, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[3] != 255);
	call_01(ffclos, f);
}

static void
test_ffgpf_long(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	long wdata[] = { -1000000, 0, 1000000, 2000000000L };
	long rdata[4];
	char nullarray[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 1, naxes);
	call_05(ffpprj, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpf, f, TLONG, 1, 4, rdata, nullarray, &anynull);
	fail_if(rdata[0] != -1000000);
	fail_if(rdata[3] != 2000000000L);
	call_01(ffclos, f);
}

/*
 * Test ffgpxv - read pixels by coordinates
 */
static void
test_ffgpxv_1d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	short wdata[10];
	short rdata[5];
	long firstpix[] = { 3 };
	int anynull;
	int i;

	for (i = 0; i < 10; i += 1) {
		wdata[i] = (short)(i * 10);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 10, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpxv, f, TSHORT, firstpix, 5, NULL, rdata, &anynull);
	fail_if(rdata[0] != 20);  /* pixel 3 = index 2 */
	fail_if(rdata[1] != 30);
	fail_if(rdata[2] != 40);
	fail_if(rdata[3] != 50);
	fail_if(rdata[4] != 60);
	call_01(ffclos, f);
}

static void
test_ffgpxv_2d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	short wdata[16];
	short rdata[4];
	long firstpix[] = { 2, 2 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_06(ffp2di, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpxv, f, TSHORT, firstpix, 4, NULL, rdata, &anynull);
	/* Starting at (2,2), reading 4 elements */
	/* Row 2: elements at (2,2), (3,2), (4,2) then (1,3) */
	fail_if(rdata[0] != 6);   /* (2,2) = index 5 */
	fail_if(rdata[1] != 7);   /* (3,2) */
	fail_if(rdata[2] != 8);   /* (4,2) */
	fail_if(rdata[3] != 9);   /* (1,3) */
	call_01(ffclos, f);
}

/*
 * Test ffgpxvll - read pixels by LONGLONG coordinates
 */
static void
test_ffgpxvll_1d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	short wdata[10];
	short rdata[3];
	LONGLONG firstpix[] = { 5 };
	int anynull;
	int i;

	for (i = 0; i < 10; i += 1) {
		wdata[i] = (short)(i * 100);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 10, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpxvll, f, TSHORT, firstpix, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != 400);  /* pixel 5 = index 4 */
	fail_if(rdata[1] != 500);
	fail_if(rdata[2] != 600);
	call_01(ffclos, f);
}

/*
 * Test ffgpxf - read pixels by coordinates with null flagging
 */
static void
test_ffgpxf_1d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short wdata[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short rdata[5];
	char nullarray[5];
	long firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	/* For images, write BLANK keyword to define null value */
	call_04(ffukyj, f, "BLANK", nulval, "null value");
	call_05(ffppri, f, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpxf, f, TSHORT, firstpix, 5, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 100);
	fail_if(rdata[2] != 200);
	fail_if(rdata[4] != 300);
	fail_if(nullarray[1] != 1);
	fail_if(nullarray[3] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

/*
 * Test ffgpxfll - read pixels by LONGLONG coordinates with null flagging
 */
static void
test_ffgpxfll_1d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	short wdata[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short rdata[5];
	char nullarray[5];
	LONGLONG firstpix[] = { 1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	/* For images, write BLANK keyword to define null value */
	call_04(ffukyj, f, "BLANK", nulval, "null value");
	call_05(ffppri, f, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpxfll, f, TSHORT, firstpix, 5, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 100);
	fail_if(nullarray[1] != 1);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

/*
 * Test ffgsv - read subsection with strides
 */
static void
test_ffgsv_whole_image(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	short wdata[16];
	short rdata[16];
	long blc[] = { 1, 1 };
	long trc[] = { 4, 4 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_06(ffp2di, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TSHORT, blc, trc, inc, NULL, rdata, &anynull);
	fail_if(rdata[0] != 1);
	fail_if(rdata[15] != 16);
	call_01(ffclos, f);
}

static void
test_ffgsv_subsection(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	short wdata[16];
	short rdata[4];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_06(ffp2di, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TSHORT, blc, trc, inc, NULL, rdata, &anynull);
	/* Reading 2x2 subsection starting at (2,2) */
	fail_if(rdata[0] != 6);   /* (2,2) */
	fail_if(rdata[1] != 7);   /* (3,2) */
	fail_if(rdata[2] != 10);  /* (2,3) */
	fail_if(rdata[3] != 11);  /* (3,3) */
	call_01(ffclos, f);
}

static void
test_ffgsv_with_stride(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 6, 6 };
	short wdata[36];
	short rdata[9];
	long blc[] = { 1, 1 };
	long trc[] = { 6, 6 };
	long inc[] = { 2, 2 };
	int anynull;
	int i;

	for (i = 0; i < 36; i += 1) {
		wdata[i] = (short)(i + 1);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_06(ffp2di, f, 1, 6, 6, 6, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TSHORT, blc, trc, inc, NULL, rdata, &anynull);
	/* Reading every other pixel: (1,1), (3,1), (5,1), (1,3), ... */
	fail_if(rdata[0] != 1);   /* (1,1) */
	fail_if(rdata[1] != 3);   /* (3,1) */
	fail_if(rdata[2] != 5);   /* (5,1) */
	fail_if(rdata[3] != 13);  /* (1,3) */
	call_01(ffclos, f);
}

static void
test_ffgsv_float(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	float wdata[16];
	float rdata[4];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (float)(i + 1) * 0.5f;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 2, naxes);
	call_06(ffp2de, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TFLOAT, blc, trc, inc, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - 3.0f) > 1e-6);  /* (2,2) = index 5 -> 6*0.5 */
	fail_if(fabs(rdata[1] - 3.5f) > 1e-6);  /* (3,2) */
	call_01(ffclos, f);
}

/*
 * Test ffgcv - read table column
 */
static void
test_ffgcv_short(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "1I" };
	short wdata[] = { -32768, 0, 32767 };
	short rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != -32768);
	fail_if(rdata[1] != 0);
	fail_if(rdata[2] != 32767);
	call_01(ffclos, f);
}

static void
test_ffgcv_long(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "1J" };
	long wdata[] = { -2000000000L, 0, 2000000000L };
	long rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONG, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != -2000000000L);
	fail_if(rdata[1] != 0);
	fail_if(rdata[2] != 2000000000L);
	call_01(ffclos, f);
}

static void
test_ffgcv_float(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "1E" };
	float wdata[] = { -1.5f, 0.0f, 3.14159f };
	float rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TFLOAT, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - (-1.5f)) > 1e-6);
	fail_if(fabs(rdata[1] - 0.0f) > 1e-6);
	fail_if(fabs(rdata[2] - 3.14159f) > 1e-5);
	call_01(ffclos, f);
}

static void
test_ffgcv_double(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "1D" };
	double wdata[] = { -1.5, 0.0, 3.14159265358979 };
	double rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDOUBLE, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - (-1.5)) > 1e-10);
	fail_if(fabs(rdata[1] - 0.0) > 1e-10);
	fail_if(fabs(rdata[2] - 3.14159265358979) > 1e-10);
	call_01(ffclos, f);
}

static void
test_ffgcv_byte(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "1B" };
	unsigned char wdata[] = { 0, 127, 255 };
	unsigned char rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 127);
	fail_if(rdata[2] != 255);
	call_01(ffclos, f);
}

static void
test_ffgcv_sbyte(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SBCOL" };
	char *tform[] = { "1S" };
	signed char wdata[] = { -128, 0, 127 };
	signed char rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclsb, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSBYTE, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != -128);
	fail_if(rdata[1] != 0);
	fail_if(rdata[2] != 127);
	call_01(ffclos, f);
}

static void
test_ffgcv_ushort(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UICOL" };
	char *tform[] = { "1U" };
	unsigned short wdata[] = { 0, 30000, 65535 };
	unsigned short rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclui, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TUSHORT, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 30000);
	fail_if(rdata[2] != 65535);
	call_01(ffclos, f);
}

static void
test_ffgcv_uint(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UKCOL" };
	char *tform[] = { "1V" };
	unsigned int wdata[] = { 0, 2000000000U, 4000000000U };
	unsigned int rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluk, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TUINT, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 2000000000U);
	fail_if(rdata[2] != 4000000000U);
	call_01(ffclos, f);
}

static void
test_ffgcv_int(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1J" };
	int wdata[] = { -2000000000, 0, 2000000000 };
	int rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclk, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TINT, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != -2000000000);
	fail_if(rdata[1] != 0);
	fail_if(rdata[2] != 2000000000);
	call_01(ffclos, f);
}

static void
test_ffgcv_ulong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "UJCOL" };
	char *tform[] = { "1V" };
	unsigned long wdata[] = { 0, 2000000000UL, 4000000000UL };
	unsigned long rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcluj, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TULONG, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 2000000000UL);
	fail_if(rdata[2] != 4000000000UL);
	call_01(ffclos, f);
}

static void
test_ffgcv_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "1K" };
	LONGLONG wdata[] = { -9000000000000LL, 0, 9000000000000LL };
	LONGLONG rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLONGLONG, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != -9000000000000LL);
	fail_if(rdata[1] != 0);
	fail_if(rdata[2] != 9000000000000LL);
	call_01(ffclos, f);
}

static void
test_ffgcv_logical(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LCOL" };
	char *tform[] = { "1L" };
	char wdata[] = { 0, 1, 1 };
	char rdata[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 1);
	fail_if(rdata[2] != 1);
	call_01(ffclos, f);
}

static void
test_ffgcv_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "10A" };
	char *wdata[] = { "hello", "world", "test" };
	char *rdata[3];
	char buf1[11], buf2[11], buf3[11];
	int anynull;

	rdata[0] = buf1;
	rdata[1] = buf2;
	rdata[2] = buf3;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(strcmp(rdata[0], "hello") != 0);
	fail_if(strcmp(rdata[1], "world") != 0);
	fail_if(strcmp(rdata[2], "test") != 0);
	call_01(ffclos, f);
}

static void
test_ffgcv_complex(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CCOL" };
	char *tform[] = { "1C" };
	float wdata[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };  /* 3 complex */
	float rdata[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclc, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TCOMPLEX, 1, 1, 1, 3, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - 1.0f) > 1e-6);
	fail_if(fabs(rdata[1] - 2.0f) > 1e-6);
	fail_if(fabs(rdata[2] - 3.0f) > 1e-6);
	fail_if(fabs(rdata[3] - 4.0f) > 1e-6);
	call_01(ffclos, f);
}

static void
test_ffgcv_dblcomplex(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "MCOL" };
	char *tform[] = { "1M" };
	double wdata[] = { 1.0, 2.0, 3.0, 4.0 };  /* 2 complex */
	double rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclm, f, 1, 1, 1, 2, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TDBLCOMPLEX, 1, 1, 1, 2, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - 1.0) > 1e-10);
	fail_if(fabs(rdata[1] - 2.0) > 1e-10);
	fail_if(fabs(rdata[2] - 3.0) > 1e-10);
	fail_if(fabs(rdata[3] - 4.0) > 1e-10);
	call_01(ffclos, f);
}

static void
test_ffgcv_bit(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "XCOL" };
	char *tform[] = { "8X" };
	/* ffpclx expects each byte to be 0/non-0 representing one bit */
	char wdata[] = { 1, 0, 1, 0, 1, 0, 1, 0 };
	char rdata[8];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 8, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBIT, 1, 1, 1, 8, NULL, rdata, &anynull);
	fail_if(rdata[0] != 1);
	fail_if(rdata[1] != 0);
	fail_if(rdata[2] != 1);
	fail_if(rdata[3] != 0);
	fail_if(rdata[4] != 1);
	fail_if(rdata[5] != 0);
	fail_if(rdata[6] != 1);
	fail_if(rdata[7] != 0);
	call_01(ffclos, f);
}

/*
 * Test ffgcv with null value substitution
 */
static void
test_ffgcv_with_nulval(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "5I" };
	short wdata[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short rdata[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Write TNULL keyword to define null value for column */
	call_04(ffukyj, f, "TNULL1", nulval, "null value");
	call_06(ffpcli, f, 1, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	short replace = -999;
	call_09(ffgcv, f, TSHORT, 1, 1, 1, 5, &replace, rdata, &anynull);
	fail_if(rdata[0] != 100);
	fail_if(rdata[1] != -999);
	fail_if(rdata[2] != 200);
	fail_if(rdata[3] != -999);
	fail_if(rdata[4] != 300);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

/*
 * Test ffgcf - read column with null flagging
 */
static void
test_ffgcf_short(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ICOL" };
	char *tform[] = { "5I" };
	short wdata[] = { 100, -32768, 200, -32768, 300 };
	short nulval = -32768;
	short rdata[5];
	char nullarray[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Write TNULL keyword to define null value for column */
	call_04(ffukyj, f, "TNULL1", nulval, "null value");
	call_06(ffpcli, f, 1, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TSHORT, 1, 1, 1, 5, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 100);
	fail_if(rdata[2] != 200);
	fail_if(rdata[4] != 300);
	fail_if(nullarray[0] != 0);
	fail_if(nullarray[1] != 1);
	fail_if(nullarray[2] != 0);
	fail_if(nullarray[3] != 1);
	fail_if(nullarray[4] != 0);
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_ffgcf_float(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ECOL" };
	char *tform[] = { "3E" };
	float wdata[] = { 1.0f, 2.0f, 3.0f };
	float rdata[3];
	char nullarray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TFLOAT, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(fabs(rdata[0] - 1.0f) > 1e-6);
	fail_if(fabs(rdata[1] - 2.0f) > 1e-6);
	fail_if(fabs(rdata[2] - 3.0f) > 1e-6);
	fail_if(nullarray[0] != 0);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_ffgcf_double(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL" };
	char *tform[] = { "3D" };
	double wdata[] = { 1.0, 2.0, 3.0 };
	double rdata[3];
	char nullarray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TDOUBLE, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(fabs(rdata[0] - 1.0) > 1e-10);
	fail_if(fabs(rdata[2] - 3.0) > 1e-10);
	call_01(ffclos, f);
}

static void
test_ffgcf_byte(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BCOL" };
	char *tform[] = { "3B" };
	unsigned char wdata[] = { 0, 127, 255 };
	unsigned char rdata[3];
	char nullarray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TBYTE, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[2] != 255);
	call_01(ffclos, f);
}

static void
test_ffgcf_long(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "JCOL" };
	char *tform[] = { "3J" };
	long wdata[] = { -1000000, 0, 1000000 };
	long rdata[3];
	char nullarray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TLONG, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(rdata[0] != -1000000);
	fail_if(rdata[2] != 1000000);
	call_01(ffclos, f);
}

static void
test_ffgcf_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "KCOL" };
	char *tform[] = { "3K" };
	LONGLONG wdata[] = { -9000000000000LL, 0, 9000000000000LL };
	LONGLONG rdata[3];
	char nullarray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TLONGLONG, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(rdata[0] != -9000000000000LL);
	fail_if(rdata[2] != 9000000000000LL);
	call_01(ffclos, f);
}

static void
test_ffgcf_logical(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LCOL" };
	char *tform[] = { "3L" };
	char wdata[] = { 0, 1, 1 };
	char rdata[3];
	char nullarray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TLOGICAL, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(rdata[0] != 0);
	fail_if(rdata[1] != 1);
	fail_if(rdata[2] != 1);
	call_01(ffclos, f);
}

static void
test_ffgcf_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ACOL" };
	char *tform[] = { "10A" };
	char *wdata[] = { "hello", "world", "test" };
	char *rdata[3];
	char buf1[11], buf2[11], buf3[11];
	char nullarray[3];
	int anynull;

	rdata[0] = buf1;
	rdata[1] = buf2;
	rdata[2] = buf3;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TSTRING, 1, 1, 1, 3, rdata, nullarray, &anynull);
	fail_if(strcmp(rdata[0], "hello") != 0);
	fail_if(strcmp(rdata[1], "world") != 0);
	fail_if(strcmp(rdata[2], "test") != 0);
	call_01(ffclos, f);
}

static void
test_ffgcf_complex(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "CCOL" };
	char *tform[] = { "2C" };
	float wdata[] = { 1.0f, 2.0f, 3.0f, 4.0f };  /* 2 complex */
	float rdata[4];
	char nullarray[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclc, f, 1, 1, 1, 2, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TCOMPLEX, 1, 1, 1, 2, rdata, nullarray, &anynull);
	fail_if(fabs(rdata[0] - 1.0f) > 1e-6);
	fail_if(fabs(rdata[1] - 2.0f) > 1e-6);
	call_01(ffclos, f);
}

static void
test_ffgcf_dblcomplex(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "MCOL" };
	char *tform[] = { "2M" };
	double wdata[] = { 1.0, 2.0, 3.0, 4.0 };  /* 2 complex */
	double rdata[4];
	char nullarray[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclm, f, 1, 1, 1, 2, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcf, f, TDBLCOMPLEX, 1, 1, 1, 2, rdata, nullarray, &anynull);
	fail_if(fabs(rdata[0] - 1.0) > 1e-10);
	fail_if(fabs(rdata[1] - 2.0) > 1e-10);
	call_01(ffclos, f);
}

/*
 * Test bad datatype error
 */
static void
test_bad_datatype(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	short wdata[] = { 1, 2, 3, 4 };
	short rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgpv(f, 999, 1, 4, NULL, rdata, &anynull, &status);
	fail_if(status != BAD_DATATYPE);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test data conversion - read short as float
 */
static void
test_data_conversion(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4 };
	short wdata[] = { -100, 0, 100, 1000 };
	float rdata[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_05(ffppri, f, 1, 1, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TFLOAT, 1, 4, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - (-100.0f)) > 1e-6);
	fail_if(fabs(rdata[1] - 0.0f) > 1e-6);
	fail_if(fabs(rdata[2] - 100.0f) > 1e-6);
	fail_if(fabs(rdata[3] - 1000.0f) > 1e-6);
	call_01(ffclos, f);
}

/*
 * Test reading 2D image with ffgsv different datatypes
 */
static void
test_ffgsv_double(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	double wdata[16];
	double rdata[4];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (double)(i + 1) * 0.1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 2, naxes);
	call_06(ffp2dd, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TDOUBLE, blc, trc, inc, NULL, rdata, &anynull);
	fail_if(fabs(rdata[0] - 0.6) > 1e-10);  /* (2,2) = index 5 -> 6*0.1 */
	fail_if(fabs(rdata[1] - 0.7) > 1e-10);
	call_01(ffclos, f);
}

static void
test_ffgsv_byte(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	unsigned char wdata[16];
	unsigned char rdata[4];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (unsigned char)((i + 1) * 10);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 2, naxes);
	call_06(ffp2db, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TBYTE, blc, trc, inc, NULL, rdata, &anynull);
	fail_if(rdata[0] != 60);  /* (2,2) = index 5 -> 6*10 */
	fail_if(rdata[1] != 70);
	call_01(ffclos, f);
}

static void
test_ffgsv_long(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	long wdata[16];
	long rdata[4];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (long)((i + 1) * 1000);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 2, naxes);
	call_06(ffp2dj, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TLONG, blc, trc, inc, NULL, rdata, &anynull);
	fail_if(rdata[0] != 6000);  /* (2,2) = index 5 -> 6*1000 */
	fail_if(rdata[1] != 7000);
	call_01(ffclos, f);
}

static void
test_ffgsv_longlong(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 4 };
	LONGLONG wdata[16];
	LONGLONG rdata[4];
	long blc[] = { 2, 2 };
	long trc[] = { 3, 3 };
	long inc[] = { 1, 1 };
	int anynull;
	int i;

	for (i = 0; i < 16; i += 1) {
		wdata[i] = (LONGLONG)((i + 1) * 1000000000LL);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 2, naxes);
	call_06(ffp2djj, f, 1, 4, 4, 4, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_08(ffgsv, f, TLONGLONG, blc, trc, inc, NULL, rdata, &anynull);
	fail_if(rdata[0] != 6000000000LL);
	fail_if(rdata[1] != 7000000000LL);
	call_01(ffclos, f);
}

static void
test_ffgcvn(void)
{
	/* Test ffgcvn - read from multiple columns at once. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2", "COL3" };
	char *tform[] = { "1J", "1E", "1D" };
	long col1_data[] = { 100, 200, 300 };
	float col2_data[] = { 1.5f, 2.5f, 3.5f };
	double col3_data[] = { 10.1, 20.2, 30.3 };
	long col1_result[3];
	float col2_result[3];
	double col3_result[3];
	void *arrays[3];
	void *nulvals[3];
	int datatypes[] = { TLONG, TFLOAT, TDOUBLE };
	int colnums[] = { 1, 2, 3 };
	int anynul[3];
	long nul1 = 0;
	float nul2 = 0.0f;
	double nul3 = 0.0;

	arrays[0] = col1_result;
	arrays[1] = col2_result;
	arrays[2] = col3_result;
	nulvals[0] = &nul1;
	nulvals[1] = &nul2;
	nulvals[2] = &nul3;

	/* Create table with data. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 3, ttype, tform, NULL, "DATA");
	call_06(ffpclj, f, 1, 1, 1, 3, col1_data);
	call_06(ffpcle, f, 2, 1, 1, 3, col2_data);
	call_06(ffpcld, f, 3, 1, 1, 3, col3_data);
	call_01(ffclos, f);

	/* Read all 3 columns at once. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcvn, f, 3, datatypes, colnums, 1, 3, nulvals, arrays, anynul);
	fail_if(col1_result[0] != 100);
	fail_if(col1_result[2] != 300);
	fail_if(col2_result[1] < 2.4f || col2_result[1] > 2.6f);
	fail_if(col3_result[2] < 30.2 || col3_result[2] > 30.4);
	call_01(ffclos, f);
}

int
main(void)
{
	/* Test ffgpv with various datatypes */
	test_ffgpv_short();
	test_ffgpv_long();
	test_ffgpv_float();
	test_ffgpv_double();
	test_ffgpv_byte();
	test_ffgpv_sbyte();
	test_ffgpv_ushort();
	test_ffgpv_uint();
	test_ffgpv_int();
	test_ffgpv_ulong();
	test_ffgpv_longlong();
	test_ffgpv_with_nulval();

	/* Test ffgpf with null flagging */
	test_ffgpf_short();
	test_ffgpf_float();
	test_ffgpf_double();
	test_ffgpf_byte();
	test_ffgpf_long();

	/* Test ffgpxv pixel coordinate reading */
	test_ffgpxv_1d();
	test_ffgpxv_2d();
	test_ffgpxvll_1d();

	/* Test ffgpxf with null flagging */
	test_ffgpxf_1d();
	test_ffgpxfll_1d();

	/* Test ffgsv subsection reading */
	test_ffgsv_whole_image();
	test_ffgsv_subsection();
	test_ffgsv_with_stride();
	test_ffgsv_float();
	test_ffgsv_double();
	test_ffgsv_byte();
	test_ffgsv_long();
	test_ffgsv_longlong();

	/* Test ffgcv column reading */
	test_ffgcv_short();
	test_ffgcv_long();
	test_ffgcv_float();
	test_ffgcv_double();
	test_ffgcv_byte();
	test_ffgcv_sbyte();
	test_ffgcv_ushort();
	test_ffgcv_uint();
	test_ffgcv_int();
	test_ffgcv_ulong();
	test_ffgcv_longlong();
	test_ffgcv_logical();
	test_ffgcv_string();
	test_ffgcv_complex();
	test_ffgcv_dblcomplex();
	test_ffgcv_bit();
	test_ffgcv_with_nulval();

	/* Test ffgcf column reading with null flagging */
	test_ffgcf_short();
	test_ffgcf_float();
	test_ffgcf_double();
	test_ffgcf_byte();
	test_ffgcf_long();
	test_ffgcf_longlong();
	test_ffgcf_logical();
	test_ffgcf_string();
	test_ffgcf_complex();
	test_ffgcf_dblcomplex();

	/* Test error conditions */
	test_bad_datatype();

	/* Test data conversion */
	test_data_conversion();

	/* Test ffgcvn - multi-column read */
	test_ffgcvn();

	remove(test_path);

	return 0;
}
