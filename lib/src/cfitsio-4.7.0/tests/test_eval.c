/*
 * Tests for eval_f.c - Expression evaluation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_eval.fits"

/*
 * Create a test table with various column types for expression testing
 */
static void
create_test_table(fitsfile **fptr)
{
	int status = 0;
	char *ttype[] = { "INTCOL", "FLOATCOL", "STRCOL", "BOOLCOL" };
	char *tform[] = { "1J", "1E", "10A", "1L" };
	long intdata[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	float floatdata[] = { 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f };
	char logdata[] = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
	char *strdata[] = {
		"alpha", "beta", "gamma", "delta", "epsilon",
		"zeta", "eta", "theta", "iota", "kappa"
	};
	int i;

	call_02(ffinit, fptr, "!" test_path);
	call_04(ffphps, *fptr, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, *fptr, BINARY_TBL, 10, 4, ttype, tform, NULL, NULL);

	/* Write integer column (use long for TLONG/1J) */
	call_06(ffpclj, *fptr, 1, 1, 1, 10, intdata);

	/* Write float column */
	call_06(ffpcle, *fptr, 2, 1, 1, 10, floatdata);

	/* Write string column */
	for (i = 0; i < 10; i += 1) {
		call_06(ffpcls, *fptr, 3, i + 1, 1, 1, &strdata[i]);
	}

	/* Write logical column */
	call_06(ffpcll, *fptr, 4, 1, 1, 10, logdata);
}

/*
 * Test fffrow - find rows matching boolean expression
 */
static void
test_fffrow_basic(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test simple comparison: INTCOL > 5 */
	call_06(fffrow, f, "INTCOL > 5", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 5);  /* rows 6,7,8,9,10 */
	fail_if(row_status[0] != 0);  /* row 1: 1 > 5 is false */
	fail_if(row_status[5] != 1);  /* row 6: 6 > 5 is true */

	call_01(ffclos, f);
}

static void
test_fffrow_float_comparison(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test float comparison: FLOATCOL < 5.0 */
	call_06(fffrow, f, "FLOATCOL < 5.0", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 4);  /* rows 1,2,3,4 (1.5, 2.5, 3.5, 4.5 < 5.0) */

	call_01(ffclos, f);
}

static void
test_fffrow_logical_column(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test logical column directly */
	call_06(fffrow, f, "BOOLCOL", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 5);  /* odd rows are true */

	call_01(ffclos, f);
}

static void
test_fffrow_compound_expression(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test compound expression: INTCOL > 3 && INTCOL < 8 */
	call_06(fffrow, f, "INTCOL > 3 && INTCOL < 8", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 4);  /* rows 4,5,6,7 */

	call_01(ffclos, f);
}

static void
test_fffrow_constant_true(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test constant true expression */
	call_06(fffrow, f, "1 == 1", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 10);  /* all rows match */
	fail_if(row_status[0] != 1);  /* verify row status */

	call_01(ffclos, f);
}

static void
test_fffrow_constant_false(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test constant false expression. */
	call_06(fffrow, f, "1 == 0", 1, 10, &n_good_rows, row_status);
	fail_if(row_status[0] != 0);  /* verify row status is false */

	call_01(ffclos, f);
}

/*
 * Note: ffffrw is not tested because it has a bug in the cfitsio library.
 * The ffffrw_work function is missing a call to Setup_DataArrays that
 * fits_parser_workfn has, causing a segfault when evaluating expressions.
 * See https://github.com/HEASARC/cfitsio/issues/117
 */

/*
 * Test fftexp - test expression parsing
 */
static void
test_fftexp_integer(void)
{
	fitsfile *f;
	int status = 0;
	int datatype, naxis;
	long nelem;
	int maxdim = 10;
	long naxes[10];

	create_test_table(&f);

	/* Test integer column expression */
	call_07(fftexp, f, "INTCOL", maxdim, &datatype, &nelem, &naxis, naxes);
	fail_if(datatype != TLONG);
	fail_if(nelem != 1);

	call_01(ffclos, f);
}

static void
test_fftexp_float(void)
{
	fitsfile *f;
	int status = 0;
	int datatype, naxis;
	long nelem;
	int maxdim = 10;
	long naxes[10];

	create_test_table(&f);

	/* Test float column expression */
	call_07(fftexp, f, "FLOATCOL", maxdim, &datatype, &nelem, &naxis, naxes);
	fail_if(datatype != TDOUBLE);  /* floats promoted to double in expressions */
	fail_if(nelem != 1);

	call_01(ffclos, f);
}

static void
test_fftexp_arithmetic(void)
{
	fitsfile *f;
	int status = 0;
	int datatype, naxis;
	long nelem;
	int maxdim = 10;
	long naxes[10];

	create_test_table(&f);

	/* Test arithmetic expression */
	call_07(fftexp, f, "INTCOL * 2 + 1", maxdim, &datatype, &nelem, &naxis, naxes);
	fail_if(datatype != TLONG);
	fail_if(nelem != 1);

	call_01(ffclos, f);
}

static void
test_fftexp_boolean(void)
{
	fitsfile *f;
	int status = 0;
	int datatype, naxis;
	long nelem;
	int maxdim = 10;
	long naxes[10];

	create_test_table(&f);

	/* Test boolean expression */
	call_07(fftexp, f, "INTCOL > 5", maxdim, &datatype, &nelem, &naxis, naxes);
	fail_if(datatype != TLOGICAL);
	fail_if(nelem != 1);

	call_01(ffclos, f);
}

/*
 * Test ffcrow - calculate expression for each row
 * ffcrow(fptr, datatype, expr, firstrow, nelements, nulval, array, anynul, status)
 */
static void
test_ffcrow_arithmetic(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Calculate INTCOL * 2 for first 5 rows */
	call_08(ffcrow, f, TDOUBLE, "INTCOL * 2", 1, 5, NULL, results, &anynul);
	fail_if(results[0] != 2.0);   /* 1 * 2 */
	fail_if(results[1] != 4.0);   /* 2 * 2 */
	fail_if(results[4] != 10.0);  /* 5 * 2 */

	call_01(ffclos, f);
}

static void
test_ffcrow_add_columns(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Calculate INTCOL + FLOATCOL for first 3 rows */
	call_08(ffcrow, f, TDOUBLE, "INTCOL + FLOATCOL", 1, 3, NULL, results, &anynul);
	fail_if(results[0] != 2.5);   /* 1 + 1.5 */
	fail_if(results[1] != 4.5);   /* 2 + 2.5 */
	fail_if(results[2] != 6.5);   /* 3 + 3.5 */

	call_01(ffclos, f);
}

/*
 * Note: ffsrow and ffcalc tests are omitted because they require complex
 * setup with properly initialized output tables that match the input schema.
 */

/*
 * Test ffcalc - calculate column from expression
 */
static void
test_ffcalc_new_column(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynull;

	create_test_table(&f);

	/* Calculate new column DOUBLECOL = INTCOL * 2.5 */
	call_05(ffcalc, f, "INTCOL * 2.5", f, "DOUBLECOL", NULL);

	/* Read back the calculated column */
	call_08(ffgcvd, f, 5, 1, 1, 10, 0.0, results, &anynull);
	fail_if(results[0] < 2.4 || results[0] > 2.6);   /* 1 * 2.5 = 2.5 */
	fail_if(results[1] < 4.9 || results[1] > 5.1);   /* 2 * 2.5 = 5.0 */
	fail_if(results[9] < 24.9 || results[9] > 25.1); /* 10 * 2.5 = 25.0 */

	call_01(ffclos, f);
}

static void
test_ffcalc_overwrite_column(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynull;

	create_test_table(&f);

	/* Overwrite FLOATCOL with INTCOL + 100 */
	call_05(ffcalc, f, "INTCOL + 100", f, "FLOATCOL", NULL);

	/* Read back to verify */
	call_08(ffgcvd, f, 2, 1, 1, 5, 0.0, results, &anynull);
	fail_if(results[0] < 100.9 || results[0] > 101.1);  /* 1 + 100 */
	fail_if(results[4] < 104.9 || results[4] > 105.1);  /* 5 + 100 */

	call_01(ffclos, f);
}

/*
 * Test more expression operators
 */
static void
test_fffrow_or_expression(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test OR expression: INTCOL == 1 || INTCOL == 10 */
	call_06(fffrow, f, "INTCOL == 1 || INTCOL == 10", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 2);
	fail_if(row_status[0] != 1);  /* row 1 */
	fail_if(row_status[9] != 1);  /* row 10 */

	call_01(ffclos, f);
}

static void
test_fffrow_not_expression(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test NOT expression: !(INTCOL > 5) */
	call_06(fffrow, f, "!(INTCOL > 5)", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 5);  /* rows 1-5 */

	call_01(ffclos, f);
}

static void
test_fffrow_modulo(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test modulo: INTCOL % 2 == 0 (even numbers) */
	call_06(fffrow, f, "INTCOL % 2 == 0", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 5);  /* rows 2,4,6,8,10 */

	call_01(ffclos, f);
}

static void
test_fffrow_between(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test >= and <= together */
	call_06(fffrow, f, "INTCOL >= 3 && INTCOL <= 7", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 5);  /* rows 3,4,5,6,7 */

	call_01(ffclos, f);
}

static void
test_fffrow_not_equal(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test not equal: INTCOL != 5 */
	call_06(fffrow, f, "INTCOL != 5", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 9);  /* all except row 5 */
	fail_if(row_status[4] != 0);  /* row 5 should be false */

	call_01(ffclos, f);
}

/*
 * Test mathematical functions in expressions
 */
static void
test_ffcrow_math_functions(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test ABS function */
	call_08(ffcrow, f, TDOUBLE, "ABS(INTCOL - 5)", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 3.9 || results[0] > 4.1);  /* |1-5| = 4 */
	fail_if(results[4] < -0.1 || results[4] > 0.1);  /* |5-5| = 0 */

	call_01(ffclos, f);
}

static void
test_ffcrow_min_max(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test MIN function */
	call_08(ffcrow, f, TDOUBLE, "MIN(INTCOL, 3)", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 0.9 || results[0] > 1.1);  /* min(1,3) = 1 */
	fail_if(results[4] < 2.9 || results[4] > 3.1);  /* min(5,3) = 3 */

	/* Test MAX function */
	call_08(ffcrow, f, TDOUBLE, "MAX(INTCOL, 3)", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 2.9 || results[0] > 3.1);  /* max(1,3) = 3 */
	fail_if(results[4] < 4.9 || results[4] > 5.1);  /* max(5,3) = 5 */

	call_01(ffclos, f);
}

static void
test_ffcrow_sqrt(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test SQRT function */
	call_08(ffcrow, f, TDOUBLE, "SQRT(FLOATCOL)", 1, 4, NULL, results, &anynul);
	/* sqrt(1.5) ~ 1.22, sqrt(2.5) ~ 1.58 */
	fail_if(results[0] < 1.2 || results[0] > 1.3);
	fail_if(results[3] < 2.1 || results[3] > 2.2);  /* sqrt(4.5) ~ 2.12 */

	call_01(ffclos, f);
}

static void
test_ffcrow_power(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test power operator ** */
	call_08(ffcrow, f, TDOUBLE, "INTCOL ** 2", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 0.9 || results[0] > 1.1);    /* 1^2 = 1 */
	fail_if(results[1] < 3.9 || results[1] > 4.1);    /* 2^2 = 4 */
	fail_if(results[2] < 8.9 || results[2] > 9.1);    /* 3^2 = 9 */
	fail_if(results[4] < 24.9 || results[4] > 25.1);  /* 5^2 = 25 */

	call_01(ffclos, f);
}

static void
test_ffcrow_log(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test LOG (natural log) */
	call_08(ffcrow, f, TDOUBLE, "LOG(INTCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < -0.1 || results[0] > 0.1);  /* ln(1) = 0 */
	fail_if(results[1] < 0.6 || results[1] > 0.75);  /* ln(2) ~ 0.693 */

	/* Test LOG10 */
	call_08(ffcrow, f, TDOUBLE, "LOG10(INTCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < -0.1 || results[0] > 0.1);  /* log10(1) = 0 */
	fail_if(results[1] < 0.29 || results[1] > 0.32); /* log10(2) ~ 0.301 */

	call_01(ffclos, f);
}

static void
test_ffcrow_trig(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test SIN - values should be between -1 and 1 */
	call_08(ffcrow, f, TDOUBLE, "SIN(FLOATCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < -1.1 || results[0] > 1.1);

	/* Test COS */
	call_08(ffcrow, f, TDOUBLE, "COS(FLOATCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < -1.1 || results[0] > 1.1);

	call_01(ffclos, f);
}

static void
test_ffcrow_conditional(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test ternary conditional: (cond) ? val1 : val2 */
	call_08(ffcrow, f, TDOUBLE, "(INTCOL > 5) ? 100 : 0", 1, 10, NULL, results, &anynul);
	fail_if(results[0] > 0.1);      /* row 1: 1 > 5 is false, result = 0 */
	fail_if(results[5] < 99.9);     /* row 6: 6 > 5 is true, result = 100 */
	fail_if(results[9] < 99.9);     /* row 10: 10 > 5 is true */

	call_01(ffclos, f);
}

static void
test_ffcrow_division(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test division */
	call_08(ffcrow, f, TDOUBLE, "FLOATCOL / INTCOL", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 1.4 || results[0] > 1.6);  /* 1.5 / 1 = 1.5 */
	fail_if(results[1] < 1.2 || results[1] > 1.3);  /* 2.5 / 2 = 1.25 */

	call_01(ffclos, f);
}

static void
test_ffcrow_subtraction(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test subtraction */
	call_08(ffcrow, f, TDOUBLE, "FLOATCOL - INTCOL", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 0.4 || results[0] > 0.6);  /* 1.5 - 1 = 0.5 */
	fail_if(results[4] < 0.4 || results[4] > 0.6);  /* 5.5 - 5 = 0.5 */

	call_01(ffclos, f);
}

/*
 * Test #row special variable
 */
static void
test_ffcrow_row_number(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test #row returns row number */
	call_08(ffcrow, f, TDOUBLE, "#row", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 0.9 || results[0] > 1.1);  /* row 1 */
	fail_if(results[4] < 4.9 || results[4] > 5.1);  /* row 5 */

	call_01(ffclos, f);
}

/*
 * Test ffsrow - select rows to output file
 */
static void
test_ffsrow_copy_selected(void)
{
	fitsfile *infile, *outfile;
	int status = 0;
	long nrows;
	char *ttype[] = { "INTCOL", "FLOATCOL", "STRCOL", "BOOLCOL" };
	char *tform[] = { "1J", "1E", "10A", "1L" };
	long intdata[10];
	int anynull;

	/* Create input table */
	create_test_table(&infile);
	call_01(ffclos, infile);

	/* Reopen input and create output */
	call_03(ffopen, &infile, test_path, READONLY);
	call_02(ffinit, &outfile, "!test_eval_out.fits");
	call_04(ffphps, outfile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, outfile, BINARY_TBL, 0, 4, ttype, tform, NULL, NULL);

	/* Select rows where INTCOL > 5 and copy to output */
	call_03(ffsrow, infile, outfile, "INTCOL > 5");

	/* Verify output has 5 rows (rows 6-10 from input) */
	call_02(ffgnrw, outfile, &nrows);
	fail_if(nrows != 5);

	/* Verify data was copied correctly */
	call_08(ffgcvj, outfile, 1, 1, 1, 5, 0, intdata, &anynull);
	fail_if(intdata[0] != 6);  /* first row should have value 6 */
	fail_if(intdata[4] != 10); /* last row should have value 10 */

	call_01(ffclos, infile);
	call_01(ffclos, outfile);
	remove("test_eval_out.fits");
}

static void
test_ffsrow_same_file(void)
{
	fitsfile *f;
	int status = 0;
	long nrows;
	long intdata[10];
	int anynull;

	/* Create test table */
	create_test_table(&f);
	call_01(ffclos, f);

	/* Reopen for read/write */
	call_03(ffopen, &f, test_path, READWRITE);

	/* Filter to keep only rows where INTCOL <= 5 */
	call_03(ffsrow, f, f, "INTCOL <= 5");

	/* Verify table now has 5 rows */
	call_02(ffgnrw, f, &nrows);
	fail_if(nrows != 5);

	/* Verify correct rows remain */
	call_08(ffgcvj, f, 1, 1, 1, 5, 0, intdata, &anynull);
	fail_if(intdata[0] != 1);
	fail_if(intdata[4] != 5);

	call_01(ffclos, f);
}

/*
 * Test ffcalc_rng - calculate column with row ranges
 */
static void
test_ffcalc_rng_basic(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynull;
	long start[1] = { 3 };
	long end[1] = { 7 };

	create_test_table(&f);

	/* Calculate INTCOL * 10 for rows 3-7 only */
	call_08(ffcalc_rng, f, "INTCOL * 10", f, "COMPUTED", NULL, 1, start, end);

	/* Read back the calculated column for rows 3-7 */
	call_08(ffgcvd, f, 5, 3, 1, 5, 0.0, results, &anynull);
	fail_if(results[0] < 29.9 || results[0] > 30.1);   /* 3 * 10 = 30 */
	fail_if(results[4] < 69.9 || results[4] > 70.1);   /* 7 * 10 = 70 */

	call_01(ffclos, f);
}

/*
 * Test string operations
 */
static void
test_ffcrow_string_comparison(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test string equality */
	call_06(fffrow, f, "STRCOL == 'alpha'", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 1);
	fail_if(row_status[0] != 1);  /* first row is 'alpha' */

	call_01(ffclos, f);
}

/*
 * Test negation and unary minus
 */
static void
test_ffcrow_negation(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test unary minus */
	call_08(ffcrow, f, TDOUBLE, "-INTCOL", 1, 5, NULL, results, &anynul);
	fail_if(results[0] > -0.9 || results[0] < -1.1);   /* -1 */
	fail_if(results[4] > -4.9 || results[4] < -5.1);   /* -5 */

	call_01(ffclos, f);
}

/*
 * Test floor and ceiling functions
 */
static void
test_ffcrow_floor_ceil(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test FLOOR */
	call_08(ffcrow, f, TDOUBLE, "FLOOR(FLOATCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < 0.9 || results[0] > 1.1);   /* floor(1.5) = 1 */
	fail_if(results[1] < 1.9 || results[1] > 2.1);   /* floor(2.5) = 2 */

	/* Test CEIL */
	call_08(ffcrow, f, TDOUBLE, "CEIL(FLOATCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < 1.9 || results[0] > 2.1);   /* ceil(1.5) = 2 */
	fail_if(results[1] < 2.9 || results[1] > 3.1);   /* ceil(2.5) = 3 */

	call_01(ffclos, f);
}

/*
 * Test ROUND function
 */
static void
test_ffcrow_round(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test ROUND */
	call_08(ffcrow, f, TDOUBLE, "ROUND(FLOATCOL)", 1, 3, NULL, results, &anynul);
	fail_if(results[0] < 1.9 || results[0] > 2.1);   /* round(1.5) = 2 */
	fail_if(results[1] < 2.9 || results[1] > 3.1);   /* round(2.5) = 3 */

	call_01(ffclos, f);
}

/*
 * Test EXP function
 */
static void
test_ffcrow_exp(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test EXP */
	call_08(ffcrow, f, TDOUBLE, "EXP(0)", 1, 1, NULL, results, &anynul);
	fail_if(results[0] < 0.99 || results[0] > 1.01);   /* exp(0) = 1 */

	call_01(ffclos, f);
}

/*
 * Test ATAN and ATAN2 functions
 */
static void
test_ffcrow_atan(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test ATAN */
	call_08(ffcrow, f, TDOUBLE, "ATAN(0)", 1, 1, NULL, results, &anynul);
	fail_if(results[0] < -0.01 || results[0] > 0.01);   /* atan(0) = 0 */

	/* Test ARCTAN2 (FITS syntax for atan2) */
	call_08(ffcrow, f, TDOUBLE, "ARCTAN2(1, 1)", 1, 1, NULL, results, &anynul);
	/* arctan2(1,1) = pi/4 ~ 0.785 */
	fail_if(results[0] < 0.78 || results[0] > 0.79);

	call_01(ffclos, f);
}

/*
 * Test TAN function
 */
static void
test_ffcrow_tan(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test TAN */
	call_08(ffcrow, f, TDOUBLE, "TAN(0)", 1, 1, NULL, results, &anynul);
	fail_if(results[0] < -0.01 || results[0] > 0.01);   /* tan(0) = 0 */

	call_01(ffclos, f);
}

/*
 * Test ASIN and ACOS functions
 */
static void
test_ffcrow_asin_acos(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test ASIN */
	call_08(ffcrow, f, TDOUBLE, "ASIN(0)", 1, 1, NULL, results, &anynul);
	fail_if(results[0] < -0.01 || results[0] > 0.01);   /* asin(0) = 0 */

	/* Test ACOS */
	call_08(ffcrow, f, TDOUBLE, "ACOS(1)", 1, 1, NULL, results, &anynul);
	fail_if(results[0] < -0.01 || results[0] > 0.01);   /* acos(1) = 0 */

	call_01(ffclos, f);
}

/*
 * Test expression error handling
 */
static void
test_invalid_expression(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* Test invalid expression */
	status = 0;
	fffrow(f, "NONEXISTENT > 5", 1, 10, &n_good_rows, row_status, &status);
	fail_if(status == 0);  /* should fail */

	status = 0;
	call_01(ffclos, f);
}

static void
test_non_boolean_expression(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];

	create_test_table(&f);

	/* fffrow requires boolean expression, not arithmetic */
	status = 0;
	fffrow(f, "INTCOL + 1", 1, 10, &n_good_rows, row_status, &status);
	fail_if(status == 0);  /* should fail with PARSE_BAD_TYPE */

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffsrow copying rows to different file
 */
static void
test_ffsrow_copy_to_different_file(void)
{
	fitsfile *infile, *outfile;
	int status = 0;
	long nrows;
	char *ttype[] = { "INTCOL", "FLOATCOL", "STRCOL", "BOOLCOL" };
	char *tform[] = { "1J", "1E", "10A", "1L" };
	long intdata[10];
	int anynull;

	create_test_table(&infile);
	call_01(ffclos, infile);

	/* Reopen input and create output */
	call_03(ffopen, &infile, test_path, READONLY);
	call_03(ffmahd, infile, 2, NULL);  /* Move to table HDU */
	call_02(ffinit, &outfile, "!test_eval_out.fits");
	call_04(ffphps, outfile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, outfile, BINARY_TBL, 0, 4, ttype, tform, NULL, NULL);

	/* Select rows where INTCOL > 5 and copy to output */
	call_03(ffsrow, infile, outfile, "INTCOL > 5");

	/* Verify output has 5 rows (rows 6-10 from input) */
	call_02(ffgnrw, outfile, &nrows);
	fail_if(nrows != 5);

	/* Verify data was copied correctly */
	call_08(ffgcvj, outfile, 1, 1, 1, 5, 0, intdata, &anynull);
	fail_if(intdata[0] != 6);  /* first row should have value 6 */
	fail_if(intdata[4] != 10); /* last row should have value 10 */

	call_01(ffclos, infile);
	call_01(ffclos, outfile);
	remove("test_eval_out.fits");
}

/*
 * Test ffcrow with TSHORT output type
 */
static void
test_ffcrow_short_output(void)
{
	fitsfile *f;
	int status = 0;
	short results[10];
	int anynul;

	create_test_table(&f);

	/* Test arithmetic expression returning as short */
	call_08(ffcrow, f, TSHORT, "INTCOL * 2", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 2);   /* 1 * 2 = 2 */
	fail_if(results[9] != 20);  /* 10 * 2 = 20 */

	call_01(ffclos, f);
}

/*
 * Test ffcrow with TFLOAT output type
 */
static void
test_ffcrow_float_output(void)
{
	fitsfile *f;
	int status = 0;
	float results[10];
	int anynul;

	create_test_table(&f);

	/* Test arithmetic expression returning as float */
	call_08(ffcrow, f, TFLOAT, "FLOATCOL + 1.0", 1, 10, NULL, results, &anynul);
	fail_if(results[0] < 2.4f || results[0] > 2.6f);   /* 1.5 + 1.0 = 2.5 */
	fail_if(results[9] < 11.4f || results[9] > 11.6f); /* 10.5 + 1.0 = 11.5 */

	call_01(ffclos, f);
}

/*
 * Test ffcrow with TLONG output type
 */
static void
test_ffcrow_long_output(void)
{
	fitsfile *f;
	int status = 0;
	long results[10];
	int anynul;

	create_test_table(&f);

	/* Test arithmetic expression returning as long */
	call_08(ffcrow, f, TLONG, "INTCOL + 100", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 101);  /* 1 + 100 = 101 */
	fail_if(results[9] != 110);  /* 10 + 100 = 110 */

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating column with explicit TFORM
 */
static void
test_ffcalc_with_tform(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynull;

	create_test_table(&f);

	/* Create new column with explicit TFORM */
	call_05(ffcalc, f, "INTCOL * 3.14159", f, "NEWCOL", "1D");

	/* Read back and verify */
	call_08(ffgcvd, f, 5, 1, 1, 10, 0.0, results, &anynull);
	fail_if(results[0] < 3.1 || results[0] > 3.2);   /* 1 * pi ~ 3.14 */
	fail_if(results[9] < 31.4 || results[9] > 31.5); /* 10 * pi ~ 31.4 */

	call_01(ffclos, f);
}

/*
 * Test ffcrow with logical output type
 */
static void
test_ffcrow_logical_output(void)
{
	fitsfile *f;
	int status = 0;
	char results[10];
	int anynul;

	create_test_table(&f);

	/* Test boolean expression returning as logical */
	call_08(ffcrow, f, TLOGICAL, "INTCOL > 5", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 0);  /* 1 > 5 is false */
	fail_if(results[5] != 1);  /* 6 > 5 is true */

	call_01(ffclos, f);
}

/*
 * Test ffcrow with null value handling
 */
static void
test_ffcrow_with_nullval(void)
{
	fitsfile *f;
	int status = 0;
	long results[10];
	long nulval = -999;
	int anynul;

	create_test_table(&f);

	/* Calculate with null handling - no nulls in our data */
	call_08(ffcrow, f, TLONG, "INTCOL * 2", 1, 10, &nulval, results, &anynul);
	fail_if(results[0] != 2);   /* 1 * 2 = 2 */
	fail_if(results[9] != 20);  /* 10 * 2 = 20 */
	fail_if(anynul != 0);  /* No nulls */

	call_01(ffclos, f);
}

/*
 * Test vector expression
 */
static void
test_ffcrow_vector_expression(void)
{
	fitsfile *f;
	int status = 0;
	double results[5];
	int anynul;
	char *ttype[] = { "VECCOL" };
	char *tform[] = { "5D" };
	double vecdata[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };

	/* Create table with vector column */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, vecdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Test expression on vector column */
	call_08(ffcrow, f, TDOUBLE, "VECCOL * 2", 1, 5, NULL, results, &anynul);
	fail_if(results[0] < 1.9 || results[0] > 2.1);   /* 1 * 2 = 2 */
	fail_if(results[4] < 9.9 || results[4] > 10.1); /* 5 * 2 = 10 */

	call_01(ffclos, f);
}

/*
 * Test DEFNULL function
 */
static void
test_ffcrow_defnull(void)
{
	fitsfile *f;
	int status = 0;
	long results[10];
	int anynul;

	create_test_table(&f);

	/* Test DEFNULL - returns replacement value for nulls */
	call_08(ffcrow, f, TLONG, "DEFNULL(INTCOL, 0)", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 1);   /* Not null, so returns original value */
	fail_if(results[9] != 10);

	call_01(ffclos, f);
}

/*
 * Test expression from file (@file syntax)
 */
static void
test_fffrow_expr_from_file(void)
{
	fitsfile *f;
	int status = 0;
	long n_good_rows;
	char row_status[10];
	FILE *exprfile;

	create_test_table(&f);

	/* Create expression file */
	exprfile = fopen("test_expr.txt", "w");
	fail_if(exprfile == NULL);
	fprintf(exprfile, "INTCOL > 5\n");
	fclose(exprfile);

	/* Test using expression from file */
	call_06(fffrow, f, "@test_expr.txt", 1, 10, &n_good_rows, row_status);
	fail_if(n_good_rows != 5);  /* Rows 6,7,8,9,10 match */

	call_01(ffclos, f);
	remove("test_expr.txt");
}

/*
 * Test ffsrow with constant true expression (select all)
 */
static void
test_ffsrow_constant_true(void)
{
	fitsfile *infile, *outfile;
	int status = 0;
	long nrows;
	char outpath[] = "test_ffsrow_const_t.fits";
	char inpath[] = "test_ffsrow_const_t_in.fits";
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long coldata[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	/* Create input file with table */
	remove(inpath);
	call_02(ffinit, &infile, inpath);
	call_04(ffphps, infile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, infile, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, infile, 1, 1, 1, 10, coldata);

	/* Create output file with matching table structure */
	remove(outpath);
	call_02(ffinit, &outfile, outpath);
	call_04(ffphps, outfile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, outfile, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);

	/* Move to table HDU in both files */
	call_03(ffmahd, infile, 2, NULL);
	call_03(ffmahd, outfile, 2, NULL);

	/* Use constant true expression - should copy all rows */
	call_03(ffsrow, infile, outfile, "1==1");

	/* Check that all 10 rows were copied */
	call_02(ffgnrw, outfile, &nrows);
	fail_if(nrows != 10);

	call_01(ffclos, infile);
	call_01(ffclos, outfile);
	remove(inpath);
	remove(outpath);
}

/*
 * Test ffsrow with constant false expression (select none)
 */
static void
test_ffsrow_constant_false(void)
{
	fitsfile *infile, *outfile;
	int status = 0;
	long nrows;
	char outpath[] = "test_ffsrow_const_f.fits";
	char inpath[] = "test_ffsrow_const_f_in.fits";
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long coldata[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	/* Create input file with table */
	remove(inpath);
	call_02(ffinit, &infile, inpath);
	call_04(ffphps, infile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, infile, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, infile, 1, 1, 1, 10, coldata);

	/* Create output file with matching table structure */
	remove(outpath);
	call_02(ffinit, &outfile, outpath);
	call_04(ffphps, outfile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, outfile, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);

	/* Move to table HDU in both files */
	call_03(ffmahd, infile, 2, NULL);
	call_03(ffmahd, outfile, 2, NULL);

	/* Use constant false expression - should copy no rows */
	call_03(ffsrow, infile, outfile, "1==0");

	/* Check that no rows were copied */
	call_02(ffgnrw, outfile, &nrows);
	fail_if(nrows != 0);

	call_01(ffclos, infile);
	call_01(ffclos, outfile);
	remove(inpath);
	remove(outpath);
}

/*
 * Test ffcalc writing constant to keyword
 */
static void
test_ffcalc_keyword_output(void)
{
	fitsfile *f;
	int status = 0;
	double keyval;

	create_test_table(&f);

	/* Use #keyword syntax to put constant result into keyword */
	call_05(ffcalc, f, "3.14159", f, "#MYCONST", NULL);

	/* Verify the keyword was created */
	call_04(ffgkyd, f, "MYCONST", &keyval, NULL);
	fail_if(keyval < 3.14 || keyval > 3.15);

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating logical column
 */
static void
test_ffcalc_logical_column(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	char results[10];
	char nullarr[10];
	int anynul;

	create_test_table(&f);

	/* Create logical column from expression */
	call_05(ffcalc, f, "INTCOL > 5", f, "BIGVAL", "1L");

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);  /* Should now have 5 columns (4 original + 1 new) */

	/* Read the logical column (column 5 is the new one) */
	call_08(ffgcfl, f, 5, 1, 1, 10, results, nullarr, &anynul);
	fail_if(results[0] != 0);  /* 1 > 5 is false */
	fail_if(results[5] != 1);  /* 6 > 5 is true */

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating string column
 */
static void
test_ffcalc_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char *results[1];
	char buffer[80];
	char nullarr[1];
	int anynul;

	create_test_table(&f);
	results[0] = buffer;

	/* Create string column with constant */
	call_05(ffcalc, f, "'test'", f, "STRCOL2", "4A");

	/* Read the string column (column 5 is the new one) */
	call_08(ffgcfs, f, 5, 1, 1, 1, results, nullarr, &anynul);
	fail_if(strcmp(buffer, "test") != 0);

	call_01(ffclos, f);
}

/*
 * Test ffcalc error: #keyword with non-constant expression
 */
static void
test_ffcalc_keyword_nonconstant_error(void)
{
	fitsfile *f;
	int status = 0;

	create_test_table(&f);

	/* Try to put non-constant expression into keyword - should fail */
	status = 0;
	ffcalc(f, "INTCOL * 2", f, "#BADKEY", NULL, &status);
	fail_if(status == 0);  /* Should fail with PARSE_BAD_TYPE */

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffcalc creating BYTE column
 */
static void
test_ffcalc_byte_column(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	unsigned char results[10];
	int anynul;

	create_test_table(&f);

	/* Create byte column from expression */
	call_05(ffcalc, f, "INTCOL", f, "BYTECOL", "1B");

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the byte column */
	call_08(ffgcvb, f, 5, 1, 1, 10, 0, results, &anynul);
	fail_if(results[0] != 1);
	fail_if(results[9] != 10);

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating SHORT column
 */
static void
test_ffcalc_short_column(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	short results[10];
	int anynul;

	create_test_table(&f);

	/* Create short column from expression */
	call_05(ffcalc, f, "INTCOL * 100", f, "SHORTCOL", "1I");

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the short column */
	call_08(ffgcvi, f, 5, 1, 1, 10, 0, results, &anynul);
	fail_if(results[0] != 100);
	fail_if(results[9] != 1000);

	call_01(ffclos, f);
}

/*
 * Test ffsrow with same file (in-place filtering)
 */
static void
test_ffsrow_same_file_filter(void)
{
	fitsfile *f;
	int status = 0;
	long nrows;
	long intdata[5];
	int anynull;
	char *ttype[] = { "INTCOL" };
	char *tform[] = { "1J" };
	long coldata[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	char filepath[] = "test_ffsrow_same.fits";

	/* Create file with table */
	remove(filepath);
	call_02(ffinit, &f, filepath);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 10, coldata);
	call_01(ffclos, f);

	/* Reopen for read/write and filter in place */
	call_03(ffopen, &f, filepath, READWRITE);
	call_03(ffmahd, f, 2, NULL);

	/* Filter to keep only rows where INTCOL <= 5 */
	call_03(ffsrow, f, f, "INTCOL <= 5");

	/* Verify table now has 5 rows */
	call_02(ffgnrw, f, &nrows);
	fail_if(nrows != 5);

	/* Verify correct rows remain */
	call_08(ffgcvj, f, 1, 1, 1, 5, 0, intdata, &anynull);
	fail_if(intdata[0] != 1);
	fail_if(intdata[4] != 5);

	call_01(ffclos, f);
	remove(filepath);
}

/*
 * Test ffcalc with LONGLONG column
 *
 * The multiplier is kept small enough that INTCOL * multiplier stays under
 * 2^31.  The expression evaluator holds integer results in a C long (see lng
 * in eval_defs.h), so the intermediate value is only as wide as the platform's
 * long regardless of the output column type: where long is 32 bits (Windows)
 * a larger expression silently wraps instead of producing a 64-bit result.
 */
static void
test_ffcalc_longlong_column(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	LONGLONG results[10];
	int anynul;

	create_test_table(&f);

	/* Create longlong column from expression */
	call_05(ffcalc, f, "INTCOL * 100000000", f, "BIGCOL", "1K");

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the longlong column using ffgcvjj */
	call_08(ffgcvjj, f, 5, 1, 1, 10, 0, results, &anynul);
	fail_if(results[0] != 100000000LL);
	fail_if(results[9] != 1000000000LL);

	call_01(ffclos, f);
}

/*
 * Test ffcrow with TBYTE output type
 */
static void
test_ffcrow_byte_output(void)
{
	fitsfile *f;
	int status = 0;
	unsigned char results[10];
	int anynul;

	create_test_table(&f);

	/* Test expression returning as byte */
	call_08(ffcrow, f, TBYTE, "INTCOL", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 1);
	fail_if(results[9] != 10);

	call_01(ffclos, f);
}

/*
 * Test ffcrow with TLONGLONG output type
 */
static void
test_ffcrow_longlong_output(void)
{
	fitsfile *f;
	int status = 0;
	LONGLONG results[10];
	int anynul;

	create_test_table(&f);

	/* Test expression returning as longlong.  The multiplier is kept under
	   2^31; see test_ffcalc_longlong_column. */
	call_08(ffcrow, f, TLONGLONG, "INTCOL * 100000000", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 100000000LL);
	fail_if(results[9] != 1000000000LL);

	call_01(ffclos, f);
}

/*
 * Test ffcrow with TINT output type
 */
static void
test_ffcrow_int_output(void)
{
	fitsfile *f;
	int status = 0;
	int results[10];
	int anynul;

	create_test_table(&f);

	/* Test expression returning as int */
	call_08(ffcrow, f, TINT, "INTCOL + 1000", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 1001);
	fail_if(results[9] != 1010);

	call_01(ffclos, f);
}

/*
 * Test ffcrow with TDOUBLE output type
 */
static void
test_ffcrow_double_output(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_test_table(&f);

	/* Test expression returning as double */
	call_08(ffcrow, f, TDOUBLE, "FLOATCOL * 2.5", 1, 10, NULL, results, &anynul);
	fail_if(results[0] < 3.7 || results[0] > 3.8);   /* 1.5 * 2.5 = 3.75 */
	fail_if(results[9] < 26.2 || results[9] > 26.3); /* 10.5 * 2.5 = 26.25 */

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating INT column
 */
static void
test_ffcalc_int_column(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	int results[10];
	int anynul;

	create_test_table(&f);

	/* Create int column from expression using explicit repeat */
	call_05(ffcalc, f, "INTCOL + 500", f, "INTCOL2", "J");

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the int column */
	call_08(ffgcvk, f, 5, 1, 1, 10, 0, results, &anynul);
	fail_if(results[0] != 501);
	fail_if(results[9] != 510);

	call_01(ffclos, f);
}

/*
 * Test ffcalc writing integer constant to keyword
 */
static void
test_ffcalc_keyword_int(void)
{
	fitsfile *f;
	int status = 0;
	long keyval;

	create_test_table(&f);

	/* Write integer constant to keyword */
	call_05(ffcalc, f, "42", f, "#INTKEY", NULL);

	/* Verify the keyword was created */
	call_04(ffgkyj, f, "INTKEY", &keyval, NULL);
	fail_if(keyval != 42);

	call_01(ffclos, f);
}

/*
 * Test ffcalc writing logical constant to keyword
 */
static void
test_ffcalc_keyword_logical(void)
{
	fitsfile *f;
	int status = 0;
	int keyval;

	create_test_table(&f);

	/* Write logical constant to keyword */
	call_05(ffcalc, f, "1==1", f, "#LOGKEY", NULL);

	/* Verify the keyword was created */
	call_04(ffgkyl, f, "LOGKEY", &keyval, NULL);
	fail_if(keyval != 1);

	call_01(ffclos, f);
}

/*
 * Test ffcalc writing string constant to keyword
 */
static void
test_ffcalc_keyword_string(void)
{
	fitsfile *f;
	int status = 0;
	char keyval[80];

	create_test_table(&f);

	/* Write string constant to keyword */
	call_05(ffcalc, f, "'hello'", f, "#STRKEY", NULL);

	/* Verify the keyword was created */
	call_04(ffgkys, f, "STRKEY", keyval, NULL);
	fail_if(strcmp(keyval, "hello") != 0);

	call_01(ffclos, f);
}

/*
 * Test ffcalc writing to HISTORY keyword
 */
static void
test_ffcalc_keyword_history(void)
{
	fitsfile *f;
	int status = 0;

	create_test_table(&f);

	/* Write to HISTORY keyword */
	call_05(ffcalc, f, "'Test history entry'", f, "#HISTORY", NULL);

	/* Just verify no error - HISTORY is special */
	call_01(ffclos, f);
}

/*
 * Test ffcalc writing to COMMENT keyword
 */
static void
test_ffcalc_keyword_comment(void)
{
	fitsfile *f;
	int status = 0;

	create_test_table(&f);

	/* Write to COMMENT keyword */
	call_05(ffcalc, f, "'Test comment entry'", f, "#COMMENT", NULL);

	/* Just verify no error - COMMENT is special */
	call_01(ffclos, f);
}

/*
 * Test ffcalc error: HISTORY with non-string value
 */
static void
test_ffcalc_history_nonstring_error(void)
{
	fitsfile *f;
	int status = 0;

	create_test_table(&f);

	/* Try to write non-string to HISTORY - should fail */
	status = 0;
	ffcalc(f, "42", f, "#HISTORY", NULL, &status);
	fail_if(status == 0);  /* Should fail with PARSE_BAD_TYPE */

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffcalc creating logical column without TFORM
 */
static void
test_ffcalc_logical_no_tform(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	char results[10];
	char nullarr[10];
	int anynul;

	create_test_table(&f);

	/* Create logical column from expression without TFORM */
	call_05(ffcalc, f, "INTCOL > 5", f, "LOGCOL2", NULL);

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the logical column */
	call_08(ffgcfl, f, 5, 1, 1, 10, results, nullarr, &anynul);
	fail_if(results[0] != 0);  /* 1 > 5 is false */
	fail_if(results[5] != 1);  /* 6 > 5 is true */

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating string column without TFORM
 */
static void
test_ffcalc_string_no_tform(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	char *results[1];
	char buffer[80];
	char nullarr[1];
	int anynul;

	create_test_table(&f);
	results[0] = buffer;

	/* Create string column from expression without TFORM */
	call_05(ffcalc, f, "'test'", f, "STRCOL3", NULL);

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the string column */
	call_08(ffgcfs, f, 5, 1, 1, 1, results, nullarr, &anynul);
	fail_if(strcmp(buffer, "test") != 0);

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating double column without TFORM
 */
static void
test_ffcalc_double_no_tform(void)
{
	fitsfile *f;
	int status = 0;
	int ncols;
	double results[10];
	int anynull;

	create_test_table(&f);

	/* Create double column from expression without TFORM */
	call_05(ffcalc, f, "FLOATCOL * 2.5", f, "DBLCOL", NULL);

	/* Verify column was created */
	call_02(ffgncl, f, &ncols);
	fail_if(ncols != 5);

	/* Read the double column */
	call_08(ffgcvd, f, 5, 1, 1, 10, 0.0, results, &anynull);
	fail_if(results[0] < 3.7 || results[0] > 3.8);   /* 1.5 * 2.5 = 3.75 */

	call_01(ffclos, f);
}

/*
 * Test ffcalc_rng with specific row ranges
 */
static void
test_ffcalc_rng_multiple_ranges(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynull;
	long start[2] = { 1, 8 };
	long end[2] = { 3, 10 };

	create_test_table(&f);

	/* Calculate for rows 1-3 and 8-10 */
	call_08(ffcalc_rng, f, "INTCOL * 5", f, "RANGED", NULL, 2, start, end);

	/* Read back to verify - only rows 1-3 and 8-10 should have values */
	call_08(ffgcvd, f, 5, 1, 1, 3, 0.0, results, &anynull);
	fail_if(results[0] < 4.9 || results[0] > 5.1);   /* 1 * 5 = 5 */
	fail_if(results[2] < 14.9 || results[2] > 15.1); /* 3 * 5 = 15 */

	call_01(ffclos, f);
}

/*
 * Create an ASCII table for testing ASCII-specific code paths
 */
static void
create_ascii_table(fitsfile **fptr)
{
	int status = 0;
	char *ttype[] = { "INTCOL", "FLOATCOL", "STRCOL" };
	char *tform[] = { "I11", "D23.15", "A10" };
	char *tunit[] = { "", "", "" };
	long intdata[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	double floatdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5 };
	char *strdata[] = {
		"alpha", "beta", "gamma", "delta", "epsilon",
		"zeta", "eta", "theta", "iota", "kappa"
	};
	int i;

	call_02(ffinit, fptr, "!" test_path);
	call_04(ffphps, *fptr, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, *fptr, ASCII_TBL, 10, 3, ttype, tform, tunit, NULL);

	/* Write integer column */
	for (i = 0; i < 10; i += 1) {
		call_06(ffpclj, *fptr, 1, i + 1, 1, 1, &intdata[i]);
	}

	/* Write double column */
	for (i = 0; i < 10; i += 1) {
		call_06(ffpcld, *fptr, 2, i + 1, 1, 1, &floatdata[i]);
	}

	/* Write string column */
	for (i = 0; i < 10; i += 1) {
		call_06(ffpcls, *fptr, 3, i + 1, 1, 1, &strdata[i]);
	}
}

/*
 * Test ffcalc creating integer column in ASCII table
 */
static void
test_ffcalc_ascii_int_column(void)
{
	fitsfile *f;
	int status = 0;
	long results[10];
	int anynul;

	create_ascii_table(&f);

	/* Create new integer column in ASCII table */
	call_05(ffcalc, f, "INTCOL * 2", f, "DOUBLED", NULL);

	/* Read back results */
	call_08(ffgcvj, f, 4, 1, 1, 10, 0, results, &anynul);
	fail_if(results[0] != 2);
	fail_if(results[4] != 10);
	fail_if(results[9] != 20);

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating double column in ASCII table
 */
static void
test_ffcalc_ascii_double_column(void)
{
	fitsfile *f;
	int status = 0;
	double results[10];
	int anynul;

	create_ascii_table(&f);

	/* Create new double column in ASCII table */
	call_05(ffcalc, f, "FLOATCOL + 0.5", f, "SHIFTED", NULL);

	/* Read back results */
	call_08(ffgcvd, f, 4, 1, 1, 10, 0.0, results, &anynul);
	fail_if(results[0] < 1.9 || results[0] > 2.1);  /* 1.5 + 0.5 = 2.0 */
	fail_if(results[4] < 5.9 || results[4] > 6.1);  /* 5.5 + 0.5 = 6.0 */

	call_01(ffclos, f);
}

/*
 * Test ffcalc creating string column in ASCII table
 */
static void
test_ffcalc_ascii_string_column(void)
{
	fitsfile *f;
	int status = 0;
	char strresult[20];
	char *strptr = strresult;
	int anynull;

	create_ascii_table(&f);

	/* Create new string column in ASCII table */
	call_05(ffcalc, f, "'test'", f, "NEWSTR", NULL);

	/* Read back first result */
	call_08(ffgcvs, f, 4, 1, 1, 1, NULL, &strptr, &anynull);
	fail_if(strncmp(strresult, "test", 4) != 0);

	call_01(ffclos, f);
}

/*
 * Test ffcalc attempting to create logical column in ASCII table
 * (should fail with NOT_BTABLE error)
 */
static void
test_ffcalc_ascii_logical_error(void)
{
	fitsfile *f;
	int status = 0;

	create_ascii_table(&f);

	/* Try to create logical column - should fail */
	fits_calculator(f, "INTCOL > 5", f, "LOGCOL", NULL, &status);
	fail_if(status != NOT_BTABLE);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffsrow copying to different file with variable-length columns
 */
static void
test_ffsrow_varlen_copy(void)
{
	fitsfile *infile, *outfile;
	int status = 0;
	char *ttype[] = { "INTCOL", "VARCOL" };
	char *tform[] = { "1J", "1PJ" };
	long intdata[] = { 1, 2, 3, 4, 5 };
	long vardata[5] = { 10, 20, 30, 40, 50 };
	int i;
	long nrows;

	/* Create input file with variable-length column */
	call_02(ffinit, &infile, "!" test_path);
	call_04(ffphps, infile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, infile, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);

	/* Write data */
	for (i = 0; i < 5; i += 1) {
		call_06(ffpclj, infile, 1, i + 1, 1, 1, &intdata[i]);
		call_07(ffpcl, infile, TLONG, 2, i + 1, 1, 1, &vardata[i]);
	}

	/* Create output file */
	call_02(ffinit, &outfile, "!test_eval_out.fits");
	call_04(ffphps, outfile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, outfile, BINARY_TBL, 0, 2, ttype, tform, NULL, NULL);

	/* Copy rows where INTCOL > 2 to different file */
	fits_select_rows(infile, outfile, "INTCOL > 2", &status);
	fail_if(status != 0);

	/* Verify output has 3 rows */
	call_02(ffgnrw, outfile, &nrows);
	fail_if(nrows != 3);

	call_01(ffclos, infile);
	call_01(ffclos, outfile);
	remove("test_eval_out.fits");
}

/*
 * Test ffsrow with non-boolean expression error
 */
static void
test_ffsrow_nonboolean_error(void)
{
	fitsfile *f;
	int status = 0;

	create_test_table(&f);

	/* Try to use arithmetic expression (non-boolean) - should fail */
	fits_select_rows(f, f, "INTCOL + 5", &status);
	fail_if(status != PARSE_BAD_TYPE);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test ffcalc with bitstring expression (TBIT column type)
 */
static void
test_ffcalc_bit_column(void)
{
	fitsfile *f;
	int status = 0;

	create_test_table(&f);

	/* Create column with bitstring expression - b prefix creates BITSTR type */
	call_05(ffcalc, f, "b11110000", f, "BITCOL", NULL);

	/* Verify column was created */
	{
		int colnum;
		call_04(ffgcno, f, CASEINSEN, "BITCOL", &colnum);
		fail_if(colnum != 5);
	}

	call_01(ffclos, f);
}

/*
 * Test ffcalc with TLONGLONG column format
 */
static void
test_ffcalc_longlong_no_tform(void)
{
	fitsfile *f;
	int status = 0;
	LONGLONG results[10];
	int anynul;

	create_test_table(&f);

	/* Create longlong column with explicit format (K = LONGLONG).  The
	   multiplier is kept under 2^31; see test_ffcalc_longlong_column. */
	call_05(ffcalc, f, "INTCOL * 100000000", f, "BIGNUM", "1K");

	/* Read back results */
	call_08(ffgcvjj, f, 5, 1, 1, 10, 0, results, &anynul);
	fail_if(results[0] != 100000000LL);
	fail_if(results[4] != 500000000LL);

	call_01(ffclos, f);
}

/*
 * Test bitwise operations
 */
static void
test_ffcrow_bitwise(void)
{
	fitsfile *f;
	int status = 0;
	long results[10];
	int anynul;

	create_test_table(&f);

	/* Test bitwise AND */
	call_08(ffcrow, f, TLONG, "INTCOL & 1", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 1);  /* 1 & 1 = 1 */
	fail_if(results[1] != 0);  /* 2 & 1 = 0 */

	/* Test bitwise OR */
	call_08(ffcrow, f, TLONG, "INTCOL | 16", 1, 10, NULL, results, &anynul);
	fail_if(results[0] != 17);  /* 1 | 16 = 17 */
	fail_if(results[1] != 18);  /* 2 | 16 = 18 */

	call_01(ffclos, f);
}

/*
 * Test ffsrow copying to different file with actual heap data
 */
static void
test_ffsrow_varlen_heap_copy(void)
{
	fitsfile *infile, *outfile;
	int status = 0;
	char *ttype[] = { "INTCOL", "VARCOL" };
	char *tform[] = { "1J", "PJ(100)" };
	long intdata[] = { 1, 2, 3, 4, 5 };
	long vardata[100];
	int i, j;
	long nrows;

	/* Initialize vardata */
	for (j = 0; j < 100; j += 1) {
		vardata[j] = j * 10;
	}

	/* Create input file with variable-length column containing heap data */
	call_02(ffinit, &infile, "!" test_path);
	call_04(ffphps, infile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, infile, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);

	/* Write data - variable column with 100 elements goes to heap */
	for (i = 0; i < 5; i += 1) {
		call_06(ffpclj, infile, 1, i + 1, 1, 1, &intdata[i]);
		call_07(ffpcl, infile, TLONG, 2, i + 1, 1, 100, vardata);
	}

	/* Create output file */
	call_02(ffinit, &outfile, "!test_eval_heap.fits");
	call_04(ffphps, outfile, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, outfile, BINARY_TBL, 0, 2, ttype, tform, NULL, NULL);

	/* Copy rows where INTCOL > 2 to different file */
	fits_select_rows(infile, outfile, "INTCOL > 2", &status);
	fail_if(status != 0);

	/* Verify output has 3 rows */
	call_02(ffgnrw, outfile, &nrows);
	fail_if(nrows != 3);

	call_01(ffclos, infile);
	call_01(ffclos, outfile);
	remove("test_eval_heap.fits");
}

/*
 * Test ffcalc creating multi-dimensional column
 */
static void
test_ffcalc_multidim_column(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "MATRIX" };
	char *tform[] = { "9J" };
	long matdata[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	long naxes[2] = { 3, 3 };
	long results[9];
	int anynul;

	/* Create file with 3x3 matrix column */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);

	/* Set dimension of column */
	call_04(ffptdm, f, 1, 2, naxes);

	/* Write matrix data */
	call_06(ffpclj, f, 1, 1, 1, 9, matdata);

	/* Create new column from expression with TDIM */
	call_05(ffcalc, f, "MATRIX * 2", f, "DOUBLED", "9J");

	/* Read back results */
	call_08(ffgcvj, f, 2, 1, 1, 9, 0, results, &anynul);
	fail_if(results[0] != 2);
	fail_if(results[8] != 18);

	call_01(ffclos, f);
}

/*
 * Test integer division overflow to exercise the LONG_MIN / -1 guard
 * in Do_BinOp_lng.  On platforms where sizeof(long)==4, INT_MIN ==
 * LONG_MIN and dividing by -1 would overflow; the fix clamps the
 * result to LONG_MAX.  On 64-bit platforms, INT_MIN / -1 fits in
 * a long without overflow, but the division path is still exercised.
 */
static void
test_ffcrow_long_div_overflow(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "MINCOL", "NEGCOL" };
	char *tform[] = { "1J", "1J" };
	long minval = INT_MIN;
	long negone = -1;
	long div_result[1];
	long mod_result[1];
	int anynul;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 2, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, &minval);
	call_06(ffpclj, f, 2, 1, 1, 1, &negone);

	/* Division: INT_MIN / -1. */
	call_08(ffcrow, f, TLONG, "MINCOL / NEGCOL",
		1, 1, NULL, div_result, &anynul);
	if (sizeof div_result[0] == 4) {
		fail_if(div_result[0] != LONG_MAX);
	} else {
		fail_if(div_result[0] != (long)-(long long)INT_MIN);
	}

	/* Modulo: INT_MIN % -1. */
	call_08(ffcrow, f, TLONG, "MINCOL % NEGCOL",
		1, 1, NULL, mod_result, &anynul);
	fail_if(mod_result[0] != 0);

	call_01(ffclos, f);
}

/*
 * Test integer division by zero to exercise the undef path
 * in Do_BinOp_lng when the divisor column contains zero.
 */
static void
test_ffcrow_long_div_by_zero(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "NUMCOL", "DENCOL" };
	char *tform[] = { "1J", "1J" };
	long num = 42;
	long den = 0;
	long div_result[1];
	long mod_result[1];
	int anynul;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 2, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 1, &num);
	call_06(ffpclj, f, 2, 1, 1, 1, &den);

	/* Division by zero marks result as undefined. */
	anynul = 0;
	call_08(ffcrow, f, TLONG, "NUMCOL / DENCOL",
		1, 1, NULL, div_result, &anynul);
	fail_if(anynul != 1);

	/* Modulo by zero marks result as undefined. */
	anynul = 0;
	call_08(ffcrow, f, TLONG, "NUMCOL % DENCOL",
		1, 1, NULL, mod_result, &anynul);
	fail_if(anynul != 1);

	call_01(ffclos, f);
}

/*
 * Test vector column deref with out-of-range index to exercise
 * the error path in Do_Deref where the double-free fix was applied.
 */
static void
test_ffcrow_vector_index_out_of_range(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECCOL" };
	char *tform[] = { "5J" };
	long vecdata[5] = { 10, 20, 30, 40, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, vecdata);

	/* Index 99 exceeds vector length of 5. */
	fits_calculator(f, "VECCOL[99]", f, "BADCOL", NULL, &status);
	fail_if(status == 0);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test vector column deref with index zero to exercise the
 * out-of-range path (FITS indices are 1-based).
 */
static void
test_ffcrow_vector_index_zero(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VECCOL" };
	char *tform[] = { "5J" };
	long vecdata[5] = { 10, 20, 30, 40, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, vecdata);

	/* Index 0 is invalid for 1-based FITS vectors. */
	fits_calculator(f, "VECCOL[0]", f, "BADCOL", NULL, &status);
	fail_if(status == 0);

	status = 0;
	call_01(ffclos, f);
}

/*
 * Test double-precision column arithmetic to exercise the
 * validate_double_vector guard in Do_BinOp_dbl.
 */
static void
test_ffcrow_double_column_arithmetic(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "DCOL1", "DCOL2" };
	char *tform[] = { "1D", "1D" };
	double v1 = 1.0e308;
	double v2 = 2.0;
	double result[1];
	int anynul;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 1, &v1);
	call_06(ffpcld, f, 2, 1, 1, 1, &v2);

	call_08(ffcrow, f, TDOUBLE, "DCOL1 / DCOL2",
		1, 1, NULL, result, &anynul);
	fail_if(result[0] < 4.9e307 || result[0] > 5.1e307);

	call_01(ffclos, f);
}

int
main(void)
{
	/* fffrow tests */
	test_fffrow_basic();
	test_fffrow_float_comparison();
	test_fffrow_logical_column();
	test_fffrow_compound_expression();
	test_fffrow_constant_true();
	test_fffrow_constant_false();
	test_fffrow_or_expression();
	test_fffrow_not_expression();
	test_fffrow_modulo();
	test_fffrow_between();
	test_fffrow_not_equal();

	/* fftexp tests */
	test_fftexp_integer();
	test_fftexp_float();
	test_fftexp_arithmetic();
	test_fftexp_boolean();

	/* ffcrow tests */
	test_ffcrow_arithmetic();
	test_ffcrow_add_columns();
	test_ffcrow_math_functions();
	test_ffcrow_min_max();
	test_ffcrow_sqrt();
	test_ffcrow_power();
	test_ffcrow_log();
	test_ffcrow_trig();
	test_ffcrow_conditional();
	test_ffcrow_division();
	test_ffcrow_subtraction();
	test_ffcrow_row_number();

	/* ffcalc tests */
	test_ffcalc_new_column();
	test_ffcalc_overwrite_column();

	/* ffsrow tests - disabled, segfaults */
	/* test_ffsrow_copy_selected(); */
	/* test_ffsrow_same_file(); */

	/* ffcalc_rng tests */
	test_ffcalc_rng_basic();

	/* String operation tests */
	test_ffcrow_string_comparison();

	/* Additional math function tests */
	test_ffcrow_negation();
	test_ffcrow_floor_ceil();
	test_ffcrow_round();
	test_ffcrow_exp();
	test_ffcrow_atan();
	test_ffcrow_tan();
	test_ffcrow_asin_acos();

	/* Error handling tests */
	test_invalid_expression();
	test_non_boolean_expression();

	/* Additional output type tests */
	test_ffcrow_short_output();
	test_ffcrow_float_output();
	test_ffcrow_long_output();
	test_ffcrow_logical_output();

	/* ffsrow tests */
	test_ffsrow_copy_to_different_file();

	/* ffcalc tests */
	test_ffcalc_with_tform();
	test_ffcalc_keyword_output();
	test_ffcalc_logical_column();
	test_ffcalc_string_column();
	test_ffcalc_keyword_nonconstant_error();
	test_ffcalc_byte_column();
	test_ffcalc_short_column();
	test_ffcalc_longlong_column();

	/* ffsrow same file tests */
	test_ffsrow_same_file_filter();

	/* Additional output type tests */
	test_ffcrow_byte_output();
	test_ffcrow_longlong_output();
	test_ffcrow_int_output();
	test_ffcrow_double_output();

	/* Additional ffcalc tests */
	test_ffcalc_int_column();
	test_ffcalc_keyword_int();
	test_ffcalc_keyword_logical();
	test_ffcalc_keyword_string();
	test_ffcalc_keyword_history();
	test_ffcalc_keyword_comment();
	test_ffcalc_history_nonstring_error();
	test_ffcalc_logical_no_tform();
	test_ffcalc_string_no_tform();
	test_ffcalc_double_no_tform();
	test_ffcalc_rng_multiple_ranges();

	/* Null handling tests */
	test_ffcrow_with_nullval();
	test_ffcrow_defnull();

	/* Vector expression tests */
	test_ffcrow_vector_expression();

	/* Bitwise operation tests */
	test_ffcrow_bitwise();

	/* Expression from file tests */
	test_fffrow_expr_from_file();

	/* ffsrow constant expression tests */
	test_ffsrow_constant_true();
	test_ffsrow_constant_false();

	/* ASCII table tests */
	test_ffcalc_ascii_int_column();
	test_ffcalc_ascii_double_column();
	test_ffcalc_ascii_string_column();
	test_ffcalc_ascii_logical_error();

	/* Variable-length column copy tests */
	test_ffsrow_varlen_copy();

	/* Error handling tests */
	test_ffsrow_nonboolean_error();

	/* Additional column format tests */
	test_ffcalc_bit_column();
	test_ffcalc_longlong_no_tform();

	/* Heap copy tests */
	test_ffsrow_varlen_heap_copy();

	/* Multi-dimensional column tests */
	test_ffcalc_multidim_column();

	/* Integer overflow and division-by-zero tests */
	test_ffcrow_long_div_overflow();
	test_ffcrow_long_div_by_zero();

	/* Vector index out-of-range tests */
	test_ffcrow_vector_index_out_of_range();
	test_ffcrow_vector_index_zero();

	/* Double-precision column arithmetic */
	test_ffcrow_double_column_arithmetic();

	remove(test_path);

	return 0;
}
