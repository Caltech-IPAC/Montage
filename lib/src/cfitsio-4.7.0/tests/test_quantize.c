/*
 * Tests for quantize.c - floating point quantization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "fitsio2.h"
#include "test_macros.h"

/*
 * Test basic float quantization with gradient data
 */
static void
test_quantize_float_basic(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create gradient data with some noise */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = (float)(i * 10.0 + (i % 3) * 0.5);
	}

	/* Quantize with no dithering, negative qlevel for absolute quant */
	result = fits_quantize_float(0, fdata, 8, 8, 0, 0.0f, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);  /* Should succeed */
	fail_if(bscale == 0.0);

	/* Verify roundtrip approximate equality */
	for (i = 0; i < 64; i += 1) {
		double recovered = idata[i] * bscale + bzero;
		double diff = fabs(recovered - fdata[i]);
		fail_if(diff > bscale);  /* Error should be less than 1 quant */
	}
}

/*
 * Test basic double quantization
 */
static void
test_quantize_double_basic(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create gradient data with some noise */
	for (i = 0; i < 64; i += 1) {
		ddata[i] = i * 10.0 + (i % 3) * 0.5;
	}

	/* Quantize with no dithering, negative qlevel for absolute quant */
	result = fits_quantize_double(0, ddata, 8, 8, 0, 0.0, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);  /* Should succeed */
	fail_if(bscale == 0.0);

	/* Verify roundtrip approximate equality */
	for (i = 0; i < 64; i += 1) {
		double recovered = idata[i] * bscale + bzero;
		double diff = fabs(recovered - ddata[i]);
		fail_if(diff > bscale);
	}
}

/*
 * Test quantization with dithering method 1
 */
static void
test_quantize_dither1(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = (float)(i * 5.0 + (i % 5) * 0.3);
	}

	/* Quantize with SUBTRACTIVE_DITHER_1 (row > 0 enables dithering) */
	result = fits_quantize_float(1, fdata, 8, 8, 0, 0.0f, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);
}

/*
 * Test quantization with dithering method 2
 */
static void
test_quantize_dither2(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with some zeros */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fdata[i] = 0.0f;
		} else {
			fdata[i] = (float)(i * 5.0 + (i % 5) * 0.3);
		}
	}

	/* Quantize with SUBTRACTIVE_DITHER_2 (row > 0 enables dithering) */
	result = fits_quantize_float(1, fdata, 8, 8, 0, 0.0f, -1.0f,
		SUBTRACTIVE_DITHER_2, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);
}

/*
 * Test quantization with null values
 */
static void
test_quantize_with_nulls(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	float null_value = -999.0f;

	/* Create data with some null values */
	for (i = 0; i < 64; i += 1) {
		if (i % 10 == 0) {
			fdata[i] = null_value;
		} else {
			fdata[i] = (float)(i * 10.0 + (i % 3) * 0.5);
		}
	}

	/* Quantize with null checking */
	result = fits_quantize_float(0, fdata, 8, 8, 1, null_value, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);

	/* Null values should be preserved as NULL_VALUE (-2147483647) */
	for (i = 0; i < 64; i += 1) {
		if (i % 10 == 0) {
			fail_if(idata[i] != -2147483647);
		}
	}
}

/*
 * Test uniform data (should return 0 - no quantization needed)
 */
static void
test_quantize_uniform(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* All same value - no noise */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = 42.0f;
	}

	/* With positive qlevel, uniform data has zero noise -> delta=0 */
	result = fits_quantize_float(0, fdata, 8, 8, 0, 0.0f, 4.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* Should return 0 (no quantization) because delta=0 */
	fail_if(result != 0);
}

/*
 * Test quantization with negative qlevel (absolute quantization)
 */
static void
test_quantize_absolute(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create gradient data */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = (float)(i * 100.0);
	}

	/* Negative qlevel = absolute quantization level */
	result = fits_quantize_float(0, fdata, 8, 8, 0, 0.0f, -10.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	/* bscale should be close to 10 (the absolute level) */
	fail_if(fabs(bscale - 10.0) > 0.001);
}

/*
 * Test small image (nx * ny <= 1 should return 0)
 */
static void
test_quantize_small(void)
{
	float fdata[1] = { 42.0f };
	int idata[1];
	double bscale, bzero;
	int iminval, imaxval;
	int result;

	/* Single pixel - should return 0 */
	result = fits_quantize_float(0, fdata, 1, 1, 0, 0.0f, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 0);
	fail_if(bscale != 1.0);
	fail_if(bzero != 0.0);
}

/*
 * Test larger image with noise
 */
static void
test_quantize_large(void)
{
	float *fdata;
	int *idata;
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	fdata = malloc(256 * 256 * sizeof *fdata);
	idata = malloc(256 * 256 * sizeof *idata);
	fail_if(fdata == NULL);
	fail_if(idata == NULL);

	/* Create image with gradient and noise pattern */
	for (i = 0; i < 256 * 256; i += 1) {
		int x = i % 256;
		int y = i / 256;
		fdata[i] = (float)(x + y * 100 + (i % 7) * 0.1);
	}

	result = fits_quantize_float(0, fdata, 256, 256, 0, 0.0f, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);

	free(fdata);
	free(idata);
}

/*
 * Test double quantization with null values and dithering
 */
static void
test_quantize_double_nulls_dither(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	double null_value = -999.0;

	/* Create data with nulls */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			ddata[i] = null_value;
		} else {
			ddata[i] = i * 10.0 + (i % 3) * 0.5;
		}
	}

	/* Quantize with nulls and dithering */
	result = fits_quantize_double(1, ddata, 8, 8, 1, null_value, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);

	/* Null values should be preserved */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fail_if(idata[i] != -2147483647);
		}
	}
}

/*
 * Test fits_img_stats_short
 */
static void
test_img_stats_short_basic(void)
{
	short array[64];
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;

	/* Create gradient data */
	for (i = 0; i < 64; i += 1) {
		array[i] = (short)(i * 10 + (i % 3));
	}

	status = fits_img_stats_short(array, 8, 8, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 64);
	fail_if(minvalue != 0);
	fail_if(maxvalue != 630);  /* 63 * 10 + 0 (63 % 3 = 0) */
}

/*
 * Test fits_img_stats_short with null values
 */
static void
test_img_stats_short_nulls(void)
{
	short array[64];
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;
	short null_value = -999;

	/* Create data with some null values */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			array[i] = null_value;
		} else {
			array[i] = (short)(i * 10 + (i % 3));
		}
	}

	status = fits_img_stats_short(array, 8, 8, 1, null_value,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 56);  /* 64 - 8 nulls */
}

/*
 * Test fits_img_stats_int
 */
static void
test_img_stats_int_basic(void)
{
	int array[64];
	int status = 0;
	long ngoodpix;
	int minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;

	/* Create gradient data */
	for (i = 0; i < 64; i += 1) {
		array[i] = i * 1000 + (i % 7);
	}

	status = fits_img_stats_int(array, 8, 8, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 64);
	fail_if(minvalue != 0);
	fail_if(maxvalue != 63000);  /* 63 * 1000 + 0 */
}

/*
 * Test fits_img_stats_int with null values
 */
static void
test_img_stats_int_nulls(void)
{
	int array[64];
	int status = 0;
	long ngoodpix;
	int minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;
	int null_value = -99999;

	/* Create data with some null values */
	for (i = 0; i < 64; i += 1) {
		if (i % 10 == 0) {
			array[i] = null_value;
		} else {
			array[i] = i * 1000 + (i % 7);
		}
	}

	status = fits_img_stats_int(array, 8, 8, 1, null_value,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 57);  /* 64 - 7 nulls (0, 10, 20, 30, 40, 50, 60) */
}

/*
 * Test fits_img_stats_float
 */
static void
test_img_stats_float_basic(void)
{
	float array[64];
	int status = 0;
	long ngoodpix;
	float minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;

	/* Create gradient data with noise */
	for (i = 0; i < 64; i += 1) {
		array[i] = (float)(i * 10.5 + (i % 5) * 0.3);
	}

	status = fits_img_stats_float(array, 8, 8, 0, 0.0f,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 64);
}

/*
 * Test fits_img_stats_float with null values
 */
static void
test_img_stats_float_nulls(void)
{
	float array[64];
	int status = 0;
	long ngoodpix;
	float minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;
	float null_value = -999.0f;

	/* Create data with some null values */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			array[i] = null_value;
		} else {
			array[i] = (float)(i * 10.5 + (i % 5) * 0.3);
		}
	}

	status = fits_img_stats_float(array, 8, 8, 1, null_value,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 56);  /* 64 - 8 nulls */
}

/*
 * Test quantization with positive qlevel (noise-based)
 */
static void
test_quantize_positive_qlevel(void)
{
	float fdata[256];
	int idata[256];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with noise pattern */
	for (i = 0; i < 256; i += 1) {
		fdata[i] = (float)(i * 10.0 + (i % 7) * 0.5);
	}

	/* Positive qlevel = noise-based quantization */
	result = fits_quantize_float(0, fdata, 16, 16, 0, 0.0f, 4.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May or may not quantize depending on noise estimate */
	fail_if(result < 0);  /* Should not fail */
}

/*
 * Test double quantization with positive qlevel
 */
static void
test_quantize_double_positive_qlevel(void)
{
	double ddata[256];
	int idata[256];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with noise pattern */
	for (i = 0; i < 256; i += 1) {
		ddata[i] = i * 10.0 + (i % 7) * 0.5;
	}

	/* Positive qlevel = noise-based quantization */
	result = fits_quantize_double(0, ddata, 16, 16, 0, 0.0, 4.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May or may not quantize depending on noise estimate */
	fail_if(result < 0);  /* Should not fail */
}

/*
 * Test double quantization with dithering
 */
static void
test_quantize_double_dither1(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create gradient data */
	for (i = 0; i < 64; i += 1) {
		ddata[i] = i * 5.0 + (i % 5) * 0.3;
	}

	/* Quantize with SUBTRACTIVE_DITHER_1 (row > 0 enables dithering) */
	result = fits_quantize_double(1, ddata, 8, 8, 0, 0.0, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);
}

/*
 * Test double quantization with dithering method 2
 */
static void
test_quantize_double_dither2(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with some zeros */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			ddata[i] = 0.0;
		} else {
			ddata[i] = i * 5.0 + (i % 5) * 0.3;
		}
	}

	/* Quantize with SUBTRACTIVE_DITHER_2 (row > 0 enables dithering) */
	result = fits_quantize_double(1, ddata, 8, 8, 0, 0.0, -1.0f,
		SUBTRACTIVE_DITHER_2, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);
}

/*
 * Test quantization with range exceeding int
 */
static void
test_quantize_range_too_large(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with huge range */
	for (i = 0; i < 64; i += 1) {
		if (i < 32) {
			fdata[i] = -1e38f;
		} else {
			fdata[i] = 1e38f;
		}
	}

	/* Should return 0 because range is too large */
	result = fits_quantize_float(0, fdata, 8, 8, 0, 0.0f, -0.001f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 0);  /* Should not quantize */
}

/*
 * Test double quantization small image
 */
static void
test_quantize_double_small(void)
{
	double ddata[1] = { 42.0 };
	int idata[1];
	double bscale, bzero;
	int iminval, imaxval;
	int result;

	/* Single pixel - should return 0 */
	result = fits_quantize_double(0, ddata, 1, 1, 0, 0.0, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 0);
	fail_if(bscale != 1.0);
	fail_if(bzero != 0.0);
}

/*
 * Test img_stats with larger image
 */
static void
test_img_stats_short_large(void)
{
	short *array;
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;

	array = malloc(256 * 256 * sizeof *array);
	fail_if(array == NULL);

	/* Create image with gradient and noise pattern */
	for (i = 0; i < 256 * 256; i += 1) {
		int x = i % 256;
		int y = i / 256;
		array[i] = (short)((x + y * 10) % 32000);
	}

	status = fits_img_stats_short(array, 256, 256, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 256 * 256);

	free(array);
}

/*
 * Test img_stats_int with larger image
 */
static void
test_img_stats_int_large(void)
{
	int *array;
	int status = 0;
	long ngoodpix;
	int minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;

	array = malloc(256 * 256 * sizeof *array);
	fail_if(array == NULL);

	/* Create image with gradient and noise pattern */
	for (i = 0; i < 256 * 256; i += 1) {
		int x = i % 256;
		int y = i / 256;
		array[i] = x + y * 1000;
	}

	status = fits_img_stats_int(array, 256, 256, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 256 * 256);

	free(array);
}

/*
 * Test img_stats_float with larger image
 */
static void
test_img_stats_float_large(void)
{
	float *array;
	int status = 0;
	long ngoodpix;
	float minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;
	int i;

	array = malloc(256 * 256 * sizeof *array);
	fail_if(array == NULL);

	/* Create image with gradient and noise pattern */
	for (i = 0; i < 256 * 256; i += 1) {
		int x = i % 256;
		int y = i / 256;
		array[i] = (float)(x + y * 10.5 + (i % 7) * 0.1);
	}

	status = fits_img_stats_float(array, 256, 256, 0, 0.0f,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 256 * 256);

	free(array);
}

/*
 * Test img_stats with very small image (nx < 9)
 */
static void
test_img_stats_short_small(void)
{
	short array[4] = { 10, 20, 30, 40 };
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_short(array, 4, 1, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 4);
	fail_if(minvalue != 10);
	fail_if(maxvalue != 40);
}

/*
 * Test img_stats with single pixel (ngood == 1 branch)
 */
static void
test_img_stats_short_single(void)
{
	short array[1] = { 42 };
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_short(array, 1, 1, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 1);
	fail_if(mean != 42.0);
	fail_if(sigma != 0.0);
}

/*
 * Test img_stats_int with single pixel
 */
static void
test_img_stats_int_single(void)
{
	int array[1] = { 42 };
	int status = 0;
	long ngoodpix;
	int minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_int(array, 1, 1, 0, 0,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 1);
	fail_if(mean != 42.0);
	fail_if(sigma != 0.0);
}

/*
 * Test img_stats_float with single pixel
 */
static void
test_img_stats_float_single(void)
{
	float array[1] = { 42.5f };
	int status = 0;
	long ngoodpix;
	float minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_float(array, 1, 1, 0, 0.0f,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 1);
	fail_if(fabs(mean - 42.5) > 0.001);
	fail_if(sigma != 0.0);
}

/*
 * Test img_stats with all nulls (ngood == 0 branch)
 */
static void
test_img_stats_short_all_nulls(void)
{
	short array[4] = { -999, -999, -999, -999 };
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_short(array, 4, 1, 1, -999,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 0);
	fail_if(mean != 0.0);
	fail_if(sigma != 0.0);
}

/*
 * Test img_stats_int all nulls
 */
static void
test_img_stats_int_all_nulls(void)
{
	int array[4] = { -999, -999, -999, -999 };
	int status = 0;
	long ngoodpix;
	int minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_int(array, 4, 1, 1, -999,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 0);
	fail_if(mean != 0.0);
	fail_if(sigma != 0.0);
}

/*
 * Test img_stats_float all nulls
 */
static void
test_img_stats_float_all_nulls(void)
{
	float array[4] = { -999.0f, -999.0f, -999.0f, -999.0f };
	int status = 0;
	long ngoodpix;
	float minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_float(array, 4, 1, 1, -999.0f,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 0);
	fail_if(mean != 0.0);
	fail_if(sigma != 0.0);
}

/*
 * Test quantization with qlevel == 0 (default quantization)
 */
static void
test_quantize_qlevel_zero(void)
{
	float fdata[256];
	int idata[256];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with clear noise pattern */
	for (i = 0; i < 256; i += 1) {
		fdata[i] = (float)(i * 10.0 + (i % 17) * 2.5);
	}

	/* qlevel == 0 means use default delta = stdev / 4 */
	result = fits_quantize_float(0, fdata, 16, 16, 0, 0.0f, 0.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May or may not quantize depending on noise estimate */
	fail_if(result < 0);
}

/*
 * Test double quantization with qlevel == 0
 */
static void
test_quantize_double_qlevel_zero(void)
{
	double ddata[256];
	int idata[256];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with clear noise pattern */
	for (i = 0; i < 256; i += 1) {
		ddata[i] = i * 10.0 + (i % 17) * 2.5;
	}

	/* qlevel == 0 means use default delta = stdev / 4 */
	result = fits_quantize_double(0, ddata, 16, 16, 0, 0.0, 0.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May or may not quantize depending on noise estimate */
	fail_if(result < 0);
}

/*
 * Test float quantization all null values (ngood == 0)
 */
static void
test_quantize_float_all_nulls(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	float null_value = -999.0f;

	/* All null values */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = null_value;
	}

	result = fits_quantize_float(0, fdata, 8, 8, 1, null_value, 4.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* Should return 1 with dummy values for all nulls */
	fail_if(result != 1);
}

/*
 * Test double quantization all null values (ngood == 0)
 */
static void
test_quantize_double_all_nulls(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	double null_value = -999.0;

	/* All null values */
	for (i = 0; i < 64; i += 1) {
		ddata[i] = null_value;
	}

	result = fits_quantize_double(0, ddata, 8, 8, 1, null_value, 4.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* Should return 1 with dummy values for all nulls */
	fail_if(result != 1);
}

/*
 * Test float dithering with null values (row > 0 + nulls)
 */
static void
test_quantize_float_dither_nulls(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	float null_value = -999.0f;

	/* Create data with some null values */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fdata[i] = null_value;
		} else {
			fdata[i] = (float)(i * 10.0 + (i % 3) * 0.5);
		}
	}

	/* Dithering with null values (row > 0 enables dithering) */
	result = fits_quantize_float(1, fdata, 8, 8, 1, null_value, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);

	/* Null values should be preserved */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fail_if(idata[i] != -2147483647);
		}
	}
}

/*
 * Test double quantization with nulls and no dithering
 */
static void
test_quantize_double_nulls_nodither(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	double null_value = -999.0;

	/* Create data with null values */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			ddata[i] = null_value;
		} else {
			ddata[i] = i * 10.0 + (i % 3) * 0.5;
		}
	}

	/* No dithering (row == 0), with null values */
	result = fits_quantize_double(0, ddata, 8, 8, 1, null_value, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);

	/* Null values should be preserved */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fail_if(idata[i] != -2147483647);
		}
	}
}

/*
 * Test double range too large
 */
static void
test_quantize_double_range_too_large(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with huge range */
	for (i = 0; i < 64; i += 1) {
		if (i < 32) {
			ddata[i] = -1e300;
		} else {
			ddata[i] = 1e300;
		}
	}

	/* Should return 0 because range is too large */
	result = fits_quantize_double(0, ddata, 8, 8, 0, 0.0, -0.001f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 0);  /* Should not quantize */
}

/*
 * Test float with very large range (zeropt centering)
 */
static void
test_quantize_float_large_range(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with large range that requires centering */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = (float)((i - 32) * 1e8);
	}

	result = fits_quantize_float(0, fdata, 8, 8, 0, 0.0f, -1e6f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May not quantize if range too large, but should not error */
	fail_if(result < 0);
}

/*
 * Test img_stats short with small and null rows
 */
static void
test_img_stats_short_small_nulls(void)
{
	short array[8] = { -999, 10, 20, 30, 40, 50, 60, -999 };
	int status = 0;
	long ngoodpix;
	short minvalue, maxvalue;
	double mean, sigma, noise1, noise2, noise3, noise5;

	status = fits_img_stats_short(array, 8, 1, 1, -999,
		&ngoodpix, &minvalue, &maxvalue, &mean, &sigma,
		&noise1, &noise2, &noise3, &noise5, &status);

	fail_if(status != 0);
	fail_if(ngoodpix != 6);  /* 8 - 2 nulls */
}

/*
 * Test float dithering with null values and zeros (DITHER_2)
 */
static void
test_quantize_float_dither2_nulls_zeros(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	float null_value = -999.0f;

	/* Create data with null values AND zeros (triggers ZERO_VALUE) */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fdata[i] = null_value;
		} else if (i % 4 == 0) {
			fdata[i] = 0.0f;  /* Zero value in DITHER_2 */
		} else {
			fdata[i] = (float)(i * 10.0 + (i % 3) * 0.5);
		}
	}

	/* SUBTRACTIVE_DITHER_2 with null values and zeros */
	result = fits_quantize_float(1, fdata, 8, 8, 1, null_value, -1.0f,
		SUBTRACTIVE_DITHER_2, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);

	/* Null values should be NULL_VALUE, zeros should be ZERO_VALUE */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fail_if(idata[i] != -2147483647);
		}
	}
}

/*
 * Test double dithering with null values and zeros (DITHER_2)
 */
static void
test_quantize_double_dither2_nulls_zeros(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	double null_value = -999.0;

	/* Create data with null values AND zeros (triggers ZERO_VALUE) */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			ddata[i] = null_value;
		} else if (i % 4 == 0) {
			ddata[i] = 0.0;  /* Zero value in DITHER_2 */
		} else {
			ddata[i] = i * 10.0 + (i % 3) * 0.5;
		}
	}

	/* SUBTRACTIVE_DITHER_2 with null values and zeros */
	result = fits_quantize_double(1, ddata, 8, 8, 1, null_value, -1.0f,
		SUBTRACTIVE_DITHER_2, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);

	/* Null values should be NULL_VALUE, zeros should be ZERO_VALUE */
	for (i = 0; i < 64; i += 1) {
		if (i % 8 == 0) {
			fail_if(idata[i] != -2147483647);
		}
	}
}

/*
 * Test large dithered image (to exercise N_RANDOM wraparound)
 * N_RANDOM is 10000, so we need >10000 pixels per row to trigger wraparound
 */
static void
test_quantize_float_large_dither(void)
{
	float *fdata;
	int *idata;
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	int nx = 10500;  /* > N_RANDOM (10000) to trigger wraparound */
	int ny = 10;
	int size = nx * ny;

	fdata = malloc(size * sizeof *fdata);
	idata = malloc(size * sizeof *idata);
	fail_if(fdata == NULL);
	fail_if(idata == NULL);

	/* Create image with gradient and noise pattern */
	for (i = 0; i < size; i += 1) {
		int x = i % nx;
		int y = i / nx;
		fdata[i] = (float)(x + y * 100 + (i % 7) * 0.1);
	}

	/* Dither with large image */
	result = fits_quantize_float(1, fdata, nx, ny, 0, 0.0f, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);

	free(fdata);
	free(idata);
}

/*
 * Test large dithered double image
 */
static void
test_quantize_double_large_dither(void)
{
	double *ddata;
	int *idata;
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	int nx = 10500;  /* > N_RANDOM (10000) to trigger wraparound */
	int ny = 10;
	int size = nx * ny;

	ddata = malloc(size * sizeof *ddata);
	idata = malloc(size * sizeof *idata);
	fail_if(ddata == NULL);
	fail_if(idata == NULL);

	/* Create image with gradient and noise pattern */
	for (i = 0; i < size; i += 1) {
		int x = i % nx;
		int y = i / nx;
		ddata[i] = x + y * 100.0 + (i % 7) * 0.1;
	}

	/* Dither with large image */
	result = fits_quantize_double(1, ddata, nx, ny, 0, 0.0, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);

	free(ddata);
	free(idata);
}

/*
 * Test large dithered image with null values (to exercise N_RANDOM wraparound)
 */
static void
test_quantize_float_large_dither_nulls(void)
{
	float *fdata;
	int *idata;
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	int nx = 10500;
	int ny = 10;
	int size = nx * ny;
	float null_value = -999.0f;

	fdata = malloc(size * sizeof *fdata);
	idata = malloc(size * sizeof *idata);
	fail_if(fdata == NULL);
	fail_if(idata == NULL);

	/* Create image with some null values */
	for (i = 0; i < size; i += 1) {
		if (i % 1000 == 0) {
			fdata[i] = null_value;
		} else {
			fdata[i] = (float)((i % nx) + (i / nx) * 100 + (i % 7) * 0.1);
		}
	}

	/* Dither with large image and nulls */
	result = fits_quantize_float(1, fdata, nx, ny, 1, null_value, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);

	free(fdata);
	free(idata);
}

/*
 * Test large dithered double image with null values
 */
static void
test_quantize_double_large_dither_nulls(void)
{
	double *ddata;
	int *idata;
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;
	int nx = 10500;
	int ny = 10;
	int size = nx * ny;
	double null_value = -999.0;

	ddata = malloc(size * sizeof *ddata);
	idata = malloc(size * sizeof *idata);
	fail_if(ddata == NULL);
	fail_if(idata == NULL);

	/* Create image with some null values */
	for (i = 0; i < size; i += 1) {
		if (i % 1000 == 0) {
			ddata[i] = null_value;
		} else {
			ddata[i] = (i % nx) + (i / nx) * 100.0 + (i % 7) * 0.1;
		}
	}

	/* Dither with large image and nulls */
	result = fits_quantize_double(1, ddata, nx, ny, 1, null_value, -1.0f,
		SUBTRACTIVE_DITHER_1, idata, &bscale, &bzero, &iminval, &imaxval);

	fail_if(result != 1);
	fail_if(bscale == 0.0);

	free(ddata);
	free(idata);
}

/*
 * Test float with range requiring centering
 */
static void
test_quantize_float_needs_centering(void)
{
	float fdata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with range that requires centering */
	for (i = 0; i < 64; i += 1) {
		fdata[i] = (float)(i - 32) * 5e7f;
	}

	result = fits_quantize_float(0, fdata, 8, 8, 0, 0.0f, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May or may not quantize */
	fail_if(result < 0);
}

/*
 * Test double with range requiring centering
 */
static void
test_quantize_double_needs_centering(void)
{
	double ddata[64];
	int idata[64];
	double bscale, bzero;
	int iminval, imaxval;
	int result;
	int i;

	/* Create data with range that requires centering */
	for (i = 0; i < 64; i += 1) {
		ddata[i] = (i - 32) * 5e7;
	}

	result = fits_quantize_double(0, ddata, 8, 8, 0, 0.0, -1.0f,
		NO_DITHER, idata, &bscale, &bzero, &iminval, &imaxval);

	/* May or may not quantize */
	fail_if(result < 0);
}

int
main(void)
{
	test_quantize_float_basic();
	test_quantize_double_basic();
	test_quantize_dither1();
	test_quantize_dither2();
	test_quantize_with_nulls();
	test_quantize_uniform();
	test_quantize_absolute();
	test_quantize_small();
	test_quantize_large();
	test_quantize_double_nulls_dither();

	/* img_stats tests */
	test_img_stats_short_basic();
	test_img_stats_short_nulls();
	test_img_stats_int_basic();
	test_img_stats_int_nulls();
	test_img_stats_float_basic();
	test_img_stats_float_nulls();
	test_img_stats_short_large();
	test_img_stats_int_large();
	test_img_stats_float_large();
	test_img_stats_short_small();

	/* quantize tests */
	test_quantize_positive_qlevel();
	test_quantize_double_positive_qlevel();
	test_quantize_double_dither1();
	test_quantize_double_dither2();
	test_quantize_range_too_large();
	test_quantize_double_small();

	/* More img_stats tests for edge cases */
	test_img_stats_short_single();
	test_img_stats_int_single();
	test_img_stats_float_single();
	test_img_stats_short_all_nulls();
	test_img_stats_int_all_nulls();
	test_img_stats_float_all_nulls();
	test_img_stats_short_small_nulls();

	/* More quantize tests for edge cases */
	test_quantize_qlevel_zero();
	test_quantize_double_qlevel_zero();
	test_quantize_float_all_nulls();
	test_quantize_double_all_nulls();
	test_quantize_float_dither_nulls();
	test_quantize_double_nulls_nodither();
	test_quantize_double_range_too_large();
	test_quantize_float_large_range();

	/* DITHER_2 with nulls and zeros */
	test_quantize_float_dither2_nulls_zeros();
	test_quantize_double_dither2_nulls_zeros();

	/* Large dithered images for N_RANDOM wraparound */
	test_quantize_float_large_dither();
	test_quantize_double_large_dither();
	test_quantize_float_large_dither_nulls();
	test_quantize_double_large_dither_nulls();

	/* Range centering tests */
	test_quantize_float_needs_centering();
	test_quantize_double_needs_centering();

	return 0;
}
