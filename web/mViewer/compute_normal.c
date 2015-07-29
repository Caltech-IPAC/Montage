#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <strings.h>
#include <sys/types.h>


void cal_xyz (double ra_in, double dec_in, double *x, double *y, double *z)
{
    double ra, dec;
    double cos_dec, pi, rpd;
    double len, xtmp, ytmp, ztmp;

    pi = 4.*atan(1.);

/*
    pi = 3.141592654;
*/

    rpd = pi/180.;
    
    ra = ra_in*rpd;
    dec = dec_in*rpd;

    cos_dec = cos(dec);
    xtmp = cos(ra)*cos_dec;
    ytmp = sin(ra)*cos_dec;
    ztmp = sin(dec);
   
    len = sqrt (xtmp*xtmp + ytmp*ytmp + ztmp*ztmp);

    *x = xtmp/len;
    *y = ytmp/len;
    *z = ztmp/len;
    
    return;
}


void compute_nrml (int npoly, double *ra, double *dec, 
    double **xnrml, double **ynrml, double **znrml)
{
    int             l;

    double    *x, *y, *z;
    double    *xn, *yn, *zn;

    x = (double *)malloc (npoly*sizeof(double));
    y = (double *)malloc (npoly*sizeof(double));
    z = (double *)malloc (npoly*sizeof(double));

    xn = (double *)malloc (npoly*sizeof(double));
    yn = (double *)malloc (npoly*sizeof(double));
    zn = (double *)malloc (npoly*sizeof(double));

    for (l=0; l<npoly; l++) {
	cal_xyz (ra[l], dec[l], &x[l], &y[l], &z[l]);
    }

    for (l=0; l<npoly-1; l++) {
        xn[l] = y[l]*z[l+1] - z[l]*y[l+1];
        yn[l] = z[l]*x[l+1] - x[l]*z[l+1];
        zn[l] = x[l]*y[l+1] - y[l]*x[l+1];
    }

    xn[npoly-1] = y[npoly-1]*z[0] - z[npoly-1]*y[0];
    yn[npoly-1] = z[npoly-1]*x[0] - x[npoly-1]*z[0];
    zn[npoly-1] = x[npoly-1]*y[0] - y[npoly-1]*x[0];
   
    free (x);
    free (y);
    free (z);
    
    *xnrml = xn;
    *ynrml = yn;
    *znrml = zn;

    return;
}

