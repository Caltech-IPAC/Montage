/*
 * Tests for ricecomp.c - Rice compression/decompression
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_macros.h"
#include "fitsio2.h"


/*
 * Test basic int compression/decompression roundtrip
 */
static void
test_rcomp_roundtrip(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Create gradient data */
	for (i = 0; i < 64; i += 1) {
		original[i] = i * 10;
	}

	/* Compress */
	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	/* Verify */
	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short compression/decompression roundtrip
 */
static void
test_rcomp_short_roundtrip(void)
{
	short original[64];
	unsigned short decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Create gradient data */
	for (i = 0; i < 64; i += 1) {
		original[i] = (short)(i * 100);
	}

	/* Compress */
	nbytes = fits_rcomp_short(original, 64, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 64,
		32) != 0);

	/* Verify */
	for (i = 0; i < 64; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte compression/decompression roundtrip
 */
static void
test_rcomp_byte_roundtrip(void)
{
	signed char original[64];
	unsigned char decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Create gradient data (0-63) */
	for (i = 0; i < 64; i += 1) {
		original[i] = (signed char)i;
	}

	/* Compress */
	nbytes = fits_rcomp_byte(original, 64, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 64,
		32) != 0);

	/* Verify */
	for (i = 0; i < 64; i += 1) {
		fail_if((signed char)decompressed[i] != original[i]);
	}
}

/*
 * Test compression of uniform data
 */
static void
test_rcomp_uniform(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* All same value */
	for (i = 0; i < 64; i += 1) {
		original[i] = 42;
	}

	/* Compress - uniform data should compress very well */
	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);
	fail_if(nbytes > 50);  /* Should be very small */

	/* Decompress */
	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	/* Verify */
	for (i = 0; i < 64; i += 1) {
		fail_if(decompressed[i] != 42);
	}
}

/*
 * Test compression with different block sizes
 */
static void
test_rcomp_block_sizes(void)
{
	int original[128];
	unsigned int decompressed[128];
	unsigned char compressed[512];
	int nbytes;
	int i;
	int block_sizes[] = { 16, 32, 64 };

	/* Create test data */
	for (i = 0; i < 128; i += 1) {
		original[i] = i * 5;
	}

	for (i = 0; i < 3; i += 1) {
		int nblock = block_sizes[i];

		nbytes = fits_rcomp(original, 128, compressed,
			sizeof compressed, nblock);
		fail_if(nbytes <= 0);

		fail_if(fits_rdecomp(compressed, nbytes, decompressed, 128,
			nblock) != 0);

		for (int j = 0; j < 128; j += 1) {
			fail_if((int)decompressed[j] != original[j]);
		}
	}
}

/*
 * Test compression with larger values
 */
static void
test_rcomp_large_values(void)
{
	int original[32];
	unsigned int decompressed[32];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Large values with small differences */
	for (i = 0; i < 32; i += 1) {
		original[i] = 1000000 + i;
	}

	/* Compress */
	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 32, 16) != 0);

	/* Verify */
	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test compression with zero data
 */
static void
test_rcomp_zeros(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* All zeros */
	for (i = 0; i < 64; i += 1) {
		original[i] = 0;
	}

	/* Compress */
	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	/* Verify */
	for (i = 0; i < 64; i += 1) {
		fail_if(decompressed[i] != 0);
	}
}

/*
 * Test compression with alternating pattern
 */
static void
test_rcomp_alternating(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Alternating values */
	for (i = 0; i < 64; i += 1) {
		original[i] = (i % 2) ? 100 : 200;
	}

	/* Compress */
	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	/* Verify */
	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short with larger array
 */
static void
test_rcomp_short_large_array(void)
{
	short original[256];
	unsigned short decompressed[256];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Create test data */
	for (i = 0; i < 256; i += 1) {
		original[i] = (short)(i * 10);
	}

	/* Compress */
	nbytes = fits_rcomp_short(original, 256, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 256,
		32) != 0);

	/* Verify */
	for (i = 0; i < 256; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with full range (0-127 for signed char)
 */
static void
test_rcomp_byte_range(void)
{
	signed char original[128];
	unsigned char decompressed[128];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Full positive range */
	for (i = 0; i < 128; i += 1) {
		original[i] = (signed char)i;
	}

	/* Compress */
	nbytes = fits_rcomp_byte(original, 128, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	/* Decompress */
	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 128,
		32) != 0);

	/* Verify */
	for (i = 0; i < 128; i += 1) {
		fail_if(decompressed[i] != (unsigned char)i);
	}
}
/*
 * Test with negative values (signed data)
 */
static void
test_rcomp_negative_values(void)
{
	int original[32];
	unsigned int decompressed[32];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Mix of positive and negative values */
	for (i = 0; i < 32; i += 1) {
		original[i] = (i % 2) ? -i * 10 : i * 10;
	}

	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 32, 16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test with high entropy random-like data
 */
static void
test_rcomp_high_entropy(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[512];  /* Larger buffer for incompressible data */
	int nbytes;
	int i;

	/* Pseudo-random data with large differences */
	for (i = 0; i < 64; i += 1) {
		original[i] = (((unsigned)i * 1103515245U + 12345U) >> 16) & 0x7FFF;
	}

	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short with negative values
 */
static void
test_rcomp_short_negative(void)
{
	short original[64];
	unsigned short decompressed[64];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Mix of positive and negative */
	for (i = 0; i < 64; i += 1) {
		original[i] = (short)((i % 2) ? -i * 100 : i * 100);
	}

	nbytes = fits_rcomp_short(original, 64, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 64,
		32) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with negative values
 */
static void
test_rcomp_byte_negative(void)
{
	signed char original[64];
	unsigned char decompressed[64];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Mix of positive and negative in signed char range */
	for (i = 0; i < 64; i += 1) {
		original[i] = (signed char)((i % 2) ? -(i % 128) : (i % 128));
	}

	nbytes = fits_rcomp_byte(original, 64, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 64,
		32) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((signed char)decompressed[i] != original[i]);
	}
}

/*
 * Test with very small block size
 */
static void
test_rcomp_small_block(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[512];
	int nbytes;
	int i;

	for (i = 0; i < 64; i += 1) {
		original[i] = i * 3;
	}

	/* Use smallest block size */
	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 8);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 8) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test with extreme values near INT_MAX/INT_MIN
 */
static void
test_rcomp_extreme_values(void)
{
	int original[16];
	unsigned int decompressed[16];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Values near extremes but with small differences */
	for (i = 0; i < 16; i += 1) {
		original[i] = 2000000000 + i;
	}

	nbytes = fits_rcomp(original, 16, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 16, 16) != 0);

	for (i = 0; i < 16; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short with extreme values
 */
static void
test_rcomp_short_extreme(void)
{
	short original[16];
	unsigned short decompressed[16];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Values near short limits */
	for (i = 0; i < 16; i += 1) {
		original[i] = (short)(32000 + i);
	}

	nbytes = fits_rcomp_short(original, 16, compressed, sizeof compressed,
		16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 16,
		16) != 0);

	for (i = 0; i < 16; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test with data requiring many FS bits
 */
static void
test_rcomp_high_fs(void)
{
	int original[32];
	unsigned int decompressed[32];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Large value differences requiring high FS */
	for (i = 0; i < 32; i += 1) {
		original[i] = (i % 2) ? 1000000 : 0;
	}

	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 32, 16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test single element
 */
static void
test_rcomp_single_element(void)
{
	int original[1] = { 12345 };
	unsigned int decompressed[1];
	unsigned char compressed[64];
	int nbytes;

	nbytes = fits_rcomp(original, 1, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 1, 32) != 0);
	fail_if((int)decompressed[0] != original[0]);
}

/*
 * Test with monotonically decreasing data
 */
static void
test_rcomp_decreasing(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Decreasing values */
	for (i = 0; i < 64; i += 1) {
		original[i] = 1000 - i * 10;
	}

	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test decompression with buffer too small (clen < 4)
 */
static void
test_rdecomp_buffer_too_small(void)
{
	unsigned char compressed[3] = { 0, 0, 0 };
	unsigned int decompressed[64];

	/* Should fail - needs at least 4 bytes for first pixel */
	fail_if(fits_rdecomp(compressed, 3, decompressed, 64, 32) == 0);
}

/*
 * Test short decompression with valid small buffer
 * (short only needs 2 bytes for first pixel - no clen check)
 */
static void
test_rdecomp_short_small_nx(void)
{
	short original[1] = { 12345 };
	unsigned short decompressed[1];
	unsigned char compressed[32];
	int nbytes;

	nbytes = fits_rcomp_short(original, 1, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 1,
		32) != 0);
	fail_if((short)decompressed[0] != original[0]);
}

/*
 * Test byte decompression with valid small buffer
 * (byte only needs 1 byte for first pixel - no clen check)
 */
static void
test_rdecomp_byte_small_nx(void)
{
	signed char original[1] = { 42 };
	unsigned char decompressed[1];
	unsigned char compressed[32];
	int nbytes;

	nbytes = fits_rcomp_byte(original, 1, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 1,
		32) != 0);
	fail_if((signed char)decompressed[0] != original[0]);
}

/*
 * Test decompression with high-entropy data that triggers fs==fsmax path
 */
static void
test_rcomp_fsmax_path(void)
{
	int original[32];
	unsigned int decompressed[32];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Very large differences to trigger fsmax encoding */
	for (i = 0; i < 32; i += 1) {
		/* Alternating between very different values */
		original[i] = (i % 2) ? 0x7FFFFFFF : 0;
	}

	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 32, 16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short with fsmax path
 */
static void
test_rcomp_short_fsmax_path(void)
{
	short original[32];
	unsigned short decompressed[32];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Large alternating differences */
	for (i = 0; i < 32; i += 1) {
		original[i] = (short)((i % 2) ? 32767 : -32768);
	}

	nbytes = fits_rcomp_short(original, 32, compressed, sizeof compressed,
		16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 32,
		16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with fsmax path
 */
static void
test_rcomp_byte_fsmax_path(void)
{
	signed char original[32];
	unsigned char decompressed[32];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Large alternating differences for bytes */
	for (i = 0; i < 32; i += 1) {
		original[i] = (signed char)((i % 2) ? 127 : -128);
	}

	nbytes = fits_rcomp_byte(original, 32, compressed, sizeof compressed,
		16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 32,
		16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((signed char)decompressed[i] != original[i]);
	}
}

/*
 * Test with random-like patterns requiring various FS values
 */
static void
test_rcomp_varying_fs(void)
{
	int original[128];
	unsigned int decompressed[128];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Pattern that should require different FS for different blocks */
	for (i = 0; i < 32; i += 1) {
		original[i] = i;  /* Low entropy block */
	}
	for (i = 32; i < 64; i += 1) {
		original[i] = ((i * 7919) % 10000);  /* Medium entropy */
	}
	for (i = 64; i < 96; i += 1) {
		original[i] = ((i * 104729) % 1000000);  /* Higher entropy */
	}
	for (i = 96; i < 128; i += 1) {
		original[i] = 42;  /* Uniform block */
	}

	nbytes = fits_rcomp(original, 128, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 128, 32) != 0);

	for (i = 0; i < 128; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test partial block at end (nx not divisible by nblock)
 */
static void
test_rcomp_partial_block(void)
{
	int original[100];  /* Not divisible by 32 */
	unsigned int decompressed[100];
	unsigned char compressed[512];
	int nbytes;
	int i;

	for (i = 0; i < 100; i += 1) {
		original[i] = i * 7;
	}

	nbytes = fits_rcomp(original, 100, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 100, 32) != 0);

	for (i = 0; i < 100; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test all zeros for several blocks - tests fs<0 path
 */
static void
test_rcomp_multi_block_zeros(void)
{
	int original[128];
	unsigned int decompressed[128];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* All zeros - should compress very well using fs<0 path */
	for (i = 0; i < 128; i += 1) {
		original[i] = 0;
	}

	nbytes = fits_rcomp(original, 128, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);
	fail_if(nbytes > 40);  /* Should be tiny */

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 128, 32) != 0);

	for (i = 0; i < 128; i += 1) {
		fail_if(decompressed[i] != 0);
	}
}

/*
 * Test constant non-zero value across multiple blocks
 */
static void
test_rcomp_constant_multi_block(void)
{
	int original[128];
	unsigned int decompressed[128];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Constant non-zero value */
	for (i = 0; i < 128; i += 1) {
		original[i] = 999999;
	}

	nbytes = fits_rcomp(original, 128, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 128, 32) != 0);

	for (i = 0; i < 128; i += 1) {
		fail_if(decompressed[i] != 999999);
	}
}

/*
 * Test truncated compressed data - triggers end of stream error
 */
static void
test_rdecomp_truncated(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Create valid compressed data */
	for (i = 0; i < 64; i += 1) {
		original[i] = i * 100;
	}
	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	/* Try to decompress with truncated data - should fail */
	/* Only give it half the bytes */
	fail_if(fits_rdecomp(compressed, nbytes / 2, decompressed, 64,
		32) == 0);
}

/*
 * Test short with truncated data
 */
static void
test_rdecomp_short_truncated(void)
{
	short original[64];
	unsigned short decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	for (i = 0; i < 64; i += 1) {
		original[i] = (short)(i * 50);
	}
	nbytes = fits_rcomp_short(original, 64, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	/* Truncate data */
	fail_if(fits_rdecomp_short(compressed, nbytes / 2, decompressed, 64,
		32) == 0);
}

/*
 * Test byte with truncated data
 */
static void
test_rdecomp_byte_truncated(void)
{
	signed char original[64];
	unsigned char decompressed[64];
	unsigned char compressed[128];
	int nbytes;
	int i;

	for (i = 0; i < 64; i += 1) {
		original[i] = (signed char)(i % 100);
	}
	nbytes = fits_rcomp_byte(original, 64, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	/* Truncate data */
	fail_if(fits_rdecomp_byte(compressed, nbytes / 2, decompressed, 64,
		32) == 0);
}

/*
 * Test with more output pixels requested than were compressed
 */
static void
test_rdecomp_nx_mismatch(void)
{
	int original[32];
	unsigned int decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Compress 32 values */
	for (i = 0; i < 32; i += 1) {
		original[i] = i * 10;
	}
	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	/* Try to decompress expecting 64 values - should fail */
	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 16) == 0);
}

/*
 * Test large nblock (larger than nx)
 */
static void
test_rcomp_nblock_larger_than_nx(void)
{
	int original[16];
	unsigned int decompressed[16];
	unsigned char compressed[128];
	int nbytes;
	int i;

	for (i = 0; i < 16; i += 1) {
		original[i] = i * 5;
	}

	/* nblock=32 but only 16 elements */
	nbytes = fits_rcomp(original, 16, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 16, 32) != 0);

	for (i = 0; i < 16; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test with very large differences that push nbits handling
 */
static void
test_rcomp_nbits_edge(void)
{
	int original[32];
	unsigned int decompressed[32];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Data designed to exercise nbits boundary conditions */
	for (i = 0; i < 32; i += 1) {
		/* Alternate between values that differ by powers of 2 */
		if (i % 4 == 0) {
			original[i] = 0;
		} else if (i % 4 == 1) {
			original[i] = 0x7F;
		} else if (i % 4 == 2) {
			original[i] = 0x7FFF;
		} else {
			original[i] = 0x7FFFFF;
		}
	}

	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 32, 16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test decompression with extra bytes (triggers unused bytes warning)
 */
static void
test_rdecomp_extra_bytes(void)
{
	int original[32];
	unsigned int decompressed[32];
	unsigned char compressed[512];
	int nbytes;
	int i;

	for (i = 0; i < 32; i += 1) {
		original[i] = i * 10;
	}
	nbytes = fits_rcomp(original, 32, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	/* Add extra bytes at end */
	compressed[nbytes] = 0xFF;
	compressed[nbytes + 1] = 0xFF;

	/* Decompress with extra bytes - should warn but succeed */
	fail_if(fits_rdecomp(compressed, nbytes + 2, decompressed, 32,
		16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short with extra bytes
 */
static void
test_rdecomp_short_extra_bytes(void)
{
	short original[32];
	unsigned short decompressed[32];
	unsigned char compressed[256];
	int nbytes;
	int i;

	for (i = 0; i < 32; i += 1) {
		original[i] = (short)(i * 100);
	}
	nbytes = fits_rcomp_short(original, 32, compressed, sizeof compressed,
		16);
	fail_if(nbytes <= 0);

	compressed[nbytes] = 0xFF;
	compressed[nbytes + 1] = 0xFF;

	fail_if(fits_rdecomp_short(compressed, nbytes + 2, decompressed, 32,
		16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with extra bytes
 */
static void
test_rdecomp_byte_extra_bytes(void)
{
	signed char original[32];
	unsigned char decompressed[32];
	unsigned char compressed[128];
	int nbytes;
	int i;

	for (i = 0; i < 32; i += 1) {
		original[i] = (signed char)(i * 2);
	}
	nbytes = fits_rcomp_byte(original, 32, compressed, sizeof compressed,
		16);
	fail_if(nbytes <= 0);

	compressed[nbytes] = 0xFF;
	compressed[nbytes + 1] = 0xFF;

	fail_if(fits_rdecomp_byte(compressed, nbytes + 2, decompressed, 32,
		16) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((signed char)decompressed[i] != original[i]);
	}
}

/*
 * Test with data that specifically triggers fs==fsmax in decompression
 * with specific nbits conditions
 */
static void
test_rcomp_fsmax_nbits_path(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[2048];
	int nbytes;
	int i;

	/* Very high entropy data with maximum differences */
	for (i = 0; i < 64; i += 1) {
		/* Alternating extreme values */
		original[i] = (i % 2) ? (int)0x7FFFFFFF : (int)0x80000001;
	}

	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 32) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test with data requiring specific bit alignment
 */
static void
test_rcomp_bit_alignment(void)
{
	int original[48];
	unsigned int decompressed[48];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Pattern to test different bit alignments */
	for (i = 0; i < 48; i += 1) {
		original[i] = ((i * 0x1234567) ^ (i << 16)) & 0x7FFFFFFF;
	}

	nbytes = fits_rcomp(original, 48, compressed, sizeof compressed, 16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 48, 16) != 0);

	for (i = 0; i < 48; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

/*
 * Test short with specific nbits>0 path in fsmax decoding
 * Using small block size to affect bit alignment
 */
static void
test_rcomp_short_nbits_path(void)
{
	short original[64];
	unsigned short decompressed[64];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Pattern with extreme alternating values using small blocks */
	for (i = 0; i < 64; i += 1) {
		original[i] = (short)((i % 2) ? 32767 : -32768);
	}

	/* Use block size 8 to cause different bit alignment */
	nbytes = fits_rcomp_short(original, 64, compressed, sizeof compressed,
		8);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 64,
		8) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with specific nbits>0 path in fsmax decoding
 */
static void
test_rcomp_byte_nbits_path(void)
{
	signed char original[64];
	unsigned char decompressed[64];
	unsigned char compressed[512];
	int nbytes;
	int i;

	/* Pattern with extreme alternating values */
	for (i = 0; i < 64; i += 1) {
		original[i] = (signed char)((i % 2) ? 127 : -128);
	}

	/* Use block size 8 */
	nbytes = fits_rcomp_byte(original, 64, compressed, sizeof compressed,
		8);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 64,
		8) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((signed char)decompressed[i] != original[i]);
	}
}

/*
 * Test with different block sizes to test bit alignment edge cases
 */
static void
test_rcomp_various_block_sizes(void)
{
	int original[96];
	unsigned int decompressed[96];
	unsigned char compressed[1024];
	int nbytes;
	int i;
	int block_sizes[] = { 8, 12, 20, 24 };

	/* Create pattern with high variance */
	for (i = 0; i < 96; i += 1) {
		original[i] = (i % 3) ? ((i * 999983) % 10000000) : 0;
	}

	for (i = 0; i < 4; i += 1) {
		int nblock = block_sizes[i];

		nbytes = fits_rcomp(original, 96, compressed,
			sizeof compressed, nblock);
		fail_if(nbytes <= 0);

		fail_if(fits_rdecomp(compressed, nbytes, decompressed, 96,
			nblock) != 0);

		for (int j = 0; j < 96; j += 1) {
			fail_if((int)decompressed[j] != original[j]);
		}
	}
}

/*
 * Test with primes as array elements (high entropy)
 */
static void
test_rcomp_prime_sequence(void)
{
	/* Large primes with significant differences */
	int primes[] = {
		2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
		1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061,
		10007, 10009, 10037, 10039, 10061, 10067
	};
	unsigned int decompressed[32];
	unsigned char compressed[512];
	int nbytes;
	int i;

	nbytes = fits_rcomp(primes, 32, compressed, sizeof compressed, 8);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 32, 8) != 0);

	for (i = 0; i < 32; i += 1) {
		fail_if((int)decompressed[i] != primes[i]);
	}
}

/*
 * Test short with mixed patterns in multiple blocks
 */
static void
test_rcomp_short_mixed_blocks(void)
{
	short original[128];
	unsigned short decompressed[128];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Block 0-31: constant */
	for (i = 0; i < 32; i += 1) {
		original[i] = 1000;
	}
	/* Block 32-63: gradual */
	for (i = 32; i < 64; i += 1) {
		original[i] = (short)(i * 10);
	}
	/* Block 64-95: extreme alternating */
	for (i = 64; i < 96; i += 1) {
		original[i] = (short)((i % 2) ? 32000 : -32000);
	}
	/* Block 96-127: random-like */
	for (i = 96; i < 128; i += 1) {
		original[i] = (short)((i * 7919) % 30000);
	}

	nbytes = fits_rcomp_short(original, 128, compressed, sizeof compressed,
		32);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_short(compressed, nbytes, decompressed, 128,
		32) != 0);

	for (i = 0; i < 128; i += 1) {
		fail_if((short)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with boundary values
 */
static void
test_rcomp_byte_boundaries(void)
{
	signed char original[64];
	unsigned char decompressed[64];
	unsigned char compressed[256];
	int nbytes;
	int i;

	/* Pattern hitting various byte boundaries */
	for (i = 0; i < 64; i += 1) {
		int val = (i * 5) % 256;
		if (val > 127) {
			val -= 256;
		}
		original[i] = (signed char)val;
	}

	nbytes = fits_rcomp_byte(original, 64, compressed, sizeof compressed,
		16);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed, 64,
		16) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((signed char)decompressed[i] != original[i]);
	}
}

/*
 * Test byte with extreme fsmax triggering data
 * Tries to hit the specific nbits branch in byte decompression
 */
static void
test_rcomp_byte_fsmax_extreme(void)
{
	signed char original[128];
	unsigned char decompressed[128];
	unsigned char compressed[1024];
	int nbytes;
	int i;

	/* Extreme alternating pattern with full byte range */
	for (i = 0; i < 128; i += 1) {
		/* Alternate between -128 and 127 for maximum difference */
		original[i] = (signed char)((i % 2) ? 127 : -128);
	}

	/* Try multiple block sizes to trigger different bit alignments */
	int block_sizes[] = { 4, 8, 16 };
	for (int b = 0; b < 3; b += 1) {
		nbytes = fits_rcomp_byte(original, 128, compressed,
			sizeof compressed, block_sizes[b]);
		fail_if(nbytes <= 0);

		fail_if(fits_rdecomp_byte(compressed, nbytes, decompressed,
			128, block_sizes[b]) != 0);

		for (i = 0; i < 128; i += 1) {
			fail_if((signed char)decompressed[i] != original[i]);
		}
	}
}

/*
 * Test int fsmax with extreme patterns
 */
static void
test_rcomp_int_fsmax_extreme(void)
{
	int original[64];
	unsigned int decompressed[64];
	unsigned char compressed[2048];
	int nbytes;
	int i;

	/* Pattern that should trigger fsmax in decompression */
	/* Values that map to large unsigned differences */
	for (i = 0; i < 64; i += 1) {
		original[i] = (i % 2) ? (int)0xFFFFFFFF : 0;
	}

	nbytes = fits_rcomp(original, 64, compressed, sizeof compressed, 8);
	fail_if(nbytes <= 0);

	fail_if(fits_rdecomp(compressed, nbytes, decompressed, 64, 8) != 0);

	for (i = 0; i < 64; i += 1) {
		fail_if((int)decompressed[i] != original[i]);
	}
}

int
main(void)
{
	test_rcomp_roundtrip();
	test_rcomp_short_roundtrip();
	test_rcomp_byte_roundtrip();
	test_rcomp_uniform();
	test_rcomp_block_sizes();
	test_rcomp_large_values();
	test_rcomp_zeros();
	test_rcomp_alternating();
	test_rcomp_short_large_array();
	test_rcomp_byte_range();
	test_rcomp_negative_values();
	test_rcomp_high_entropy();
	test_rcomp_short_negative();
	test_rcomp_byte_negative();
	test_rcomp_small_block();
	test_rcomp_extreme_values();
	test_rcomp_short_extreme();
	test_rcomp_high_fs();
	test_rcomp_single_element();
	test_rcomp_decreasing();
	test_rdecomp_buffer_too_small();
	test_rdecomp_short_small_nx();
	test_rdecomp_byte_small_nx();
	test_rcomp_fsmax_path();
	test_rcomp_short_fsmax_path();
	test_rcomp_byte_fsmax_path();
	test_rcomp_varying_fs();
	test_rcomp_partial_block();
	test_rcomp_multi_block_zeros();
	test_rcomp_constant_multi_block();
	test_rdecomp_truncated();
	test_rdecomp_short_truncated();
	test_rdecomp_byte_truncated();
	test_rdecomp_nx_mismatch();
	test_rcomp_nblock_larger_than_nx();
	test_rcomp_nbits_edge();
	test_rdecomp_extra_bytes();
	test_rdecomp_short_extra_bytes();
	test_rdecomp_byte_extra_bytes();
	test_rcomp_fsmax_nbits_path();
	test_rcomp_bit_alignment();
	test_rcomp_short_nbits_path();
	test_rcomp_byte_nbits_path();
	test_rcomp_various_block_sizes();
	test_rcomp_prime_sequence();
	test_rcomp_short_mixed_blocks();
	test_rcomp_byte_boundaries();
	test_rcomp_byte_fsmax_extreme();
	test_rcomp_int_fsmax_extreme();

	return 0;
}
