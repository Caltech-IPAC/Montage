/*
 * Tests for file manipulation functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "tg123x.kfj"

static void
test_create_file(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 4, 3 };
	unsigned char data[] = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12
	};

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 2, naxes);
	call_05(ffppr, f, TBYTE, 1, sizeof data, data);
	call_01(ffclos, f);
}


static void
test_open_and_read(void)
{
	fitsfile *f;
	int status = 0;
	unsigned char data[12];
	int anynull;

	call_03(ffopen, &f, test_path, READONLY);
	call_07(ffgpv, f, TBYTE, 1, 12, NULL, data, &anynull);
	fail_if(data[0] != 1);
	fail_if(data[5] != 6);
	fail_if(data[11] != 12);
	call_01(ffclos, f);
}


static void
test_write_keywords(fitsfile *f)
{
	int status = 0;

	call_04(ffpkys, f, "STRKEY", "hello", "string comment");
	call_04(ffpkyj, f, "LONGKEY", 42, "long comment");
	call_05(ffpkyd, f, "DBLKEY", 3.14159, 6, "double comment");
}


static void
test_read_keywords(fitsfile *f)
{
	int status = 0;
	char strval[FLEN_VALUE];
	char comment[FLEN_COMMENT];
	long longval;
	double dblval;

	call_04(ffgkys, f, "STRKEY", strval, comment);
	fail_if(strcmp(strval, "hello"));
	fail_if(strcmp(comment, "string comment"));
	call_05(ffgky, f, TLONG, "LONGKEY", &longval, comment);
	fail_if(longval != 42);
	call_05(ffgky, f, TDOUBLE, "DBLKEY", &dblval, comment);
	fail_if(dblval < 3.14 || dblval > 3.15);
}


static void
test_hdu_operations(fitsfile *f)
{
	int status = 0;
	int hdutype;
	int hdunum;
	long naxes[] = { 5, 5 };
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };

	ffghdn(f, &hdunum);
	fail_if(hdunum != 1);
	call_04(ffcrim, f, SHORT_IMG, 2, naxes);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 2);
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 3);
	call_03(ffmahd, f, 1, &hdutype);
	fail_if(hdutype != IMAGE_HDU);
	ffghdn(f, &hdunum);
	fail_if(hdunum != 1);
}


static void
test_error_handling(void)
{
	fitsfile *f;
	int status = 0;
	char errmsg[FLEN_ERRMSG];

	fail_if(ffopen(&f, "nonexistent_file.fits", READONLY, &status) == 0);
	fail_if(status == 0);
	ffgerr(status, errmsg);
	fail_if(strlen(errmsg) == 0);
	ffcmsg();
}


static void
test_keyword_types(fitsfile *f)
{
	int status = 0;
	char value[FLEN_VALUE];
	char comment[FLEN_COMMENT];

	call_04(ffpkys, f, "KEY1", "val1", "comment");
	call_04(ffgkey, f, "KEY1", value, comment);
	fail_if(strcmp(value, "'val1    '"));
	call_04(ffpkys, f, "KEY2", "", "empty");
	call_04(ffgkey, f, "KEY2", value, comment);
	fail_if(strcmp(value, "'        '"));
	call_04(ffpkys, f, "KEY3", NULL, "null");
	call_04(ffgkey, f, "KEY3", value, comment);
	fail_if(strcmp(value, "''"));
}


static void
test_raw_record(fitsfile *f)
{
	int status = 0;
	char val[FLEN_VALUE];
	char comment[FLEN_COMMENT];

	call_02(ffprec, f, "RAWKEY  = 'rawval' / raw comment");
	call_04(ffgkey, f, "RAWKEY", val, comment);
	fail_if(strcmp(val, "'rawval'"));
}


static void
test_header_space(fitsfile *f)
{
	int status = 0;
	int nkeys, morekeys;
	char keyname[FLEN_KEYWORD];
	char value[FLEN_VALUE];
	char comment[FLEN_COMMENT];

	call_03(ffghsp, f, &nkeys, &morekeys);
	fail_if(nkeys < 1);
	for (int i = 1; i <= nkeys; i += 1) {
		call_05(ffgkyn, f, i, keyname, value, comment);
	}
}


int
main(void)
{
	fitsfile *f;
	int status = 0;

	test_create_file();
	test_open_and_read();
	test_error_handling();
	call_03(ffopen, &f, test_path, READWRITE);
	test_hdu_operations(f);
	test_write_keywords(f);
	test_read_keywords(f);
	test_keyword_types(f);
	test_raw_record(f);
	test_header_space(f);
	call_01(ffclos, f);

	return 0;
}
