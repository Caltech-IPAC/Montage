/*
Theme: This routine reads the whold FITS image cube file to memory, 
    extract wavelength axis data to an array, sort, and find the 
    meadian value for each pixel to produce a median image.
   
Input:  

    fits cube file  

    naxis1: ns,
    naxis2: nl,
    naxis3: nplane, and
    bitpix: -64

Output:

    an median image.

Date written: October 23, 2014 (Mihseh Kong)
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

void  sort (double *arr, int n);

extern FILE *fp_debug;


int generateMedianPlane (char *cubepath, char *impath, int iplane,
    int nplane_in, int *startplane, int *endplane, char *errmsg)
{
    struct stat     buf;
    struct FitsHdr  hdr;

    char   str[1024];
    char   cmd[1024];
    
    int    bitpix;
    int    istatus;
    
    int    nhdu, hdutype, hdunum;

    
    long    nelements;
    
    int    naxis3;
    int    l;
    int    i;
    int    j;
    int    jj;

    int    nullcnt;
    
    int    splane;
    int    eplane;
    int    nplane;
    
    
    int    indx;
    int    indx1;
    int    indx2;

    int    npixel;
    int    midpoint; 

    long   fpixel[4];
    long   fpixelo[4];

    double *fitscubebuf;
    double *outbuf;
    double *fitsbuf1d;
    double *wavebuf;


    fitsfile  *infptr;
    fitsfile  *outfptr;

    
    int    debugfile = 1;


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "\nEnter generateMedianPlane: cubepath= [%s]\n", 
	    cubepath);
	fprintf (fp_debug, "impath= [%s]\n", impath);
	fprintf (fp_debug, "iplane= [%d]\n", iplane);
	fprintf (fp_debug, "nplane_in= [%d]\n", nplane_in);
        fflush (fp_debug);
    }

    nplane = nplane_in;

    splane = iplane - nplane/2;
    eplane = splane + nplane - 1;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "splane= [%d] eplane= [%d]\n", splane, eplane);
        fflush (fp_debug);
    }


/*
    Open input fits cube 
*/
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
	
        fprintf (fp_debug, "hdr.nplane= [%d]\n", hdr.nplane);
        fflush (fp_debug);
    }


    if (splane < 1)
        splane = 1;
    
    if (eplane > hdr.nplane)
        eplane = hdr.nplane;

    nplane = eplane - splane + 1;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "hdr.nplane= [%d]\n", hdr.nplane);
	fprintf (fp_debug, "splane= [%d] eplane= [%d]\n", splane, eplane);
	fprintf (fp_debug, "nplane= [%d]\n", nplane);
        fflush (fp_debug);
    }


/*
    malloc arrays for reading data
*/
    fitscubebuf  
    = (double *)malloc(hdr.ns*hdr.nl*nplane*sizeof(double));
    
    outbuf  = (double *)malloc(hdr.ns*hdr.nl*sizeof(double));
    
    wavebuf  = (double *)malloc(nplane*sizeof(double));
    
    fitsbuf1d  = (double *)malloc(hdr.ns*sizeof(double));

    nelements = hdr.ns;

    npixel = hdr.ns*hdr.nl;
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "npixel= [%d]\n", npixel);
        fflush (fp_debug);
    }


/*
    Read cube data to fitscubebuf
*/
    fpixel[0] = 1;
    fpixel[3] = 1;
    
    indx = 0;
    for (l=splane; l<=eplane; l++)
    {
       
	indx2 = hdr.nl*hdr.ns*(l-splane); 

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "l= [%d]\n", l);
            fflush (fp_debug);
	}


        fpixel[1] = 1;
        fpixel[2] = l;
        
	for (j=0; j<hdr.nl; j++)
        {
            indx1 = hdr.ns*j; 

            if (fits_read_pix (infptr, TDOUBLE, fpixel, nelements, NULL,
                fitsbuf1d, &nullcnt, &istatus)) {
	        break;
	    }

            for (i=0; i<nelements; i++) {
                
		fitscubebuf[indx2+indx1+i] = fitsbuf1d[i];
                
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            
		    if ((i == 30) && (j == 25)) {
		        fprintf (fp_debug, "i=[%d] j=[%d] l=[%d] pixel=[%lf]\n",
			    i, j, l, fitscubebuf[indx2+indx1+i]);
                        fflush (fp_debug);
	            }
		}


	    }

	    fpixel[1]++;
	}
   }

/*
    istatus = 0;
    if (fits_close_file (infptr, &istatus)) {
        sprintf (errmsg, "Failed to close cubepath [%s]\n", cubepath);
        return (-1); 
    }
*/


/*
    Extract wave axis and compute median values
*/
    midpoint = nplane / 2;
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "midpint= [%d]\n", midpoint);
        fflush (fp_debug);
    }

    for (j=0; j<hdr.nl; j++) {
        
	indx1 = hdr.ns*j;

        for (i=0; i<hdr.ns; i++) {
        
            for (l=0; l<nplane; l++) {
	        
		indx = npixel*l + indx1 + i;

		wavebuf[l] = fitscubebuf[indx];
		    
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            
		    if ((i == 30) && (j == 25)) {
		        fprintf (fp_debug, "i=[%d] j=[%d] l=[%d] pixel=[%lf]\n",
			    i, j, l, wavebuf[l]);
                        fflush (fp_debug);
	            }
		}
	    }

	    sort (wavebuf, nplane);
	        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            
		
		if ((i == 30) && (j == 25)) {
		    
		    fprintf (fp_debug, "after qsort\n");
                    for (l=0; l<nplane; l++) {
		        fprintf (fp_debug, "l=[%d] pixel=[%lf]\n", 
			    l, wavebuf[l]);
		    }
                    fflush (fp_debug);
	        }
	    }

            outbuf[indx1+i] = wavebuf[midpoint];
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
		if ((i == 30) && (j == 25)) {
	            fprintf (fp_debug, "outbuf= [%lf]\n", outbuf[indx1+i]);
                    fflush (fp_debug);
	        }
	    }

	}
    }



/*
    Create output fits file
*/
    istatus = stat (impath, &buf);
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "impath exists? : istatus= [%d]\n", istatus);
        fflush (fp_debug); 
    }

    if (istatus >= 0) {
        sprintf (cmd, "unlink %s", impath);
	istatus = system (cmd);
    }	


    istatus = 0;
    if (fits_create_file (&outfptr, impath, &istatus)) {
	    
        sprintf (errmsg, "Failed to create output fitsfile [%s]\n", 
	    impath);
        
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

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "header copied\n");
        fflush (fp_debug);
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

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "naxis3 updated\n");
        fflush (fp_debug);
    }


/*
    Write to output fitsfile
*/
    nelements = hdr.ns;
    fpixelo[0] = 1;
    fpixelo[1] = 1;
    fpixelo[2] = 1;
    fpixelo[3] = 1;
       
    for (j=0; j<hdr.nl; j++) {
    
        jj = hdr.ns*j;
        for (i=0; i<hdr.ns; i++) {
                
	    fitsbuf1d[i] = outbuf[jj+i];
	}

        if (fits_write_pix (outfptr, TDOUBLE, fpixelo, nelements,
	     (void *)fitsbuf1d, &istatus)) {

            sprintf (errmsg, "fits write error: l= [%d] j= [%d]\n", l, j);
            return (-1);
        }

        fpixelo[1]++;
    }

        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here2\n");
        fflush (fp_debug);
    }


    istatus = 0;
    if (fits_close_file (infptr, &istatus)) {
        sprintf (errmsg, "Failed to close cubepath [%s]\n", cubepath);
        return (-1); 
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here3\n");
        fflush (fp_debug);
    }


    istatus = 0;
    if (fits_close_file (outfptr, &istatus)) {
        sprintf (errmsg, "Failed to close impath [%s]\n", impath);
        return (-1); 
    }

    *startplane = splane;
    *endplane = eplane;


    return (0);
}

