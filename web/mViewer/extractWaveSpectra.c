/*
Theme: This routine extract the wavelength spectra from the Fits cube.
    The cube data has been arranged to be
    
    bitpix = -64,
    naxis1 = ra,
    naxis2 = dec,
    naxis3 = nplane.

Input:  

Image cube fits file, 
pixel x, y

Output:

1D spectra in IPAC format (dn vs. wavelength).

Written: October 24, 2014 (Mihseh Kong)
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


int getFitshdr (char *cubepath, struct FitsHdr *hdr, int iscube);

int  str2Integer (char *str, int *intval, char *errmsg);
int  str2Double (char *str, double *dblval, char *errmsg);
    
int checkFileExist (char *filenamel, char *rootname, char *suffix,
        char *directory, char *filepath);


extern FILE *fp_debug;

int extractWaveSpectra (struct Mviewer *param)
{
    FILE   *fp;

    struct FitsHdr  hdr;

    char   str[1024];

    char   colname[40];
    char   datatype[40];
    char   null[40];
    char   unit[40];

    char   rootname[1024];
    char   suffix[40];

    int    istatus;
    int    status;
    int    fileExist; 

    long    nelements;
  
    int    iscube;    

    int    ixpix;
    int    iypix;

    int    ixs;
    int    iys;
    int    ixe;
    int    iye;

    double xs;
    double ys;
    double xe;
    double ye;

    int    npixel;

    int    l;
    int    j;
    int    i;
    int    nullcnt;

    long   fpixel[4];
    
    double *data;
    double *xarr;
    double *yarr;
    
    fitsfile  *infptr;

    
    int    debugfile = 1;


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "\nEnter extractWaveSpectra: cubepath= [%s]\n", 
	    param->imcubepath);
	
	fprintf (fp_debug, "\nEnter extractWaveSpectra: imageFile= [%s]\n", 
	    param->imageFile);

	fprintf (fp_debug, "\nshrunkimfile= [%s]\n", param->shrunkimfile);
	fprintf (fp_debug, "\nwaveplottype= [%s]\n", param->waveplottype);

        if (strcasecmp (param->waveplottype, "pix") == 0) {
	    fprintf (fp_debug, "xs= [%lf] ys= [%lf]\n", 
	        param->xs, param->ys);
	}
	else {
	    fprintf (fp_debug, "xs= [%lf] ys= [%lf] xe= [%lf] ye= [%lf]\n", 
	        param->xs, param->ys, param->xe, param->ye);
        }
        
	fprintf (fp_debug, "zoomfactor= [%lf]\n", param->zoomfactor);
        fprintf (fp_debug, "ss= [%lf] sl= [%lf\n", param->ss, param->sl);
        fprintf (fp_debug, "xflip= [%d] yflip= [%d]\n", 
	    param->xflip, param->yflip);
        fflush (fp_debug);
    }


    iscube = 1;
    istatus = getFitshdr (param->imcubepath, &hdr, iscube); 
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "returned getFitshdr: istatus= %d\n", istatus);
        fflush (fp_debug);
    }

    if (istatus == -1) {
	sprintf (param->errmsg, "Failed to getFitshdr: FITS file %s\n", 
	    param->imcubepath);
	return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
        fprintf (fp_debug, "ns= [%d] nl= [%d] nplane= [%d]\n", 
	    hdr.ns, hdr.nl, hdr.nplane);
        fprintf (fp_debug, "crval3= [%lf] cdelt3= [%lf]\n", 
	    hdr.crval[2], hdr.cdelt[2]);
        fflush (fp_debug);
    }


/*
    Adjust xpix, ypix based on xflip, yflip
*/
    if (strcasecmp (param->waveplottype, "pix") == 0) {
    
        param->xpix = param->xs/param->zoomfactor + param->ss;
        param->ypix = param->ys/param->zoomfactor + param->sl;
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "xpix= [%lf] ypix= [%lf]\n", 
	        param->xpix, param->ypix);
	    fflush (fp_debug);
        }
        
	ixpix = (int)(param->xpix+0.5);
	iypix = (int)(param->ypix+0.5);

        if (param->xflip)
            ixpix = hdr.ns - ixpix;
        else
            ixpix = ixpix + 1;
    
        if (param->yflip)
            iypix = hdr.nl - iypix;
        else
            iypix = iypix + 1;
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "ixpix= [%d] iypix= [%d]\n", ixpix, iypix);
	    fflush (fp_debug);
        }
        
    }
    else if (strcasecmp (param->waveplottype, "ave") == 0) {

        xs = param->xs/param->zoomfactor + param->ss;
        ys = param->ys/param->zoomfactor + param->sl;
        xe = param->xe/param->zoomfactor + param->ss;
        ye = param->ye/param->zoomfactor + param->sl;
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "xs= [%lf] ys= [%lf]\n", xs, ys);
	    fprintf (fp_debug, "xe= [%lf] ye= [%lf]\n", xe, ye);
	    fflush (fp_debug);
        }
        
	ixs = (int)(xs+0.5);
	iys = (int)(ys+0.5);
	ixe = (int)(xe+0.5);
	iye = (int)(ye+0.5);

        
	if (param->xflip) {
            ixs = hdr.ns - ixs;
            ixe = hdr.ns - ixe;
        }
	else {
            ixs = ixs + 1;
            ixe = ixe + 1;
        }
    
        if (param->yflip) {
            iys = hdr.nl - iys;
            iye = hdr.nl - iye;
        }
	else {
            iys = iys + 1;
            iye = iye + 1;
        }

	if (iys > iye) {
            i = iys;
	    iys = iye;
	    iye = i;
	}
	if (ixs > ixe) {
            i = ixs;
	    ixs = ixe;
	    ixe = i;
	}
    
        npixel = (ixe-ixs+1) * (iye-iys+1);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "ixs= [%d] iys= [%d] ixe= [%d] iye= [%d]\n", 
	        ixs, iys, ixe, iye);
	    fprintf (fp_debug, "npixel= [%d]\n", npixel);
	    fflush (fp_debug);
        }
    
    }


/*
    malloc plot arrays: xarr and yarr
*/
    xarr = (double *)malloc (hdr.nplane*sizeof(double));
    yarr = (double *)malloc (hdr.nplane*sizeof(double));
    for (i=0; i<hdr.nplane; i++) {
        xarr[i] = 0.;
        yarr[i] = 0.;
    }


/*
    Create plot xarr based on wavelen axis (i.e. crval3 and cdelt3)
*/
    for (i=0; i<hdr.nplane; i++) {
        xarr[i] = hdr.crval[2] + i*hdr.cdelt[2];
    }

/*
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
        for (i=0; i<hdr.nplane; i++) {
	    fprintf (fp_debug, "i= [%d] xarr= [%lf]\n", i, xarr[i]);
        }
	fflush (fp_debug);
    }
*/


/*
    Open cubefile for read
*/
    istatus = 0;
    if (fits_open_file (&infptr, param->imcubepath, READONLY, &status)) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	    fprintf (fp_debug, "istatus= [%d] status= [%d]\n", istatus, status);
            fflush (fp_debug);
	}

	sprintf (param->errmsg, "Failed to open FITS cubefile %s\n", 
	    param->imcubepath);
        
	sprintf (param->retstr, "[struct stat=error, msg=%s]\n", 
	    param->errmsg); 
	return (-1);
    } 
   

/*
    Read data from nth plane and write to output fitsfile
*/
    nelements = 1;
    data  = (double *)malloc(nelements*sizeof(double));

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "waveplottype= [%s]\n", param->waveplottype);
        fflush (fp_debug);
    }         
	
    
    for (l=0; l<hdr.nplane; l++) { 

        fpixel[2] = l;
        fpixel[3] = 1;
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "l= [%d] fpixel[2]= [%ld]\n", l, fpixel[2]);
            fflush (fp_debug);
        }         
	
	if (strcasecmp (param->waveplottype, "pix") == 0) {

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "here1\n");
                fflush (fp_debug);
            }             
            
	    fpixel[0] = ixpix;
            fpixel[1] = iypix;
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "ixpix= [%d] iypix= [%d]\n", ixpix, iypix);
	        fprintf (fp_debug, "fpixel[0]= [%ld] fpixel[1]= [%ld]\n", 
		    fpixel[0], fpixel[1]);
	        fprintf (fp_debug, "call fits_read_pix\n");
                fflush (fp_debug);
            }             

            istatus = 0;
	    istatus = fits_read_pix (infptr, TDOUBLE, fpixel, nelements, NULL,
                (void *)data, &nullcnt, &istatus);
           
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "returned fits_read_pix: istatus= [%d]\n",
		    istatus);
                fflush (fp_debug);
            }    

            
            if (istatus == -1) {
                sprintf (param->errmsg, "Failed to read FITS file %s\n",
		    param->imcubepath);
	        sprintf (param->retstr, "[struct stat=error, msg=%s]\n", 
	            param->errmsg); 
	        return (-1);
            }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "l= [%d] data= [%lf]\n", l, data[0]);
                fflush (fp_debug);
            }         

            yarr[l] = (double)data[0];
	}
	else if (strcasecmp (param->waveplottype, "ave") == 0) {

            for (j=iys; j<=iye; j++) {
	        
                fpixel[1] = j;
	
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "j= [%d] fpixel[1]= [%ld]\n", 
		        j, fpixel[1]);
	            fflush (fp_debug);
	        }        
		
                for (i=ixs; i<=ixe; i++) {

                    fpixel[0] = i;
		
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "i= [%d] fpixel[0]= [%ld]\n", 
		            i, fpixel[0]);
                        fflush (fp_debug);
                    }
                    
		    istatus = 0;
	            istatus = fits_read_pix (infptr, TDOUBLE, fpixel, 
		        nelements, NULL, (void *)data, &nullcnt, &istatus);
	    
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, 
			    "returned fits_read_pix: istatus= [%d]\n", istatus);
                        fflush (fp_debug);
                    }    
            
                    if (istatus == -1) {
                        sprintf (param->errmsg, "Failed to read FITS file %s\n",
		            param->imcubepath);
	                sprintf (param->retstr, "[struct stat=error, msg=%s]\n",
	                    param->errmsg); 
	                return (-1);
                    }

	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "l= [%d] data= [%lf]\n", l, data[0]);
                        fflush (fp_debug);
                    }         
            
                    yarr[l] += (double)data[0]/npixel;
	            
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "yarr= [%lf]\n", yarr[l]);
                        fflush (fp_debug);
                    }         
            
	        }
	    }
	}

    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        for (l=0; l<hdr.nplane; l++) {
	    fprintf (fp_debug, "l= [%d] xarr= [%lf] yarr= [%lf]\n", 
	        l, xarr[l], yarr[l]);
        }
        fflush (fp_debug);
    }         

    istatus = 0;
    if (fits_close_file (infptr, &istatus)) {
        sprintf (param->errmsg, "Failed to close imcubepath %s\n", 
	    param->imcubepath);
        
	sprintf (param->retstr, "[struct stat=error, msg=%s]\n", 
	    param->errmsg); 
        return (-1); 
    }
    

/*
    Write tblarr to output IPAC table
*/
    sprintf (param->plotfile, "%s_plot.tbl", param->imageFile);
    sprintf (param->plotpath, "%s/%s", param->directory, param->plotfile);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "plotpath= [%s]\n", param->plotpath);
        fflush (fp_debug);
    }         

    fp = (FILE *)NULL;
    if ((fp = fopen (param->plotpath, "w+")) == (FILE *)NULL) {
   
	param->errmsg[0] = '\0';
	sprintf (param->errmsg, "Failed to open tblpath %s\n", 
	    param->plotpath);
        
	sprintf (param->retstr, "[struct stat=error, msg=%s]\n", 
	    param->errmsg); 
        return (-1);
    }
    
/*
    Write header
*/
    strcpy (colname, "planenum");
    fprintf (fp, "|%-16s|", colname); 

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "plotxaxis= [%s] plotyaxis= [%s]\n", 
	    param->plotxaxis, param->plotyaxis);
        fflush (fp_debug);
    }         


    if ((int)strlen(param->plotxaxis) > 0) {
        sprintf (colname, "%s", param->plotxaxis);
    }
    else {
        strcpy (colname, "wave");
    } 
    fprintf (fp, "%-16s|", colname);
    
    if ((int)strlen(param->plotyaxis) > 0) {
        sprintf (colname, "%s", param->plotyaxis);
    }
    else {
        strcpy (colname, "flux");
    } 
    fprintf (fp, "%-16s|", colname);
    fprintf (fp, "\n"); 
    fflush (fp);

    strcpy (datatype, "int");
    fprintf (fp, "|%-16s|", datatype); 
    strcpy (datatype, "double");
    fprintf (fp, "%-16s|", datatype);
    strcpy (datatype, "double");
    fprintf (fp, "%-16s|", datatype);
    fprintf (fp, "\n"); 
    fflush (fp);

    strcpy (unit, "");
    fprintf (fp, "|%-16s|", unit); 
    strcpy (unit, hdr.cunit[2]);
    fprintf (fp, "%-16s|", unit);
    strcpy (unit, "dn");
    fprintf (fp, "%-16s|", unit);
    fprintf (fp, "\n"); 
    fflush (fp);

    strcpy (null, "null");
    fprintf (fp, "|%-16s|", null); 
    fprintf (fp, "%-16s|", null);
    fprintf (fp, "%-16s|", null);
    fprintf (fp, "\n"); 
    fflush (fp);

/*
    Write data 
*/
    for (i=0; i<hdr.nplane; i++) {

        sprintf (str, "%d", i);
        fprintf (fp, " %-16s ", str); 
    
        sprintf (str, "%16.2f", xarr[i]);
        fprintf (fp, "%-16s ", str); 
    
        sprintf (str, "%16.4f", yarr[i]);
        fprintf (fp, "%-16s ", str); 
    
        fprintf (fp, "\n"); 
        fflush (fp);
    }
        
    fclose (fp);        


/*
    Plot jsonfile
*/
    sprintf (param->plotjsonfile, "%s_plot.json", param->imageFile);
	
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "call checkFileExist (plotjsonfile)\n");
        fflush (fp_debug);  
    }

    fileExist = checkFileExist (param->plotjsonfile, rootname, suffix,
        param->directory, param->plotjsonpath);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "returned checkFileExist: fileExist= [%d]\n", 
	    fileExist);
	fprintf (fp_debug, "plotjsonfile= [%s]\n", param->plotjsonfile);
	fprintf (fp_debug, "plotjsonpath= [%s]\n", param->plotjsonpath);
	fflush (fp_debug);  
	
	fprintf (fp_debug, "plotwidth= [%d]\n", param->plotwidth);
	fprintf (fp_debug, "plotheight= [%d]\n", param->plotheight);
	fprintf (fp_debug, "plotbgcolor= [%s]\n", param->plotbgcolor);
	
	fprintf (fp_debug, "plothiststyle= [%d]\n", param->plothiststyle);
	fprintf (fp_debug, "plothistvalue= [%s]\n", param->plothistvalue);

	fprintf (fp_debug, "plotcolor= [%s]\n", param->plotcolor);
	fprintf (fp_debug, "plotlinecolor= [%s]\n", param->plotlinecolor);
	fprintf (fp_debug, "plotlinestyle= [%s]\n", param->plotlinestyle);
	

	fflush (fp_debug);  
    }
        

    if (!fileExist) {


/*
    Create default plot jsonfile
*/
        sprintf (param->plotjsonpath, "%s/%s", 
	    param->directory, param->plotjsonfile);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "plotjsonpaht= [%s]\n", param->plotjsonpath); 
	fflush (fp_debug);  
    }

        fp = (FILE *)NULL;
        if ((fp = fopen (param->plotjsonpath, "w+")) == (FILE *)NULL) {
   
	    param->errmsg[0] = '\0';
	    sprintf (param->errmsg, "Failed to open tblpath %s\n", 
	        param->plotjsonpath);
        
	    sprintf (param->retstr, "[struct stat=error, msg=%s]\n", 
	        param->errmsg); 
            return (-1);
        }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "start writint plotjsonfile: [%s]\n",
	    param->plotjsonpath);
	fflush (fp_debug);  
    }

/* 
    Create a default jsonfile; in this case, the following input parameters
    have to exist.
*/

         fprintf(fp, "{\n");
         
	 fprintf(fp, "   \"jpeg\" :\n");
         fprintf(fp, "   {\n");
         fprintf(fp, "      \"width\"  : %d,\n", param->plotwidth);
         fprintf(fp, "      \"height\" : %d,\n", param->plotheight);
         fprintf(fp, "      \"background\" : \"%s\"\n", 
	     param->plotbgcolor);
         fprintf(fp, "   },\n");
         fprintf(fp, "\n");

         if (param->plothiststyle) {
	     fprintf(fp, "   \"histogram\" : \"%s\",\n", param->plothistvalue);
             fprintf(fp, "\n");
         }

         fprintf(fp, "   \"axes\" :\n");
         fprintf(fp, "   {\n");
         fprintf(fp, "      \"colors\" :\n");
         fprintf(fp, "      {\n");
         fprintf(fp, "         \"axes\"   : \"black\",\n");
         fprintf(fp, "         \"labels\" : \"black\"\n");
         fprintf(fp, "      },\n");

         if(strlen(param->plottitle) > 0)
         {
            fprintf(fp, "\n");
            fprintf(fp, "      \"title\" : \"%s\",\n", param->plottitle);
         }

         fprintf(fp, "\n");
         fprintf(fp, "      \"xaxis\" :\n");
         fprintf(fp, "      {\n");
         
	 if ((int)strlen(param->plotxlabel) > 0) {
             fprintf(fp, "         \"label\"       : \"%s\",\n", 
	         param->plotxlabel);
         }
	 else {
             fprintf(fp, "         \"label\"       : \"%s\",\n", 
	         param->plotxaxis);
	 }

	 if ((int)strlen(param->plotxlabeloffset) > 0) {
             fprintf(fp, "         \"offsetlabel\"       : \"%s\",\n", 
	         param->plotxlabeloffset);
         }
	 else {
	     fprintf(fp, "         \"offsetlabel\" : \"false\",\n");
         } 
	 
	 fprintf(fp, "         \"autoscale\"   : true,\n");
         fprintf(fp, "         \"scaling\"     : \"linear\"\n");
         fprintf(fp, "      },\n");
         fprintf(fp, "\n");
         fprintf(fp, "      \"yaxis\" :\n");
         fprintf(fp, "      {\n");
         
	 if ((int)strlen(param->plotylabel) > 0) {
	     fprintf(fp, "         \"label\"       : \"%s\",\n", 
	         param->plotylabel);
         }
	 else {
             fprintf(fp, "         \"label\"       : \"%s\",\n", 
	         param->plotyaxis);
	 }
	 
	 if ((int)strlen(param->plotylabeloffset) > 0) {
             fprintf(fp, "         \"offsetlabel\"       : \"%s\",\n", 
	         param->plotxlabeloffset);
         }
	 else {
	     fprintf(fp, "         \"offsetlabel\" : \"false\",\n");
         } 
	 
         fprintf(fp, "         \"autoscale\"   : true,\n");
         fprintf(fp, "         \"scaling\"     : \"linear\"\n");
         fprintf(fp, "      }\n");
         fprintf(fp, "   },\n");
         fprintf(fp, "\n");
         fprintf(fp, "   \"pointset\" : \n");
         fprintf(fp, "   [\n");
         fprintf(fp, "      {\n");
         fprintf(fp, "         \"source\" :\n");
         fprintf(fp, "         {\n");
         fprintf(fp, "            \"table\" : \"%s\",\n", param->plotpath);
         fprintf(fp, "\n");


         fprintf(fp, "            \"xcolumn\" : \"%s\",\n", param->plotxaxis);
         fprintf(fp, "            \"ycolumn\" : \"%s\"\n", param->plotyaxis);
         fprintf(fp, "         },\n");
         fprintf(fp, "\n");
         fprintf(fp, "         \"points\" :\n");
         fprintf(fp, "         {\n");
         fprintf(fp, "            \"symbol\" : \"%s\",\n", param->plotsymbol);
         fprintf(fp, "            \"size\"   : 0.5,\n");
         fprintf(fp, "            \"color\"  : \"%s\"\n", param->plotcolor);
         fprintf(fp, "         },\n");
         fprintf(fp, "\n");
         fprintf(fp, "         \"lines\" :\n");
         fprintf(fp, "         {\n");
         fprintf(fp, "            \"style\" : \"%s\",\n", param->plotlinestyle);
         fprintf(fp, "            \"width\"   : 1.0,\n");
         fprintf(fp, "            \"color\"  : \"%s\"\n", param->plotlinecolor);
         fprintf(fp, "         }\n");
         fprintf(fp, "      }\n");
         fprintf(fp, "   ]\n");
         fprintf(fp, "}\n");

         fflush(fp);
         fclose(fp);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "done writing plotjsonpath= [%s]\n", 
	    param->plotjsonpath); 
	fflush (fp_debug);  
    }

    }

    return (0);
}

