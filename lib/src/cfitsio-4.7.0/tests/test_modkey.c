/*
 * Tests for modkey.c - keyword modification, insertion, and deletion functions.
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_modkey.fits"


static void
test_ffuky_string(void)
{
	fitsfile *f;
	int status = 0;
	char value[FLEN_VALUE];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Update when keyword does not exist - should insert */
	call_04(ffukys, f, "TESTKEY", "value1", "comment1");
	call_04(ffgkys, f, "TESTKEY", value, NULL);
	fail_if(strcmp(value, "value1") != 0);
	/* Update when keyword exists - should modify */
	call_04(ffukys, f, "TESTKEY", "value2", "comment2");
	call_04(ffgkys, f, "TESTKEY", value, NULL);
	fail_if(strcmp(value, "value2") != 0);
	call_01(ffclos, f);
}

static void
test_ffuky_logical(void)
{
	fitsfile *f;
	int status = 0;
	int value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Update when keyword does not exist */
	call_04(ffukyl, f, "LOGKEY", 1, "logical true");
	call_04(ffgkyl, f, "LOGKEY", &value, NULL);
	fail_if(value != 1);
	/* Update when keyword exists */
	call_04(ffukyl, f, "LOGKEY", 0, "logical false");
	call_04(ffgkyl, f, "LOGKEY", &value, NULL);
	fail_if(value != 0);
	call_01(ffclos, f);
}

static void
test_ffuky_longlong(void)
{
	fitsfile *f;
	int status = 0;
	LONGLONG value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffukyj, f, "LLKEY", 123456789012LL, "longlong value");
	call_04(ffgkyjj, f, "LLKEY", &value, NULL);
	fail_if(value != 123456789012LL);
	/* Update existing */
	call_04(ffukyj, f, "LLKEY", -987654321012LL, "updated");
	call_04(ffgkyjj, f, "LLKEY", &value, NULL);
	fail_if(value != -987654321012LL);
	call_01(ffclos, f);
}

static void
test_ffuky_ulonglong(void)
{
	fitsfile *f;
	int status = 0;
	ULONGLONG value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffukyuj, f, "ULLKEY", 18446744073709551000ULL, "ulonglong");
	call_04(ffgkyujj, f, "ULLKEY", &value, NULL);
	fail_if(value != 18446744073709551000ULL);
	call_01(ffclos, f);
}

static void
test_ffuky_float_fixed(void)
{
	fitsfile *f;
	int status = 0;
	float value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukyf, f, "FLTKEY", 3.14159f, 5, "float fixed");
	call_04(ffgkye, f, "FLTKEY", &value, NULL);
	fail_if(fabs(value - 3.14159f) > 0.0001f);
	call_01(ffclos, f);
}

static void
test_ffuky_float_exp(void)
{
	fitsfile *f;
	int status = 0;
	float value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukye, f, "FLTEXP", 1.23e10f, 4, "float exponential");
	call_04(ffgkye, f, "FLTEXP", &value, NULL);
	fail_if(fabs(value - 1.23e10f) / 1.23e10f > 0.001f);
	call_01(ffclos, f);
}

static void
test_ffuky_double_fixed(void)
{
	fitsfile *f;
	int status = 0;
	double value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukyg, f, "DBLKEY", 3.141592653589793, 10, "double fixed");
	call_04(ffgkyd, f, "DBLKEY", &value, NULL);
	fail_if(fabs(value - 3.141592653589793) > 1e-10);
	call_01(ffclos, f);
}

static void
test_ffuky_double_exp(void)
{
	fitsfile *f;
	int status = 0;
	double value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukyd, f, "DBLEXP", 1.23456789e20, 8, "double exponential");
	call_04(ffgkyd, f, "DBLEXP", &value, NULL);
	fail_if(fabs(value - 1.23456789e20) / 1.23456789e20 > 1e-8);
	call_01(ffclos, f);
}

static void
test_ffuky_undefined(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffukyu, f, "UNDEF", "undefined keyword");
	call_03(ffgcrd, f, "UNDEF", card);
	fail_if(strstr(card, "UNDEF") == NULL);
	call_01(ffclos, f);
}

static void
test_ffuky_complex_float(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = {1.5f, 2.5f};
	float result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukyc, f, "CMPLXF", cval, 4, "complex float");
	call_04(ffgkyc, f, "CMPLXF", result, NULL);
	fail_if(fabs(result[0] - 1.5f) > 0.01f || fabs(result[1] - 2.5f) > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffuky_complex_double(void)
{
	fitsfile *f;
	int status = 0;
	double cval[2] = {1.23456789, 9.87654321};
	double result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukym, f, "CMPLXD", cval, 8, "complex double");
	call_04(ffgkym, f, "CMPLXD", result, NULL);
	fail_if(fabs(result[0] - 1.23456789) > 1e-6);
	fail_if(fabs(result[1] - 9.87654321) > 1e-6);
	call_01(ffclos, f);
}

static void
test_ffuky_complex_float_fixed(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = {10.5f, 20.5f};
	float result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukfc, f, "CMPLXFF", cval, 2, "complex float fixed");
	call_04(ffgkyc, f, "CMPLXFF", result, NULL);
	fail_if(fabs(result[0] - 10.5f) > 0.1f || fabs(result[1] - 20.5f) > 0.1f);
	call_01(ffclos, f);
}

static void
test_ffuky_complex_double_fixed(void)
{
	fitsfile *f;
	int status = 0;
	double cval[2] = {100.125, 200.875};
	double result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffukfm, f, "CMPLXDF", cval, 3, "complex double fixed");
	call_04(ffgkym, f, "CMPLXDF", result, NULL);
	fail_if(fabs(result[0] - 100.125) > 0.01);
	fail_if(fabs(result[1] - 200.875) > 0.01);
	call_01(ffclos, f);
}

static void
test_ffuky_long_string(void)
{
	fitsfile *f;
	int status = 0;
	char *longstr = "This is a very long string that exceeds the normal "
		"FITS keyword value length limit of 68 characters and requires "
		"CONTINUE keywords to store properly.";
	char *result;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffukls, f, "LONGSTR", longstr, "long string");
	call_04(ffgkls, f, "LONGSTR", &result, NULL);
	fail_if(strcmp(result, longstr) != 0);
	fffree(result, &status);
	call_01(ffclos, f);
}

static void
test_ffuky_update_card(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "OLDKEY", "oldvalue", "old comment");
	call_03(ffucrd, f, "OLDKEY", "OLDKEY  = 'newvalue' / new comment");
	call_03(ffgcrd, f, "OLDKEY", card);
	fail_if(strstr(card, "newvalue") == NULL);
	call_01(ffclos, f);
}

static void
test_ffuky_generic(void)
{
	fitsfile *f;
	int status = 0;
	char sval[] = "teststring";
	unsigned char bval = 42;
	signed char sbval = -42;
	unsigned short usval = 1000;
	short shval = -1000;
	int ival = 123456;
	unsigned int uival = 4000000000U;
	int lval = 1;
	unsigned long ulval = 3000000000UL;
	ULONGLONG ullval = 18000000000000000000ULL;
	long lngval = 1234567890L;
	LONGLONG llval = 9876543210LL;
	float fval = 3.14f;
	double dval = 2.71828;
	float cval[2] = {1.0f, 2.0f};
	double mval[2] = {3.0, 4.0};

	char rstr[FLEN_VALUE];
	LONGLONG rll;
	float rflt;
	double rdbl;
	int rint;
	float rcmplx[2];
	double rdmplx[2];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	call_05(ffuky, f, TSTRING, "GENSTR", sval, "string");
	call_04(ffgkys, f, "GENSTR", rstr, NULL);
	fail_if(strcmp(rstr, sval) != 0);

	call_05(ffuky, f, TBYTE, "GENBYTE", &bval, "byte");
	call_04(ffgkyjj, f, "GENBYTE", &rll, NULL);
	fail_if(rll != 42);

	call_05(ffuky, f, TSBYTE, "GENSBYTE", &sbval, "sbyte");
	call_04(ffgkyjj, f, "GENSBYTE", &rll, NULL);
	fail_if(rll != -42);

	call_05(ffuky, f, TUSHORT, "GENUSHRT", &usval, "ushort");
	call_04(ffgkyjj, f, "GENUSHRT", &rll, NULL);
	fail_if(rll != 1000);

	call_05(ffuky, f, TSHORT, "GENSHORT", &shval, "short");
	call_04(ffgkyjj, f, "GENSHORT", &rll, NULL);
	fail_if(rll != -1000);

	call_05(ffuky, f, TINT, "GENINT", &ival, "int");
	call_04(ffgkyjj, f, "GENINT", &rll, NULL);
	fail_if(rll != 123456);

	call_05(ffuky, f, TUINT, "GENUINT", &uival, "uint");
	call_04(ffgkyd, f, "GENUINT", &rdbl, NULL);
	fail_if(fabs(rdbl - 4000000000.0) > 1.0);

	call_05(ffuky, f, TLOGICAL, "GENLOG", &lval, "logical");
	call_04(ffgkyl, f, "GENLOG", &rint, NULL);
	fail_if(rint != 1);

	call_05(ffuky, f, TULONG, "GENULONG", &ulval, "ulong");
	rll = 0;
	call_04(ffgkyujj, f, "GENULONG", (ULONGLONG *)&rll, NULL);
	fail_if((ULONGLONG)rll != 3000000000UL);

	call_05(ffuky, f, TULONGLONG, "GENULL", &ullval, "ulonglong");
	{
		ULONGLONG rull;
		call_04(ffgkyujj, f, "GENULL", &rull, NULL);
		fail_if(rull != 18000000000000000000ULL);
	}

	call_05(ffuky, f, TLONG, "GENLONG", &lngval, "long");
	call_04(ffgkyjj, f, "GENLONG", &rll, NULL);
	fail_if(rll != 1234567890LL);

	call_05(ffuky, f, TLONGLONG, "GENLL", &llval, "longlong");
	call_04(ffgkyjj, f, "GENLL", &rll, NULL);
	fail_if(rll != 9876543210LL);

	call_05(ffuky, f, TFLOAT, "GENFLT", &fval, "float");
	call_04(ffgkye, f, "GENFLT", &rflt, NULL);
	fail_if(fabs(rflt - 3.14f) > 0.01f);

	call_05(ffuky, f, TDOUBLE, "GENDBL", &dval, "double");
	call_04(ffgkyd, f, "GENDBL", &rdbl, NULL);
	fail_if(fabs(rdbl - 2.71828) > 1e-5);

	call_05(ffuky, f, TCOMPLEX, "GENCMPLX", cval, "complex");
	call_04(ffgkyc, f, "GENCMPLX", rcmplx, NULL);
	fail_if(fabs(rcmplx[0] - 1.0f) > 0.01f);

	call_05(ffuky, f, TDBLCOMPLEX, "GENDCMPLX", mval, "dblcomplex");
	call_04(ffgkym, f, "GENDCMPLX", rdmplx, NULL);
	fail_if(fabs(rdmplx[0] - 3.0) > 0.01);

	call_01(ffclos, f);
}

static void
test_ffuky_bad_datatype(void)
{
	fitsfile *f;
	int status = 0;
	int val = 1;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	ffuky(f, 9999, "BADTYPE", &val, "bad type", &status);
	fail_if(status != BAD_DATATYPE);
	status = 0;
	call_01(ffclos, f);
}


static void
test_ffmrec(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "val1", "comment1");
	call_03(ffmrec, f, 7, "KEY1    = 'modified' / new comment");
	call_03(ffgcrd, f, "KEY1", card);
	fail_if(strstr(card, "modified") == NULL);
	call_01(ffclos, f);
}

static void
test_ffmcrd(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "original", "original comment");
	call_03(ffmcrd, f, "KEY1", "KEY1    = 'changed' / changed comment");
	call_03(ffgcrd, f, "KEY1", card);
	fail_if(strstr(card, "changed") == NULL);
	call_01(ffclos, f);
}

static void
test_ffmnam(void)
{
	fitsfile *f;
	int status = 0;
	char value[FLEN_VALUE];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "OLDNAME", "testval", "comment");
	call_03(ffmnam, f, "OLDNAME", "NEWNAME");
	call_04(ffgkys, f, "NEWNAME", value, NULL);
	fail_if(strcmp(value, "testval") != 0);
	/* Old name should not exist */
	ffgkys(f, "OLDNAME", value, NULL, &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffmcom(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "TESTKEY", "value", "old comment");
	call_03(ffmcom, f, "TESTKEY", "new comment");
	call_03(ffgcrd, f, "TESTKEY", card);
	fail_if(strstr(card, "new comment") == NULL);
	call_01(ffclos, f);
}

static void
test_ffpunt(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "DIST", 100, "distance");
	call_03(ffpunt, f, "DIST", "kpc");
	call_03(ffgcrd, f, "DIST", card);
	fail_if(strstr(card, "[kpc]") == NULL);
	call_01(ffclos, f);
}

static void
test_ffpunt_replace_units(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "DIST", 100, "[pc] distance");
	call_03(ffpunt, f, "DIST", "kpc");
	call_03(ffgcrd, f, "DIST", card);
	fail_if(strstr(card, "[kpc]") == NULL);
	fail_if(strstr(card, "[pc]") != NULL);
	call_01(ffclos, f);
}

static void
test_ffpunt_empty(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "DIST", 100, "[pc] distance");
	call_03(ffpunt, f, "DIST", "");
	call_03(ffgcrd, f, "DIST", card);
	/* Units should be removed */
	fail_if(strstr(card, "[") != NULL);
	call_01(ffclos, f);
}

/*
 * Test ffpunt on keyword with no existing units (else branch when no
 * [ in comment)
 */
static void
test_ffpunt_no_existing_units(void)
{

	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "DIST", 100, "distance no units");
	call_03(ffpunt, f, "DIST", "kpc");
	call_03(ffgcrd, f, "DIST", card);
	/* Should have [kpc] and old comment */
	fail_if(strstr(card, "[kpc]") == NULL);
	fail_if(strstr(card, "distance") == NULL);
	call_01(ffclos, f);
}

/* Test ffpunt on keyword with malformed units ([ but no ]) */
static void
test_ffpunt_malformed_units(void)
{

	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Write a card with malformed units using ffprec */
	call_02(ffprec, f, "DIST    =                  100 / [malformed units no close bracket");
	call_03(ffpunt, f, "DIST", "kpc");
	call_03(ffgcrd, f, "DIST", card);
	fail_if(strstr(card, "[kpc]") == NULL);
	call_01(ffclos, f);
}

static void
test_ffmkyu(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "value", "comment");
	call_03(ffmkyu, f, "KEY1", "now undefined");
	call_03(ffgcrd, f, "KEY1", card);
	/* Should have comment but no value (just spaces) */
	fail_if(strstr(card, "now undefined") == NULL);
	call_01(ffclos, f);
}

/* Test ffmkyu with NULL comment to preserve old comment */
static void
test_ffmkyu_preserve_comment(void)
{

	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "value", "original comment");
	call_03(ffmkyu, f, "KEY1", NULL);
	call_03(ffgcrd, f, "KEY1", card);
	/* Should preserve original comment */
	fail_if(strstr(card, "original comment") == NULL);
	call_01(ffclos, f);
}

/* Test ffmkyu with "&" comment to preserve old comment */
static void
test_ffmkyu_ampersand_comment(void)
{

	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "value", "keep this");
	call_03(ffmkyu, f, "KEY1", "&");
	call_03(ffgcrd, f, "KEY1", card);
	fail_if(strstr(card, "keep this") == NULL);
	call_01(ffclos, f);
}

static void
test_ffmkys(void)
{
	fitsfile *f;
	int status = 0;
	char value[FLEN_VALUE];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "STRKEY", "oldval", "old comment");
	call_04(ffmkys, f, "STRKEY", "newval", "new comment");
	call_04(ffgkys, f, "STRKEY", value, NULL);
	fail_if(strcmp(value, "newval") != 0);
	call_01(ffclos, f);
}

static void
test_ffmkys_preserve_comment(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "STRKEY", "oldval", "preserve this");
	call_04(ffmkys, f, "STRKEY", "newval", "&");
	call_03(ffgcrd, f, "STRKEY", card);
	fail_if(strstr(card, "preserve this") == NULL);
	call_01(ffclos, f);
}

/*
 * Test ffmkys on a keyword that has CONTINUE cards. This exercises
 * the continuation deletion path
 */
static void
test_ffmkys_delete_continuation(void)
{

	fitsfile *f;
	int status = 0;
	char longstr[256];
	char value[FLEN_VALUE];
	int nkeys_before, nkeys_after;

	/* Build a string long enough to require CONTINUE (>68 chars) */
	strcpy(longstr, "This is a very long string that exceeds the normal "
		"FITS keyword value length and requires CONTINUE keywords to "
		"store properly in the header.");

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Write long string with ffpkls - creates CONTINUE cards */
	call_04(ffpkls, f, "CONTKEY", longstr, "long string");
	call_03(ffghsp, f, &nkeys_before, NULL);
	/* Use ffmkys to modify to short value - should delete CONTINUE */
	call_04(ffmkys, f, "CONTKEY", "short", "now short");
	call_03(ffghsp, f, &nkeys_after, NULL);
	fail_if(nkeys_after >= nkeys_before);
	call_04(ffgkys, f, "CONTKEY", value, NULL);
	fail_if(strcmp(value, "short") != 0);
	call_01(ffclos, f);
}

static void
test_ffmkyl(void)
{
	fitsfile *f;
	int status = 0;
	int value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyl, f, "LOGKEY", 0, "was false");
	call_04(ffmkyl, f, "LOGKEY", 1, "now true");
	call_04(ffgkyl, f, "LOGKEY", &value, NULL);
	fail_if(value != 1);
	call_01(ffclos, f);
}

static void
test_ffmkyj(void)
{
	fitsfile *f;
	int status = 0;
	LONGLONG value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "INTKEY", 100, "original");
	call_04(ffmkyj, f, "INTKEY", 200, "modified");
	call_04(ffgkyjj, f, "INTKEY", &value, NULL);
	fail_if(value != 200);
	call_01(ffclos, f);
}

static void
test_ffmkyuj(void)
{
	fitsfile *f;
	int status = 0;
	ULONGLONG value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyuj, f, "ULLKEY", 1000ULL, "original");
	call_04(ffmkyuj, f, "ULLKEY", 2000ULL, "modified");
	call_04(ffgkyujj, f, "ULLKEY", &value, NULL);
	fail_if(value != 2000ULL);
	call_01(ffclos, f);
}

/* Test ffmkyuj with "&" comment to preserve old comment */
static void
test_ffmkyuj_preserve_comment(void)
{

	fitsfile *f;
	int status = 0;
	ULONGLONG value;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyuj, f, "ULLKEY", 1000ULL, "keep this comment");
	call_04(ffmkyuj, f, "ULLKEY", 2000ULL, "&");
	call_04(ffgkyujj, f, "ULLKEY", &value, NULL);
	fail_if(value != 2000ULL);
	call_03(ffgcrd, f, "ULLKEY", card);
	fail_if(strstr(card, "keep this") == NULL);
	call_01(ffclos, f);
}

static void
test_ffmkyf(void)
{
	fitsfile *f;
	int status = 0;
	float value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyf, f, "FLTKEY", 1.5f, 2, "original");
	call_05(ffmkyf, f, "FLTKEY", 2.5f, 2, "modified");
	call_04(ffgkye, f, "FLTKEY", &value, NULL);
	fail_if(fabs(value - 2.5f) > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffmkye(void)
{
	fitsfile *f;
	int status = 0;
	float value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkye, f, "FLTEXP", 1.5e5f, 3, "original");
	call_05(ffmkye, f, "FLTEXP", 2.5e5f, 3, "modified");
	call_04(ffgkye, f, "FLTEXP", &value, NULL);
	fail_if(fabs(value - 2.5e5f) / 2.5e5f > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffmkyg(void)
{
	fitsfile *f;
	int status = 0;
	double value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyg, f, "DBLKEY", 1.234, 4, "original");
	call_05(ffmkyg, f, "DBLKEY", 5.678, 4, "modified");
	call_04(ffgkyd, f, "DBLKEY", &value, NULL);
	fail_if(fabs(value - 5.678) > 0.001);
	call_01(ffclos, f);
}

static void
test_ffmkyd(void)
{
	fitsfile *f;
	int status = 0;
	double value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyd, f, "DBLEXP", 1.234e10, 6, "original");
	call_05(ffmkyd, f, "DBLEXP", 5.678e10, 6, "modified");
	call_04(ffgkyd, f, "DBLEXP", &value, NULL);
	fail_if(fabs(value - 5.678e10) / 5.678e10 > 1e-6);
	call_01(ffclos, f);
}

static void
test_ffmkfc(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = {1.0f, 2.0f};
	float newval[2] = {3.0f, 4.0f};
	float result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkfc, f, "CMPLXF", cval, 2, "original");
	call_05(ffmkfc, f, "CMPLXF", newval, 2, "modified");
	call_04(ffgkyc, f, "CMPLXF", result, NULL);
	fail_if(fabs(result[0] - 3.0f) > 0.01f);
	fail_if(fabs(result[1] - 4.0f) > 0.01f);
	call_01(ffclos, f);
}

/* Test ffmkfc with "&" comment to preserve old comment */
static void
test_ffmkfc_preserve_comment(void)
{

	fitsfile *f;
	int status = 0;
	float cval[2] = {1.0f, 2.0f};
	float newval[2] = {3.0f, 4.0f};
	float result[2];
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkfc, f, "CMPLXF", cval, 2, "keep this");
	call_05(ffmkfc, f, "CMPLXF", newval, 2, "&");
	call_04(ffgkyc, f, "CMPLXF", result, NULL);
	fail_if(fabs(result[0] - 3.0f) > 0.01f);
	call_03(ffgcrd, f, "CMPLXF", card);
	fail_if(strstr(card, "keep this") == NULL);
	call_01(ffclos, f);
}

static void
test_ffmkyc(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = {1.0f, 2.0f};
	float newval[2] = {5.0f, 6.0f};
	float result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyc, f, "CMPLXE", cval, 3, "original");
	call_05(ffmkyc, f, "CMPLXE", newval, 3, "modified");
	call_04(ffgkyc, f, "CMPLXE", result, NULL);
	fail_if(fabs(result[0] - 5.0f) > 0.01f);
	fail_if(fabs(result[1] - 6.0f) > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffmkfm(void)
{
	fitsfile *f;
	int status = 0;
	double cval[2] = {1.0, 2.0};
	double newval[2] = {7.0, 8.0};
	double result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkfm, f, "CMPLXDF", cval, 3, "original");
	call_05(ffmkfm, f, "CMPLXDF", newval, 3, "modified");
	call_04(ffgkym, f, "CMPLXDF", result, NULL);
	fail_if(fabs(result[0] - 7.0) > 0.01);
	fail_if(fabs(result[1] - 8.0) > 0.01);
	call_01(ffclos, f);
}

static void
test_ffmkym(void)
{
	fitsfile *f;
	int status = 0;
	double cval[2] = {1.0, 2.0};
	double newval[2] = {9.0, 10.0};
	double result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkym, f, "CMPLXDE", cval, 4, "original");
	call_05(ffmkym, f, "CMPLXDE", newval, 4, "modified");
	call_04(ffgkym, f, "CMPLXDE", result, NULL);
	fail_if(fabs(result[0] - 9.0) > 0.01);
	fail_if(fabs(result[1] - 10.0) > 0.01);
	call_01(ffclos, f);
}

static void
test_ffmkls(void)
{
	fitsfile *f;
	int status = 0;
	char *longstr = "This is a very long string that exceeds the normal "
		"FITS keyword value length limit and requires CONTINUE keywords.";
	char *newstr = "This is a completely different long string value that "
		"also requires CONTINUE keywords for proper storage in the header.";
	char *result;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkls, f, "LONGSTR", longstr, "original");
	call_04(ffmkls, f, "LONGSTR", newstr, "modified");
	call_04(ffgkls, f, "LONGSTR", &result, NULL);
	fail_if(strcmp(result, newstr) != 0);
	fffree(result, &status);
	call_01(ffclos, f);
}


static void
test_ffirec(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "val1", "first key");
	call_04(ffpkys, f, "KEY3", "val3", "third key");
	/* Insert at position 5 (after KEY1, before KEY3) */
	call_03(ffirec, f, 5, "KEY2    = 'val2' / second key");
	/* Read all cards to verify order */
	call_03(ffgrec, f, 5, card);
	fail_if(strstr(card, "KEY2") == NULL);
	call_01(ffclos, f);
}

static void
test_ffikey(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	int nkeys;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	/* Move to position after existing keywords */
	call_02(ffmaky, f, nkeys + 1);
	call_02(ffikey, f, "NEWKEY  = 'inserted' / inserted card");
	call_03(ffgcrd, f, "NEWKEY", card);
	fail_if(strstr(card, "inserted") == NULL);
	call_01(ffclos, f);
}

static void
test_ffikyu(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_03(ffikyu, f, "UNDEF", "undefined value");
	call_03(ffgcrd, f, "UNDEF", card);
	fail_if(strstr(card, "UNDEF") == NULL);
	call_01(ffclos, f);
}

static void
test_ffikys(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	char value[FLEN_VALUE];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_04(ffikys, f, "STRINS", "inserted", "string insert");
	call_04(ffgkys, f, "STRINS", value, NULL);
	fail_if(strcmp(value, "inserted") != 0);
	call_01(ffclos, f);
}

static void
test_ffikls(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	char *longstr = "This is a very long string value that needs to be "
		"continued over multiple CONTINUE keywords in the FITS header.";
	char *result;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_04(ffikls, f, "LONGINS", longstr, "long insert");
	call_04(ffgkls, f, "LONGINS", &result, NULL);
	fail_if(strcmp(result, longstr) != 0);
	fffree(result, &status);
	call_01(ffclos, f);
}

static void
test_ffikyl(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	int value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_04(ffikyl, f, "LOGINS", 1, "logical insert");
	call_04(ffgkyl, f, "LOGINS", &value, NULL);
	fail_if(value != 1);
	call_01(ffclos, f);
}

static void
test_ffikyj(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	LONGLONG value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_04(ffikyj, f, "INTINS", 12345678901LL, "integer insert");
	call_04(ffgkyjj, f, "INTINS", &value, NULL);
	fail_if(value != 12345678901LL);
	call_01(ffclos, f);
}

static void
test_ffikyf(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	float value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikyf, f, "FLTINS", 1.234f, 3, "float insert");
	call_04(ffgkye, f, "FLTINS", &value, NULL);
	fail_if(fabs(value - 1.234f) > 0.001f);
	call_01(ffclos, f);
}

static void
test_ffikye(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	float value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikye, f, "FLTEXP", 1.234e5f, 3, "float exp insert");
	call_04(ffgkye, f, "FLTEXP", &value, NULL);
	fail_if(fabs(value - 1.234e5f) / 1.234e5f > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffikyg(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	double value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikyg, f, "DBLINS", 1.23456789, 8, "double insert");
	call_04(ffgkyd, f, "DBLINS", &value, NULL);
	fail_if(fabs(value - 1.23456789) > 1e-8);
	call_01(ffclos, f);
}

static void
test_ffikyd(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	double value;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikyd, f, "DBLEXP", 1.234567e15, 6, "double exp insert");
	call_04(ffgkyd, f, "DBLEXP", &value, NULL);
	fail_if(fabs(value - 1.234567e15) / 1.234567e15 > 1e-6);
	call_01(ffclos, f);
}

static void
test_ffikfc(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	float cval[2] = {1.5f, 2.5f};
	float result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikfc, f, "CMPLXF", cval, 2, "complex fixed insert");
	call_04(ffgkyc, f, "CMPLXF", result, NULL);
	fail_if(fabs(result[0] - 1.5f) > 0.01f);
	fail_if(fabs(result[1] - 2.5f) > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffikyc(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	float cval[2] = {1.5e3f, 2.5e3f};
	float result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikyc, f, "CMPLXE", cval, 3, "complex exp insert");
	call_04(ffgkyc, f, "CMPLXE", result, NULL);
	fail_if(fabs(result[0] - 1.5e3f) / 1.5e3f > 0.01f);
	call_01(ffclos, f);
}

static void
test_ffikfm(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	double cval[2] = {1.5, 2.5};
	double result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikfm, f, "DCMPLXF", cval, 3, "dcomplex fixed insert");
	call_04(ffgkym, f, "DCMPLXF", result, NULL);
	fail_if(fabs(result[0] - 1.5) > 0.01);
	fail_if(fabs(result[1] - 2.5) > 0.01);
	call_01(ffclos, f);
}

static void
test_ffikym(void)
{
	fitsfile *f;
	int status = 0;
	int nkeys;
	double cval[2] = {1.5e10, 2.5e10};
	double result[2];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_05(ffikym, f, "DCMPLXE", cval, 4, "dcomplex exp insert");
	call_04(ffgkym, f, "DCMPLXE", result, NULL);
	fail_if(fabs(result[0] - 1.5e10) / 1.5e10 > 0.001);
	call_01(ffclos, f);
}


static void
test_ffdkey(void)
{
	fitsfile *f;
	int status = 0;
	char value[FLEN_VALUE];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "DELKEY", "todelete", "to be deleted");
	call_04(ffgkys, f, "DELKEY", value, NULL);
	fail_if(strcmp(value, "todelete") != 0);
	call_02(ffdkey, f, "DELKEY");
	ffgkys(f, "DELKEY", value, NULL, &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffdkey_not_exist(void)
{
	fitsfile *f;
	int status = 0;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	ffdkey(f, "NOEXIST", &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffdstr(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_02(ffprec, f, "COMMENT This is a test comment to delete");
	call_02(ffdstr, f, "test comment");
	ffdstr(f, "test comment", &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffdrec(void)
{
	fitsfile *f;
	int status = 0;
	char value[FLEN_VALUE];
	int nkeys;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "KEY1", "val1", "first");
	call_04(ffpkys, f, "KEY2", "val2", "second");
	call_04(ffpkys, f, "KEY3", "val3", "third");
	call_03(ffghsp, f, &nkeys, NULL);
	/* Delete KEY2 (at position 8, after SIMPLE, BITPIX, NAXIS, EXTEND, 2 COMMENT, KEY1) */
	call_02(ffdrec, f, 8);
	/* KEY3 should still exist */
	call_04(ffgkys, f, "KEY3", value, NULL);
	fail_if(strcmp(value, "val3") != 0);
	/* KEY2 should not exist */
	ffgkys(f, "KEY2", value, NULL, &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffdrec_out_of_bounds(void)
{
	fitsfile *f;
	int status = 0;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	ffdrec(f, 0, &status);
	fail_if(status != KEY_OUT_BOUNDS);
	status = 0;
	ffdrec(f, 1000, &status);
	fail_if(status != KEY_OUT_BOUNDS);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffdkey_continued_string(void)
{
	fitsfile *f;
	int status = 0;
	char *longstr = "This is a very long string that requires CONTINUE "
		"keywords to store properly in the FITS header format.";
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkls, f, "LONGKEY", longstr, "long string");
	call_02(ffdkey, f, "LONGKEY");
	/* Verify the key and all CONTINUE cards are gone */
	ffdkey(f, "LONGKEY", &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffmcrd_undefined_value(void)
{
	/*
	* Test ffmcrd with a keyword that has an undefined value.
	* This exercises the VALUE_UNDEFINED path.
	*/
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	char value[FLEN_VALUE];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Write a card with no value (just keyword and comment) */
	call_02(ffprec, f, "TESTKEY =                      / comment only");
	/* Modify the card */
	call_03(ffmcrd, f, "TESTKEY", "TESTKEY = 'newvalue' / new comment");
	/* Verify the modification */
	call_04(ffgkys, f, "TESTKEY", value, NULL);
	fail_if(strcmp(value, "newvalue") != 0);
	call_01(ffclos, f);
}

/*
 * Test ffmcrd with a keyword that has a continued string value. This
 * requires a string > 68 chars to trigger actual CONTINUE keywords.
 * When modified, the CONTINUE cards should be deleted
 */
static void
test_ffmcrd_continued_string(void)
{

	fitsfile *f;
	int status = 0;
	char longstr[256];
	char value[FLEN_VALUE];
	char *result;
	int nkeys_before, nkeys_after;

	/* Build a string long enough to require CONTINUE (>68 chars) */
	strcpy(longstr, "This is a very long string that is more than "
		"sixty-eight characters and requires CONTINUE keywords to "
		"store in the FITS header properly.");

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Write a long string that uses continuation */
	call_04(ffpkls, f, "LONGKEY", longstr, "long string");
	/* Count keywords - should include CONTINUE cards */
	call_03(ffghsp, f, &nkeys_before, NULL);
	/* Verify long string was written correctly */
	call_04(ffgkls, f, "LONGKEY", &result, NULL);
	fail_if(strcmp(result, longstr) != 0);
	fffree(result, &status);
	/* Modify the keyword to a short value - this should delete CONTINUE */
	call_03(ffmcrd, f, "LONGKEY", "LONGKEY = 'short' / now short");
	/* Count keywords again - should be fewer now */
	call_03(ffghsp, f, &nkeys_after, NULL);
	fail_if(nkeys_after >= nkeys_before);
	/* Verify */
	call_04(ffgkys, f, "LONGKEY", value, NULL);
	fail_if(strcmp(value, "short") != 0);
	call_01(ffclos, f);
}

/*
 * Test that functions return early when status > 0. This covers the
 * error return paths at the start of many functions
 */
static void
test_error_status_returns(void)
{

	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD] = "TEST    = 'value'";
	float fcval[2] = {1.0f, 2.0f};
	double dcval[2] = {1.0, 2.0};

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test ffuky* functions with error status */
	status = 1;
	ffukys(f, "KEY", "val", NULL, &status);
	fail_if(status != 1);
	ffukyl(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffukyj(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffukyuj(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffukyf(f, "KEY", 1.0f, 2, NULL, &status);
	fail_if(status != 1);
	ffukye(f, "KEY", 1.0f, 2, NULL, &status);
	fail_if(status != 1);
	ffukyg(f, "KEY", 1.0, 2, NULL, &status);
	fail_if(status != 1);
	ffukyd(f, "KEY", 1.0, 2, NULL, &status);
	fail_if(status != 1);
	ffukyu(f, "KEY", NULL, &status);
	fail_if(status != 1);
	ffukyc(f, "KEY", fcval, 2, NULL, &status);
	fail_if(status != 1);
	ffukym(f, "KEY", dcval, 2, NULL, &status);
	fail_if(status != 1);
	ffukfc(f, "KEY", fcval, 2, NULL, &status);
	fail_if(status != 1);
	ffukfm(f, "KEY", dcval, 2, NULL, &status);
	fail_if(status != 1);

	/* Test ffmky* functions with error status */
	ffmrec(f, 1, card, &status);
	fail_if(status != 1);
	ffmcrd(f, "KEY", card, &status);
	fail_if(status != 1);
	ffmnam(f, "KEY", "NEW", &status);
	fail_if(status != 1);
	ffmcom(f, "KEY", "comment", &status);
	fail_if(status != 1);
	ffmkyu(f, "KEY", NULL, &status);
	fail_if(status != 1);
	ffmkys(f, "KEY", "val", NULL, &status);
	fail_if(status != 1);
	ffmkyl(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffmkyj(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffmkyuj(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffmkyf(f, "KEY", 1.0f, 2, NULL, &status);
	fail_if(status != 1);
	ffmkye(f, "KEY", 1.0f, 2, NULL, &status);
	fail_if(status != 1);
	ffmkyg(f, "KEY", 1.0, 2, NULL, &status);
	fail_if(status != 1);
	ffmkyd(f, "KEY", 1.0, 2, NULL, &status);
	fail_if(status != 1);
	ffmkfc(f, "KEY", fcval, 2, NULL, &status);
	fail_if(status != 1);
	ffmkyc(f, "KEY", fcval, 2, NULL, &status);
	fail_if(status != 1);
	ffmkfm(f, "KEY", dcval, 2, NULL, &status);
	fail_if(status != 1);
	ffmkym(f, "KEY", dcval, 2, NULL, &status);
	fail_if(status != 1);

	/* Test ffiky* functions with error status */
	ffirec(f, 1, card, &status);
	fail_if(status != 1);
	ffikey(f, card, &status);
	fail_if(status != 1);
	ffikyu(f, "KEY", NULL, &status);
	fail_if(status != 1);
	ffikys(f, "KEY", "val", NULL, &status);
	fail_if(status != 1);
	ffikyl(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffikyj(f, "KEY", 1, NULL, &status);
	fail_if(status != 1);
	ffikyf(f, "KEY", 1.0f, 2, NULL, &status);
	fail_if(status != 1);
	ffikye(f, "KEY", 1.0f, 2, NULL, &status);
	fail_if(status != 1);
	ffikyg(f, "KEY", 1.0, 2, NULL, &status);
	fail_if(status != 1);
	ffikyd(f, "KEY", 1.0, 2, NULL, &status);
	fail_if(status != 1);
	ffikfc(f, "KEY", fcval, 2, NULL, &status);
	fail_if(status != 1);
	ffikyc(f, "KEY", fcval, 2, NULL, &status);
	fail_if(status != 1);
	ffikfm(f, "KEY", dcval, 2, NULL, &status);
	fail_if(status != 1);
	ffikym(f, "KEY", dcval, 2, NULL, &status);
	fail_if(status != 1);

	/* Test ffd* functions with error status */
	ffdkey(f, "KEY", &status);
	fail_if(status != 1);
	ffdrec(f, 1, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test BAD_F2C error when complex value is too long. Test complex key
 * overflow in ffmkfc, ffmkyc, ffmkfm, ffmkym
 */
static void
test_complex_value_overflow(void)
{

	fitsfile *f;
	int status = 0;
	float fcval[2] = {1.0f, 2.0f};
	double dcval[2] = {1.0, 2.0};

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Create a complex key first */
	call_05(ffpkyc, f, "CPLXKEY", fcval, 2, "test complex");

	/* Try to modify with very high precision - should cause overflow */
	ffmkfc(f, "CPLXKEY", fcval, 50, NULL, &status);
	fail_if(status != BAD_F2C);
	status = 0;

	/* Similar for ffmkyc */
	ffmkyc(f, "CPLXKEY", fcval, 50, NULL, &status);
	fail_if(status != BAD_F2C);
	status = 0;

	/* Create a double complex key */
	call_05(ffpkym, f, "DCPLXKEY", dcval, 2, "test complex double");

	/* Try to modify with very high precision */
	ffmkfm(f, "DCPLXKEY", dcval, 50, NULL, &status);
	fail_if(status != BAD_F2C);
	status = 0;

	ffmkym(f, "DCPLXKEY", dcval, 50, NULL, &status);
	fail_if(status != BAD_F2C);
	status = 0;

	call_01(ffclos, f);
}

/* Test ffikls with string containing quote characters */
static void
test_insert_long_string_with_quotes(void)
{
	fitsfile *f;
	int status = 0;
	char *result = NULL;
	int nkeys;

	/* String with embedded quotes - tests quote counting loop */
	const char *value = "This string has 'single quotes' and more 'quotes' here.";

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_04(ffikls, f, "LONGSTR", value, NULL);
	call_04(ffgkls, f, "LONGSTR", &result, NULL);
	fail_if(strcmp(value, result) != 0);
	if (result) {
		free(result);
	}
	call_01(ffclos, f);
}

/*
 * Test ffikls where string segment ends with quote.
 * Creates a long string (>68 chars) that requires continuation,
 * with a quote near the segment boundary.
 */
static void
test_insert_long_string_ending_with_quote(void)
{
	fitsfile *f;
	int status = 0;
	char *result = NULL;
	int nkeys;
	const char *value = "This is a very long string that will need to "
		"continue onto the next card and has 'quote' near the end";

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffghsp, f, &nkeys, NULL);
	call_02(ffmaky, f, nkeys + 1);
	call_04(ffikls, f, "QTEST", value, NULL);
	call_04(ffgkls, f, "QTEST", &result, NULL);
	fail_if(strcmp(value, result) != 0);
	if (result) {
		free(result);
	}
	call_01(ffclos, f);
}

int
main(void)
{
	/* Update keyword tests */
	test_ffuky_string();
	test_ffuky_logical();
	test_ffuky_longlong();
	test_ffuky_ulonglong();
	test_ffuky_float_fixed();
	test_ffuky_float_exp();
	test_ffuky_double_fixed();
	test_ffuky_double_exp();
	test_ffuky_undefined();
	test_ffuky_complex_float();
	test_ffuky_complex_double();
	test_ffuky_complex_float_fixed();
	test_ffuky_complex_double_fixed();
	test_ffuky_long_string();
	test_ffuky_update_card();
	test_ffuky_generic();
	test_ffuky_bad_datatype();

	/* Modify keyword tests */
	test_ffmrec();
	test_ffmcrd();
	test_ffmnam();
	test_ffmcom();
	test_ffpunt();
	test_ffpunt_replace_units();
	test_ffpunt_empty();
	test_ffpunt_no_existing_units();
	test_ffpunt_malformed_units();
	test_ffmkyu();
	test_ffmkyu_preserve_comment();
	test_ffmkyu_ampersand_comment();
	test_ffmkys();
	test_ffmkys_preserve_comment();
	test_ffmkys_delete_continuation();
	test_ffmkyl();
	test_ffmkyj();
	test_ffmkyuj();
	test_ffmkyuj_preserve_comment();
	test_ffmkyf();
	test_ffmkye();
	test_ffmkyg();
	test_ffmkyd();
	test_ffmkfc();
	test_ffmkfc_preserve_comment();
	test_ffmkyc();
	test_ffmkfm();
	test_ffmkym();
	test_ffmkls();

	/* Insert keyword tests */
	test_ffirec();
	test_ffikey();
	test_ffikyu();
	test_ffikys();
	test_ffikls();
	test_ffikyl();
	test_ffikyj();
	test_ffikyf();
	test_ffikye();
	test_ffikyg();
	test_ffikyd();
	test_ffikfc();
	test_ffikyc();
	test_ffikfm();
	test_ffikym();

	/* Delete keyword tests */
	test_ffdkey();
	test_ffdkey_not_exist();
	test_ffdstr();
	test_ffdrec();
	test_ffdrec_out_of_bounds();
	test_ffdkey_continued_string();

	/* Additional coverage tests */
	test_ffmcrd_undefined_value();
	test_ffmcrd_continued_string();
	test_error_status_returns();
	test_complex_value_overflow();
	test_insert_long_string_with_quotes();
	test_insert_long_string_ending_with_quote();

	remove(test_path);
	return 0;
}
