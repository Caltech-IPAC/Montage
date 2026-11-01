/*
 * Tests to exercise wcssub.c - WCS coordinate system functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_wcssub.fits"

static void
cleanup(void)
{
	remove(test_path);
}

static void
test_ffgics_cdelt(void)
{
	/* Test ffgics with CDELT/CROTA keywords. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* Write WCS keywords using CDELT form. */
	call_05(ffpkyd, f, "CRVAL1", 180.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 45.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CDELT1", -0.001, 10, NULL);
	call_05(ffpkyd, f, "CDELT2", 0.001, 10, NULL);
	call_05(ffpkyd, f, "CROTA2", 30.0, 10, NULL);
	call_04(ffpkys, f, "CTYPE1", "RA---TAN", NULL);
	call_04(ffpkys, f, "CTYPE2", "DEC--TAN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	fail_if(fabs(xrval - 180.0) > 0.001);
	fail_if(fabs(yrval - 45.0) > 0.001);
	fail_if(fabs(xrpix - 50.0) > 0.001);
	fail_if(fabs(yrpix - 50.0) > 0.001);
	fail_if(fabs(xinc - (-0.001)) > 0.0001);
	fail_if(fabs(yinc - 0.001) > 0.0001);
	fail_if(fabs(rot - 30.0) > 0.1);
	fail_if(strcmp(type, "-TAN") != 0);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgics_cd_matrix(void)
{
	/* Test ffgics with CD matrix keywords. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];
	double scale = 0.001;

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* Write WCS keywords using CD matrix form (no rotation). */
	call_05(ffpkyd, f, "CRVAL1", 120.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", -30.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CD1_1", -scale, 10, NULL);
	call_05(ffpkyd, f, "CD1_2", 0.0, 10, NULL);
	call_05(ffpkyd, f, "CD2_1", 0.0, 10, NULL);
	call_05(ffpkyd, f, "CD2_2", scale, 10, NULL);
	call_04(ffpkys, f, "CTYPE1", "RA---TAN", NULL);
	call_04(ffpkys, f, "CTYPE2", "DEC--TAN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	fail_if(fabs(xrval - 120.0) > 0.001);
	fail_if(fabs(yrval - (-30.0)) > 0.001);
	fail_if(fabs(xinc) - scale > 0.0001);
	fail_if(fabs(yinc) - scale > 0.0001);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgics_pc_matrix(void)
{
	/* Test ffgics with PC matrix keywords. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* Write WCS keywords using PC matrix form. */
	call_05(ffpkyd, f, "CRVAL1", 90.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 0.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CDELT1", -0.001, 10, NULL);
	call_05(ffpkyd, f, "CDELT2", 0.001, 10, NULL);
	call_05(ffpkyd, f, "PC1_1", 1.0, 10, NULL);
	call_05(ffpkyd, f, "PC1_2", 0.0, 10, NULL);
	call_05(ffpkyd, f, "PC2_1", 0.0, 10, NULL);
	call_05(ffpkyd, f, "PC2_2", 1.0, 10, NULL);
	call_04(ffpkys, f, "CTYPE1", "RA---SIN", NULL);
	call_04(ffpkys, f, "CTYPE2", "DEC--SIN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	fail_if(fabs(xrval - 90.0) > 0.001);
	fail_if(fabs(yrval - 0.0) > 0.001);
	fail_if(strcmp(type, "-SIN") != 0);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgics_defaults(void)
{
	/* Test ffgics with missing keywords (uses defaults). */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);
	/* No WCS keywords - should use defaults. */
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	/* Default values. */
	fail_if(fabs(xrval - 0.0) > 0.001);
	fail_if(fabs(yrval - 0.0) > 0.001);
	fail_if(fabs(xrpix - 0.0) > 0.001);
	fail_if(fabs(yrpix - 0.0) > 0.001);
	fail_if(fabs(xinc - 1.0) > 0.001);
	fail_if(fabs(yinc - 1.0) > 0.001);
	fail_if(fabs(rot - 0.0) > 0.001);
	fail_if(type[0] != '\0');

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgics_dec_first(void)
{
	/* Test ffgics with DEC axis first (swapped). */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* DEC is first axis. */
	call_05(ffpkyd, f, "CRVAL1", 45.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 180.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CDELT1", 0.001, 10, NULL);
	call_05(ffpkyd, f, "CDELT2", -0.001, 10, NULL);
	call_04(ffpkys, f, "CTYPE1", "DEC--TAN", NULL);
	call_04(ffpkys, f, "CTYPE2", "RA---TAN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	/* Values should be swapped. */
	fail_if(fabs(xrval - 180.0) > 0.001);
	fail_if(fabs(yrval - 45.0) > 0.001);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgicsa_version(void)
{
	/* Test ffgicsa with alternate WCS version. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* Write primary WCS. */
	call_05(ffpkyd, f, "CRVAL1", 180.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 45.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CDELT1", -0.001, 10, NULL);
	call_05(ffpkyd, f, "CDELT2", 0.001, 10, NULL);
	call_04(ffpkys, f, "CTYPE1", "RA---TAN", NULL);
	call_04(ffpkys, f, "CTYPE2", "DEC--TAN", NULL);

	/* Write alternate WCS version A. */
	call_05(ffpkyd, f, "CRVAL1A", 90.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2A", 30.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1A", 25.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2A", 25.0, 10, NULL);
	call_05(ffpkyd, f, "CDELT1A", -0.002, 10, NULL);
	call_05(ffpkyd, f, "CDELT2A", 0.002, 10, NULL);
	call_04(ffpkys, f, "CTYPE1A", "RA---SIN", NULL);
	call_04(ffpkys, f, "CTYPE2A", "DEC--SIN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);

	/* Read primary WCS (blank version). */
	call_10(ffgicsa, f, ' ', &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);
	fail_if(fabs(xrval - 180.0) > 0.001);
	fail_if(fabs(yrval - 45.0) > 0.001);

	/* Read alternate WCS version A. */
	status = 0;
	call_10(ffgicsa, f, 'A', &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);
	fail_if(fabs(xrval - 90.0) > 0.001);
	fail_if(fabs(yrval - 30.0) > 0.001);
	fail_if(fabs(xrpix - 25.0) > 0.001);
	fail_if(fabs(xinc - (-0.002)) > 0.0001);
	fail_if(strcmp(type, "-SIN") != 0);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgicsa_invalid_version(void)
{
	/* Test ffgicsa with invalid version code. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);

	/* Invalid version code (not A-Z or blank). */
	ffgicsa(f, '1', &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type, &status);
	fail_if(status == 0);

	status = 0;
	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgiwcs(void)
{
	/* Test ffgiwcs (deprecated but still functional). */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	char *header = NULL;

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);
	call_05(ffpkyd, f, "CRVAL1", 180.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 45.0, 10, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_02(ffgiwcs, f, &header);

	fail_if(header == NULL);
	/* Header should contain WCS keywords. */
	fail_if(strstr(header, "CRVAL1") == NULL);
	fail_if(strstr(header, "CRVAL2") == NULL);

	free(header);
	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgiwcs_not_image(void)
{
	/* Test ffgiwcs error when HDU is not an image. */
	fitsfile *f;
	int status = 0;
	char *header = NULL;
	char *ttype[] = {"X"};
	char *tform[] = {"1D"};

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	/* Create a primary array. */
	long naxes[2] = {0, 0};
	call_04(ffcrim, f, BYTE_IMG, 0, naxes);
	/* Create a binary table extension. */
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	/* Move to the table extension. */
	call_03(ffmahd, f, 2, NULL);

	/* Should fail because we're on a table, not an image. */
	ffgiwcs(f, &header, &status);
	fail_if(status == 0);

	if (header != NULL) {
		free(header);
	}
	status = 0;
	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgtcs(void)
{
	/* Test ffgtcs - get WCS from table columns. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = {"X", "Y"};
	char *tform[] = {"1D", "1D"};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];
	long naxes[2] = {0, 0};

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, BYTE_IMG, 0, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 2, ttype, tform, NULL, NULL);

	/* Write WCS keywords for table columns. */
	call_04(ffpkys, f, "TCTYP1", "RA---TAN", NULL);
	call_04(ffpkys, f, "TCTYP2", "DEC--TAN", NULL);
	call_05(ffpkyd, f, "TCRVL1", 180.0, 10, NULL);
	call_05(ffpkyd, f, "TCRVL2", 45.0, 10, NULL);
	call_05(ffpkyd, f, "TCRPX1", 1.0, 10, NULL);
	call_05(ffpkyd, f, "TCRPX2", 1.0, 10, NULL);
	call_05(ffpkyd, f, "TCDLT1", -0.001, 10, NULL);
	call_05(ffpkyd, f, "TCDLT2", 0.001, 10, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_11(ffgtcs, f, 1, 2, &xrval, &yrval, &xrpix, &yrpix,
		&xinc, &yinc, &rot, type);

	fail_if(fabs(xrval - 180.0) > 0.001);
	fail_if(fabs(yrval - 45.0) > 0.001);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgtwcs(void)
{
	/* Test ffgtwcs - get table WCS keywords string. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = {"X", "Y"};
	char *tform[] = {"1D", "1D"};
	char *header = NULL;
	long naxes[2] = {0, 0};

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, BYTE_IMG, 0, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 2, ttype, tform, NULL, NULL);

	/* Write WCS keywords for table columns. */
	call_04(ffpkys, f, "TCTYP1", "RA---TAN", NULL);
	call_04(ffpkys, f, "TCTYP2", "DEC--TAN", NULL);
	call_05(ffpkyd, f, "TCRVL1", 180.0, 10, NULL);
	call_05(ffpkyd, f, "TCRVL2", 45.0, 10, NULL);
	call_05(ffpkyd, f, "TCRPX1", 1.0, 10, NULL);
	call_05(ffpkyd, f, "TCRPX2", 1.0, 10, NULL);
	call_05(ffpkyd, f, "TCDLT1", -0.001, 10, NULL);
	call_05(ffpkyd, f, "TCDLT2", 0.001, 10, NULL);
	call_04(ffpkyj, f, "TLMIN1", 1, NULL);
	call_04(ffpkyj, f, "TLMAX1", 100, NULL);
	call_04(ffpkyj, f, "TLMIN2", 1, NULL);
	call_04(ffpkyj, f, "TLMAX2", 100, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);
	call_04(ffgtwcs, f, 1, 2, &header);

	fail_if(header == NULL);
	/* Header should contain converted WCS keywords. */
	fail_if(strstr(header, "NAXIS") == NULL);
	fail_if(strstr(header, "CTYPE1") == NULL);
	fail_if(strstr(header, "CRVAL1") == NULL);
	fail_if(strstr(header, "END") == NULL);

	free(header);
	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgtwcs_not_table(void)
{
	/* Test ffgtwcs error when HDU is not a table. */
	fitsfile *f;
	int status = 0;
	char *header = NULL;
	long naxes[2] = {100, 100};

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);

	/* Should fail because we're on an image, not a table. */
	ffgtwcs(f, 1, 2, &header, &status);
	fail_if(status == 0);

	if (header != NULL) {
		free(header);
	}
	status = 0;
	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgtwcs_bad_col(void)
{
	/* Test ffgtwcs error with bad column number. */
	fitsfile *f;
	int status = 0;
	char *ttype[] = {"X", "Y"};
	char *tform[] = {"1D", "1D"};
	char *header = NULL;
	long naxes[2] = {0, 0};

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, BYTE_IMG, 0, naxes);
	call_08(ffcrtb, f, BINARY_TBL, 10, 2, ttype, tform, NULL, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffmahd, f, 2, NULL);

	/* Invalid column number. */
	ffgtwcs(f, 0, 2, &header, &status);
	fail_if(status == 0);

	if (header != NULL) {
		free(header);
	}
	status = 0;

	ffgtwcs(f, 1, 99, &header, &status);
	fail_if(status == 0);

	if (header != NULL) {
		free(header);
	}
	status = 0;
	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgics_cd_rotated(void)
{
	/* Test ffgics with rotated CD matrix. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];
	double scale = 0.001;
	double angle = 45.0 * 3.14159265 / 180.0;  /* 45 degrees in radians */
	double cosa = cos(angle);
	double sina = sin(angle);

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* Write rotated CD matrix. */
	call_05(ffpkyd, f, "CRVAL1", 0.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 0.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CD1_1", -scale * cosa, 15, NULL);
	call_05(ffpkyd, f, "CD1_2", scale * sina, 15, NULL);
	call_05(ffpkyd, f, "CD2_1", scale * sina, 15, NULL);
	call_05(ffpkyd, f, "CD2_2", scale * cosa, 15, NULL);
	call_04(ffpkys, f, "CTYPE1", "RA---TAN", NULL);
	call_04(ffpkys, f, "CTYPE2", "DEC--TAN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	/* Should extract rotation angle ~-45 degrees. */
	fail_if(fabs(rot - (-45.0)) > 1.0);

	call_01(ffclos, f);
	cleanup();
}

static void
test_ffgics_glat(void)
{
	/* Test ffgics with galactic coordinates (GLAT first). */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};
	double xrval, yrval, xrpix, yrpix, xinc, yinc, rot;
	char type[5];

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);

	/* GLAT is first axis. */
	call_05(ffpkyd, f, "CRVAL1", 30.0, 10, NULL);
	call_05(ffpkyd, f, "CRVAL2", 120.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX1", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CRPIX2", 50.0, 10, NULL);
	call_05(ffpkyd, f, "CDELT1", 0.001, 10, NULL);
	call_05(ffpkyd, f, "CDELT2", -0.001, 10, NULL);
	call_04(ffpkys, f, "CTYPE1", "GLAT-TAN", NULL);
	call_04(ffpkys, f, "CTYPE2", "GLON-TAN", NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_09(ffgics, f, &xrval, &yrval, &xrpix, &yrpix, &xinc, &yinc, &rot, type);

	/* Values should be swapped because LAT is first. */
	fail_if(fabs(xrval - 120.0) > 0.001);
	fail_if(fabs(yrval - 30.0) > 0.001);

	call_01(ffclos, f);
	cleanup();
}

static void
test_fits_read_wcstab_null(void)
{
	/* Test fits_read_wcstab with NULL pointer. */
	int status = 0;

	fits_read_wcstab(NULL, 0, NULL, &status);
	fail_if(status == 0);
}

static void
test_fits_read_wcstab_zero(void)
{
	/* Test fits_read_wcstab with nwtb=0. */
	fitsfile *f;
	int status = 0;
	long naxes[2] = {100, 100};

	cleanup();

	call_02(ffinit, &f, "!" test_path);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);

	/* With nwtb=0, should return immediately with no error. */
	fits_read_wcstab(f, 0, NULL, &status);
	fail_if(status != 0);

	call_01(ffclos, f);
	cleanup();
}

int
main(void)
{
	test_ffgics_cdelt();
	test_ffgics_cd_matrix();
	test_ffgics_cd_rotated();
	test_ffgics_pc_matrix();
	test_ffgics_defaults();
	test_ffgics_dec_first();
	test_ffgics_glat();
	test_ffgicsa_version();
	test_ffgicsa_invalid_version();
	test_ffgiwcs();
	test_ffgiwcs_not_image();
	test_ffgtcs();
	test_ffgtwcs();
	test_ffgtwcs_not_table();
	test_ffgtwcs_bad_col();
	test_fits_read_wcstab_null();
	test_fits_read_wcstab_zero();
	return 0;
}
