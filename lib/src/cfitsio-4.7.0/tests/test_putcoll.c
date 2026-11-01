/*
 * Tests for putcoll.c - logical write functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putcoll.fits"

static void
test_write_logical_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1, 0, 1, 1, 0 };
	char result[5];
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 1);
	call_01(ffclos, f);
}

static void
test_write_not_logical_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	char data[] = { 1 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpcll(f, 1, 1, 1, 1, data, &status);
	fail_if(status != NOT_LOGICAL_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_logical_with_nulls(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1, 99, 0, 99, 1 };
	char nulval = 99;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_07(ffpcnl, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);
}

static void
test_write_nulls_at_end(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1, 0, 99, 99 };
	char nulval = 99;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 1, ttype, tform, NULL, NULL);
	call_07(ffpcnl, f, 1, 1, 1, 4, data, nulval);
	call_01(ffclos, f);
}

static void
test_write_bit_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
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
	call_01(ffclos, f);
}

static void
test_write_bit_bad_row(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char bits[] = { 1 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclx(f, 1, 0, 1, 1, bits, &status);
	fail_if(status != BAD_ROW_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_bit_bad_elem(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char bits[] = { 1 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclx(f, 1, 1, 0, 1, bits, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_bit_not_logical(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	char bits[] = { 1 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclx(f, 1, 1, 1, 1, bits, &status);
	fail_if(status != NOT_LOGICAL_COL);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_bit_beyond_repeat(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char bits[] = { 1 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpclx(f, 1, 1, 9, 1, bits, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_varlen_bit(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARBIT" };
	char *tform[] = { "1PX" };
	char bits[] = { 1, 0, 1, 1, 0, 1 };
	char result[6];
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclx, f, 1, 1, 1, 6, bits);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_06(ffgcx, f, 1, 1, 1, 6, result);
	fail_if(result[0] != 1);
	call_01(ffclos, f);
}

static void
test_logical_spanning_rows(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "3L" };
	char data[] = { 1, 0, 1, 0, 1, 0, 1, 0, 1 };
	char result[9];
	int anynull;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcll, f, 1, 1, 1, 9, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 9, NULL, result, &anynull);
	for (int i = 0; i < 9; i += 1) {
		fail_if(result[i] != data[i]);
	}
	call_01(ffclos, f);
}

static void
test_varlen_logical_with_nulls(void)
{
	/*
	* Test writing variable length logical array with nulls.
	* This exercises the tcode < 0 path in ffpcnl.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARCOL" };
	char *tform[] = { "1PL" };
	char data[] = { 1, 0, 1, 0, 1 };
	char nulval = 0;
	char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_07(ffpcnl, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 5, NULL, result, &anynull);
	fail_if(result[0] != 1);
	fail_if(result[2] != 1);
	fail_if(result[4] != 1);
	call_01(ffclos, f);
}

static void
test_logical_nulls_mixed(void)
{
	/*
	* Test writing logical values with mixed null/non-null values.
	* Exercise the nbad path with nulls at different positions.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "5L" };
	char data[] = { 1, 99, 99, 1, 99 };
	char nulval = 99;
	char result[5];
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_07(ffpcnl, f, 1, 1, 1, 5, data, nulval);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TLOGICAL, 1, 1, 1, 5, NULL, result, &anynull);
	/* Non-null values should be 1 */
	fail_if(result[0] != 1);
	fail_if(result[3] != 1);
	call_01(ffclos, f);
}

static void
test_bit_bad_elem_beyond(void)
{
	/*
	* Test BAD_ELEM_NUM error when fbit exceeds repeat count.
	* This exercises the fbyte > repeat check in ffpclx.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char bits[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Try to write at bit position 100 which exceeds 8-bit column */
	ffpclx(f, 1, 1, 100, 1, bits, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_bit_byte_col_overflow(void)
{
	/*
	* Test BAD_ELEM_NUM when writing bits beyond byte column width.
	* Uses single byte column to trigger the fbyte > repeat path.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BYTECOL" };
	char *tform[] = { "1B" };  /* 1 byte = 8 bits */
	char bits[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* Try to write at bit 9 which exceeds 8 bits (1 byte) */
	ffpclx(f, 1, 1, 9, 1, bits, &status);
	fail_if(status != BAD_ELEM_NUM);
	status = 0;
	call_01(ffclos, f);
}

static void
test_write_bits_spanning_rows(void)
{
	/*
	* Test writing bits that span multiple rows.
	* Exercises the row boundary handling in ffpclx.
	*/
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "4X" };  /* 4 bits per row */
	char bits[12] = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
	char result[12];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Write 12 bits starting at row 1, which spans all 3 rows */
	call_06(ffpclx, f, 1, 1, 1, 12, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_06(ffgcx, f, 1, 1, 1, 4, result);
	fail_if(result[0] != 1);
	call_06(ffgcx, f, 1, 2, 1, 4, &result[4]);
	fail_if(result[4] != 1);
	call_01(ffclos, f);
}

/*
 * Test writing bits to a row beyond current table size. When writing
 * past EOF, ffpclu reads cbuff which triggers the END_OF_FILE case and
 * sets status=0, cbuff=0
 */
static void
test_write_bits_to_new_row(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char bits[] = { 1, 0, 1, 0 };
	unsigned char result;
	int anynull;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	/* Create table with 0 rows initially */
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	/* Write bits to row 1 which extends the table */
	call_06(ffpclx, f, 1, 1, 1, 4, bits);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_09(ffgcv, f, TBYTE, 1, 1, 1, 1, NULL, &result, &anynull);
	/* Bits 1,0,1,0 starting at bit 1 = binary 10100000 = 0xA0 = 160 */
	fail_if(result != 0xA0);
	call_01(ffclos, f);
}

/* Test ffpclx with nbit=0 for early return */
static void
test_write_zero_bits(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "BITCOL" };
	char *tform[] = { "8X" };
	char bits[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* nbit=0 should return early without error */
	ffpclx(f, 1, 1, 1, 0, bits, &status);
	fail_if(status != 0);
	call_01(ffclos, f);
}

/* Test that functions return early when status > 0 */
static void
test_calls_with_error_status(void)
{

	fitsfile *f;
	int status = 0;
	char *ttype[] = { "LOGCOL" };
	char *tform[] = { "1L" };
	char data[] = { 1 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);

	/* Call ffpcll with pre-existing error */
	status = 1;
	ffpcll(f, 1, 1, 1, 1, data, &status);
	fail_if(status != 1);

	/* Call ffpcnl with pre-existing error */
	status = 1;
	ffpcnl(f, 1, 1, 1, 1, data, 0, &status);
	fail_if(status != 1);

	/* Call ffpclx with pre-existing error */
	status = 1;
	ffpclx(f, 1, 1, 1, 1, data, &status);
	fail_if(status != 1);

	status = 0;
	call_01(ffclos, f);
}

int
main(void)
{
	test_write_logical_column();
	test_write_not_logical_column();
	test_write_logical_with_nulls();
	test_write_nulls_at_end();
	test_write_bit_column();
	test_write_bit_bad_row();
	test_write_bit_bad_elem();
	test_write_bit_not_logical();
	test_write_bit_beyond_repeat();
	test_write_varlen_bit();
	test_logical_spanning_rows();
	test_varlen_logical_with_nulls();
	test_logical_nulls_mixed();
	test_bit_bad_elem_beyond();
	test_write_bit_byte_col_overflow();
	test_write_bits_spanning_rows();
	test_write_bits_to_new_row();
	test_write_zero_bits();
	test_calls_with_error_status();
	remove(test_path);
	return 0;
}
