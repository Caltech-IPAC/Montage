#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

/*
#include <fitsio.h>
#include <fitshead.h>
*/

#include <wcs.h>
#include <coord.h>


char* fitsrhead (char *fname, int *lhead, int *nbhead);
    

extern FILE *fp_debug;

int pix2sky (char *fname, int ix, int iy, int h, 
    int sysin, double epochin, int sysout, double epochout, 
    double *ra, double *dec, char *errmsg) 
{
    struct WorldCoor *wcs;

    char             *header;

    int              lhead, nbhead;
    int              offscl;

    double           xpix, ypix, x, y;
    double           ra_value, dec_value;
    double           raout, decout;
    
    int              debugfile = 0;

    
    errmsg[0] = '\0';
    header = (char *)fitsrhead (fname, &lhead, &nbhead);
    if (header == (char *)NULL) {
        sprintf (errmsg, "Cannot read FITS file %s\n", fname);
	return (-1);
    }


    wcs = wcsinit (header);
    if (nowcs (wcs)) {
	strcpy (errmsg, "Fail to create wcs structure");
	return (-1);
    }
    
    x = ix + 1.;
    y = (double)h - iy + 1.;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "From pix2sky\n");
        fprintf (fp_debug, "x= [%lf] y= [%lf]\n", x, y);
        fprintf (fp_debug, "sysin= [%d] epochin= [%lf]\n", sysin, epochin);
        fprintf (fp_debug, "sysout= [%d] epochout= [%lf]\n", sysout, epochout);
        fflush(fp_debug);
    }
   
    pix2wcs (wcs, x, y, &ra_value, &dec_value);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "offscal= [%d]\n", wcs->offscl);
        fflush(fp_debug);
    }

    if (wcs->offscl != 0) {
	strcpy (errmsg, "Illegal location: probably off the image.");
	return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "ra_value= %lf dec_value= %lf\n", 
	    ra_value, dec_value);
        fflush(fp_debug);
    }

    wcs2pix (wcs, ra_value, dec_value, &xpix, &ypix, &offscl);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "xpix= %lf ypix= %lf\n", xpix, ypix);
        fprintf (fp_debug, "(xpix-x)= %lf (ypix-y)= %lf\n", 
	    fabs(xpix-x), fabs(ypix-y));
        fflush(fp_debug);
    }

    
    if ((fabs(xpix-x) > 0.001) || (fabs(ypix-y) > 0.001)) {
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "xpix= %lf ypix= %lf\n", xpix, ypix);
            fflush(fp_debug);
        }

	strcpy (errmsg, "Illegal location: probably off the image.");
	return (-1);
    }


    if (wcs) {
	free ((char *)wcs);
    }
        
    if (wcs) {
	free (header);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "ra_value= %lf dec_value= %lf\n", 
	    ra_value, dec_value);
        fflush(fp_debug);
    }

    convertCoordinates (sysin, epochin, ra_value, dec_value, sysout, epochout, 
	&raout, &decout, 0.0); 
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "raout= %lf decout= %lf\n", raout, decout);
        fflush(fp_debug);
    }

    *ra = raout;
    *dec = decout;
    
    return (0);
}

