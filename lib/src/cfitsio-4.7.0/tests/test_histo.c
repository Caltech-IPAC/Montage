/*
 * Tests for histo.c - histogram functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_histo.fits"
#define hist_path "test_histo_out.fits"

/* Maximum number of histogram axes supported by ffbins/ffhist functions. */
#define MAXHISTAXES 4

static void
test_parse_binspec_1d(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (X)";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 1);
	fail_if(strcmp(colname[0], "X") != 0);
}

static void
test_parse_binspec_2d(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (X, Y)";  /* Need "bin" prefix. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 2);
	fail_if(strcmp(colname[0], "X") != 0);
	fail_if(strcmp(colname[1], "Y") != 0);
}

static void
test_parse_binspec_with_range(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:100:10";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 1);
	fail_if(strcmp(colname[0], "X") != 0);
	fail_if(minin[0] != 0.0);
	fail_if(maxin[0] != 100.0);
	fail_if(binsizein[0] != 10.0);
}

static void
test_parse_binspec_with_binsize_only(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin 4";  /* Just a binsize with "bin" prefix. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 2);  /* Default is 2D with X, Y. */
	fail_if(binsizein[0] != 4.0);
	fail_if(binsizein[1] != 4.0);
}

static void
test_parse_binspec_with_weight(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:10;W";  /* Weight column after ";". */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 1);
	fail_if(strcmp(wtname, "W") != 0);
}

static void
test_parse_binspec_imagetype(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bind (X)";  /* "bind" for double precision. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TDOUBLE);
}

static void
test_parse_binr_simple(void)
{
	int status = 0;
	char *ptr;
	char binspec[] = "XCOL=10:20:2";
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(strcmp(colname, "XCOL") != 0);
	fail_if(minin != 10.0);
	fail_if(maxin != 20.0);
	fail_if(binsizein != 2.0);
}

static void
test_parse_binr_partial(void)
{
	int status = 0;
	char *ptr;
	char binspec[] = "COL=:50:5";  /* No min specified. */
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	minin = -999.0;  /* Initialize to detect if it's changed. */
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(strcmp(colname, "COL") != 0);
	fail_if(minin != -999.0);  /* Should be unchanged. */
	fail_if(maxin != 50.0);
	fail_if(binsizein != 5.0);
}

static void
test_create_1d_histogram(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = {
		1.0, 2.0, 3.0, 4.0, 5.0,
		1.0, 2.0, 3.0, 4.0, 5.0
	};  /* 10 values. */
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table with X column. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 10, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 10, xdata);
	call_01(ffclos, f);

	/* Open table and create histogram. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 6.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify histogram dimensions. */
	{
		int naxis;
		long naxes[MAXHISTAXES];
		call_02(ffgidm, histptr, &naxis);
		fail_if(naxis != 1);
		call_03(ffgisz, histptr, 1, naxes);
		fail_if(naxes[0] != 6);  /* 6 bins: 0-1, 1-2, 2-3, 3-4, 4-5, 5-6. */
	}

	/* Read histogram values. */
	{
		long hist[6];
		int anynull;
		call_07(ffgpvj, histptr, 1, 1, 6, 0, hist, &anynull);
		/* Values 1,2,3,4,5 each occur twice. */
		fail_if(hist[0] != 0);  /* 0-1: nothing. */
		fail_if(hist[1] != 2);  /* 1-2: two 1.0 values. */
		fail_if(hist[2] != 2);  /* 2-3: two 2.0 values. */
		fail_if(hist[3] != 2);  /* 3-4: two 3.0 values. */
		fail_if(hist[4] != 2);  /* 4-5: two 4.0 values. */
		fail_if(hist[5] != 2);  /* 5-6: two 5.0 values. */
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_2d_histogram(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X", "Y" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 0.5, 1.5, 0.5, 1.5 };
	double ydata[] = { 0.5, 0.5, 1.5, 1.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table with X, Y columns. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 4, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 4, xdata);
	call_06(ffpcld, f, 2, 1, 1, 4, ydata);
	call_01(ffclos, f);

	/* Open table and create 2D histogram. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	strcpy(colname[1], "Y");
	for (i = 2; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 2.0;
	binsizein[0] = 1.0;
	minin[1] = 0.0;
	maxin[1] = 2.0;
	binsizein[1] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 2, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify histogram dimensions. */
	{
		int naxis;
		long naxes[MAXHISTAXES];
		call_02(ffgidm, histptr, &naxis);
		fail_if(naxis != 2);
		call_03(ffgisz, histptr, 2, naxes);
		fail_if(naxes[0] != 2);
		fail_if(naxes[1] != 2);
	}

	/* Read histogram values - 2x2 image. */
	{
		long hist[4];
		int anynull;
		call_07(ffgpvj, histptr, 1, 1, 4, 0, hist, &anynull);
		/* Each corner has one point. */
		fail_if(hist[0] != 1);  /* (0,0) */
		fail_if(hist[1] != 1);  /* (1,0) */
		fail_if(hist[2] != 1);  /* (0,1) */
		fail_if(hist[3] != 1);  /* (1,1) */
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_histogram_with_weight(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X", "W" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 1.0, 2.0, 3.0 };
	double wdata[] = { 2.0, 3.0, 5.0 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	char wtcol[FLEN_VALUE];
	int i;

	/* Create table with X and weight columns. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_06(ffpcld, f, 2, 1, 1, 3, wdata);
	call_01(ffclos, f);

	/* Open table and create weighted histogram. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 4.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}
	strcpy(wtcol, "W");

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		0.0, wtcol, 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Read histogram values. */
	{
		long hist[4];
		int anynull;
		call_07(ffgpvj, histptr, 1, 1, 4, 0, hist, &anynull);
		/* Weighted counts: bin 1 has 2.0, bin 2 has 3.0, bin 3 has 5.0. */
		fail_if(hist[0] != 0);
		fail_if(hist[1] != 2);
		fail_if(hist[2] != 3);
		fail_if(hist[3] != 5);
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_histogram_float_output(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_01(ffclos, f);

	/* Create histogram with float output. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 5.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TFLOAT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify it's a float image. */
	{
		int bitpix;
		call_02(ffgiet, histptr, &bitpix);
		fail_if(bitpix != FLOAT_IMG);
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_histogram_double_output(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_01(ffclos, f);

	/* Create histogram with double output. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 5.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TDOUBLE, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify it's a double image. */
	{
		int bitpix;
		call_02(ffgiet, histptr, &bitpix);
		fail_if(bitpix != DOUBLE_IMG);
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_histogram_short_output(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_01(ffclos, f);

	/* Create histogram with short output. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 5.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TSHORT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify it's a short image. */
	{
		int bitpix;
		call_02(ffgiet, histptr, &bitpix);
		fail_if(bitpix != SHORT_IMG);
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_histogram_byte_output(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_01(ffclos, f);

	/* Create histogram with byte output. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 5.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TBYTE, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify it's a byte image. */
	{
		int bitpix;
		call_02(ffgiet, histptr, &bitpix);
		fail_if(bitpix != BYTE_IMG);
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_bad_datatype(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 1, xdata);
	call_01(ffclos, f);

	/* Try to create histogram with invalid output type. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 5.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	/* TSTRING is not a valid histogram output type. */
	histptr = ffhist3(
		f, "!" hist_path, TSTRING, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != BAD_DATATYPE);
	fail_if(histptr != NULL);

	status = 0;
	call_01(ffclos, f);
}

static void
test_histogram_too_many_axes(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "A", "B", "C", "D", "E" };
	char *tform[] = { "1D", "1D", "1D", "1D", "1D" };
	double data[] = { 1.0 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table with 5 columns. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 5, ttype, tform, NULL, NULL);
	for (i = 0; i < 5; i += 1) {
		call_06(ffpcld, f, i + 1, 1, 1, 1, data);
	}
	call_01(ffclos, f);

	/* Try to create 5D histogram (max is 4). */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "A");
	strcpy(colname[1], "B");
	strcpy(colname[2], "C");
	strcpy(colname[3], "D");
	/* Can't specify 5th axis in the colname array. */
	for (i = 0; i < 4; i += 1) {
		minin[i] = 0.0;
		maxin[i] = 5.0;
		binsizein[i] = 1.0;
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	/* Request 5 axes - should fail. */
	histptr = ffhist3(
		f, "!" hist_path, TINT, 5, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != BAD_DIMEN);
	fail_if(histptr != NULL);

	status = 0;
	call_01(ffclos, f);
}

static void
test_create_3d_histogram(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X", "Y", "Z" };
	char *tform[] = { "1D", "1D", "1D" };
	double xdata[] = { 0.5, 1.5, 0.5, 1.5, 0.5, 1.5, 0.5, 1.5 };
	double ydata[] = { 0.5, 0.5, 1.5, 1.5, 0.5, 0.5, 1.5, 1.5 };
	double zdata[] = { 0.5, 0.5, 0.5, 0.5, 1.5, 1.5, 1.5, 1.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table with X, Y, Z columns. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 8, 3, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 8, xdata);
	call_06(ffpcld, f, 2, 1, 1, 8, ydata);
	call_06(ffpcld, f, 3, 1, 1, 8, zdata);
	call_01(ffclos, f);

	/* Open table and create 3D histogram. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	strcpy(colname[1], "Y");
	strcpy(colname[2], "Z");
	colname[3][0] = '\0';
	for (i = 0; i < 3; i += 1) {
		minin[i] = 0.0;
		maxin[i] = 2.0;
		binsizein[i] = 1.0;
	}
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 3, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify histogram dimensions. */
	{
		int naxis;
		long naxes[MAXHISTAXES];
		call_02(ffgidm, histptr, &naxis);
		fail_if(naxis != 3);
		call_03(ffgisz, histptr, 3, naxes);
		fail_if(naxes[0] != 2);
		fail_if(naxes[1] != 2);
		fail_if(naxes[2] != 2);
	}

	/* Read histogram - 2x2x2 = 8 values, one in each bin. */
	{
		long hist[8];
		int anynull;
		call_07(ffgpvj, histptr, 1, 1, 8, 0, hist, &anynull);
		for (i = 0; i < 8; i += 1) {
			fail_if(hist[i] != 1);
		}
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_create_4d_histogram(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "A", "B", "C", "D" };
	char *tform[] = { "1D", "1D", "1D", "1D" };
	double data[16];
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i, j;

	/* Create 16 rows with all 2^4 combinations of 0.5 and 1.5. */
	for (i = 0; i < 16; i += 1) {
		data[i] = ((i >> 0) & 1) ? 1.5 : 0.5;
	}

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 16, 4, ttype, tform, NULL, NULL);

	/* Fill each column with appropriate pattern. */
	for (j = 0; j < 4; j += 1) {
		for (i = 0; i < 16; i += 1) {
			data[i] = ((i >> j) & 1) ? 1.5 : 0.5;
		}
		call_06(ffpcld, f, j + 1, 1, 1, 16, data);
	}
	call_01(ffclos, f);

	/* Create 4D histogram. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "A");
	strcpy(colname[1], "B");
	strcpy(colname[2], "C");
	strcpy(colname[3], "D");
	for (i = 0; i < 4; i += 1) {
		minin[i] = 0.0;
		maxin[i] = 2.0;
		binsizein[i] = 1.0;
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 4, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Verify histogram dimensions. */
	{
		int naxis;
		long naxes[MAXHISTAXES];
		call_02(ffgidm, histptr, &naxis);
		fail_if(naxis != 4);
		call_03(ffgisz, histptr, 4, naxes);
		for (i = 0; i < 4; i += 1) {
			fail_if(naxes[i] != 2);
		}
	}

	/* Read histogram - 2^4 = 16 values, one in each bin. */
	{
		long hist[16];
		int anynull;
		call_07(ffgpvj, histptr, 1, 1, 16, 0, hist, &anynull);
		for (i = 0; i < 16; i += 1) {
			fail_if(hist[i] != 1);
		}
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_with_constant_weight(void)
{
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_01(ffclos, f);

	/* Create histogram with constant weight of 5.0. */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 5.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		5.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	/* Read histogram values. */
	{
		long hist[5];
		int anynull;
		call_07(ffgpvj, histptr, 1, 1, 5, 0, hist, &anynull);
		/* Each bin with data has count * 5. */
		fail_if(hist[0] != 0);
		fail_if(hist[1] != 5);  /* 1.5 -> bin 1. */
		fail_if(hist[2] != 5);  /* 2.5 -> bin 2. */
		fail_if(hist[3] != 5);  /* 3.5 -> bin 3. */
		fail_if(hist[4] != 0);
	}

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_parse_binspec_binj(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "binj (X)";  /* "binj" for long integer output. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TINT);
}

static void
test_parse_binspec_bini(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bini (X)";  /* "bini" for short integer output. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TSHORT);
}

static void
test_parse_binspec_binb(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "binb (X)";  /* "binb" for byte output. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TBYTE);
}

static void
test_parse_binspec_binr(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "binr (X)";  /* "binr" for float output. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TFLOAT);
}

static void
test_parse_binspec_3d(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (X, Y, Z)";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 3);
	fail_if(strcmp(colname[0], "X") != 0);
	fail_if(strcmp(colname[1], "Y") != 0);
	fail_if(strcmp(colname[2], "Z") != 0);
}

static void
test_parse_binspec_4d(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (A, B, C, D)";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 4);
}

static void
test_parse_binspec_minmax_only(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=10:20";  /* No binsize specified. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(minin[0] != 10.0);
	fail_if(maxin[0] != 20.0);
}

static void
test_parse_binspec_negative_binsize(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:100:-10";  /* Negative binsize. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(binsizein[0] != -10.0);
}

static void
test_parse_binspec_weight_value(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:10;3.5";  /* Constant weight value. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(weight != 3.5);
}

static void
test_parse_binspec_reciprocal(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:10;/W";  /* Reciprocal weight. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(recip != 1);
	fail_if(strcmp(wtname, "W") != 0);
}

static void
test_parse_binr_maxonly(void)
{
	int status = 0;
	char *ptr;
	char binspec[] = "COL=:50";  /* No min, no binsize. */
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	minin = -999.0;
	binsizein = -999.0;
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(maxin != 50.0);
	fail_if(minin != -999.0);  /* Unchanged. */
	fail_if(binsizein != -999.0);  /* Unchanged. */
}

static void
test_parse_binr_minonly(void)
{
	int status = 0;
	char *ptr;
	char binspec[] = "COL=10:";  /* Min, no max, no binsize. */
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	maxin = -999.0;
	binsizein = -999.0;
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(minin != 10.0);
	fail_if(maxin != -999.0);  /* Unchanged. */
	fail_if(binsizein != -999.0);  /* Unchanged. */
}

static void
test_parse_binr_all_colnames(void)
{
	int status = 0;
	char *ptr;
	char binspec[] = "COL=#MINC:#MAXC:#BINC";  /* Column name refs. */
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	minin = -999.0;
	maxin = -999.0;
	binsizein = -999.0;
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(strcmp(minname, "#MINC") != 0);
	fail_if(strcmp(maxname, "#MAXC") != 0);
	fail_if(strcmp(binname, "#BINC") != 0);
}

static void
test_parse_binspec_bind(void)
{
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bind (X)";  /* "bind" for double output. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TDOUBLE);
}

static void
test_parse_binspec_2d_with_range(void)
{
	/* Test bin (X,Y)=binsize format - applies same binsize to all axes. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (X,Y)=5";  /* 2D with uniform binsize. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 2);
	fail_if(binsizein[0] != 5.0);
	fail_if(binsizein[1] != 5.0);
}

static void
test_parse_binspec_2d_with_minmax(void)
{
	/* Test bin (X,Y)=min:max format - applies same range to all axes. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (X,Y)=0:100:10";  /* 2D with uniform range. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 2);
	fail_if(minin[0] != 0.0);
	fail_if(minin[1] != 0.0);
	fail_if(maxin[0] != 100.0);
	fail_if(maxin[1] != 100.0);
	fail_if(binsizein[0] != 10.0);
	fail_if(binsizein[1] != 10.0);
}

static void
test_parse_binspec_2d_with_weight(void)
{
	/* Test bin (X,Y)=binsize;weight format. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin (X,Y)=5;2.5";  /* 2D with uniform binsize and weight. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 2);
	fail_if(binsizein[0] != 5.0);
	fail_if(weight != 2.5);
}

static void
test_parse_binspec_empty_defaults(void)
{
	/* Test "bin" or "binr" with no other params - uses all defaults. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin";  /* Just "bin" uses all defaults. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TINT);  /* Default type. */
}

static void
test_parse_binspec_just_type(void)
{
	/* Test "binr" with no other params - uses defaults with float type. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "binr";  /* Just "binr" uses defaults with float type. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TFLOAT);
}

static void
test_parse_binspec_type_with_spaces(void)
{
	/* Test "binr " with spaces after type. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "binr   ";  /* "binr" followed by spaces. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(imagetype != TFLOAT);
}

static void
test_parse_binspec_syntax_error(void)
{
	/* Test syntax error - "binx" not recognized. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "binx (X)";  /* Invalid type 'x'. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status == 0);  /* Should fail. */
}

static void
test_parse_binr_single_value(void)
{
	/* Test single value = just binsize, no colons. */
	int status = 0;
	char *ptr;
	char binspec[] = "COL=10";  /* Just binsize. */
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	minin = -999.0;
	maxin = -999.0;
	binsizein = -999.0;
	minname[0] = '\0';
	maxname[0] = '\0';
	binname[0] = '\0';
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(binsizein != 10.0);
	fail_if(minin != -999.0);  /* Unchanged. */
	fail_if(maxin != -999.0);  /* Unchanged. */
}

static void
test_parse_binr_binsize_name(void)
{
	/* Test single value as column name (not number). */
	int status = 0;
	char *ptr;
	char binspec[] = "COL=#BINKEY";  /* Binsize from keyword. */
	char colname[FLEN_VALUE];
	double minin, maxin, binsizein;
	char minname[FLEN_VALUE], maxname[FLEN_VALUE], binname[FLEN_VALUE];

	ptr = binspec;
	minin = -999.0;
	maxin = -999.0;
	binsizein = -999.0;
	minname[0] = '\0';
	maxname[0] = '\0';
	binname[0] = '\0';
	ffbinr(
		&ptr, colname, &minin, &maxin, &binsizein,
		minname, maxname, binname, &status
	);
	fail_if(status != 0);
	fail_if(strcmp(binname, "#BINKEY") != 0);
	fail_if(binsizein != -999.0);  /* Unchanged since using name. */
}

static void
test_histogram_integer_columns(void)
{
	/* Test histogram with integer column data - exercises integer binning path. */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1J" };  /* Integer column. */
	long xdata[] = { 1, 2, 3, 4, 5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	/* Create table with integer column. */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	/* Use integer limits to exercise integer binning path. */
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 2.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_reversed_minmax(void)
{
	/* Test histogram with max < min - binsize should be negated. */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 3, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	/* Reversed: max < min. */
	minin[0] = 5.0;
	maxin[0] = 0.0;
	binsizein[0] = 1.0;  /* Positive binsize with reversed range. */
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_with_selectrow(void)
{
	/* Test histogram with row selection array. */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
	char selectrow[] = { 1, 0, 1, 0, 1 };  /* Select rows 1, 3, 5. */
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, selectrow, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_auto_range(void)
{
	/* Test histogram with auto-determined range (DOUBLENULLVALUE). */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	/* Use DOUBLENULLVALUE to auto-determine range. */
	minin[0] = DOUBLENULLVALUE;
	maxin[0] = DOUBLENULLVALUE;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_auto_binsize(void)
{
	/* Test histogram with auto-determined binsize (DOUBLENULLVALUE). */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 10.0;
	/* Use DOUBLENULLVALUE to auto-determine binsize. */
	binsizein[0] = DOUBLENULLVALUE;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_with_wtcol(void)
{
	/* Test histogram with weight column name. */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X", "W" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
	double wdata[] = { 1.0, 2.0, 1.0, 2.0, 1.0 };  /* Weight column. */
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_06(ffpcld, f, 2, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	/* Use weight column "W". */
	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		0.0, "W", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_with_reciprocal_weight(void)
{
	/* Test histogram with reciprocal weight column. */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X", "W" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
	double wdata[] = { 0.5, 1.0, 0.5, 1.0, 0.5 };  /* Weight column. */
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_06(ffpcld, f, 2, 1, 1, 5, wdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	/* Use reciprocal of weight column "W". */
	histptr = ffhist3(
		f, "!" hist_path, TFLOAT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		0.0, "W", 1, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_parse_binspec_2d_no_parens(void)
{
	/* Test 2D binspec without parentheses: X=0:10, Y=0:20 */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:10, Y=0:20";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 2);
	fail_if(strcmp(colname[0], "X") != 0);
	fail_if(strcmp(colname[1], "Y") != 0);
	fail_if(minin[0] != 0.0);
	fail_if(maxin[0] != 10.0);
	fail_if(minin[1] != 0.0);
	fail_if(maxin[1] != 20.0);
}

static void
test_parse_binspec_3d_no_parens(void)
{
	/* Test 3D binspec without parentheses. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:10:2, Y=0:20:4, Z=0:30:6";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 3);
	fail_if(minin[0] != 0.0);
	fail_if(maxin[0] != 10.0);
	fail_if(binsizein[0] != 2.0);
	fail_if(minin[1] != 0.0);
	fail_if(maxin[1] != 20.0);
	fail_if(binsizein[1] != 4.0);
	fail_if(minin[2] != 0.0);
	fail_if(maxin[2] != 30.0);
	fail_if(binsizein[2] != 6.0);
}

static void
test_parse_binspec_4d_no_parens(void)
{
	/* Test 4D binspec without parentheses. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin A=0:10, B=0:20, C=0:30, D=0:40";

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(histaxis != 4);
	fail_if(minin[0] != 0.0);
	fail_if(maxin[0] != 10.0);
	fail_if(minin[1] != 0.0);
	fail_if(maxin[1] != 20.0);
	fail_if(minin[2] != 0.0);
	fail_if(maxin[2] != 30.0);
	fail_if(minin[3] != 0.0);
	fail_if(maxin[3] != 40.0);
}

static void
test_parse_binspec_with_wtcol(void)
{
	/* Test binspec with weight column name. */
	int status = 0;
	int imagetype, histaxis, recip;
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	double weight;
	char wtname[FLEN_VALUE];
	char binspec[] = "bin X=0:10;W";  /* Weight column W. */

	ffbins(
		binspec, &imagetype, &histaxis, colname, minin, maxin,
		binsizein, minname, maxname, binname, &weight, wtname,
		&recip, &status
	);
	fail_if(status != 0);
	fail_if(strcmp(wtname, "W") != 0);
	fail_if(recip != 0);  /* Not reciprocal. */
}

static void
test_histogram_with_tlmin_tlmax(void)
{
	/* Test histogram using TLMIN/TLMAX keywords for range. */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1D" };
	double xdata[] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;
	double tlmin = 0.0, tlmax = 10.0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	/* Add TLMIN1 and TLMAX1 keywords. */
	call_04(ffpkyj, f, "TLMIN1", (LONGLONG)tlmin, "Min value");
	call_04(ffpkyj, f, "TLMAX1", (LONGLONG)tlmax, "Max value");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	/* Use DOUBLENULLVALUE to get range from TLMIN/TLMAX. */
	minin[0] = DOUBLENULLVALUE;
	maxin[0] = DOUBLENULLVALUE;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_float_column(void)
{
	/* Test histogram with float column (exercises float datatype path). */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1E" };  /* Single precision float column. */
	float xdata[] = { 1.5f, 2.5f, 3.5f, 4.5f, 5.5f };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcle, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_short_column(void)
{
	/* Test histogram with short column (exercises short datatype path). */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1I" };  /* Short column. */
	short xdata[] = { 1, 2, 3, 4, 5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpcli, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	/* Use integer limits to exercise integer binning path. */
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_histogram_byte_column(void)
{
	/* Test histogram with byte column (exercises byte datatype path). */
	fitsfile *f, *histptr;
	int status = 0;
	char *ttype[] = { "X" };
	char *tform[] = { "1B" };  /* Byte column. */
	unsigned char xdata[] = { 1, 2, 3, 4, 5 };
	char colname[MAXHISTAXES][FLEN_VALUE];
	double minin[MAXHISTAXES];
	double maxin[MAXHISTAXES];
	double binsizein[MAXHISTAXES];
	char minname[MAXHISTAXES][FLEN_VALUE];
	char maxname[MAXHISTAXES][FLEN_VALUE];
	char binname[MAXHISTAXES][FLEN_VALUE];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 1, ttype, tform, NULL, NULL);
	call_06(ffpclb, f, 1, 1, 1, 5, xdata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	for (i = 1; i < 4; i += 1) {
		colname[i][0] = '\0';
	}
	minin[0] = 0.0;
	maxin[0] = 10.0;
	binsizein[0] = 1.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	histptr = ffhist3(
		f, "!" hist_path, TINT, 1, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, "", 0, NULL, &status
	);
	fail_if(status != 0);
	fail_if(histptr == NULL);

	call_01(ffclos, histptr);
	call_01(ffclos, f);
	remove(hist_path);
}

static void
test_fits_calc_binning(void)
{
	/* Test fits_calc_binning for computing bin parameters. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "X", "Y" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double ydata[] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
	int naxis = 2;
	char colname[4][FLEN_VALUE];
	double minin[4], maxin[4], binsizein[4];
	char minname[4][FLEN_VALUE], maxname[4][FLEN_VALUE];
	char binname[4][FLEN_VALUE];
	int colnum[4];
	long haxes[4];
	float amin[4], amax[4], binsize[4];
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_06(ffpcld, f, 2, 1, 1, 5, ydata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	strcpy(colname[1], "Y");
	colname[2][0] = '\0';
	colname[3][0] = '\0';

	for (i = 0; i < 4; i += 1) {
		minin[i] = DOUBLENULLVALUE;
		maxin[i] = DOUBLENULLVALUE;
		binsizein[i] = DOUBLENULLVALUE;
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	status = fits_calc_binning(
		f, naxis, colname, minin, maxin, binsizein,
		minname, maxname, binname,
		colnum, haxes, amin, amax, binsize, &status
	);
	fail_if(status != 0);

	call_01(ffclos, f);
}

static void
test_ffhist2(void)
{
	/* Test ffhist2 for creating 2D histogram. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "X", "Y" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double ydata[] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
	char colname[4][FLEN_VALUE];
	double minin[4], maxin[4], binsizein[4];
	char minname[4][FLEN_VALUE], maxname[4][FLEN_VALUE];
	char binname[4][FLEN_VALUE];
	char wtcol[FLEN_VALUE] = "";
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_06(ffpcld, f, 2, 1, 1, 5, ydata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	strcpy(colname[1], "Y");
	colname[2][0] = '\0';
	colname[3][0] = '\0';

	minin[0] = 0.0; maxin[0] = 6.0; binsizein[0] = 1.0;
	minin[1] = 0.0; maxin[1] = 60.0; binsizein[1] = 10.0;
	for (i = 0; i < 4; i += 1) {
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}

	status = ffhist2(
		&f, "!" hist_path, TINT, 2, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, wtcol, 0, NULL, &status
	);
	fail_if(status != 0);
	if (f) {
		call_01(ffclos, f);
	}
	remove(hist_path);
}

static void
test_ffhist2_auto_range(void)
{
	/* Test ffhist2 with automatic range detection. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "X", "Y" };
	char *tform[] = { "1D", "1D" };
	double xdata[] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
	double ydata[] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
	char colname[4][FLEN_VALUE];
	double minin[4], maxin[4], binsizein[4];
	char minname[4][FLEN_VALUE], maxname[4][FLEN_VALUE];
	char binname[4][FLEN_VALUE];
	char wtcol[FLEN_VALUE] = "";
	int i;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 5, 2, ttype, tform, NULL, NULL);
	call_06(ffpcld, f, 1, 1, 1, 5, xdata);
	call_06(ffpcld, f, 2, 1, 1, 5, ydata);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READWRITE);
	call_03(ffmahd, f, 2, NULL);

	strcpy(colname[0], "X");
	strcpy(colname[1], "Y");
	colname[2][0] = '\0';
	colname[3][0] = '\0';

	for (i = 0; i < 4; i += 1) {
		minin[i] = DOUBLENULLVALUE;
		maxin[i] = DOUBLENULLVALUE;
		binsizein[i] = 1.0;
		minname[i][0] = '\0';
		maxname[i][0] = '\0';
		binname[i][0] = '\0';
	}
	binsizein[1] = 10.0;

	status = ffhist2(
		&f, "!" hist_path, TINT, 2, colname,
		minin, maxin, binsizein,
		minname, maxname, binname,
		1.0, wtcol, 0, NULL, &status
	);
	fail_if(status != 0);
	if (f) {
		call_01(ffclos, f);
	}
	remove(hist_path);
}

int
main(void)
{
	test_parse_binspec_1d();
	test_parse_binspec_2d();
	test_parse_binspec_with_range();
	test_parse_binspec_with_binsize_only();
	test_parse_binspec_with_weight();
	test_parse_binspec_imagetype();
	test_parse_binspec_binj();
	test_parse_binspec_bini();
	test_parse_binspec_binb();
	test_parse_binspec_binr();
	test_parse_binspec_3d();
	test_parse_binspec_4d();
	test_parse_binspec_minmax_only();
	test_parse_binspec_negative_binsize();
	test_parse_binspec_weight_value();
	test_parse_binspec_reciprocal();
	test_parse_binr_simple();
	test_parse_binr_partial();
	test_parse_binr_maxonly();
	test_parse_binr_minonly();
	test_parse_binr_all_colnames();
	test_parse_binspec_bind();
	test_parse_binspec_2d_with_range();
	test_parse_binspec_2d_with_minmax();
	test_parse_binspec_2d_with_weight();
	test_parse_binspec_empty_defaults();
	test_parse_binspec_just_type();
	test_parse_binspec_type_with_spaces();
	test_parse_binspec_syntax_error();
	test_parse_binr_single_value();
	test_parse_binr_binsize_name();
	test_create_1d_histogram();
	test_create_2d_histogram();
	test_create_histogram_with_weight();
	test_create_histogram_float_output();
	test_create_histogram_double_output();
	test_create_histogram_short_output();
	test_create_histogram_byte_output();
	test_histogram_bad_datatype();
	test_histogram_too_many_axes();
	test_create_3d_histogram();
	test_create_4d_histogram();
	test_histogram_with_constant_weight();
	test_histogram_integer_columns();
	test_histogram_reversed_minmax();
	test_histogram_with_selectrow();
	test_histogram_auto_range();
	test_histogram_auto_binsize();
	test_histogram_with_wtcol();
	test_histogram_with_reciprocal_weight();
	test_parse_binspec_2d_no_parens();
	test_parse_binspec_3d_no_parens();
	test_parse_binspec_4d_no_parens();
	test_parse_binspec_with_wtcol();
	test_histogram_with_tlmin_tlmax();
	test_histogram_float_column();
	test_histogram_short_column();
	test_histogram_byte_column();
	test_fits_calc_binning();
	test_ffhist2();
	test_ffhist2_auto_range();
	remove(test_path);
	return 0;
}
