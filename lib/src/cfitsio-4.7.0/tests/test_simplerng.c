/*
 * Tests for simplerng.c - simple random number generator.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "simplerng.h"
#include "test_macros.h"

/* Not in public header, but we want to test them. */
extern unsigned int simplerng_getuint(void);
extern double simplerng_getuniform_pr(unsigned int *u, unsigned int *v);

static void
test_state(void)
{
	unsigned int u, v;

	simplerng_setstate(12345, 67890);
	simplerng_getstate(&u, &v);
	fail_if(u != 12345);
	fail_if(v != 67890);
}

static void
test_srand(void)
{
	unsigned int u1, v1, u2, v2;

	simplerng_srand(42);
	simplerng_getstate(&u1, &v1);
	simplerng_srand(42);
	simplerng_getstate(&u2, &v2);
	fail_if(u1 != u2);
	fail_if(v1 != v2);
}

static void
test_getuint(void)
{
	unsigned int u1, u2;

	simplerng_srand(12345);
	u1 = simplerng_getuint();
	u2 = simplerng_getuint();
	fail_if(u1 == u2);
}

static void
test_getuniform_pr(void)
{
	unsigned int u = 12345, v = 67890;
	double val;
	int i;

	for (i = 0; i < 100; i += 1) {
		val = simplerng_getuniform_pr(&u, &v);
		fail_if(val < 0.0 || val >= 1.0);
	}
}

static void
test_uniform(void)
{
	double val;
	int i;
	int in_range = 1;

	simplerng_srand(12345);
	for (i = 0; i < 1000; i += 1) {
		val = simplerng_getuniform();
		if (val < 0.0 || val >= 1.0) {
			in_range = 0;
			break;
		}
	}
	fail_if(!in_range);
}

static void
test_norm(void)
{
	double sum = 0.0;
	double sumsq = 0.0;
	double mean, variance;
	int i;
	int n = 10000;

	simplerng_srand(54321);
	for (i = 0; i < n; i += 1) {
		double val = simplerng_getnorm();
		sum += val;
		sumsq += val * val;
	}
	mean = sum / n;
	variance = sumsq / n - mean * mean;

	fail_if(fabs(mean) > 0.1);
	fail_if(fabs(variance - 1.0) > 0.1);
}

static void
test_poisson(void)
{
	double sum = 0.0;
	double mean;
	double lambda = 5.0;
	int i;
	int n = 10000;

	simplerng_srand(99999);
	for (i = 0; i < n; i += 1) {
		int val = simplerng_getpoisson(lambda);
		fail_if(val < 0);
		sum += val;
	}
	mean = sum / n;
	fail_if(fabs(mean - lambda) > 0.2);
}

static void
test_poisson_large(void)
{
	double sum = 0.0;
	double mean;
	double lambda = 50.0;
	int i;
	int n = 10000;

	simplerng_srand(11111);
	for (i = 0; i < n; i += 1) {
		int val = simplerng_getpoisson(lambda);
		fail_if(val < 0);
		sum += val;
	}
	mean = sum / n;
	fail_if(fabs(mean - lambda) > 1.0);
}

static void
test_logfactorial(void)
{
	double lf300;

	fail_if(fabs(simplerng_logfactorial(0) - 0.0) > 0.001);
	fail_if(fabs(simplerng_logfactorial(1) - 0.0) > 0.001);
	fail_if(fabs(simplerng_logfactorial(5) - log(120.0)) > 0.001);
	fail_if(fabs(simplerng_logfactorial(10) - log(3628800.0)) > 0.001);

	/* Test large n (uses Stirling approximation for n > 254) */
	lf300 = simplerng_logfactorial(300);
	fail_if(lf300 < 1400.0 || lf300 > 1420.0);
}

int
main(void)
{
	test_state();
	test_srand();
	test_getuint();
	test_getuniform_pr();
	test_uniform();
	test_norm();
	test_poisson();
	test_poisson_large();
	test_logfactorial();

	return 0;
}
