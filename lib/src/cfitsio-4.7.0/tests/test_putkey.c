/*
 * Tests for putkey.c - keyword writing functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_putkey.fits"


static void
test_putkey_string(void)
{
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkys, f, "STRKEY", "test value", "string keyword");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "STRKEY", sval, NULL);
	fail_if(strcmp(sval, "test value") != 0);
	call_01(ffclos, f);
}


static void
test_putkey_long(void)
{
	fitsfile *f;
	int status = 0;
	long lval;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "LONGKEY", 123456789, "long keyword");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffgky, f, TLONG, "LONGKEY", &lval, NULL);
	fail_if(lval != 123456789);
	call_01(ffclos, f);
}


static void
test_putkey_double(void)
{
	fitsfile *f;
	int status = 0;
	double dval;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyd, f, "DBLKEY", 3.14159265358979, 15, "double keyword");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffgky, f, TDOUBLE, "DBLKEY", &dval, NULL);
	fail_if(dval < 3.14 || dval > 3.15);
	call_01(ffclos, f);
}


static void
test_putkey_logical(void)
{
	fitsfile *f;
	int status = 0;
	int lval;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyl, f, "BOOLKEY", 1, "logical keyword");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkyl, f, "BOOLKEY", &lval, NULL);
	fail_if(lval != 1);
	call_01(ffclos, f);
}


static void
test_putkey_float(void)
{
	fitsfile *f;
	int status = 0;
	float fval;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkye, f, "FLTKEY", 2.71828f, 6, "float keyword");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffgky, f, TFLOAT, "FLTKEY", &fval, NULL);
	fail_if(fval < 2.71 || fval > 2.72);
	call_01(ffclos, f);
}


/*
 * Test ffpky with various data types via the generic interface.
 */
static void
test_putkey_generic_types(void)
{
	fitsfile *f;
	int status = 0;
	signed char sbval = -42;
	unsigned short usval = 12345;
	unsigned int uival = 3000000000U;
	LONGLONG llval = 9876543210LL;
	float fval = 1.5f;
	double dval = 2.5;
	float cval[2] = { 1.0f, 2.0f };
	double mval[2] = { 3.0, 4.0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	call_05(ffpky, f, TSBYTE, "SBKEY", &sbval, "signed byte");
	call_05(ffpky, f, TUSHORT, "USKEY", &usval, "unsigned short");
	call_05(ffpky, f, TUINT, "UIKEY", &uival, "unsigned int");
	call_05(ffpky, f, TLONGLONG, "LLKEY", &llval, "longlong");
	call_05(ffpky, f, TFLOAT, "FKEY", &fval, "float");
	call_05(ffpky, f, TDOUBLE, "DKEY", &dval, "double");
	call_05(ffpky, f, TCOMPLEX, "CKEY", cval, "complex");
	call_05(ffpky, f, TDBLCOMPLEX, "MKEY", mval, "dblcomplex");

	call_01(ffclos, f);
}


/*
 * Test unsigned long and ulonglong keywords.
 */
static void
test_putkey_unsigned_long(void)
{
	fitsfile *f;
	int status = 0;
	unsigned long ulval = 4000000000UL;
	ULONGLONG ullval = 18000000000000000000ULL;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	call_05(ffpky, f, TULONG, "ULKEY", &ulval, "unsigned long");
	call_05(ffpky, f, TULONGLONG, "ULLKEY", &ullval, "ulonglong");

	call_01(ffclos, f);
}


/*
 * Test writing keywords with null/undefined values.
 */
static void
test_putkey_null(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_03(ffpkyu, f, "NULLKEY", "undefined value");
	call_01(ffclos, f);
}


/*
 * Test writing keyword units.
 */
static void
test_putkey_units(void)
{
	fitsfile *f;
	int status = 0;
	char units[FLEN_VALUE];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "EXPTIME", 300, "Exposure time");
	call_03(ffpunt, f, "EXPTIME", "seconds");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffgunt, f, "EXPTIME", units);
	fail_if(strcmp(units, "seconds") != 0);
	call_01(ffclos, f);
}


/*
 * Test writing fixed-format keywords (G format).
 */
static void
test_putkey_fixfmt(void)
{
	fitsfile *f;
	int status = 0;
	double dval = 123.456789;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyg, f, "GKEY", dval, 10, "G format");
	call_01(ffclos, f);
}


/*
 * Test writing fixed-format float keywords (F format).
 */
static void
test_putkey_fixfmt_float(void)
{
	fitsfile *f;
	int status = 0;
	double dval = 987.654321;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyf, f, "FKEY", dval, 6, "F format");
	call_01(ffclos, f);
}


/*
 * Test writing complex keywords.
 */
static void
test_putkey_complex(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = { 1.5f, -2.5f };
	double mval[2] = { 3.14159, -2.71828 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyc, f, "CFLT", cval, 6, "complex float");
	call_05(ffpkym, f, "CDBL", mval, 14, "complex double");
	call_01(ffclos, f);
}


/*
 * Test writing fixed complex keywords.
 */
static void
test_putkey_complex_fixed(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = { 100.0f, 200.0f };
	double mval[2] = { 300.0, 400.0 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkfc, f, "FCFLT", cval, 4, "fixed complex float");
	call_05(ffpkfm, f, "FCDBL", mval, 8, "fixed complex double");
	call_01(ffclos, f);
}


/*
 * Test writing comment and history keywords.
 */
static void
test_putkey_comment_history(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_02(ffpcom, f, "This is a comment keyword");
	call_02(ffphis, f, "This is a history keyword");
	call_01(ffclos, f);
}


/*
 * Test writing date keywords.
 */
static void
test_putkey_date(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffpdat, f);
	call_01(ffclos, f);
}


/*
 * Test updating keywords.
 */
static void
test_update_key(void)
{
	fitsfile *f;
	int status = 0;
	long lval;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkyj, f, "UPDKEY", 100, "original");
	call_04(ffukyj, f, "UPDKEY", 200, "updated");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_05(ffgky, f, TLONG, "UPDKEY", &lval, NULL);
	fail_if(lval != 200);
	call_01(ffclos, f);
}


/*
 * Test indexed array keywords (ffpkns, ffpknl, ffpknj, etc.)
 */
static void
test_putkey_indexed_arrays(void)
{
	fitsfile *f;
	int status = 0;
	char *svals[3] = { "val1", "val2", "val3" };
	int lvals[3] = { 1, 0, 1 };
	long jvals[3] = { 100, 200, 300 };
	LONGLONG jjvals[3] = { 1000000000LL, 2000000000LL, 3000000000LL };
	float fvals[3] = { 1.1f, 2.2f, 3.3f };
	float evals[3] = { 1.0e10f, 2.0e20f, 3.0e30f };
	double gvals[3] = { 1.111, 2.222, 3.333 };
	double dvals[3] = { 1.0e100, 2.0e200, 3.0e-100 };
	char *comm[3] = { "comment &", "comment 2", "comment 3" };
	char *comm_norep[3] = { "comm1", "comm2", "comm3" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test with repeated comment (ending with &) */
	call_06(ffpkns, f, "SKEY", 1, 3, svals, comm);
	call_06(ffpknl, f, "LKEY", 1, 3, lvals, comm);
	call_06(ffpknj, f, "JKEY", 1, 3, jvals, comm);
	call_06(ffpknjj, f, "JJKEY", 1, 3, jjvals, NULL);  /* Test with NULL comment */
	call_07(ffpknf, f, "FKEY", 1, 3, fvals, 2, comm);
	call_07(ffpkne, f, "EKEY", 1, 3, evals, 3, comm);
	call_07(ffpkng, f, "GKEY", 1, 3, gvals, 4, comm);
	call_07(ffpknd, f, "DKEY", 1, 3, dvals, 5, comm);

	/* Test with non-repeated comments */
	call_06(ffpknl, f, "LKEY2", 1, 3, lvals, comm_norep);
	call_06(ffpknj, f, "JKEY2", 1, 3, jvals, comm_norep);

	call_01(ffclos, f);
}


/*
 * Test long string keywords (ffpkls).
 */
static void
test_putkey_long_string(void)
{
	fitsfile *f;
	int status = 0;
	char longstr[200];
	char *result = NULL;

	/* Create a string longer than 68 chars to trigger CONTINUE keywords */
	strcpy(longstr, "This is a very long string value that exceeds the normal "
		"FITS keyword value length limit of 68 characters and requires continuation");

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkls, f, "LONGSTR", longstr, "long string test");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkls, f, "LONGSTR", &result, NULL);
	fail_if(result == NULL || strcmp(result, longstr) != 0);
	if (result) {
		free(result);
	}
	call_01(ffclos, f);
}


/*
 * Test template file keywords (ffpktp).
 */
static void
test_putkey_template(void)
{
	fitsfile *f;
	int status = 0;
	FILE *tpl;
	char sval[FLEN_VALUE];

	/* Create a template file */
	tpl = fopen("test_putkey.tpl", "w");
	if (tpl) {
		fprintf(tpl, "TPLKEY1 = 'template value' / added by template\n");
		fprintf(tpl, "TPLKEY2 = 42 / integer from template\n");
		fprintf(tpl, "END\n");
		fclose(tpl);
	}

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_02(ffpktp, f, "test_putkey.tpl");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "TPLKEY1", sval, NULL);
	fail_if(strcmp(sval, "template value") != 0);
	call_01(ffclos, f);

	remove("test_putkey.tpl");
}


/*
 * Test date/time string conversion functions.
 */
static void
test_putkey_datetime(void)
{
	int status = 0;
	int yr, mon, day, hr, min;
	double sec;
	char datestr[30], timestr[30];

	/* Test ffdt2s - convert date to string (new format) */
	call_04(ffdt2s, 2024, 1, 15, datestr);
	fail_if(strcmp(datestr, "2024-01-15") != 0);

	/* Test ffdt2s - old format date (year 1900-1998) */
	call_04(ffdt2s, 1995, 6, 15, datestr);
	fail_if(strcmp(datestr, "15/06/95") != 0);

	/* Test ffs2dt - convert string to date */
	call_04(ffs2dt, "2024-06-20", &yr, &mon, &day);
	fail_if(yr != 2024 || mon != 6 || day != 20);

	/* Test old-style date format */
	call_04(ffs2dt, "15/03/99", &yr, &mon, &day);
	fail_if(yr != 1999 || mon != 3 || day != 15);

	/* Test fftm2s - convert date-time to string */
	call_08(fftm2s, 2024, 7, 4, 12, 30, 45.5, 3, timestr);
	fail_if(strncmp(timestr, "2024-07-04T12:30:45", 19) != 0);

	/* Test fftm2s with 0 decimals */
	call_08(fftm2s, 2024, 8, 15, 10, 20, 30.0, 0, timestr);
	fail_if(strncmp(timestr, "2024-08-15T10:20:30", 19) != 0);

	/* Test fftm2s with negative decimals (date only) */
	call_08(fftm2s, 2024, 9, 1, 0, 0, 0.0, -1, timestr);
	fail_if(strcmp(timestr, "2024-09-01") != 0);

	/* Test fftm2s with time only (year=month=day=0) */
	call_08(fftm2s, 0, 0, 0, 14, 30, 45.5, 3, timestr);
	fail_if(strncmp(timestr, "14:30:45", 8) != 0);

	/* Test ffs2tm - convert string to date-time */
	call_07(ffs2tm, "2024-12-25T08:15:30.123", &yr, &mon, &day, &hr, &min, &sec);
	fail_if(yr != 2024 || mon != 12 || day != 25);
	fail_if(hr != 8 || min != 15);

	/* Test ffs2tm with date only */
	call_07(ffs2tm, "2024-06-15", &yr, &mon, &day, &hr, &min, &sec);
	fail_if(yr != 2024 || mon != 6 || day != 15);

	/* Test ffs2tm with time only format */
	status = 0;
	call_07(ffs2tm, "10:30:45.5", &yr, &mon, &day, &hr, &min, &sec);

	/* Test ffgstm - get system time */
	status = 0;
	call_02(ffgstm, timestr, NULL);
	fail_if(strlen(timestr) < 10);
}


/*
 * Test dimension keywords (ffptdm).
 */
static void
test_putkey_tdim(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[3] = { 10, 20, 30 };
	char *ttype[1] = { "DATA" };
	char *tform[1] = { "6000J" };

	call_02(ffinit, &f, "!" test_path);

	/* Create a binary table */
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);

	/* Write TDIM keyword with long */
	call_04(ffptdm, f, 1, 3, naxes);

	call_01(ffclos, f);
}


/*
 * Test time keyword (ffpkyt).
 */
static void
test_putkey_time(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_05(ffpkyt, f, "TIMEKEY", 12345, 0.6789, "time value");
	call_01(ffclos, f);
}


/*
 * Test raw record writing (ffprec).
 */
static void
test_putkey_rawrec(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Write a raw 80-character card */
	strcpy(card, "RAWKEY  = 'raw value'          / written as raw record                         ");
	call_02(ffprec, f, card);

	call_01(ffclos, f);
}


/*
 * Test date verification (ffverifydate).
 */
static void
test_putkey_verifydate(void)
{
	int status = 0;

	/* Valid dates */
	call_03(ffverifydate, 2024, 1, 1);
	call_03(ffverifydate, 2024, 12, 31);
	call_03(ffverifydate, 2000, 2, 29);  /* leap year */

	/* Test boundary conditions */
	call_03(ffverifydate, 1999, 1, 1);
	call_03(ffverifydate, 9999, 12, 31);
}


/*
 * Test system date (ffgsdt).
 */
static void
test_putkey_sysdate(void)
{
	int status = 0;
	int day, month, year;

	call_03(ffgsdt, &day, &month, &year);
	fail_if(year < 2020 || year > 2100);
	fail_if(month < 1 || month > 12);
	fail_if(day < 1 || day > 31);
}


/*
 * Test inserting keywords at specific positions (ffirec).
 */
static void
test_putkey_insert(void)
{
	fitsfile *f;
	int status = 0;
	char card[FLEN_CARD];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Add some keywords first */
	call_04(ffpkyj, f, "KEY1", 1, "first key");
	call_04(ffpkyj, f, "KEY3", 3, "third key");

	/* Insert a keyword between them */
	strcpy(card, "KEY2    =                    2 / second key                                     ");
	call_03(ffirec, f, 4, card);

	call_01(ffclos, f);
}




/*
 * Test header writing functions (ffphpr, ffphprll).
 */
static void
test_putkey_header(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[2] = { 100, 100 };
	LONGLONG naxesll[2] = { 200, 200 };

	/* Test ffphpr */
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, 16, 2, naxes, 0, 1, 1);
	call_01(ffclos, f);

	/* Test ffphprll */
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphprll, f, 1, 16, 2, naxesll, 0, 1, 1);
	call_01(ffclos, f);
}


/*
 * Test table header functions (ffphtb, ffphbn).
 */
static void
test_putkey_table_headers(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[2] = { "COL1", "COL2" };
	char *tform_ascii[2] = { "A10", "I5" };
	char *tform_bin[2] = { "10A", "1J" };
	char *tunit[2] = { "unit1", "unit2" };
	long tbcol[2] = { 1, 11 };

	/* Test ffphtb (ASCII table header) */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);  /* Need primary */
	call_01(ffcrhd, f);  /* Create extension */
	call_09(ffphtb, f, 20, 10, 2, ttype, tbcol, tform_ascii, tunit, "TESTTBL");
	call_01(ffclos, f);

	/* Test ffphbn (binary table header) */
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 10, 2, ttype, tform_bin, tunit, "BINTBL", 0);
	call_01(ffclos, f);
}


/*
 * Test extension header (ffphext).
 */
static void
test_putkey_exthead(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[2] = { 50, 50 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffcrhd, f);
	call_07(ffphext, f, "IMAGE", 16, 2, naxes, 0, 1);
	call_01(ffclos, f);
}


/*
 * Test more update functions (ffuky*).
 */
static void
test_putkey_updates(void)
{
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];
	double dval;
	float fval;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Write and update string */
	call_04(ffpkys, f, "STRKEY", "original", "string comment");
	call_04(ffukys, f, "STRKEY", "updated", "new comment");

	/* Write and update float (E format) */
	call_05(ffpkye, f, "FLTKEY", 1.5f, 3, "float comment");
	call_05(ffukye, f, "FLTKEY", 2.5f, 3, "updated float");

	/* Write and update double (D format) */
	call_05(ffpkyd, f, "DBLKEY", 1.5, 6, "double comment");
	call_05(ffukyd, f, "DBLKEY", 2.5, 6, "updated double");

	/* Write and update fixed format G */
	call_05(ffpkyg, f, "GKEY", 100.5, 4, "G format");
	call_05(ffukyg, f, "GKEY", 200.5, 4, "updated G");

	/* Write and update fixed format F */
	call_05(ffpkyf, f, "FKEY", 100.5f, 3, "F format");
	call_05(ffukyf, f, "FKEY", 200.5f, 3, "updated F");

	call_01(ffclos, f);

	/* Verify updates */
	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "STRKEY", sval, NULL);
	fail_if(strcmp(sval, "updated") != 0);
	call_05(ffgky, f, TDOUBLE, "DBLKEY", &dval, NULL);
	fail_if(dval < 2.4 || dval > 2.6);
	call_05(ffgky, f, TFLOAT, "FLTKEY", &fval, NULL);
	fail_if(fval < 2.4f || fval > 2.6f);
	call_01(ffclos, f);
}


/*
 * Test longlong image and create image extension (ffcrimll).
 */
static void
test_putkey_image_ext(void)
{
	fitsfile *f;
	int status = 0;
	LONGLONG naxesll[2] = { 10, 10 };

	call_02(ffinit, &f, "!" test_path);

	/* Create primary array first */
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Create image extension with LONGLONG axes */
	call_04(ffcrimll, f, SHORT_IMG, 2, naxesll);

	call_01(ffclos, f);
}


/*
 * Test special image types (unsigned, signed byte).
 */
static void
test_putkey_special_images(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[2] = { 10, 10 };

	/* Test unsigned short image */
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, USHORT_IMG, 2, naxes, 0, 1, 1);
	call_01(ffclos, f);

	/* Test unsigned long image */
	status = 0;
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, ULONG_IMG, 2, naxes, 0, 1, 1);
	call_01(ffclos, f);

	/* Test signed byte image */
	status = 0;
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, SBYTE_IMG, 2, naxes, 0, 1, 1);
	call_01(ffclos, f);

	/* Test unsigned longlong image */
	status = 0;
	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, ULONGLONG_IMG, 2, naxes, 0, 1, 1);
	call_01(ffclos, f);
}


/*
 * Test write long string warning (ffplsw).
 */
static void
test_putkey_longstrn(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* This writes the LONGSTRN keyword and COMMENT keywords */
	call_01(ffplsw, f);

	/* Calling again should return without writing (keyword exists) */
	call_01(ffplsw, f);

	call_01(ffclos, f);
}


/*
 * Test random groups (pcount > 0, gcount > 1).
 */
static void
test_putkey_random_groups(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[2] = { 10, 10 };

	call_02(ffinit, &f, "!" test_path);

	/* Write header with random groups (pcount=2, gcount=3) */
	call_08(ffphpr, f, 1, 16, 2, naxes, 2, 3, 1);

	call_01(ffclos, f);
}


/*
 * Test bad datatype error in ffpky.
 */
static void
test_putkey_bad_datatype(void)
{
	fitsfile *f;
	int status = 0;
	int value = 42;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Use an invalid datatype code */
	status = 0;
	ffpky(f, 9999, "BADTYPE", &value, "bad datatype", &status);
	fail_if(status != BAD_DATATYPE);

	status = 0;
	call_01(ffclos, f);
}


/*
 * Test template file not found.
 */
static void
test_putkey_template_notfound(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Try to open a template file that doesn't exist */
	status = 0;
	ffpktp(f, "nonexistent_template.tpl", &status);
	fail_if(status != FILE_NOT_OPENED);

	status = 0;
	call_01(ffclos, f);
}


/*
 * Test date parsing errors.
 */
static void
test_putkey_date_errors(void)
{
	int status = 0;
	int yr, mon, day, hr, min;
	double sec;

	/* Test invalid date formats for ffs2dt */
	status = 0;
	ffs2dt("invalid", &yr, &mon, &day, &status);
	fail_if(status != BAD_DATE);

	/* Test null input */
	status = 0;
	ffs2dt(NULL, &yr, &mon, &day, &status);
	fail_if(status != BAD_DATE);

	/* Test too short string */
	status = 0;
	ffs2dt("12", &yr, &mon, &day, &status);
	fail_if(status != BAD_DATE);

	/* Test invalid old format (wrong separator) */
	status = 0;
	ffs2dt("12-34-56", &yr, &mon, &day, &status);
	fail_if(status != BAD_DATE);

	/* Test ffs2tm with null input */
	status = 0;
	ffs2tm(NULL, &yr, &mon, &day, &hr, &min, &sec, &status);
	fail_if(status != BAD_DATE);

	/* Test ffs2tm with invalid format */
	status = 0;
	ffs2tm("not-a-date", &yr, &mon, &day, &hr, &min, &sec, &status);
	fail_if(status != BAD_DATE);

	/* Test ffs2tm with invalid time format */
	status = 0;
	ffs2tm("12:34", &yr, &mon, &day, &hr, &min, &sec, &status);
	fail_if(status != BAD_DATE);

	/* Test ffs2tm with invalid date-time separator */
	status = 0;
	ffs2tm("2024-01-01X12:00:00", &yr, &mon, &day, &hr, &min, &sec, &status);
	fail_if(status != BAD_DATE);
}


/*
 * Test time to string conversion errors (fftm2s).
 */
static void
test_putkey_time_errors(void)
{
	int status = 0;
	char timestr[40];

	/* Test invalid year (> 9999) */
	status = 0;
	fftm2s(10000, 1, 1, 0, 0, 0.0, 0, timestr, &status);
	fail_if(status != BAD_DATE);

	/* Test invalid month (> 12) */
	status = 0;
	fftm2s(2024, 13, 1, 0, 0, 0.0, 0, timestr, &status);
	fail_if(status != BAD_DATE);

	/* Test invalid day (> 31) */
	status = 0;
	fftm2s(2024, 1, 32, 0, 0, 0.0, 0, timestr, &status);
	fail_if(status != BAD_DATE);

	/* Test invalid hour (> 23) */
	status = 0;
	fftm2s(2024, 1, 1, 24, 0, 0.0, 0, timestr, &status);
	fail_if(status != BAD_DATE);

	/* Test invalid minute (> 59) */
	status = 0;
	fftm2s(2024, 1, 1, 0, 60, 0.0, 0, timestr, &status);
	fail_if(status != BAD_DATE);

	/* Test invalid second (>= 61) */
	status = 0;
	fftm2s(2024, 1, 1, 0, 0, 61.0, 0, timestr, &status);
	fail_if(status != BAD_DATE);
}


/*
 * Test date verification errors (ffverifydate).
 */
static void
test_putkey_verifydate_errors(void)
{
	int status = 0;

	/* Invalid year */
	status = 0;
	ffverifydate(10000, 1, 1, &status);
	fail_if(status != BAD_DATE);

	/* Invalid month high */
	status = 0;
	ffverifydate(2024, 13, 1, &status);
	fail_if(status != BAD_DATE);

	/* Invalid month low */
	status = 0;
	ffverifydate(2024, 0, 1, &status);
	fail_if(status != BAD_DATE);

	/* Invalid day high */
	status = 0;
	ffverifydate(2024, 1, 32, &status);
	fail_if(status != BAD_DATE);

	/* Invalid day low */
	status = 0;
	ffverifydate(2024, 1, 0, &status);
	fail_if(status != BAD_DATE);

	/* Invalid Feb 29 in non-leap year */
	status = 0;
	ffverifydate(2023, 2, 29, &status);
	fail_if(status != BAD_DATE);

	/* Valid Feb 29 in leap year */
	status = 0;
	call_03(ffverifydate, 2024, 2, 29);
}


/*
 * Test date to string errors (ffdt2s).
 */
static void
test_putkey_dt2s_errors(void)
{
	int status = 0;
	char datestr[30];

	/* Test invalid date */
	status = 0;
	ffdt2s(2024, 13, 1, datestr, &status);
	fail_if(status != BAD_DATE);
}


/*
 * Test ffpkyt errors.
 */
static void
test_putkey_time_keyword_errors(void)
{
	fitsfile *f;
	int status = 0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test fraction out of range (negative) */
	status = 0;
	ffpkyt(f, "BADTIME1", 12345, -0.5, "bad fraction", &status);
	fail_if(status != BAD_F2C);

	/* Test fraction out of range (> 1) */
	status = 0;
	ffpkyt(f, "BADTIME2", 12345, 1.5, "bad fraction", &status);
	fail_if(status != BAD_F2C);

	status = 0;
	call_01(ffclos, f);
}


/*
 * Test TDIM errors.
 */
static void
test_putkey_tdim_errors(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[3] = { 10, 20, 30 };
	char *ttype[1] = { "DATA" };
	char *tform[1] = { "6000J" };

	call_02(ffinit, &f, "!" test_path);
	call_08(ffcrtb, f, BINARY_TBL, 0, 1, ttype, tform, NULL, NULL);

	/* Test column number out of range (0) */
	status = 0;
	ffptdm(f, 0, 3, naxes, &status);
	fail_if(status != BAD_COL_NUM);

	/* Test column number out of range (1000) */
	status = 0;
	ffptdm(f, 1000, 3, naxes, &status);
	fail_if(status != BAD_COL_NUM);

	/* Test naxis < 1 */
	status = 0;
	ffptdm(f, 1, 0, naxes, &status);
	fail_if(status != BAD_DIMEN);

	/* Test negative axis value */
	long bad_naxes[3] = { 10, -20, 30 };
	status = 0;
	ffptdm(f, 1, 3, bad_naxes, &status);
	fail_if(status != BAD_TDIM);

	status = 0;
	call_01(ffclos, f);
}


/*
 * Test NOT_TABLE error in ffptdm on non-table HDU.
 */
static void
test_putkey_tdim_nottable(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[2] = { 10, 10 };
	long tdim[2] = { 5, 20 };

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, 16, 2, naxes, 0, 1, 1);

	/* Try to write TDIM on image HDU - should fail */
	status = 0;
	ffptdm(f, 1, 2, tdim, &status);
	fail_if(status != NOT_BTABLE);

	status = 0;
	call_01(ffclos, f);
}


/*
 * Test very long string keyword that requires many CONTINUE cards.
 */
static void
test_putkey_very_long_string(void)
{
	fitsfile *f;
	int status = 0;
	char longstr[500];
	char *result = NULL;

	/* Create a string that requires multiple CONTINUE cards */
	memset(longstr, 'x', 400);
	longstr[400] = '\0';

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkls, f, "VERYLONG", longstr, "very long string");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkls, f, "VERYLONG", &result, NULL);
	fail_if(result == NULL || strcmp(result, longstr) != 0);
	if (result) {
		free(result);
	}
	call_01(ffclos, f);
}


/*
 * Test long string with embedded quotes.
 */
static void
test_putkey_long_string_quotes(void)
{
	fitsfile *f;
	int status = 0;
	char longstr[200];
	char *result = NULL;

	/* Create a long string with embedded quotes */
	strcpy(longstr, "This is a string with 'quotes' and more 'quotes' "
		"that needs to be properly escaped when written");

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkls, f, "QUOTSTR", longstr, "string with quotes");
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkls, f, "QUOTSTR", &result, NULL);
	fail_if(result == NULL || strcmp(result, longstr) != 0);
	if (result) {
		free(result);
	}
	call_01(ffclos, f);
}


/*
 * Test header writing with special image types (LONGLONG_IMG).
 */
static void
test_putkey_header_longlong(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[2] = { 20, 20 };

	call_02(ffinit, &f, "!" test_path);
	call_08(ffphpr, f, 1, LONGLONG_IMG, 2, naxes, 0, 1, 1);
	call_01(ffclos, f);
}


/*
 * Test binary table header with variable length arrays.
 */
static void
test_putkey_bintable_vla(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[2] = { "COL1", "COL2" };
	char *tform[2] = { "1J", "PJ" };  /* Variable length array */

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffcrhd, f);
	call_08(ffphbn, f, 10, 2, ttype, tform, NULL, "TESTTBL", 100);
	call_01(ffclos, f);
}


/*
 * Test ASCII table header with multiple columns.
 */
static void
test_putkey_asciitable_multi(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[3] = { "NAME", "VALUE", "COUNT" };
	char *tform[3] = { "A20", "E15.7", "I10" };
	char *tunit[3] = { "text", "meters", "items" };
	long tbcol[3] = { 1, 21, 37 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffcrhd, f);
	call_09(ffphtb, f, 50, 100, 3, ttype, tbcol, tform, tunit, "TESTTBL");
	call_01(ffclos, f);
}


/*
 * Test extension header with multi-dimensional image.
 */
static void
test_putkey_extheader_3d(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[3] = { 10, 20, 5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffcrhd, f);
	call_07(ffphext, f, "IMAGE", 32, 3, naxes, 0, 1);
	call_01(ffclos, f);
}


/*
 * Test writing empty string keyword.
 */
static void
test_putkey_empty_string(void)
{
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test writing empty string */
	call_04(ffpkys, f, "EMPTY", "", "empty string test");

	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "EMPTY", sval, NULL);
	fail_if(strcmp(sval, "") != 0);
	call_01(ffclos, f);
}


/*
 * Test time-only format in fftm2s (year=month=day=0).
 */
static void
test_putkey_timeonly(void)
{
	int status = 0;
	char timestr[40];

	/* Test time-only format (year=0, month=0, day=0) */
	call_08(fftm2s, 0, 0, 0, 14, 30, 45.5, 3, timestr);
	/* Result should be in time-only format: hh:mm:ss.ddd */
	fail_if(strlen(timestr) < 8);
	fail_if(timestr[2] != ':' || timestr[5] != ':');
}


/*
 * Test more date conversion edge cases.
 */
static void
test_putkey_date_edge_cases(void)
{
	int status = 0;
	int yr, mon, day;
	char datestr[30];

	/* Test century boundary (year 2000) */
	call_04(ffdt2s, 2000, 1, 1, datestr);
	fail_if(strcmp(datestr, "2000-01-01") != 0);

	/* Test old format with single digit day */
	call_04(ffs2dt, "01/02/98", &yr, &mon, &day);
	fail_if(yr != 1998 || mon != 2 || day != 1);
}


/*
 * Test complex keyword with NaN or special values.
 */
static void
test_putkey_complex_special(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = { 1.5f, 2.5f };
	double mval[2] = { 3.5, 4.5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test with very small precision */
	call_05(ffpkyc, f, "CKEY1", cval, 0, "complex low precision");
	call_05(ffpkym, f, "MKEY1", mval, 0, "dblcomplex low precision");

	/* Test with high precision */
	call_05(ffpkyc, f, "CKEY2", cval, 14, "complex high precision");
	call_05(ffpkym, f, "MKEY2", mval, 14, "dblcomplex high precision");

	/* Test fixed format complex */
	call_05(ffpkfc, f, "FCKEY", cval, 0, "fixed complex low prec");
	call_05(ffpkfm, f, "FMKEY", mval, 0, "fixed dblcomplex low prec");

	call_01(ffclos, f);
}


/*
 * Test old style (pre-1999) date handling.
 */
static void
test_putkey_old_date(void)
{
	int status = 0;
	char datestr[30];

	/* Test pre-1999 date (should use DD/MM/YY format) */
	call_04(ffdt2s, 1985, 3, 15, datestr);
	fail_if(strcmp(datestr, "15/03/85") != 0);

	/* Test 1998 (last year for old format) */
	call_04(ffdt2s, 1998, 12, 31, datestr);
	fail_if(strcmp(datestr, "31/12/98") != 0);

	/* Test 1999 (first year for new format) */
	call_04(ffdt2s, 1999, 1, 1, datestr);
	fail_if(strcmp(datestr, "1999-01-01") != 0);
}


/*
 * Test updating unsigned keyword.
 */
static void
test_putkey_update_unsigned(void)
{
	fitsfile *f;
	int status = 0;
	unsigned long ulval = 3000000000UL;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Write and update unsigned long */
	call_05(ffpky, f, TULONG, "ULKEY", &ulval, "unsigned long");
	ulval = 4000000000UL;
	call_05(ffuky, f, TULONG, "ULKEY", &ulval, "updated unsigned long");

	call_01(ffclos, f);
}


/*
 * Test array keywords with NULL comments and non-repeating comments.
 */
static void
test_putkey_array_null_comment(void)
{
	fitsfile *f;
	int status = 0;
	char *svals[3] = { "alpha", "beta", "gamma" };
	int lvals[3] = { 1, 0, 1 };
	long jvals[3] = { 100, 200, 300 };
	float fvals[3] = { 1.1f, 2.2f, 3.3f };
	double dvals[3] = { 4.4, 5.5, 6.6 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test array keywords with NULL comments */
	call_06(ffpkns, f, "SKEY", 1, 3, svals, NULL);
	call_06(ffpknl, f, "LKEY", 1, 3, lvals, NULL);
	call_06(ffpknj, f, "JKEY", 1, 3, jvals, NULL);
	call_07(ffpknf, f, "FKEY", 1, 3, fvals, 2, NULL);
	call_07(ffpknd, f, "DKEY", 1, 3, dvals, 4, NULL);

	call_01(ffclos, f);
}


/*
 * Test array keywords with non-repeating comments (no & suffix).
 */
static void
test_putkey_array_nonrepeat_comment(void)
{
	fitsfile *f;
	int status = 0;
	char *svals[3] = { "one", "two", "three" };
	int lvals[3] = { 0, 1, 0 };
	long jvals[3] = { 1000, 2000, 3000 };
	float evals[3] = { 1.0e10f, 2.0e10f, 3.0e10f };
	double gvals[3] = { 1.111, 2.222, 3.333 };
	/* Non-repeating comments (no trailing &) */
	char *comm[3] = { "comment one", "comment two", "comment three" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test with non-repeating comments */
	call_06(ffpkns, f, "NRSKEY", 1, 3, svals, comm);
	call_06(ffpknl, f, "NRLKEY", 1, 3, lvals, comm);
	call_06(ffpknj, f, "NRJKEY", 1, 3, jvals, comm);
	call_07(ffpkne, f, "NREKEY", 1, 3, evals, 3, comm);
	call_07(ffpkng, f, "NRGKEY", 1, 3, gvals, 4, comm);

	call_01(ffclos, f);
}


/*
 * Test long string with NULL comment.
 */
static void
test_putkey_longstr_null_comment(void)
{
	fitsfile *f;
	int status = 0;
	char longstr[150];
	char *result = NULL;

	memset(longstr, 'a', 100);
	longstr[100] = '\0';

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_04(ffpkls, f, "LSNULL", longstr, NULL);
	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkls, f, "LSNULL", &result, NULL);
	fail_if(result == NULL || strcmp(result, longstr) != 0);
	if (result) {
		free(result);
	}
	call_01(ffclos, f);
}


/*
 * Test HIERARCH keywords.
 */
static void
test_putkey_hierarch(void)
{
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Explicit HIERARCH keyword */
	call_04(ffpkys, f, "HIERARCH MY.LONG.KEYWORD", "hier value", "hierarch test");

	/* Long keyword that will have HIERARCH prepended */
	call_04(ffpkys, f, "MY.EXTRA.LONG.KEYWORD.NAME", "auto value", "auto hierarch");

	call_01(ffclos, f);
}


/*
 * Test short keyword writing with leading spaces.
 */
static void
test_putkey_leading_spaces(void)
{
	fitsfile *f;
	int status = 0;
	char sval[FLEN_VALUE];

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Keyword with trailing space - should be trimmed */
	call_04(ffpkys, f, "SPACEKEY ", "test value", "space test");

	call_01(ffclos, f);

	call_03(ffopen, &f, test_path, READONLY);
	call_04(ffgkys, f, "SPACEKEY", sval, NULL);
	fail_if(strcmp(sval, "test value") != 0);
	call_01(ffclos, f);
}


/*
 * Test floating point format keywords with edge cases.
 */
static void
test_putkey_float_formats(void)
{
	fitsfile *f;
	int status = 0;
	float fval = 0.0f;
	double dval = 0.0;

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test zero values */
	call_05(ffpkye, f, "FZERO", fval, 6, "zero float E");
	call_05(ffpkyf, f, "FZEROF", fval, 3, "zero float F");
	call_05(ffpkyd, f, "DZERO", dval, 10, "zero double E");
	call_05(ffpkyg, f, "DZEROG", dval, 6, "zero double G");

	/* Test very small values */
	fval = 1.23e-30f;
	dval = 4.56e-200;
	call_05(ffpkye, f, "FSMALL", fval, 6, "small float E");
	call_05(ffpkyd, f, "DSMALL", dval, 10, "small double E");

	/* Test very large values */
	fval = 9.87e30f;
	dval = 1.23e200;
	call_05(ffpkye, f, "FLARGE", fval, 6, "large float E");
	call_05(ffpkyd, f, "DLARGE", dval, 10, "large double E");

	call_01(ffclos, f);
}


/*
 * Test array keywords with comments containing trailing blanks.
 */
static void
test_putkey_array_trailing_blanks(void)
{
	fitsfile *f;
	int status = 0;
	long jvals[3] = { 10, 20, 30 };
	/* Comments with trailing blanks followed by & */
	char *comm[3] = { "comment with spaces   &", "second", "third" };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Test with trailing blanks before & */
	call_06(ffpknj, f, "TBKEY", 1, 3, jvals, comm);

	call_01(ffclos, f);
}

/*
 * Test complex number error when value string is too long
 */
static void
test_putkey_complex_too_long(void)
{
	fitsfile *f;
	int status = 0;
	float cval[2] = { 1.5f, 2.5f };
	double mval[2] = { 3.5, 4.5 };

	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);

	/* Use very high precision to trigger BAD_F2C error */
	/* FLEN_VALUE is 71, complex format is "(val1, val2)" */
	/* With decim=40, each value string would be ~45 chars, total > 71 */
	status = 0;
	ffpkyc(f, "CKEY", cval, 40, "too long", &status);
	fail_if(status != BAD_F2C);

	status = 0;
	ffpkym(f, "MKEY", mval, 40, "too long", &status);
	fail_if(status != BAD_F2C);

	status = 0;
	ffpkfc(f, "FCKEY", cval, 40, "too long", &status);
	fail_if(status != BAD_F2C);

	status = 0;
	ffpkfm(f, "FMKEY", mval, 40, "too long", &status);
	fail_if(status != BAD_F2C);

	status = 0;
	call_01(ffclos, f);
}


int
main(void)
{
	test_putkey_string();
	test_putkey_long();
	test_putkey_double();
	test_putkey_logical();
	test_putkey_float();
	test_putkey_generic_types();
	test_putkey_unsigned_long();
	test_putkey_null();
	test_putkey_units();
	test_putkey_fixfmt();
	test_putkey_fixfmt_float();
	test_putkey_complex();
	test_putkey_complex_fixed();
	test_putkey_comment_history();
	test_putkey_date();
	test_update_key();
	test_putkey_indexed_arrays();
	test_putkey_long_string();
	test_putkey_template();
	test_putkey_datetime();
	test_putkey_tdim();
	test_putkey_time();
	test_putkey_rawrec();
	test_putkey_verifydate();
	test_putkey_sysdate();
	test_putkey_insert();
	test_putkey_header();
	test_putkey_table_headers();
	test_putkey_exthead();
	test_putkey_updates();
	test_putkey_image_ext();
	test_putkey_special_images();
	test_putkey_longstrn();
	test_putkey_random_groups();
	test_putkey_bad_datatype();
	test_putkey_template_notfound();
	test_putkey_date_errors();
	test_putkey_time_errors();
	test_putkey_verifydate_errors();
	test_putkey_dt2s_errors();
	test_putkey_time_keyword_errors();
	test_putkey_tdim_errors();
	test_putkey_tdim_nottable();
	test_putkey_very_long_string();
	test_putkey_long_string_quotes();
	test_putkey_header_longlong();
	test_putkey_bintable_vla();
	test_putkey_asciitable_multi();
	test_putkey_extheader_3d();
	test_putkey_empty_string();
	test_putkey_timeonly();
	test_putkey_date_edge_cases();
	test_putkey_complex_special();
	test_putkey_old_date();
	test_putkey_update_unsigned();
	test_putkey_array_null_comment();
	test_putkey_array_nonrepeat_comment();
	test_putkey_longstr_null_comment();
	test_putkey_hierarch();
	test_putkey_leading_spaces();
	test_putkey_float_formats();
	test_putkey_array_trailing_blanks();
	test_putkey_complex_too_long();
	return 0;
}
