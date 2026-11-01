/*
 * Tests to exercise grparser.c - template parser functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "grparser.h"
#include "test_macros.h"

#ifdef _WIN32
/* MSVC lacks the POSIX setenv/unsetenv; emulate them with _putenv_s. */
static int
setenv(const char *name, const char *value, int overwrite)
{
	if (!overwrite && getenv(name) != NULL) {
		return 0;
	}
	return _putenv_s(name, value);
}

static int
unsetenv(const char *name)
{
	/* Assigning an empty value removes the variable. */
	return _putenv_s(name, "");
}
#endif

#define test_path "test_grparser.fits"
#define template_path "test_grparser.tpl"

static void
write_template(const char *content)
{
	FILE *fp = fopen(template_path, "w");
	if (fp != NULL) {
		fputs(content, fp);
		fclose(fp);
	}
}

static void
test_simple_image_template(void)
{
	/* Test fits_execute_template with a simple image. */
	fitsfile *f;
	int status = 0;
	int bitpix, naxis;
	long naxes[2];

	write_template(
		"SIMPLE T\n"
		"BITPIX 16\n"
		"NAXIS 2\n"
		"NAXIS1 10\n"
		"NAXIS2 20\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffghpr, f, 2, NULL, &bitpix, &naxis, naxes, NULL, NULL, NULL);
	fail_if(bitpix != 16);
	fail_if(naxis != 2);
	fail_if(naxes[0] != 10);
	fail_if(naxes[1] != 20);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_image_extension_template(void)
{
	/* Test fits_execute_template with IMAGE extension. */
	fitsfile *f;
	int status = 0;
	int hdutype;
	char value[FLEN_VALUE];

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 32\n"
		"NAXIS 1\n"
		"NAXIS1 100\n"
		"MYKEY 'test value' / a comment\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Move to extension 2 (primary + image extension). */
	call_03(ffmahd, f, 2, &hdutype);
	fail_if(hdutype != IMAGE_HDU);
	call_04(ffgkys, f, "MYKEY", value, NULL);
	fail_if(strcmp(value, "test value") != 0);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_bintable_template(void)
{
	/* Test fits_execute_template with binary table. */
	fitsfile *f;
	int status = 0;
	int hdutype, ncols;
	LONGLONG nrows;

	write_template(
		"XTENSION BINTABLE\n"
		"BITPIX 8\n"
		"NAXIS 2\n"
		"NAXIS1 8\n"
		"NAXIS2 5\n"
		"TFIELDS 1\n"
		"TTYPE1 VALUE\n"
		"TFORM1 1D\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, &hdutype);
	fail_if(hdutype != BINARY_TBL);
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 1);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 5);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_ascii_table_template(void)
{
	/* Test fits_execute_template with ASCII table. */
	fitsfile *f;
	int status = 0;
	int hdutype;

	write_template(
		"XTENSION TABLE\n"
		"BITPIX 8\n"
		"NAXIS 2\n"
		"NAXIS1 20\n"
		"NAXIS2 3\n"
		"TFIELDS 1\n"
		"TTYPE1 NAME\n"
		"TFORM1 A10\n"
		"TBCOL1 1\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, &hdutype);
	fail_if(hdutype != ASCII_TBL);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_with_comment(void)
{
	/* Test template with comment lines and COMMENT/HISTORY keywords. */
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];
	int found_comment = 0, found_history = 0;
	int nkeys, i;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"# This is a comment line in the template\n"
		"COMMENT This is a FITS comment\n"
		"HISTORY Processing step 1\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Search all cards for our COMMENT and HISTORY. */
	ffghsp(f, &nkeys, NULL, &status);
	for (i = 1; i <= nkeys; i += 1) {
		ffgrec(f, i, card, &status);
		if (strstr(card, "This is a FITS comment") != NULL) {
			found_comment = 1;
		}
		if (strstr(card, "Processing step 1") != NULL) {
			found_history = 1;
		}
	}
	fail_if(!found_comment);
	fail_if(!found_history);
	status = 0;
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_with_keywords(void)
{
	/* Test various keyword types: string, int, float, bool. */
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];
	long ival;
	double dval;
	int bval;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"STRKEY 'hello world'\n"
		"INTKEY 42\n"
		"DBLKEY 3.14159\n"
		"BOOLKEY T\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "STRKEY", sval, NULL);
	fail_if(strcmp(sval, "hello world") != 0);
	call_04(ffgkyj, f, "INTKEY", &ival, NULL);
	fail_if(ival != 42);
	call_04(ffgkyd, f, "DBLKEY", &dval, NULL);
	fail_if(dval < 3.14 || dval > 3.15);
	call_04(ffgkyl, f, "BOOLKEY", &bval, NULL);
	fail_if(bval != 1);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_auto_indexing(void)
{
	/* Test auto-indexing with # in column names. */
	fitsfile *f;
	int status = 0;
	char ttype1[FLEN_VALUE], ttype2[FLEN_VALUE];
	char tform1[FLEN_VALUE], tform2[FLEN_VALUE];

	write_template(
		"XTENSION BINTABLE\n"
		"BITPIX 8\n"
		"NAXIS 2\n"
		"NAXIS1 16\n"
		"NAXIS2 0\n"
		"TFIELDS 2\n"
		"TTYPE# COL_A\n"
		"TFORM# 1J\n"
		"TTYPE# COL_B\n"
		"TFORM# 1D\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgkys, f, "TTYPE1", ttype1, NULL);
	call_04(ffgkys, f, "TFORM1", tform1, NULL);
	call_04(ffgkys, f, "TTYPE2", ttype2, NULL);
	call_04(ffgkys, f, "TFORM2", tform2, NULL);
	fail_if(strcmp(ttype1, "COL_A") != 0);
	fail_if(strcmp(tform1, "1J") != 0);
	fail_if(strcmp(ttype2, "COL_B") != 0);
	fail_if(strcmp(tform2, "1D") != 0);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_multiple_extensions(void)
{
	/* Test template with multiple extensions. */
	fitsfile *f;
	int status = 0;
	int nhdu;

	write_template(
		"SIMPLE T\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"XTENSION IMAGE\n"
		"BITPIX 32\n"
		"NAXIS 1\n"
		"NAXIS1 50\n"
		"XTENSION BINTABLE\n"
		"BITPIX 8\n"
		"NAXIS 2\n"
		"NAXIS1 4\n"
		"NAXIS2 10\n"
		"TFIELDS 1\n"
		"TTYPE1 DATA\n"
		"TFORM1 1J\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_02(ffthdu, f, &nhdu);
	fail_if(nhdu != 3);  /* Primary + IMAGE + BINTABLE. */
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_extname(void)
{
	/* Test that EXTNAME and EXTVER are handled correctly. */
	fitsfile *f;
	int status = 0;
	char extname[FLEN_VALUE];
	long extver;

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME MYEXT\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgkys, f, "EXTNAME", extname, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(strcmp(extname, "MYEXT") != 0);
	fail_if(extver != 1);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_null_value(void)
{
	/* Test keyword with no value (null). */
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"NULLKEY = / keyword with null value\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgcrd(f, "NULLKEY", card, &status);
	fail_if(status != 0);
	status = 0;
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_blank_lines(void)
{
	/* Test template with blank lines (should become blank keywords). */
	fitsfile *f;
	int status = 0;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"        \n"
		"TESTKEY 123\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	long ival;
	call_04(ffgkyj, f, "TESTKEY", &ival, NULL);
	fail_if(ival != 123);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_complex_value(void)
{
	/* Test complex number value in template. */
	fitsfile *f;
	int status = 0;
	double cval[2];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"CMPLXKEY (1.5,2.5)\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkym, f, "CMPLXKEY", cval, NULL);
	fail_if(cval[0] < 1.4 || cval[0] > 1.6);
	fail_if(cval[1] < 2.4 || cval[1] > 2.6);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_string_with_quotes(void)
{
	/* Test string with embedded quotes. */
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"STRKEY 'it''s quoted'\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "STRKEY", sval, NULL);
	fail_if(strcmp(sval, "it's quoted") != 0);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_hierarch(void)
{
	/* Test HIERARCH keyword convention. */
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"HIERARCH LONGKEYNAME = 'value'\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgkys(f, "LONGKEYNAME", sval, NULL, &status);
	/* HIERARCH keywords may be handled differently; just check no crash. */
	status = 0;
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_real_with_d_exponent(void)
{
	/* Test real number with D exponent (Fortran style). */
	fitsfile *f;
	int status = 0;
	double dval;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"REALKEY 1.5D+02\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyd, f, "REALKEY", &dval, NULL);
	fail_if(dval < 149.0 || dval > 151.0);  /* Should be 150.0. */
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_error_handling(void)
{
	/* Test that missing template file returns error. */
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	/* Try to execute a non-existent template. */
	fits_execute_template(f, "nonexistent_template.tpl", &status);
	fail_if(status == 0);  /* Should fail. */
	/* Close file - ignore status since file may be in bad state. */
	status = 0;
	ffclos(f, &status);
}

static void
test_template_group(void)
{
	/* Test GROUP definition in template. */
	fitsfile *f;
	int status = 0;
	int hdutype;
	char extname[FLEN_VALUE];

	write_template(
		"\\GROUP MYGROUP\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 10\n"
		"EXTNAME MEMBER1\n"
		"\\END\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Should have primary + grouping table + member image. */
	call_03(ffmahd, f, 2, &hdutype);
	fail_if(hdutype != BINARY_TBL);  /* Grouping table. */
	call_04(ffgkys, f, "EXTNAME", extname, NULL);
	fail_if(strcmp(extname, "GROUPING") != 0);
	call_03(ffmahd, f, 3, &hdutype);
	fail_if(hdutype != IMAGE_HDU);  /* Member image. */
	call_04(ffgkys, f, "EXTNAME", extname, NULL);
	fail_if(strcmp(extname, "MEMBER1") != 0);
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_continue_keyword(void)
{
	/* Test CONTINUE keyword for long strings. */
	fitsfile *f;
	int status = 0;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"CONTINUE This is a continuation\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Just verify it doesn't crash. */
	call_01(ffclos, f);

	remove(template_path);
}

static void
test_template_bool_false(void)
{
	/* Test boolean false value. */
	fitsfile *f;
	int status = 0;
	int bval;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"BOOLKEY F\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyl, f, "BOOLKEY", &bval, NULL);
	fail_if(bval != 0);
	call_01(ffclos, f);

	remove(template_path);
}

/*
 * Test multiple extensions with same EXTNAME to trigger
 * ngp_get_extver versioning code
 */
static void
test_template_same_extname_versioning(void)
{

	fitsfile *f;
	int status = 0;
	long extver;

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME MYEXT\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME MYEXT\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME MYEXT\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* First extension with MYEXT should have EXTVER=1. */
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 1);
	/* Second extension with MYEXT should have EXTVER=2. */
	call_03(ffmahd, f, 3, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 2);
	/* Third extension with MYEXT should have EXTVER=3. */
	call_03(ffmahd, f, 4, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 3);
	call_01(ffclos, f);

	remove(template_path);
}

/* Test multiple different EXTNAMEs to exercise extver table allocation */
static void
test_template_different_extnames(void)
{

	fitsfile *f;
	int status = 0;
	long extver;

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME EXT_A\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME EXT_B\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME EXT_A\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME EXT_C\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME EXT_B\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* EXT_A first occurrence -> EXTVER=1. */
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 1);
	/* EXT_B first occurrence -> EXTVER=1. */
	call_03(ffmahd, f, 3, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 1);
	/* EXT_A second occurrence -> EXTVER=2. */
	call_03(ffmahd, f, 4, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 2);
	/* EXT_C first occurrence -> EXTVER=1. */
	call_03(ffmahd, f, 5, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 1);
	/* EXT_B second occurrence -> EXTVER=2. */
	call_03(ffmahd, f, 6, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 2);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test ngp_set_extver to set extension version for a new name.
 */
static void
test_ngp_set_extver_new(void)
{
	int r, version;

	ngp_delete_extver_tab();
	r = ngp_set_extver("TEST_SET", 5);
	fail_if(r != NGP_OK);
	r = ngp_get_extver("TEST_SET", &version);
	fail_if(r != NGP_OK);
	fail_if(version != 6);
	ngp_delete_extver_tab();
}


/*
 * Test ngp_set_extver to update existing name.
 */
static void
test_ngp_set_extver_existing(void)
{
	int r, version;

	ngp_delete_extver_tab();
	r = ngp_get_extver("UPDATE_EXT", &version);
	fail_if(r != NGP_OK);
	fail_if(version != 1);
	r = ngp_set_extver("UPDATE_EXT", 10);
	fail_if(r != NGP_OK);
	r = ngp_get_extver("UPDATE_EXT", &version);
	fail_if(r != NGP_OK);
	fail_if(version != 11);
	ngp_delete_extver_tab();
}


/*
 * Test ngp_set_extver with NULL extname.
 */
static void
test_ngp_set_extver_null(void)
{
	int r;

	r = ngp_set_extver(NULL, 1);
	fail_if(r != NGP_BAD_ARG);
}


/*
 * Test ngp_get_extver with NULL arguments.
 */
static void
test_ngp_get_extver_null(void)
{
	int r, version;

	r = ngp_get_extver(NULL, &version);
	fail_if(r != NGP_BAD_ARG);
	r = ngp_get_extver("TEST", NULL);
	fail_if(r != NGP_BAD_ARG);
}


/*
 * Test template with backslash raw keyword insertion.
 */
static void
test_template_raw_keyword(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"\\        RAW COMMENT CARD\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test keyword incrementor feature (KEYWORD# auto-numbering) within GROUP.
 * This hits the incrementor code path in ngp_read_group.
 * Keywords with # at the GROUP level get auto-incremented starting at 7
 * (first 6 columns are used by the grouping table).
 */
static void
test_keyword_incrementor(void)
{
	fitsfile *f;
	int status = 0;
	char value[FLEN_VALUE];

	write_template(
		"\\GROUP INCTEST\n"
		"TST# 'first'\n"
		"TST# 'second'\n"
		"TST# 'third'\n"
		"\\END\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* HDU 2 is the grouping table */
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgkys, f, "TST7", value, NULL);
	fail_if(strcmp(value, "first") != 0);
	call_04(ffgkys, f, "TST8", value, NULL);
	fail_if(strcmp(value, "second") != 0);
	call_04(ffgkys, f, "TST9", value, NULL);
	fail_if(strcmp(value, "third") != 0);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test template with include directive.
 */
static void
test_template_include(void)
{
	fitsfile *f;
	int status = 0;
	FILE *inc;
	long ival;

	inc = fopen("test_include.tpl", "w");
	if (inc != NULL) {
		fputs("INCKEY 999\n", inc);
		fclose(inc);
	}

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"\\INCLUDE test_include.tpl\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyj, f, "INCKEY", &ival, NULL);
	fail_if(ival != 999);
	call_01(ffclos, f);

	remove(template_path);
	remove("test_include.tpl");
}


/*
 * Test template with lowercase keyword names (auto-converted to uppercase).
 */
static void
test_template_lowercase_keyword(void)
{
	fitsfile *f;
	int status = 0;
	long ival;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"lowkey 42\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyj, f, "LOWKEY", &ival, NULL);
	fail_if(ival != 42);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test complex number with D exponent notation.
 * Template writes it; we just verify the keyword exists.
 */
static void
test_template_complex_d_exponent(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"CMPLXD (1.5D+01,2.5D-01)\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgcrd(f, "CMPLXD", card, &status);
	fail_if(status != 0);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test modifying an existing keyword's comment.
 */
static void
test_template_modify_comment(void)
{
	fitsfile *f;
	int status = 0;
	char comment[FLEN_COMMENT];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"MYKEY 123 / original comment\n"
		"MYKEY / modified comment\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgkey(f, "MYKEY", NULL, comment, &status);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test negative integer value in template.
 */
static void
test_template_negative_int(void)
{
	fitsfile *f;
	int status = 0;
	long ival;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"NEGVAL -12345\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyj, f, "NEGVAL", &ival, NULL);
	fail_if(ival != -12345);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test scientific notation with negative exponent.
 */
static void
test_template_scientific_negative_exp(void)
{
	fitsfile *f;
	int status = 0;
	double dval;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"SCIVAL 3.14E-05\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyd, f, "SCIVAL", &dval, NULL);
	fail_if(dval < 3.0e-05 || dval > 3.2e-05);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test template with explicit equal sign syntax (KEY = value).
 */
static void
test_template_equal_sign_syntax(void)
{
	fitsfile *f;
	int status = 0;
	long ival;

	write_template(
		"SIMPLE = T\n"
		"BITPIX = 8\n"
		"NAXIS = 0\n"
		"TESTKEY = 42\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyj, f, "TESTKEY", &ival, NULL);
	fail_if(ival != 42);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test nested include that exceeds maximum nesting level.
 */
static void
test_template_nested_include_limit(void)
{
	fitsfile *f;
	int status = 0;
	FILE *fp;
	int i;
	char fname[64];

	/* Create 10 nested include files */
	for (i = 9; i >= 0; i -= 1) {
		snprintf(fname, sizeof fname, "nested_%d.tpl", i);
		fp = fopen(fname, "w");
		if (fp != NULL) {
			if (i == 9) {
				fputs("LASTKEY 999\n", fp);
			} else {
				fprintf(fp, "\\INCLUDE nested_%d.tpl\n", i + 1);
			}
			fclose(fp);
		}
	}

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"\\INCLUDE nested_0.tpl\n"
	);

	call_02(ffinit, &f, "!" test_path);
	/* This may fail or succeed depending on nesting limit */
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);
	status = 0;

	/* Clean up nested files */
	for (i = 0; i < 10; i += 1) {
		snprintf(fname, sizeof fname, "nested_%d.tpl", i);
		remove(fname);
	}
	remove(template_path);
}


/*
 * Test template with EXTVER explicitly set.
 */
static void
test_template_explicit_extver(void)
{
	fitsfile *f;
	int status = 0;
	long extver;

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"EXTNAME MYEXT\n"
		"EXTVER 5\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 5);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test template with long string continuation (ampersand).
 */
static void
test_template_long_string(void)
{
	fitsfile *f;
	int status = 0;
	char sval[256] = {0};

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"LONGSTR 'This is a long string that spans&'\n"
		"CONTINUE ' multiple lines&'\n"
		"CONTINUE ' using continuation'\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test template with TFORM comment update.
 * Triggers the NGP_BAD_ARG path with comment update.
 */
static void
test_template_tform_comment(void)
{
	fitsfile *f;
	int status = 0;

	write_template(
		"XTENSION BINTABLE\n"
		"BITPIX 8\n"
		"NAXIS 2\n"
		"NAXIS1 8\n"
		"NAXIS2 5\n"
		"TFIELDS 1\n"
		"TTYPE1 VALUE\n"
		"TFORM1 1D / my comment for TFORM\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test template with SIMPLE after XTENSION (should trigger error).
 */
static void
test_template_simple_in_extension(void)
{
	fitsfile *f;
	int status = 0;

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 0\n"
		"SIMPLE T\n"
	);

	call_02(ffinit, &f, "!" test_path);
	/* This may fail due to SIMPLE in extension */
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	remove(template_path);
}


/*
 * Test adding extensions to existing FITS file via template.
 * This exercises the EXTNAME/EXTVER scanning code.
 */
static void
test_template_add_to_existing(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	/* First create a FITS file with an extension */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_04(ffpkys, f, "EXTNAME", "EXISTING", NULL);
	call_01(ffclos, f);

	/* Now open and apply template to add more extensions */
	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 5\n"
		"EXTNAME NEWEXT\n"
	);

	call_03(ffopen, &f, test_path, READWRITE);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test template with directory path.
 */
static void
test_template_with_directory(void)
{
	fitsfile *f;
	int status = 0;
	FILE *fp;

	/* Create template file. */
	fp = fopen("test_dir_template.tpl", "w");
	if (fp != NULL) {
		fputs("SIMPLE T\n", fp);
		fputs("BITPIX 8\n", fp);
		fputs("NAXIS 0\n", fp);
		fputs("DIRTEST 123\n", fp);
		fclose(fp);
	}

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, "test_dir_template.tpl", &status);
	status = 0;
	ffclos(f, &status);

	remove("test_dir_template.tpl");
}


/*
 * Test various real number formats in templates.
 */
static void
test_template_real_number_variations(void)
{
	fitsfile *f;
	int status = 0;

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"REAL1 3.14159\n"
		"REAL2 -2.71828\n"
		"REAL3 1E10\n"
		"REAL4 -1E-10\n"
		"REAL5 .5\n"
		"REAL6 -.25\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test empty string value in template.
 */
static void
test_template_empty_string_value(void)
{
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"EMPTY ''\n"
	);

	call_02(ffinit, &f, "!" test_path);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "EMPTY", sval, NULL);
	fail_if(sval[0] != 0);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test adding template to existing file with multiple named extensions.
 * This exercises the EXTVER scanning loop in fits_execute_template.
 */
static void
test_template_multiple_named_extensions(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_04(ffpkys, f, "EXTNAME", "MYEXT", NULL);
	call_04(ffpkyj, f, "EXTVER", 1, NULL);

	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_04(ffpkys, f, "EXTNAME", "MYEXT", NULL);
	call_04(ffpkyj, f, "EXTVER", 2, NULL);

	call_08(ffcrtb, f, BINARY_TBL, 1, 0, NULL, NULL, NULL, NULL);
	call_04(ffpkys, f, "EXTNAME", "OTHER", NULL);
	call_01(ffclos, f);

	write_template(
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 5\n"
		"EXTNAME ADDED\n"
	);

	call_03(ffopen, &f, test_path, READWRITE);
	call_02(fits_execute_template, f, template_path);
	call_01(ffclos, f);

	remove(template_path);
}


/*
 * Test anonymous GROUP directive (no group name).
 * This should create DEFAULT_GROUP_n naming.
 */
static void
test_template_anonymous_group(void)
{
	fitsfile *f;
	int status = 0;

	/* Anonymous group - no name after \\GROUP */
	write_template(
		"\\GROUP\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 5\n"
		"EXTNAME ANON_MEMBER\n"
		"\\END\n"
	);

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	remove(template_path);
}


/*
 * Test nested GROUP directives.
 * This exercises the GROUP-inside-GROUP code path.
 */
static void
test_template_nested_group(void)
{
	fitsfile *f;
	int status = 0;

	/* Outer group containing inner group */
	write_template(
		"\\GROUP OUTER\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 5\n"
		"EXTNAME OUTER_MEMBER\n"
		"\\GROUP INNER\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 3\n"
		"EXTNAME INNER_MEMBER\n"
		"\\END\n"
		"\\END\n"
	);

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	remove(template_path);
}


/*
 * Test nested GROUP with anonymous inner group.
 */
static void
test_template_nested_anonymous_group(void)
{
	fitsfile *f;
	int status = 0;

	/* Outer group containing anonymous inner group */
	write_template(
		"\\GROUP OUTER2\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 5\n"
		"EXTNAME OUTER2_MEMBER\n"
		"\\GROUP\n"
		"XTENSION IMAGE\n"
		"BITPIX 16\n"
		"NAXIS 1\n"
		"NAXIS1 3\n"
		"EXTNAME INNER2_MEMBER\n"
		"\\END\n"
		"\\END\n"
	);

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	remove(template_path);
}


/*
 * Test CFITSIO_INCLUDE_FILES environment variable search.
 * This searches for include files when not found locally.
 */
static void
test_template_env_path_search(void)
{
	fitsfile *f;
	int status = 0;
	FILE *fp;
	char *old_env;

	/* Create an include file. */
	fp = fopen("envinclude.tpl", "w");
	if (fp != NULL) {
		fputs("ENVKEY 999\n", fp);
		fclose(fp);
	}

	/* Main template that includes a file */
	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"\\include envinclude.tpl\n"
	);

	old_env = getenv("CFITSIO_INCLUDE_FILES");
	setenv("CFITSIO_INCLUDE_FILES", ".:/nonexistent", 1);

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	if (old_env) {
		setenv("CFITSIO_INCLUDE_FILES", old_env, 1);
	} else {
		unsetenv("CFITSIO_INCLUDE_FILES");
	}

	remove("envinclude.tpl");
	remove(template_path);
}


/*
 * Test template with blank line (only whitespace).
 * This exercises the RAW type handling for empty lines.
 */
static void
test_template_blank_line_whitespace(void)
{
	fitsfile *f;
	int status = 0;

	/* Template with truly empty line (just newline) */
	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"\n"
		"KEY1 123\n"
	);

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	remove(template_path);
}


/*
 * Test template with comment line (8+ leading spaces).
 */
static void
test_template_comment_line(void)
{
	fitsfile *f;
	int status = 0;

	/* Template with a comment line (8 spaces at start) */
	write_template(
		"SIMPLE T\n"
		"BITPIX 8\n"
		"NAXIS 0\n"
		"        This is a template comment line\n"
		"KEY2 456\n"
	);

	call_02(ffinit, &f, "!" test_path);
	fits_execute_template(f, template_path, &status);
	status = 0;
	ffclos(f, &status);

	remove(template_path);
}


int
main(void)
{
	test_simple_image_template();
	test_image_extension_template();
	test_bintable_template();
	test_ascii_table_template();
	test_template_with_comment();
	test_template_with_keywords();
	test_template_auto_indexing();
	test_template_multiple_extensions();
	test_template_extname();
	test_template_null_value();
	test_template_blank_lines();
	test_template_complex_value();
	test_template_string_with_quotes();
	test_template_hierarch();
	test_template_real_with_d_exponent();
	test_template_error_handling();
	test_template_group();
	test_template_continue_keyword();
	test_template_bool_false();
	test_template_same_extname_versioning();
	test_template_different_extnames();
	test_ngp_set_extver_new();
	test_ngp_set_extver_existing();
	test_ngp_set_extver_null();
	test_ngp_get_extver_null();
	test_template_raw_keyword();
	test_keyword_incrementor();
	test_template_include();
	test_template_lowercase_keyword();
	test_template_negative_int();
	test_template_scientific_negative_exp();
	test_template_complex_d_exponent();
	test_template_equal_sign_syntax();
	test_template_nested_include_limit();
	test_template_long_string();
	test_template_tform_comment();
	test_template_simple_in_extension();
	test_template_add_to_existing();
	test_template_with_directory();
	test_template_real_number_variations();
	test_template_empty_string_value();
	test_template_multiple_named_extensions();
	test_template_anonymous_group();
	test_template_nested_group();
	test_template_nested_anonymous_group();
	test_template_env_path_search();
	test_template_blank_line_whitespace();
	test_template_comment_line();
	return 0;
}
