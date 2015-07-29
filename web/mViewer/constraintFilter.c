/*
    case within: radius is in degrees;

    case polygon: istatus == 1 : inside the box
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>


void cal_xyz (double ra, double dec, double *x, double *y, double *z);
void compute_nrml (int npoly, double *ra, double *dec, 
    double **xnrml, double **ynrml, double **znrml);


int isWithin (double x, double y, double z, 
    double ra0, double dec0, double radius) { 

    int    isIn;
    double x0, y0, z0;
    double dot, cos_r;
    double pi, rpd;

    int    debug = 0;
    
    pi = 3.141592654;
    rpd = pi/180.;
    
    cal_xyz (ra0, dec0, &x0, &y0, &z0);

    if (debug) {
        printf ("From isWithin: ra0= [%lf] dec0= [%lf] radius= [%lf]\n", 
	    ra0, dec0, radius);
        printf ("x0= [%lf] y0= [%lf] z0= [%lf]\n", x0, y0, z0);
        printf ("x= [%lf] y= [%lf] z= [%lf]\n", x, y, z);
	fflush (stdout);
    }

    radius *= rpd; 
    cos_r = cos (radius);
    
/*
    dot product
*/
    dot = x0*x + y0*y + z0*z;
    
    if (debug) {
        printf ("cos_r= [%lf] dot= [%lf]\n", cos_r, dot);
	fflush (stdout);
    }


    if (dot > cos_r)
        isIn = 1;
    else 
        isIn = 0;
    
    return (isIn);
}


int inbox (double x0, double y0, double z0, int npoly,
    double *xn, double *yn, double *zn)
{
    int            istatus, l, last_isign, isign, sign_change;
    int            debug;
    double         cos_alpha;


    debug = 0;

    if (debug) {
        printf ("\nfrom inbox:\n");
        printf ("x= %lf y= %lf z= %lf\n", x0, y0, z0);
        for (l=0; l<npoly; l++) {
	    printf ("normalVec l= %d x= %lf y= %lf z= %lf\n", 
	        l, xn[l], yn[l], zn[l]);
        }
	fflush (stdout);
    }


    cos_alpha = x0*xn[0] + y0*yn[0] + z0*zn[0];
    
    if (cos_alpha <= 1.0e-15) last_isign = -1;
    else last_isign = 1;
    
    if (debug) {
        printf ("\ncos_alpha= [%lf]\n", cos_alpha);
        printf ("last_isign= [%d]\n", last_isign);
	fflush (stdout);
    }
  
  
    sign_change = 0;
    l = 1;
    while ((l < npoly) && (sign_change == 0)) {
        cos_alpha = x0*xn[l] + y0*yn[l] + z0*zn[l];
	
	if (cos_alpha <= 1.0e-15) isign = -1;
	else isign = 1;
    
        if (debug) {
            printf ("\nl= [%d] cos_alpha= [%lf]\n", l, cos_alpha);
            printf ("last_isign= [%d] isign= [%d]\n", last_isign, isign);
	    fflush (stdout);
        }
  
	if (isign*last_isign <= 0) sign_change = 1; 
        
	last_isign = isign;

	if (debug) {
            printf ("\nsign_change= [%d]\n", sign_change);
	    fflush (stdout);
        }
  
        
	l++;
    }

    if ((sign_change == 0) && (isign == -1)) 
	istatus = 1;
    else istatus = 0;

    return (istatus);
}

