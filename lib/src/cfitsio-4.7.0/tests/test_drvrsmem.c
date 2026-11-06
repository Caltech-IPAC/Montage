#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
#include "test_macros.h"

#ifdef HAVE_SHMEM_SERVICES

/* drvrsmem.h pulls in <sys/ipc.h> etc, which do not exist on all platforms. */
#include "drvrsmem.h"

static void
test_create_shmem_file(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	short data[100];

	for (int i = 0; i < 100; i += 1) {
		data[i] = i;
	}

	call_02(ffinit, &f, "shmem://h0");
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_05(ffppr, f, TSHORT, 1, 100, data);
	call_01(ffclos, f);
}


static void
test_open_and_read_shmem(void)
{
	fitsfile *f;
	int status = 0;
	short data[100];
	int anynull;

	call_03(ffopen, &f, "shmem://h0", READONLY);
	call_07(ffgpv, f, TSHORT, 1, 100, NULL, data, &anynull);
	fail_if(data[0] != 0);
	fail_if(data[50] != 50);
	fail_if(data[99] != 99);
	call_01(ffclos, f);
}


static void
test_shmem_keywords(void)
{
	fitsfile *f;
	int status = 0;
	char strval[FLEN_VALUE];
	char comment[FLEN_COMMENT];
	long longval;

	call_03(ffopen, &f, "shmem://h0", READWRITE);
	call_04(ffpkys, f, "TESTKEY", "testval", "test comment");
	call_04(ffpkyj, f, "INTKEY", 12345, "integer key");
	call_04(ffgkys, f, "TESTKEY", strval, comment);
	fail_if(strcmp(strval, "testval"));
	call_05(ffgky, f, TLONG, "INTKEY", &longval, comment);
	fail_if(longval != 12345);
	call_01(ffclos, f);
}


static void
test_delete_shmem(void)
{
	fitsfile *f;
	int status = 0;

	call_03(ffopen, &f, "shmem://h0", READWRITE);
	call_01(ffdelt, f);
}


static void
test_create_second_segment(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };

	call_02(ffinit, &f, "shmem://h1");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffdelt, f);
}


static void
test_smem_option_functions(void)
{
	int options;
	int version;

	fail_if(smem_setoptions(42));
	fail_if(smem_getoptions(&options));
	fail_if(options != 0);
	fail_if(smem_getoptions(NULL) != SHARED_NULPTR);
	fail_if(smem_getversion(&version));
	fail_if(version != 10);
	fail_if(smem_getversion(NULL) != SHARED_NULPTR);
}


/*
 * Test shared memory utility functions: shared_list, shared_getaddr,
 * shared_recover, and shared_uncond_delete.
 */
static void
test_shared_utility_functions(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	char *addr;

	call_02(ffinit, &f, "shmem://h6");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	fail_if(shared_list(-1));
	fail_if(shared_list(6));
	fail_if(shared_getaddr(6, &addr));
	fail_if(addr == NULL);
	fail_if(shared_recover(-1));
	fail_if(shared_recover(6));
	fail_if(shared_uncond_delete(6));
}


static void
test_read_beyond_eof(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 2 };
	unsigned char data[1000];
	int anynull;

	call_02(ffinit, &f, "shmem://h3");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	call_03(ffopen, &f, "shmem://h3", READONLY);
	fail_if(ffgpv(f, TBYTE, 1, 1000, NULL, data, &anynull, &status) == 0);

	status = 0;
	ffclos(f, &status);

	status = 0;
	call_03(ffopen, &f, "shmem://h3", READWRITE);
	call_01(ffdelt, f);
}


/*
 * Test smem_read returning SHARED_BADARG when reading beyond EOF.
 * Uses low-level smem_* functions to exercise the read-beyond-EOF
 * check in smem_read that higher-level CFITSIO functions bypass.
 */
static void
test_smem_read_beyond_eof(void)
{
	fitsfile *f;
	int status = 0;
	int handle;
	long naxes[] = { 2 };
	unsigned char data[1000];

	call_02(ffinit, &f, "shmem://h4");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	fail_if(smem_open("h4", READONLY, &handle));
	fail_if(smem_seek(handle, 99999));
	fail_if(smem_read(handle, data, 1000) != SHARED_BADARG);
	fail_if(smem_close(handle));
	call_03(ffopen, &f, "shmem://h4", READWRITE);
	call_01(ffdelt, f);
}


static void
test_cleanup_locked_segment(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };

	call_02(ffinit, &f, "shmem://h2");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	smem_shutdown();
}


/*
 * Test cleanup with debug mode enabled to exercise the debug print
 * statements in shared_cleanup.
 */
static void
test_cleanup_with_debug(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };

	shared_set_debug(1);
	call_02(ffinit, &f, "shmem://h5");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	fail_if(smem_shutdown());
}


static void
test_smem_size(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10, 10 };
	int handle;
	LONGLONG size;

	call_02(ffinit, &f, "shmem://h7");
	call_04(ffphps, f, SHORT_IMG, 2, naxes);
	call_01(ffclos, f);
	fail_if(smem_open("h7", READONLY, &handle));
	fail_if(smem_size(handle, &size));
	fail_if(size < 2880);
	fail_if(smem_close(handle));
	call_03(ffopen, &f, "shmem://h7", READWRITE);
	call_01(ffdelt, f);
}


static void
test_smem_flush(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };
	int handle;

	call_02(ffinit, &f, "shmem://h8");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	fail_if(smem_open("h8", READWRITE, &handle));
	fail_if(smem_flush(handle));
	fail_if(smem_close(handle));
	call_03(ffopen, &f, "shmem://h8", READWRITE);
	call_01(ffdelt, f);
}


static void
test_smem_write(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int handle;
	unsigned char data[] = { 0xFF, 0xFE, 0xFD, 0xFC };
	unsigned char readbuf[4];

	call_02(ffinit, &f, "shmem://h9");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	fail_if(smem_open("h9", READWRITE, &handle));
	fail_if(smem_seek(handle, 2880));
	fail_if(smem_write(handle, data, 4));
	fail_if(smem_seek(handle, 2880));
	fail_if(smem_read(handle, readbuf, 4));
	fail_if(readbuf[0] != 0xFF);
	fail_if(readbuf[1] != 0xFE);
	fail_if(smem_close(handle));
	call_03(ffopen, &f, "shmem://h9", READWRITE);
	call_01(ffdelt, f);
}


static void
test_smem_remove(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 3 };
	int handle;

	call_02(ffinit, &f, "shmem://h10");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	/* smem_remove should fail on open segment or work on closed */
	fail_if(smem_remove("h10"));
}


static void
test_shared_set_createmode(void)
{
	int oldmode;
	oldmode = shared_set_createmode(0666);
	shared_set_createmode(oldmode);
}


static void
test_smem_open_nonexistent(void)
{
	int handle;
	/* Opening non-existent segment should fail */
	fail_if(smem_open("nonexistent_segment_xyz", READONLY, &handle) == 0);
}


static void
test_smem_create_and_delete(void)
{
	int handle;
	fail_if(smem_create("h11", &handle));
	fail_if(smem_close(handle));
	fail_if(smem_open("h11", READWRITE, &handle));
	fail_if(smem_close(handle));
	fail_if(smem_remove("h11"));
}


static void
test_shared_attr(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 10 };
	int handle;
	int attr;

	call_02(ffinit, &f, "shmem://h13");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	call_01(ffclos, f);
	fail_if(smem_open("h13", READONLY, &handle));
	attr = shared_attr(handle);
	fail_if(smem_close(handle));
	call_03(ffopen, &f, "shmem://h13", READWRITE);
	call_01(ffdelt, f);
}


static void
test_list_with_segments(void)
{
	fitsfile *f;
	int status = 0;
	long naxes[] = { 5 };

	/* Create a segment and list it */
	call_02(ffinit, &f, "shmem://h14");
	call_04(ffphps, f, BYTE_IMG, 1, naxes);
	fail_if(shared_list(-1));
	call_01(ffdelt, f);
}
#endif


int
main(void)
{
#ifdef HAVE_SHMEM_SERVICES
	test_create_shmem_file();
	test_open_and_read_shmem();
	test_shmem_keywords();
	test_delete_shmem();
	test_create_second_segment();
	test_smem_option_functions();
	test_shared_utility_functions();
	test_read_beyond_eof();
	test_smem_read_beyond_eof();
	test_cleanup_locked_segment();
	test_cleanup_with_debug();
	test_smem_size();
	test_smem_flush();
	test_smem_write();
	test_smem_remove();
	test_shared_set_createmode();
	test_smem_open_nonexistent();
	test_smem_create_and_delete();
	test_shared_attr();
	test_list_with_segments();

	return 0;
#else
	return 77;
#endif
}
