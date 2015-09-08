/*
Theme: This routine extract N image planes from an Image Fits cube
    and generates the averaged plane:
    
    the cube data has been arranged to be
    
    bitpix = -64,
    naxis1 = ra,
    naxis2 = dec,
    naxis3 = nplane.

Input:  

Image cube fits file,
center plane number,
number of planes

Output:

2D averaged image fits file.

Written: December 7, 2013 (Mihseh Kong)

Modified to add plane averaging: 
October 20, 2014 (Mihseh Kong)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/file.h>

#include <fitsio.h>
#include "fitshdr.h"


int  str2Integer (char *str, int *intval, char *errmsg);
int  str2Double (char *str, double *dblval, char *errmsg);

extern FILE *fp_debug;


int extractAvePlane (char *cubepath, char *impath, int iplane, int nplaneave,
    int *splaneave, int *eplaneave, char *errmsg)
{
    struct stat     buf;
    struct FitsHdr  hdr;

    char   str[1024];
    char   cmd[1024];
    
    int    bitpix;
    int    istatus;
    
    int    nhdu, hdutype, hdunum;

    int    nelements;
   
    int    naxis3;
    int    l;
    int    j;
    int    i;
    int    jj;
    int    nullcnt;

    int    splane;
    int    eplane;

    long   fpixel[4];
    long   fpixelo[4];

    double *fitsbuf;
    double *imbuff;
    
    fitsfile  *infptr;
    fitsfile  *outfptr;

    
    int    debugfile = 1;


/*
     Make a NaN value to use setting blank pixels
 */

    union
    {
        double d;
        char   c[8];
    }
    value;

    double nan;

    for(i=0; i<8; ++i)
        value.c[i] = 255;

    nan = value.d;


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "\nEnter extractAvePlane: cubepath= [%s]\n", 
	    cubepath);
	fprintf (fp_debug, "iplane= [%d]\n", iplane);
	fprintf (fp_debug, "nplaneave= [%d]\n", nplaneave);
	fprintf (fp_debug, "impath= [%s]\n", impath);
        fflush (fp_debug);
    }

    splane = iplane - nplaneave/2;
    eplane = splane + nplaneave - 1;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "splane= [%d] eplane= [%d]\n", splane, eplane);
        fflush (fp_debug);
    }

    istatus = 0;
    if (fits_open_file (&infptr, cubepath, READONLY, &istatus)) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "istatus= [%d]\n", istatus);
            fflush (fp_debug);
	}

	sprintf (errmsg, "Failed to open FITS file [%s]\n", cubepath);
	return (-1);
    } 
   
    hdunum = 1; 
    nhdu = 0;
    istatus = 0;
    istatus = fits_get_num_hdus (infptr, &nhdu, &istatus);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, 
	    "returned fits_get_hdu_num: istatus= [%d] nhdu= [%d]\n",
	    istatus, nhdu);
        fflush (fp_debug);
    }

    if (hdunum > nhdu) {

        sprintf (errmsg, "fname [%s] doesn't contain any HDU", cubepath);
	return (-1);
    }


/*
    Read fits keywords from the first HDU
*/
    hdutype = 0;
    istatus = 0;
    istatus = fits_movabs_hdu (infptr, hdunum, &hdutype, &istatus);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, 
	    "returned fits_movabs_hdu: istatus= [%d] hdutype= [%d]\n",
	    istatus, hdutype);
        fflush (fp_debug);
    }

/*
    Read fits keywords
*/
    istatus = 0;
    istatus = fits_read_key (infptr, TSTRING, "simple", str, (char *)NULL, 
        &istatus);
    
    if (istatus == KEY_NO_EXIST) {
        sprintf (errmsg, "keyword SIMPLE not found in fits header");
        return (-1);
    }
   
    if ((strcmp (str, "T") != 0) && (strcmp (str, "F") != 0)) {
        sprintf (errmsg, "keyword SIMPLE must be T or F");
        return (-1);
    }

    istatus = 0;
    istatus = fits_read_key (infptr, TSTRING, "bitpix", str, (char *)NULL, 
        &istatus);
    
    if (istatus == KEY_NO_EXIST) {
        sprintf (errmsg, "keyword BITPIX not found in fits header");
        return (-1);
    }
  
    istatus = str2Integer (str, &bitpix, errmsg);
    if (istatus != 0) {
        sprintf (errmsg, "keyword BITPIX must be an integer");
        return (-1);
    }

    if ((bitpix != 8) &&
        (bitpix != 16) &&
        (bitpix != 32) &&
        (bitpix != 64) &&
        (bitpix != -32) &&
        (bitpix != -64)) {
        
	sprintf (errmsg, 
	    "keyword BITPIX value must be 8, 16, 32, 64, -32, -64");
        return (-1);
    }

    hdr.bitpix = bitpix;

    istatus = 0;
    istatus = fits_read_key (infptr, TSTRING, "naxis", str, (char *)NULL, 
        &istatus);
        
    if (istatus == KEY_NO_EXIST) {
        sprintf (errmsg, "keyword naxis not found in fits header");
        return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "str= [%s]\n", str);
        fflush (fp_debug);
    }
    istatus = str2Integer (str, &hdr.naxis, errmsg);
    if (istatus < 0) {
        sprintf (errmsg, "Failed to convert naxis to integer");
        return (-1);
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "naxis= [%d]\n", hdr.naxis);
        fflush (fp_debug);
    }


    istatus = 0;
    istatus = fits_read_key (infptr, TSTRING, "naxis1", str, (char *)NULL, 
        &istatus);
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "returned fits_read_key: istatus= [%d]\n", istatus);
        fflush (fp_debug);
    }
        
    if (istatus == KEY_NO_EXIST) {
        sprintf (errmsg, "keyword naxis1 not found in fits header");
        return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "str= [%s]\n", str);
        fflush (fp_debug);
    }

    istatus = str2Integer (str, &hdr.ns, errmsg);
    
    if (istatus < 0) {
        sprintf (errmsg, "Failed to convert naxis1 string to integer");
        return (-1);
    }
    hdr.naxes[0] = hdr.ns;

    istatus = 0;
    istatus = fits_read_key (infptr, TSTRING, "naxis2", str, 
        (char *)NULL, &istatus);
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "returned fits_read_key: istatus= [%d]\n", istatus);
        fflush (fp_debug);
    }
        
    if (istatus == KEY_NO_EXIST) {
        sprintf (errmsg, "keyword naxis2 not found in fits header");
        return (-1);
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "str= [%s]\n", str);
        fflush (fp_debug);
    }

    istatus = str2Integer (str, &hdr.nl, errmsg);
    
    if (istatus < 0) {
        sprintf (errmsg, "Failed to convert naxis2 string to integer");
        return (-1);
    }
    hdr.naxes[1] = hdr.nl;

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "ns= [%d] nl= [%d]\n", hdr.ns, hdr.nl);
        fflush (fp_debug);
    }

    hdr.nplane = 1;

    if (hdr.naxis > 2) {
    
        istatus = 0;
        istatus = fits_read_key (infptr, TSTRING, "naxis3", str, 
            (char *)NULL, &istatus);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

            fprintf (fp_debug, "returned fits_read_key: istatus= [%d]\n", 
	        istatus);
            fflush (fp_debug);
        }
        
        if (istatus == KEY_NO_EXIST) {
            sprintf (errmsg, "keyword naxis3 not found in fits header");
            return (-1);
        }
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

            fprintf (fp_debug, "str= [%s]\n", str);
            fflush (fp_debug);
        }

        istatus = str2Integer (str, &hdr.naxes[2], errmsg);
    
        if (istatus < 0) {
            sprintf (errmsg, "Failed to convert naxis3 string to integer");
            return (-1);
        }
        hdr.nplane = hdr.naxes[2];
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

            fprintf (fp_debug, "naxes[2]= [%d]\n", hdr.naxes[2]);
            fflush (fp_debug);
        }
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "nplane= [%d]\n", hdr.nplane);
        fflush (fp_debug);
    }


    istatus = stat (impath, &buf);
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "impath exists? : istatus= [%d]\n", istatus);
        fflush (fp_debug); 
    }

    if (istatus >= 0) {
        sprintf (cmd, "unlink %s", impath);
	istatus = system (cmd);
    }	

/*
    Create output fits file
*/
    istatus = 0;
    if (fits_create_file (&outfptr, impath, &istatus)) {
	    
        sprintf (errmsg, "Failed to create output fitsfile [%s]\n", impath);
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "err: [%s]\n", errmsg);
	    fflush (fp_debug);
        }
	return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "outptr created\n");
        fflush (fp_debug);
    }


/* 
    Copy input fits header to output fitsfile
*/
    istatus = 0;
    if (fits_copy_header (infptr, outfptr, &istatus)) {

        strcpy (errmsg, "Failed to copy fitshdr\n");
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "err: [%s]\n", errmsg);
	    fflush (fp_debug);
        }
	return (-1);
    }

/*
    Update header keyword NAXIS3
*/
    naxis3 = 1;
    istatus = 0;
    if (fits_update_key_lng(outfptr, "NAXIS3", naxis3, (char *)NULL, 
        &istatus)) {
	
        strcpy (errmsg, "Failed to update keyword NAXIS3\n");
	return (-1);
    }

    istatus = 0;
    if (fits_close_file (infptr, &istatus)) {
        sprintf (errmsg, "Failed to close cubepath [%s]\n", cubepath);
        return (-1); 
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
	fprintf (fp_debug, "cubepath= [%s]\n", cubepath);
        fflush (fp_debug);
    }

    istatus = 0;
    if (fits_open_file (&infptr, cubepath, READONLY, &istatus)) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "istatus= [%d]\n", istatus);
            fflush (fp_debug);
	}

	sprintf (errmsg, "Failed to open FITS file [%s]\n", cubepath);
	return (-1);
    } 
  
/*
    Create imbuff for average image
*/
    imbuff = (double *)malloc (hdr.ns*hdr.nl*sizeof(double));
    for (i=0; i<hdr.nl*hdr.ns; i++) {
        imbuff[i] = 0.;
    }


/*
    Read data from nth plane and write to output fitsfile
*/
    nelements = hdr.ns;
       
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "iplane= [%d]\n", iplane);
        fflush (fp_debug);
    }

    fitsbuf  = (double *)malloc(hdr.ns*sizeof(double));

    if (splane < 1)
        splane = 1;
    
    if (eplane > hdr.nplane)
        eplane = hdr.nplane;

    for (l=splane; l<=eplane; l++) { 
    
        fpixel[0] = 1;
        fpixel[1] = 1;
        fpixel[2] = l;
        fpixel[3] = 1;

        for (j=0; j<hdr.nl; j++) {

	    if (j == 0) {
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
	            fprintf (fp_debug, "l= [%d] fpixel[2]= [%ld]\n", 
		        l, fpixel[2]);
                    fflush (fp_debug);
	        }
            }

	    if (fits_read_pix (infptr, TDOUBLE, fpixel, nelements, &nan,
                fitsbuf, &nullcnt, &istatus)) {
	        break;
	    }
            
	    if (j == 10) {
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    for (i=0; i<hdr.ns; i++) {
	                fprintf (fp_debug, "j= [%d] i= [%d] fitsbuf= [%lf]\n",
	                    j, i, fitsbuf[i]);
	                fprintf (fp_debug, "fitsbuf/nplaneave= [%lf]\n",
	                    fitsbuf[i]/nplaneave);
		    }
                    fflush (fp_debug);
	        }
            }         

/*
    Copy data to imbuff for plane averaging
*/
            jj = hdr.ns*j;
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "jj= [%d]\n", jj);
                fflush (fp_debug);
	    }
	        
            for (i=0; i<hdr.ns; i++) {
		imbuff[jj+i] += fitsbuf[i]/nplaneave; 
            }         
 
            if (j == 10) {
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
                    for (i=0; i<hdr.ns; i++) {
	                fprintf (fp_debug, "j= [%d] i= [%d] imbuff= [%lf]\n",
	                    j, i, imbuff[jj+i]);
		    }
                    fflush (fp_debug);
	        }
            }         
            fpixel[1]++;
        }
    }

/*
    Write averaged image to output FITS file
*/
    fpixelo[0] = 1;
    fpixelo[1] = 1;
    fpixelo[2] = 1;
    fpixelo[3] = 1;
    
    for (j=0; j<hdr.nl; j++) {

        jj = hdr.ns*j;
        for (i=0; i<hdr.ns; i++) {
	    fitsbuf[i] = imbuff[jj+i]; 
        }
            
	if (j == 10) {
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
                for (i=0; i<hdr.ns; i++) {
	            fprintf (fp_debug, "j= [%d] i= [%d] fitsbuf= [%lf]\n", 
		        j, i, fitsbuf[i]);
		}
                fflush (fp_debug);
	    }
        }         

        
	if (fits_write_pix (outfptr, TDOUBLE, fpixelo, nelements,
			 (void *)fitsbuf, &istatus)) {

            sprintf (errmsg, "fits write error: l= [%d]\n", l);
            return (-1);
        }

        fpixelo[1]++;
    }   


    istatus = 0;
    if (fits_close_file (infptr, &istatus)) {
        sprintf (errmsg, "Failed to close cubepath [%s]\n", cubepath);
        return (-1); 
    }
    
    istatus = 0;
    if (fits_close_file (outfptr, &istatus)) {
        sprintf (errmsg, "Failed to close impath [%s]\n", impath);
        return (-1); 
    }

    *splaneave = splane;
    *eplaneave = eplane;

    return (0);
}

