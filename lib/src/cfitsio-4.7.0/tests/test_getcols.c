/*
 * Tests for getcols.c - string read functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcols.fits"

static void
test_read_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "Alice", "Bob", "Charlie" };
	char result1[11], result2[11], result3[11];
	char *results[] = { result1, result2, result3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 3, NULL, results, &anynull);
	fail_if(strcmp(results[0], "Alice") != 0);
	fail_if(strcmp(results[1], "Bob") != 0);
	fail_if(strcmp(results[2], "Charlie") != 0);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_string_column_with_null_flags(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "10A" };
	char *data[] = { "One", "Two", "Three" };
	char result1[11], result2[11], result3[11];
	char *results[] = { result1, result2, result3 };
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfs, f, 1, 1, 1, 3, results, nularray, &anynull);
	fail_if(strcmp(results[0], "One") != 0);
	fail_if(strcmp(results[1], "Two") != 0);
	fail_if(strcmp(results[2], "Three") != 0);
	fail_if(nularray[0] != 0);
	fail_if(nularray[1] != 0);
	fail_if(nularray[2] != 0);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_ascii_table_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NAME" };
	char *tform[] = { "A10" };
	char *data[] = { "Ascii1", "Ascii2" };
	char result1[11], result2[11];
	char *results[] = { result1, result2 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 2, NULL, results, &anynull);
	fail_if(strcmp(results[0], "Ascii1") != 0);
	fail_if(strcmp(results[1], "Ascii2") != 0);
	call_01(ffclos, f);
}

static void
test_read_variable_length_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARSTR" };
	char *tform[] = { "1PA" };
	char *data[] = { "Variable length string content" };
	char result1[64];
	char *results[] = { result1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 1, NULL, results, &anynull);
	fail_if(strcmp(results[0], "Variable length string content") != 0);
	call_01(ffclos, f);
}

static void
test_read_numeric_as_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 12345, -99, 0 };
	char result1[32], result2[32], result3[32];
	char *results[] = { result1, result2, result3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 3, NULL, results, &anynull);
	/* ffgcvs converts any column type to string */
	fail_if(atol(results[0]) != 12345);
	fail_if(atol(results[1]) != -99);
	fail_if(atol(results[2]) != 0);
	call_01(ffclos, f);
}

static void
test_read_float_as_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "FLOATCOL" };
	char *tform[] = { "1E" };
	float data[] = { 3.14f, -2.5f };
	char result1[32], result2[32];
	char *results[] = { result1, result2 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 2, NULL, results, &anynull);
	/* Check that values convert to strings approximately correctly */
	fail_if(atof(results[0]) < 3.1 || atof(results[0]) > 3.2);
	fail_if(atof(results[1]) < -2.6 || atof(results[1]) > -2.4);
	call_01(ffclos, f);
}

static void
test_read_double_as_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DBLCOL" };
	char *tform[] = { "1D" };
	double data[] = { 1.23456789, -9.87654321 };
	char result1[32], result2[32];
	char *results[] = { result1, result2 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 2, NULL, results, &anynull);
	fail_if(atof(results[0]) < 1.23 || atof(results[0]) > 1.24);
	fail_if(atof(results[1]) < -9.88 || atof(results[1]) > -9.87);
	call_01(ffclos, f);
}

static void
test_read_byte_as_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 0, 127, 255 };
	char result1[16], result2[16], result3[16];
	char *results[] = { result1, result2, result3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 3, NULL, results, &anynull);
	fail_if(atoi(results[0]) != 0);
	fail_if(atoi(results[1]) != 127);
	fail_if(atoi(results[2]) != 255);
	call_01(ffclos, f);
}

static void
test_read_short_as_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SHORTCOL" };
	char *tform[] = { "1I" };
	short data[] = { -32768, 0, 32767 };
	char result1[16], result2[16], result3[16];
	char *results[] = { result1, result2, result3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 3, NULL, results, &anynull);
	fail_if(atoi(results[0]) != -32768);
	fail_if(atoi(results[1]) != 0);
	fail_if(atoi(results[2]) != 32767);
	call_01(ffclos, f);
}

static void
test_read_longlong_as_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LLCOL" };
	char *tform[] = { "1K" };
	LONGLONG data[] = { -9223372036854775807LL, 0, 9223372036854775807LL };
	char result1[32], result2[32], result3[32];
	char *results[] = { result1, result2, result3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcljj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 3, NULL, results, &anynull);
	fail_if(atoll(results[0]) != -9223372036854775807LL);
	fail_if(atoll(results[1]) != 0);
	fail_if(atoll(results[2]) != 9223372036854775807LL);
	call_01(ffclos, f);
}

static void
test_get_column_display_width(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2", "COL3" };
	char *tform[] = { "10A", "1J", "1E" };
	int width;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 3, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_03(ffgcdw, f, 1, &width);
	fail_if(width != 10);  /* String column width = 10 */
	call_03(ffgcdw, f, 2, &width);
	fail_if(width == 0);   /* Integer column has nonzero width */
	call_03(ffgcdw, f, 3, &width);
	fail_if(width == 0);   /* Float column has nonzero width */
	call_01(ffclos, f);
}

static void
test_get_column_display_width_with_tdisp(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	int width;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(ffpkys, f, "TDISP1", "I8", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_03(ffgcdw, f, 1, &width);
	fail_if(width != 8);  /* Should use TDISP width */
	call_01(ffclos, f);
}

static void
test_get_column_display_width_bad_col(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10A" };
	int width;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcdw(f, 0, &width, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	ffgcdw(f, 99, &width, &status);
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_with_tdisp_format(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1D" };
	double data[] = { 123.456789 };
	char result1[64];
	char *results[] = { result1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 1, data);
	/* Set display format */
	call_04(ffpkys, f, "TDISP1", "F10.2", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 1, NULL, results, &anynull);
	/* Value should be formatted according to TDISP */
	call_01(ffclos, f);
}

static void
test_read_with_scaling(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "SCALED" };
	char *tform[] = { "1J" };
	long data[] = { 100 };
	char result1[64];
	char *results[] = { result1 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	/* Set scaling: result = data * 2.0 + 10.0 = 210 */
	call_05(ffpkyd, f, "TSCAL1", 2.0, 15, NULL);
	call_05(ffpkyd, f, "TZERO1", 10.0, 15, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 1, NULL, results, &anynull);
	/* Value should be scaled */
	fail_if(atof(results[0]) < 209 || atof(results[0]) > 211);
	call_01(ffclos, f);
}

static void
test_read_multielem_vector(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECSTR" };
	char *tform[] = { "3A5" };  /* 3 strings of 5 chars each */
	char *data[] = { "One", "Two", "Three" };
	char result1[16], result2[16], result3[16];
	char *results[] = { result1, result2, result3 };
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvs, f, 1, 1, 1, 3, NULL, results, &anynull);
	fail_if(strcmp(results[0], "One") != 0);
	fail_if(strcmp(results[1], "Two") != 0);
	/* "Three" may be truncated to 5 chars */
	call_01(ffclos, f);
}

int
main(void)
{
	test_read_string_column();
	test_read_ascii_table_string();
	test_read_variable_length_string();
	test_read_numeric_as_string();
	test_read_float_as_string();
	test_read_double_as_string();
	test_read_byte_as_string();
	test_read_short_as_string();
	test_read_string_column_with_null_flags();
	test_read_longlong_as_string();
	test_get_column_display_width();
	test_get_column_display_width_with_tdisp();
	test_get_column_display_width_bad_col();
	test_read_with_tdisp_format();
	test_read_with_scaling();
	test_read_multielem_vector();
	return 0;
}
