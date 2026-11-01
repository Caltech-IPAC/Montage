/*
 * Tests for scalnull.c - scaling and null value functions.
 */

#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_scalnull.fits"

static void
test_ffpthp_basic(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1PA" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_02(ffpthp, f, 100);
	call_01(ffclos, f);
}

static void
test_ffpthp_bad_theap(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	/* theap < 1 should return early */
	ffpthp(f, 0, &status);
	fail_if(status != 0);
	call_01(ffclos, f);
}

static void
test_ffpscl_basic(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_03(ffpscl, f, 2.0, 100.0);
	call_01(ffclos, f);
}

static void
test_ffpscl_zero_scale(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	ffpscl(f, 0.0, 0.0, &status);
	fail_if(status != ZERO_SCALE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffpscl_not_image(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpscl(f, 1.0, 0.0, &status);
	fail_if(status != NOT_IMAGE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffpscl_error_status(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	status = 1;
	ffpscl(f, 1.0, 0.0, &status);
	fail_if(status != 1);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffpnul_basic(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_02(ffpnul, f, -999);
	call_01(ffclos, f);
}

static void
test_ffpnul_not_image(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffpnul(f, -999, &status);
	fail_if(status != NOT_IMAGE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffpnul_error_status(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	status = 1;
	ffpnul(f, -999, &status);
	fail_if(status != 1);
	status = 0;
	call_01(ffclos, f);
}

static void
test_fftscl_basic(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_04(fftscl, f, 1, 2.0, 100.0);
	call_01(ffclos, f);
}

static void
test_fftscl_zero_scale(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	fftscl(f, 1, 0.0, 0.0, &status);
	fail_if(status != ZERO_SCALE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_fftscl_not_table(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, SHORT_IMG, 1, naxes);
	fftscl(f, 1, 1.0, 0.0, &status);
	fail_if(status != NOT_TABLE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_fftscl_error_status(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	status = 1;
	fftscl(f, 1, 1.0, 0.0, &status);
	fail_if(status != 1);
	status = 0;
	call_01(ffclos, f);
}

static void
test_fftnul_basic(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(fftnul, f, 1, -999);
	call_01(ffclos, f);
}

static void
test_fftnul_not_btable(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "A10" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	fftnul(f, 1, -999, &status);
	fail_if(status != NOT_BTABLE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_fftnul_error_status(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	status = 1;
	fftnul(f, 1, -999, &status);
	fail_if(status != 1);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffsnul_basic(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "A10" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	call_03(ffsnul, f, 1, "N/A");
	call_01(ffclos, f);
}

static void
test_ffsnul_not_atable(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 1, 1, ttype, tform, NULL, NULL);
	ffsnul(f, 1, "N/A", &status);
	fail_if(status != NOT_ATABLE);
	status = 0;
	call_01(ffclos, f);
}

static void
test_ffsnul_error_status(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "A10" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, ASCII_TBL, 1, 1, ttype, tform, NULL, NULL);
	status = 1;
	ffsnul(f, 1, "N/A", &status);
	fail_if(status != 1);
	status = 0;
	call_01(ffclos, f);
}

int
main(void)
{
	test_ffpthp_basic();
	test_ffpthp_bad_theap();
	test_ffpscl_basic();
	test_ffpscl_zero_scale();
	test_ffpscl_not_image();
	test_ffpscl_error_status();
	test_ffpnul_basic();
	test_ffpnul_not_image();
	test_ffpnul_error_status();
	test_fftscl_basic();
	test_fftscl_zero_scale();
	test_fftscl_not_table();
	test_fftscl_error_status();
	test_fftnul_basic();
	test_fftnul_not_btable();
	test_fftnul_error_status();
	test_ffsnul_basic();
	test_ffsnul_not_atable();
	test_ffsnul_error_status();
	remove(test_path);
	return 0;
}
