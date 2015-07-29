/*
Theme: This routine extract FITS keywords to a fitshdr structure.

Input:  

Image fits (or image fits cube) file

Output:

 fits header structure

Date written: August 1, 2014 (Mihseh Kong)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <math.h>
#include <sys/file.h>

#include <fitsio.h>
#include "fitshdr.h"


int  str2Integer (char *str, int *intval, char *errmsg);
int  str2Double (char *str, double *dblval, char *errmsg);

extern FILE *fp_debug;


int getFitshdr (char *fname, struct FitsHdr *hdr, int iscube)
{
    char   errmsg[1024];

    char   bscale[40];
    char   bzero[40];
    char   blank[40];

    char   key[40];
    char   *cptr;

    char   str[1024], substr1[10], substr2[10];
   
    char   epochstr[40];

    int    bitpix;
    int    istatus;
    int    istatus1;
    int    l;
    int    nhdu;
    int    hdutype;
    int    hdunum;

    int    ind;
    int    ind0;
    int    ind1;
    int    i;

    int    haveCdeltinfo;

    double epoch;
    
    fitsfile  *fitsptr;
    
    int    debugfile = 0;

    
    if (debugfile) {
	fprintf (fp_debug, "\nEnter getFitshdr: fname= [%s]\n", fname);
        fflush (fp_debug);
    }

    istatus = 0;
    if (fits_open_file (&fitsptr, fname, READONLY, &istatus)) {

        if (debugfile) {
	    fprintf (fp_debug, "istatus= [%d]\n", istatus);
            fflush (fp_debug);
	}

	sprintf (hdr->errmsg, "Failed to open FITS file [%s]\n", fname);
	return (-1);
    } 
   
    hdunum = 1; 
    nhdu = 0;
    istatus = 0;
    istatus = fits_get_num_hdus (fitsptr, &nhdu, &istatus);
    
    if (debugfile) {
	fprintf (fp_debug, 
	    "returned fits_get_hdu_num: istatus= [%d] nhdu= [%d]\n",
	    istatus, nhdu);
        fflush (fp_debug);
    }

    if (hdunum > nhdu) {

        sprintf (hdr->errmsg, "fname [%s] doesn't contain any HDU", fname);
	return (-1);
    }
        
    hdutype = 0;
    istatus = 0;
    istatus = fits_movabs_hdu (fitsptr, hdunum, &hdutype, &istatus);

    if (debugfile) {
	fprintf (fp_debug, 
	    "returned fits_movabs_hdu: istatus= [%d] hdutype= [%d]\n",
	    istatus, hdutype);
        fflush (fp_debug);
    }

/*
    Read fits keywords
*/
    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "simple", str, (char *)NULL, 
        &istatus);
    
    if (debugfile) {
	fprintf (fp_debug, "istatus (simple)= [%d]\n", istatus);
        fflush (fp_debug);
    }

    
    if (istatus == KEY_NO_EXIST) {
        sprintf (hdr->errmsg, "keyword SIMPLE not found in fits header");
        return (-1);
    }
   
    if ((strcmp (str, "T") != 0) && (strcmp (str, "F") != 0)) {
        sprintf (hdr->errmsg, "keyword SIMPLE must be T or F");
        return (-1);
    }
    
    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "bitpix", str, (char *)NULL, 
        &istatus);
    
    if (debugfile) {
	fprintf (fp_debug, "istatus (bitpix)= [%d]\n", istatus);
        fflush (fp_debug);
    }

    
    if (istatus == KEY_NO_EXIST) {
        sprintf (hdr->errmsg, "keyword BITPIX not found in fits header");
        return (-1);
    }
  
    istatus = str2Integer (str, &bitpix, errmsg);
    if (istatus != 0) {
        sprintf (hdr->errmsg, "keyword BITPIX must be an integer");
        return (-1);
    }

    if ((bitpix != 8) &&
        (bitpix != 16) &&
        (bitpix != 32) &&
        (bitpix != 64) &&
        (bitpix != -32) &&
        (bitpix != -64)) {
        
	sprintf (hdr->errmsg, 
	    "keyword BITPIX value must be 8, 16, 32, 64, -32, -64");
        return (-1);
    }

    hdr->bitpix = bitpix;

    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "naxis", str, (char *)NULL, 
        &istatus);
        
    if (istatus == KEY_NO_EXIST) {
        sprintf (hdr->errmsg, "keyword naxis not found in fits header");
        return (-1);
    }

    if (debugfile) {
        fprintf (fp_debug, "str= [%s]\n", str);
        fflush (fp_debug);
    }
    istatus = str2Integer (str, &hdr->naxis, errmsg);
    if (istatus < 0) {
        sprintf (hdr->errmsg, "Failed to convert naxis to integer");
        return (-1);
    }
    
    if (debugfile) {
        fprintf (fp_debug, "naxis= [%d]\n", hdr->naxis);
        fflush (fp_debug);
    }

    
/*
    Extract naxes
*/
    for (l=0; l<hdr->naxis; l++) {

	sprintf (key, "naxis%d", l+1);
        if (debugfile) {
            fprintf (fp_debug, "key= [%s]\n", key);
            fflush (fp_debug);
        }
        
	istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
            &istatus);
        
        if (debugfile) {
            fprintf (fp_debug, "returned fits_read_key (%s): istatus= [%d]\n", 
    	        key, istatus);
            fflush (fp_debug);
        }
        
        if (istatus == KEY_NO_EXIST) {
            sprintf (errmsg, "keyword %s not found in fits header", key);
            return (-1);
        }
            
	if (debugfile) {
            fprintf (fp_debug, "str= [%s]\n", str);
            fflush (fp_debug);
        }

        istatus = str2Integer (str, &hdr->naxes[l], errmsg);
    
        if (istatus < 0) {
            sprintf (errmsg, "Failed to convert %s string to integer", str);
            return (-1);
        }
    }


    hdr->bunit[0] = '\0'; 
    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "bunit", hdr->bunit, 
        (char *)NULL, &istatus);
    
    if (debugfile) {
	fprintf (fp_debug, "istatus (bunit)= [%d]\n", istatus);
        fflush (fp_debug);
    }

    if (istatus == KEY_NO_EXIST) {
	strcpy (hdr->bunit, "DN");
    }

    istatus = 0;
    bscale[0] = '\0';
    hdr->bscaleExist = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "BSCALE", bscale, 
            (char *)NULL, &istatus);
        
    if (debugfile) {
	fprintf (fp_debug, "istatus (bscale)= [%d]\n", istatus);
        fflush (fp_debug);
    }

    if (istatus != KEY_NO_EXIST) {
        
	if (debugfile) {
            fprintf (fp_debug, "bscale= [%s]\n", bscale);
            fflush (fp_debug);
        }

	hdr->bscaleExist = 1;
        
	istatus = str2Double (bscale, &hdr->bscale, errmsg);
    
        if (istatus < 0) {
            hdr->bscaleExist = 0; 
	}
    }

    if (debugfile) {
        fprintf (fp_debug, "bscaleExsit= [%d]\n",  hdr->bscaleExist);
        if (hdr->bscaleExist) 
	    fprintf (fp_debug, "bscale= [%lf]\n",  hdr->bscale);
        fflush (fp_debug);
    }


    istatus = 0;
    bzero[0] = '\0';
    hdr->bzeroExist = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "BZERO", bzero, 
            (char *)NULL, &istatus);
        
    if (debugfile) {
	fprintf (fp_debug, "istatus (bzero)= [%d]\n", istatus);
        fflush (fp_debug);
    }

    if (istatus != KEY_NO_EXIST) {
    
        if (debugfile) {
            fprintf (fp_debug, "bzero= [%s]\n", bzero);
            fflush (fp_debug);
        }
	
	hdr->bscaleExist = 1;

        istatus = str2Double (bzero, &hdr->bzero, errmsg);
    
        if (istatus < 0) {
	    hdr->bscaleExist = 0;
	}
    }

    if (debugfile) {
        fprintf (fp_debug, "bzeroExsit= [%d]\n",  hdr->bzeroExist);
        if (hdr->bzeroExist) 
	    fprintf (fp_debug, "bzero= [%lf]\n",  hdr->bzero);
        fflush (fp_debug);
    }


    istatus = 0;
    blank[0] = '\0';
    hdr->blankExist = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "BLANK", blank, 
            (char *)NULL, &istatus);
        
    if (debugfile) {
	fprintf (fp_debug, "istatus (blank)= [%d]\n", istatus);
        fflush (fp_debug);
    }

    if (istatus != KEY_NO_EXIST) {
    
        if (debugfile) {
            fprintf (fp_debug, "blank= [%s]\n", blank);
            fflush (fp_debug);
        }

	hdr->blankExist = 1;

        istatus = str2Double (blank, &hdr->blank, errmsg);
    
        if (istatus < 0) {
	    hdr->blankExist = 0;
        }
    }
    if (debugfile) {
        fprintf (fp_debug, 
	    "blankExist= [%d]\n",  hdr->blankExist);
        if (hdr->blankExist)
	    fprintf (fp_debug, "blank= [%lf]\n",  hdr->blank);
        fflush (fp_debug);
    }


/*
    Extract cunit
*/
    for (l=0; l<hdr->naxis; l++) {

	sprintf (key, "cunit%d", l+1);
        if (debugfile) {
            fprintf (fp_debug, "key= [%s]\n", key);
            fflush (fp_debug);
        }
        
	hdr->cunit[l][0] = '\0';

	istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
            &istatus);
        
        if (debugfile) {
            fprintf (fp_debug, "returned fits_read_key (%s): istatus= [%d]\n", 
	        key, istatus);
            fflush (fp_debug);
        }
   
        hdr->haveCunit[l] = 1;
        if (istatus == KEY_NO_EXIST) {
            hdr->haveCunit[l] = 0;
        }
        
	strcpy (hdr->cunit[l], str);
    }

        
/*
    Extract ctype
*/
    for (l=0; l<hdr->naxis; l++) {

	sprintf (key, "ctype%d", l+1);
        if (debugfile) {
            fprintf (fp_debug, "key= [%s]\n", key);
            fflush (fp_debug);
        }
        
	hdr->ctype[l][0] = '\0';

	istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
            &istatus);
        
        if (debugfile) {
            fprintf (fp_debug, "returned fits_read_key (%s): istatus= [%d]\n", 
	        key, istatus);
            fflush (fp_debug);
        }
   
        hdr->haveCtype[l] = 1;
        if (istatus == KEY_NO_EXIST) {
            hdr->haveCtype[l] = 0;
        }
        
	strcpy (hdr->ctype[l], str);
    }

	
/*
    Extract crpix
*/
    for (l=0; l<hdr->naxis; l++) {
	
	sprintf (key, "CRPIX%d", l+1);
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "key= [%s]\n", key);
	    fflush (fp_debug);
        }
        
	istatus = 0;
	istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
            &istatus);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, 
	        "returned fits_read_key %s: istatus= [%d]\n", key, istatus);
	    fflush (fp_debug);
        }
        
        if (istatus == KEY_NO_EXIST) {

            hdr->haveCrpix[l] = 0;
            hdr->crpix[l] = 0.;

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "key= [%s] doesn't exists\n", key);
	        fflush (fp_debug);
            }
        }
	else {
            hdr->haveCrpix[l] = 1;
       
            istatus = str2Double (str, &hdr->crpix[l], errmsg);
    
            if (istatus < 0) {
                sprintf (errmsg, "Failed to convert %s string to double", str);
                    return (-1);
            }
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "crpix[%d]= [%lf]\n", l, hdr->crpix[l]);
	        fflush (fp_debug);
            }
        
        }
    }

/*
    Extract crval
*/
    for (l=0; l<hdr->naxis; l++) {
	
        sprintf (key, "CRVAL%d", l+1);
        
	istatus = 0;
	istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
            &istatus);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, 
	        "returned fits_read_key %s: istatus= [%d]\n", key, istatus);
	    fflush (fp_debug);
        }
        
        if (istatus == KEY_NO_EXIST) {

            hdr->haveCrval[l] = 0;
            hdr->crval[l] = 0.;


	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "key= [%s] doesn't exists\n", key);
	        fflush (fp_debug);
            }
        }
	else {
            hdr->haveCrval[l] = 1;
       
            istatus = str2Double (str, &hdr->crval[l], errmsg);
    
            if (istatus < 0) {
                sprintf (errmsg, "Failed to convert %s string to double", str);
                    return (-1);
            }
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "crval[%d]= [%lf]\n", l, hdr->crval[l]);
	        fflush (fp_debug);
            }
        
        }
    }

/*
    Extract cdelt
*/
    for (l=0; l<hdr->naxis; l++) {
	
	sprintf (key, "CDELT%d", l+1);
        
	istatus = 0;
	istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
            &istatus);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returned fits_read_key %s: istatus= [%d]\n",
		key, istatus);
	    fflush (fp_debug);
        }
        
        if (istatus == KEY_NO_EXIST) {

            hdr->haveCdelt[l] = 0;
            hdr->cdelt[l] = 0.;

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "key= [%s] doesn't exists\n", key);
	        fflush (fp_debug);
            }
        }
	else {
            hdr->haveCdelt[l] = 1;
       
            istatus = str2Double (str, &hdr->cdelt[l], errmsg);
    
            if (istatus < 0) {
                sprintf (errmsg, "Failed to convert %s string to double", str);
                    return (-1);
            }
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "cdelt[%d]= [%lf]\n", l, hdr->cdelt[l]);
	        fflush (fp_debug);
            }
        
        }
    }

        

/*
    haveCdeltinfo = 0;
    if (hdr->haveCdelt[0] && hdr->haveCdelt[1]) {
        haveCdeltinfo = 1;
    }
    
    if (!haveCdeltinfo) {
        if (hdr->haveCd[0][0] && hdr->haveCd[0][1] && 
	    hdr->haveCd[1][0] && hdr->haveCd[1][1]) {
	    
	    haveCdeltinfo = 1;
	}
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "haveCdeltinfo= [%d]\n", haveCdeltinfo); 
	fflush (fp_debug);
    }

    if ((!hdr->haveCtype[0]) || (!hdr->haveCtype[1]) ||
	(!hdr->haveCrpix[0]) || (!hdr->haveCrpix[1]) ||
        (!hdr->haveCrval[0]) || (!hdr->haveCrval[1]) ||
        (!haveCdeltinfo)) {
        
	hdr->nowcs = 1;
    
        hdr->axisIndx[0] = 0;
        hdr->axisIndx[1] = 1;
        hdr->axisIndx[2] = 2;
        hdr->ns = hdr->naxes[0];
        hdr->nl = hdr->naxes[1];
        hdr->nplane = hdr->naxes[2];

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "nowcs= [%d]\n", hdr->nowcs); 
            fprintf (fp_debug, "ns= [%d] nl= [%d] nplane= [%d]\n", 
	        hdr->ns, hdr->nl, hdr->nplane); 
	    fflush (fp_debug);
        }

        
	return (0);
    }
   
    hdr->nowcs = 0;

*/

/*
    Assign the default axisIndx
*/        
	
    hdr->axisIndx[0] = 0;
    hdr->axisIndx[1] = 1;
    hdr->axisIndx[2] = 2;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nXXX: default axisIndx\n");
        fprintf (fp_debug, "axisIndx[0]= [%d]\n", hdr->axisIndx[0]); 
        fprintf (fp_debug, "axisIndx[1]= [%d]\n", hdr->axisIndx[1]); 
        fprintf (fp_debug, "axisIndx[2]= [%d]\n", hdr->axisIndx[2]); 
        fprintf (fp_debug, "\n");
	fflush (fp_debug);
    }


/*
    Figure out which axis is ns, nl, and nplane:
    axisIndx[0] : ns,
    axisIndx[1] : nl,
    axisIndx[2] : nplane
*/
    for (l=0; l<hdr->naxis; l++) {
	
        strcpy (str, hdr->ctype[l]);
        cptr = (char *)NULL;
        cptr = strchr (str, '-');
        if (cptr != (char *)NULL) {
            *cptr = '\0';
        }
        
	if (debugfile) {
            fprintf (fp_debug, "str= [%s]\n", str);
            fflush (fp_debug);
        }

        if ((strcmp (str, "RA") == 0)  || 
            (strcmp (str, "GLON") == 0) ||
            (strcmp (str, "ELON") == 0)) {
        
	    if (debugfile) {
                fprintf (fp_debug, "here1\n");
                fflush (fp_debug);
            }

	    hdr->axisIndx[0] = l;
            hdr->ns = hdr->naxes[l];
    
            if (debugfile) {
                fprintf (fp_debug, "axisIndx[0]= [%d] ns= [%d]\n", l, hdr->ns); 
                fflush (fp_debug);
            }

	}
	else if ((strcmp (str, "DEC") == 0)  || 
            (strcmp (str, "GLAT") == 0) ||
            (strcmp (str, "ELAT") == 0)) {
       
	    if (debugfile) {
                fprintf (fp_debug, "here2\n");
                fflush (fp_debug);
            }

            hdr->nl = hdr->naxes[l];
            hdr->axisIndx[1] = l;
            
	    if (debugfile) {
                fprintf (fp_debug, "axisIndx[1]= [%d] nl= [%d]\n", l, hdr->nl); 
                fflush (fp_debug);
            }
	}
	else {

	    if (debugfile) {
                fprintf (fp_debug, "here3\n");
                fflush (fp_debug);
            }

	    hdr->nplane = hdr->naxes[l];
            hdr->axisIndx[2] = l;
	    
	    if (debugfile) {
                fprintf (fp_debug, "axisIndx[2]= [%d] nplane= [%d]\n", 
		    l, hdr->nplane);
                fflush (fp_debug);
            }
	}
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nXXX: Final axisIndx\n");
        fprintf (fp_debug, "axisIndx[0]= [%d]\n", hdr->axisIndx[0]); 
        fprintf (fp_debug, "axisIndx[1]= [%d]\n", hdr->axisIndx[1]); 
        fprintf (fp_debug, "axisIndx[2]= [%d]\n", hdr->axisIndx[2]); 
        fprintf (fp_debug, "\n");
	fflush (fp_debug);
    }


/*
    Extract crota
*/
    for (l=0; l<hdr->naxis; l++) {
	
        sprintf (key, "CROTA%d", l+1);
        
        istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, key, str, 
            (char *)NULL, &istatus);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returned fits_read_key %s: istatus= [%d]\n",
	        key, istatus);
	    fflush (fp_debug);
        }
        
        if (istatus == KEY_NO_EXIST) {

	    if (debugfile) {
                fprintf (fp_debug, "xxx1\n");
                fflush (fp_debug);
            }

            hdr->haveCrota[l] = 0;
            hdr->crota[l] = 0.;


	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "key= [%s] doesn't exists\n", key);
	        fflush (fp_debug);
            }
        }
        else {
	    
	    if (debugfile) {
                fprintf (fp_debug, "xxx2: str= [%s]\n", str);
                fflush (fp_debug);
            }

            hdr->haveCrota[l] = 1;
       
            istatus = str2Double (str, &hdr->crota[l], errmsg);
    
            if (istatus < 0) {
                hdr->haveCrota[l] = 0; 
	    }
    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

                fprintf (fp_debug, "haveCrota[%d]= [%d]\n", 
		    l, hdr->haveCrota[l]);
                
		if (hdr->haveCrota[l]) 
                    fprintf (fp_debug, "crota[%d]= [%lf]\n", l, hdr->crota[l]);
	        fflush (fp_debug);
            }
        }
            
	    if (debugfile) {
                fprintf (fp_debug, "xxx1\n");
                fflush (fp_debug);
            }

    }

            if (debugfile) {
                fprintf (fp_debug, "here1\n");
                fflush (fp_debug);
            }

/*
    Extract cd matrix
*/
    for (l=0; l<hdr->naxis; l++) {

	for (i=0; i<hdr->naxis; i++) {
        
	    sprintf (key, "CD%d_%d", l+1, i+1);
        
	    istatus = 0;
	    istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
                &istatus);
        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "returned fits_read_key %s: istatus= [%d]\n",
		    key, istatus);
	        fflush (fp_debug);
            }
        
            if (istatus == KEY_NO_EXIST) {

                hdr->haveCd[l][i] = 0;
                hdr->cd[l][i] = 0.;


	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "key= [%s] doesn't exists\n", key);
	            fflush (fp_debug);
                }
            }
	    else {
                hdr->haveCd[l][i] = 1;
       
                istatus = str2Double (str, &hdr->cd[l][i], errmsg);
    
                if (istatus < 0) {
                    sprintf (errmsg, "Failed to convert %s string to double", 
		        str);
                    return (-1);
                }
	        
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "cd[%d][%d]= [%lf]\n", 
		        l, i, hdr->cd[l][i]);
	            fflush (fp_debug);
                }
        
            }
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                
		fprintf (fp_debug, "haveCd[%d][%d]= [%d]\n", 
		    l, i, hdr->haveCd[l][i]);
		
		if (hdr->haveCd[l][i]) {
                    fprintf (fp_debug, "Cd[%d][%d]= [%lf]\n", 
		        l, i, hdr->cd[l][i]);
		}
		fflush (fp_debug);
            }
	}
    }

            if (debugfile) {
                fprintf (fp_debug, "here2\n");
                fflush (fp_debug);
            }

/*
    Extract pc matrix
*/
    for (l=0; l<hdr->naxis; l++) {

	for (i=0; i<hdr->naxis; i++) {
        
            sprintf (key, "PC%d_%d", l+1, i+1);
        
	    istatus = 0;
	    istatus = fits_read_key (fitsptr, TSTRING, key, str, (char *)NULL, 
                &istatus);
        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "returned fits_read_key %s: istatus= [%d]\n",
		    key, istatus);
	        fflush (fp_debug);
            }
        
            if (istatus == KEY_NO_EXIST) {

                hdr->havePc[l][i] = 0;
                hdr->pc[l][i] = 0;

	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "key= [%s] doesn't exists\n", key);
	            fflush (fp_debug);
                }
            }
	    else {
                hdr->havePc[l][i] = 1;
       
                istatus = str2Double (str, &hdr->pc[l][i], errmsg);
    
                if (istatus < 0) {
                    sprintf (errmsg, "Failed to convert %s string to double", 
		        str);
                    return (-1);
                }
            }
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                
		fprintf (fp_debug, "havePc[%d][%d]= [%d]\n", 
		    l, i, hdr->havePc[l][i]);
		
		if (hdr->havePc[l][i]) {
                    fprintf (fp_debug, "pc[%d][%d]= [%lf]\n", 
		        l, i, hdr->pc[l][i]);
		}
		fflush (fp_debug);
            }
        
	}
    }

            if (debugfile) {
                fprintf (fp_debug, "here3\n");
                fflush (fp_debug);
            }


    hdr->csysstr[0] = '\0';
    ind = hdr->axisIndx[0];

    if (hdr->haveCtype[ind]) {
    
        strncpy (substr1, hdr->ctype[ind], 4); 
        substr1[4] = '\0';
        strncpy (substr2, substr1, 3); 
        substr2[3] = '\0';
        
        if (debugfile) {
            fprintf (fp_debug, "substr1= [%s] substr2= [%s]\n", 
	        substr1, substr2);
            fflush (fp_debug);
        }

        hdr->csysstr[0] = '\0'; 
        if ((strcmp (substr2, "LON") == 0) || 
            (strcmp (substr1, "GLON") == 0)) {
	    
	    strcpy (hdr->csysstr, "gal");
        }
        if (strcmp (substr1, "ELON") == 0) {
	    strcpy (hdr->csysstr, "ec");
        }

        strncpy (substr2, substr1, 2); 
        substr2[2] = '\0';
        
        if (debugfile) {
            fprintf (fp_debug, "substr1= [%s] substr2= [%s]\n", 
	        substr1, substr2);
            fflush (fp_debug);
        }

        if (strcmp (substr2, "RA") == 0) {
	    strcpy (hdr->csysstr, "eq");
        }
    }
    
            if (debugfile) {
                fprintf (fp_debug, "here4\n");
                fflush (fp_debug);
            }

    if (debugfile) {
        fprintf (fp_debug, "csysstr= [%s]\n", hdr->csysstr);
        fflush (fp_debug);
    }


    hdr->equinoxstr[0] = '\0';

    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "EQUINOX", hdr->equinoxstr, 
        (char *)NULL, &istatus);
    
    if (debugfile) {
        fprintf (fp_debug, "returned EQUINOX: istatus= [%d]\n", istatus);
        fprintf (fp_debug, "equinoxstr= [%s]\n", hdr->equinoxstr);
        fflush (fp_debug);
    }

    if (istatus == KEY_NO_EXIST) {
	hdr->haveEquinox = 0;
    }
    else {
	hdr->haveEquinox = 1;
        istatus = str2Double (hdr->equinoxstr, &hdr->equinox, errmsg);
        if (istatus < 0) {
	    hdr->haveEquinox = 0;
        }
        
        if (debugfile) {
	    fprintf (fp_debug, "equinoxstr= [%s]\n", hdr->equinoxstr);
            fflush (fp_debug);
        }
    }

    hdr->epochstr[0] = '\0';

    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "EPOCH", hdr->epochstr, 
        (char *)NULL, &istatus);
   
    if (istatus == KEY_NO_EXIST) {
	hdr->haveEpoch = 0;
    }
    else {
	hdr->haveEpoch = 1;
        istatus = str2Double (hdr->epochstr, &hdr->epoch, errmsg);
        if (istatus < 0) {
	    hdr->haveEpoch = 0;
        }
        
	if (debugfile) {
	    fprintf (fp_debug, "epochstr= [%s]\n", hdr->epochstr);
	    fprintf (fp_debug, "epoch= [%lf]\n", hdr->epoch);
            fflush (fp_debug);
        }

    }
    
    if ((hdr->haveEquinox) || (hdr->haveEpoch)) {	

        if (hdr->haveEquinox) {
            epoch = hdr->equinox;
            strcpy (epochstr, hdr->equinoxstr);
        }
	else { 
            epoch = hdr->epoch;
            strcpy (epochstr, hdr->epochstr);
        }


        if (epoch < 1980.0) {
            sprintf (hdr->epochstr, "b%s", epochstr);
    
	    if (strcasecmp (hdr->csysstr, "eq") == 0) 
	        hdr->sys = 1;
            else if (strcasecmp (hdr->csysstr, "ec") == 0) 
	        hdr->sys = 3;
            else if (strcasecmp (hdr->csysstr, "gal") == 0) 
	        hdr->sys = 4;
            else if (strcasecmp (hdr->csysstr, "sgal") == 0) 
	        hdr->sys = 5;
        }
        else {
            sprintf (hdr->epochstr, "j%s", epochstr);
    
	    if (strcasecmp (hdr->csysstr, "eq") == 0) 
	        hdr->sys = 0;
            else if (strcasecmp (hdr->csysstr, "ec") == 0) 
	        hdr->sys = 2;
            else if (strcasecmp (hdr->csysstr, "gal") == 0) 
	        hdr->sys = 4;
            else if (strcasecmp (hdr->csysstr, "sgal") == 0) 
	        hdr->sys = 5;
        }
    }

    if (debugfile) {
	fprintf (fp_debug, "hdr->epochstr= [%s]\n", hdr->epochstr);
        fflush (fp_debug);
    }



    ind0 = hdr->axisIndx[0];
    ind1 = hdr->axisIndx[1];

    if (debugfile) {
	fprintf (fp_debug, "ind0= [%d] ind1= [%d]\n", ind0, ind1);
        fflush (fp_debug);
    }

    haveCdeltinfo = 0;
    if (hdr->haveCdelt[ind0] && hdr->haveCdelt[ind1]) {
        haveCdeltinfo = 1;
    }
    
    if (!haveCdeltinfo) {
        if (hdr->haveCd[ind0][ind0] && hdr->haveCd[ind0][ind1] && 
	    hdr->haveCd[ind1][ind0] && hdr->haveCd[ind1][ind1]) {
	    
	    haveCdeltinfo = 1;
	}
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "haveCdeltinfo= [%d]\n", haveCdeltinfo); 
	fflush (fp_debug);
    }


/*
    No WCS info, treat cube as x, y, and time/wave
*/
    if ((!hdr->haveCtype[ind0]) || (!hdr->haveCtype[ind1]) ||
	(!hdr->haveCrpix[ind0]) || (!hdr->haveCrpix[ind1]) ||
        (!hdr->haveCrval[ind0]) || (!hdr->haveCrval[ind1]) ||
        (!haveCdeltinfo)) {
        
	hdr->nowcs = 1;
    
        hdr->axisIndx[0] = 0;
        hdr->axisIndx[1] = 1;
        hdr->axisIndx[2] = 2;
        hdr->ns = hdr->naxes[0];
        hdr->nl = hdr->naxes[1];
        hdr->nplane = hdr->naxes[2];

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "nowcs= [%d]\n", hdr->nowcs); 
            fprintf (fp_debug, "ns= [%d] nl= [%d] nplane= [%d]\n", 
	        hdr->ns, hdr->nl, hdr->nplane); 
	    fflush (fp_debug);
        }
    }
    else {
        hdr->nowcs = 0;
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "nowcs= [%d]\n", hdr->nowcs); 
	fflush (fp_debug);
    }


/*
    Special keywords for projects
    
    Osiris Cube: 
    
    OBJECT: object name (objname),  
    SFILTER: filter (sfilter),
    SSCALE: pixel scale (specscale)

    NIRC2 image: 

    TARGNAME: object name (objname), 
    FILTER: filter (filter),
    PIXSCALE: pixel scale (pixscale) 

*/
    hdr->objname[0] = '\0';
    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "OBJECT", hdr->objname, 
        (char *)NULL, &istatus);
   
    if (debugfile) {
	fprintf (fp_debug, "read keyword OBJECT: istatus= [%d]\n", istatus);
        fflush (fp_debug);
    }
    
    if (istatus == KEY_NO_EXIST) {
    
        hdr->objname[0] = '\0';
        istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, "TARGNAME", hdr->objname, 
            (char *)NULL, &istatus);
   
        if (debugfile) {
	    fprintf (fp_debug, "read keyword TARGNAME: istatus= [%d]\n", 
	        istatus);
            fflush (fp_debug);
        }
    }
        
    if (debugfile) {
	fprintf (fp_debug, "objname(TARGNAME)= [%s]\n", hdr->objname);
        fflush (fp_debug);
    }
   
    hdr->filter[0] = '\0';
    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "SFILTER", hdr->filter, 
        (char *)NULL, &istatus);
   
    if (debugfile) {
	fprintf (fp_debug, "read keyword SFILTER: istatus= [%d]\n", istatus);
        fflush (fp_debug);
    }
    
    if (istatus == KEY_NO_EXIST) {
    
        hdr->filter[0] = '\0';
        istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, "FILTER", hdr->filter, 
            (char *)NULL, &istatus);
    
        if (debugfile) {
	    fprintf (fp_debug, "read keyword FILTER: istatus= [%d]\n", 
	        istatus);
            fflush (fp_debug);
        }
    }
   
    if (debugfile) {
	fprintf (fp_debug, "filter= [%s]\n", hdr->filter);
        fflush (fp_debug);
    }
    
    hdr->pixscale[0] = '\0';
    istatus = 0;
    istatus = fits_read_key (fitsptr, TSTRING, "SSCALE", hdr->pixscale, 
        (char *)NULL, &istatus);
   
    if (debugfile) {
	fprintf (fp_debug, "read keyword SSCALE: istatus= [%d]\n", istatus);
        fflush (fp_debug);
    }
    
    if (istatus == KEY_NO_EXIST) {
    
        hdr->pixscale[0] = '\0';
        istatus = 0;
        istatus = fits_read_key (fitsptr, TSTRING, "PIXSCALE", hdr->pixscale, 
            (char *)NULL, &istatus);
    
        if (debugfile) {
	    fprintf (fp_debug, "read keyword PIXSCALE: istatus= [%d]\n", 
	        istatus);
            fflush (fp_debug);
        }
    }
   
    if (debugfile) {
	fprintf (fp_debug, "pixscale= [%s]\n", hdr->pixscale);
        fflush (fp_debug);
    }
 

    istatus = fits_close_file (fitsptr, &istatus1);

    return (0);
}

