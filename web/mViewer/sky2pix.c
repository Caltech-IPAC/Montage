/*
Theme: Convert sky coordinate (ra,dec) of a given point to image pixel (i,j).
       Both image and the given sky coord are assumed to be the same
       coordinate system and epoch, only map pojection is performed using
       SAO wcs lib routines.
       
Written: March 23, 2011 (Mih-seh Kong)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include <wcs.h>
#include <coord.h>

char* fitsrhead (char *fname, int *lhead, int *nbhead);

extern FILE *fp_debug;

/*
int sky2pix (char *fitsHeader, double ra_in, double dec_in, int sysin,
    double epochin, int sysout, double epochout, double *xout, double *yout, 
    int *outbound, char *errmsg)
*/
int sky2pix (char *fname, double ra_in, double dec_in, int sysin,
    double epochin, int h, int sysout, double epochout, 
    double *xout, double *yout, int *outbound, char *errmsg)
{

    struct WorldCoor *wcs;
    
    char             *header;
   
    int              lhead, nbhead;     
    int              offscale;
    double           ra, dec;
    double           ra_val, dec_val;
    double           x, y;
    
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

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "From sky2pix\n");
        fprintf (fp_debug, "fname= [%s]\n", fname);
        fprintf (fp_debug, "ra_in= [%lf] dec_in= [%lf]\n", ra_in, dec_in);
        fprintf (fp_debug, "sysin= [%d] epochin= [%lf]\n", sysin, epochin);
        fprintf (fp_debug, "sysout= [%d] epochout= [%lf]\n", sysout, epochout);
        fflush(fp_debug);
    }


    
    if ((sysin != sysout) || ((epochin-epochout) > 0.0001)) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "call convertCoordinates\n");
            fflush(fp_debug);
        }
	
	convertCoordinates (sysin, epochin, ra_in, dec_in, sysout, epochout, 
	    &ra, &dec, 0.0); 
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returned convertCoordinates\n");
            fprintf (fp_debug, "ra= [%lf] dec= [%lf]\n", ra, dec);
            fflush(fp_debug);
        }
	
    }
    else {
        ra = ra_in;
	dec = dec_in;
    }

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "ra= [%lf] dec= [%lf]\n", ra, dec);
        fflush(fp_debug);
    }

    wcs2pix (wcs, ra, dec, &x, &y, &offscale);
	
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "x= [%lf] y= [%lf] offscale= [%d]\n", 
	    x, y, offscale);
        fflush(fp_debug);
    }


    if (wcs)
	free ((char *)wcs);

    if (wcs) {
	free (header);
    }

    *xout = x - 1.0;
    *yout = h - y;
    *outbound = offscale;
   
    
    pix2wcs (wcs, x, y, &ra_val, &dec_val);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "convert back: ra_val= [%lf] dec_val= [%lf]\n", 
	    ra_val, dec_val);
        fflush(fp_debug);
    }
    
    
    return (0);
}

