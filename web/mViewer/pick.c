#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <svc.h>

#include <fitsio.h>
#include <wcs.h>
#include <fitshead.h>

#include <mtbl.h>
#include <coord.h>

#include "mviewer.h"
#include "fitshdr.h"


int getFitshdr (char *fname, struct FitsHdr *hdr);
char* fitsrhead (char *fname, int *lhead, int *nbhead);
    
int pix2sky (char *fname, int xmap, int ymap, int h, int sysin,
    double epochin, int sysout, double epochout, double *ra, double *dec,
    char *errmsg);

int sky2pix (char *fname, double ra, double dec, int sysin,
    double epochin, int h, int sysout, double epochout, 
    double *ix, double *iy, int *outbound, char *errmsg);

int getTblNrows (char *fname, char *errmsg);
                
char *strtrim (char *);
int str2Double (char *str, double *dval, char *errmsg);

void cal_xyz (double ra, double dec, double *x, double *y, double *z);
void compute_nrml (int npoly, double *ra_arr, double *dec_arr,
    double **xnrml_arr, double **ynrml_arr, double ** znrml_arr);

int computeSkyDist (double ra0, double dec0, double ra, double dec, 
    double *dist);

int parseCsysstr (char *csysstrIn, int *sys, double *epoch, char *errmsg);

int  inbox (double x0, double y0, double z0,
    int npoly, double *xnrml_arr, double *ynrml_arr, double *znrml_arr);


extern FILE *fp_debug;

int readPixelValue (char *fname, int ixpix, int iypix, int w, int h, 
    int xflip, int yflip, double *pvalue, char *errmsg)
{
    
    fitsfile         *fitsptr;
    
    int              ix, iy;
    int              istatus;
    int              nullcnt, status, istatus1;
    long             fpixel[4];
    long             npix;
    
    double           *data;

    int              debugfile = 0;


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "fname= [%s]\n", fname);
	
	fprintf (fp_debug, "ixpix= [%d] iypix= [%d] w= [%d] h= [%d]\n", 
	    ixpix, iypix, w, h);
	fprintf (fp_debug, "xflip= [%d] yflip= [%d]\n", xflip, yflip);
	fflush (fp_debug);
    }
        

/*
    Read pixel value
*/
    if (xflip)
        ix = w - ixpix;
    else
        ix = ixpix + 1;
    
    if (yflip)
        iy = h - iypix;
    else
        iy = iypix + 1;

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "ix= [%d] iy= [%d]\n", ix, iy);
	fflush (fp_debug);
    }
        

    istatus = 0;
    if (fits_open_file (&fitsptr, fname, READONLY, &istatus)) {

	sprintf (errmsg, "Failed to open FITS file %s\n", fname);
	return (-1);
    } 
  
    npix = 1;
    data = (double *)malloc(npix*sizeof(double *));

    fpixel[0] = ix;
    fpixel[1] = iy;
    fpixel[2] = 1;


    istatus = 0;
    status = 0;

    istatus = fits_read_pix (fitsptr, TDOUBLE, fpixel, npix, NULL,
        (void *)data, &nullcnt, &status);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "istatus= [%d] status= [%d]\n", 
	    istatus, status);
	fprintf (fp_debug, "ix= [%d] iy= [%d] data[0]= [%lf]\n", 
	    ix, iy, data[0]);
	fflush (fp_debug);
    }

    if (istatus == -1) {
        sprintf (errmsg, "Failed to read fits file\n");
	return (-1);
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "nullcnt= [%d]\n", nullcnt);
	fprintf (fp_debug, "data[0]= [%lf]\n", data[0]);
        fflush (fp_debug);
    }

    *pvalue = data[0];
    
    istatus = fits_close_file (fitsptr, &istatus1);

    return (0);
}



int pick (struct Mviewer *param)
{
    struct FitsHdr hdr;
    
    char             impath[1024];
    char             fpath[1024];
    char             rastr[40], decstr[40];
    char             imcsys[40];
    char             colname[1024];
    char             str[1024];
    
    char             lonstr[40];
    char             latstr[40];
    
    int              offscl, baddata;
    
    int              npoly;
    int              ix[4], iy[4];

    int              isysim, isystbl, isysout;
    int              istatus, istatus1, istatus2;
    int              istatus_inbox;
    int              istatus_reverse;

    int              ncols, irow;
    int              l, i, j, k;
    int              icol_ra, icol_dec;
    int              icollon[4], icollat[4];

    int              minrow;
    
    int              niminfotbl;
    int              niminfoarr;
    
    int              ixpix;
    int              iypix;

    int              pickrow_cnt;
    int              pickrow_recno[200];
    double           pick_cra[200], pick_cdec[200];
    int              pickix[200][4], pickiy[200][4];
    double           pickra[200][4], pickdec[200][4];
    
    double           epochim, epochtbl, epochout;
    double           ra_im, dec_im;

    double           ra0, dec0;
    double           *ra, *dec, *xnrml, *ynrml, *znrml;
    double           *ra_reverse, *dec_reverse;

    double           raval, decval;
    double           x0, y0, z0, xval, yval, zval;
    double           x[4], y[4];
   
    double           dist;
    double           ramin, decmin, distmin;
    double           xmin, ymin;

    int              debugfile = 1;

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "From pick: iscolor= [%d]\n", param->iscolor);
	fflush (fp_debug);
    }


/*
    Extract fits header from shrunkimpath
*/
//    svc_debug (stdout);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "From pick: iscolor= [%d]\n", param->iscolor);
	fflush (fp_debug);
    }
       
    if (param->iscolor)
        sprintf (impath, "%s/%s", param->directory, param->shrunkredfile);
    else
        sprintf (impath, "%s/%s", param->directory, param->shrunkimfile);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "impath= [%s]\n", impath);
	fflush (fp_debug);
    }


    istatus = getFitshdr (impath, &hdr);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "returned getFitshdr: istatus= [%d]\n", istatus);
	fflush (fp_debug);
    }
 
    if (istatus == -1) {
	sprintf (param->errmsg, 
	    "Failed to extract FITS image header: %s", hdr.errmsg);
	return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "ns= [%d] nl= [%d]\n", hdr.ns, hdr.nl);
	fprintf (fp_debug, "csysstr= [%s]\n", hdr.csysstr);
	fprintf (fp_debug, "epochstr= [%s]\n", hdr.epochstr);
	fprintf (fp_debug, "nowcs= [%d]\n", hdr.nowcs);
	fflush (fp_debug);
    }

    if (param->nowcs == -1) {
        param->nowcs = hdr.nowcs;
    }

    if (!param->nowcs) {
       
	sprintf (imcsys, "%s %s", hdr.csysstr, hdr.epochstr);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "imcsys= [%s]\n", imcsys);
	    fprintf (fp_debug, "call parseCsysstr\n");
	    fflush (fp_debug);
        }
    
        istatus = parseCsysstr (imcsys, &isysim, &epochim, param->errmsg);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned parseCsysstr: istatus=[%d]\n", 
	        istatus);
	    fflush (fp_debug);
        }
    
        if (istatus == -1) {
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "Error parseCsysstr [%s]\n", param->imcsys);
	        fflush (fp_debug);
            }
	    return (-1); 
        }
   

        ixpix = (int) (param->xs + 0.5);
        iypix = (int) (param->ys + 0.5);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "call pix2sky: ixpix= [%d] iypix= [%d]\n",
	        ixpix, iypix);
	    fflush (fp_debug);
        }
   
        

        istatus = pix2sky (impath, ixpix, iypix, hdr.nl, 
            isysim, epochim, isysim, epochim, &ra_im, &dec_im, param->errmsg); 
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "returned pix2sky: istatus= [%d]\n", istatus);
	    fflush (fp_debug);
        }
	
	if (istatus == -1) {
	    return (-1); 
        }
        

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "ra_im = [%lf] dec_im= [%lf]\n", ra_im, dec_im);
	    fprintf (fp_debug, "call parseCsysstr\n");
            fflush (fp_debug);
        }


        istatus = parseCsysstr (param->pickcsys, &isysout, &epochout, 
	    param->errmsg);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned parseCsysstr: istatus=[%d]\n", 
	        istatus);
	    fflush (fp_debug);
        }
    
        if (istatus == -1) {
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "Error parseCsysstr [%s]\n", 
		    param->pickcsys);
	        fflush (fp_debug);
            }
        
	    return (-1); 
        }
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "isysim = [%d] epochim= [%lf]\n", 
	        isysim, epochim);
            fprintf(fp_debug, "isysout = [%d] epochout= [%lf]\n", 
	        isysout, epochout);
            fflush (fp_debug);
        }
    
        if ((isysim != isysout) || (epochim != epochout)) {
        
            convertCoordinates (isysim, epochim, ra_im, dec_im, 
	        isysout, epochout, &param->rapick, &param->decpick, 0.0); 
       
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "\nconvertCoordnates\n");
                fflush (fp_debug);
	    }
        }
        else {
            param->rapick = ra_im;
	    param->decpick = dec_im;
        }

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "rapick = [%lf] decpick= [%lf]\n", 
	        param->rapick, param->decpick);
            fflush (fp_debug);
        }

/*
    Convert rapick, decpick to sexagesimal
*/
        istatus = degreeToSex (param->rapick, param->decpick, 
	    &lonstr[0], &latstr[0]);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "returned degreeToSex: istatus= [%d]\n", istatus);
            fflush (fp_debug);
        }
        
	if (istatus < 0) {
	    sprintf (param->errmsg, 
	        "Failed to convert (%lf %lf) to sexagesimal representation",
		param->rapick, param->decpick); 
	    
	    return (-1); 
        }

	strcpy (param->sexrapick, lonstr);
	strcpy (param->sexdecpick, latstr);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf(fp_debug, "rapick = [%lf] decpick= [%lf]\n", 
	        param->rapick, param->decpick);
	    fprintf(fp_debug, "lonstr= %s latstr= %s\n", lonstr, latstr);
            fflush (fp_debug);
        }

    } 
    
    param->xpick = ixpix;
    param->ypick = iypix;
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf(fp_debug, "xpick = [%d] ypick= [%d]\n", 
	    param->xpick, param->ypick);
        fprintf(fp_debug, "hdr.ns = [%d] hdr.nl= [%d]\n", hdr.ns, hdr.nl);
        fflush (fp_debug);
    } 

    if ((param->xpick < 0) ||
        (param->xpick >= hdr.ns) || 
        (param->ypick < 0) || 
        (param->ypick >= hdr.nl)) {

        sprintf (param->errmsg, 
	    "pixel location outside image boundary: (0:%d, 0:%d)",
	    hdr.ns, hdr.nl);
	return (-1); 

    }


/*
    Read pixel value
*/
    if (param->iscolor) {

        sprintf (fpath, "%s/%s", 
	    param->directory, param->shrunkredfile);
	
	istatus = readPixelValue (fpath, param->xpick, param->ypick, 
	    hdr.ns, hdr.nl, param->xflip, param->yflip, &param->pickrval, 
	    param->errmsg);
       
        if (istatus == -1) {
	    return (-1); 
        }	
	
        sprintf (fpath, "%s/%s", 
	    param->directory, param->shrunkgrnfile);
	
	istatus = readPixelValue (fpath, param->xpick, param->ypick, 
	    hdr.ns, hdr.nl, param->xflip, param->yflip, &param->pickgval, 
	    param->errmsg);
        
        if (istatus == -1) {
	    return (-1); 
        }	
	
        sprintf (fpath, "%s/%s", 
	    param->directory, param->shrunkbluefile);
	
	istatus = readPixelValue (fpath, param->xpick, param->ypick, 
	    hdr.ns, hdr.nl, param->xflip, param->yflip, &param->pickbval, 
	    param->errmsg);
        
        if (istatus == -1) {
	    return (-1); 
        }	
	
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, 
		"pickrval= [%lf] pickgval= [%lf] pickbval= [%lf]\n",
		param->pickrval, param->pickgval, param->pickbval);
            fflush (fp_debug);
        }
    }
    else {
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "call readPixelvalue\n");
            fflush (fp_debug);
        }
        
	istatus = readPixelValue (impath, param->xpick, param->ypick, 
	    hdr.ns, hdr.nl, param->xflip, param->yflip, &param->pickval, 
	    param->errmsg);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "returned readPixelvalue: istatus= [%d]\n",
	        istatus);
            fflush (fp_debug);
        }
        
	if (istatus == -1) {
	    return (-1); 
        }	
           
        strcpy (param->bunit, hdr.bunit);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "bunit= [%s]\n", param->bunit);
            fprintf(fp_debug, "pickval= [%lf]\n", param->pickval);
            fflush (fp_debug);
        }
    }
        
    
    if ((!param->srcpick) || (param->noverlay == 0)) 
        return (0);
        
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf(fp_debug, ": noverlay= [%d]\n", param->noverlay);
        fflush (fp_debug);
    }
	
/*
    Process srctbl to find mintbl
*/
    npoly = 4;
    ra = (double *)malloc (npoly*sizeof(double));
    dec = (double *)malloc (npoly*sizeof(double));
    ra_reverse = (double *)malloc (npoly*sizeof(double));
    dec_reverse = (double *)malloc (npoly*sizeof(double));

/*
    Process srctbl to find mintbl
*/
    distmin = 10000.;
    param->mintbl[0] = '\0';

    for (l=0; l<param->noverlay; l++) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "l= [%d] type= [%s] filename= [%s]\n", 
		l, param->overlay[l].type, param->overlay[l].dataFile);
            fflush (fp_debug);
        }

            
	if (strcasecmp (param->overlay[l].type, "catalog") != 0)
	    continue;


/*
    First find ra and dec column from tinfo array;
*/
        sprintf (fpath, "%s/%s", param->directory, 
	    param->overlay[l].dataFile); 
            
	ncols = topen (fpath);
	    
	if (ncols <= 0) {
            continue;
        }
        

        icol_ra = -1;
	icol_dec = -1;
	for (i=0; i<ncols; i++) {
                
	    strcpy (colname, tinfo(i));

	    if (strcasecmp (colname, "ra") == 0) {
		icol_ra = i;
	    }
	    if (strcasecmp (colname, "dec") == 0) {
		icol_dec = i;
	    }
	}

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "icol_ra= [%d] icol_dec= [%d]\n", 
	        icol_ra, icol_dec); 
	    fflush (fp_debug);
        }

/*
    Convert ra_im, dec_im to param->layercsys[l] coordinate
*/
	istatus = parseCsysstr (param->overlay[l].coordSys, &isystbl, 
	    &epochtbl, param->errmsg);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returned parseCsysstr: istatus=[%d]\n", 
	        istatus);
	    fflush (fp_debug);
        }
    
        if (istatus == -1) {
/*
    Set to default value eq j2000 if parse failed.
*/
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "Error parseCsysstr [%s]\n", 
		    param->overlay[l].coordSys);
	        fflush (fp_debug);
            }

	    isystbl = 0;
	    epochtbl = 2000.;
	}
            
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "isystbl= [%d] epochtbl= [%lf]\n", 
		isystbl, epochtbl);
            fflush (fp_debug);
        }

/*
    Compute ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4 (i.e. four corners of
    param->shrunkImpath in layercsys[l] coordinate; this is used in inbox 
    routine.
*/
        ix[0] = 0;
        iy[0] = 0;
        ix[1] = 0;
        iy[1] = hdr.nl;
        ix[2] = hdr.ns;
        iy[2] = hdr.nl;
        ix[3] = hdr.ns;
        iy[3] = 0;

        for (i=0; i<4; i++) {
                
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "\ncall pix2sky\n");
                fflush (fp_debug);
            }
		
	    istatus = pix2sky (impath, ix[i], iy[i], hdr.nl, isysim, epochim, 
	        isystbl, epochtbl, &ra[i], &dec[i], param->errmsg); 

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "i= [%d] ra= [%lf] dec= [%lf]\n", 
		    i, ra[i], dec[i]);
                fflush (fp_debug);
            }
	}

        compute_nrml (npoly, ra, dec, &xnrml, &ynrml, &znrml);
            
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "returned compute_nrml\n");
            fflush (fp_debug);
        }

/*
    Convert picked coord (ra_im, dec_im) to param->overlay[l]->csys 
    coordinate (ra0, dec0) and xyz coord (x0, y0, z0)
*/
        if ((isysim != isystbl) || (epochim-epochtbl > 0.0001)) {
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "\ncall convertCoordinate\n");
                fflush (fp_debug);
	    }

            convertCoordinates (isysim, epochim, ra_im, dec_im, 
	        isystbl, epochtbl, &ra0, &dec0, 0.0); 
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "\nreturned convertCoordnates\n");
                fflush (fp_debug);
	    }
        }
	else {
            ra0 = ra_im;
	    dec0 = dec_im;
	}

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "ra0= [%lf] dec0= [%lf]\n", ra0, dec0);
            fflush (fp_debug);
        }
	
        cal_xyz (ra0, dec0, &x0, &y0, &z0);

/*
    Scan the table and compute the distance of each src to the picked image 
    point
*/
	istatus = 0;
	irow = 0;
	while (istatus >= 0) {

            istatus = tread();

            if (istatus < 0)
		break;

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "irow= [%d]\n", irow);
                fflush (fp_debug);
            }

            rastr[0] = '\0';
            decstr[0] = '\0';

	    if (tval (icol_ra) != (char *)NULL) {
		strcpy (rastr, strtrim(tval(icol_ra)));
            }

	    if (tval (icol_dec) != (char *)NULL) {
		strcpy (decstr, strtrim(tval(icol_dec)));
            }
                
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "rastr= [%s] decstr= [%s]\n", rastr, decstr);
                fflush (fp_debug);
            }

	    
/*
    Skip the record if rastr or decstr is not defined
*/
	    if (((int)strlen(rastr) == 0) || 
		((int)strlen(decstr) == 0)) {
                    
		irow++;
		continue;
	    }

	    istatus1 = str2Double (rastr, &raval, param->errmsg);
	    istatus2 = str2Double (decstr, &decval, param->errmsg);
	   
/*
    Skip the record if rastr or decstr cannot be converted to double
*/
	    if ((istatus1 < 0) || (istatus2 < 0)) {

		irow++;
		continue;
	    }
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "raval= [%lf] decval= [%lf]\n", 
		    raval, decval);
                fflush (fp_debug);
            }

/*
    Check if the point is in the subimage box, skip the record if it isn't.
*/
	    cal_xyz (raval, decval, &xval, &yval, &zval);

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "xval= [%lf] yval= [%lf] zval= [%lf]\n", 
		    xval, yval, zval);
                fflush (fp_debug);
            }

            istatus_inbox = inbox (xval, yval, zval, npoly, 
	        xnrml, ynrml, znrml);
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "returned inbox: istatus_inbox= [%d]\n",
		    istatus_inbox);
                fflush (fp_debug);
            }

            if (!istatus_inbox) {
		irow++;
		continue;
	    }
		

/*
    Now compute distance
*/
            istatus = computeSkyDist (ra0, dec0, raval, decval, &dist); 
                
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "returned computeSkyDist: istatus= [%d]\n", 
		    istatus);
                fprintf(fp_debug, "dist= [%lf] distmin= [%lf]\n", 
		    dist, distmin);
                fprintf(fp_debug, "cdelt[0]= [%lf]\n", hdr.cdelt[0]);
                fflush (fp_debug);
            }


	    if (dist < distmin) {
		    
		distmin = dist;
                ramin = raval;
		decmin = decval;
		strcpy (param->mintbl, param->overlay[l].dataFile);
		minrow = irow;
	    }

	    irow++;
	}

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "distmin= [%lf]\n", distmin);
            fprintf(fp_debug, "table processed: mintbl= [%s] minrow= [%d]\n", 
	        param->mintbl, minrow);
            fflush (fp_debug);
        }
    }

        
    if (param->mintbl[0] != '\0') {
/*
     The record number in IceTable display starts with 1
*/
//            param->minrecno = minrow+1;
        param->minrecno = minrow;
	    
	    
	param->minra = ramin;
	param->mindec = decmin;

/*
    Convert (ra, dec) to (ix, iy)
*/
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "call sky2pix\n");
            fprintf(fp_debug, "ramin= [%lf] decmin= [%lf]\n", ramin, decmin);
            fprintf(fp_debug, "isystbl= [%d] epochtbl= [%lf]\n", 
		isystbl, epochtbl);
            fprintf(fp_debug, "isysout= [%d] epochout= [%lf]\n", 
		isysout, epochout);
            fflush (fp_debug);
        }


        istatus = sky2pix (impath, ramin, decmin, isystbl, epochtbl, 
	    hdr.nl, isysim, epochim, &xmin, &ymin, &offscl, param->errmsg);

	param->minix = (int)(xmin + 0.5);
	param->miniy = (int)(ymin + 0.5);
	    
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "returned sky2pix\n");
            fprintf(fp_debug, "xmin= [%lf] ymin= [%lf]\n", xmin, ymin);
            fprintf(fp_debug, "minix= [%d] miniy= [%d]\n", 
		param->minix, param->miniy);
            fflush (fp_debug);
        }
    }
	    

/*
    Process iminfotbl to find all footprints that cover the picked point
*/
    niminfotbl = 0;
    for (l=0; l<param->noverlay; l++) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "l= [%d] type= [%s] filename= [%s]\n", 
		l, param->overlay[l].type, param->overlay[l].dataFile);
            fflush (fp_debug);
        }

        if (strcasecmp (param->overlay[l].type, "iminfo") != 0)
	    continue;
            
	niminfotbl++;
    }
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf(fp_debug, "niminfotbl= [%d]\n", niminfotbl);
        fflush (fp_debug);
    }

    if (niminfotbl == 0) {
        return (0);
    }

    param->niminfotbl = niminfotbl; 
    param->iminfoArr 
	= (struct IminfoRec **)malloc(niminfotbl*sizeof(struct IminfoRec *));


    niminfoarr = 0;
    for (l=0; l<param->noverlay; l++) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "l= [%d] type= [%s] filename= [%s]\n", 
		l, param->overlay[l].type, param->overlay[l].dataFile);
            fflush (fp_debug);
        }

        if (strcasecmp (param->overlay[l].type, "iminfo") != 0)
	    continue;

/*
    Find all image outline that covers the picked point
*/
        ncols = topen (fpath);
	    
	if (ncols <= 0) {
            continue;
        }
        
        icol_ra = -1;
        icol_dec = -1;
    	for (j=0; j<4; j++) {
	    icollon[j] = -1;
	    icollat[j] = -1;
        }

	for (i=0; i<ncols; i++) {
               
	    strcpy (colname, tinfo(i));

/*
    Find image col indices of center (ra, dec) and 
    four corners (ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4) 
    from tinfo array 
*/
	    if (strcasecmp (colname, "ra") == 0) {
		icol_ra = i;
	    }
	    if (strcasecmp (colname, "dec") == 0) {
		icol_dec = i;
	    }

	    for (j=0; j<4; j++) {
                    
		k = j+1;
                sprintf (str, "ra%d", k); 
		if (strcasecmp (colname, str) == 0) {
		    icollon[j] = i;
		}
                    
		sprintf (str, "dec%d", k); 
		if (strcasecmp (colname, str) == 0) {
		    icollat[j] = i;
		}
	    }
	}

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
	    fprintf (fp_debug, "icol_ra= [%d] icol_dec= [%d]\n", 
	        icol_ra, icol_dec); 
	    for (j=0; j<4; j++) {
	        fprintf (fp_debug, "j= [%d] icollon= [%d] icollat= [%d]\n", 
	            j, icollon[j], icollat[j]); 
	    }
	    fflush (fp_debug);
        }


/*
    parse layercsys and convert ra_im, dec_im to param->layercsys[l] coordinate
*/
	istatus = parseCsysstr (param->overlay[l].coordSys, &isystbl, 
	    &epochtbl, param->errmsg);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned parseCsysstr: istatus=[%d]\n", 
	        istatus);
	    fflush (fp_debug);
        }
    
        if (istatus == -1) {
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "Error parseCsysstr [%s]\n", 
		    param->overlay[l].coordSys);
	        fflush (fp_debug);
            }
                
	    continue; 
	}
            
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "isystbl= [%d] epochtbl= [%lf]\n", 
		isystbl, epochtbl);
            fflush (fp_debug);
        }

        if ((isysim != isystbl) || (epochim-epochtbl > 0.0001)) {
        
            convertCoordinates (isysim, epochim, ra_im, dec_im, 
	        isystbl, epochtbl, &ra0, &dec0, 0.0); 
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "\nconvertCoordnates\n");
                fflush (fp_debug);
	    }
        }
	else {
            ra0 = ra_im;
	    dec0 = dec_im;
	}

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "here9: ra0= [%lf] dec0= [%lf]\n", ra0, dec0);
            fflush (fp_debug);
        }
	
        cal_xyz (ra0, dec0, &x0, &y0, &z0);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "tblfile= [%s]\n", param->overlay[l].dataFile);
            fflush (fp_debug);
        }

/*
    Scan iminfo table, retrieve four corners and use inbox routine
    to compute whether the image covers the picked point (x0, y0, z0).
*/
	istatus = 0;
	irow = 0;
	pickrow_cnt = 0;
	while (istatus >= 0) {

            istatus = tread();

            if (istatus < 0)
		break;

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "irow= [%d]\n", irow);
                fflush (fp_debug);
            }

            rastr[0] = '\0';
            decstr[0] = '\0';

	    if (tval (icol_ra) != (char *)NULL) {
		strcpy (rastr, strtrim(tval(icol_ra)));
            }

	    if (tval (icol_dec) != (char *)NULL) {
		strcpy (decstr, strtrim(tval(icol_dec)));
            }
                
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "ftprt center: rastr=[%s] decstr=[%s]\n",
                    rastr, decstr);
                fflush (fp_debug);
            }
	    
	    if (((int)strlen(rastr) == 0) || 
		((int)strlen(decstr) == 0)) {
		    
		irow++;
		continue;
	    }
		
	    istatus1 = str2Double (rastr, &pick_cra[pickrow_cnt], 
	        param->errmsg);
	    istatus2 = str2Double (decstr, &pick_cdec[pickrow_cnt], 
		param->errmsg);
	            
	    if ((istatus1 < 0) || (istatus2 < 0)) {
	        irow++; 
	        continue;
            }        
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "pickrow_cnt= [%d]\n", pickrow_cnt);
                fprintf(fp_debug, "pick_cra= [%lf] pick_cdec= [%lf]\n", 
		    pick_cra[pickrow_cnt], pick_cdec[pickrow_cnt]);
                fflush (fp_debug);
            }

            baddata = 0;
            for (j=0; j<4; j++) { 

                rastr[0] = '\0';
                decstr[0] = '\0';
		
		if (tval (icollon[j]) != (char *)NULL) {
		    strcpy (rastr, strtrim(tval(icollon[j])));
                }

		if (tval (icollat[j]) != (char *)NULL) {
		    strcpy (decstr, strtrim(tval(icollat[j])));
                }
                
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf(fp_debug, "j= [%d] rastr= [%s] decstr= [%s]\n",
			j, rastr, decstr);
                    fflush (fp_debug);
                }
	    
	        if (((int)strlen(rastr) == 0) || 
		    ((int)strlen(decstr) == 0)) {
                    baddata = 1;        
		    break;
		}
		
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf(fp_debug, "here1\n");
                    fflush (fp_debug);
                }
	    
		istatus1 = str2Double (rastr, &ra[j], param->errmsg);
		istatus2 = str2Double (decstr, &dec[j], param->errmsg);
	            
		if ((istatus1 < 0) || (istatus2 < 0)) {
                    baddata = 1;        
		    break;
		}
            }    
		    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "baddata= [%d]\n", baddata);
                fflush (fp_debug);
            }
	    

	    if (baddata) {
	        irow++; 
	        continue;
	    }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "four corner retrieved\n");
                fflush (fp_debug);
            }

            compute_nrml (npoly, ra, dec, &xnrml, &ynrml, &znrml);
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "returned compute_nrml\n");
                fflush (fp_debug);
            }
	
	    istatus_reverse = 0;
	    istatus_inbox = 0;
	    istatus_inbox = inbox (x0, y0, z0, npoly, xnrml, ynrml, znrml);
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "returned inbox: istatus_inbox= [%d]\n", 
		    istatus_inbox);
                fflush (fp_debug);
            }

/*
    If inbox test failed, reverse the order of vertex and try again
*/
            if (!istatus_inbox) {

                for (i=0; i<npoly; i++) {
		    ra_reverse[i] = ra[npoly-i-1];
		    dec_reverse[i] = dec[npoly-i-1];
		}

		compute_nrml (npoly, ra_reverse, dec_reverse, 
		    &xnrml, &ynrml, &znrml);

		istatus_reverse 
		    = inbox (x0, y0, z0, npoly, xnrml, ynrml, znrml);
		
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf(fp_debug, "istatus_reverse= [%d]\n", istatus_inbox);
                    fflush (fp_debug);
                }
            }

		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "istatus_inbox= [%d] istatus_reverse= [%d]\n",
		    istatus_inbox, istatus_reverse);
                fflush (fp_debug);
            }

            if ((!istatus_inbox) && (!istatus_reverse)) {
		irow++;
		continue;
	    }

 	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf(fp_debug, "ipickrow= [%d] irow= [%d]\n", 
		    pickrow_cnt, irow);
                fflush (fp_debug);
            }

            pickrow_recno[pickrow_cnt] = irow;
                
	    for (j=0; j<4; j++) {
                
		if (istatus_inbox) {
		    istatus1 = sky2pix (impath, ra[j], dec[j], isystbl, 
		        epochtbl, hdr.nl, isysim, epochim, &x[j], &y[j], 
			&offscl, param->errmsg);

		    pickra[pickrow_cnt][j] = ra[j]; 
		    pickdec[pickrow_cnt][j] = dec[j]; 
		}
		else {
		    istatus1 = sky2pix (impath, ra_reverse[j], 
			dec_reverse[j], isystbl, epochtbl, hdr.nl, isysim, 
			epochim, &x[j], &y[j], &offscl, param->errmsg);

		    pickra[pickrow_cnt][j] = ra_reverse[j]; 
		    pickdec[pickrow_cnt][j] = dec_reverse[j]; 
		}
	            
		pickix[pickrow_cnt][j] = (int)(x[j] + 0.5);
	        pickiy[pickrow_cnt][j] = (int)(y[j] + 0.5);
            }
		
	    pickrow_cnt++;
	    irow++;
	}

            
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "pickrow_cnt= [%d]\n", pickrow_cnt);
            fprintf(fp_debug, "irow= [%d]\n", irow);
            fprintf(fp_debug, "tblfile= [%s]\n", param->overlay[l].dataFile);
            fflush (fp_debug);
        }

/*
    malloc iminfoArr 
*/
        if (pickrow_cnt > 0) {
        
	    param->iminfoArr[niminfoarr] 
	        = (struct IminfoRec *)malloc(sizeof(struct IminfoRec));
	    
	   param->iminfoArr[niminfoarr]->recno 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   
	   param->iminfoArr[niminfoarr]->cra 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->cdec 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   
	   param->iminfoArr[niminfoarr]->ra0 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->dec0 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->ra1 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->dec1 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->ra2 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->dec2 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->ra3 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
	   param->iminfoArr[niminfoarr]->dec3 
	       = (double *)malloc(pickrow_cnt*sizeof(double));     
		
	   param->iminfoArr[niminfoarr]->ix0 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->iy0 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->ix1 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->iy1 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->ix2 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->iy2 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->ix3 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   param->iminfoArr[niminfoarr]->iy3 
	       = (int *)malloc(pickrow_cnt*sizeof(int));     
	   
	   
	    strcpy (param->iminfoArr[niminfoarr]->filename, 
	        param->overlay[l].dataFile);

	    param->iminfoArr[niminfoarr]->nrec = pickrow_cnt;
            
	    for (i=0; i<pickrow_cnt; i++) {

                param->iminfoArr[niminfoarr]->recno[i] = pickrow_recno[i];

                param->iminfoArr[niminfoarr]->cra[i] = pick_cra[i];
                param->iminfoArr[niminfoarr]->cdec[i] = pick_cdec[i];
		        
		param->iminfoArr[niminfoarr]->ra0[i] = pickra[i][0];
		param->iminfoArr[niminfoarr]->dec0[i] = pickdec[i][0];
		param->iminfoArr[niminfoarr]->ra1[i] = pickra[i][1];
		param->iminfoArr[niminfoarr]->dec1[i] = pickdec[i][1];
		param->iminfoArr[niminfoarr]->ra2[i] = pickra[i][2];
		param->iminfoArr[niminfoarr]->dec2[i] = pickdec[i][2];
		param->iminfoArr[niminfoarr]->ra3[i] = pickra[i][3];
		param->iminfoArr[niminfoarr]->dec3[i] = pickdec[i][3];

		param->iminfoArr[niminfoarr]->ix0[i] = pickix[i][0];
		param->iminfoArr[niminfoarr]->iy0[i] = pickiy[i][0];
		param->iminfoArr[niminfoarr]->ix1[i] = pickix[i][1];
		param->iminfoArr[niminfoarr]->iy1[i] = pickiy[i][1];
		param->iminfoArr[niminfoarr]->ix2[i] = pickix[i][2];
		param->iminfoArr[niminfoarr]->iy2[i] = pickiy[i][2];
		param->iminfoArr[niminfoarr]->ix3[i] = pickix[i][3];
	        param->iminfoArr[niminfoarr]->iy3[i] = pickiy[i][3];
            } 
	        
	    niminfoarr++;
        }
    }
    param->niminfoarr = niminfoarr;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf(fp_debug, "niminfoarr= [%d]\n", param->niminfoarr);
            fflush (fp_debug);
    }

    return (0);
}

