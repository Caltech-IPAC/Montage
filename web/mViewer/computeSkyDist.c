/*
Theme: Compute the distance between two sky position
    (ra1,dec1) and (ra2,dec2); input are in degrees.



Date Written: March 18, 2011 (Mih-seh Kong) 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>


void cal_xyz (double ra, double dec, double *x, double *y, double *z);


extern FILE *fp_debug;

int computeSkyDist (double ra0, double dec0, double ra, double dec,
    double *dist)
{
    double cos_alpha;
    double r;
    double pi;
    double dpr;
    double   x0, y0, z0;
    double   x, y, z;


    int    debugfile = 0;


    pi = 4.*atan(1.);
/*
    pi = 3.141592654;
*/

    dpr = 180./pi;
    
    cal_xyz (ra0, dec0, &x0, &y0, &z0);
    cal_xyz (ra, dec, &x, &y, &z);
    
    cos_alpha = x*x0 + y*y0 + z*z0;
    r = dpr * acos (cos_alpha); 

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "From computSkyDist\n");
        fprintf (fp_debug, "cos_alpha= [%lf]\n", cos_alpha);
        fprintf (fp_debug, "r= [%lf]\n", r);
        fflush(fp_debug);
    }
    
    
    if (errno != 0)
        return (-1);

    *dist = r;

    return (0);
}


