/*
 * Tests for checksum.c - FITS checksum functions.
 */

#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#define test_path "test_checksum.fits"

static void
test_write_checksum(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
}

static void
test_verify_checksum(void)
{
	fitsfile *f;
	int status = 0;
	int datastatus, hdustatus;
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
}

static void
test_update_checksum(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffpcks, f);
	call_01(ffupck, f);
	call_01(ffclos, f);
}

static void
test_get_checksums(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	unsigned long datasum, hdusum;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_03(ffgcks, f, &datasum, &hdusum);
	call_01(ffclos, f);
}

static void
test_encode_decode_checksum(void)
{
	unsigned long sum = 0x12345678;
	unsigned long decoded;
	char ascii[17];
	ffesum(sum, 0, ascii);
	ffdsum(ascii, 0, &decoded);
	fail_if(decoded != sum);
}

static void
test_encode_complement(void)
{
	unsigned long sum = 0xABCDEF00;
	char ascii[17];
	ffesum(sum, 1, ascii);
	fail_if(strlen(ascii) != 16);
}

static void
test_verify_no_keywords(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 0);
	fail_if(hdustatus != 0);
	call_01(ffclos, f);
}

static void
test_checksum_binary_table(void)
{
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "COL1" };
	char *tform[] = { "1J" };
	long data[] = { 100, 200, 300 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 3, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
}

static void
test_update_existing_checksum(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	unsigned char newdata[] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READWRITE);
	call_05(ffppr, f, TBYTE, 1, 10, newdata);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
}

static void
test_large_data_checksum(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10000 };
	unsigned char *data;
	int i;
	int datastatus, hdustatus;
	data = malloc(10000 * sizeof *data);
	fail_if(data == NULL);
	for (i = 0; i < 10000; i += 1) {
		data[i] = (unsigned char)(i % 256);
	}
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10000, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
	free(data);
}

static void
test_decode_with_carry(void)
{
	/* Test ffdsum with values that cause carry in the decode loop */
	unsigned long sum;
	char ascii[17];
	/* Use a value that will cause carries during decode */
	ffesum(0xFFFFFFFF, 0, ascii);
	ffdsum(ascii, 0, &sum);
	fail_if(sum != 0xFFFFFFFF);
	/* Test with complement */
	ffesum(0xFFFFFFFF, 1, ascii);
	ffdsum(ascii, 1, &sum);
	fail_if(sum != 0xFFFFFFFF);
}

static void
test_upck_no_checksum_keyword(void)
{
	/* Test ffupck when CHECKSUM keyword doesn't exist */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	/* Write DATASUM but not CHECKSUM */
	call_04(ffpkys, f, "DATASUM", "         0", NULL);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READWRITE);
	call_01(ffupck, f);
	call_01(ffclos, f);
}

static void
test_upck_no_datasum_keyword(void)
{
	/* Test ffupck when DATASUM keyword doesn't exist */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READWRITE);
	status = 0;
	ffupck(f, &status);
	fail_if(status != KEY_NO_EXIST);
	status = 0;
	call_01(ffclos, f);
}

static void
test_verify_incorrect_checksum(void)
{
	/* Test ffvcks with incorrect checksum values */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	/* Corrupt the file by changing data after checksum was written */
	call_03(ffopen, &f, test_path, READWRITE);
	data[0] = 99;
	call_05(ffppr, f, TBYTE, 1, 1, data);
	call_01(ffclos, f);
	/* Verify should fail now */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != -1);
	fail_if(hdustatus != -1);
	call_01(ffclos, f);
}

static void
test_pcks_update_existing_datasum(void)
{
	/* Test ffpcks when DATASUM exists but data changes */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	unsigned char newdata[] = { 99, 98, 97, 96, 95, 94, 93, 92, 91, 90 };
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	/* Open and change data, then recalculate checksum */
	call_03(ffopen, &f, test_path, READWRITE);
	call_05(ffppr, f, TBYTE, 1, 10, newdata);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	/* Verify new checksum is correct */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
}

static void
test_upck_existing_correct_checksum(void)
{
	/* Test ffupck when CHECKSUM already exists and is correct */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	/* Call ffupck on file with correct checksum */
	call_03(ffopen, &f, test_path, READWRITE);
	call_01(ffupck, f);
	call_01(ffclos, f);
	/* Verify checksum is still correct */
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
}

static void
test_pcks_bad_status(void)
{
	/* Test that ffpcks returns immediately with bad status */
	fitsfile *f;
	int status = 1;
	ffpcks(f, &status);
	fail_if(status != 1);
}

static void
test_vcks_bad_status(void)
{
	/* Test that ffvcks returns immediately with bad status */
	fitsfile *f;
	int status = 1;
	int datastatus, hdustatus;
	ffvcks(f, &datastatus, &hdustatus, &status);
	fail_if(status != 1);
}

static void
test_gcks_bad_status(void)
{
	/* Test that ffgcks returns immediately with bad status */
	fitsfile *f;
	int status = 1;
	unsigned long datasum, hdusum;
	ffgcks(f, &datasum, &hdusum, &status);
	fail_if(status != 1);
}

static void
test_upck_bad_status(void)
{
	/* Test that ffupck returns immediately with bad status */
	fitsfile *f;
	int status = 1;
	ffupck(f, &status);
	fail_if(status != 1);
}

static void
test_csum_bad_status(void)
{
	/* Test that ffcsum returns immediately with bad status */
	fitsfile *f;
	int status = 1;
	unsigned long sum = 0;
	ffcsum(f, 1, &sum, &status);
	fail_if(status != 1);
}

static void
test_empty_hdu_checksum(void)
{
	/* Test checksum on HDU with no data */
	fitsfile *f;
	int status = 0;
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
}

static void
test_varlen_column_checksum(void)
{
	/* Test checksum with variable-length column (heapsize > 0) */
	fitsfile *f;
	int status = 0;
	char *ttype[] = { "VARDATA" };
	char *tform[] = { "1PJ(100)" };
	long data1[] = { 1, 2, 3 };
	long data2[] = { 10, 20, 30, 40, 50 };
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 0, NULL);
	call_08(ffcrtb, f, BINARY_TBL, 2, 1, ttype, tform, NULL, NULL);
	call_06(ffpclj, f, 1, 1, 1, 3, data1);
	call_06(ffpclj, f, 1, 2, 1, 5, data2);
	call_01(ffpcks, f);
	call_01(ffclos, f);
}

static void
test_pcks_nonzero_checksum(void)
{
	/* Test ffpcks when CHECKSUM is not "0000..." and DATASUM exists */
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	unsigned char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	int datastatus, hdustatus;
	call_02(ffinit, &f, "!" test_path);
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_05(ffppr, f, TBYTE, 1, 10, data);
	/* Write checksum first */
	call_01(ffpcks, f);
	call_01(ffclos, f);
	/* Reopen and call ffpcks again - checksum should be rechecked */
	call_03(ffopen, &f, test_path, READWRITE);
	call_01(ffpcks, f);
	call_01(ffclos, f);
	call_03(ffopen, &f, test_path, READONLY);
	call_03(ffvcks, f, &datastatus, &hdustatus);
	fail_if(datastatus != 1);
	fail_if(hdustatus != 1);
	call_01(ffclos, f);
}

static void
test_decode_large_values(void)
{
	/* Test ffdsum with values that cause carry in decode loop */
	unsigned long sum;
	char ascii[17];
	unsigned long test_values[] = {
		0xFFFFFFFF, 0xAAAAAAAA, 0x55555555, 0x80808080,
		0xF0F0F0F0, 0x0F0F0F0F, 0x12345678, 0x87654321
	};
	int i;
	for (i = 0; i < 8; i += 1) {
		ffesum(test_values[i], 0, ascii);
		ffdsum(ascii, 0, &sum);
		fail_if(sum != test_values[i]);
	}
}

int
main(void)
{
	test_write_checksum();
	test_verify_checksum();
	test_update_checksum();
	test_get_checksums();
	test_encode_decode_checksum();
	test_encode_complement();
	test_verify_no_keywords();
	test_checksum_binary_table();
	test_update_existing_checksum();
	test_large_data_checksum();
	test_decode_with_carry();
	test_upck_no_checksum_keyword();
	test_upck_no_datasum_keyword();
	test_verify_incorrect_checksum();
	test_pcks_update_existing_datasum();
	test_upck_existing_correct_checksum();
	test_pcks_bad_status();
	test_vcks_bad_status();
	test_gcks_bad_status();
	test_upck_bad_status();
	test_csum_bad_status();
	test_empty_hdu_checksum();
	test_varlen_column_checksum();
	test_pcks_nonzero_checksum();
	test_decode_large_values();
	remove(test_path);
	return 0;
}
