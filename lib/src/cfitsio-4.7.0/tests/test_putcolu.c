/*
 * Tests for putcolu.c - null value write functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "fitsio2.h"
#include "test_macros.h"

#define test_path "test_putcolu.fits"

static void
test_write_null_primary_float(void)
{
	/* Float/double images use NaN for null, no TNULL needed */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, FLOAT_IMG, 1, naxes);
	call_05(ffppr, f, TFLOAT, 1, 10, data);
	call_04(ffppru, f, 1, 1, 5);
	call_01(ffclos, f);
}

static void
test_write_null_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10B" };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 255);
	call_06(ffpclb, f, 1, 1, 1, 10, data);
	call_05(ffpclu, f, 1, 1, 1, 3);
	call_01(ffclos, f);
}

static void
test_write_null_short_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "5I" };
	short data[] = { 100, 200, 300, 400, 500 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -32768);
	call_06(ffpcli, f, 1, 1, 1, 5, data);
	call_05(ffpclu, f, 1, 1, 1, 2);
	call_01(ffclos, f);
}

static void
test_write_null_long_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "3J" };
	long data[] = { 1000, 2000, 3000 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -2147483647L);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_05(ffpclu, f, 1, 1, 1, 1);
	call_01(ffclos, f);
}

static void
test_write_null_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "2K" };
	LONGLONG data[] = { 1000000000LL, 2000000000LL };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -9223372036854775807LL);
	call_06(ffpcljj, f, 1, 1, 1, 2, data);
	call_05(ffpclu, f, 1, 1, 1, 1);
	call_01(ffclos, f);
}

static void
test_write_null_float_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "4E" };
	float data[] = { 1.1f, 2.2f, 3.3f, 4.4f };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 4, data);
	call_05(ffpclu, f, 1, 1, 1, 2);
	call_01(ffclos, f);
}

static void
test_write_null_double_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "3D" };
	double data[] = { 1.11, 2.22, 3.33 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, data);
	call_05(ffpclu, f, 1, 1, 1, 1);
	call_01(ffclos, f);
}

static void
test_write_null_logical_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "5L" };
	char data[] = { 'T', 'F', 'T', 'F', 'T' };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 5, data);
	call_05(ffpclu, f, 1, 1, 2, 2);
	call_01(ffclos, f);
}

static void
test_write_null_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10A" };
	char *data[] = { "hello", "world" };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_04(ffpkys, f, "TNULL1", "*", NULL);
	call_06(ffpcls, f, 1, 1, 1, 2, data);
	call_05(ffpclu, f, 1, 1, 1, 1);
	call_01(ffclos, f);
}

static void
test_write_null_rows(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2" };
	char *tform[] = { "5E", "3D" };
	float fdata[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	double ddata[] = { 1.1, 2.2, 3.3 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 2, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, fdata);
	call_06(ffpcle, f, 1, 2, 1, 5, fdata);
	call_06(ffpcle, f, 1, 3, 1, 5, fdata);
	call_06(ffpcld, f, 2, 1, 1, 3, ddata);
	call_06(ffpcld, f, 2, 2, 1, 3, ddata);
	call_06(ffpcld, f, 2, 3, 1, 3, ddata);
	call_03(ffprwu, f, 2, 1);
	call_01(ffclos, f);
}

static void
test_write_null_complex_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "3C" };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 6, data);
	call_05(ffpclu, f, 1, 1, 1, 2);
	call_01(ffclos, f);
}

static void
test_pclu_bad_status(void)
{
	fitsfile *f;
	int status = 1;
	ffpclu(f, 1, 1, 1, 1, &status);
	fail_if(status != 1);
}

static void
test_prwu_bad_row(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "5E" };
	float data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, data);
	call_06(ffpcle, f, 1, 2, 1, 5, data);
	status = 0;
	ffprwu(f, 0, 1, &status);
	fail_if(status != BAD_ROW_NUM);
	status = 0;
	ffprwu(f, 1, 10, &status);
	fail_if(status != BAD_ROW_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_pclu_no_null_defined(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10B" };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 10, data);
	status = 0;
	ffpclu(f, 1, 1, 1, 3, &status);
	fail_if(status != NO_NULL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ascii_table_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "I10" };
	char *tunit[] = { "" };
	long data[] = { 100, 200, 300 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 3, 1, ttype, tform, tunit, NULL);
	call_04(ffpkys, f, "TNULL1", "-999", NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_05(ffpclu, f, 1, 2, 1, 1);
	call_01(ffclos, f);
}

static void
test_write_null_primary_double(void)
{
	/* Double image uses NaN for null */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, DOUBLE_IMG, 1, naxes);
	call_05(ffppr, f, TDOUBLE, 1, 10, data);
	call_03(ffpprn, f, 1, 5);
	call_01(ffclos, f);
}

static void
test_pcluc_byte(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10B" };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, 255);
	call_06(ffpclb, f, 1, 1, 1, 10, data);
	call_05(ffpcluc, f, 1, 1, 1, 3);
	call_01(ffclos, f);
}

static void
test_pcluc_longlong(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "5K" };
	LONGLONG data[] = { 100, 200, 300, 400, 500 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -9223372036854775807LL);
	call_06(ffpcljj, f, 1, 1, 1, 5, data);
	call_05(ffpcluc, f, 1, 1, 1, 2);
	call_01(ffclos, f);
}

static void
test_pcluc_logical(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10L" };
	char data[] = { 'T', 'F', 'T', 'F', 'T', 'T', 'F', 'T', 'F', 'T' };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 10, data);
	call_05(ffpcluc, f, 1, 1, 3, 4);
	call_01(ffclos, f);
}

static void
test_pcluc_string(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "10A" };
	char *data[] = { "hello", "world", "test" };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_04(ffpkys, f, "TNULL1", "*", NULL);
	call_06(ffpcls, f, 1, 1, 1, 3, data);
	call_05(ffpcluc, f, 1, 2, 1, 1);
	call_01(ffclos, f);
}

static void
test_pcluc_float(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "5E" };
	float data[] = { 1.1f, 2.2f, 3.3f, 4.4f, 5.5f };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, data);
	call_05(ffpcluc, f, 1, 1, 1, 2);
	call_01(ffclos, f);
}

static void
test_pcluc_double(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "4D" };
	double data[] = { 1.11, 2.22, 3.33, 4.44 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 4, data);
	call_05(ffpcluc, f, 1, 1, 1, 2);
	call_01(ffclos, f);
}

static void
test_pcluc_bad_status(void)
{
	fitsfile *f;
	int status = 1;
	ffpcluc(f, 1, 1, 1, 1, &status);
	fail_if(status != 1);
}

/* Test writing nulls that span multiple rows */
static void
test_pcluc_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "5E" };
	float data[] = { 1.1f, 2.2f, 3.3f, 4.4f, 5.5f };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, data);
	call_06(ffpcle, f, 1, 2, 1, 5, data);
	call_06(ffpcle, f, 1, 3, 1, 5, data);
	/* Write nulls across rows - start at elem 4 of row 1, continue to row 2 */
	call_05(ffpcluc, f, 1, 1, 4, 7);
	call_01(ffclos, f);
}

/* Test writing nulls that span multiple rows with ffpclu */
static void
test_pclu_multirow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "4D" };
	double data[] = { 1.1, 2.2, 3.3, 4.4 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 4, data);
	call_06(ffpcld, f, 1, 2, 1, 4, data);
	call_06(ffpcld, f, 1, 3, 1, 4, data);
	/* Write nulls across rows - start at elem 3 of row 1, continue to row 3 */
	call_05(ffpclu, f, 1, 1, 3, 8);
	call_01(ffclos, f);
}

/*
 * Test NO_NULL error for ASCII table string column without null defined.
 * This exercises the TSTRING path in ffpclu.
 */
static void
test_pclu_ascii_string_no_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STRCOL" };
	char *tform[] = { "A8" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Try to write nulls without defining null string - should fail */
	ffpclu(f, 1, 1, 1, 1, &status);
	fail_if(status != NO_NULL);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test NO_NULL error for ASCII table string column without null defined.
 * This exercises the TSTRING path in ffpcluc.
 */
static void
test_pcluc_ascii_string_no_null(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "STRCOL" };
	char *tform[] = { "A8" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Try to write nulls without defining null string - should fail */
	ffpcluc(f, 1, 1, 1, 1, &status);
	fail_if(status != NO_NULL);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test NO_NULL error for integer column without null defined in ffpcluc.
 * This exercises the integer NO_NULL path in ffpcluc.
 */
static void
test_pcluc_no_null_int(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1, 2, 3 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	/* Try to write nulls without defining TNULL - should fail */
	ffpcluc(f, 1, 1, 1, 1, &status);
	fail_if(status != NO_NULL);
	status = 0;
	call_01(ffclos, f);
}

int
main(void)
{
	test_write_null_primary_float();
	test_write_null_primary_double();
	test_write_null_byte_column();
	test_write_null_short_column();
	test_write_null_long_column();
	test_write_null_longlong_column();
	test_write_null_float_column();
	test_write_null_double_column();
	test_write_null_logical_column();
	test_write_null_string_column();
	test_write_null_rows();
	test_write_null_complex_column();
	test_pclu_bad_status();
	test_prwu_bad_row();
	test_pclu_no_null_defined();
	test_ascii_table_null();
	test_pcluc_byte();
	test_pcluc_longlong();
	test_pcluc_logical();
	test_pcluc_string();
	test_pcluc_float();
	test_pcluc_double();
	test_pcluc_bad_status();
	test_pcluc_multirow();
	test_pclu_multirow();
	test_pclu_ascii_string_no_null();
	test_pcluc_ascii_string_no_null();
	test_pcluc_no_null_int();
	remove(test_path);
	return 0;
}
