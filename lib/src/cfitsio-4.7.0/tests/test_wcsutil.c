/*
 * Tests for wcsutil.c - WCS coordinate conversion functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "test_macros.h"

#define TOLERANCE 1e-10

/*
 * Test CAR (Cartesian/linear) projection pixel to world
 */
static void
test_wldp_car(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAR";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
	fail_if(fabs(xpos - 180.0) > TOLERANCE);
	fail_if(fabs(ypos - 45.0) > TOLERANCE);
}

/*
 * Test CAR projection with offset
 */
static void
test_wldp_car_offset(void)
{
	int status = 0;
	double xpix = 110.0, ypix = 105.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAR";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
	fail_if(fabs(xpos - 180.1) > TOLERANCE);
	fail_if(fabs(ypos - 45.05) > TOLERANCE);
}

/*
 * Test CAR projection with rotation
 */
static void
test_wldp_car_rotation(void)
{
	int status = 0;
	double xpix = 110.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 45.0;
	char type[] = "-CAR";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test TAN (gnomonic) projection
 */
static void
test_wldp_tan(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-TAN";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test SIN (orthographic) projection
 */
static void
test_wldp_sin(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-SIN";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test ARC (zenithal equidistant) projection
 */
static void
test_wldp_arc(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-ARC";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test NCP (north celestial pole) projection
 */
static void
test_wldp_ncp(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-NCP";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test GLS (global sinusoidal) projection
 */
static void
test_wldp_gls(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-GLS";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test MER (Mercator) projection
 */
static void
test_wldp_mer(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-MER";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test AIT (Aitoff) projection
 */
static void
test_wldp_ait(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test STG (stereographic) projection
 */
static void
test_wldp_stg(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-STG";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test invalid projection type
 */
static void
test_wldp_invalid_projection(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-XXX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test invalid projection type (no leading dash)
 */
static void
test_wldp_no_dash(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "CAR";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test inverse CAR projection (world to pixel)
 */
static void
test_xypx_car(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAR";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
	fail_if(fabs(xpix - 100.0) > TOLERANCE);
	fail_if(fabs(ypix - 100.0) > TOLERANCE);
}

/*
 * Test inverse TAN projection
 */
static void
test_xypx_tan(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-TAN";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse SIN projection
 */
static void
test_xypx_sin(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-SIN";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse ARC projection
 */
static void
test_xypx_arc(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-ARC";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse NCP projection
 */
static void
test_xypx_ncp(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-NCP";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse GLS projection
 */
static void
test_xypx_gls(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-GLS";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse MER projection
 */
static void
test_xypx_mer(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-MER";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse AIT projection
 */
static void
test_xypx_ait(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse STG projection
 */
static void
test_xypx_stg(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-STG";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test inverse invalid projection
 */
static void
test_xypx_invalid(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-XXX";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 504);
}

/*
 * Test roundtrip (pixel to world to pixel)
 */
static void
test_roundtrip_car(void)
{
	int status = 0;
	double xpix_in = 150.0, ypix_in = 75.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAR";
	double xpos, ypos, xpix_out, ypix_out;

	status = ffwldp(xpix_in, ypix_in, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);
	fail_if(status != 0);

	status = 0;
	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix_out, &ypix_out, &status);
	fail_if(status != 0);

	fail_if(fabs(xpix_in - xpix_out) > TOLERANCE);
	fail_if(fabs(ypix_in - ypix_out) > TOLERANCE);
}

/*
 * Test with rotation for inverse
 */
static void
test_xypx_rotation(void)
{
	int status = 0;
	double xpos = 180.1, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 30.0;
	char type[] = "-CAR";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test with pre-existing status (forward)
 */
static void
test_wldp_prestatus(void)
{
	int status = 1;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAR";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 1);
}

/*
 * Test SIN projection with sins > 1.0 (error 501)
 */
static void
test_wldp_sin_bounds(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 0.0, yrefpix = 0.0;
	double xinc = 1.0, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-SIN";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test ARC projection with sins too large
 */
static void
test_wldp_arc_bounds(void)
{
	int status = 0;
	double xpix = 1000.0, ypix = 1000.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 0.0, yrefpix = 0.0;
	double xinc = 1.0, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-ARC";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test NCP projection with dect out of range
 */
static void
test_wldp_ncp_bounds(void)
{
	int status = 0;
	double xpix = 200.0, ypix = 200.0;
	double xref = 180.0, yref = 89.0;
	double xrefpix = 0.0, yrefpix = 0.0;
	double xinc = 1.0, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-NCP";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test GLS projection with dec too large
 */
static void
test_wldp_gls_bounds(void)
{
	int status = 0;
	double xpix = 200.0, ypix = 200.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 0.0, yrefpix = 0.0;
	double xinc = 1.0, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-GLS";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test MER projection with l out of range
 */
static void
test_wldp_mer_bounds(void)
{
	int status = 0;
	double xpix = 2000.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 0.0, yrefpix = 0.0;
	double xinc = 1.0, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-MER";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test AIT projection with l and m outside valid range
 */
static void
test_wldp_ait_bounds(void)
{
	int status = 0;
	double xpix = 500.0, ypix = 500.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 0.0, yrefpix = 0.0;
	double xinc = 1.0, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test AIT projection center point (l=0, m=0 branch)
 */
static void
test_wldp_ait_center(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
	fail_if(fabs(xpos - 180.0) > TOLERANCE);
	fail_if(fabs(ypos - 45.0) > TOLERANCE);
}

/*
 * Test AIT projection with offset (l,m != 0 branch)
 */
static void
test_wldp_ait_offset(void)
{
	int status = 0;
	double xpix = 110.0, ypix = 105.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.1, yinc = 0.1;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 0);
}

/*
 * Test AIT roundtrip
 */
static void
test_roundtrip_ait(void)
{
	int status = 0;
	double xpix_in = 110.0, ypix_in = 105.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.1, yinc = 0.1;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpos, ypos, xpix_out, ypix_out;

	status = ffwldp(xpix_in, ypix_in, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);
	fail_if(status != 0);

	status = 0;
	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix_out, &ypix_out, &status);
	fail_if(status != 0);

	fail_if(fabs(xpix_in - xpix_out) > 0.01);
	fail_if(fabs(ypix_in - ypix_out) > 0.01);
}

/*
 * Test TAN inverse with small sin(ra0) (alternate path)
 */
static void
test_xypx_tan_smallsin(void)
{
	int status = 0;
	double xpos = 0.1, ypos = 45.0;
	double xref = 0.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-TAN";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test various invalid CAR-like projections (e.g., -CAX, -CXR)
 */
static void
test_wldp_invalid_car_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type1[] = "-CAX";
	char type2[] = "-CXR";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type1, &xpos, &ypos, &status);
	fail_if(status != 504);

	status = 0;
	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type2, &xpos, &ypos, &status);
	fail_if(status != 504);
}

/*
 * Test invalid TAN-like projections
 */
static void
test_wldp_invalid_tan_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-TAX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test invalid NCP-like projections
 */
static void
test_wldp_invalid_ncp_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-NCX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test invalid GLS-like projections
 */
static void
test_wldp_invalid_gls_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-GLX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test invalid MER-like projections
 */
static void
test_wldp_invalid_mer_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-MEX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test invalid S*-like projections (not SIN or STG)
 */
static void
test_wldp_invalid_sin_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-SXX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test invalid A*-like projections (not ARC or AIT)
 */
static void
test_wldp_invalid_arc_variant(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-AXX";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 504);
}

/*
 * Test NCP forward dect==0 case
 */
static void
test_wldp_ncp_dect_zero(void)
{
	int status = 0;
	double xpix = 100.0, ypix = 100.0;
	double xref = 180.0, yref = 89.999;
	double xrefpix = 100.0, yrefpix = 0.0;
	double xinc = 0.01, yinc = 1.0;
	double rot = 0.0;
	char type[] = "-NCP";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test GLS forward l too large
 */
static void
test_wldp_gls_l_large(void)
{
	int status = 0;
	double xpix = 10000.0, ypix = 100.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.1, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-GLS";
	double xpos, ypos;

	status = ffwldp(xpix, ypix, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpos, &ypos, &status);

	fail_if(status != 501);
}

/*
 * Test inverse CAR-like invalid projections
 */
static void
test_xypx_invalid_car_variant(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAX";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 504);
}

/*
 * Test inverse TAN-like invalid projections
 */
static void
test_xypx_invalid_tan_variant(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.01, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-TAX";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 504);
}

/*
 * Test inverse with zero xinc (error 502)
 */
static void
test_xypx_zero_inc(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.0, yinc = 0.01;
	double rot = 0.0;
	char type[] = "-CAR";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 502);
}

/*
 * Test TAN inverse with sint<=0 (error 501)
 */
static void
test_xypx_tan_bounds(void)
{
	int status = 0;
	double xpos = 0.0, ypos = -45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-TAN";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 501);
}

/*
 * Test TAN inverse near pole (cos0<0.001 branch)
 */
static void
test_xypx_tan_pole(void)
{
	int status = 0;
	double xpos = 180.1, ypos = 89.95;
	double xref = 180.0, yref = 89.95;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-TAN";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 0);
}

/*
 * Test SIN inverse with sint<0 (error 501)
 */
static void
test_xypx_sin_bounds(void)
{
	int status = 0;
	double xpos = 0.0, ypos = -45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-SIN";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 501);
}

/*
 * Test STG inverse with dec too large (error 501)
 */
static void
test_xypx_stg_bounds(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 91.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-STG";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 501);
}

/*
 * Test AIT inverse with da too large (error 501)
 */
static void
test_xypx_ait_bounds(void)
{
	int status = 0;
	double xpos = 0.0, ypos = 45.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-AIT";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 501);
}

/*
 * Test NCP inverse with dec0==0 (equator, error 501)
 */
static void
test_xypx_ncp_equator(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 45.0;
	double xref = 180.0, yref = 0.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-NCP";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 501);
}

/*
 * Test GLS inverse with dec too large (error 501)
 */
static void
test_xypx_gls_bounds(void)
{
	int status = 0;
	double xpos = 180.0, ypos = 91.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-GLS";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 501);
}

/*
 * Test MER inverse with dec at pole (error 502)
 */
static void
test_xypx_mer_pole(void)
{
	int status = 0;
	double xpos = 180.0, ypos = -90.0;
	double xref = 180.0, yref = 45.0;
	double xrefpix = 100.0, yrefpix = 100.0;
	double xinc = 0.001, yinc = 0.001;
	double rot = 0.0;
	char type[] = "-MER";
	double xpix, ypix;

	status = ffxypx(xpos, ypos, xref, yref, xrefpix, yrefpix,
		xinc, yinc, rot, type, &xpix, &ypix, &status);

	fail_if(status != 502);
}

int
main(void)
{
	/* Forward transformations (pixel to world) */
	test_wldp_car();
	test_wldp_car_offset();
	test_wldp_car_rotation();
	test_wldp_tan();
	test_wldp_sin();
	test_wldp_arc();
	test_wldp_ncp();
	test_wldp_gls();
	test_wldp_mer();
	test_wldp_ait();
	test_wldp_stg();
	test_wldp_invalid_projection();
	test_wldp_no_dash();

	/* Forward boundary errors */
	test_wldp_prestatus();
	test_wldp_sin_bounds();
	test_wldp_arc_bounds();
	test_wldp_ncp_bounds();
	test_wldp_gls_bounds();
	test_wldp_mer_bounds();
	test_wldp_ait_bounds();
	test_wldp_ait_center();
	test_wldp_ait_offset();
	test_wldp_ncp_dect_zero();
	test_wldp_gls_l_large();

	/* Invalid projection type variants */
	test_wldp_invalid_car_variant();
	test_wldp_invalid_tan_variant();
	test_wldp_invalid_ncp_variant();
	test_wldp_invalid_gls_variant();
	test_wldp_invalid_mer_variant();
	test_wldp_invalid_sin_variant();
	test_wldp_invalid_arc_variant();

	/* Inverse transformations (world to pixel) */
	test_xypx_car();
	test_xypx_tan();
	test_xypx_sin();
	test_xypx_arc();
	test_xypx_ncp();
	test_xypx_gls();
	test_xypx_mer();
	test_xypx_ait();
	test_xypx_stg();
	test_xypx_invalid();
	test_xypx_rotation();

	/* Inverse boundary errors */
	test_xypx_zero_inc();
	test_xypx_tan_bounds();
	test_xypx_tan_pole();
	test_xypx_tan_smallsin();
	test_xypx_sin_bounds();
	test_xypx_stg_bounds();
	test_xypx_ait_bounds();
	test_xypx_ncp_equator();
	test_xypx_gls_bounds();
	test_xypx_mer_pole();

	/* Inverse invalid projection variants */
	test_xypx_invalid_car_variant();
	test_xypx_invalid_tan_variant();

	/* Roundtrip tests */
	test_roundtrip_car();
	test_roundtrip_ait();

	return 0;
}
