/*
Theme: This routine writes the return json structure.

Written: June 24, 2015 (Mihseh Kong)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "mviewer.h"


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



int  str2Integer (char *str, int *intval, char *errmsg);
int  str2Double (char *str, double *dblval, char *errmsg);
int  lower (char *str);
    
int getColortblIndx (char *colorstr);

    
extern FILE *fp_debug;


int color2hexcolor (char *color, char *hexval, int ncolors) 
{
    char  hexcolor[30];
    char  str[40]; 
    char  colorlowercase[30];
    
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

    strcpy (colorlowercase, color);
    lower (colorlowercase);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "colorlowercase= [%s]\n", colorlowercase);
	fflush (fp_debug);
    }

    indx = -1;
    for (l=0; l<ndefaultcolor; l++) {
        
	strcpy (str, defaultcolor[l]);
	lower (str);
	strcpy (hexcolor, &defaultHexcolor[l][1]);
	lower (hexcolor);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

            fprintf (fp_debug, "l= [%d] defaultcolor: str= [%s]\n", l, str);
            fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor);
	    fflush (fp_debug);
        }

	if ((strcasecmp (color, str) == 0) ||
            (strcasecmp (color, hexcolor) == 0))
	{
            indx = l;
            break;
        }
    }

    if (indx == -1)
	return (-1);
  
    strcpy (hexval, &defaultHexcolor[indx][1]);
    lower (hexval);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

        fprintf (fp_debug, "hexval= [%s]\n", hexval);
	fflush (fp_debug);
    }

    return (0);
}



int constructRetjson (struct Mviewer *param)
{
    char   retstr[4096];
    char   str[1024];

    char  layerfilename[40];
    
    char  layertype[40];
    char  layercolor[40];
    char  layervis[40];
    char  layercsys[40];
    
    char  hexcolor[40];

    char  symtype[40];
    char  symsize[40];
    char  symside[40];
    char  datatype[40];
    char  dataref[40];
    char  datacol[40];
    
    char  location[200];

    int  l;    
    int  indx;
    int  istatus;


    int  debugfile = 1;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "\nFrom constructRetjson\n");
	fflush (fp_debug);
    }

    sprintf (retstr, "{\n");
  
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "imageFile= [%s]\n", param->imageFile);
	fprintf (fp_debug, "imageType= [%s]\n", param->imageType);
	fprintf (fp_debug, "nowcs= [%d]\n", param->nowcs);
	fprintf (fp_debug, "canvasWidth= [%d] canvasHeight= [%d]\n", 
	    param->canvasWidth, param->canvasHeight);
	fprintf (fp_debug, "cmd= [%s]\n", param->cmd); 
	fprintf (fp_debug, "baseurl= [%s]\n", param->baseURL); 
	fprintf (fp_debug, "workspace= [%s]\n", param->workspace); 
	fprintf (fp_debug, "nim= [%d]\n", param->nim); 
	fprintf (fp_debug, "jpgfile= [%s]\n", param->jpgfile); 
	fprintf (fp_debug, "refjpgfile= [%s]\n", param->refjpgfile); 
        
	fflush (fp_debug);
    }

    if ((int)strlen(param->imageType) == 0) 
        strcpy (param->imageType, "jpeg");
   
    sprintf (str, "  \"file\": \"%s/%s\",\n", 
        param->baseURL, param->jpgfile);
    strcat (retstr, str);
   
    if (param->nim == 2) {
        sprintf (str, "  \"reffile\": \"%s/%s\",\n", 
            param->baseURL, param->refjpgfile);
        strcat (retstr, str);
    }
    
    sprintf (str, "  \"imageFile\": \"%s\",\n", param->imageFile);
    strcat (retstr, str);
    sprintf (str, "  \"imageType\": \"%s\",\n", param->imageType);
    strcat (retstr, str);

    sprintf (str, "  \"canvasWidth\": \"%d\",\n", param->canvasWidth);
    strcat (retstr, str);
    sprintf (str, "  \"canvasHeight\": \"%d\",\n", param->canvasHeight);
    strcat (retstr, str);
   
    if ((param->refWidth > 0) && (param->refHeight > 0)) {

        sprintf (str, "  \"refWidth\": \"%d\",\n", param->refWidth);
        strcat (retstr, str);
        sprintf (str, "  \"refHeight\": \"%d\",\n", param->refHeight);
        strcat (retstr, str);
    }

    
    sprintf (str, "  \"imageWidth\": \"%d\",\n", param->imageWidth);
    strcat (retstr, str);
    sprintf (str, "  \"imageHeight\": \"%d\",\n", param->imageHeight);
    strcat (retstr, str);
    
    sprintf (str, "  \"cutoutWidth\": \"%d\",\n", param->ns);
    strcat (retstr, str);
    sprintf (str, "  \"cutoutHeight\": \"%d\",\n", param->nl);
    strcat (retstr, str);
    sprintf (str, "  \"ss\": \"%.1f\",\n", param->ss);
    strcat (retstr, str);
    sprintf (str, "  \"sl\": \"%.1f\",\n", param->sl);
    strcat (retstr, str);
    
    
    sprintf (str, "  \"nowcs\": \"%d\",\n", param->nowcs);
    strcat (retstr, str);

    if (!param->nowcs) {
        sprintf (str, "  \"imCsys\": \"%s\",\n", param->imcsys);
        strcat (retstr, str);
    }
    
    sprintf (str, "  \"scale\": \"%.4f\",\n", param->zoomfactor);
    strcat (retstr, str);

/*
    subimage boundary on the original fits image
*/
    sprintf (str, "  \"xmin\": \"%.1f\",\n", param->xmin);
    strcat (retstr, str);
    sprintf (str, "  \"xmax\": \"%.1f\",\n", param->xmax);
    strcat (retstr, str);
    sprintf (str, "  \"ymin\": \"%.1f\",\n", param->ymin);
    strcat (retstr, str);
    sprintf (str, "  \"ymax\": \"%.1f\",\n", param->ymax);
    strcat (retstr, str);

    sprintf(str, "  \"objName\": \"%s\",\n", param->objname);
    strcat (retstr, str);
    sprintf(str, "  \"filter\": \"%s\",\n", param->filter);
    strcat (retstr, str);
    sprintf(str, "  \"pixScale\": \"%s\",\n", param->pixscale);
    strcat (retstr, str);

    if ((int)strlen(param->imcursormode) > 0) {
	sprintf(str, "  \"imcursorMode\": \"%s\",\n", param->imcursormode);
        strcat (retstr, str);
    }	
       
/*
    click/box input parameters
*/
    sprintf (str, "  \"xs\": \"%.1f\",\n", param->xs);
    strcat (retstr, str);
    sprintf (str, "  \"xe\": \"%.1f\",\n", param->xe);
    strcat (retstr, str);
    sprintf (str, "  \"ys\": \"%.1f\",\n", param->ys);
    strcat (retstr, str);
    sprintf (str, "  \"ye\": \"%.1f\",\n", param->ye);
    strcat (retstr, str);

/*
    click/box result parameters
*/
    sprintf (str, "  \"pickValue\": \"%lf\",\n", param->pickval);
    strcat (retstr, str);
    sprintf (str, "  \"pickCsys\": \"%s\",\n", param->pickcsys);
    strcat (retstr, str);
	
    sprintf (str, "  \"xPick\": \"%d\",\n", param->xpick);
    strcat (retstr, str);
    sprintf (str, "  \"yPick\": \"%d\",\n", param->ypick);
    strcat (retstr, str);

    sprintf (str, "  \"raPick\": \"%lf\",\n", param->rapick);
    strcat (retstr, str);
    sprintf (str, "  \"decPick\": \"%lf\",\n", param->decpick);
    strcat (retstr, str);

    sprintf (str, "  \"sexraPick\": \"%s\",\n", param->sexrapick);
    strcat (retstr, str);
    sprintf (str, "  \"sexdecPick\": \"%s\",\n", param->sexdecpick);
    strcat (retstr, str);


    if (param->isimcube) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "imcubepath= [%s]\n", param->imcubepath);
            fflush (fp_debug);
        }

	sprintf(str, "  \"imcubeFile\":\n");
        strcat (retstr, str);
            
	sprintf(str, "  {\n");
        strcat (retstr, str);

        if ((int)strlen(param->imcubepathOrig) > 0)  {
	    sprintf(str, "    \"fitsFileOrig\": \"%s\",\n", 
		param->imcubepathOrig);
            strcat (retstr, str);
	}

	sprintf(str, "    \"fitsFile\": \"%s\",\n", param->imcubepath);
        strcat (retstr, str);
	
	sprintf(str, "    \"imcubeMode\": \"%s\",\n", param->imcubemode);
        strcat (retstr, str);
	
	sprintf(str, "    \"planeNum\": \"%d\",\n", param->nfitsplane);
        strcat (retstr, str);
	sprintf(str, "    \"nplaneAve\": \"%d\",\n", param->nplaneave);
        strcat (retstr, str);
	sprintf(str, "    \"centerPlane\": \"%d\",\n", param->centerplane);
        strcat (retstr, str);
	
	sprintf (str, "    \"ctype3\": \"%s\",\n", param->ctype3);
        strcat (retstr, str);
	sprintf (str, "    \"crval3\": \"%lf\",\n", param->crval3);
        strcat (retstr, str);
	    
	if ((int)strlen(param->waveplottype) > 0) {
	        
	    sprintf (str, "    \"cdelt3\": \"%lf\",\n", param->cdelt3);
            strcat (retstr, str);
	        
	    sprintf(str, "    \"waveplotType\": \"%s\",\n", 
		param->waveplottype);
            strcat (retstr, str);

	    sprintf (str, "    \"plotjsonFile\": \"%s\",\n", 
		param->plotjsonfile);
            strcat (retstr, str);
	        
	    sprintf (str, "    \"plotFile\": \"%s\"\n", 
		param->plotpath);
            strcat (retstr, str);

        }
	else { 
	    sprintf (str, "    \"cdelt3\": \"%lf\"\n", param->cdelt3);
            strcat (retstr, str);
        }

	sprintf (str, "   },\n");
        strcat (retstr, str);
    }

    
    
    if (!param->iscolor) {

/*
    grayFile
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "grayfile= [%s]\n", param->grayFile);
            fflush (fp_debug);
        }

	sprintf (str, "  \"grayFile\":\n");
        strcat (retstr, str);
    
        sprintf (str, "  {\n");
        strcat (retstr, str);
        
	sprintf (str, "    \"fitsFile\": \"%s\",\n", param->grayFile);
        strcat (retstr, str);
	
	sprintf (str, "    \"cutoutFile\": \"%s\",\n", param->subsetimfile);
        strcat (retstr, str);
	sprintf (str, "    \"shrunkFile\": \"%s\",\n", param->shrunkimfile);
        strcat (retstr, str);

/*
	
    Find colortbl index 
*/
        
	indx = getColortblIndx (param->colorTable);


        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "colortbl= [%s]\n", param->colorTable);
	    fprintf (fp_debug, "indx= [%d]\n", indx);
            fflush (fp_debug);
        }

	sprintf (str, "    \"colorTable\": \"%d\",\n", indx);
        strcat (retstr, str);

	sprintf (str, "    \"stretchMin\": \"%s\",\n", param->stretchMin);
        strcat (retstr, str);
	sprintf (str, "    \"stretchMax\": \"%s\",\n", param->stretchMax);
        strcat (retstr, str);
	sprintf (str, "    \"stretchMode\": \"%s\",\n", param->stretchMode);
        strcat (retstr, str);

/*
	sprintf (str, "    \"stretchMinval\": \"%s\",\n", param->stretchMinval);
        strcat (retstr, str);
	sprintf (str, "    \"stretchMaxval\": \"%s\",\n", param->stretchMaxval);
        strcat (retstr, str);
	
	sprintf (str, "    \"stretchMinunit\": \"%s\",\n", 
	    param->stretchMinunit);
        strcat (retstr, str);
	sprintf (str, "    \"stretchMaxunit\": \"%s\",\n", 
	    param->stretchMaxunit);
        strcat (retstr, str);
*/


	sprintf (str, "    \"dataMin\": \"%s\",\n", param->datamin);
        strcat (retstr, str);
	sprintf (str, "    \"dataMax\": \"%s\",\n", param->datamax);
        strcat (retstr, str);

	sprintf (str, "    \"percMin\": \"%s\",\n", param->percminstr);
        strcat (retstr, str);
	sprintf (str, "    \"percMax\": \"%s\",\n", param->percmaxstr);
        strcat (retstr, str);

	sprintf (str, "    \"sigmaMin\": \"%s\",\n", param->sigmaminstr);
        strcat (retstr, str);
	sprintf (str, "    \"sigmaMax\": \"%s\",\n", param->sigmamaxstr);
        strcat (retstr, str);

	sprintf (str, "    \"dispMin\": \"%s\",\n", param->minstr);
        strcat (retstr, str);
	sprintf (str, "    \"dispMax\": \"%s\",\n", param->maxstr);
        strcat (retstr, str);

	sprintf (str, "    \"xflip\": \"%d\",\n", param->xflip);
        strcat (retstr, str);
	sprintf (str, "    \"yflip\": \"%d\",\n", param->yflip);
        strcat (retstr, str);
	
	sprintf (str, "    \"bunit\": \"%s\"\n", param->bunit);
        strcat (retstr, str);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "noverlay= [%d]\n", param->noverlay);
	    fprintf (fp_debug, "nim= [%d]\n", param->nim);
	    fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
	    fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
	    fflush (fp_debug);
        }

	sprintf (str, "   }");
        strcat (retstr, str);
    
    }
    else {
/*
    redFile
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "redfile= [%s]\n", param->redFile);
            fflush (fp_debug);
        }
	
	sprintf (str, "  \"redFile\":\n");
        strcat (retstr, str);
    
        sprintf (str, "  {\n");
        strcat (retstr, str);
        
	sprintf (str, "    \"fitsFile\": \"%s\",\n", param->redFile);
        strcat (retstr, str);
	
	sprintf (str, "    \"cutoutFile\": \"%s\",\n", param->subsetredfile);
        strcat (retstr, str);
	sprintf (str, "    \"shrunkFile\": \"%s\",\n", param->shrunkredfile);
        strcat (retstr, str);

	sprintf (str, "    \"stretchMin\": \"%s\",\n", param->redMin);
        strcat (retstr, str);
	sprintf (str, "    \"stretchMax\": \"%s\",\n", param->redMax);
        strcat (retstr, str);
	sprintf (str, "    \"stretchMode\": \"%s\",\n", param->redMode);
        strcat (retstr, str);

	sprintf (str, "    \"dataMin\": \"%s\",\n", param->reddatamin);
        strcat (retstr, str);
	sprintf (str, "    \"dataMax\": \"%s\",\n", param->reddatamax);
        strcat (retstr, str);

	sprintf (str, "    \"percMin\": \"%s\",\n", param->redpercminstr);
        strcat (retstr, str);
	sprintf (str, "    \"percMax\": \"%s\",\n", param->redpercmaxstr);
        strcat (retstr, str);

	sprintf (str, "    \"sigmaMin\": \"%s\",\n", param->redsigmaminstr);
        strcat (retstr, str);
	sprintf (str, "    \"sigmaMax\": \"%s\",\n", param->redsigmamaxstr);
        strcat (retstr, str);

	sprintf (str, "    \"dispMin\": \"%s\",\n", param->redminstr);
        strcat (retstr, str);
	sprintf (str, "    \"dispMax\": \"%s\",\n", param->redmaxstr);
        strcat (retstr, str);


/*
    The following parameters are same for all three planes so
    only included in the reFile
*/
	sprintf (str, "    \"xflip\": \"%d\",\n", param->xflip);
        strcat (retstr, str);
	sprintf (str, "    \"yflip\": \"%d\",\n", param->yflip);
        strcat (retstr, str);
	sprintf (str, "    \"bunit\": \"%s\"\n", param->bunit);
        strcat (retstr, str);
    
   
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "noverlay= [%d]\n", param->noverlay);
	    fprintf (fp_debug, "nim= [%d]\n", param->nim);
	    fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
	    fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
	    fflush (fp_debug);
        }
	
	sprintf (str, "   }");
        strcat (retstr, str);

        if ((int)strlen (param->greenFile) > 0) {
/*
    grnFile
*/
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, "greenfile= [%s]\n", param->greenFile);
                fflush (fp_debug);
            }
	
	    sprintf (str, ",\n");
            strcat (retstr, str);
	
	    sprintf (str, "  \"greenFile\":\n");
            strcat (retstr, str);
    
            sprintf (str, "  {\n");
            strcat (retstr, str);
        
	    sprintf (str, "    \"fitsFile\": \"%s\",\n", param->greenFile);
            strcat (retstr, str);
	
	    sprintf (str, "    \"cutoutFile\": \"%s\",\n", 
	        param->subsetgrnfile);
            strcat (retstr, str);
	    sprintf (str, "    \"shrunkFile\": \"%s\",\n", 
	        param->shrunkgrnfile);
            strcat (retstr, str);

	    sprintf (str, "    \"stretchMin\": \"%s\",\n", param->greenMin);
            strcat (retstr, str);
	    sprintf (str, "    \"stretchMax\": \"%s\",\n", param->greenMax);
            strcat (retstr, str);
	    sprintf (str, "    \"stretchMode\": \"%s\",\n", param->greenMode);
            strcat (retstr, str);

	    sprintf (str, "    \"dataMin\": \"%s\",\n", param->grndatamin);
            strcat (retstr, str);
	    sprintf (str, "    \"dataMax\": \"%s\",\n", param->grndatamax);
            strcat (retstr, str);

	    sprintf (str, "    \"percMin\": \"%s\",\n", param->grnpercminstr);
            strcat (retstr, str);
	    sprintf (str, "    \"percMax\": \"%s\",\n", param->grnpercmaxstr);
            strcat (retstr, str);

	    sprintf (str, "    \"sigmaMin\": \"%s\",\n", param->grnsigmaminstr);
            strcat (retstr, str);
	    sprintf (str, "    \"sigmaMax\": \"%s\",\n", param->grnsigmamaxstr);
            strcat (retstr, str);

	    sprintf (str, "    \"dispMin\": \"%s\",\n", param->grnminstr);
            strcat (retstr, str);
	    sprintf (str, "    \"dispMax\": \"%s\"\n", param->grnmaxstr);
            strcat (retstr, str);
	
	    sprintf (str, "   }");
            strcat (retstr, str);

        }
    
    
        if ((int)strlen (param->blueFile) > 0) {
/*
    blueFile
*/
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, "bluefile= [%s]\n", param->blueFile);
                fflush (fp_debug);
            }
	
	    sprintf (str, ",\n");
            strcat (retstr, str);
	
	    sprintf (str, "  \"blueFile\":\n");
            strcat (retstr, str);
    
            sprintf (str, "  {\n");
            strcat (retstr, str);
        
	    sprintf (str, "    \"fitsFile\": \"%s\",\n", param->blueFile);
            strcat (retstr, str);
	
	    sprintf (str, "    \"cutoutFile\": \"%s\",\n", 
	        param->subsetbluefile);
            strcat (retstr, str);
	    sprintf (str, "    \"shrunkFile\": \"%s\",\n", 
	        param->shrunkbluefile);
            strcat (retstr, str);

	    sprintf (str, "    \"stretchMin\": \"%s\",\n", param->blueMin);
            strcat (retstr, str);
	    sprintf (str, "    \"stretchMax\": \"%s\",\n", param->blueMax);
            strcat (retstr, str);
	    sprintf (str, "    \"stretchMode\": \"%s\",\n", param->blueMode);
            strcat (retstr, str);

	    sprintf (str, "    \"dataMin\": \"%s\",\n", param->bluedatamin);
            strcat (retstr, str);
	    sprintf (str, "    \"dataMax\": \"%s\",\n", param->bluedatamax);
            strcat (retstr, str);

	    sprintf (str, "    \"percMin\": \"%s\",\n", param->bluepercminstr);
            strcat (retstr, str);
	    sprintf (str, "    \"percMax\": \"%s\",\n", param->bluepercmaxstr);
            strcat (retstr, str);

	    sprintf (str, "    \"sigmaMin\": \"%s\",\n", 
	        param->bluesigmaminstr);
            strcat (retstr, str);
	    sprintf (str, "    \"sigmaMax\": \"%s\",\n", 
	        param->bluesigmamaxstr);
            strcat (retstr, str);

	    sprintf (str, "    \"dispMin\": \"%s\",\n", param->blueminstr);
            strcat (retstr, str);
	    sprintf (str, "    \"dispMax\": \"%s\"\n", param->bluemaxstr);
            strcat (retstr, str);
	
	    sprintf (str, "   }");
            strcat (retstr, str);

        }
    
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "noverlay= [%d]\n", param->noverlay);
	fprintf (fp_debug, "nim= [%d]\n", param->nim);
	fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
	fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
	fflush (fp_debug);
    }
	

    if (param->noverlay == 0) { 


	    sprintf (str, "\n");
            strcat (retstr, str);

	    sprintf (str, "}\n");
            strcat (retstr, str);

/*
            sprintf (str, "   ]\n");
            strcat (retstr, str);
            sprintf (str, "}");
            strcat (retstr, str);
*/

            strcpy (param->jsonStr, retstr);

	    return (0);
    }
	
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "here1\n");
	fflush (fp_debug);
    }

/*
    overlay:  
*/
    sprintf (str, ",\n");
    strcat (retstr, str);
	
    sprintf (str, "  \"overlay\":\n");
    strcat (retstr, str);

    sprintf (str, "  [\n");
    strcat (retstr, str);

        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "noverlay= [%d]\n", param->noverlay);
	fflush (fp_debug);
    }

    for (l=0; l<param->noverlay; l++) {

        strcpy (layervis, param->overlay[l].visible);
            
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "l= [%d] layervis= [%s]\n", l, layervis);
	    fflush (fp_debug);
        }

	strcpy (layertype, param->overlay[l].type);
        strcpy (layercolor, param->overlay[l].color);
        strcpy (layercsys, param->overlay[l].coordSys);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "layertype= [%s]\n", layertype);
	    fprintf (fp_debug, "layercolor= [%s]\n", layercolor);
	    fprintf (fp_debug, "layercsys= [%s]\n", layercsys);
	    fflush (fp_debug);
        }

/*
    overlay: grid 
*/
            
        if (strcasecmp (layertype, "grid") == 0) {
       
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, "xxx: grid\n");
	        fflush (fp_debug);
            }

	    sprintf (str, "    {\n");
            strcat (retstr, str);
		
	    sprintf (str, "      \"type\": \"grid\",\n");
            strcat (retstr, str);
		
	    sprintf (str, "      \"coordSys\": \"%s\",\n", layercsys);
            strcat (retstr, str);
		
	    istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, 
		    "returned color2hexcolor, istatus= [%d]\n", istatus); 
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

	    sprintf (str, "      \"color\": \"%s\",\n", hexcolor);
            strcat (retstr, str);

	        
	    if ((strcasecmp (layervis, "true") == 0) ||
		(strcasecmp (layervis, "yes") == 0) ||
		(strcasecmp (layervis, "1") == 0)) {
		    
		sprintf (str, "      \"visible\": \"true\"\n");
                strcat (retstr, str);
            }
	    else {
		sprintf (str, "      \"visible\": \"false\"\n");
                strcat (retstr, str);
            }
	    sprintf (str, "     }");
            strcat (retstr, str);
        }
        else if ((strcasecmp (layertype, "marker") == 0) ||
            (strcasecmp (layertype, "mark") == 0)) {
       
       
/*
    overlay: marker (compass) 
*/
	    strcpy (symtype, param->overlay[l].symType);
            strcpy (symsize, param->overlay[l].symSize);
            strcpy (location, param->overlay[l].location);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, "symtype= [%s]\n", symtype);
	        fprintf (fp_debug, "symsize= [%s]\n", symsize);
	        fprintf (fp_debug, "location= [%s]\n", location);
	        fflush (fp_debug);
            }

	    sprintf (str, ",\n");
            strcat (retstr, str);

	    sprintf (str, "    {\n");
            strcat (retstr, str);

	    sprintf (str, "      \"type\": \"mark\",\n");
            strcat (retstr, str);

	    sprintf (str, "      \"symType\": \"%s\",\n", symtype);
            strcat (retstr, str);

		
	    sprintf (str, "      \"symSize\": \"%s\",\n", symsize);
            strcat (retstr, str);

		
	    sprintf (str, "      \"location\": \"%s\",\n", location);
            strcat (retstr, str);

		
	    istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	        fprintf (fp_debug, 
		    "returned color2hexcolor, istatus= [%d]\n", istatus); 
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

		sprintf (str, "      \"color\": \"%s\",\n", hexcolor);
                strcat (retstr, str);

	        
		if ((strcasecmp (layervis, "true") == 0) ||
		    (strcasecmp (layervis, "yes") == 0)) {
		    
		    sprintf (str, "      \"visible\": \"true\"\n");
                    strcat (retstr, str);

                }
		else {
		    sprintf (str, "      \"visible\": \"false\"\n");
                    strcat (retstr, str);

                }
	        sprintf (str, "     }");
                strcat (retstr, str);

        }
        else if (strcasecmp (layertype, "catalog") == 0) {
/*
    overlay: catalog 
*/
	        strcpy (layerfilename, param->overlay[l].dataFile);
	        strcpy (symtype, param->overlay[l].symType);
                strcpy (symside, param->overlay[l].symSide);
                strcpy (symsize, param->overlay[l].symSize);
                strcpy (datacol, param->overlay[l].dataCol);
                strcpy (dataref, param->overlay[l].dataRef);
                strcpy (datatype, param->overlay[l].dataType);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "layerfilename= [%s]\n", layerfilename);
	            fprintf (fp_debug, "symtype= [%s]\n", symtype);
	            fprintf (fp_debug, "symsize= [%s]\n", symsize);
	            fprintf (fp_debug, "symside= [%s]\n", symside);
	            fprintf (fp_debug, "datacol= [%s]\n", datacol);
	            fprintf (fp_debug, "dataref= [%s]\n", dataref);
	            fprintf (fp_debug, "datatype= [%s]\n", datatype);
	            fflush (fp_debug);
                }

		sprintf (str, "     ,\n");
                strcat (retstr, str);

		sprintf (str, "    {\n");
                strcat (retstr, str);

		sprintf (str, "      \"type\": \"catalog\",\n");
                strcat (retstr, str);

		
		sprintf (str, "      \"dataFile\": \"%s\",\n", layerfilename);
                strcat (retstr, str);
	
		istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
		
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, 
		        "returned color2hexcolor, istatus= [%d]\n", istatus); 
	            fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	            fflush (fp_debug);
                }
	        
		sprintf (str, "      \"color\": \"%s\",\n", hexcolor);
                strcat (retstr, str);
	

		sprintf (str, "      \"symType\": \"%s\",\n", symtype);
                strcat (retstr, str);
	
		sprintf (str, "      \"symSize\": \"%s\",\n", symsize);
                strcat (retstr, str);
		sprintf (str, "      \"symSide\": \"%s\",\n", symside);
                strcat (retstr, str);
	
	
                if ((int)strlen(datacol) > 0) {

		    sprintf (str, "      \"dataCol\": \"%s\",\n", datacol);
                    strcat (retstr, str);
	
		    sprintf (str, "      \"dataRef\": \"%s\",\n", dataref);
                    strcat (retstr, str);
	
		    sprintf (str, "      \"dataType\": \"%s\",\n", datatype);
                    strcat (retstr, str);
	
		}

	        
		if ((strcasecmp (layervis, "true") == 0) ||
		    (strcasecmp (layervis, "yes") == 0)) {
		    
		    sprintf (str, "      \"visible\": \"true\"\n");
                    strcat (retstr, str);
	
                }
		else {
		    sprintf (str, "      \"visible\": \"false\"\n");
                    strcat (retstr, str);
	
                }
	        sprintf (str, "     }");
                strcat (retstr, str);
	
        }
        else if (strcasecmp (layertype, "iminfo") == 0) {
/*
    overlay: iminfo 
*/
	        strcpy (layerfilename, param->overlay[l].dataFile);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, "layerfilename= [%s]\n", layerfilename);
	            fflush (fp_debug);
                }

		sprintf (str, "     ,\n");
                strcat (retstr, str);

		sprintf (str, "    {\n");
                strcat (retstr, str);
	
		sprintf (str, "      \"type\": \"iminfo\",\n");
                strcat (retstr, str);
	
		
		sprintf (str, "      \"dataFile\": \"%s\",\n", layerfilename);
                strcat (retstr, str);
	

		istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
		
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, 
		        "returned color2hexcolor, istatus= [%d]\n", istatus); 
	            fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	            fflush (fp_debug);
                }
	        
		sprintf (str, "      \"color\": \"%s\",\n", hexcolor);
                strcat (retstr, str);
	

		if ((strcasecmp (layervis, "true") == 0) ||
		    (strcasecmp (layervis, "yes") == 0)) {
		    
		    sprintf (str, "      \"visible\": \"true\"\n");
                    strcat (retstr, str);
	
                }
		else {
		    sprintf (str, "      \"visible\": \"false\"\n");
                    strcat (retstr, str);
	
                }
	        sprintf (str, "     }");
                strcat (retstr, str);
	
        }
        else if (strcasecmp (layertype, "label") == 0) {
/*
    overlay: label  
*/
		sprintf (str, "     ,\n");
                strcat (retstr, str);

		sprintf (str, "    {\n");
                strcat (retstr, str);
	
		sprintf (str, "      \"type\": \"label\",\n");
                strcat (retstr, str);
	
		istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	        
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, 
		        "returned color2hexcolor, istatus= [%d]\n", istatus); 
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

		sprintf (str, "      \"color\": \"%s\",\n", hexcolor);
                strcat (retstr, str);
	
		sprintf (str, "      \"location\": \"%s\",\n", 
		    param->overlay[l].location);
                strcat (retstr, str);

		sprintf (str, "      \"text\": \"%s\",\n", 
		    param->overlay[l].text);
                strcat (retstr, str);
	
		if ((strcasecmp (layervis, "true") == 0) ||
		    (strcasecmp (layervis, "yes") == 0)) {
		    
		    sprintf (str, "      \"visible\": \"true\"\n");
                    strcat (retstr, str);
                }
		else {
		    sprintf (str, "      \"visible\": \"false\"\n");
                    strcat (retstr, str);
                }
	        sprintf (str, "     }");
                strcat (retstr, str);
	
        }
    }

    sprintf (str, "\n");
    strcat (retstr, str);
    sprintf (str, "   ]\n");
    strcat (retstr, str);

    sprintf (str, "}");
    strcat (retstr, str);
   
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "retstr= [%s] len= [%d]\n", retstr, 
	    (int)strlen(retstr));
	fflush (fp_debug);
    }
    
    strcpy (param->jsonStr, retstr);

    return (0);
}



