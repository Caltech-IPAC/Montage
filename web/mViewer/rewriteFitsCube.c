/*
Theme: This routine reads a FITS image cube file to re-arrange the axis
    and modify the datatype so that the output fitscube is:
    
    naxis1: ns,
    naxis2: nl,
    naxis3: nplane, and
    bitpix: -64

Input:  

    fits cube file

Output:

    fits cube filename

Date written: August 1, 2014 (Mihseh Kong)
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
#include "mviewer.h"


int  getFitshdr (char *fname, struct FitsHdr *hdr, int iscube);

int  str2Integer (char *str, int *intval, char *errmsg);
int  str2Double (char *str, double *dblval, char *errmsg);

extern FILE *fp_debug;


int rewriteFitsCube (char *cubepath, char *outcubepath, char *errmsg)
{
    struct stat     buf;
    struct FitsHdr  hdr;

    char   key[40];
    char   cmd[1024];
    
    int    istatus;

    long    nelements;
   
    int    l;
    int    i;
    int    j;
    int    nullcnt;
    int    indx;
    int    indx1;
    int    indx2;

    int    ind0;
    int    ind1;

    int    npixel;
   
    long   fpixel[4];
    long   fpixelo[4];

    double *fitscubebuf;
    double *fitsbuf1d;
    double *outbuf;

    fitsfile  *infptr;
    fitsfile  *outfptr;

    
    int    debugfile = 0;


    if (debugfile) {
	fprintf (fp_debug, "\nEnter rewriteFitsCube\n");
	fprintf (fp_debug, "cubepath= [%s]\n", cubepath);
        fflush (fp_debug);
    }
   
/*
    getFitshdr to analyze if the cube data needs to be re-arranged
*/
    if (debugfile) {
        fprintf (fp_debug, "call getFitshdr\n");
        fprintf (fp_debug, "cubepath= [%s]\n", cubepath);
        fflush (fp_debug);
    }
           
    istatus = getFitshdr (cubepath, &hdr, 1);

    if (debugfile) {
        fprintf (fp_debug, "returned getFitshdr, istatus= [%d]\n", istatus);
        fprintf (fp_debug, "naxes[0]= [%d] axisIndx[0]= [%d]\n", 
            hdr.naxes[0], hdr.axisIndx[0]);
        fprintf (fp_debug, "naxes[1]= [%d] axisIndx[1]= [%d]\n", 
            hdr.naxes[1], hdr.axisIndx[1]);
        fprintf (fp_debug, "naxes[2]= [%d] axisIndx[2]= [%d]\n", 
            hdr.naxes[2], hdr.axisIndx[2]);
        fflush (fp_debug);
    }


/*
    Open Original fits cube 
*/
    istatus = 0;
    if (fits_open_file (&infptr, cubepath, READONLY, &istatus)) {

        if (debugfile) {
	    fprintf (fp_debug, "istatus= [%d]\n", istatus);
            fflush (fp_debug);
	}

	sprintf (hdr.errmsg, "Failed to open FITS file [%s]\n", cubepath);
	return (-1);
    } 


/*
    malloc arrays for reading data
*/
    fitscubebuf  
    = (double *)malloc(hdr.naxes[0]*hdr.naxes[1]*hdr.naxes[2]*sizeof(double));
    
    fitsbuf1d  = (double *)malloc(hdr.naxes[0]*sizeof(double));

/*
    Read data in the original order but put it in the re-arranged location
    in the fitscubebuf
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "Read data in the original order\n");
	for (i=0; i<hdr.naxis; i++) {
	    fprintf (fp_debug, "naxes[%d]= [%d]\n", i, hdr.naxes[i]);
	}
	fflush (fp_debug);
    }


    nelements = hdr.naxes[0];

    fpixel[0] = 1;
    fpixel[1] = 1;
    fpixel[2] = 1;
    fpixel[3] = 1;
       
    npixel = hdr.naxes[0]*hdr.naxes[1];
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "npixel= [%d]\n", npixel);
        fflush (fp_debug);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "hdr.axisIndx[0]= [%d]\n", hdr.axisIndx[0]);
	fprintf (fp_debug, "hdr.axisIndx[1]= [%d]\n", hdr.axisIndx[1]);
	fprintf (fp_debug, "hdr.axisIndx[2]= [%d]\n", hdr.axisIndx[2]);
	fflush (fp_debug);
    }

    indx = 0;
    for (l=0; l<hdr.naxes[2]; l++)
    {
	if (hdr.axisIndx[0] == 2) {
	    indx2 = l;
	}
	else if (hdr.axisIndx[1] == 2) {
            indx2 = hdr.ns*l; 
	}
	else if (hdr.axisIndx[2] == 2) {
	    indx2 = hdr.nl*hdr.ns*l; 
        }


	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "l= [%d]\n", l);
            fflush (fp_debug);
	}


        fpixel[1] = 1;
        for (j=0; j<hdr.naxes[1]; j++)
        {
            if (hdr.axisIndx[0] == 1) {
	        indx1 = j;
	    }
	    else if (hdr.axisIndx[1] == 1) {
                indx1 = hdr.ns*j; 
	    }
	    else if (hdr.axisIndx[2] == 1) {
	        indx1 = hdr.nl*hdr.ns*j; 
            }

            if (fits_read_pix (infptr, TDOUBLE, fpixel, nelements, NULL,
                fitsbuf1d, &nullcnt, &istatus)) {
	        break;
	    }

/*	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
		fprintf (fp_debug, "l= [%d] j= [%d]\n", l, j);
            
	        for (i=0; i<nelements; i++) {
	            if (fabs(fitsbuf1d[i]) > 1.0)         
		        fprintf (fp_debug, "i= [%d] fitsbuf1d= [%f]\n", 
	                    i, fitsbuf1d[i]);
	        }
                fflush (fp_debug);
	    }
*/


/*
    copy data to fitscubebuf: l*(nplane*ns) + j*nplane + i
*/
	    for (i=0; i<nelements; i++) {
            
	        if (hdr.axisIndx[0] == 0) {
		    indx = indx1 + indx2 + i; 
	        }
	        else if (hdr.axisIndx[1] == 0) {
                    indx = indx1 + indx2 + hdr.ns*i; 
	        }
	        else if (hdr.axisIndx[2] == 0) {
	            indx = indx1 + indx2 + hdr.nl*hdr.ns*i; 
                }

		fitscubebuf[indx] = (double)fitsbuf1d[i];
		   
/*
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                   
		    if (i == 100) {
		       
			if (fitscubebuf[indx] >0.001) {
		            fprintf (fp_debug, "j= [%d] l= [%d]\n", j, l); 
		            fprintf (fp_debug, "fitscubebuf= [%lf]\n", 
		                fitscubebuf[indx]);
                            fflush (fp_debug);
	                }	
		    }
                }
*/


	    }
	
	    fpixel[1]++;
	}
	fpixel[2]++;
   }
        

/*
    Create output fits file
*/
    istatus = stat (outcubepath, &buf);
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "output cubepath exists? : istatus= [%d]\n", 
	    istatus);
        fflush (fp_debug); 
    }

    if (istatus >= 0) {
        sprintf (cmd, "unlink %s", outcubepath);
	istatus = system (cmd);
    }	


    istatus = 0;
    if (fits_create_file (&outfptr, outcubepath, &istatus)) {
	    
        sprintf (errmsg, "Failed to create output fitsfile [%s]\n", 
	    outcubepath);
        
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
    Update header keyword bitpix and NAXIS
*/
    if (hdr.bitpix != -64) {
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "update bitpix to -64 (double)\n");
	    fflush (fp_debug);
        }

        l = -64;
        strcpy (key, "BITPIX");
        istatus = 0;
        if (fits_update_key (outfptr, TINT, key , &l, (char *)NULL, &istatus)) 
        {
            strcpy (errmsg, "Failed to update keyword BITPIX\n");
	    return (-1);
        }

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "keyword BITPIX value updated to -64\n");
	    fflush (fp_debug);
        }     
    }

    ind0 = hdr.axisIndx[0];
    ind1 = hdr.axisIndx[1];

    for (l=0; l<hdr.naxis; l++) {
     
        indx = hdr.axisIndx[l];

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "l= [%d indx= [%d]\n", l, indx);
            fprintf (fp_debug, "naxis= [%d]\n", hdr.naxes[indx]);
	    fflush (fp_debug);
        }

        if (l != indx) {
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "here1\n");
	        fflush (fp_debug);
            }

	    sprintf (key, "naxis%d", l+1);
         
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "l= [%d key= [%s]\n", l, key);
                fprintf (fp_debug, "naxis= [%d]\n", hdr.naxes[indx]);
	        fflush (fp_debug);
            }

            istatus = 0;
        
	    if (fits_update_key (outfptr, TINT, key, &hdr.naxes[indx], 
	        (char *)NULL, &istatus)) 
	    {

                sprintf (errmsg, "Failed to update keyword %s\n", key);
	        return (-1);
            }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "keyword %s value updated to %d\n", 
	            key, hdr.naxes[indx]);
	        fflush (fp_debug);
            }

	    
	    sprintf (key, "cunit%d", l+1);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "l= [%d key= [%s]\n", l, key);
                fprintf (fp_debug, "cunit= [%s]\n", hdr.cunit[indx]);
	        fflush (fp_debug);
            }

            istatus = 0;
	    if (fits_update_key (outfptr, TSTRING, key, hdr.cunit[indx], 
	        (char *)NULL, &istatus)) {

                sprintf (errmsg, "Failed to update keyword %s\n", key);
	        return (-1);
            }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "keyword %s value updated to %s\n", 
	            key, hdr.cunit[indx]);
	        fflush (fp_debug);
            }

	
	    sprintf (key, "ctype%d", l+1);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "l= [%d key= [%s]\n", l, key);
                fprintf (fp_debug, "ctype= [%s]\n", hdr.ctype[indx]);
	        fflush (fp_debug);
            }

            istatus = 0;
	    if (fits_update_key (outfptr, TSTRING, key, hdr.ctype[indx], 
	        (char *)NULL, &istatus)) {

                sprintf (errmsg, "Failed to update keyword %s\n", key);
	        return (-1);
            }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "keyword %s value updated to %s\n", 
	            key, hdr.ctype[indx]);
	        fflush (fp_debug);
            }

	
	    sprintf (key, "crpix%d", l+1);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "l= [%d key= [%s]\n", l, key);
                fprintf (fp_debug, "crpix= [%lf]\n", hdr.crpix[indx]);
	        fflush (fp_debug);
            }

            istatus = 0;
	    if (fits_update_key (outfptr, TDOUBLE, key, &hdr.crpix[indx], 
	        (char *)NULL, &istatus)) {

                sprintf (errmsg, "Failed to update keyword %s\n", key);
	        return (-1);
            }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "keyword %s value updated to %lf\n", 
	            key, hdr.crpix[indx]);
	        fflush (fp_debug);
            }

	    sprintf (key, "crval%d", l+1);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "l= [%d key= [%s]\n", l, key);
                fprintf (fp_debug, "crval= [%lf]\n", hdr.crval[indx]);
	        fflush (fp_debug);
            }

            istatus = 0;
	    if (fits_update_key (outfptr, TDOUBLE, key, &hdr.crval[indx], 
	        (char *)NULL, &istatus)) {

                sprintf (errmsg, "Failed to update keyword %s\n", key);
	        return (-1);
            }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "keyword %s value updated to %lf\n", 
	            key, hdr.crval[indx]);
	        fflush (fp_debug);
            }

            if (hdr.haveCdelt[hdr.axisIndx[l]]) {
	
	        sprintf (key, "cdelt%d", l+1);
        
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "l= [%d key= [%s]\n", l, key);
                    fprintf (fp_debug, "cdelt= [%lf]\n", hdr.cdelt[indx]);
	            fflush (fp_debug);
                }

                istatus = 0;
	        if (fits_update_key (outfptr, TDOUBLE, key, &hdr.cdelt[indx], 
	            (char *)NULL, &istatus)) {

                    sprintf (errmsg, "Failed to update keyword %s\n", key);
	            return (-1);
                }

	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "keyword %s value updated to %lf\n", 
	                key, hdr.cdelt[indx]);
	            fflush (fp_debug);
                }
            }
    

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "Update CD matrix\n");
                fflush (fp_debug);
            }

	    for (i=0; i<hdr.naxis; i++) {
       
	        indx1 = hdr.axisIndx[i];

	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "ind0= [%d] ind1= [%d]\n", ind0, ind1); 
                    fprintf (fp_debug, "haveCd= [%d]\n", 
		        hdr.haveCd[ind0][ind1]); 
	        
		    fflush (fp_debug);
            
	        }

	   
	        if (hdr.haveCd[ind0][ind1]) {
	
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "update CD matrix\n");
	                fflush (fp_debug);
                    }
	   
	            sprintf (key, "cd%d_%d", l+1, i+1);
       
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "l= [%d] i= [%d] key= [%s]\n", 
		            l, i, key);
                        fprintf (fp_debug, "cd= [%lf]\n", hdr.cd[indx][indx1]);
	                fflush (fp_debug);
                    }

                    istatus = 0;
	            if (fits_update_key (outfptr, TDOUBLE, key, 
		        &hdr.cd[indx][indx1], (char *)NULL, &istatus)) {

                        sprintf (errmsg, "Failed to update keyword %s\n", key);
	                return (-1);
                    }

	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "keyword %s value updated to %lf\n", 
	                    key, hdr.cd[indx][indx1]);
	                fflush (fp_debug);
                    }
                }
	    }

    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "Update PC matrix\n");
                fflush (fp_debug);
            }
     
            for (i=0; i<hdr.naxis; i++) {
       
                ind1 = hdr.axisIndx[i];
	        sprintf (key, "pc%d_%d", l+1, i+1);

	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, 
		        "\nl= [%d] i= [%d] ind0= [%d] ind1= [%d]\n", 
		        l, i, ind0, ind1); 
                    fprintf (fp_debug, "key= [%s]\n", key);
                    fprintf (fp_debug, "havePc= [%d]\n", 
		        hdr.havePc[ind0][ind1]); 
		    fflush (fp_debug);
	        }


                if (hdr.havePc[ind0][ind1]) {
	
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "update PC matrix\n");
	                fflush (fp_debug);
                    }
        
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "pc= [%lf]\n", hdr.pc[ind0][ind1]);
	                fflush (fp_debug);
                    }

                    istatus = 0;
	            if (fits_update_key (outfptr, TDOUBLE, key, 
		        &hdr.pc[ind0][ind1], (char *)NULL, &istatus)) {

                        sprintf (errmsg, "Failed to update keyword %s\n", key);
	                return (-1);
                    }

	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "keyword %s value updated to %lf\n", 
	                    key, hdr.pc[ind0][ind1]);
	                fflush (fp_debug);
                    }
                }
	        else {
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "delete PC matrix key\n");
	                fflush (fp_debug);
                    }
	   
                    istatus = 0;
	            if (fits_delete_key (outfptr, key, &istatus)) {

                        sprintf (errmsg, "Failed to delete keyword %s\n", key);
	                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                            fprintf (fp_debug, "%s\n", errmsg);
	                    fflush (fp_debug);
                        }
                    }
		    else {
	                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                            fprintf (fp_debug, "keyword %s deleted\n", key);
	                    fflush (fp_debug);
                        }
                    }

	        }

	    }
    
            if (hdr.haveCrota[l]) {

                sprintf (key, "CROTA%d", l+1);

                istatus = 0;
                if (fits_update_key (outfptr, TDOUBLE, key, &hdr.crota[l], 
	            (char *)NULL, &istatus)) {
	
                sprintf (errmsg, "Failed to update keyword %s\n", key);
	            return (-1);
                }
    
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "%s updated\n", key);
	            fflush (fp_debug);
                }
        
	    }
        }

    }

/*
    Write to output fitsfile
*/
    nelements = hdr.ns;
    fpixelo[0] = 1;
    fpixelo[1] = 1;
    fpixelo[2] = 1;
    fpixelo[3] = 1;
       
    outbuf  = (double *)malloc(hdr.ns*sizeof(double));
    
    npixel = hdr.ns*hdr.nl;

    
    for (l=0; l<hdr.nplane; l++) {
        
	fpixelo[1] = 1;

        for (j=0; j<hdr.nl; j++) {
        
            for (i=0; i<hdr.ns; i++) {
                
		indx = npixel*l + hdr.ns*j + i;
		outbuf[i] = fitscubebuf[indx];

	    }

            if (fits_write_pix (outfptr, TDOUBLE, fpixelo, nelements,
		 (void *)outbuf, &istatus)) {

                sprintf (errmsg, "fits write error: l= [%d] j= [%d]\n", l, j);
                return (-1);
            }

            fpixelo[1]++;
        }

        fpixelo[2]++;

    }
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here2\n");
        fflush (fp_debug);
    }

    istatus = 0;
    if (fits_close_file (infptr, &istatus)) {
        sprintf (errmsg, "Failed to close input cubepath [%s]\n", cubepath);
        return (-1); 
    }

    istatus = 0;
    if (fits_close_file (outfptr, &istatus)) {
        sprintf (errmsg, "Failed to close out outcubepath [%s]\n", 
	    outcubepath);
        return (-1); 
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "Done rewriteFitsCube\n");
        fflush (fp_debug);
    }

    return (0);
}

