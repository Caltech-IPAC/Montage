/*
 * Tests to exercise getcoll.c - logical (boolean) read functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_getcoll.fits"

static void
test_read_logical_column(void)
{
	/* Test ffgcvl - read logical values with null substitution. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1, 0, 1, 0, 1 };
	char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvl, f, 1, 1, 1, 5, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1);
	fail_if(result[3] != 0);
	fail_if(result[4] != 1);
	fail_if(anynull != 0);
	call_01(ffclos, f);
}

static void
test_read_logical_deprecated(void)
{
	/* Test ffgcl - deprecated logical read function. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1, 0, 1 };
	char result[3];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_06(ffgcl, f, 1, 1, 1, 3, result);
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1);
	call_01(ffclos, f);
}

static void
test_read_logical_with_null_flags(void)
{
	/* Test ffgcfl - read logical with null flag array. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1, 0 };
	char result[3];
	char nularray[3];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Write T, F, then leave row 3 as undefined. */
	call_06(ffpcll, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcfl, f, 1, 1, 1, 3, result, nularray, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(nularray[2] != 1);  /* Row 3 is undefined. */
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_logical_vector(void)
{
	/* Test reading logical vector column. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGVEC" };
	char *tform[] = { "4L" };
	char data[] = { 1, 0, 1, 0 };
	char result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 4, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvl, f, 1, 1, 1, 4, 0, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1);
	fail_if(result[3] != 0);
	call_01(ffclos, f);
}

static void
test_read_logical_multiple_rows(void)
{
	/* Test reading logical values across multiple rows. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "2L" };
	char data[] = { 1, 0, 0, 1, 1, 1 };
	char result[6];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 6, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvl, f, 1, 1, 1, 6, 0, result, &anynull);
	/* Row 1: T, F; Row 2: F, T; Row 3: T, T. */
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(result[2] != 0);
	fail_if(result[3] != 1);
	fail_if(result[4] != 1);
	fail_if(result[5] != 1);
	call_01(ffclos, f);
}

static void
test_read_logical_with_nulval(void)
{
	/* Test null value substitution with ffgcvl. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1 };
	char result[2];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 1, data);
	/* Row 2 is undefined. */
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Use 99 as the null substitute value. */
	call_08(ffgcvl, f, 1, 1, 1, 2, 99, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[1] != 99);  /* Null replaced with 99. */
	fail_if(anynull != 1);
	call_01(ffclos, f);
}

static void
test_read_bits_as_logical(void)
{
	/* Test ffgcx - read bits from X column as logical array. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };  /* 8 bits per row. */
	char bits[] = { 1, 0, 1, 0, 1, 0, 1, 0 };
	char result[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 8, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_06(ffgcx, f, 1, 1, 1, 8, result);
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1);
	fail_if(result[3] != 0);
	call_01(ffclos, f);
}

static void
test_read_bits_from_byte_column(void)
{
	/* Test ffgcx with B column (byte). */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };
	unsigned char data[] = { 0xAA };  /* 10101010 in binary. */
	char result[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_06(ffgcx, f, 1, 1, 1, 8, result);
	/* 0xAA = 10101010: bit 1=1, bit 2=0, bit 3=1, bit 4=0, etc. */
	fail_if(result[0] != 1);
	fail_if(result[1] != 0);
	fail_if(result[2] != 1);
	fail_if(result[3] != 0);
	fail_if(result[4] != 1);
	fail_if(result[5] != 0);
	fail_if(result[6] != 1);
	fail_if(result[7] != 0);
	call_01(ffclos, f);
}

static void
test_read_bits_spanning_rows(void)
{
	/* Test ffgcx reading bits that span multiple rows. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "4X" };  /* 4 bits per row. */
	char bits_row1[] = { 1, 1, 0, 0 };
	char bits_row2[] = { 0, 0, 1, 1 };
	char result[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 4, bits_row1);
	call_06(ffpclx, f, 1, 2, 1, 4, bits_row2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Read 4 bits from row 1, then 4 bits from row 2. */
	call_06(ffgcx, f, 1, 1, 1, 4, result);
	fail_if(result[0] != 1);
	fail_if(result[1] != 1);
	fail_if(result[2] != 0);
	fail_if(result[3] != 0);
	call_06(ffgcx, f, 1, 2, 1, 4, &result[4]);
	fail_if(result[4] != 0);
	fail_if(result[5] != 0);
	fail_if(result[6] != 1);
	fail_if(result[7] != 1);
	call_01(ffclos, f);
}

static void
test_read_bits_as_ushort(void)
{
	/* Test ffgcxui - read bits as unsigned short. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "16X" };
	char bits[16];
	unsigned short result[2];
	int i;

	/* Set up bit pattern: 0xFFFF = all 1s, 0x0000 = all 0s. */
	for (i = 0; i < 16; i += 1) {
		bits[i] = 1;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 16, bits);
	for (i = 0; i < 16; i += 1) {
		bits[i] = 0;
	}
	call_06(ffpclx, f, 1, 2, 1, 16, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_07(ffgcxui, f, 1, 1, 2, 1, 16, result);
	fail_if(result[0] != 0xFFFF);
	fail_if(result[1] != 0x0000);
	call_01(ffclos, f);
}

static void
test_read_bits_as_ushort_partial(void)
{
	/* Test ffgcxui - read partial bits as unsigned short. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "16X" };
	char bits[16] = {
		1, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 16, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Read only first 4 bits: 1010 = 10 in decimal. */
	call_07(ffgcxui, f, 1, 1, 1, 1, 4, result);
	fail_if(result[0] != 10);
	call_01(ffclos, f);
}

static void
test_read_bits_as_uint(void)
{
	/* Test ffgcxuk - read bits as unsigned int. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "32X" };
	char bits[32];
	unsigned int result[1];
	int i;

	/* Set up bit pattern: high byte = 0xFF, rest = 0. */
	for (i = 0; i < 8; i += 1) {
		bits[i] = 1;
	}
	for (i = 8; i < 32; i += 1) {
		bits[i] = 0;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 32, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_07(ffgcxuk, f, 1, 1, 1, 1, 32, result);
	/* 11111111 00000000 00000000 00000000 = 0xFF000000. */
	fail_if(result[0] != 0xFF000000);
	call_01(ffclos, f);
}

static void
test_read_bits_as_uint_partial(void)
{
	/* Test ffgcxuk - read partial bits as unsigned int. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "32X" };
	char bits[32] = {
		1, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 32, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Read only first 8 bits: 11110000 = 240 in decimal. */
	call_07(ffgcxuk, f, 1, 1, 1, 1, 8, result);
	fail_if(result[0] != 240);
	call_01(ffclos, f);
}

static void
test_read_bits_bad_row(void)
{
	/* Test ffgcx with bad row number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char result[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcx(f, 1, 0, 1, 8, result, &status);  /* Row 0 is invalid. */
	fail_if(status != BAD_ROW_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_bad_elem(void)
{
	/* Test ffgcx with bad element number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char result[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcx(f, 1, 1, 0, 8, result, &status);  /* Bit 0 is invalid. */
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_ushort_bad_row(void)
{
	/* Test ffgcxui with bad row number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "16X" };
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxui(f, 1, 0, 1, 1, 16, result, &status);  /* Row 0 is invalid. */
	fail_if(status != BAD_ROW_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_ushort_bad_bit(void)
{
	/* Test ffgcxui with bad first bit number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "16X" };
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxui(f, 1, 1, 1, 0, 16, result, &status);  /* Bit 0 is invalid. */
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_ushort_too_many(void)
{
	/* Test ffgcxui with too many bits (>16). */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "32X" };
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxui(f, 1, 1, 1, 1, 17, result, &status);  /* > 16 bits. */
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_bad_row(void)
{
	/* Test ffgcxuk with bad row number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "32X" };
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxuk(f, 1, 0, 1, 1, 32, result, &status);  /* Row 0 is invalid. */
	fail_if(status != BAD_ROW_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_bad_bit(void)
{
	/* Test ffgcxuk with bad first bit number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "32X" };
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxuk(f, 1, 1, 1, 0, 32, result, &status);  /* Bit 0 is invalid. */
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_too_many(void)
{
	/* Test ffgcxuk with too many bits (>32). */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "64X" };
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxuk(f, 1, 1, 1, 1, 33, result, &status);  /* > 32 bits. */
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_logical_not_logical_col(void)
{
	/* Test ffgcvl with non-logical column returns error. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1 };
	char result[1];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcvl(f, 1, 1, 1, 1, 0, result, &anynull, &status);
	fail_if(status != NOT_LOGICAL_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_not_bit_col(void)
{
	/* Test ffgcx with non-bit/byte column returns error. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1 };
	char result[8];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcx(f, 1, 1, 1, 8, result, &status);
	fail_if(status != NOT_LOGICAL_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_ushort_not_bit_col(void)
{
	/* Test ffgcxui with non-bit/byte column returns error. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1 };
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxui(f, 1, 1, 1, 1, 16, result, &status);
	fail_if(status != NOT_LOGICAL_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_not_bit_col(void)
{
	/* Test ffgcxuk with non-bit/byte column returns error. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long data[] = { 1 };
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxuk(f, 1, 1, 1, 1, 32, result, &status);
	fail_if(status != NOT_LOGICAL_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_ushort_bad_col(void)
{
	/* Test ffgcxui with bad column number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "16X" };
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxui(f, 99, 1, 1, 1, 16, result, &status);  /* Column 99 invalid. */
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_bad_col(void)
{
	/* Test ffgcxuk with bad column number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "32X" };
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	ffgcxuk(f, 99, 1, 1, 1, 32, result, &status);  /* Column 99 invalid. */
	fail_if(status != BAD_COL_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_ushort_not_bintable(void)
{
	/* Test ffgcxui when not in binary table. */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgcxui(f, 1, 1, 1, 1, 8, result, &status);
	fail_if(status != NOT_BTABLE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_not_bintable(void)
{
	/* Test ffgcxuk when not in binary table. */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	ffgcxuk(f, 1, 1, 1, 1, 8, result, &status);
	fail_if(status != NOT_BTABLE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_overflow_x_col(void)
{
	/* Test ffgcxui reading past end of X column. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };  /* Only 8 bits. */
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Try to read 16 bits from an 8-bit column. */
	ffgcxui(f, 1, 1, 1, 1, 16, result, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_overflow_b_col(void)
{
	/* Test ffgcxui reading past end of B column. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };  /* Only 8 bits (1 byte). */
	unsigned short result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Try to read 16 bits starting at bit 1 from a 1-byte column. */
	ffgcxui(f, 1, 1, 1, 1, 16, result, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_illegal_logical_char(void)
{
	/*
	* Test reading a logical column that contains illegal characters.
	* Write raw bytes to a logical column to simulate corruption.
	* This exercises the "other illegal character" path in ffgcll.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "4L" };
	char illegal_data[] = { 'T', 'X', 'F', 1 };  /* 'X' is illegal, 1 is special case */
	char result[4];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Write raw bytes directly to bypass validation */
	call_05(ffptbb, f, 1, 1, 4, (unsigned char *)illegal_data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 4, NULL, result, &anynull);
	/* 'T' should become 1, 'X' should return as char value, 'F' should become 0 */
	/* The value 1 is a special case - returns 49 (ASCII '1') */
	fail_if(result[0] != 1);  /* 'T' */
	fail_if(result[1] != 'X');  /* illegal char returned as-is */
	fail_if(result[2] != 0);  /* 'F' */
	fail_if(result[3] != 49);  /* byte value 1 becomes ASCII '1' (49) */
	call_01(ffclos, f);
}

static void
test_read_bits_varlen_overflow(void)
{
	/*
	* Test BAD_ELEM_NUM error for variable length bit column.
	* Try to read more bits than exist in a variable length descriptor.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARBITS" };
	char *tform[] = { "1PX" };
	char bits[] = { 1, 0, 1, 0 };
	char result[10];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 4, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Try to read 100 bits but we only wrote 4 */
	ffgcx(f, 1, 1, 1, 100, result, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_overflow_x_col(void)
{
	/* Test ffgcxuk reading past end of X column (TBIT). */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };  /* Only 8 bits. */
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Try to read 32 bits from an 8-bit column. */
	ffgcxuk(f, 1, 1, 1, 1, 32, result, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_read_bits_uint_overflow_b_col(void)
{
	/* Test ffgcxuk reading past end of B column (TBYTE). */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };  /* Only 8 bits (1 byte). */
	unsigned int result[1];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Try to read 32 bits starting at bit 1 from a 1-byte column. */
	ffgcxuk(f, 1, 1, 1, 1, 32, result, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

int
main(void)
{
	test_read_logical_column();
	test_read_logical_deprecated();
	test_read_logical_with_null_flags();
	test_read_logical_vector();
	test_read_logical_multiple_rows();
	test_read_logical_with_nulval();
	test_read_bits_as_logical();
	test_read_bits_from_byte_column();
	test_read_bits_spanning_rows();
	test_read_bits_as_ushort();
	test_read_bits_as_ushort_partial();
	test_read_bits_as_uint();
	test_read_bits_as_uint_partial();
	test_read_bits_bad_row();
	test_read_bits_bad_elem();
	test_read_bits_ushort_bad_row();
	test_read_bits_ushort_bad_bit();
	test_read_bits_ushort_too_many();
	test_read_bits_uint_bad_row();
	test_read_bits_uint_bad_bit();
	test_read_bits_uint_too_many();
	test_read_logical_not_logical_col();
	test_read_bits_not_bit_col();
	test_read_bits_ushort_not_bit_col();
	test_read_bits_uint_not_bit_col();
	test_read_bits_ushort_bad_col();
	test_read_bits_uint_bad_col();
	test_read_bits_ushort_not_bintable();
	test_read_bits_uint_not_bintable();
	test_read_bits_overflow_x_col();
	test_read_bits_overflow_b_col();
	test_read_illegal_logical_char();
	test_read_bits_varlen_overflow();
	test_read_bits_uint_overflow_x_col();
	test_read_bits_uint_overflow_b_col();
	return 0;
}
