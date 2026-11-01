/*
 * Tests for pliocomp.c - PLIO compression functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_macros.h"

int pl_p2li(int *, int, short *, int);
int pl_l2pi(short *, size_t, int, int *, int);


static void
test_array(int *in, size_t siz)
{
	short linelist[100];
	int output[16];

	fail_if(pl_p2li(in, 1, linelist, siz) <= 0);
	fail_if(pl_l2pi(linelist, 100, 1, output, siz) != siz);

	for (size_t i = 0; i < siz; i += 1) {
		fail_if(output[i] != in[i]);
	}
}


static void
test_empty_input(void)
{
	int pixels[1] = { 0 };
	short linelist[100];

	fail_if(pl_p2li(pixels, 1, linelist, 0));
}

static void
test_data(void)
{
	int a0[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int a1[5] = { 42, 42, 42, 42, 42 };
	int a2[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int a3[10] = { 0, 0, 5, 5, 0, 0, 0, 10, 0, 0 };
	int a4[3] = { 0, 100000, 0 };
	int a5[4] = { 1, 10000, 1, 10000 };
	int a6[1] = { 99 };
	int a7[6] = { 1, 0, 1, 0, 1, 0 };
	int a8[5] = { 100, 80, 60, 40, 20 };
	int a9[3] = { 10000, 1, 10000 };

	test_array(a0, 10);
	test_array(a1, 5);
	test_array(a2, 8);
	test_array(a3, 10);
	test_array(a4, 3);
	test_array(a5, 4);
	test_array(a6, 1);
	test_array(a7, 6);
	test_array(a8, 5);
	test_array(a9, 3);
}

static void
test_negative_clamp(void)
{
	int pixels[3] = { -5, 10, -3 };
	short linelist[100];
	int output[3];

	fail_if(pl_p2li(pixels, 1, linelist, 3) <= 0);
	fail_if(pl_l2pi(linelist, 100, 1, output, 3) != 3);

	fail_if(output[0] != 0);
	fail_if(output[1] != 10);
	fail_if(output[2] != 0);
}

static void
test_partial_decode(void)
{
	int pixels[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	short linelist[100];
	int output[5];

	fail_if(pl_p2li(pixels, 1, linelist, 10) <= 0);
	fail_if(pl_l2pi(linelist, 100, 1, output, 5) != 5);

	for (int i = 0; i < 5; i += 1) {
		fail_if(output[i] != pixels[i]);
	}
}

static void
test_l2pi_empty(void)
{
	short linelist[10] = { 0 };
	int output[10];

	linelist[3] = 0;
	fail_if(pl_l2pi(linelist, 100, 1, output, 0) != 0);
}

static void
test_decode_offset(void)
{
	int pixels[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	short linelist[100];
	int output[5];

	fail_if(pl_p2li(pixels, 1, linelist, 10) <= 0);
	fail_if(pl_l2pi(linelist, 100, 5, output, 5) != 5);
}

static void
test_decode_more_than_encoded(void)
{
	int pixels[5] = { 1, 2, 3, 4, 5 };
	short linelist[100];
	int output[10];

	fail_if(pl_p2li(pixels, 1, linelist, 5) <= 0);
	fail_if(pl_l2pi(linelist, 100, 1, output, 10) != 10);

	for (int i = 0; i < 5; i += 1) {
		fail_if(output[i] != pixels[i]);
	}
	for (int i = 5; i < 10; i += 1) {
		fail_if(output[i] != 0);
	}
}

static void
test_old_format_linelist(void)
{
	int output[10];
	short linelist[20];  /* old format: ll_src[3] = linelist[2] > 0 */

	linelist[0] = 0;
	linelist[1] = 0;
	linelist[2] = 6;  /* becomes ll_src[3], triggers old format */
	linelist[3] = (4 << 12) | 5;  /* opcode 4, run of 5 pixels */
	linelist[4] = 0;
	linelist[5] = 0;

	fail_if(pl_l2pi(linelist, 20, 1, output, 5) != 5);
}

static void
test_opcode3_negative_delta(void)
{
	short linelist[20];  /* manually construct with opcode 3 */
	int output[3];

	linelist[0] = 0;
	linelist[1] = 7;      /* llfirt - 1 */
	linelist[2] = -100;   /* < 0 triggers new format */
	linelist[3] = 12;     /* length low word */
	linelist[4] = 0;      /* length high word */
	linelist[5] = 0;
	linelist[6] = 0;
	linelist[7] = (2 << 12) | 10;   /* opcode 2: pv += 10, so pv = 11 */
	linelist[8] = (4 << 12) | 1;    /* opcode 4: run of 1 at pv=11 */
	linelist[9] = (3 << 12) | 5;    /* opcode 3: pv -= 5, so pv = 6 */
	linelist[10] = (4 << 12) | 1;   /* opcode 4: run of 1 at pv=6 */
	linelist[11] = (0 << 12) | 1;   /* opcode 0: 1 zero */

	fail_if(pl_l2pi(linelist, 20, 1, output, 3) != 3);
	fail_if(output[0] != 11);
	fail_if(output[1] != 6);
	fail_if(output[2] != 0);
}

int
main(void)
{
	test_empty_input();
	test_data();
	test_negative_clamp();
	test_partial_decode();
	test_l2pi_empty();
	test_decode_offset();
	test_decode_more_than_encoded();
	test_old_format_linelist();
	test_opcode3_negative_delta();

	return 0;
}
