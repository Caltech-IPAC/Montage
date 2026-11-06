/*
 * Tests to exercise editcol.c - table row/column editing functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_editcol.fits"
#define test_path2 "test_editcol2.fits"

static void
test_resize_image(void)
{
	/* Test ffrsim - resize an existing image. */
	fitsfile *f;
	int status = 0;
	long naxes_orig[] = { 10, 10 };
	long naxes_new[] = { 20, 20 };
	short data[400];
	short result[400];
	int bitpix, naxis;
	long naxes[2];
	int i;

	for (i = 0; i < 100; i += 1) {
		data[i] = (short)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes_orig);
	call_05(ffppri, f, 1, 1, 100, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	/* Resize from 10x10 to 20x20. */
	call_04(ffrsim, f, SHORT_IMG, 2, naxes_new);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffghpr, f, 2, NULL, &bitpix, &naxis, naxes, NULL, NULL, NULL);
	fail_if(naxis != 2);
	fail_if(naxes[0] != 20);
	fail_if(naxes[1] != 20);
	/* Read back data - original data should still be at the start. */
	call_07(ffgpvi, f, 1, 1, 100, 0, result, NULL);
	fail_if(result[0] != 0);
	fail_if(result[50] != 50);
	call_01(ffclos, f);
}

static void
test_resize_image_smaller(void)
{
	/* Test ffrsim - shrink an existing image. */
	fitsfile *f;
	int status = 0;
	long naxes_orig[] = { 20, 20 };
	long naxes_new[] = { 5, 5 };
	short data[400];
	int naxis;
	long naxes[2];
	int i;

	for (i = 0; i < 400; i += 1) {
		data[i] = (short)i;
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes_orig);
	call_05(ffppri, f, 1, 1, 400, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	/* Resize from 20x20 to 5x5. */
	call_04(ffrsim, f, SHORT_IMG, 2, naxes_new);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffghpr, f, 2, NULL, NULL, &naxis, naxes, NULL, NULL, NULL);
	fail_if(naxis != 2);
	fail_if(naxes[0] != 5);
	fail_if(naxes[1] != 5);
	call_01(ffclos, f);
}

static void
test_insert_rows(void)
{
	/* Test ffirow - insert rows into a table. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300 };
	long result[5];
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Insert 2 rows after row 1. */
	call_03(ffirow, f, 1, 2);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 5);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 5, 0, result, NULL);
	/* Row 1 = 100, rows 2-3 = 0 (inserted), rows 4-5 = 200, 300. */
	fail_if(result[0] != 100);
	fail_if(result[1] != 0);
	fail_if(result[2] != 0);
	fail_if(result[3] != 200);
	fail_if(result[4] != 300);
	call_01(ffclos, f);
}

static void
test_delete_rows(void)
{
	/* Test ffdrow - delete rows from a table. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300, 400, 500 };
	long result[3];
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Delete 2 rows starting at row 2. */
	call_03(ffdrow, f, 2, 2);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, NULL);
	/* Rows 2-3 deleted: 100, 400, 500. */
	fail_if(result[0] != 100);
	fail_if(result[1] != 400);
	fail_if(result[2] != 500);
	call_01(ffclos, f);
}

static void
test_delete_row_range(void)
{
	/* Test ffdrrg - delete a range of rows. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
	long result[6];
	char rowrange[] = "2-4, 8";  /* Delete rows 2, 3, 4, 8. */
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 10, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	call_02(ffdrrg, f, rowrange);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 6);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 6, 0, result, NULL);
	/* Remaining: 10, 50, 60, 70, 90, 100. */
	fail_if(result[0] != 10);
	fail_if(result[1] != 50);
	fail_if(result[2] != 60);
	fail_if(result[3] != 70);
	fail_if(result[4] != 90);
	fail_if(result[5] != 100);
	call_01(ffclos, f);
}

static void
test_delete_rows_by_list(void)
{
	/* Test ffdrws - delete rows by selection list. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 10, 20, 30, 40, 50 };
	long rowlist[] = { 2, 4 };  /* Delete rows 2 and 4. */
	long result[3];
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	call_03(ffdrws, f, rowlist, 2);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, NULL);
	/* Remaining: 10, 30, 50. */
	fail_if(result[0] != 10);
	fail_if(result[1] != 30);
	fail_if(result[2] != 50);
	call_01(ffclos, f);
}

static void
test_insert_column(void)
{
	/* Test fficol - insert a new column. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL3" };
	char *tform[] = { "1J", "1J" };
	long data1[] = { 10, 20, 30 };
	long data3[] = { 100, 200, 300 };
	long result[3];
	int ncols;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 2, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data1);
	call_06(ffpclj, f, 2, 1, 1, 3, data3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Insert a column at position 2. */
	call_04(fficol, f, 2, "COL2", "1E");
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Column 3 now has the old COL3 data. */
	call_08(ffgcvj, f, 3, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	fail_if(result[2] != 300);
	call_01(ffclos, f);
}

static void
test_insert_columns(void)
{
	/* Test fficls - insert multiple columns. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	char *newttype[] = { "COLA", "COLB" };
	char *newtform[] = { "1E", "1D" };
	int ncols;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Insert 2 columns at position 1. */
	call_05(fficls, f, 1, 2, newttype, newtform);
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 3);
	call_01(ffclos, f);
}

static void
test_delete_column(void)
{
	/* Test ffdcol - delete a column. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2", "COL3" };
	char *tform[] = { "1J", "1J", "1J" };
	long data1[] = { 10, 20, 30 };
	long data2[] = { 40, 50, 60 };
	long data3[] = { 70, 80, 90 };
	long result[3];
	int ncols;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 3, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data1);
	call_06(ffpclj, f, 2, 1, 1, 3, data2);
	call_06(ffpclj, f, 3, 1, 1, 3, data3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Delete column 2. */
	call_02(ffdcol, f, 2);
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 2);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	/* Column 2 now has the old COL3 data. */
	call_08(ffgcvj, f, 2, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 70);
	fail_if(result[1] != 80);
	fail_if(result[2] != 90);
	call_01(ffclos, f);
}

static void
test_copy_column(void)
{
	/* Test ffcpcl - copy a column between tables. */
	fitsfile *f1, *f2;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300 };
	long result[3];

	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f1, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f1, 1, 1, 1, 3, data);
	call_01(ffclos, f1);

	call_02(ffinit, &f2, "!" test_path2);
	call_04(ffphps, f2, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f2, BINARY_TBL, 3, 0, NULL, NULL, NULL, NULL);
	call_01(ffclos, f2);

	call_03(ffopen, &f1, test_path, READONLY);
	call_03(ffopen, &f2, test_path2, READWRITE);
	call_03(ffmahd, f1, 2, NULL);
	call_03(ffmahd, f2, 2, NULL);
	/* Copy column 1 from f1 to column 1 of f2 (create_col=1 to insert). */
	call_05(ffcpcl, f1, f2, 1, 1, 1);
	call_01(ffclos, f1);
	call_01(ffclos, f2);

	call_03(ffopen, &f2, test_path2, READONLY);
	call_03(ffmahd, f2, 2, NULL);
	call_08(ffgcvj, f2, 1, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 100);
	fail_if(result[1] != 200);
	fail_if(result[2] != 300);
	call_01(ffclos, f2);
}

static void
test_copy_rows(void)
{
	/* Test ffcprw - copy rows between tables. */
	fitsfile *f1, *f2;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 10, 20, 30, 40, 50 };
	long result[3];
	LONGLONG nrows;

	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f1, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f1, 1, 1, 1, 5, data);
	call_01(ffclos, f1);

	call_02(ffinit, &f2, "!" test_path2);
	call_04(ffphps, f2, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f2, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f2);

	call_03(ffopen, &f1, test_path, READONLY);
	call_03(ffopen, &f2, test_path2, READWRITE);
	call_03(ffmahd, f1, 2, NULL);
	call_03(ffmahd, f2, 2, NULL);
	/* Copy 3 rows starting at row 2 (rows 2, 3, 4). */
	call_04(ffcprw, f1, f2, 2, 3);
	call_01(ffclos, f1);
	call_01(ffclos, f2);

	call_03(ffopen, &f2, test_path2, READONLY);
	call_03(ffmahd, f2, 2, NULL);
	call_02(ffgnrwll, f2, &nrows);
	fail_if(nrows != 3);
	call_08(ffgcvj, f2, 1, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 20);
	fail_if(result[1] != 30);
	fail_if(result[2] != 40);
	call_01(ffclos, f2);
}

static void
test_copy_selected_rows(void)
{
	/* Test ffsrow - select and copy rows by expression. */
	fitsfile *f1, *f2;
	int status = 0;
	char *ttype[] = { "VALUE" };
	char *tform[] = { "1J" };
	long data[] = { 10, 25, 30, 45, 50 };
	long result[4];
	LONGLONG nrows;
	char expr[] = "VALUE > 20";

	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f1, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f1, 1, 1, 1, 5, data);
	call_01(ffclos, f1);

	call_02(ffinit, &f2, "!" test_path2);
	call_04(ffphps, f2, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f2, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f2);

	call_03(ffopen, &f1, test_path, READONLY);
	call_03(ffopen, &f2, test_path2, READWRITE);
	call_03(ffmahd, f1, 2, NULL);
	call_03(ffmahd, f2, 2, NULL);
	/* Copy rows where VALUE > 20. */
	call_03(ffsrow, f1, f2, expr);
	call_01(ffclos, f1);
	call_01(ffclos, f2);

	call_03(ffopen, &f2, test_path2, READONLY);
	call_03(ffmahd, f2, 2, NULL);
	call_02(ffgnrwll, f2, &nrows);
	fail_if(nrows != 4);  /* 25, 30, 45, 50 all > 20. */
	call_08(ffgcvj, f2, 1, 1, 1, 4, 0, result, NULL);
	fail_if(result[0] != 25);
	fail_if(result[1] != 30);
	fail_if(result[2] != 45);
	fail_if(result[3] != 50);
	call_01(ffclos, f2);
}

static void
test_modify_vector_column(void)
{
	/* Test ffmvec - modify vector length of a column. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECTOR" };
	char *tform[] = { "3J" };  /* 3 longs per row. */
	long data[] = { 1, 2, 3, 4, 5, 6 };
	long result[8];
	int typecode;
	long repeat, width;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 6, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Expand vector from 3 to 4 elements. */
	call_03(ffmvec, f, 1, 4);
	call_05(ffgtcl, f, 1, &typecode, &repeat, &width);
	fail_if(repeat != 4);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 8, 0, result, NULL);
	/* Original data preserved, new element is 0. */
	fail_if(result[0] != 1);
	fail_if(result[1] != 2);
	fail_if(result[2] != 3);
	fail_if(result[3] != 0);  /* New 4th element. */
	fail_if(result[4] != 4);
	fail_if(result[5] != 5);
	fail_if(result[6] != 6);
	fail_if(result[7] != 0);  /* New 4th element. */
	call_01(ffclos, f);
}

static void
test_resize_image_bad_bitpix(void)
{
	/* Test ffrsim with bad BITPIX value. */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	status = 0;
	ffrsim(f, 99, 1, naxes, &status);  /* Invalid BITPIX. */
	fail_if(status != BAD_BITPIX);
	status = 0;
	call_01(ffclos, f);
}

static void
test_resize_image_bad_naxis(void)
{
	/* Test ffrsim with bad NAXIS value. */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	status = 0;
	ffrsim(f, SHORT_IMG, -1, naxes, &status);  /* Negative NAXIS. */
	fail_if(status != BAD_NAXIS);
	status = 0;
	call_01(ffclos, f);
}

static void
test_resize_image_bad_naxes(void)
{
	/* Test ffrsimll with bad NAXES value. */
	fitsfile *f;
	int status = 0;
	long naxes_orig[] = { 10 };
	LONGLONG naxes_new[] = { -5 };  /* Negative dimension. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes_orig);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	status = 0;
	ffrsimll(f, SHORT_IMG, 1, naxes_new, &status);
	fail_if(status != BAD_NAXES);
	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffcpky - copy indexed keywords between HDUs. The function
 * copies keywords like TTYPE1 -> TTYPE2. ffcpky appends a new keyword
 * record; it doesn't replace existing ones
 */
static void
test_copy_indexed_keyword(void)
{

	fitsfile *f1, *f2;
	int status = 0;
	char *ttype[] = { "SRCNAME" };
	char *tform[] = { "1J" };
	char *ttype2[] = { "DESTCOL" };
	char *tform2[] = { "1J" };
	char value[FLEN_VALUE];

	call_02(ffinit, &f1, "!" test_path);
	call_04(ffphps, f1, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f1, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	/* Add a custom indexed keyword MYKEY1. */
	call_04(ffpkys, f1, "MYKEY1", "VALUE1", NULL);
	call_01(ffclos, f1);

	call_02(ffinit, &f2, "!" test_path2);
	call_04(ffphps, f2, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f2, BINARY_TBL, 3, 1, ttype2, tform2, NULL, NULL);
	call_01(ffclos, f2);

	call_03(ffopen, &f1, test_path, READONLY);
	call_03(ffopen, &f2, test_path2, READWRITE);
	call_03(ffmahd, f1, 2, NULL);
	call_03(ffmahd, f2, 2, NULL);
	/* Copy MYKEY1 from f1 to MYKEY2 in f2. */
	call_05(ffcpky, f1, f2, 1, 2, "MYKEY");
	call_01(ffclos, f1);
	call_01(ffclos, f2);

	call_03(ffopen, &f2, test_path2, READONLY);
	call_03(ffmahd, f2, 2, NULL);
	/* MYKEY2 should be created with value from MYKEY1. */
	call_04(ffgkys, f2, "MYKEY2", value, NULL);
	fail_if(strcmp(value, "VALUE1") != 0);
	call_01(ffclos, f2);
}

static void
test_insert_rows_at_start(void)
{
	/* Test ffirow - insert rows at the beginning. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200 };
	long result[4];
	LONGLONG nrows;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 2, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Insert 2 rows at the beginning (after row 0). */
	call_03(ffirow, f, 0, 2);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 4);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 4, 0, result, NULL);
	/* Rows 1-2 = 0 (inserted), rows 3-4 = 100, 200. */
	fail_if(result[0] != 0);
	fail_if(result[1] != 0);
	fail_if(result[2] != 100);
	fail_if(result[3] != 200);
	call_01(ffclos, f);
}

static void
test_resize_to_zero_dimensions(void)
{
	/* Test ffrsim - resize to 0 dimensions (null array). */
	fitsfile *f;
	int status = 0;
	long naxes_orig[] = { 10, 10 };
	long naxes_new[] = { 0 };
	int naxis;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes_orig);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_04(ffrsim, f, SHORT_IMG, 0, naxes_new);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffghpr, f, 2, NULL, NULL, &naxis, NULL, NULL, NULL, NULL);
	fail_if(naxis != 0);
	call_01(ffclos, f);
}

static void
test_copy_column_same_file(void)
{
	/* Test ffcpcl - copy column within same file. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1", "COL2" };
	char *tform[] = { "1J", "1J" };
	long data[] = { 10, 20, 30 };
	long result[3];
	int ncols;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 2, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Copy column 1 to column 3 (insert new column). */
	call_05(ffcpcl, f, f, 1, 3, 1);
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 3, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 10);
	fail_if(result[1] != 20);
	fail_if(result[2] != 30);
	call_01(ffclos, f);
}

static void
test_delete_row_range_longlong(void)
{
	/* Test ffdrwsll - delete row range with LONGLONG coordinates. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 10, 20, 30, 40, 50 };
	long result[3];
	LONGLONG nrows;
	LONGLONG rowlist[] = { 2, 3 };  /* Delete rows 2 and 3. */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, data);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);
	/* Delete rows 2-3 using LONGLONG coordinates. */
	call_03(ffdrwsll, f, rowlist, 2LL);
	call_02(ffgnrwll, f, &nrows);
	fail_if(nrows != 3);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_08(ffgcvj, f, 1, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 10);  /* Row 1 preserved. */
	fail_if(result[1] != 40);  /* Original row 4, now row 2. */
	fail_if(result[2] != 50);  /* Original row 5, now row 3. */
	call_01(ffclos, f);
}

static void
test_ffrwrgll(void)
{
	/* Test ffrwrgll - parse row list into ranges. */
	int status = 0;
	int numranges;
	LONGLONG minrow[10], maxrow[10];

	/* Parse simple range list. */
	call_06(ffrwrgll, "1-5,10-20,30", 50, 10, &numranges, minrow, maxrow);
	fail_if(numranges != 3);
	fail_if(minrow[0] != 1 || maxrow[0] != 5);
	fail_if(minrow[1] != 10 || maxrow[1] != 20);
	fail_if(minrow[2] != 30 || maxrow[2] != 30);

	/* Parse single row list. */
	call_06(ffrwrgll, "5,10,15", 20, 10, &numranges, minrow, maxrow);
	fail_if(numranges != 3);
	fail_if(minrow[0] != 5 || maxrow[0] != 5);
	fail_if(minrow[1] != 10 || maxrow[1] != 10);
	fail_if(minrow[2] != 15 || maxrow[2] != 15);
}

static void
test_ffccls(void)
{
	/* Test ffccls - copy multiple columns between files. */
	fitsfile *in, *out;
	int status = 0;
	char *ttype[] = { "COL1", "COL2", "COL3" };
	char *tform[] = { "1J", "1E", "10A" };
	long data[] = { 10, 20, 30, 40, 50 };
	long result[5];
	int ncols;

	/* Create input table with 3 columns, 5 rows. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, in, BINARY_TBL, 5, 3, ttype, tform, NULL, "DATA");
	call_06(ffpclj, in, 1, 1, 1, 5, data);
	call_01(ffclos, in);

	/* Create output table with 1 column. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, out, BINARY_TBL, 5, 1, ttype, tform, NULL, "OUTPUT");
	call_01(ffclos, out);

	/* Reopen both. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	call_03(ffmahd, in, 2, NULL);
	call_03(ffmahd, out, 2, NULL);

	/* Copy columns 1-2 from input to output (create new cols). */
	call_06(ffccls, in, out, 1, 2, 2, 1);

	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify the copied columns. */
	call_03(ffopen, &out, test_path2, READONLY);
	call_03(ffmahd, out, 2, NULL);
	call_02(ffgncl, out, &ncols);
	fail_if(ncols != 3);  /* Original 1 + 2 copied = 3 */
	call_08(ffgcvj, out, 2, 1, 1, 5, 0, result, NULL);
	fail_if(result[0] != 10);
	fail_if(result[4] != 50);
	call_01(ffclos, out);
}

static void
test_ffcpsr(void)
{
	/* Test ffcpsr - copy selected rows. */
	fitsfile *in, *out;
	int status = 0;
	char *ttype[] = { "DATA" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300, 400, 500 };
	long result[5];
	LONGLONG nrows;
	char row_status[] = { 1, 0, 1, 0, 1 };  /* Keep rows 1, 3, 5. */

	/* Create input table. */
	call_02(ffinit, &in, "!" test_path);
	call_04(ffphps, in, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, in, BINARY_TBL, 5, 1, ttype, tform, NULL, "DATA");
	call_06(ffpclj, in, 1, 1, 1, 5, data);
	call_01(ffclos, in);

	/* Create output table with 0 rows. */
	call_02(ffinit, &out, "!" test_path2);
	call_04(ffphps, out, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, out, BINARY_TBL, 0, 1, ttype, tform, NULL, "OUTPUT");
	call_01(ffclos, out);

	/* Reopen both. */
	call_03(ffopen, &in, test_path, READONLY);
	call_03(ffopen, &out, test_path2, READWRITE);
	call_03(ffmahd, in, 2, NULL);
	call_03(ffmahd, out, 2, NULL);

	/* Copy selected rows. */
	call_05(ffcpsr, in, out, 1, 5, row_status);

	call_01(ffclos, in);
	call_01(ffclos, out);

	/* Verify the copied rows. */
	call_03(ffopen, &out, test_path2, READONLY);
	call_03(ffmahd, out, 2, NULL);
	call_02(ffgnrwll, out, &nrows);
	fail_if(nrows != 3);  /* Only 3 rows should be copied. */
	call_08(ffgcvj, out, 1, 1, 1, 3, 0, result, NULL);
	fail_if(result[0] != 100);  /* Row 1. */
	fail_if(result[1] != 300);  /* Row 3. */
	fail_if(result[2] != 500);  /* Row 5. */
	call_01(ffclos, out);
}

/* Test ffrsim with BYTE_IMG bitpix handling */
static void
test_resize_image_byte(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long naxes_new[] = { 20 };
	int bitpix;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_04(ffrsim, f, BYTE_IMG, 1, naxes_new);
	call_02(ffgiet, f, &bitpix);
	fail_if(bitpix != BYTE_IMG);
	call_01(ffclos, f);
}

/* Test ffrsim with LONGLONG_IMG bitpix handling */
static void
test_resize_image_longlong(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long naxes_new[] = { 20 };
	int bitpix;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, LONGLONG_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_04(ffrsim, f, LONGLONG_IMG, 1, naxes_new);
	call_02(ffgiet, f, &bitpix);
	fail_if(bitpix != LONGLONG_IMG);
	call_01(ffclos, f);
}

/*
 * Test ffrsim with USHORT_IMG, verifying BZERO/BSCALE for unsigned
 * short
 */
static void
test_resize_image_ushort(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long naxes_new[] = { 20 };
	double bzero;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, USHORT_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_04(ffrsim, f, USHORT_IMG, 1, naxes_new);
	call_04(ffgkyd, f, "BZERO", &bzero, NULL);
	fail_if(bzero != 32768.0);
	call_01(ffclos, f);
}

/* Test ffrsim with ULONG_IMG, verifying BZERO/BSCALE for unsigned long */
static void
test_resize_image_ulong(void)
{

	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	long naxes_new[] = { 20 };
	double bzero;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, ULONG_IMG, 1, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_04(ffrsim, f, ULONG_IMG, 1, naxes_new);
	call_04(ffgkyd, f, "BZERO", &bzero, NULL);
	fail_if(bzero != 2147483648.0);
	call_01(ffclos, f);
}

int
main(void)
{
	test_resize_image();
	test_resize_image_smaller();
	test_insert_rows();
	test_delete_rows();
	test_delete_row_range();
	test_delete_row_range_longlong();
	test_delete_rows_by_list();
	test_insert_column();
	test_insert_columns();
	test_delete_column();
	test_copy_column();
	test_copy_rows();
	test_copy_selected_rows();
	test_modify_vector_column();
	test_resize_image_bad_bitpix();
	test_resize_image_bad_naxis();
	test_resize_image_bad_naxes();
	test_copy_indexed_keyword();
	test_insert_rows_at_start();
	test_resize_to_zero_dimensions();
	test_copy_column_same_file();
	test_ffrwrgll();
	test_ffccls();
	test_ffcpsr();
	test_resize_image_byte();
	test_resize_image_longlong();
	test_resize_image_ushort();
	test_resize_image_ulong();
	return 0;
}
