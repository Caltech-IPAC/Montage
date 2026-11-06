
#define fail_if(x) \
	do { \
		if (x) { \
			fprintf(stderr, "%s test failed at line %u: %s\n", \
				__FILE__, __LINE__, #x); \
			exit(1); \
		} \
	} while(0)
#define fail_st(x) fail_if((x) || (status != 0))

#define call_01(ff, a)                fail_st(ff(a, &status))
#define call_02(ff, a, b)             fail_st(ff(a, b, &status))
#define call_03(ff, a, b, c)          fail_st(ff(a, b, c, &status))
#define call_04(ff, a, b, c, d)       fail_st(ff(a, b, c, d, &status))
#define call_05(ff, a, b, c, d, e)    fail_st(ff(a, b, c, d, e, &status))
#define call_06(ff, a, b, c, d, e, f) fail_st(ff(a, b, c, d, e, f, &status))
#define call_07(ff, a, b, c, d, e, f, g) \
	fail_st(ff(a, b, c, d, e, f, g, &status))
#define call_08(ff, a, b, c, d, e, f, g, h) \
	fail_st(ff(a, b, c, d, e, f, g, h, &status))
#define call_09(ff, a, b, c, d, e, f, g, h, i) \
	fail_st(ff(a, b, c, d, e, f, g, h, i, &status))
#define call_10(ff, a, b, c, d, e, f, g, h, i, j) \
	fail_st(ff(a, b, c, d, e, f, g, h, i, j, &status))
#define call_11(ff, a, b, c, d, e, f, g, h, i, j, k) \
	fail_st(ff(a, b, c, d, e, f, g, h, i, j, k, &status))
