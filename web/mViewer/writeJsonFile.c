/*
Theme: This routine writes a default JSON file for the viewer based on 
       the spec in the input parameter file
    
Input:  ViewerApp structure containing the input parameters.

Output: JSON file

Date: March 11, 2015 (Mihseh Kong)
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

#include "fitshdr.h"
#include "viewerapp.h"


int  str2Integer (char *str, int *intval, char *errmsg);
int  str2Double (char *str, double *dblval, char *errmsg);
    
extern FILE *fp_debug;


int ndefaultcolor = 24;;
static char defaultcolor[][30] = {
    "black",
    "white",
    "grey",
    "lightgrey",
    "darkgrey",
    "green",
    "darkgreen",
    "lightgreen",
    "yellow",
    "lemonchiffon",
    "orange",
    "red",
    "deepred",
    "pink",
    "hotpink",
    "magenta",
    "magenta4",
    "purple",
    "cyan",
    "blue",
    "aliceblue",
    "cornflowerblue",
    "dodgerblue",
    "lightskyblue"
};
    
static char defaultHexcolor[][30] = {
    "#000000",
    "#FFFFFF",
    "#7F7F7F",
    "#D3D3D3",
    "#474747",
    "#00FF00",
    "#008800",
    "#09F911",
    "#FFFF00",
    "#FF7F00",
    "#FFFACD",
    "#FF0000",
    "#8b0000",
    "#FFC0CB",
    "#FF69B4",
    "#FF00FF",
    "#8B008B",
    "#7D26CD",
    "#00FFFF",
    "#0000FF",
    "#F0F8FF",
    "#6495ED",
    "#104E8B",
    "#87CEFA"
};


int ncolortbl = 9;
static char colortbl[][30] = {
    "greyscale",
    "reversegreyscale",
    "thermal",
    "reversethermal",
    "logarithmicthermal",
    "velocity",
    "redramp",
    "greenramp",
    "blueramp"
};


int ncolors_symbol = 18;
static char defaultSymbolcolor[][30] = {
	
    "darkgreen",
    "deepred",
    "cornflowerblue",
    "magenta",
    "cyan",
    "yellow",
    "hotpink",
    "green",
    "red",
    "aliceblue",
    "magenta4",
    "lightgreen",
    "orange",
    "pink",
    "blue",
    "lemonchiffon",
    "dodgerblue",
    "lightskyblue",
    "purple"
};

int color2hexcolor (char *color, char *hexval, int ncolors) 
{
    char  hexcolor[30]; 
    int   indx;
    int   l;

    int   debugfile = 1;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "From color2hexcolor: color= [%s]\n", color);
	fflush (fp_debug);
    }

    if (color[0] == '#') {

        strcpy (hexval, &color[1]);
        return (0);
    }

                   
    indx = -1;
    for (l=0; l<ndefaultcolor; l++) {
                        
        if (strcasecmp (color, defaultcolor[l]) == 0) {
                            
            indx = l;
            break;
        }
    }

    if (indx == -1)
	return (-1);
   
    strcpy (hexcolor, defaultHexcolor[indx]);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor);
	fflush (fp_debug);
    }

    strcpy (hexval, &hexcolor[1]);
    
    return (0);
}




int writeJsonFile (struct ViewerApp *param)
{
    FILE   *fp;

    char   str[40]; 
    char   hexcolor[40];
    int    istatus;
    int    ntbl;
    int    noverlay;
    int    indx;
    int    l;

    int    debugfile = 1;


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "\nEnter writeJsonFile: jsonpath= [%s]\n", 
	    param->jsonpath);
        fflush (fp_debug);
    }

        
    fp = (FILE *)NULL;
    if ((fp = fopen (param->jsonpath, "w+")) == (FILE *)NULL) {
   
	param->errmsg[0] = '\0';
	sprintf (param->errmsg, "Failed to open jsonpath %s for write\n", 
	    param->jsonpath);
        
        return (-1);
    }

    fprintf(fp, "{\n");
  
/*
    top level parameters: imageFile, imageType, canvas sizes etc. 
*/
/*
    if ((strlen(param->imname) == 0) {
       
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "imname= [%s]\n", param->imname);
	    fprintf (fp_debug, "grayfile= [%s]\n", param->grayfile);
            fflush (fp_debug);
        }

        strcpy (str, param->grayfile);

        cptr = (char *)NULL;
	cptr = strrchr (str, '.');

	if (cptr != (char *)NULL) {
            
	    *cptr = '\0';
	}
	    
	strcpy (param->imname, str);
    
    }
*/

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "imname= [%s]\n", param->imname);
	fprintf (fp_debug, "outimtype= [%s]\n", param->outimtype);
	fprintf (fp_debug, "canvaswidth= [%d] canvasheight= [%d]\n", 
	    param->canvaswidth, param->canvasheight);
	fprintf (fp_debug, "objname= [%s] pixscale= [%s] filter= [%s]\n", 
	    param->objname, param->pixscale, param->filter);
        fflush (fp_debug);
    }

    if ((int)strlen(param->outimtype) == 0) 
        strcpy (param->outimtype, "jpeg");
    
    fprintf(fp, "  \"imageFile\": \"%s\",\n", param->imname);
    fprintf(fp, "  \"imageType\": \"%s\",\n", param->outimtype);
    fprintf(fp, "  \"canvasWidth\": \"%d\",\n", param->canvaswidth);
    fprintf(fp, "  \"canvasHeight\": \"%d\",\n", param->canvasheight);
   
    if ((param->refwidth > 0) && (param->refheight > 0)) {
        fprintf(fp, "  \"refWidth\": \"%d\",\n", param->refwidth);
        fprintf(fp, "  \"refHeight\": \"%d\",\n", param->refheight);
    }

    fprintf(fp, "  \"nowcs\": \"%d\",\n", param->nowcs);

    if ((int)strlen(param->imcursormode) > 0) {
	fprintf(fp, "  \"imcursorMode\": \"%s\",\n", param->imcursormode);
    }

    if ((int)strlen(param->objname) > 0) {
        fprintf(fp, "  \"objName\": \"%s\",\n", param->objname);
    }

    if ((int)strlen(param->filter) > 0) {
        fprintf(fp, "  \"filter\": \"%s\",\n", param->filter);
    }
    
    if ((int)strlen(param->pixscale) > 0) {
        fprintf(fp, "  \"pixScale\": \"%s\",\n", param->pixscale);
    }

    if (param->isimcube) {
        
/*
    imcubeFile
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "imcubefile= [%s]\n", param->imcubefile);
            fflush (fp_debug);
        }

	fprintf(fp, "  \"imcubeFile\":\n");
        fprintf(fp, "  {\n");
	
	fprintf(fp, "    \"fitsFile\": \"%s\",\n", param->imcubepath);

/*
    Compute centerPlane from input startplane and nplaneave 
*/
        param->endplane = param->startplane + param->nplaneave;
	param->centerplane = param->startplane + param->nplaneave/2;
        param->endplane = param->centerplane + param->nplaneave/2;

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "waveplottype= [%s]\n", param->waveplottype);
	    fprintf (fp_debug, "imcubemode= [%s]\n", param->imcubemode);
	    fprintf (fp_debug, "imcursormode= [%s]\n", param->imcursormode);
	    fprintf (fp_debug, "nfitsplane= [%d]\n", param->nfitsplane);
	    fprintf (fp_debug, "nplaneave= [%d]\n", param->nplaneave);
	    fprintf (fp_debug, "startplane= [%d]\n", param->startplane);
	    fprintf (fp_debug, "endplane= [%d]\n", param->endplane);
	    fprintf (fp_debug, "centerplane= [%d]\n", param->centerplane);
	    fflush (fp_debug);
        }

	fprintf(fp, "    \"imcubeMode\": \"%s\",\n", param->imcubemode);
	
	fprintf(fp, "    \"planeNum\": \"%d\",\n", param->nfitsplane);
	fprintf(fp, "    \"nplaneAve\": \"%d\",\n", param->nplaneave);
	
	if ((int)strlen(param->waveplottype) > 0) {
	    
	    fprintf(fp, "    \"centerPlane\": \"%d\",\n", param->centerplane);

            strcpy (str, "");
	    fprintf(fp, "    \"waveplotType\": \"%s\",\n", param->waveplottype);
	    fprintf(fp, "    \"plotjsonFile\": \"%s\",\n", param->plotjsonpath);
	    
	    fprintf(fp, "    \"plotXaxis\": \"%s\",\n", param->plotxaxis);
	    fprintf(fp, "    \"plotYaxis\": \"%s\"\n", param->plotyaxis);
	}
	else {
	    fprintf(fp, "    \"centerPlane\": \"%d\"\n", param->centerplane);
        }
	

/*
	fprintf(fp, "    \"ctype3\": \"%s\",\n", param->ctype3);
	fprintf(fp, "    \"crval3\": \"%lf\",\n", param->crval3);
	fprintf(fp, "    \"cdelt3\": \"%lf\"\n", param->cdelt3);
*/

/*
	fprintf(fp, "    \"startPlane\": \"%d\",\n", param->startplane);
	fprintf(fp, "    \"endPlane\": \"%d\",\n", param->endplane);
*/

	fprintf(fp, "   }");

    }

    if (!param->iscolor) {

        if (param->isimcube) {
	    
	    fprintf(fp, "    ,\n");
	
	}
        
/*
    grayFile
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "grayfile= [%s]\n", param->grayfile);
            fflush (fp_debug);
        }

	fprintf(fp, "  \"grayFile\":\n");
        fprintf(fp, "  {\n");
        
	fprintf(fp, "    \"fitsFile\": \"%s\",\n", param->grayfile);
      
/*
    if colortbl not found, default to grayscale
*/
        indx = -1;
	for (l=0; l<ncolortbl; l++) {

	    if (strcasecmp (param->colortbl, colortbl[l]) == 0) {
	        indx = l;
		break;
	    }
	}

	if (indx == -1)
	    indx = 0;

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "colortbl= [%s]\n", param->colortbl);
	    fprintf (fp_debug, "indx= [%d]\n", indx);
            fflush (fp_debug);
        }

	fprintf(fp, "    \"colorTable\": \"%d\",\n", indx);
	fprintf(fp, "    \"stretchMin\": \"%s\",\n", param->minstretch);
	fprintf(fp, "    \"stretchMax\": \"%s\",\n", param->maxstretch);
	fprintf(fp, "    \"stretchMode\": \"%s\"\n", param->stretchmode);
  
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "ngrid= [%d]\n", param->ngrid);
	    fprintf (fp_debug, "nlabel= [%d]\n", param->nlabel);
	    fprintf (fp_debug, "nmarker= [%d]\n", param->nmarker);
	    fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
	    fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
	    fflush (fp_debug);
        }

	fprintf(fp, "   }");

        if ((param->ngrid == 0) &&
            (param->nlabel == 0) &&
            (param->nsrctbl == 0) &&
            (param->niminfo == 0) &&
            (param->nmarker == 0)) {
            
	    fprintf(fp, "\n");
	    fprintf(fp, "}\n");
            
	    fflush (fp);
	    fclose(fp);

	    return (0);
	}
	
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "here1\n");
	    fflush (fp_debug);
        }

/*
    overlay:  
*/
	fprintf(fp, ",\n");
	
	fprintf(fp, "  \"overlay\":\n");
        fprintf(fp, "  [\n");
       
        noverlay = 0;

/*
    overlay: grid 
*/
        if (param->ngrid > 0) {
       
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, "here2\n");
	        fflush (fp_debug);
            }

            for (l=0; l<param->ngrid; l++) {

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "here2-0\n");
	            fflush (fp_debug);
                }

		if (noverlay > 0) {
		    fprintf(fp, ",\n");
		}

		noverlay++;

                if (param->gridcsys[l] == (char *)NULL) {
	            param->gridcsys[l] 
		        = (char *)malloc(40*sizeof(char));
                    param->gridcsys[l][0] = '\0';
                
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

		        fprintf (fp_debug, "here2-1: malloc gridcsys[%d]\n", l);
		        fflush (fp_debug);
		    }

		}
                if (param->gridcolor[l] == (char *)NULL) {
	            param->gridcolor[l] 
		        = (char *)malloc(40*sizeof(char));
                    param->gridcolor[l][0] = '\0';
		    
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

		        fprintf (fp_debug, 
			    "here2-2: malloc gridcolor[%d]\n", l);
		        fflush (fp_debug);
		    }

		}


                if ((int)strlen(param->gridcsys[l]) == 0) {
		    
		    strcpy (param->gridcsys[l], "eq j2000");
		    
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

		        fprintf (fp_debug, "gridcsys[%d] null, set to [%s]\n", 
			    l, param->gridcsys[l]);
		        fflush (fp_debug);
		    }

                }

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "here3: gridcsys= [%s]\n",
		        param->gridcsys[l]);
	            fflush (fp_debug);
                }

		fprintf(fp, "    {\n");
		fprintf(fp, "      \"type\": \"grid\",\n");
		fprintf(fp, "      \"coordSys\": \"%s\",\n", 
		    param->gridcsys[l]);
		
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "l= [%d] gridcolor= [%s]\n", 
		        l, param->gridcolor[l]);
	            fflush (fp_debug);
                }
               
		    istatus = color2hexcolor (param->gridcolor[l], hexcolor, 
		        ndefaultcolor);
	        
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	                fprintf (fp_debug, 
		            "returned color2hexcolor, istatus= [%d]\n", 
			    istatus); 
	                fflush (fp_debug);
                    }


/*
    default: darkgrey 
*/
		    if (istatus == -1) {
                        strcpy (hexcolor, &defaultHexcolor[4][1]);
                    }
		
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	            fflush (fp_debug);
                }

		fprintf(fp, "      \"color\": \"%s\",\n", hexcolor);

	        
		if (param->gridvis[l]) {
		    fprintf(fp, "      \"visible\": \"true\"\n");
                }
		else {
		    fprintf(fp, "      \"visible\": \"false\"\n");
                }
	        fprintf(fp, "     }");
            }
	}

/*
    overlay: marker (compass) 
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "noverlay= [%d]\n", noverlay); 
	    fflush (fp_debug);
        }


        if (param->nmarker > 0) {

            for (l=0; l<param->nmarker; l++) {
	        
		if (noverlay > 0) {
		    fprintf(fp, ",\n");
		}
		noverlay++;
                
		if (param->markertype[l] == (char *)NULL) {
	            param->markertype[l] 
		        = (char *)malloc(40*sizeof(char));
                    param->markertype[l][0] = '\0';
		}
		if (param->markercolor[l] == (char *)NULL) {
	            param->markercolor[l] 
		        = (char *)malloc(40*sizeof(char));
                    param->markercolor[l][0] = '\0';
		}
		if (param->markerlocstr[l] == (char *)NULL) {
	            param->markerlocstr[l] 
		        = (char *)malloc(40*sizeof(char));
                    param->markerlocstr[l][0] = '\0';
		}


		fprintf(fp, "    {\n");
		fprintf(fp, "      \"type\": \"mark\",\n");
		fprintf(fp, "      \"symType\": \"%s\",\n", 
		    param->markertype[l]);
	
	        if ((int)strlen(param->markersize[l]) == 0) {
                    strcpy (param->markersize[l], "60.0p");
		}

		fprintf(fp, "      \"symSize\": \"%s\",\n", 
		    param->markersize[l]);
		
		fprintf(fp, "      \"location\": \"%s\",\n", 
		    param->markerlocstr[l]);
		
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "l= [%d] markercolor= [%s]\n", 
		        l, param->markercolor[l]);
	            fflush (fp_debug);
                }
                
		    istatus = color2hexcolor (param->markercolor[l], hexcolor, 
		        ndefaultcolor);
	        
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	                fprintf (fp_debug, 
		            "returned color2hexcolor, istatus= [%d]\n", 
			    istatus); 
	                fflush (fp_debug);
                    }

/*
    default: red 
*/
		    if (istatus == -1) {
                        strcpy (hexcolor, "880000");
                    }

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	            fflush (fp_debug);
                }

		fprintf(fp, "      \"color\": \"%s\",\n", hexcolor);

	        
		if (param->markervis[l]) {
		    fprintf(fp, "      \"visible\": \"true\"\n");
                }
		else {
		    fprintf(fp, "      \"visible\": \"false\"\n");
                }
	        fprintf(fp, "     }");
            }

        }

/*
    overlay: catalog 
*/
        ntbl = 0;
        if (param->nsrctbl) {

            for (l=0; l<param->nsrctbl; l++) {
	        
		if (noverlay > 0) {
		    fprintf(fp, "     ,\n");
		}
		noverlay++;


		fprintf(fp, "    {\n");
		fprintf(fp, "      \"type\": \"catalog\",\n");
		
		fprintf(fp, "      \"dataFile\": \"%s\",\n", 
		    param->srctblpath[l]);
	

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "l= [%d] srctblcolor= [%s]\n", 
		        l, param->srctblcolor[l]);
	            fflush (fp_debug);
                }

		    istatus = color2hexcolor (defaultSymbolcolor[ntbl], 
		        hexcolor, ndefaultcolor);
		    ntbl++;
		
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	                fprintf (fp_debug, 
		            "returned color2hexcolor, istatus= [%d]\n", 
			    istatus); 
	                fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	                fflush (fp_debug);
                    }

                strcpy (param->srctblcolor[l], hexcolor);
                
		fprintf(fp, "      \"color\": \"%s\",\n", 
		    param->srctblcolor[l]);

		fprintf(fp, "      \"symType\": \"%s\",\n", 
		    param->tblsymtype[l]);
		
		fprintf(fp, "      \"symSize\": \"%lf\",\n", 
		    param->tblsymsize[l]);
	
                if ((int)strlen(param->datacol[l]) > 0) {

		    fprintf(fp, "      \"dataCol\": \"%s\",\n", 
		        param->datacol[l]);
		    fprintf(fp, "      \"dataRef\": \"%lf\",\n", 
		        param->dataref[l]);
		    fprintf(fp, "      \"dataType\": \"%s\",\n", 
		        param->datafluxmode[l]);
		}

	        
		if (param->srctblvis[l]) {
		    fprintf(fp, "      \"visible\": \"true\"\n");
                }
		else {
		    fprintf(fp, "      \"visible\": \"false\"\n");
                }
	        fprintf(fp, "     }");
            }


        }

/*
    overlay: iminfo  
*/
        if (param->niminfo) {

            for (l=0; l<param->niminfo; l++) {
	        
		if (noverlay > 0) {
		    fprintf(fp, "     ,\n");
		}
		noverlay++;

		fprintf(fp, "    {\n");
		fprintf(fp, "      \"type\": \"iminfo\",\n");
		
		fprintf(fp, "      \"dataFile\": \"%s\",\n", 
		    param->iminfopath[l]);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "l= [%d] iminfocolor= [%s]\n", 
		        l, param->iminfocolor[l]);
	            fflush (fp_debug);
                }

		    istatus = color2hexcolor (defaultSymbolcolor[ntbl], 
		        hexcolor, ndefaultcolor);
		    ntbl++;
		
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	                fprintf (fp_debug, 
			    "returned color2hexcolor, istatus= [%d]\n", 
			    istatus); 
	                fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	                fflush (fp_debug);
                    }

                strcpy (param->iminfocolor[l], hexcolor);

		fprintf(fp, "      \"color\": \"%s\",\n", 
		    param->iminfocolor[l]);

		if (param->iminfovis[l]) {
		    fprintf(fp, "      \"visible\": \"true\"\n");
                }
		else {
		    fprintf(fp, "      \"visible\": \"false\"\n");
                }
	        fprintf(fp, "     }");
            }

        }

/*
    overlay: label  
*/
        if (param->nlabel) {

            for (l=0; l<param->nlabel; l++) {
	        
		if (l > 0) {
		    fprintf(fp, "     ,\n");
		}

		fprintf(fp, "    {\n");
		fprintf(fp, "      \"type\": \"label\",\n");
	

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "l= [%d] labelcolor= [%s]\n", 
		        l, param->labelcolor[l]);
	            fflush (fp_debug);
                }
                
		    istatus = color2hexcolor (param->labelcolor[l], hexcolor, 
		        ndefaultcolor);
	        
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	                fprintf (fp_debug, 
		            "returned color2hexcolor, istatus= [%d]\n", 
			    istatus); 
	                fflush (fp_debug);
                    }

/*
    default: darkgrey 
*/
		    if (istatus == -1) {
                        strcpy (hexcolor, &defaultHexcolor[4][1]);
                    }
		
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	            fflush (fp_debug);
                }

		fprintf(fp, "      \"color\": \"%s\",\n", hexcolor);

		fprintf(fp, "      \"location\": \"%s\",\n", 
		    param->labellocstr[l]);

		fprintf(fp, "      \"text\": \"%s\",\n", 
		    param->labeltext[l]);

		if (param->labelvis[l]) {
		    fprintf(fp, "      \"visible\": \"true\"\n");
                }
		else {
		    fprintf(fp, "      \"visible\": \"false\"\n");
                }
	        fprintf(fp, "     }");
            }


        }


    }
    else {

    }

    fprintf(fp, "\n");
    fprintf(fp, "   ]\n");
    fprintf(fp, "}");


    fflush (fp);
    fclose(fp);
    

    return (0);
}




int writePlotJson (struct ViewerApp *param)
{
    FILE   *fp;

    char   str[40];
    char   hexcolor[40];
    int    istatus;

    int    debugfile = 1;


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "\nEnter writePlotJson: plotjsonpath= [%s]\n", 
	    param->plotjsonpath);
        fflush (fp_debug);
    }
        
    fp = (FILE *)NULL;
    if ((fp = fopen (param->plotjsonpath, "w+")) == (FILE *)NULL) {
   
	param->errmsg[0] = '\0';
	sprintf (param->errmsg, "Failed to open plotjsonpath %s for write\n", 
	    param->plotjsonpath);
        
        return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "plottitle= [%s]\n", param->plottitle);
	fprintf (fp_debug, "plotwidth= [%d] plotheight= [%d]\n", 
	    param->plotwidth, param->plotheight);
	fprintf (fp_debug, "plotxaxis= [%s] plotyaxis= [%s]\n", 
	    param->plotxaxis, param->plotyaxis);
        fflush (fp_debug);
    }

/* 
    Create a default jsonfile; in this case, the following input parameters
    have to exist.
*/
    fprintf (fp, "{\n");
         
    fprintf (fp, "   \"jpeg\" :\n");
    fprintf (fp, "   {\n");
    fprintf (fp, "      \"width\"  : %d,\n", param->plotwidth);
    fprintf (fp, "      \"height\" : %d,\n", param->plotheight);


    istatus = color2hexcolor (param->plotbgcolor, hexcolor, ndefaultcolor);
    
    fprintf (fp, "      \"background\" : \"%s\"\n", hexcolor);
    
    fprintf (fp, "   },\n");
    fprintf (fp, "\n");

    if (param->plothiststyle) {
	fprintf (fp, "   \"histogram\" : \"%s\",\n", param->plothistvalue);
        fprintf (fp, "\n");
    }

    fprintf (fp, "   \"axes\" :\n");
    fprintf (fp, "   {\n");
    fprintf (fp, "      \"colors\" :\n");
    fprintf (fp, "      {\n");
    fprintf (fp, "         \"axes\"   : \"black\",\n");
    fprintf (fp, "         \"labels\" : \"black\"\n");
    fprintf (fp, "      },\n");

    if (strlen(param->plottitle) > 0)
    {
        fprintf (fp, "\n");
        fprintf (fp, "      \"title\" : \"%s\",\n", param->plottitle);
    }

    fprintf (fp, "\n");
    fprintf (fp, "      \"xaxis\" :\n");
    fprintf (fp, "      {\n");
         
    if ((int)strlen(param->plotxlabel) > 0) {
        fprintf (fp, "         \"label\"       : \"%s\",\n", param->plotxlabel);
    }
    else {
        fprintf (fp, "         \"label\"       : \"%s\",\n", param->plotxaxis);
    }

    if ((int)strlen(param->plotxlabeloffset) > 0) {
        fprintf (fp, "         \"offsetlabel\"       : \"%s\",\n", 
            param->plotxlabeloffset);
    }
    else {
        fprintf (fp, "         \"offsetlabel\" : \"false\",\n");
    } 
	 
    fprintf (fp, "         \"autoscale\"   : true,\n");
    fprintf (fp, "         \"scaling\"     : \"linear\"\n");
    fprintf (fp, "      },\n");
    fprintf (fp, "\n");
    fprintf (fp, "      \"yaxis\" :\n");
    fprintf (fp, "      {\n");
         
    if ((int)strlen(param->plotylabel) > 0) {
        fprintf (fp, "         \"label\"       : \"%s\",\n", param->plotylabel);
    }
    else {
        fprintf (fp, "         \"label\"       : \"%s\",\n", param->plotyaxis);
    }
	 
    if ((int)strlen(param->plotxlabeloffset) > 0) {
        fprintf (fp, "         \"offsetlabel\"       : \"%s\",\n", 
        param->plotxlabeloffset);
    }
    else {
        fprintf (fp, "         \"offsetlabel\" : \"false\",\n");
    } 
	 
    fprintf (fp, "         \"autoscale\"   : true,\n");
    fprintf (fp, "         \"scaling\"     : \"linear\"\n");
    fprintf (fp, "      }\n");
    fprintf (fp, "   },\n");
    fprintf (fp, "\n");
    fprintf (fp, "   \"pointset\" : \n");
    fprintf (fp, "   [\n");
    fprintf (fp, "      {\n");
   
    if ((int)strlen(param->waveplotfile) == 0) {
        sprintf (param->waveplotfile, "%s_plot.tbl", param->imname);
    }
    sprintf (param->waveplotpath, "%s/%s", 
        param->directory, param->waveplotfile);

    fprintf (fp, "         \"source\" :\n");
    fprintf (fp, "         {\n");
    fprintf (fp, "            \"table\" : \"%s\",\n", param->waveplotpath);
    fprintf (fp, "\n");
    fprintf (fp, "            \"xcolumn\" : \"%s\",\n", param->plotxaxis);
    fprintf (fp, "            \"ycolumn\" : \"%s\"\n", param->plotyaxis);
    fprintf (fp, "         },\n");
    fprintf (fp, "\n");
    
    fprintf (fp, "         \"points\" :\n");
    fprintf (fp, "         {\n");
    fprintf (fp, "            \"symbol\" : \"%s\",\n", param->plotsymbol);
    fprintf (fp, "            \"size\"   : 0.2,\n");
    
    istatus = color2hexcolor (param->plotcolor, hexcolor, ndefaultcolor);
    
    fprintf (fp, "            \"color\"  : \"%s\"\n", hexcolor);
    fprintf (fp, "         },\n");

    fprintf (fp, "         \"lines\" :\n");
    fprintf (fp, "         {\n");
    fprintf (fp, "            \"style\" : \"%s\",\n", param->plotlinestyle);
    fprintf (fp, "            \"width\"   : 1.0,\n");
    
    istatus = color2hexcolor (param->plotlinecolor, hexcolor, ndefaultcolor);
    
    fprintf (fp, "            \"color\"  : \"%s\"\n", hexcolor);
    fprintf (fp, "         }\n");
    
    fprintf (fp, "      }\n");
    fprintf (fp, "   ]\n");
    fprintf (fp, "}\n");

    fflush (fp);
    fclose(fp);
    

    return (0);
}








