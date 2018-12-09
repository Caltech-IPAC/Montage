/*
Theme: This routine writes the return json structure.

Written: October 07, 2016 (Mihseh Kong)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "mviewer.h"


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

    int   debugfile = 0;

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

    char  layerfilename[1024];
    
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


    int  debugfile = 0;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "\nFrom constructRetjson\n");
	fflush (fp_debug);
    }

    sprintf (retstr, "{\n");
    
/*
    top level
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "cmd= [%s]\n", param->cmd); 
	fprintf (fp_debug, "workspace= [%s]\n", param->workspace); 
	fprintf (fp_debug, "baseurl= [%s]\n", param->baseURL); 
	fprintf (fp_debug, "helpHtml= [%s]\n", param->helphtml); 
	fflush (fp_debug);
    }
    
    sprintf (str, "  \"baseurl\": \"%s\",\n", param->baseURL);
    strcat (retstr, str);

    sprintf (str, "  \"helphtml\": \"%s\",\n", param->helphtml);
    strcat (retstr, str);
    sprintf (str, "  \"cmd\": \"%s\",\n", param->cmd);
    strcat (retstr, str);

/*
    imcube
*/
    if (param->isimcube) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "imcubefile= [%s]\n", param->imcubefile);
	    fprintf (fp_debug, "cubedatadir= [%s]\n", param->cubedatadir);
            fflush (fp_debug);
        }

	sprintf(str, "  \"imcube\":\n");
        strcat (retstr, str);
	
	sprintf(str, "  {\n");
        strcat (retstr, str);

	sprintf(str, "    \"datadir\": \"%s\",\n", param->cubedatadir);
        strcat (retstr, str);

	sprintf(str, "    \"fitsfile\": \"%s\",\n", param->imcubefile);
        strcat (retstr, str);
	
	sprintf(str, "    \"planeavemode\": \"%s\",\n", param->planeavemode);
        strcat (retstr, str);
	
	sprintf(str, "    \"planenum\": \"%d\",\n", param->nfitsplane);
        strcat (retstr, str);
	sprintf(str, "    \"nplaneave\": \"%d\",\n", param->nplaneave);
        strcat (retstr, str);
	sprintf(str, "    \"centerplane\": \"%d\",\n", param->centerplane);
        strcat (retstr, str);
	
	sprintf(str, "    \"startplane\": \"%d\",\n", param->startplane);
        strcat (retstr, str);
	sprintf(str, "    \"endplane\": \"%d\"\n", param->endplane);
        strcat (retstr, str);
	
	sprintf (str, "   },\n");
        strcat (retstr, str);
    }

/*
    image
*/
    sprintf (str, "  \"image\":\n");
    strcat (retstr, str);
    sprintf (str, "  {\n");
    strcat (retstr, str);
  
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	fprintf (fp_debug, "imageFile= [%s]\n", param->imageFile);
	fprintf (fp_debug, "imageType= [%s]\n", param->imageType);
	fprintf (fp_debug, "imname= [%s]\n", param->imname);
	
	fprintf (fp_debug, "jpgfile= [%s]\n", param->jpgfile); 
	fprintf (fp_debug, "nim= [%d]\n", param->nim); 
	fprintf (fp_debug, "refjpgfile= [%s]\n", param->refjpgfile); 
	
	fprintf (fp_debug, "nowcs= [%d]\n", param->nowcs);
	fprintf (fp_debug, "canvasWidth= [%d] canvasHeight= [%d]\n", 
	    param->canvasWidth, param->canvasHeight);
	fflush (fp_debug);
    }

    if (param->iscolor) {
        sprintf (str, "    \"type\": \"color\",\n");
    }
    else {
        sprintf (str, "    \"type\": \"grayscale\",\n");
    }
    strcat (retstr, str);
    
    sprintf (str, "    \"imagename\": \"%s\",\n", param->imname);
    strcat (retstr, str);

/*
    sprintf (str, "    \"file\": \"%s/%s\",\n", param->baseURL, param->jpgfile);
    strcat (retstr, str);

    if (param->nim == 2) {
        sprintf (str, "    \"reffile\": \"%s/%s\",\n", 
            param->baseURL, param->refjpgfile);
        strcat (retstr, str);
    }
*/
    
    sprintf (str, "    \"file\": \"%s\",\n", param->jpgfile);
    strcat (retstr, str);

    if (param->nim == 2) {
        sprintf (str, "    \"reffile\": \"%s\",\n", param->refjpgfile);
        strcat (retstr, str);
    }
    
    sprintf (str, "    \"imagetype\": \"%s\",\n", param->imageType);
    strcat (retstr, str);
    
    sprintf (str, "    \"canvaswidth\": \"%d\",\n", param->canvasWidth);
    strcat (retstr, str);
    sprintf (str, "    \"canvasheight\": \"%d\",\n", param->canvasHeight);
    strcat (retstr, str);
   
    if ((param->refWidth > 0) && (param->refHeight > 0)) {

        sprintf (str, "    \"refwidth\": \"%d\",\n", param->refWidth);
        strcat (retstr, str);
        sprintf (str, "    \"refheight\": \"%d\",\n", param->refHeight);
        strcat (retstr, str);
    }

    
    sprintf (str, "    \"imagewidth\": \"%d\",\n", param->imageWidth);
    strcat (retstr, str);
    sprintf (str, "    \"imageheight\": \"%d\",\n", param->imageHeight);
    strcat (retstr, str);
    sprintf (str, "    \"nowcs\": \"%d\",\n", param->nowcs);
    strcat (retstr, str);
    
    if (!param->nowcs) {
        sprintf (str, "    \"imsys\": \"%s\",\n", param->imcsys);
        strcat (retstr, str);
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "writing zoomfactor= [%.4f]\n", param->zoomfactor);
	fflush (fp_debug);
    }

    indx = -1;
    sprintf (str, "    \"factor\": \"%.4f\",\n", param->zoomfactor);
    strcat (retstr, str);

    

    if (!param->iscolor) {

/*
    grayFile
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "grayfile= [%s]\n", param->grayFile);
            fflush (fp_debug);
        }

	sprintf (str, "    \"grayfile\":\n");
        strcat (retstr, str);
    
        sprintf (str, "    {\n");
        strcat (retstr, str);
       
	sprintf (str, "      \"datadir\": \"%s\",\n", param->imdatadir);
        strcat (retstr, str);
	
	sprintf (str, "      \"fitsfile\": \"%s\",\n", param->grayFile);
        strcat (retstr, str);
	
	sprintf (str, "      \"cutoutfile\": \"%s\",\n", param->subsetimfile);
        strcat (retstr, str);
	sprintf (str, "      \"shrunkfile\": \"%s\",\n", param->shrunkimfile);
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

	sprintf (str, "      \"colortable\": \"%d\",\n", indx);
        strcat (retstr, str);

	sprintf (str, "      \"stretchmin\": \"%s\",\n", param->stretchMin);
        strcat (retstr, str);
	sprintf (str, "      \"stretchmax\": \"%s\",\n", param->stretchMax);
        strcat (retstr, str);
	sprintf (str, "      \"stretchmode\": \"%s\",\n", param->stretchMode);
        strcat (retstr, str);

	sprintf (str, "      \"datamin\": \"%s\",\n", param->datamin);
        strcat (retstr, str);
	sprintf (str, "      \"datamax\": \"%s\",\n", param->datamax);
        strcat (retstr, str);

	sprintf (str, "      \"percmin\": \"%s\",\n", param->percminstr);
        strcat (retstr, str);
	sprintf (str, "      \"percmax\": \"%s\",\n", param->percmaxstr);
        strcat (retstr, str);

	sprintf (str, "      \"sigmamin\": \"%s\",\n", param->sigmaminstr);
        strcat (retstr, str);
	sprintf (str, "      \"sigmamax\": \"%s\",\n", param->sigmamaxstr);
        strcat (retstr, str);

	sprintf (str, "      \"dispmin\": \"%s\",\n", param->minstr);
        strcat (retstr, str);
	sprintf (str, "      \"dispmax\": \"%s\",\n", param->maxstr);
        strcat (retstr, str);

	sprintf (str, "      \"xflip\": \"%d\",\n", param->xflip);
        strcat (retstr, str);
	sprintf (str, "      \"yflip\": \"%d\",\n", param->yflip);
        strcat (retstr, str);
	
	sprintf (str, "      \"bunit\": \"%s\"\n", param->bunit);
        strcat (retstr, str);
	
	sprintf (str, "    }\n");
        strcat (retstr, str);
	sprintf (str, "  },\n");
        strcat (retstr, str);
    

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "noverlay= [%d]\n", param->noverlay);
	    fprintf (fp_debug, "nim= [%d]\n", param->nim);
	    fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
	    fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
	    fflush (fp_debug);
        }

    }
    else {
/*
    redFile
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	    fprintf (fp_debug, "redfile= [%s]\n", param->redFile);
            fflush (fp_debug);
        }
	
	sprintf (str, "    \"redFile\":\n");
        strcat (retstr, str);
    
        sprintf (str, "    {\n");
        strcat (retstr, str);
        
	sprintf (str, "      \"datadir\": \"%s\",\n", param->imdatadir);
        strcat (retstr, str);
	
	sprintf (str, "      \"fitsFile\": \"%s\",\n", param->redFile);
        strcat (retstr, str);
	
	sprintf (str, "      \"cutoutFile\": \"%s\",\n", param->subsetredfile);
        strcat (retstr, str);
	sprintf (str, "      \"shrunkFile\": \"%s\",\n", param->shrunkredfile);
        strcat (retstr, str);

	sprintf (str, "      \"stretchMin\": \"%s\",\n", param->redMin);
        strcat (retstr, str);
	sprintf (str, "      \"stretchMax\": \"%s\",\n", param->redMax);
        strcat (retstr, str);
	sprintf (str, "      \"stretchMode\": \"%s\",\n", param->redMode);
        strcat (retstr, str);

	sprintf (str, "      \"dataMin\": \"%s\",\n", param->reddatamin);
        strcat (retstr, str);
	sprintf (str, "      \"dataMax\": \"%s\",\n", param->reddatamax);
        strcat (retstr, str);

	sprintf (str, "      \"percMin\": \"%s\",\n", param->redpercminstr);
        strcat (retstr, str);
	sprintf (str, "      \"percMax\": \"%s\",\n", param->redpercmaxstr);
        strcat (retstr, str);

	sprintf (str, "      \"sigmaMin\": \"%s\",\n", param->redsigmaminstr);
        strcat (retstr, str);
	sprintf (str, "      \"sigmaMax\": \"%s\",\n", param->redsigmamaxstr);
        strcat (retstr, str);

	sprintf (str, "      \"dispMin\": \"%s\",\n", param->redminstr);
        strcat (retstr, str);
	sprintf (str, "      \"dispMax\": \"%s\",\n", param->redmaxstr);
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
	
	    sprintf (str, "    ,\n");
            strcat (retstr, str);
	
	    sprintf (str, "    \"greenFile\":\n");
            strcat (retstr, str);
    
            sprintf (str, "    {\n");
            strcat (retstr, str);
        
	    sprintf (str, "      \"fitsFile\": \"%s\",\n", param->greenFile);
            strcat (retstr, str);
	
	    sprintf (str, "      \"cutoutFile\": \"%s\",\n", 
	        param->subsetgrnfile);
            strcat (retstr, str);
	    sprintf (str, "      \"shrunkFile\": \"%s\",\n", 
	        param->shrunkgrnfile);
            strcat (retstr, str);

	    sprintf (str, "      \"stretchMin\": \"%s\",\n", param->greenMin);
            strcat (retstr, str);
	    sprintf (str, "      \"stretchMax\": \"%s\",\n", param->greenMax);
            strcat (retstr, str);
	    sprintf (str, "      \"stretchMode\": \"%s\",\n", param->greenMode);
            strcat (retstr, str);

	    sprintf (str, "      \"dataMin\": \"%s\",\n", param->grndatamin);
            strcat (retstr, str);
	    sprintf (str, "      \"dataMax\": \"%s\",\n", param->grndatamax);
            strcat (retstr, str);

	    sprintf (str, "      \"percMin\": \"%s\",\n", param->grnpercminstr);
            strcat (retstr, str);
	    sprintf (str, "      \"percMax\": \"%s\",\n", param->grnpercmaxstr);
            strcat (retstr, str);

	    sprintf (str, "      \"sigmaMin\": \"%s\",\n", 
	        param->grnsigmaminstr);
            strcat (retstr, str);
	    sprintf (str, "      \"sigmaMax\": \"%s\",\n", 
	        param->grnsigmamaxstr);
            strcat (retstr, str);

	    sprintf (str, "      \"dispMin\": \"%s\",\n", param->grnminstr);
            strcat (retstr, str);
	    sprintf (str, "      \"dispMax\": \"%s\"\n", param->grnmaxstr);
            strcat (retstr, str);
	
	    sprintf (str, "    }");
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
	
	    sprintf (str, "    ,\n");
            strcat (retstr, str);
	
	    sprintf (str, "    \"blueFile\":\n");
            strcat (retstr, str);
    
            sprintf (str, "    {\n");
            strcat (retstr, str);
        
	    sprintf (str, "      \"fitsFile\": \"%s\",\n", param->blueFile);
            strcat (retstr, str);
	
	    sprintf (str, "      \"cutoutFile\": \"%s\",\n", 
	        param->subsetbluefile);
            strcat (retstr, str);
	    sprintf (str, "      \"shrunkFile\": \"%s\",\n", 
	        param->shrunkbluefile);
            strcat (retstr, str);

	    sprintf (str, "      \"stretchMin\": \"%s\",\n", param->blueMin);
            strcat (retstr, str);
	    sprintf (str, "      \"stretchMax\": \"%s\",\n", param->blueMax);
            strcat (retstr, str);
	    sprintf (str, "      \"stretchMode\": \"%s\",\n", param->blueMode);
            strcat (retstr, str);

	    sprintf (str, "      \"dataMin\": \"%s\",\n", param->bluedatamin);
            strcat (retstr, str);
	    sprintf (str, "      \"dataMax\": \"%s\",\n", param->bluedatamax);
            strcat (retstr, str);

	    sprintf (str, "      \"percMin\": \"%s\",\n", 
	        param->bluepercminstr);
            strcat (retstr, str);
	    sprintf (str, "      \"percMax\": \"%s\",\n", 
	        param->bluepercmaxstr);
            strcat (retstr, str);

	    sprintf (str, "      \"sigmaMin\": \"%s\",\n", 
	        param->bluesigmaminstr);
            strcat (retstr, str);
	    sprintf (str, "      \"sigmaMax\": \"%s\",\n", 
	        param->bluesigmamaxstr);
            strcat (retstr, str);

	    sprintf (str, "      \"dispMin\": \"%s\",\n", param->blueminstr);
            strcat (retstr, str);
	    sprintf (str, "      \"dispMax\": \"%s\"\n", param->bluemaxstr);
            strcat (retstr, str);
	
	    sprintf (str, "     }");
	    sprintf (str, "   }");
            strcat (retstr, str);

        }
    
    }


/*
    subimage
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "cutoutWidth= [%d] cutoutHeight= [%d]\n", 
	    param->cutoutWidth, param->cutoutHeight);
        fprintf (fp_debug, "ns= [%d] nl= [%d]\n", param->ns, param->nl);
	fflush (fp_debug);
    }
    
    sprintf (str, "  \"subimage\":\n");
    strcat (retstr, str);

    sprintf (str, "  {\n");
    strcat (retstr, str);

    sprintf (str, "    \"cutoutfile\": \"%s\",\n", param->subsetimfile);
    strcat (retstr, str);
  
    if ((strcasecmp (param->cmd, "movebox") == 0) ||
        (strcasecmp (param->cmd, "zoombox") == 0) ||
        (strcasecmp (param->cmd, "zoomin") == 0) ||
        (strcasecmp (param->cmd, "zoomout") == 0) ||
        (strcasecmp (param->cmd, "resetzoom") == 0)) 
    {
        sprintf (str, "    \"cutoutwidth\": \"%d\",\n", param->ns);
        strcat (retstr, str);
        sprintf (str, "    \"cutoutheight\": \"%d\",\n", param->nl);
        strcat (retstr, str);
    }
    else {
        sprintf (str, "    \"cutoutwidth\": \"%d\",\n", param->cutoutWidth);
        strcat (retstr, str);
        sprintf (str, "    \"cutoutheight\": \"%d\",\n", param->cutoutHeight);
        strcat (retstr, str);
    }

    sprintf (str, "    \"ss\": \"%.1f\",\n", param->ss);
    strcat (retstr, str);
    sprintf (str, "    \"sl\": \"%.1f\",\n", param->sl);
    strcat (retstr, str);
    
/*
    subimage boundary on the original fits image
*/
    sprintf (str, "    \"xmin\": \"%.1f\",\n", param->xmin);
    strcat (retstr, str);
    sprintf (str, "    \"xmax\": \"%.1f\",\n", param->xmax);
    strcat (retstr, str);
    sprintf (str, "    \"ymin\": \"%.1f\",\n", param->ymin);
    strcat (retstr, str);
    sprintf (str, "    \"ymax\": \"%.1f\"\n", param->ymax);
    strcat (retstr, str);
    
    sprintf (str, "  },\n");
    strcat (retstr, str);
   

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here2: retstr= [%s]\n", retstr);
	fflush (fp_debug);
    }

    sprintf (str, "  \"cursor\":\n");
    strcat (retstr, str);
    
    sprintf (str, "  {\n");
    strcat (retstr, str);
        

/*
    click/box input parameters
*/
    sprintf (str, "    \"xs\": \"%.1f\",\n", param->xs);
    strcat (retstr, str);
    sprintf (str, "    \"xe\": \"%.1f\",\n", param->xe);
    strcat (retstr, str);
    sprintf (str, "    \"ys\": \"%.1f\",\n", param->ys);
    strcat (retstr, str);
    sprintf (str, "    \"ye\": \"%.1f\",\n", param->ye);
    strcat (retstr, str);

/*
    click/box result parameters
*/
    sprintf (str, "    \"pickvalue\": \"%lf\",\n", param->pickval);
    strcat (retstr, str);
    sprintf (str, "    \"picksys\": \"%s\",\n", param->pickcsys);
    strcat (retstr, str);
	
    sprintf (str, "    \"xpick\": \"%d\",\n", param->xpick);
    strcat (retstr, str);
    sprintf (str, "    \"ypick\": \"%d\",\n", param->ypick);
    strcat (retstr, str);

    sprintf (str, "    \"rapick\": \"%lf\",\n", param->rapick);
    strcat (retstr, str);
    sprintf (str, "    \"decpick\": \"%lf\",\n", param->decpick);
    strcat (retstr, str);

    sprintf (str, "    \"sexrapick\": \"%s\",\n", param->sexrapick);
    strcat (retstr, str);
    sprintf (str, "    \"sexdecpick\": \"%s\"\n", param->sexdecpick);
    strcat (retstr, str);

    sprintf (str, "  }");
    strcat (retstr, str);
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here3: retstr= [%s]\n", retstr);
	fflush (fp_debug);
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

    strcpy (layercolor, "");
    strcpy (hexcolor, "");

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
		
	    sprintf (str, "      \"coordsys\": \"%s\",\n", layercsys);
            strcat (retstr, str);
	    
/*
    default: darkgrey 
*/
	    if ((int)strlen(layercolor) == 0) {
                strcpy (hexcolor, &defaultHexcolor[4][1]);
            }
	    else {
	        istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	        
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, 
		        "returned color2hexcolor, istatus= [%d]\n", istatus); 
	            fflush (fp_debug);
                }

	        if (istatus == -1) {
                    strcpy (hexcolor, layercolor);
                }
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

	    sprintf (str, "      \"symtype\": \"%s\",\n", symtype);
            strcat (retstr, str);

		
	    sprintf (str, "      \"symsize\": \"%s\",\n", symsize);
            strcat (retstr, str);

		
	    sprintf (str, "      \"location\": \"%s\",\n", location);
            strcat (retstr, str);

/*
    default: red 
*/
            if ((int)strlen(layercolor) == 0) {
                strcpy (hexcolor, "880000");
            }
	    else {
	        istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	        
                if ((debugfile) && (fp_debug != (FILE *)NULL)) {

	            fprintf (fp_debug, 
		        "returned color2hexcolor, istatus= [%d]\n", istatus); 
	            fflush (fp_debug);
                }

		if (istatus == -1) {
                    strcpy (hexcolor, layercolor);
                }
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

  	    sprintf (str, ",\n");
            strcat (retstr, str);

	    sprintf (str, "     {\n");
            strcat (retstr, str);

	    sprintf (str, "      \"type\": \"catalog\",\n");
            strcat (retstr, str);

	    sprintf (str, "      \"datadir\": \"%s\",\n", 
		param->overlay[l].datadir);
            strcat (retstr, str);

	    sprintf (str, "      \"datafile\": \"%s\",\n", layerfilename);
            strcat (retstr, str);
	
	    istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
		    "returned color2hexcolor, istatus= [%d]\n", istatus); 
	        fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	        fflush (fp_debug);
            }
	        
	    if (istatus < 0) {
	        strcpy (hexcolor, layercolor);
	    }
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	        fflush (fp_debug);
            }
	        
	    sprintf (str, "      \"color\": \"%s\",\n", hexcolor);
            strcat (retstr, str);

	    sprintf (str, "      \"symtype\": \"%s\",\n", symtype);
            strcat (retstr, str);
	
	    sprintf (str, "      \"symsize\": \"%s\",\n", symsize);
            strcat (retstr, str);
	    sprintf (str, "      \"symside\": \"%s\",\n", symside);
            strcat (retstr, str);
	
	
            if ((int)strlen(datacol) > 0) {

		sprintf (str, "      \"datacol\": \"%s\",\n", datacol);
                strcat (retstr, str);
	
		sprintf (str, "      \"dataref\": \"%s\",\n", dataref);
                strcat (retstr, str);
	
		sprintf (str, "      \"datatype\": \"%s\",\n", datatype);
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

	    sprintf (str, ",\n");
            strcat (retstr, str);

	    sprintf (str, "     {\n");
            strcat (retstr, str);
	
	    sprintf (str, "      \"type\": \"iminfo\",\n");
            strcat (retstr, str);
	    
	    sprintf (str, "      \"datadir\": \"%s\",\n", 
		param->overlay[l].datadir);
            strcat (retstr, str);
		
	    sprintf (str, "      \"datafile\": \"%s\",\n", layerfilename);
            strcat (retstr, str);

	    istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
		
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
		    "returned color2hexcolor, istatus= [%d]\n", istatus); 
	        fprintf (fp_debug, "hexcolor= [%s]\n", hexcolor); 
	        fflush (fp_debug);
            }

	    if (istatus < 0) {
	        strcpy (hexcolor, layercolor);
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

/*
    default: darkgrey 
*/
	    if ((int)strlen(layercolor) == 0) {
                strcpy (hexcolor, &defaultHexcolor[4][1]);
            }

	    istatus = color2hexcolor (layercolor, hexcolor, ndefaultcolor);
	        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, 
	            "returned color2hexcolor, istatus= [%d]\n", istatus); 
                fflush (fp_debug);
            }
		
	    if (istatus == -1) {
                strcpy (hexcolor, layercolor);
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
    sprintf (str, "  ]\n");
    strcat (retstr, str);

    sprintf (str, "}");
    strcat (retstr, str);
   
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "retstr= \n%s\n len= [%d]\n", retstr, 
	    (int)strlen(retstr));
	fflush (fp_debug);
    }
    
    strcpy (param->jsonStr, retstr);

    return (0);
}



