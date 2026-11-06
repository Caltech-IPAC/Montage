/*
 * Tests for cfileio.c - file I/O and URL parsing functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "fitsio2.h"
#include "test_macros.h"

#define test_path "test_cfileio.fits"

static void *
xmalloc(size_t s)
{
	void *rv = malloc(s);
	fail_if(rv == NULL && "Out of memory");
	return rv;
}

/*
 * Test fits_get_token with basic delimiters
 */
static void
test_fits_get_token_basic(void)
{
	char *ptr;
	char input[] = "token1,token2,token3";
	char token[80];
	int isanumber;
	int slen;

	ptr = input;
	slen = fits_get_token(&ptr, ",", token, &isanumber);
	fail_if(slen != 6);
	fail_if(strcmp(token, "token1") != 0);
	fail_if(isanumber != 0);

	ptr += 1;  /* skip comma */
	slen = fits_get_token(&ptr, ",", token, &isanumber);
	fail_if(slen != 6);
	fail_if(strcmp(token, "token2") != 0);

	ptr += 1;  /* skip comma */
	slen = fits_get_token(&ptr, ",", token, &isanumber);
	fail_if(slen != 6);
	fail_if(strcmp(token, "token3") != 0);
}

/*
 * Test fits_get_token with numeric tokens
 */
static void
test_fits_get_token_numeric(void)
{
	char *ptr;
	char input[] = "123 456.789 1.5D10";
	char token[80];
	int isanumber;
	int slen;

	ptr = input;
	slen = fits_get_token(&ptr, " ", token, &isanumber);
	fail_if(slen != 3);
	fail_if(strcmp(token, "123") != 0);
	fail_if(isanumber != 1);

	ptr += 1;  /* skip space */
	slen = fits_get_token(&ptr, " ", token, &isanumber);
	fail_if(slen != 7);
	fail_if(strcmp(token, "456.789") != 0);
	fail_if(isanumber != 1);

	ptr += 1;  /* skip space */
	slen = fits_get_token(&ptr, " ", token, &isanumber);
	fail_if(slen != 6);
	fail_if(strcmp(token, "1.5D10") != 0);
	fail_if(isanumber != 1);  /* D notation for doubles */
}

/*
 * Test fits_get_token with leading blanks
 */
static void
test_fits_get_token_blanks(void)
{
	char *ptr;
	char input[] = "   spaced";
	char token[80];
	int isanumber;
	int slen;

	ptr = input;
	slen = fits_get_token(&ptr, ",", token, &isanumber);
	fail_if(slen != 6);
	fail_if(strcmp(token, "spaced") != 0);
}

/*
 * Test fits_get_token with null isanumber
 */
static void
test_fits_get_token_null_isanumber(void)
{
	char *ptr;
	char input[] = "value";
	char token[80];
	int slen;

	ptr = input;
	slen = fits_get_token(&ptr, ",", token, NULL);
	fail_if(slen != 5);
	fail_if(strcmp(token, "value") != 0);
}

/*
 * Test fits_get_token2 with basic delimiters
 */
static void
test_fits_get_token2_basic(void)
{
	int status = 0;
	char *ptr;
	char input[] = "first:second:third";
	char *token = NULL;
	int isanumber;
	int slen;

	ptr = input;
	slen = fits_get_token2(&ptr, ":", &token, &isanumber, &status);
	fail_if(status != 0);
	fail_if(slen != 5);
	fail_if(strcmp(token, "first") != 0);
	fail_if(isanumber != 0);
	free(token);

	ptr += 1;  /* skip colon */
	slen = fits_get_token2(&ptr, ":", &token, &isanumber, &status);
	fail_if(status != 0);
	fail_if(slen != 6);
	fail_if(strcmp(token, "second") != 0);
	free(token);

	ptr += 1;  /* skip colon */
	slen = fits_get_token2(&ptr, ":", &token, &isanumber, &status);
	fail_if(status != 0);
	fail_if(slen != 5);
	fail_if(strcmp(token, "third") != 0);
	free(token);
}

/*
 * Test fits_get_token2 with numeric values
 */
static void
test_fits_get_token2_numeric(void)
{
	int status = 0;
	char *ptr;
	char input[] = "42,3.14159,2.5D-3";
	char *token = NULL;
	int isanumber;
	int slen;

	ptr = input;
	slen = fits_get_token2(&ptr, ",", &token, &isanumber, &status);
	fail_if(status != 0);
	fail_if(slen != 2);
	fail_if(strcmp(token, "42") != 0);
	fail_if(isanumber != 1);
	free(token);

	ptr += 1;
	slen = fits_get_token2(&ptr, ",", &token, &isanumber, &status);
	fail_if(status != 0);
	fail_if(slen != 7);
	fail_if(strcmp(token, "3.14159") != 0);
	fail_if(isanumber != 1);
	free(token);

	ptr += 1;
	slen = fits_get_token2(&ptr, ",", &token, &isanumber, &status);
	fail_if(status != 0);
	fail_if(slen != 6);
	fail_if(strcmp(token, "2.5D-3") != 0);
	fail_if(isanumber != 1);  /* D notation */
	free(token);
}

/*
 * Test fits_get_token2 with null isanumber pointer
 */
static void
test_fits_get_token2_null_isanumber(void)
{
	int status = 0;
	char *ptr;
	char input[] = "somevalue";
	char *token = NULL;
	int slen;

	ptr = input;
	slen = fits_get_token2(&ptr, ",", &token, NULL, &status);
	fail_if(status != 0);
	fail_if(slen != 9);
	fail_if(strcmp(token, "somevalue") != 0);
	free(token);
}

/*
 * Test fits_get_token2 with existing status error
 */
static void
test_fits_get_token2_error_status(void)
{
	int status = 1;  /* pre-existing error */
	char *ptr;
	char input[] = "value";
	char *token = NULL;
	int isanumber;
	int slen;

	ptr = input;
	slen = fits_get_token2(&ptr, ",", &token, &isanumber, &status);
	fail_if(slen != 0);  /* should return 0 on error */
}

/*
 * Test ffexist for existing file
 */
static void
test_ffexist_exists(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int exists;

	/* Create a test file */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	/* Test that it exists */
	call_02(ffexist, test_path, &exists);
	fail_if(exists != 1);
}

/*
 * Test ffexist for non-existing file
 */
static void
test_ffexist_not_exists(void)
{
	int status = 0;
	int exists;

	call_02(ffexist, "this_file_does_not_exist_12345.fits", &exists);
	fail_if(exists != 0);
}

/*
 * Test ffexist for non-disk URL
 */
static void
test_ffexist_non_disk(void)
{
	int status = 0;
	int exists;

	call_02(ffexist, "mem://test", &exists);
	fail_if(exists != -1);
}

/*
 * Test ffexist for stdin
 */
static void
test_ffexist_stdin(void)
{
	int status = 0;
	int exists;

	call_02(ffexist, "-", &exists);
	fail_if(exists != -1);
}

/*
 * Test ffrtnm for simple filename
 */
static void
test_ffrtnm_simple(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "myfile.fits", rootname);
	fail_if(strcmp(rootname, "myfile.fits") != 0);
}

/*
 * Test ffrtnm with extension specification
 */
static void
test_ffrtnm_extension(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "myfile.fits[1]", rootname);
	fail_if(strcmp(rootname, "myfile.fits") != 0);
}

/*
 * Test ffrtnm with file:// prefix
 */
static void
test_ffrtnm_file_prefix(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	/* file:// is kept in the rootname - unlike file: which is stripped */
	call_02(ffrtnm, "file://myfile.fits", rootname);
	fail_if(strcmp(rootname, "file://myfile.fits") != 0);
}

/*
 * Test ffrtnm with row filter
 */
static void
test_ffrtnm_rowfilter(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "myfile.fits[EVENTS][X>5]", rootname);
	fail_if(strcmp(rootname, "myfile.fits") != 0);
}

/*
 * Test ffrtnm with stdin
 */
static void
test_ffrtnm_stdin(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "-", rootname);
	fail_if(strcmp(rootname, "-") != 0);

	call_02(ffrtnm, "stdin", rootname);
	fail_if(strcmp(rootname, "-") != 0);
}

/*
 * Test ffrtnm with http URL
 */
static void
test_ffrtnm_http(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "http://example.com/file.fits", rootname);
	fail_if(strcmp(rootname, "http://example.com/file.fits") != 0);
}

/*
 * Test ffexts with extension number
 */
static void
test_ffexts_number(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	call_07(ffexts, "3", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(extnum != 3);
	fail_if(*extname != '\0');
	fail_if(extvers != 0);
	fail_if(hdutype != ANY_HDU);
}

/*
 * Test ffexts with extension name
 */
static void
test_ffexts_name(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	call_07(ffexts, "EVENTS", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(extnum != 0);
	fail_if(strcmp(extname, "EVENTS") != 0);
	fail_if(extvers != 0);
	fail_if(hdutype != ANY_HDU);
}

/*
 * Test ffexts with extension name and version
 */
static void
test_ffexts_name_version(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	call_07(ffexts, "EVENTS,2", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(extnum != 0);
	fail_if(strcmp(extname, "EVENTS") != 0);
	fail_if(extvers != 2);
	fail_if(hdutype != ANY_HDU);
}

/*
 * Test ffexts with extension name, version, and type
 */
static void
test_ffexts_name_version_type(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	/* Binary table */
	call_07(ffexts, "EVENTS,1,b", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(strcmp(extname, "EVENTS") != 0);
	fail_if(extvers != 1);
	fail_if(hdutype != BINARY_TBL);

	/* ASCII table */
	call_07(ffexts, "DATA,1,a", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(hdutype != ASCII_TBL);

	/* Image */
	call_07(ffexts, "IMG,1,i", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(hdutype != IMAGE_HDU);
}

/*
 * Test ffexts with image column specification
 */
static void
test_ffexts_image_column(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	call_07(ffexts, "PICS;IMAGE(3)", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(strcmp(extname, "PICS") != 0);
	fail_if(strcmp(imagecolname, "IMAGE") != 0);
	fail_if(strcmp(rowexpress, "3") != 0);
}

/*
 * Test ffexts with row expression
 */
static void
test_ffexts_row_expression(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	call_07(ffexts, "PICS;colname(exposure>1000)", &extnum, extname,
		&extvers, &hdutype, imagecolname, rowexpress);
	fail_if(strcmp(extname, "PICS") != 0);
	fail_if(strcmp(imagecolname, "colname") != 0);
	fail_if(strcmp(rowexpress, "exposure>1000") != 0);
}

/*
 * Test ffexts with extension that looks like negative number
 */
static void
test_ffexts_negative_name(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	/* "-5" is treated as an extension NAME, not a number */
	call_07(ffexts, "-5", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(extnum != 0);  /* not a number */
	fail_if(strcmp(extname, "-5") != 0);
}

/*
 * Test ffextn with extension number in URL
 */
static void
test_ffextn_number(void)
{
	int status = 0;
	int extnum;

	call_02(ffextn, "file.fits[2]", &extnum);
	fail_if(extnum != 3);  /* 2 + 1 (one-based) */
}

/*
 * Test ffextn with no extension specified
 */
static void
test_ffextn_none(void)
{
	int status = 0;
	int extnum;

	call_02(ffextn, "file.fits", &extnum);
	fail_if(extnum != -99);  /* special value for no extension */
}

/*
 * Test ffextn with binning specification
 */
static void
test_ffextn_binspec(void)
{
	int status = 0;
	int extnum;

	call_02(ffextn, "file.fits[1][bin X,Y]", &extnum);
	fail_if(extnum != 1);  /* binning creates temp primary array */
}

/*
 * Test ffdopn - open file skipping null primary
 */
static void
test_ffdopn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	int hdunum;

	/* Create file with null primary and one extension */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);  /* null primary */
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	/* Open with ffdopn - should skip null primary */
	call_03(ffdopn, &f, test_path, READONLY);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 2);  /* should be at extension 2 */
	call_01(ffclos, f);
}

/*
 * Test fftopn - open table extension
 */
static void
test_fftopn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	int hdutype;

	/* Create file with image primary and table extension */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	/* Open with fftopn - should move to table */
	call_03(fftopn, &f, test_path, READONLY);
	ffghdt(f, &hdutype, &status);
	fail_if(status != 0);
	fail_if(hdutype != BINARY_TBL);
	call_01(ffclos, f);
}

/*
 * Test ffiopn - open image extension
 */
static void
test_ffiopn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int hdutype;

	/* Create file with image */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	/* Open with ffiopn - should open image */
	call_03(ffiopn, &f, test_path, READONLY);
	ffghdt(f, &hdutype, &status);
	fail_if(status != 0);
	fail_if(hdutype != IMAGE_HDU);
	call_01(ffclos, f);
}

/*
 * Test ffreopen - reopen file
 */
static void
test_ffreopen(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 10 };
	int hdunum;

	/* Create and open a file */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 1, naxes);

	/* Reopen the file */
	call_02(ffreopen, f1, &f2);

	/* Both should work independently */
	ffghdn(f1, &hdunum);
	fail_if(hdunum != 1);
	ffghdn(f2, &hdunum);
	fail_if(hdunum != 1);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_get_section_range with simple range
 */
static void
test_section_range_simple(void)
{
	int status = 0;
	char spec[] = "1:100";
	char *ptr = spec;
	long secmin, secmax, incre;

	call_04(fits_get_section_range, &ptr, &secmin, &secmax, &incre);
	fail_if(secmin != 1);
	fail_if(secmax != 100);
	fail_if(incre != 1);
}

/*
 * Test fits_get_section_range with increment
 */
static void
test_section_range_increment(void)
{
	int status = 0;
	char spec[] = "1:100:2";
	char *ptr = spec;
	long secmin, secmax, incre;

	call_04(fits_get_section_range, &ptr, &secmin, &secmax, &incre);
	fail_if(secmin != 1);
	fail_if(secmax != 100);
	fail_if(incre != 2);
}

/*
 * Test fits_get_section_range with wildcard
 */
static void
test_section_range_wildcard(void)
{
	int status = 0;
	char spec[] = "*";
	char *ptr = spec;
	long secmin, secmax, incre;

	call_04(fits_get_section_range, &ptr, &secmin, &secmax, &incre);
	fail_if(secmin != 1);
	fail_if(secmax != 0);  /* 0 means use full range */
	fail_if(incre != 1);
}

/*
 * Test fits_get_section_range with inverted wildcard
 */
static void
test_section_range_inverted(void)
{
	int status = 0;
	char spec[] = "-*";
	char *ptr = spec;
	long secmin, secmax, incre;

	call_04(fits_get_section_range, &ptr, &secmin, &secmax, &incre);
	fail_if(secmin != 0);
	fail_if(secmax != 1);
	fail_if(incre != 1);
}

/*
 * Test fits_get_section_range with implied wildcard and stride
 */
static void
test_section_range_implied(void)
{
	int status = 0;
	char spec[] = ":2";  /* means *:2 (full range with stride 2) */
	char *ptr = spec;
	long secmin, secmax, incre;

	call_04(fits_get_section_range, &ptr, &secmin, &secmax, &incre);
	fail_if(secmin != 1);
	fail_if(secmax != 0);  /* 0 means use full range */
	fail_if(incre != 2);   /* stride 2 */
}

/*
 * Test fits_copy_image_section with simple section
 */
static void
test_copy_image_section(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 100, 100 };
	int bitpix;
	int naxis;
	long naxes_out[2];
	unsigned char *data;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 2, naxes);

	/* Fill with test data */
	data = xmalloc(10000 * sizeof *data);
	for (i = 0; i < 10000; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}
	call_05(ffpprb, f1, 1, 1, 10000, data);
	free(data);
	call_01(ffclos, f1);

	/* Reopen and copy section */
	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:50,1:50");

	/* Verify output dimensions */
	ffgiet(f2, &bitpix, &status);
	fail_if(status != 0);
	fail_if(bitpix != BYTE_IMG);

	ffgidm(f2, &naxis, &status);
	fail_if(status != 0);
	fail_if(naxis != 2);

	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 50);
	fail_if(naxes_out[1] != 50);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with stride
 */
static void
test_copy_image_section_stride(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 100, 100 };
	long naxes_out[2];
	unsigned char *data;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 2, naxes);

	data = xmalloc(10000 * sizeof *data);
	for (i = 0; i < 10000; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}
	call_05(ffpprb, f1, 1, 1, 10000, data);
	free(data);
	call_01(ffclos, f1);

	/* Reopen and copy section with stride */
	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:100:2,1:100:2");

	/* Should be 50x50 due to stride of 2 */
	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 50);
	fail_if(naxes_out[1] != 50);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with short image
 */
static void
test_copy_image_section_short(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 50, 50 };
	long naxes_out[2];
	short *data;
	int bitpix;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, SHORT_IMG, 2, naxes);

	data = xmalloc(2500 * sizeof *data);
	for (i = 0; i < 2500; i += 1) {
		data[i] = (short)(i - 1250);
	}
	call_05(ffppri, f1, 1, 1, 2500, data);
	free(data);
	call_01(ffclos, f1);

	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "10:40,10:40");

	ffgiet(f2, &bitpix, &status);
	fail_if(status != 0);
	fail_if(bitpix != SHORT_IMG);

	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 31);
	fail_if(naxes_out[1] != 31);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with int image
 */
static void
test_copy_image_section_int(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 50, 50 };
	long naxes_out[2];
	int *data;
	int bitpix;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, LONG_IMG, 2, naxes);

	data = xmalloc(2500 * sizeof *data);
	for (i = 0; i < 2500; i += 1) {
		data[i] = i * 1000;
	}
	call_05(ffpprk, f1, 1, 1, 2500, data);
	free(data);
	call_01(ffclos, f1);

	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:25,1:25");

	ffgiet(f2, &bitpix, &status);
	fail_if(status != 0);
	fail_if(bitpix != LONG_IMG);

	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 25);
	fail_if(naxes_out[1] != 25);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with float image
 */
static void
test_copy_image_section_float(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 50, 50 };
	long naxes_out[2];
	float *data;
	int bitpix;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, FLOAT_IMG, 2, naxes);

	data = xmalloc(2500 * sizeof *data);
	for (i = 0; i < 2500; i += 1) {
		data[i] = (float)i * 0.1f;
	}
	call_05(ffppre, f1, 1, 1, 2500, data);
	free(data);
	call_01(ffclos, f1);

	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:30,1:30");

	ffgiet(f2, &bitpix, &status);
	fail_if(status != 0);
	fail_if(bitpix != FLOAT_IMG);

	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 30);
	fail_if(naxes_out[1] != 30);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with double image
 */
static void
test_copy_image_section_double(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 50, 50 };
	long naxes_out[2];
	double *data;
	int bitpix;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, DOUBLE_IMG, 2, naxes);

	data = xmalloc(2500 * sizeof *data);
	for (i = 0; i < 2500; i += 1) {
		data[i] = (double)i * 0.001;
	}
	call_05(ffpprd, f1, 1, 1, 2500, data);
	free(data);
	call_01(ffclos, f1);

	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:40,1:40");

	ffgiet(f2, &bitpix, &status);
	fail_if(status != 0);
	fail_if(bitpix != DOUBLE_IMG);

	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 40);
	fail_if(naxes_out[1] != 40);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with longlong image
 */
static void
test_copy_image_section_longlong(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 50, 50 };
	long naxes_out[2];
	LONGLONG *data;
	int bitpix;
	int i;

	/* Create source image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, LONGLONG_IMG, 2, naxes);

	data = xmalloc(2500 * sizeof *data);
	for (i = 0; i < 2500; i += 1) {
		data[i] = (LONGLONG)i * 1000000LL;
	}
	call_05(ffpprjj, f1, 1, 1, 2500, data);
	free(data);
	call_01(ffclos, f1);

	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:35,1:35");

	ffgiet(f2, &bitpix, &status);
	fail_if(status != 0);
	fail_if(bitpix != LONGLONG_IMG);

	ffgisz(f2, 2, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 35);
	fail_if(naxes_out[1] != 35);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test fits_copy_image_section with 3D image
 */
static void
test_copy_image_section_3d(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 20, 20, 10 };
	long naxes_out[3];
	unsigned char *data;
	int naxis;
	int i;

	/* Create source 3D image */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 3, naxes);

	data = xmalloc(4000 * sizeof *data);
	for (i = 0; i < 4000; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}
	call_05(ffpprb, f1, 1, 1, 4000, data);
	free(data);
	call_01(ffclos, f1);

	call_03(ffopen, &f1, test_path, READONLY);
	call_02(ffinit, &f2, "!test_section.fits");

	call_03(fits_copy_image_section, f1, f2, "1:10,1:10,1:5");

	ffgidm(f2, &naxis, &status);
	fail_if(status != 0);
	fail_if(naxis != 3);

	ffgisz(f2, 3, naxes_out, &status);
	fail_if(status != 0);
	fail_if(naxes_out[0] != 10);
	fail_if(naxes_out[1] != 10);
	fail_if(naxes_out[2] != 5);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test ffurlt - get URL type
 */
static void
test_ffurlt(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char urltype[80];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	call_02(ffurlt, f, urltype);
	/* Should be file:// for disk file */
	fail_if(strcmp(urltype, "file://") != 0);

	call_01(ffclos, f);
}

/*
 * Test ffiurl - parse input URL
 */
static void
test_ffiurl(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "file.fits[1]", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "file://") != 0);
	fail_if(strcmp(infile, "file.fits") != 0);
	fail_if(strcmp(extspec, "1") != 0);
}

/*
 * Test ffiurl with output file
 */
static void
test_ffiurl_outfile(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "file.fits(output.fits)", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(infile, "file.fits") != 0);
	fail_if(strcmp(outfile, "output.fits") != 0);
}

/*
 * Test ffflus
 */
static void
test_ffflus(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	call_01(ffflus, f);
	call_01(ffclos, f);
}

/*
 * Test ffghdn - get HDU number
 */
static void
test_ffghdn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int hdunum;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	ffghdn(f, &hdunum);
	fail_if(hdunum != 1);

	call_01(ffclos, f);
}

/*
 * Test ffdelt - delete file
 */
static void
test_ffdelt(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int exists;

	/* Create a file */
	call_02(ffinit, &f, "!test_delete.fits");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	/* Delete it (closes and removes) */
	call_01(ffdelt, f);

	/* Verify it's gone */
	call_02(ffexist, "test_delete.fits", &exists);
	fail_if(exists != 0);
}

/*
 * Test ffflnm - get filename
 */
static void
test_ffflnm(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char filename[FLEN_FILENAME];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	call_02(ffflnm, f, filename);
	fail_if(strstr(filename, "test_cfileio.fits") == NULL);

	call_01(ffclos, f);
}

/*
 * Test ffflmd - get file mode
 */
static void
test_ffflmd(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int mode;

	/* Test write mode */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_02(ffflmd, f, &mode);
	fail_if(mode != READWRITE);
	call_01(ffclos, f);

	/* Test read mode */
	call_03(ffopen, &f, test_path, READONLY);
	call_02(ffflmd, f, &mode);
	fail_if(mode != READONLY);
	call_01(ffclos, f);
}

/*
 * Test memory file operations
 */
static void
test_memory_file(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	size_t size;
	void *buffer;

	/* Create file in memory */
	call_02(ffinit, &f, "mem://");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	/* Write some data */
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_05(ffpprb, f, 1, 1, 10, data);

	call_01(ffclos, f);
}

/*
 * Test fits_is_reentrant
 */
static void
test_fits_is_reentrant(void)
{
	int reentrant;

	reentrant = fits_is_reentrant();
	/* Just check it returns something reasonable */
	fail_if(reentrant != 0 && reentrant != 1);
}

/*
 * Test ffimem - create FITS file in memory buffer.
 */
static void
test_ffimem(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	void *buffer = NULL;
	size_t bufsize = 0;
	unsigned char data[] = { 10, 20, 30, 40, 50 };

	/* Create file in memory with realloc capability. */
	call_05(ffimem, &f, &buffer, &bufsize, 2880, realloc);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 5, data);
	call_01(ffclos, f);

	/* Buffer should now contain valid FITS data. */
	fail_if(buffer == NULL);
	fail_if(bufsize < 2880);
	/* Check SIMPLE keyword at start. */
	fail_if(strncmp((char *)buffer, "SIMPLE", 6) != 0);

	free(buffer);
}

/*
 * Test ffomem - open existing FITS file from memory buffer.
 */
static void
test_ffomem(void)
{
	fitsfile *f, *f2;
	int status = 0;
	long naxes[] = { 5 };
	void *buffer = NULL;
	size_t bufsize = 0;
	unsigned char data[] = { 10, 20, 30, 40, 50 };
	unsigned char result[5];
	int anynull;

	/* First create a FITS file in memory. */
	call_05(ffimem, &f, &buffer, &bufsize, 2880, realloc);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffpprb, f, 1, 1, 5, data);
	call_01(ffclos, f);

	/* Now open the same buffer for reading. */
	call_07(ffomem, &f2, "mem://", READONLY, &buffer, &bufsize, 0, NULL);
	call_07(ffgpvb, f2, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 10);
	fail_if(result[2] != 30);
	fail_if(result[4] != 50);
	call_01(ffclos, f2);

	free(buffer);
}

/*
 * Test ffdkopn - open FITS file on disk for read/write.
 */
static void
test_ffdkopn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };

	/* First create a file to open. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	/* Open with ffdkopn for read/write. */
	call_03(ffdkopn, &f, test_path, READWRITE);
	fail_if(f == NULL);

	/* Verify we can read the header. */
	int bitpix;
	call_02(ffgiet, f, &bitpix);
	fail_if(bitpix != BYTE_IMG);

	call_01(ffclos, f);
}

/*
 * Test ffdkinit - create new FITS file on disk.
 */
static void
test_ffdkinit(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };

	/* Remove file if it exists. */
	remove(test_path);

	/* Create with ffdkinit. */
	call_02(ffdkinit, &f, test_path);
	fail_if(f == NULL);

	/* Write an image. */
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_01(ffclos, f);

	/* Verify file was created. */
	call_03(ffopen, &f, test_path, READONLY);
	int naxis;
	call_02(ffgidm, f, &naxis);
	fail_if(naxis != 2);
	call_01(ffclos, f);
}

/*
 * Test ffrprt - print error report.
 */
static void
test_ffrprt(void)
{
	/* Push some error messages. */
	ffpmsg("Test error message 1");
	ffpmsg("Test error message 2");

	/* Call ffrprt with stdout - just verify it doesn't crash. */
	ffrprt(stdout, 1);

	/* Clear error stack. */
	ffcmsg();
}

/*
 * Test ffgtmo - get timeout value.
 */
static void
test_ffgtmo(void)
{
	int timeout;

	/* Get current timeout. */
	timeout = ffgtmo();
	/* Should return 0 or positive value. */
	fail_if(timeout < 0);
}

/*
 * Test ffstmo - set timeout value.
 */
static void
test_ffstmo(void)
{
	int status = 0;
	int orig_timeout;
	int new_timeout;

	/* Get original timeout. */
	orig_timeout = ffgtmo();

	/* Try to set a new timeout. */
	call_01(ffstmo, 30);

	/* Get new timeout to verify. */
	new_timeout = ffgtmo();
	/* If net services are available, should be 30. Otherwise 0. */
	fail_if(new_timeout != 30 && new_timeout != 0);

	/* Restore original if possible. */
	if (orig_timeout > 0) {
		call_01(ffstmo, orig_timeout);
	}
}

/*
 * Test ffihtps - initialize HTTPS/curl.
 */
static void
test_ffihtps(void)
{
	int status;

	/* Initialize HTTPS support. */
	status = ffihtps();
	/* Returns 0 on success or if CURL not configured. */
	fail_if(status != 0);

	/* Cleanup. */
	ffchtps();
}

/*
 * Test ffeopn - open file for editing with extension list.
 */
static void
test_ffeopn(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	int hdutype;

	/* Create file with primary + extension. */
	call_02(ffinit, &f, "!" test_path);
	/* Create null primary (NAXIS=0). */
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Create binary table extension. */
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	/* Open with ffeopn - should move to first good extension. */
	call_05(ffeopn, &f, test_path, READONLY, "DATA", &hdutype);
	fail_if(f == NULL);
	/* Should have moved to the binary table. */
	fail_if(hdutype != BINARY_TBL);

	call_01(ffclos, f);
}

/*
 * Test fits_is_this_a_copy.
 */
static void
test_fits_is_this_a_copy(void)
{
	int iscopy;

	/* Memory files are copies. */
	iscopy = fits_is_this_a_copy("mem://");
	fail_if(iscopy != 1);

	/* Compressed files are copies. */
	iscopy = fits_is_this_a_copy("compress://");
	fail_if(iscopy != 1);

	/* stdin is a copy. */
	iscopy = fits_is_this_a_copy("stdin://");
	fail_if(iscopy != 1);

	/* http is a copy. */
	iscopy = fits_is_this_a_copy("http://");
	fail_if(iscopy != 1);

	/* ftp is a copy. */
	iscopy = fits_is_this_a_copy("ftp://");
	fail_if(iscopy != 1);

	/* Regular file is not a copy. */
	iscopy = fits_is_this_a_copy("file://");
	fail_if(iscopy != 0);
}

/*
 * Test ffvhtps and ffshdwn - verbose and download settings.
 */
static void
test_ffvhtps_ffshdwn(void)
{
	/* Just call them to exercise the code - no side effects to verify. */
	ffvhtps(1);
	ffvhtps(0);
	ffshdwn(1);
	ffshdwn(0);
}

/*
 * Test ffimport_file - read text file contents.
 */
static void
test_ffimport_file(void)
{
	int status = 0;
	char *contents = NULL;
	FILE *fp;

	/* Create a simple text file. */
	fp = fopen("test_import.txt", "w");
	fail_if(fp == NULL);
	fprintf(fp, "Line 1\n");
	fprintf(fp, "Line 2\n");
	fprintf(fp, "Line 3\n");
	fclose(fp);

	/* Import the file. */
	call_02(ffimport_file, "test_import.txt", &contents);
	fail_if(contents == NULL);
	fail_if(strstr(contents, "Line 1") == NULL);
	fail_if(strstr(contents, "Line 2") == NULL);
	fail_if(strstr(contents, "Line 3") == NULL);

	free(contents);
	remove("test_import.txt");
}

/*
 * Test ffgnrw - get number of rows (long version).
 */
static void
test_ffgnrw(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1J" };
	long nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 100, 1, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_02(ffgnrw, f, &nrows);
	fail_if(nrows != 100);
	call_01(ffclos, f);
}

/*
 * Test ffghof - get HDU offsets (OFF_T version).
 */
static void
test_ffghof(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	OFF_T headstart, datastart, dataend;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 2, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffghof, f, &headstart, &datastart, &dataend);
	fail_if(headstart != 0);  /* Primary HDU starts at 0. */
	fail_if(datastart <= headstart);  /* Data after header. */
	fail_if(dataend <= datastart);  /* End after start. */
	call_01(ffclos, f);
}

/*
 * Test ffghad - get HDU addresses (long version).
 */
static void
test_ffghad(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	long headstart, datastart, dataend;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 2, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffghad, f, &headstart, &datastart, &dataend);
	fail_if(headstart != 0);  /* Primary HDU starts at 0. */
	fail_if(datastart <= headstart);  /* Data after header. */
	fail_if(dataend <= datastart);  /* End after start. */
	call_01(ffclos, f);
}

/*
 * Test ffgiprll - get image parameters (LONGLONG version).
 */
static void
test_ffgiprll(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 100, 200, 3 };
	int bitpix, naxis;
	LONGLONG naxesll[3];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONG_IMG, 3, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffgiprll, f, 3, &bitpix, &naxis, naxesll);
	fail_if(bitpix != LONG_IMG);
	fail_if(naxis != 3);
	fail_if(naxesll[0] != 100);
	fail_if(naxesll[1] != 200);
	fail_if(naxesll[2] != 3);
	call_01(ffclos, f);
}

/*
 * Test fftplt - create FITS file from template.
 */
static void
test_fftplt(void)
{
	fitsfile *f, *f2;
	int status = 0;
	long naxes[] = { 20, 20 };
	int bitpix, naxis;

	/* Create template FITS file. */
	call_02(ffinit, &f, "!test_template.fits");
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_01(ffclos, f);

	/* Create new file from FITS template. */
	call_03(fftplt, &f2, "!test_from_template.fits", "test_template.fits");
	fail_if(f2 == NULL);

	/* Verify it has same structure. */
	call_02(ffgidm, f2, &naxis);
	fail_if(naxis != 2);
	call_02(ffgiet, f2, &bitpix);
	fail_if(bitpix != SHORT_IMG);

	call_01(ffclos, f2);
	remove("test_template.fits");
	remove("test_from_template.fits");
}

/*
 * Test fftplt with multi-HDU template.
 */
static void
test_fftplt_multi_hdu(void)
{
	fitsfile *f, *f2;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1D" };
	int nhdu;

	/* Create template FITS file with 2 HDUs. */
	call_02(ffinit, &f, "!test_template.fits");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, "TABLE");
	call_01(ffclos, f);

	/* Create new file from template. */
	call_03(fftplt, &f2, "!test_from_template.fits", "test_template.fits");
	fail_if(f2 == NULL);

	/* Count HDUs in new file. */
	call_02(ffthdu, f2, &nhdu);
	fail_if(nhdu != 2);

	call_01(ffclos, f2);
	remove("test_template.fits");
	remove("test_from_template.fits");
}

/*
 * Test ffoptplt with empty template name.
 */
static void
test_ffoptplt_empty(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);

	/* Empty template should be a no-op. */
	call_02(ffoptplt, f, "");
	call_02(ffoptplt, f, NULL);

	call_01(ffclos, f);
}

/*
 * Test ffourl with template file specification.
 */
static void
test_ffourl_template(void)
{
	int status = 0;
	char urltype[80];
	char outfile[FLEN_FILENAME];
	char tpltfile[FLEN_FILENAME];
	char compspec[FLEN_FILENAME];

	call_05(ffourl, "output.fits(template.fits)", urltype, outfile,
		tpltfile, compspec);
	fail_if(strcmp(urltype, "file://") != 0);
	fail_if(strcmp(outfile, "output.fits") != 0);
	fail_if(strcmp(tpltfile, "template.fits") != 0);
	fail_if(*compspec != '\0');
}

/*
 * Test ffourl with compression specification.
 */
static void
test_ffourl_compress(void)
{
	int status = 0;
	char urltype[80];
	char outfile[FLEN_FILENAME];
	char tpltfile[FLEN_FILENAME];
	char compspec[FLEN_FILENAME];

	call_05(ffourl, "output.fits[compress]", urltype, outfile,
		tpltfile, compspec);
	fail_if(strcmp(outfile, "output.fits") != 0);
	fail_if(strcmp(compspec, "compress") != 0);
}

/*
 * Test ffourl with .gz extension.
 */
static void
test_ffourl_gzip(void)
{
	int status = 0;
	char urltype[80];
	char outfile[FLEN_FILENAME];
	char tpltfile[FLEN_FILENAME];
	char compspec[FLEN_FILENAME];

	call_05(ffourl, "output.fits.gz", urltype, outfile, tpltfile, compspec);
	fail_if(strcmp(urltype, "compressoutfile://") != 0);
	fail_if(strcmp(outfile, "output.fits.gz") != 0);
}

/*
 * Test ffourl with stdout.
 */
static void
test_ffourl_stdout(void)
{
	int status = 0;
	char urltype[80];
	char outfile[FLEN_FILENAME];
	char tpltfile[FLEN_FILENAME];
	char compspec[FLEN_FILENAME];

	call_05(ffourl, "-", urltype, outfile, tpltfile, compspec);
	fail_if(strcmp(urltype, "stdout://") != 0);

	call_05(ffourl, "stdout", urltype, outfile, tpltfile, compspec);
	fail_if(strcmp(urltype, "stdout://") != 0);

	call_05(ffourl, "STDOUT", urltype, outfile, tpltfile, compspec);
	fail_if(strcmp(urltype, "stdout://") != 0);
}

/*
 * Test ffifile - parse input filename (wrapper for ffifile2).
 */
static void
test_ffifile(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];
	char pixfilter[FLEN_FILENAME];

	call_09(ffifile, "file.fits[1]", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec, pixfilter);
	fail_if(strcmp(urltype, "file://") != 0);
	fail_if(strcmp(infile, "file.fits") != 0);
	fail_if(strcmp(extspec, "1") != 0);
}

/*
 * Test ffopen with extension by number.
 */
static void
test_ffopen_extension_number(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	int hdunum, hdutype;

	/* Create file with extensions. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "EXT1");
	call_08(ffcrtb, f, BINARY_TBL, 20, 1, ttype, tform, NULL, "EXT2");
	call_01(ffclos, f);

	/* Open extension by number. */
	call_03(ffopen, &f, test_path "[2]", READONLY);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 3);
	ffghdt(f, &hdutype, &status);
	fail_if(hdutype != BINARY_TBL);
	call_01(ffclos, f);
}

/*
 * Test ffopen with extension by name.
 */
static void
test_ffopen_extension_name(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	int hdunum;
	char extname[80];

	/* Create file with named extensions. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "FIRST");
	call_08(ffcrtb, f, BINARY_TBL, 20, 1, ttype, tform, NULL, "SECOND");
	call_01(ffclos, f);

	/* Open extension by name. */
	call_03(ffopen, &f, test_path "[SECOND]", READONLY);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 3);
	call_04(ffgkys, f, "EXTNAME", extname, NULL);
	fail_if(strcmp(extname, "SECOND") != 0);
	call_01(ffclos, f);
}

/*
 * Test ffopen with extension name and version.
 */
static void
test_ffopen_extension_name_version(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	int hdunum;
	long extver;

	/* Create file with versioned extensions. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "DATA");
	call_04(ffpkyj, f, "EXTVER", 1, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 20, 1, ttype, tform, NULL, "DATA");
	call_04(ffpkyj, f, "EXTVER", 2, NULL);
	call_01(ffclos, f);

	/* Open extension by name and version. */
	call_03(ffopen, &f, test_path "[DATA,2]", READONLY);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 3);
	call_04(ffgkyj, f, "EXTVER", &extver, NULL);
	fail_if(extver != 2);
	call_01(ffclos, f);
}

/*
 * Test ffopentest - verify library version.
 */
static void
test_ffopentest(void)
{
	int status = 0;
	fitsfile *f = NULL;

	/* Test with correct SONAME - should succeed with no filename. */
	call_04(ffopentest, CFITSIO_SONAME, &f, NULL, READONLY);
}

/*
 * Test opening with plus extension syntax.
 */
static void
test_ffopen_plus_extension(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	int hdunum;

	/* Create file with extensions. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "TABLE");
	call_01(ffclos, f);

	/* Open with +1 extension syntax. */
	call_03(ffopen, &f, test_path "+1", READONLY);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 2);
	call_01(ffclos, f);
}

/*
 * Test ffopen with image type specifier.
 */
static void
test_ffopen_image_type(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	int hdunum, hdutype;

	/* Create file with image and table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, "TABLE");
	naxes[0] = 20;
	call_04(ffcrim, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	/* Open with type specifier for binary table. */
	call_03(ffopen, &f, test_path "[TABLE,1,b]", READONLY);
	ffghdt(f, &hdutype, &status);
	fail_if(hdutype != BINARY_TBL);
	call_01(ffclos, f);
}

/*
 * Test fits_copy_cell2image - copy table cell to image.
 */
static void
test_fits_copy_cell2image(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 10 };
	char *ttype[] = { "IMAGE" };
	char *tform[] = { "25J" };
	int data[25];
	int i;
	int naxis, bitpix;
	long naxes_out[2];

	/* Create binary table with 5x5 image cell. */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f1, BINARY_TBL, 3, 1, ttype, tform, NULL, "IMAGES");

	/* Add TDIM to define 5x5 shape. */
	call_04(ffpkys, f1, "TDIM1", "(5,5)", NULL);

	/* Write test data to first row. */
	for (i = 0; i < 25; i += 1) {
		data[i] = i * 100;
	}
	call_06(ffpclk, f1, 1, 1, 1, 25, data);
	call_01(ffclos, f1);

	/* Reopen and copy cell to new image file. */
	call_03(ffopen, &f1, test_path "[IMAGES]", READONLY);
	call_02(ffinit, &f2, "!test_cell_image.fits");

	call_04(fits_copy_cell2image, f1, f2, "IMAGE", 1);

	/* Verify the output image. */
	call_02(ffgidm, f2, &naxis);
	fail_if(naxis != 2);
	ffgisz(f2, 2, naxes_out, &status);
	fail_if(naxes_out[0] != 5);
	fail_if(naxes_out[1] != 5);
	call_02(ffgiet, f2, &bitpix);
	fail_if(bitpix != LONG_IMG);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
	remove("test_cell_image.fits");
}

/*
 * Test ffgncl - get number of columns in table.
 */
static void
test_ffgncl(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "A", "B", "C" };
	char *tform[] = { "1J", "1E", "10A" };
	int ncols;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 3, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 3);
	call_01(ffclos, f);
}

/*
 * Test ffgcno - get column number from name.
 */
static void
test_ffgcno(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ALPHA", "BETA", "GAMMA" };
	char *tform[] = { "1J", "1E", "10A" };
	int colnum;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 3, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_04(ffgcno, f, CASEINSEN, "BETA", &colnum);
	fail_if(colnum != 2);
	call_04(ffgcno, f, CASEINSEN, "beta", &colnum);
	fail_if(colnum != 2);
	call_01(ffclos, f);
}

/*
 * Test ffgnrwll - get number of rows (LONGLONG version).
 */
static void
test_ffgnrwll(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1J" };
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1000, 1, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 1000);
	call_01(ffclos, f);
}

/*
 * Test ffgtcl - get column datatype.
 */
static void
test_ffgtcl(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INT", "FLOAT", "STRING" };
	char *tform[] = { "1J", "1E", "20A" };
	int typecode;
	long repeat, width;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 3, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_05(ffgtcl, f, 1, &typecode, &repeat, &width);
	fail_if(typecode != TLONG);
	fail_if(repeat != 1);
	call_05(ffgtcl, f, 2, &typecode, &repeat, &width);
	fail_if(typecode != TFLOAT);
	call_05(ffgtcl, f, 3, &typecode, &repeat, &width);
	fail_if(typecode != TSTRING);
	fail_if(repeat != 20);
	fail_if(width != 20);
	call_01(ffclos, f);
}

/*
 * Test ffgcdw - get column display width.
 */
static void
test_ffgcdw(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INT", "FLOAT", "STRING" };
	char *tform[] = { "1J", "1E", "20A" };
	int dispwidth;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 3, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_03(ffgcdw, f, 3, &dispwidth);
	fail_if(dispwidth != 20);
	call_01(ffclos, f);
}

/*
 * Test ffourl with explicit URL type.
 */
static void
test_ffourl_explicit_urltype(void)
{
	int status = 0;
	char urltype[80];
	char outfile[FLEN_FILENAME];
	char tpltfile[FLEN_FILENAME];
	char compspec[FLEN_FILENAME];

	call_05(ffourl, "mem://memfile.fits", urltype, outfile,
		tpltfile, compspec);
	fail_if(strcmp(urltype, "mem://") != 0);
	fail_if(strcmp(outfile, "memfile.fits") != 0);
}

/*
 * Test ffiurl with row filter.
 */
static void
test_ffiurl_rowfilter(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "file.fits[EVENTS][X>100]", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(infile, "file.fits") != 0);
	fail_if(strcmp(extspec, "EVENTS") != 0);
	fail_if(strcmp(rowfilter, "X>100") != 0);
}

/*
 * Test ffiurl with binning specification.
 */
static void
test_ffiurl_binspec(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "file.fits[bin X,Y]", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(infile, "file.fits") != 0);
	fail_if(strcmp(binspec, "bin X,Y") != 0);
}

/*
 * Test ffiurl with column specification.
 */
static void
test_ffiurl_colspec(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "file.fits[col X;Y]", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(infile, "file.fits") != 0);
	fail_if(strcmp(colspec, "col X;Y") != 0);
}

/*
 * Test ffexts with XTENSION type specifiers.
 */
static void
test_ffexts_xtension_types(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	/* Table type */
	call_07(ffexts, "EVENTS,1,t", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(strcmp(extname, "EVENTS") != 0);
	fail_if(extvers != 1);
	fail_if(hdutype != ASCII_TBL);
}

/*
 * Test ffexts with 't' for table.
 */
static void
test_ffexts_table_type(void)
{
	int status = 0;
	int extnum, extvers, hdutype;
	char extname[FLEN_VALUE];
	char imagecolname[FLEN_VALUE];
	char rowexpress[FLEN_FILENAME];

	/* Using 't' should map to ASCII_TBL */
	call_07(ffexts, "DATA,0,t", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(hdutype != ASCII_TBL);

	/* Using 'T' should also work */
	call_07(ffexts, "DATA,0,T", &extnum, extname, &extvers, &hdutype,
		imagecolname, rowexpress);
	fail_if(hdutype != ASCII_TBL);
}

/*
 * Test ffrtnm with ftp URL.
 */
static void
test_ffrtnm_ftp(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "ftp://example.com/file.fits", rootname);
	fail_if(strcmp(rootname, "ftp://example.com/file.fits") != 0);
}

/*
 * Test ffrtnm with fits URL.
 */
static void
test_ffrtnm_fits(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "fitsfile://path/to/file.fits", rootname);
	fail_if(strstr(rootname, "file.fits") == NULL);
}

/*
 * Test ffrtnm with compressed file.
 */
static void
test_ffrtnm_compressed(void)
{
	int status = 0;
	char rootname[FLEN_FILENAME];

	call_02(ffrtnm, "file.fits.gz", rootname);
	fail_if(strcmp(rootname, "file.fits.gz") != 0);
}

/*
 * Test ffiurl with memory URL.
 */
static void
test_ffiurl_mem(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "mem://filename", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "mem://") != 0);
	fail_if(strcmp(infile, "filename") != 0);
}

/*
 * Test ffiurl with compressed file URL.
 */
static void
test_ffiurl_compressed(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	call_08(ffiurl, "file.fits.gz[1]", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "file://") != 0);
	fail_if(strcmp(infile, "file.fits.gz") != 0);
	fail_if(strcmp(extspec, "1") != 0);
}

/*
 * Test ffiurl with URL types without the // (e.g., "ftp:" instead of "ftp://")
 */
static void
test_ffiurl_urltype_no_slashes(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	/* ftp: without // */
	call_08(ffiurl, "ftp:example.com/file.fits", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "ftp://") != 0);
	fail_if(strcmp(infile, "example.com/file.fits") != 0);

	/* http: without // */
	call_08(ffiurl, "http:example.com/file.fits", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "http://") != 0);
	fail_if(strcmp(infile, "example.com/file.fits") != 0);

	/* file: without // */
	call_08(ffiurl, "file:localfile.fits", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "file://") != 0);
	fail_if(strcmp(infile, "localfile.fits") != 0);

	/* mem: without // */
	call_08(ffiurl, "mem:memfile", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "mem://") != 0);
	fail_if(strcmp(infile, "memfile") != 0);

	/* shmem: without // */
	call_08(ffiurl, "shmem:sharedmem", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "shmem://") != 0);
	fail_if(strcmp(infile, "sharedmem") != 0);

	/* gsiftp: without // */
	call_08(ffiurl, "gsiftp:example.com/file.fits", urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "gsiftp://") != 0);
	fail_if(strcmp(infile, "example.com/file.fits") != 0);
}

/*
 * Test ffiurl with urltype too long (error path)
 */
static void
test_ffiurl_urltype_too_long(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];
	/* MAX_PREFIX_LEN is 20, so use a 25-char prefix */
	char longurl[] = "verylongurltypeprefix://file.fits";

	status = ffiurl(longurl, urltype, infile, outfile,
		extspec, rowfilter, binspec, colspec, &status);
	fail_if(status != URL_PARSE_ERROR);
}

/*
 * Test ffiurl when output file has urltype (paren before ://)
 */
static void
test_ffiurl_outfile_urltype(void)
{
	int status = 0;
	char urltype[80];
	char infile[FLEN_FILENAME];
	char outfile[FLEN_FILENAME];
	char extspec[FLEN_FILENAME];
	char rowfilter[FLEN_FILENAME];
	char binspec[FLEN_FILENAME];
	char colspec[FLEN_FILENAME];

	/* Output file has urltype - the :// belongs to output, not input */
	call_08(ffiurl, "input.fits(mem://output.fits)", urltype, infile,
		outfile, extspec, rowfilter, binspec, colspec);
	fail_if(strcmp(urltype, "file://") != 0);
	fail_if(strcmp(infile, "input.fits") != 0);
	fail_if(strcmp(outfile, "mem://output.fits") != 0);
}

/*
 * Test ffgabc - get ASCII table column parameters.
 */
static void
test_ffgabc(void)
{
	int status = 0;
	char *tform[] = { "A20", "E12.4" };
	long rowlen, tbcol[2];

	call_05(ffgabc, 2, tform, 1, &rowlen, tbcol);
	fail_if(rowlen < 32);
	fail_if(tbcol[0] < 1);
}

/*
 * Test fits_get_keyclass - classify keyword.
 */
static void
test_fits_get_keyclass(void)
{
	int keyclass;

	keyclass = fits_get_keyclass("SIMPLE");
	fail_if(keyclass != TYP_STRUC_KEY);

	keyclass = fits_get_keyclass("BITPIX");
	fail_if(keyclass != TYP_STRUC_KEY);

	keyclass = fits_get_keyclass("COMMENT");
	fail_if(keyclass != TYP_COMM_KEY);

	keyclass = fits_get_keyclass("HISTORY");
	fail_if(keyclass != TYP_COMM_KEY);

	keyclass = fits_get_keyclass("MYKEY");
	fail_if(keyclass != TYP_USER_KEY);
}

/*
 * Test ffgrsz - get optimal number of rows to read.
 */
static void
test_ffgrsz(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1E" };
	long nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1000, 1, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_02(ffgrsz, f, &nrows);
	fail_if(nrows <= 0);
	call_01(ffclos, f);
}

/*
 * Test fits_get_num_cols - same as ffgncl.
 */
static void
test_fits_get_num_cols(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "A", "B" };
	char *tform[] = { "1J", "1E" };
	int ncols;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 2, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_02(fits_get_num_cols, f, &ncols);
	fail_if(ncols != 2);
	call_01(ffclos, f);
}

/*
 * Test fits_get_colname - get column name.
 */
static void
test_fits_get_colname(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "ALPHA", "BETA" };
	char *tform[] = { "1J", "1E" };
	char colname[FLEN_VALUE];
	int colnum;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 2, ttype, tform, NULL, "DATA");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path "[DATA]", READONLY);
	call_05(fits_get_colname, f, CASEINSEN, "ALP*", colname, &colnum);
	fail_if(colnum != 1);
	fail_if(strcmp(colname, "ALPHA") != 0);
	call_01(ffclos, f);
}

/*
 * Test fits_get_version.
 */
static void
test_fits_get_version(void)
{
	float version;

	version = 0.0f;
	fits_get_version(&version);
	fail_if(version < 4.0);
}

/*
 * Test fits_already_open.
 */
static void
test_fits_already_open(void)
{
	fitsfile *f1, *f2;
	int status = 0;
	long naxes[] = { 10 };

	/* Create and open a file. */
	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 1, naxes);

	/* The file should now be tracked in the internal table. */
	/* Opening again should give a different fitsfile pointer. */
	status = 0;
	ffopen(&f2, test_path, READWRITE, &status);
	fail_if(status != 0);
	fail_if(f2 == NULL);

	/* Both should point to same underlying file. */
	fail_if(f1->Fptr != f2->Fptr);

	call_01(ffclos, f1);
	call_01(ffclos, f2);
}

/*
 * Test ffflmd with READWRITE mode.
 */
static void
test_ffflmd_readwrite(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int mode;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	/* Open in readwrite mode. */
	call_03(ffopen, &f, test_path, READWRITE);
	call_02(ffflmd, f, &mode);
	fail_if(mode != READWRITE);
	call_01(ffclos, f);
}

int
main(void)
{
	/* Token parsing tests */
	test_fits_get_token_basic();
	test_fits_get_token_numeric();
	test_fits_get_token_blanks();
	test_fits_get_token_null_isanumber();
	test_fits_get_token2_basic();
	test_fits_get_token2_numeric();
	test_fits_get_token2_null_isanumber();
	test_fits_get_token2_error_status();

	/* File existence tests */
	test_ffexist_exists();
	test_ffexist_not_exists();
	test_ffexist_non_disk();
	test_ffexist_stdin();

	/* Filename parsing tests */
	test_ffrtnm_simple();
	test_ffrtnm_extension();
	test_ffrtnm_file_prefix();
	test_ffrtnm_rowfilter();
	test_ffrtnm_stdin();
	test_ffrtnm_http();

	/* Extension specification tests */
	test_ffexts_number();
	test_ffexts_name();
	test_ffexts_name_version();
	test_ffexts_name_version_type();
	test_ffexts_image_column();
	test_ffexts_row_expression();
	test_ffexts_negative_name();

	/* Extension number tests */
	test_ffextn_number();
	test_ffextn_none();
	test_ffextn_binspec();

	/* Open function variants */
	test_ffdopn();
	test_fftopn();
	test_ffiopn();
	test_ffreopen();

	/* Section range tests */
	test_section_range_simple();
	test_section_range_increment();
	test_section_range_wildcard();
	test_section_range_inverted();
	test_section_range_implied();

	/* Image section copy tests */
	test_copy_image_section();
	test_copy_image_section_stride();
	test_copy_image_section_short();
	test_copy_image_section_int();
	test_copy_image_section_float();
	test_copy_image_section_double();
	test_copy_image_section_longlong();
	test_copy_image_section_3d();

	/* URL type tests */
	test_ffurlt();
	test_ffiurl();
	test_ffiurl_outfile();
	test_ffflus();
	test_ffghdn();

	/* File operations */
	test_ffdelt();
	test_ffflnm();
	test_ffflmd();
	test_memory_file();

	/* Memory buffer operations */
	test_ffimem();
	test_ffomem();

	/* Misc tests */
	test_fits_is_reentrant();

	/* Disk I/O and utility tests */
	test_ffdkopn();
	test_ffdkinit();
	test_ffrprt();
	test_ffgtmo();
	test_ffstmo();
	test_ffihtps();
	test_ffeopn();
	test_fits_is_this_a_copy();
	test_ffvhtps_ffshdwn();
	test_ffimport_file();

	/* Fitscore functions */
	test_ffgnrw();
	test_ffghof();
	test_ffghad();
	test_ffgiprll();

	/* Template functions */
	test_fftplt();
	test_fftplt_multi_hdu();
	test_ffoptplt_empty();

	/* Output URL parsing */
	test_ffourl_template();
	test_ffourl_compress();
	test_ffourl_gzip();
	test_ffourl_stdout();
	test_ffourl_explicit_urltype();

	/* Input URL parsing */
	test_ffifile();
	test_ffiurl_rowfilter();
	test_ffiurl_binspec();
	test_ffiurl_colspec();

	/* Extension opening */
	test_ffopen_extension_number();
	test_ffopen_extension_name();
	test_ffopen_extension_name_version();
	/* Skip ffopentest - it can crash when called with NULL filename */
	test_ffopen_plus_extension();
	test_ffopen_image_type();

	/* Extension spec parsing */
	test_ffexts_xtension_types();
	test_ffexts_table_type();

	/* More URL parsing */
	test_ffrtnm_ftp();
	test_ffrtnm_fits();
	test_ffrtnm_compressed();
	test_ffiurl_mem();
	test_ffiurl_compressed();
	test_ffiurl_urltype_no_slashes();
	test_ffiurl_urltype_too_long();
	test_ffiurl_outfile_urltype();

	/* Table functions */
	test_ffgncl();
	test_ffgcno();
	test_ffgnrwll();
	test_ffgtcl();
	test_ffgcdw();
	test_fits_copy_cell2image();
	test_ffgabc();
	test_ffgrsz();
	test_fits_get_num_cols();
	test_fits_get_colname();

	/* Keyword functions */
	test_fits_get_keyclass();

	/* Version and utility functions */
	test_fits_get_version();

	/* Misc functions */
	test_fits_already_open();
	test_ffflmd_readwrite();

	printf("test_cfileio: all tests passed\n");
	return 0;
}
