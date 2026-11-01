/*
 * Tests to exercise iraffits.c - IRAF to FITS conversion functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_imh_path "test_iraffits.imh"
#define test_pix_path "test_iraffits.pix"

/* IRAF format constants from iraffits.c */
#define LEN_IMHDR	2052
#define LEN_IM2HDR	2046
#define LEN_PIXHDR	1024

/* Version 1 offsets */
#define IM_PIXTYPE	16
#define IM_NDIM		20
#define IM_LEN		24
#define IM_PHYSLEN	52
#define IM_PIXOFF	88
#define IM_PIXFILE	412
#define IM_TITLE	732

/* Version 2 offsets */
#define IM2_PIXTYPE	10
#define IM2_SWAPPED	14
#define IM2_NDIM	18
#define IM2_LEN		22
#define IM2_PHYSLEN	50
#define IM2_PIXOFF	86
#define IM2_PIXFILE	126
#define IM2_TITLE	638

/* IRAF pixel types */
#define TY_SHORT	3
#define TY_INT		4
#define TY_REAL		6

/* External functions from iraffits.c */
extern int iraf2mem(char *filename, char **buffptr, size_t *buffsize,
	size_t *filesize, int *status);
extern int fits_delete_iraf_file(const char *filename, int *status);

/*
 * Write a 4-byte integer to buffer at offset (little-endian).
 */
static void
put_i4_le(char *buf, int offset, int val)
{
	buf[offset] = val & 0xff;
	buf[offset + 1] = (val >> 8) & 0xff;
	buf[offset + 2] = (val >> 16) & 0xff;
	buf[offset + 3] = (val >> 24) & 0xff;
}

/*
 * Write a string in IRAF v1 format (2 bytes per char, little-endian).
 */
static void
put_str_v1(char *buf, int offset, const char *str)
{
	int i;
	for (i = 0; str[i] != '\0'; i += 1) {
		buf[offset + i * 2] = str[i];
		buf[offset + i * 2 + 1] = 0;
	}
}

/*
 * Write a string in IRAF v2 format (1 byte per char).
 */
static void
put_str_v2(char *buf, int offset, const char *str)
{
	strcpy(buf + offset, str);
}

/*
 * Create a minimal IRAF v1 header file (.imh).
 */
static int
create_iraf_v1_header(const char *imhfile, const char *pixfile,
	int pixtype, int ndim, int *dims)
{
	FILE *fp;
	char *header;
	int i;
	int hdrlen = LEN_IMHDR + 500;

	header = (char *)calloc(hdrlen, 1);
	if (header == NULL) {
		return 1;
	}

	/* Magic word "imhdr" in 2-byte per char format. */
	put_str_v1(header, 0, "imhdr");

	/* Header length in 4-byte units. */
	put_i4_le(header, 12, hdrlen / 4);

	/* Pixel type. */
	put_i4_le(header, IM_PIXTYPE, pixtype);

	/* Number of dimensions. */
	put_i4_le(header, IM_NDIM, ndim);

	/* Image dimensions. */
	for (i = 0; i < ndim && i < 7; i += 1) {
		put_i4_le(header, IM_LEN + i * 4, dims[i]);
		put_i4_le(header, IM_PHYSLEN + i * 4, dims[i]);
	}

	/* Pixel offset (in 2-byte units, starts after LEN_PIXHDR). */
	put_i4_le(header, IM_PIXOFF, LEN_PIXHDR / 2 + 1);

	/* Pixel file name (use HDR$ prefix for same directory). */
	put_str_v1(header, IM_PIXFILE, pixfile);

	/* Title. */
	put_str_v1(header, IM_TITLE, "Test IRAF Image");

	fp = fopen(imhfile, "wb");
	if (fp == NULL) {
		free(header);
		return 1;
	}
	fwrite(header, 1, hdrlen, fp);
	fclose(fp);
	free(header);
	return 0;
}

/*
 * Create a minimal IRAF v2 header file (.imh).
 */
static int
create_iraf_v2_header(const char *imhfile, const char *pixfile,
	int pixtype, int ndim, int *dims, int swapped)
{
	FILE *fp;
	char *header;
	int i;
	int hdrlen = LEN_IM2HDR + 500;

	header = (char *)calloc(hdrlen, 1);
	if (header == NULL) {
		return 1;
	}

	/* Magic word "imhv2" in 1-byte per char format. */
	put_str_v2(header, 0, "imhv2");

	/* Header length in 4-byte units. */
	put_i4_le(header, 6, hdrlen / 4);

	/* Pixel type. */
	put_i4_le(header, IM2_PIXTYPE, pixtype);

	/* Swapped flag. */
	put_i4_le(header, IM2_SWAPPED, swapped);

	/* Number of dimensions. */
	put_i4_le(header, IM2_NDIM, ndim);

	/* Image dimensions. */
	for (i = 0; i < ndim && i < 7; i += 1) {
		put_i4_le(header, IM2_LEN + i * 4, dims[i]);
		put_i4_le(header, IM2_PHYSLEN + i * 4, dims[i]);
	}

	/* Pixel offset (in 2-byte units, starts after LEN_PIXHDR). */
	put_i4_le(header, IM2_PIXOFF, LEN_PIXHDR / 2 + 1);

	/* Pixel file name (use HDR$ prefix for same directory). */
	put_str_v2(header, IM2_PIXFILE, pixfile);

	/* Title. */
	put_str_v2(header, IM2_TITLE, "Test IRAF v2 Image");

	fp = fopen(imhfile, "wb");
	if (fp == NULL) {
		free(header);
		return 1;
	}
	fwrite(header, 1, hdrlen, fp);
	fclose(fp);
	free(header);
	return 0;
}

/*
 * Create a minimal IRAF v1 pixel file (.pix).
 */
static int
create_iraf_v1_pixfile(const char *pixfile, int pixtype, int npixels)
{
	FILE *fp;
	char *pixhdr;
	short *sdata;
	int *idata;
	float *fdata;
	int bytepix, i;

	/* Determine bytes per pixel. */
	switch (pixtype) {
	case TY_SHORT:
		bytepix = 2;
		break;
	case TY_INT:
		bytepix = 4;
		break;
	case TY_REAL:
		bytepix = 4;
		break;
	default:
		return 1;
	}

	fp = fopen(pixfile, "wb");
	if (fp == NULL) {
		return 1;
	}

	/* Write pixel header with magic word "impix". */
	pixhdr = (char *)calloc(LEN_PIXHDR, 1);
	if (pixhdr == NULL) {
		fclose(fp);
		return 1;
	}
	put_str_v1(pixhdr, 0, "impix");
	fwrite(pixhdr, 1, LEN_PIXHDR, fp);
	free(pixhdr);

	/* Write pixel data. */
	switch (pixtype) {
	case TY_SHORT:
		sdata = (short *)malloc(npixels * sizeof(short));
		if (sdata == NULL) {
			fclose(fp);
			return 1;
		}
		for (i = 0; i < npixels; i += 1) {
			sdata[i] = (short)(i % 100);
		}
		fwrite(sdata, sizeof(short), npixels, fp);
		free(sdata);
		break;
	case TY_INT:
		idata = (int *)malloc(npixels * sizeof(int));
		if (idata == NULL) {
			fclose(fp);
			return 1;
		}
		for (i = 0; i < npixels; i += 1) {
			idata[i] = i * 10;
		}
		fwrite(idata, sizeof(int), npixels, fp);
		free(idata);
		break;
	case TY_REAL:
		fdata = (float *)malloc(npixels * sizeof(float));
		if (fdata == NULL) {
			fclose(fp);
			return 1;
		}
		for (i = 0; i < npixels; i += 1) {
			fdata[i] = (float)i * 1.5f;
		}
		fwrite(fdata, sizeof(float), npixels, fp);
		free(fdata);
		break;
	}

	fclose(fp);
	return 0;
}

/*
 * Create a minimal IRAF v2 pixel file (.pix).
 */
static int
create_iraf_v2_pixfile(const char *pixfile, int pixtype, int npixels)
{
	FILE *fp;
	char *pixhdr;
	short *sdata;
	int i;

	fp = fopen(pixfile, "wb");
	if (fp == NULL) {
		return 1;
	}

	/* Write pixel header with magic word "impv2". */
	pixhdr = (char *)calloc(LEN_PIXHDR, 1);
	if (pixhdr == NULL) {
		fclose(fp);
		return 1;
	}
	strcpy(pixhdr, "impv2");
	fwrite(pixhdr, 1, LEN_PIXHDR, fp);
	free(pixhdr);

	/* Write pixel data (just short for simplicity). */
	sdata = (short *)malloc(npixels * sizeof(short));
	if (sdata == NULL) {
		fclose(fp);
		return 1;
	}
	for (i = 0; i < npixels; i += 1) {
		sdata[i] = (short)(i % 256);
	}
	fwrite(sdata, sizeof(short), npixels, fp);
	free(sdata);

	fclose(fp);
	return 0;
}

static void
cleanup_files(void)
{
	remove(test_imh_path);
	remove(test_pix_path);
}

static void
test_iraf2mem_v1_short(void)
{
	/* Test reading IRAF v1 format with short pixels. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};

	cleanup_files();

	/* Create IRAF v1 files. */
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 100) != 0);

	/* Read into memory. */
	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	/* Check results. */
	fail_if(status != 0);
	fail_if(buffer == NULL);
	fail_if(filesize == 0);

	/* Verify it's valid FITS by checking SIMPLE keyword. */
	fail_if(strncmp(buffer, "SIMPLE", 6) != 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_v1_int(void)
{
	/* Test reading IRAF v1 format with int pixels. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {8, 8};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_INT, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_INT, 64) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);
	fail_if(strncmp(buffer, "SIMPLE", 6) != 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_v1_real(void)
{
	/* Test reading IRAF v1 format with float pixels. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {5, 5};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_REAL, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_REAL, 25) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_v2(void)
{
	/* Test reading IRAF v2 format. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};

	cleanup_files();

	fail_if(create_iraf_v2_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims, 0) != 0);
	fail_if(create_iraf_v2_pixfile(test_pix_path, TY_SHORT, 100) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);
	fail_if(strncmp(buffer, "SIMPLE", 6) != 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_1d(void)
{
	/* Test reading 1-dimensional IRAF image. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[1] = {50};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 1, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 50) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_missing_header(void)
{
	/* Test error handling for missing header file. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;

	cleanup_files();

	iraf2mem("nonexistent.imh", &buffer, &buffsize, &filesize, &status);

	/* Should return FILE_NOT_OPENED (104). */
	fail_if(status == 0);
	fail_if(buffer != NULL);
}

static void
test_iraf2mem_invalid_header(void)
{
	/* Test error handling for invalid header file. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	FILE *fp;
	char junk[2100];

	cleanup_files();

	/* Create a file with invalid content. */
	memset(junk, 'X', sizeof(junk));
	fp = fopen(test_imh_path, "wb");
	fail_if(fp == NULL);
	fwrite(junk, 1, sizeof(junk), fp);
	fclose(fp);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	/* Should fail because magic word is invalid. */
	fail_if(status == 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_missing_pixfile(void)
{
	/* Test error handling for missing pixel file. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};

	cleanup_files();

	/* Create header but no pixel file. */
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	/* Should fail because pixel file is missing. */
	fail_if(status == 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_fits_delete_iraf_file(void)
{
	/* Test deleting IRAF files. */
	int status = 0;
	int dims[2] = {10, 10};
	FILE *fp;

	cleanup_files();

	/* Create IRAF files. */
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 100) != 0);

	/* Verify files exist. */
	fp = fopen(test_imh_path, "r");
	fail_if(fp == NULL);
	fclose(fp);
	fp = fopen(test_pix_path, "r");
	fail_if(fp == NULL);
	fclose(fp);

	/* Delete them. */
	fits_delete_iraf_file(test_imh_path, &status);
	fail_if(status != 0);

	/* Verify files are gone. */
	fp = fopen(test_imh_path, "r");
	fail_if(fp != NULL);
	fp = fopen(test_pix_path, "r");
	fail_if(fp != NULL);
}

static void
test_fits_delete_iraf_file_missing(void)
{
	/* Test error handling for deleting non-existent file. */
	int status = 0;

	cleanup_files();

	fits_delete_iraf_file("nonexistent.imh", &status);

	/* Should return error. */
	fail_if(status == 0);
}

static void
test_iraf2mem_header_too_small(void)
{
	/* Test error handling for header file that is too small. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	FILE *fp;
	char small[100];

	cleanup_files();

	/* Create a file that is too small to be valid. */
	memset(small, 0, sizeof(small));
	fp = fopen(test_imh_path, "wb");
	fail_if(fp == NULL);
	fwrite(small, 1, sizeof(small), fp);
	fclose(fp);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	/* Should fail because file is too small. */
	fail_if(status == 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_v2_swapped(void)
{
	/* Test reading IRAF v2 format with swapped pixels. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};

	cleanup_files();

	/* Create with swapped=1. */
	fail_if(create_iraf_v2_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims, 1) != 0);
	fail_if(create_iraf_v2_pixfile(test_pix_path, TY_SHORT, 100) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_3d(void)
{
	/* Test reading 3-dimensional IRAF image. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[3] = {4, 4, 4};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 3, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 64) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_4d(void)
{
	/* Test reading 4-dimensional IRAF image. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[4] = {2, 2, 2, 2};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 4, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 16) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_invalid_pixfile(void)
{
	/* Test error handling for pixel file with invalid magic. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};
	FILE *fp;
	char *pixhdr;

	cleanup_files();

	/* Create valid header. */
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims) != 0);

	/* Create pixel file with invalid magic. */
	fp = fopen(test_pix_path, "wb");
	fail_if(fp == NULL);
	pixhdr = (char *)calloc(LEN_PIXHDR, 1);
	fail_if(pixhdr == NULL);
	/* Write invalid magic word. */
	put_str_v1(pixhdr, 0, "XXXXX");
	fwrite(pixhdr, 1, LEN_PIXHDR, fp);
	free(pixhdr);
	fclose(fp);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	/* Should fail because pixel file magic is invalid. */
	fail_if(status == 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_truncated_pixfile(void)
{
	/* Test error handling for truncated pixel file. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};
	FILE *fp;
	char *pixhdr;

	cleanup_files();

	/* Create valid header expecting 100 pixels. */
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims) != 0);

	/* Create pixel file with only the header, no data. */
	fp = fopen(test_pix_path, "wb");
	fail_if(fp == NULL);
	pixhdr = (char *)calloc(LEN_PIXHDR, 1);
	fail_if(pixhdr == NULL);
	put_str_v1(pixhdr, 0, "impix");
	fwrite(pixhdr, 1, LEN_PIXHDR, fp);
	free(pixhdr);
	/* Don't write any pixel data - file is truncated. */
	fclose(fp);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	/* Should fail because pixel file is truncated. */
	fail_if(status == 0);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_large_image(void)
{
	/* Test reading a larger IRAF image. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {100, 100};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 10000) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_5d(void)
{
	/* Test reading 5-dimensional IRAF image. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[5] = {2, 2, 2, 2, 2};

	cleanup_files();

	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_SHORT, 5, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_SHORT, 32) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);

	fail_if(status != 0);
	fail_if(buffer == NULL);

	if (buffer != NULL) {
		free(buffer);
	}
	cleanup_files();
}

static void
test_iraf2mem_v1_different_pixtypes(void)
{
	/* Test IRAF v1 format with different pixel types in sequence. */
	int status = 0;
	char *buffer = NULL;
	size_t buffsize = 0, filesize = 0;
	int dims[2] = {10, 10};

	/* Test with TY_INT = 4. */
	cleanup_files();
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_INT, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_INT, 100) != 0);

	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);
	fail_if(status != 0);
	fail_if(buffer == NULL);
	if (buffer != NULL) {
		free(buffer);
	}
	buffer = NULL;

	/* Test with TY_REAL = 6. */
	cleanup_files();
	fail_if(create_iraf_v1_header(test_imh_path, "HDR$test_iraffits.pix",
		TY_REAL, 2, dims) != 0);
	fail_if(create_iraf_v1_pixfile(test_pix_path, TY_REAL, 100) != 0);

	status = 0;
	iraf2mem(test_imh_path, &buffer, &buffsize, &filesize, &status);
	fail_if(status != 0);
	fail_if(buffer == NULL);
	if (buffer != NULL) {
		free(buffer);
	}

	cleanup_files();
}

int
main(void)
{
	test_iraf2mem_v1_short();
	test_iraf2mem_v1_int();
	test_iraf2mem_v1_real();
	test_iraf2mem_v2();
	test_iraf2mem_v2_swapped();
	test_iraf2mem_1d();
	test_iraf2mem_3d();
	test_iraf2mem_4d();
	test_iraf2mem_5d();
	test_iraf2mem_missing_header();
	test_iraf2mem_invalid_header();
	test_iraf2mem_missing_pixfile();
	test_iraf2mem_header_too_small();
	test_iraf2mem_invalid_pixfile();
	test_iraf2mem_truncated_pixfile();
	test_iraf2mem_large_image();
	test_iraf2mem_v1_different_pixtypes();
	test_fits_delete_iraf_file();
	test_fits_delete_iraf_file_missing();
	return 0;
}
