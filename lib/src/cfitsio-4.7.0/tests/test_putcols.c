/*
 * Tests for putcols.c - string write functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcols.fits"

static void
test_write_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "Alice", "Bob", "Charlie" };
	char result[11];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "Alice") != 0);
	call_01(ffclos, f);
}

static void
test_write_ascii_table_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "A10" };
	char *data[] = { "Test1", "Test2" };
	char result[11];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "Test1") != 0);
	call_01(ffclos, f);
}

static void
test_write_variable_length_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARSTR" };
	char *tform[] = { "1PA" };
	char *data[] = { "Variable length string" };
	char result[64];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "Variable length string") != 0);
	call_01(ffclos, f);
}

static void
test_write_bad_col_num(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL" };
	char *tform[] = { "10A" };
	char *data[] = { "test" };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcls(f, 0, 1, 1, 1, data, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_not_ascii_col(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	char *data[] = { "test" };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcls(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NOT_ASCII_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_strings_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "Good1", "NULL", "Good2", "NULL" };
	char *nulval = "NULL";
	char result[11];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_07(ffpcns, f, 1, 1, 1, 4, data, nulval);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "Good1") != 0);
	call_01(ffclos, f);
}

static void
test_write_nulls_at_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "Good1", "Good2", "NULL", "NULL" };
	char *nulval = "NULL";
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_07(ffpcns, f, 1, 1, 1, 4, data, nulval);
	call_01(ffclos, f);
}

static void
test_write_all_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "NULL", "NULL", "NULL" };
	char *nulval = "NULL";
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_07(ffpcns, f, 1, 1, 1, 3, data, nulval);
	call_01(ffclos, f);
}

static void
test_write_string_vector(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAMES" };
	char *tform[] = { "30A" };
	char *data[] = { "First", "Second", "Third" };
	char result[11];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "First") != 0);
	call_01(ffclos, f);
}

static void
test_write_multirow_strings(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "20A" };
	char *data[] = { "Row1Col1", "Row1Col2", "Row2Col1", "Row2Col2" };
	char result[21];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 4, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "Row1Col1") != 0);
	call_01(ffclos, f);
}

static void
test_write_empty_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "", "NotEmpty", "" };
	char result[11];
	char *rptr = result;
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 2, 1, 1, NULL, &rptr, &anynull);
	fail_if(strcmp(result, "NotEmpty") != 0);
	call_01(ffclos, f);
}

/* Test string columns wider than IOBUFLEN (2880) */
static void
test_write_very_wide_string(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "WIDE" };
	char *tform[] = { "3000A" };  /* > 2880 */
	char data0[3001];
	char data1[3001];
	char *data[] = { data0, data1 };
	char result[3001];
	char *rptr = result;
	int anynull;
	int i;

	/* Fill with pattern */
	for (i = 0; i < 3000; i += 1) {
		data0[i] = 'A' + (i % 26);
		data1[i] = 'a' + (i % 26);
	}
	data0[3000] = '\0';
	data1[3000] = '\0';

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TSTRING, 1, 1, 1, 1, NULL, &rptr, &anynull);
	/* Just verify first few chars */
	fail_if(result[0] != 'A');
	fail_if(result[1] != 'B');
	call_01(ffclos, f);
}

/* Test that functions return early when status > 0 */
static void
test_calls_with_error_status(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "test" };
	char *nulval = "";

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);

	/* Call ffpcls with pre-existing error */
	status = 1;
	ffpcls(f, 1, 1, 1, 1, data, &status);
	fail_if(status != 1);

	/* Call ffpcns with pre-existing error */
	status = 1;
	ffpcns(f, 1, 1, 1, 1, data, nulval, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffgcprll error paths in ffpcls. Tests variable length string
 * and fixed string ffgcprll error paths
 */
static void
test_ffgcprll_errors(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARSTR", "FIXSTR" };
	char *tform[] = { "1PA", "10A" };
	char *data[] = { "test" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 2, ttype, tform, NULL, NULL);

	/* Bad row number for variable length column */
	ffpcls(f, 1, 0, 1, 1, data, &status);
	fail_if(status != BAD_ROW_NUM);
	status = 0;

	/* Bad row number for fixed length column */
	ffpcls(f, 2, 0, 1, 1, data, &status);
	fail_if(status != BAD_ROW_NUM);
	status = 0;

	call_01(ffclos, f);
}

/* Test error paths within ffpcns when ffpclu/ffpcls fail */
static void
test_ffpcns_internal_errors(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data1[] = { "NULL", "good" };
	char *data2[] = { "good", "NULL" };
	char *nulval = "NULL";

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);

	/* "NULL" then "good" - should fail when writing nulls */
	ffpcns(f, 1, 0, 1, 2, data1, nulval, &status);
	fail_if(status == 0);
	status = 0;

	/* "good" then "NULL" - should fail when writing good values */
	ffpcns(f, 1, 0, 1, 2, data2, nulval, &status);
	fail_if(status == 0);
	status = 0;

	call_01(ffclos, f);
}

/*
 * Test HDU position mismatch recovery in ffpcls and ffpcns. Create a
 * file with two tables, then manipulate HDUposition to trigger the
 * mismatch correction code
 */
static void
test_hdu_position_mismatch(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "test" };
	char *nulval = "X";

	/* Create file with primary + 2 tables */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, "TABLE1");
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, "TABLE2");
	call_01(ffclos, f);

	/* Reopen and move to HDU 3 (TABLE2) */
	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 3, NULL);

	/* Set HDUposition to 1 (TABLE1) to trigger mismatch */
	/* curhdu=2 (0-indexed, TABLE2), HDUposition=1 (TABLE1) */
	f->HDUposition = 1;

	/* ffpcls should call ffmahd to move to HDU 2 (TABLE1) */
	call_06(ffpcls, f, 1, 1, 1, 1, data);

	/* Now move back to HDU 3 and set mismatch for ffpcns */
	call_03(ffmahd, f, 3, NULL);
	f->HDUposition = 1;
	call_07(ffpcns, f, 1, 1, 1, 1, data, nulval);

	call_01(ffclos, f);
}

int
main(void)
{
	test_write_string_column();
	test_write_ascii_table_string();
	test_write_variable_length_string();
	test_write_bad_col_num();
	test_write_not_ascii_col();
	test_write_strings_with_nulls();
	test_write_nulls_at_end();
	test_write_all_nulls();
	test_write_string_vector();
	test_write_multirow_strings();
	test_write_empty_string();
	test_write_very_wide_string();
	test_calls_with_error_status();
	test_ffgcprll_errors();
	test_ffpcns_internal_errors();
	test_hdu_position_mismatch();
	remove(test_path);
	return 0;
}
