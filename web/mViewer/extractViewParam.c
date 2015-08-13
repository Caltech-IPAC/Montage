/*
Theme:  Extract input parameters from configuration file and cgi keywords.

*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include <config.h>
#include <www.h>
#include <cmd.h>
#include <json.h>


#include "mviewer.h"


char *strtrim (char *);
int str2Integer (char *strval, int *intval, char *errmsg);
int str2Double (char *strval, double *dblval, char *errmsg);

int checkFileExist (char *fname, char *rootname, char *suffix,
    char *directory, char *filePath);


extern FILE *fdebug;

int extractViewParam (struct Mviewer *param)
{
    FILE     *fp;

    char     str[1024];
    char     jsonFile[1024];
    
    char     directory[1024];
    char     baseurl[1024];
    char     name[1024];
    
    char     *keyname;
    char     *keyval;
    char     *keyfname;

    int      istatus;
    int      offset;
    int      l, i;
    
    double   dblval;

    int      debugfile = 1;

    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "From extractViewParam: cmd= [%s]\n", param->cmd);
        fflush (fdebug); 
    }

    for (l=0; l<param->nkey; l++) {

        istatus = keyword_info (l, &keyname, &keyval, &keyfname);

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "l= [%d] keyname= [%s] keyval= [%s]\n",
	        l, keyname, keyval);
            fflush (fdebug); 
        }
    }


    baseurl[0] = '\0';
    directory[0] = '\0';
    if (config_value("ISIS_WORKURL") != (char *)NULL)
        strcpy(baseurl, config_value("ISIS_WORKURL"));
    else { 
        strcpy (param->errmsg, 
	    "ISIS_WORKURL configuration variable is undefined!");
	return (-1);
    }
    
    if (config_value("ISIS_WORKDIR") != (char *)NULL)
        strcpy(directory, config_value("ISIS_WORKDIR"));
    else { 
        strcpy (param->errmsg, 
	    "ISIS_WORKDIR configuration variable is undefined!");
	return (-1);
    }

   
/* 
    Extract input workspace name
*/
    param->workspace[0] = '\0';
    if(keyword_exists("workspace")) {
        if (keyword_value("workspace") != (char *)NULL) 
	    strcpy (param->workspace, strtrim(keyword_value("workspace")));
    }
    
    if ((int)strlen(param->workspace) == 0) {
        strcpy (param->errmsg, "No workspace specified.");
	return (-1);
    }

    sprintf (param->directory, "%s/%s", directory, param->workspace);
    sprintf (param->baseURL, "%s/%s", baseurl, param->workspace);

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "directory= [%s]\n", param->directory);
	fprintf (fdebug, "baseurl= [%s]\n", param->baseURL);
        fflush (fdebug);
    }
  

    istatus = chdir(directory);

    if(istatus != 0) {
        strcpy (param->errmsg, "Cannot chdir to work directory.");
    }

    
    param->jsonStr[0] = '\0';
    if(keyword_exists("json")) {
        if (keyword_value("json") != (char *)NULL) 
            strcpy(param->jsonStr, strtrim(keyword_value("json")));
    }

    if ((int)strlen(param->jsonStr) == 0) {
        strcpy (param->errmsg, "No JSON structure.");
	return (-1);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "jsonstr= [%s]\n", param->jsonStr);
        fflush (fdebug);
    }
    
/*
    Save jsonstr to write to a file later
*/
    strcpy (param->jsonOrig, param->jsonStr);


/*
    Compress newline/carriage return caracters in JSON for parsing
*/
    offset = 0;

    for(i=0; i<strlen(param->jsonStr); ++i)
    {
        if ((param->jsonStr[i] == '\r') || 
	    (param->jsonStr[i] == '\n')) {
            
	    ++offset;
        }
        else {
            param->jsonStr[i-offset] = param->jsonStr[i];
	}
    }

    param->jsonStr[i] = '\0';


/* 
    Parse the JSON: common parameters 
*/
    param->imageFile[0] = '\0';
   
    str[0] = '\0';
    if (json_val (param->jsonStr, "imageFile", str)  != (char *)NULL) 
    {
        strcpy (param->imageFile, str);
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imageFile= [%s]\n", param->imageFile);
        fflush (fdebug);
    }
    
    strcpy (param->imageType, "jpeg");
    str[0] = '\0';
    if (json_val (param->jsonStr, "imageType", str) != (char *)NULL) 
    {
        strcpy (param->imageType, str);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imageType= [%s]\n", param->imageType);
        fflush (fdebug);
    }

    param->canvasWidth = 512;
    param->canvasHeight = 512;
    param->refWidth = 0;
    param->refHeight = 0;

    str[0] = '\0';
    if (json_val (param->jsonStr, "canvasWidth", str) != (char *)NULL) 
    {
        strcpy (param->canvasWidthStr, str);

        istatus = str2Integer (str, &param->canvasWidth, param->errmsg);
        if (istatus == -1) {
            param->canvasWidth = 512;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "canvaswidth= [%d]\n", param->canvasWidth);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "canvasHeight", str) != (char *)NULL) 
    {
        strcpy (param->canvasHeightStr, str);
        
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "canvaswidthStr= [%s]\n", str);
            fflush (fdebug);
        }
     
        istatus = str2Integer (str, &param->canvasHeight, param->errmsg);
        if (istatus == -1) {
            param->canvasHeight = 512;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "canvasheight= [%d]\n", param->canvasHeight);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "refWidth", str) != (char *)NULL) 
    {
        strcpy (param->refWidthStr, str);
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "refwidthStr= [%s]\n", str);
            fflush (fdebug);
        }

        istatus = str2Integer (str, &param->refWidth, param->errmsg);
        if (istatus == -1) {
            param->refWidth = 0;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "refwidth= [%d]\n", param->refWidth);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "refHeight", str) != (char *)NULL) 
    {
        strcpy (param->refHeightStr, str);

        istatus = str2Integer (str, &param->refHeight, param->errmsg);
        if (istatus == -1) {
            param->refHeight = 0;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "refheight= [%d]\n", param->refHeight);
        fflush (fdebug);
    }

    
    param->imageWidth = 0;
    param->imageHeight = 0;
    param->cutoutWidth = 0;
    param->cutoutHeight = 0;
    param->ss = 0.;
    param->sl = 0.;
	
    if (json_val (param->jsonStr, "imageWidth", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->imageWidth, param->errmsg);
        if (istatus == -1) {
            param->imageWidth = 0;
	}
    }
    if (json_val (param->jsonStr, "imageHeight", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->imageHeight, param->errmsg);
        if (istatus == -1) {
            param->imageHeight = 0;
	}
    }
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imagewidth= [%d]\n", param->imageWidth);
	fprintf (fdebug, "imageheight= [%d]\n", param->imageHeight);
        fflush (fdebug);
    }

    if (json_val (param->jsonStr, "cutoutWidth", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->cutoutWidth, param->errmsg);
        if (istatus == -1) {
            param->cutoutWidth = 0;
	}
    }
    
    if (json_val (param->jsonStr, "cutoutHeight", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->cutoutHeight, param->errmsg);
        if (istatus == -1) {
            param->cutoutHeight = 0;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "cutoutwidth= [%d]\n", param->cutoutWidth);
	fprintf (fdebug, "cutoutheight= [%d]\n", param->cutoutHeight);
        fflush (fdebug);
    }
  
/*
    Start pixel of the cutout image from the original image
*/
    str[0] = '\0';
    if (json_val (param->jsonStr, "ss", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->ss, param->errmsg);
        if (istatus == -1) {
            param->ss = 0.;
	}
    }
	
    str[0] = '\0';
    if (json_val (param->jsonStr, "sl", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->sl, param->errmsg);
        if (istatus == -1) {
            param->sl = 0.;
	}
    }
    
    param->nowcs = 0;
    str[0] = '\0';
    if (json_val (param->jsonStr, "nowcs", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->nowcs, param->errmsg);
        if (istatus == -1) {
            param->nowcs = 0;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "nowcs= [%d]\n", param->nowcs);
        fflush (fdebug);
    }

    
    param->srcpick = 0;
    str[0] = '\0';
    if (json_val (param->jsonStr, "srcpick", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->srcpick, param->errmsg);
        if (istatus == -1) {
            param->srcpick = 0;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "srcpick= [%d]\n", param->srcpick);
        fflush (fdebug);
    }

   
    param->zoomfactor = 1.;
    str[0] = '\0';
    if (json_val (param->jsonStr, "scale", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->zoomfactor, param->errmsg);
        if (istatus == -1) {
            param->zoomfactor = 1.;
	}
    }

    param->xmin = 0.;
    param->xmax = 0.;
    param->ymin = 0.;
    param->ymax = 0.;

    str[0] = '\0';
    if (json_val (param->jsonStr, "xmin", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->xmin, param->errmsg);
        if (istatus == -1) {
            param->xmin = 0.;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xminStr= [%s]\n", str);
	fprintf (fdebug, "xmin= [%lf]\n", param->xmin);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "xmax", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->xmax, param->errmsg);
        if (istatus == -1) {
            param->xmax = 0.;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xmaxstr= [%s]\n", str);
	fprintf (fdebug, "xmax= [%lf]\n", param->xmax);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "ymin", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->ymin, param->errmsg);
        if (istatus == -1) {
            param->ymin = 0.;
	}
    }
    
    if (param->xmin > param->xmax)
    {
	dblval = param->xmin;
        param->xmin = param->xmax;
        param->xmax = dblval;
    }

    if (param->ymin > param->ymax)
    {
        dblval =  param->ymin;
        param->ymin = param->ymax;
        param->ymax = dblval;
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "yminStr= [%s]\n", str);
	fprintf (fdebug, "ymin= [%lf]\n", param->ymin);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "ymax", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->ymax, param->errmsg);
        if (istatus == -1) {
            param->ymax = 0.;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ymaxStr= [%s]\n", str);
	fprintf (fdebug, "ymax= [%lf]\n", param->ymax);
        fflush (fdebug);
    }


    strcpy (param->imcsys, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "imCsys", str) != (char *)NULL) 
    {
        strcpy (param->imcsys, str);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imcsys= [%s]\n", param->imcsys);
        fflush (fdebug);
    }

    strcpy (param->objname, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "objName", str) != (char *)NULL) 
    {
        strcpy (param->objname, str);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "objname= [%s]\n", param->objname);
        fflush (fdebug);
    }

    strcpy (param->filter, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "filter", str) != (char *)NULL) 
    {
        strcpy (param->filter, str);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "filter= [%s]\n", param->filter);
        fflush (fdebug);
    }

    strcpy (param->pixscale, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "pixScale", str) != (char *)NULL) 
    {
        strcpy (param->pixscale, str);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pixscale= [%s]\n", param->pixscale);
        fflush (fdebug);
    }

    strcpy (param->imcursormode, "");

    if (json_val (param->jsonStr, "imcursorMode", str) 
	!= (char *)NULL) 
    {
        strcpy (param->imcursormode, str);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imcursormode= [%s]\n", param->imcursormode);
        fflush (fdebug);
    }


/*
    Subimage region to be zoomed: xmin, xmax, ymin, ymax
*/
    param->xs = 0.;
    param->xs = 0.;
    param->ye = 0.;
    param->ye = 0.;

    str[0] = '\0';
    if (json_val (param->jsonStr, "xs", str) != (char *)NULL) 
    {
            istatus = str2Double (str, &param->xs, param->errmsg);
            if (istatus == -1) {
                param->xs = 0.;
	    }
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "xsStr= [%s]\n", str);
	    fprintf (fdebug, "xs= [%lf]\n", param->xs);
            fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "xe", str) != (char *)NULL) 
    {
            istatus = str2Double (str, &param->xe, param->errmsg);
            if (istatus == -1) {
                param->xe = 0.;
	    }
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "xestr= [%s]\n", str);
	    fprintf (fdebug, "xe= [%lf]\n", param->xe);
            fflush (fdebug);
    }

    str[0] = '\0';
        if (json_val (param->jsonStr, "ys", str) != (char *)NULL) 
    {
            istatus = str2Double (str, &param->ys, param->errmsg);
            if (istatus == -1) {
                param->ys = 0.;
	    }
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ysStr= [%s]\n", str);
	fprintf (fdebug, "ys= [%lf]\n", param->ys);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "ye", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->ye, param->errmsg);
        if (istatus == -1) {
            param->ye = 0.;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "yeStr= [%s]\n", str);
	fprintf (fdebug, "ye= [%lf]\n", param->ye);
        fflush (fdebug);
    }

    
    if (param->xs > param->xe)
    {
	dblval = param->xs;
        param->xs = param->xe;
        param->xe = dblval;
    }

    if (param->ys > param->ye)
    {
        dblval =  param->ys;
        param->ys = param->ye;
        param->ye = dblval;
    }

/*
    Pick result: 
        xpick, ypick, rapick, decpick, sexrapick, sexdecpick, pickcsys
	are the same for all plane of color images, 
	but pickval is for each plane. 
*/
    param->xpick = -1;
    param->ypick = -1;
    param->rapick = 0.;
    param->decpick = 0.;
    strcpy (param->sexrapick, "");
    strcpy (param->sexdecpick, "");
    param->pickval = 0.;

    str[0] = '\0';
    if (json_val (param->jsonStr, "xPick", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->xpick, param->errmsg);
        if (istatus == -1) {
            param->xpick = -1;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xpick= [%d]\n", param->xpick);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "yPick", str) != (char *)NULL) 
    {
        istatus = str2Integer (str, &param->ypick, param->errmsg);
        if (istatus == -1) {
            param->ypick = -1;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ypick= [%d]\n", param->ypick);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "raPick", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->rapick, param->errmsg);
        if (istatus == -1) {
            param->rapick = -1;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "rapick= [%lf]\n", param->rapick);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "decPick", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->decpick, param->errmsg);
        if (istatus == -1) {
            param->decpick = -1;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "decpick= [%lf]\n", param->decpick);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "sexraPick", str) != (char *)NULL) 
    {
        strcpy (param->sexrapick, str);
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "sexrapick= [%s]\n", param->sexrapick);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "sexdecPick", str) != (char *)NULL) 
    {
        strcpy (param->sexdecpick, str);
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "sexdecpick= [%s]\n", param->sexdecpick);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "pickValue", str) != (char *)NULL) 
    {
        istatus = str2Double (str, &param->pickval, param->errmsg);
        if (istatus == -1) {
            param->pickval = 0.;
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pickval= [%lf]\n", param->pickval);
        fflush (fdebug);
    }
    
    strcpy (param->pickcsys, "eq j2000");
    str[0] = '\0';
    if (json_val (param->jsonStr, "pickcsys", str) != (char *)NULL) 
    {
        strcpy (param->pickcsys, str);
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pickcsysStrr= [%s]\n", str);
	fprintf (fdebug, "pickcsys= [%s]\n", param->pickcsys);
        fflush (fdebug);
    }


/*
    Extract imcubefile info from jsonstr
*/
    param->isimcube = 0;
    strcpy (param->imcubepathOrig, "");
    param->imcubefile[0] = '\0';
    strcpy (param->imcubemode, "ave");
    strcpy (param->waveplottype, "");
    strcpy (param->plotjsonfile, "");
    strcpy (param->ctype3, "");
    
    param->nfitsplane = 0; 
    param->nplaneave = 1; 
    param->centerplane = 1;
    param->startplane = 1;
    param->endplane = 1;
    param->crval3 = 0.;
    param->cdelt3 = 0.;
    
    if (json_val (param->jsonStr, "imcubeFile", str) != (char *)NULL) 
    {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here0: imcubefile\n");
            fflush (fdebug);
        }

	param->isimcube = 1;

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcubeFile.fitsFileOrig", str) 
            != (char *)NULL) 
        {
            strcpy (param->imcubepathOrig, str);
        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "imcubepathOrig= [%s]\n", param->imcubepathOrig);
            fflush (fdebug);
        }

        if (json_val (param->jsonStr, "imcubeFile.fitsFile", str) 
	    != (char *)NULL) 
	{
            strcpy (param->imcubefile, str);
        }

        if (json_val (param->jsonStr, "imcubeFile.imcubeMode", str) 
	    != (char *)NULL) 
	{
            strcpy (param->imcubemode, str);
        }
        
        if (json_val (param->jsonStr, "imcubeFile.waveplotType", str) 
	    != (char *)NULL) 
	{
            strcpy (param->waveplottype, str);
        }
        
	if (json_val (param->jsonStr, "imcubeFile.plotjsonFile", str) 
	    != (char *)NULL) 
	{
            strcpy (param->plotjsonfile, str);
        }


        if (json_val (param->jsonStr, "imcubeFile.planeNum", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->nfitsplane, param->errmsg);
            if (istatus == -1) {
                param->nfitsplane = 0;
	    }
        }
        
        if (json_val (param->jsonStr, "imcubeFile.nplaneAve", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->nplaneave, param->errmsg);
            if (istatus == -1) {
                param->nplaneave = 1;
	    }
        }
        
	if (json_val (param->jsonStr, "imcubeFile.centerPlane", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->centerplane, param->errmsg);
            if (istatus == -1) {
                param->centerplane = 1;
	    }
        }
	if (json_val (param->jsonStr, "imcubeFile.startPlane", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->startplane, param->errmsg);
            if (istatus == -1) {
                param->startplane = 1;
	    }
        }
	if (json_val (param->jsonStr, "imcubeFile.endPlane", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->endplane, param->errmsg);
            if (istatus == -1) {
                param->endplane = 1;
	    }
        }

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcubeFile.ctype3", str) != (char *)NULL)
        {
            strcpy (param->ctype3, str);
        }

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcubeFile.crval3", str) != (char *)NULL)
        {
            istatus = str2Double (str, &param->crval3, param->errmsg);
            if (istatus < 0)
	        param->crval3 = 0.;
        }

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcubeFile.cdelt3", str) != (char *)NULL)
        {
            istatus = str2Double (str, &param->cdelt3, param->errmsg);
            if (istatus < 0)
	        param->cdelt3 = 0.;
        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "imcubefile= [%s]\n", param->imcubefile);
	    fprintf (fdebug, "imcubemode= [%s]\n", param->imcubemode);
	    fprintf (fdebug, "waveplottype= [%s]\n", param->waveplottype);
	    fprintf (fdebug, "nfitsplane= [%d]\n", param->nfitsplane);
	    fprintf (fdebug, "nplaneave= [%d]\n", param->nplaneave);
	    fprintf (fdebug, "centerplane= [%d]\n", param->centerplane);
	    fprintf (fdebug, "startplane= [%d]\n", param->startplane);
	    fprintf (fdebug, "endplane= [%d]\n", param->endplane);
	    fprintf (fdebug, "ctype3= [%s]\n", param->ctype3);
	    fprintf (fdebug, "crval3= [%lf]\n", param->crval3);
	    fprintf (fdebug, "cdelt3= [%lf]\n", param->cdelt3);
	    fflush (fdebug);
        }
    }


/*
    grayFile, redFile, greenFile, blueFile, and imCube specific parameters
*/
    strcpy (param->bunit, "");
    
    param->grayFile[0] = '\0'; 
    param->subsetimfile[0] = '\0'; 
    param->shrunkimfile[0] = '\0'; 
    
    strcpy (param->colorTable, "grayscale"); 
    strcpy (param->stretchMode, "linear");
    strcpy (param->stretchMin, "0.5%");
    strcpy (param->stretchMax, "99.5%");
    
    param->redFile[0] = '\0';
    param->subsetredfile[0] = '\0';
    param->shrunkredfile[0] = '\0';
    strcpy (param->redMode, "linear");
    strcpy (param->redMin, "0.5%");
    strcpy (param->redMax, "99.5%");
    
    param->greenFile[0] = '\0';
    param->subsetgrnfile[0] = '\0';
    param->shrunkgrnfile[0] = '\0';
    strcpy (param->greenMode, "linear");
    strcpy (param->greenMin, "0.5%");
    strcpy (param->greenMax, "99.5%");
    
    param->blueFile[0] = '\0';
    param->subsetbluefile[0] = '\0';
    param->shrunkbluefile[0] = '\0';
    strcpy (param->blueMode, "linear");
    strcpy (param->blueMin, "0.5%");
    strcpy (param->blueMax, "99.5%");

    param->datamin[0] = '\0';
    param->datamax[0] = '\0';
    param->minstr[0] = '\0';
    param->maxstr[0] = '\0';
    param->percminstr[0] = '\0';
    param->percmaxstr[0] = '\0';
    param->sigmaminstr[0] = '\0';
    param->sigmamaxstr[0] = '\0';

    param->xflip = 0;
    param->yflip = 0;
    
    
    if (json_val (param->jsonStr, "grayFile", str) != (char *)NULL) 
    {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "grayFile exists\n");
            fflush (fdebug);
        }

	param->iscolor = 0;
   
    }
    else if ((json_val (param->jsonStr, "redFile", str) != (char *)NULL) &&
        (json_val (param->jsonStr, "blueFile", str) != (char *)NULL)) 
    {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "redFile and blueFile exist\n");
            fflush (fdebug);
        }

	param->iscolor = 1;
    } 
    else {
         strcpy (param->errmsg, 
	     "Need either single FITS file for grayscale/pseudocolor or "
	     "red/blue files or or red/green/blue files for full color.");
         return (-1);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "iscolor= [%d]\n", param->iscolor);
        fflush (fdebug);
    }

    
    if (!param->iscolor) {

        str[0] = '\0';
        if (json_val (param->jsonStr, "grayFile.bunit", str) != (char *)NULL) 
        {
            strcpy (param->bunit, str);
        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "bunit= [%s]\n", param->bunit);
            fflush (fdebug);
        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here1: grayFile.fitsFile\n");
            fflush (fdebug);
        }

        if (json_val (param->jsonStr, "grayFile.fitsFile", str) 
	    != (char *)NULL) 
	{
            strcpy (param->grayFile, str);
        }
	
        if (json_val (param->jsonStr, "grayFile.cutoutFile", str) 
	    != (char *)NULL) 
	{
            strcpy (param->subsetimfile, str);
        }

        if (json_val (param->jsonStr, "grayFile.shrunkFile", str) 
	    != (char *)NULL) 
	{
            strcpy (param->shrunkimfile, str);
        }
	
        str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.colorTable", str) 
	    != (char *)NULL) 
	{
            strcpy (param->colorTable, str);
        }
        
        str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.stretchMode", str) 
	    != (char *)NULL) 
	{
            strcpy (param->stretchMode, str);
        }
        
	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.dataMin", str) 
            != (char *)NULL) 
	{
            strcpy (param->datamin, str);
        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.dataMax", str) 
            != (char *)NULL) 
	{
            strcpy (param->datamax, str);
        }

	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.dispMin", str) 
            != (char *)NULL) 
	{
            strcpy (param->minstr, str);
        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.dispMax", str) 
            != (char *)NULL) 
	{
            strcpy (param->maxstr, str);
        }

	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.percMin", str) 
            != (char *)NULL) 
	{
            strcpy (param->percminstr, str);
        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.percMax", str) 
            != (char *)NULL) 
	{
            strcpy (param->percmaxstr, str);
        }

	
	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.sigmaMin", str) 
            != (char *)NULL) 
	{
            strcpy (param->sigmaminstr, str);
        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.sigmaMax", str) 
            != (char *)NULL) 
	{
            strcpy (param->sigmamaxstr, str);
        }


        str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.stretchMin", str) 
            != (char *)NULL) 
	{
            strcpy (param->stretchMin, str);
        }

        str[0] = '\0';
	if (json_val (param->jsonStr, "grayFile.stretchMax", str) 
	    != (char *)NULL) 
	{
            strcpy (param->stretchMax, str);
        }
        
	str[0] = '\0';
        if (json_val (param->jsonStr, "grayFile.xflip", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->xflip, param->errmsg);
            if (istatus == -1) {
                param->xflip = 0;
	    }
        }
    
	str[0] = '\0';
        if (json_val (param->jsonStr, "grayFile.yflip", str) 
	    != (char *)NULL) 
        {
            istatus = str2Integer (str, &param->yflip, param->errmsg);
            if (istatus == -1) {
                param->yflip = 0;
	    }
        }
    
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "grayfile= [%s]\n", param->grayFile);
	    fprintf (fdebug, "subsetimfile= [%s]\n", param->subsetimfile);
	    fprintf (fdebug, "shrunkimfile= [%s]\n", param->shrunkimfile);
	    fprintf (fdebug, "imcursormode= [%s]\n", param->imcursormode);
	
	    fprintf (fdebug, "colortbl= [%s]\n", param->colorTable);
	    fprintf (fdebug, "stretchmode= [%s]\n", param->stretchMode);
	    fprintf (fdebug, "stretchmin= [%s]\n", param->stretchMin);
	    fprintf (fdebug, "stretchmax= [%s]\n", param->stretchMax);
            fflush (fdebug);
        }
    
    }
    else {
        str[0] = '\0';
        if (json_val (param->jsonStr, "redFile", str) != (char *)NULL) 
        {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "here1: redfile\n");
                fflush (fdebug);
            }

            str[0] = '\0';
            if (json_val (param->jsonStr, "redFile.fitsFile", str) 
	        != (char *)NULL)
	    {
                strcpy (param->redFile, str);
            }


            if (json_val (param->jsonStr, "redFile.cutoutFile", str) 
	        != (char *)NULL) 
	    {
                strcpy (param->subsetredfile, str);
            }

            if (json_val (param->jsonStr, "redFile.shrunkFile", str) 
	        != (char *)NULL) 
	    {
                strcpy (param->shrunkredfile, str);
            }

            strcpy (param->redMode, "linear");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "redFile.stretchMode", str) 
	        != (char *)NULL) {
                strcpy (param->redMode, str);
            }

            strcpy (param->redMin, "0.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "redFile.stretchMin", str) 
	        != (char *)NULL) {
                strcpy (param->redMin, str);
            }

            strcpy (param->redMax, "99.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "redFile.stretchMax", str) 
	        != (char *)NULL) {
                strcpy (param->redMax, str);
            }
        
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "redfile= [%s]\n", param->redFile);
	        fprintf (fdebug, "redmode= [%s]\n", param->redMode);
	        fprintf (fdebug, "redmin= [%s]\n", param->redMin);
	        fprintf (fdebug, "redmax= [%s]\n", param->redMax);
	        fprintf (fdebug, "subsetredfile= [%s]\n", param->subsetredfile);
	        fprintf (fdebug, "shrunkredfile= [%s]\n", param->shrunkredfile);
                fflush (fdebug);
            }

	    str[0] = '\0';
            if (json_val (param->jsonStr, "redFile.xflip", str) 
	        != (char *)NULL) 
            {
                istatus = str2Integer (str, &param->xflip, param->errmsg);
                if (istatus == -1) {
                    param->xflip = 0;
	        }
            }
    
	    str[0] = '\0';
            if (json_val (param->jsonStr, "redFile.yflip", str) 
	        != (char *)NULL) 
            {
                istatus = str2Integer (str, &param->yflip, param->errmsg);
                if (istatus == -1) {
                    param->yflip = 0;
	        }
            }
    
    
            str[0] = '\0';
            if (json_val (param->jsonStr, "redFile.bunit", str) 
	        != (char *)NULL) 
            {
                strcpy (param->bunit, str);
            }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "xflip= [%d]\n", param->xflip);
	        fprintf (fdebug, "yflip= [%d]\n", param->yflip);
	        fprintf (fdebug, "bunit= [%s]\n", param->bunit);
                fflush (fdebug);
            }

        }


        str[0] = '\0';
        if (json_val (param->jsonStr, "greenFile", str) != (char *)NULL) 
        {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "here2: greenfile\n");
                fflush (fdebug);
            }

            str[0] = '\0';
            if (json_val (param->jsonStr, "greenFile.fitsFile", str) 
	        != (char *)NULL) {
                strcpy (param->greenFile, str);
            }

            strcpy (param->greenMode, "linear");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "greenFile.stretchMode", str) 
	        != (char *)NULL) {
                strcpy (param->greenMode, str);
            }

            strcpy (param->greenMin, "0.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "greenFile.stretchMin", str) 
	        != (char *)NULL) {
                strcpy (param->greenMin, str);
            }

            strcpy (param->greenMax, "99.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "greenFile.stretchMax", str) 
	        != (char *)NULL) {
                strcpy (param->greenMax, str);
            }

            if (json_val (param->jsonStr, "greenFile.cutoutFile", str) 
	        != (char *)NULL) 
	    {
                strcpy (param->subsetimfile, str);
            }

            if (json_val (param->jsonStr, "greenFile.shrunkFile", str) 
	        != (char *)NULL) 
	    {
                strcpy (param->shrunkimfile, str);
            }
	

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "greenFile= [%s]\n", param->greenFile);
	        fprintf (fdebug, "greenmode= [%s]\n", param->greenMode);
	        fprintf (fdebug, "greenmin= [%s]\n", param->greenMin);
	        fprintf (fdebug, "greenmax= [%s]\n", param->greenMax);
	        fprintf (fdebug, "subsetgrnfile= [%s]\n", param->subsetgrnfile);
	        fprintf (fdebug, "shrunkgrnfile= [%s]\n", param->shrunkgrnfile);
                fflush (fdebug);
            }
        }


        str[0] = '\0';
        if (json_val (param->jsonStr, "blueFile", str) != (char *)NULL) 
        {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "here3: bluefile\n");
                fflush (fdebug);
            }

            str[0] = '\0';
            if (json_val (param->jsonStr, "blueFile.fitsFile", str) 
	        != (char *)NULL) {
                strcpy (param->blueFile, str);
            }

            strcpy (param->blueMode, "linear");

            str[0] = '\0';
	    if (json_val (param->jsonStr, "blueFile.stretchMode", str) 
	        != (char *)NULL) {
                strcpy (param->blueMode, str);
            }

            strcpy (param->blueMin, "0.5%");

            str[0] = '\0';
	    if (json_val (param->jsonStr, "blueFile.stretchMin", str) 
	        != (char *)NULL) {
                strcpy (param->blueMin, str);
            }

            strcpy (param->blueMax, "99.5%");

            str[0] = '\0';
	    if (json_val (param->jsonStr, "blueFile.stretchMax", str) 
	        != (char *)NULL) {
                strcpy (param->blueMax, str);
            }

            if (json_val (param->jsonStr, "grayFile.cutoutFile", str) 
	        != (char *)NULL) 
	    {
                strcpy (param->subsetimfile, str);
            }

            if (json_val (param->jsonStr, "grayFile.shrunkFile", str) 
	        != (char *)NULL) 
	    {
                strcpy (param->shrunkimfile, str);
            }
	
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "bluefile= [%s]\n", param->blueFile);
	        fprintf (fdebug, "bluemode= [%s]\n", param->blueMode);
	        fprintf (fdebug, "bluemin= [%s]\n", param->blueMin);
	        fprintf (fdebug, "bluemax= [%s]\n", param->blueMax);
	        fprintf (fdebug, "subsetbluefile= [%s]\n", 
		    param->subsetbluefile);
	        fprintf (fdebug, "shrunkbluefile= [%s]\n", 
		    param->shrunkbluefile);
                fflush (fdebug);
            }
        }

    }
    
/*
    Overlay parameters
*/
    i = 0;
    while (1) {
        
	sprintf (name, "overlay[%d]", i);

        str[0] = '\0';
        if (json_val (param->jsonStr, name, str) == (char *)NULL)
	    break;

	i++;
    }

    param->noverlay = i;
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "noverlay= [%d]\n", param->noverlay);
        fflush (fdebug);
    }

/*
    param->overlay 
        = (struct Overlay **)malloc(param->noverlay*sizeof (struct Overlay *));
*/

    for (l=0; l<param->noverlay; l++) {

/*
        param->overlay[l] 
            = (struct Overlay *)malloc(sizeof (struct Overlay));
*/

	sprintf (name, "overlay[%d].type", l);

        str[0] = '\0';
	if (json_val (param->jsonStr, name, str) == (char *)NULL) {
	    sprintf (param->errmsg, "No type given for overlay layer %d.", l);
	    return (-1);
	}

        strcpy (param->overlay[l].type, str);	

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "l= [%d] type= [%s]\n", 
	        l, param->overlay[l].type);
            fflush (fdebug);
        }

        
	strcpy (param->overlay[l].visible, "true");

	sprintf (name, "overlay[%d].visible", l);

        str[0] = '\0';
	if (json_val (param->jsonStr, name, str) != (char *)NULL) {
	    strcpy (param->overlay[l].visible, str);
	}

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "visible= [%s]\n", param->overlay[l].visible);
            fflush (fdebug);
        }

        
	if (strcasecmp (param->overlay[l].type, "grid") == 0) {
	    strcpy (param->overlay[l].color, "grayscale");
	}
	else if (strcasecmp (param->overlay[l].type, "catalog") == 0) {
	    strcpy (param->overlay[l].color, "green");
	}
	else if (strcasecmp (param->overlay[l].type, "iminfo") == 0) {
	    strcpy (param->overlay[l].color, "yellow");
	}
	else if (strcasecmp (param->overlay[l].type, "marker") == 0) {
	    strcpy (param->overlay[l].color, "red");
	}
	else if (strcasecmp (param->overlay[l].type, "label") == 0) {
	    strcpy (param->overlay[l].color, "red");
	}

	
	sprintf (name, "overlay[%d].color", l);
	
        str[0] = '\0';
	if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	{
	    strcpy (param->overlay[l].color, str);
	}

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "color= [%s]\n", param->overlay[l].color);
            fflush (fdebug);
        }

        if (strcasecmp (param->overlay[l].type, "grid") == 0) {

	    strcpy (param->overlay[l].coordSys, "eq j2000");

	    sprintf (name, "overlay[%d].coordSys", l);

            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].coordSys, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "grid csys= [%s]\n", 
		    param->overlay[l].coordSys);
                fflush (fdebug);
            }
	}
	else if (strcasecmp (param->overlay[l].type, "catalog") == 0) {

	    sprintf (name, "overlay[%d].dataFile", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) == (char *)NULL) 
	    {
	        sprintf (param->errmsg, 
		    "No data file given for catalog overlay -- layer %d.", l);
	        return (-1);
	    }

	    strcpy (param->overlay[l].dataFile, str);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "dataFile= [%s]\n", 
		    param->overlay[l].dataFile);
                fflush (fdebug);
            }

	    
	    strcpy (param->overlay[l].coordSys, "eq j2000");
	    sprintf (name, "overlay[%d].coordSys", l);

            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].coordSys, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "csys= [%s]\n", param->overlay[l].coordSys);
                fflush (fdebug);
            }

	    
	    strcpy (param->overlay[l].symType, "0");
	    sprintf (name, "overlay[%d].symType", l);

            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].symType, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "symtype = [%s]\n", 
		    param->overlay[l].symType);
                fflush (fdebug);
            }

	    strcpy (param->overlay[l].symSide, "3");
	    
	    sprintf (name, "overlay[%d]symSide", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].symSide, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "symside = [%s]\n", 
		    param->overlay[l].symSide);
                fflush (fdebug);
            }

	
	    strcpy (param->overlay[l].symSize, "1.0");
	    
	    sprintf (name, "overlay[%d].symSize", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].symSide, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "symsize = [%s]\n", 
		    param->overlay[l].symSize);
                fflush (fdebug);
            }
	
	    strcpy (param->overlay[l].dataCol, "");
	    
	    sprintf (name, "overlay[%d].dataCol", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].dataCol, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "datacol = [%s]\n", 
		    param->overlay[l].dataCol);
                fflush (fdebug);
            }
	
	    strcpy (param->overlay[l].dataType, "");
	    
	    sprintf (name, "overlay[%d].dataType", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].dataType, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "datatype = [%s]\n", 
		    param->overlay[l].dataType);
                fflush (fdebug);
            }
	
	    strcpy (param->overlay[l].dataRef, "");
	    
	    sprintf (name, "overlay[%d].dataRef", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        strcpy (param->overlay[l].dataRef, str);
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "dataref = [%s]\n", 
		    param->overlay[l].dataRef);
                fflush (fdebug);
            }
	
	}
	else if ((strcasecmp (param->overlay[l].type, "marker") == 0) ||
	    (strcasecmp (param->overlay[l].type, "mark") == 0)) {
         
         
	    sprintf (name, "overlay[%d].symType", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) == (char *)NULL) 
	    {
	        strcpy (param->errmsg, "No marker type given.");
		return (-1);
	    }
	        
	    strcpy (param->overlay[l].symType, str);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "symType = [%s]\n", 
		    param->overlay[l].symType);
                fflush (fdebug);
            }
	    
	    
	    sprintf (name, "overlay[%d].location", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) == (char *)NULL) 
	    {
	        strcpy (param->errmsg, "No marker location given.");
		return (-1);
	    }
	        
  	    strcpy (param->overlay[l].location, str);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "location = [%s]\n", 
		    param->overlay[l].location);
                fflush (fdebug);
            }
	    
	    sprintf (name, "overlay[%d].symSize", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) == (char *)NULL) 
	    {
	        strcpy (param->errmsg, "No marker size given.");
		return (-1);
	    }
	        
  	    strcpy (param->overlay[l].symSize, str);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "marker symSize = [%s]\n", 
		    param->overlay[l].symSize);
                fflush (fdebug);
            }
	}
	else if (strcasecmp (param->overlay[l].type, "label") == 0) {
   
	    sprintf (name, "overlay[%d].location", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) == (char *)NULL) 
	    {
	        strcpy (param->errmsg, "No label location given.");
		return (-1);
	    }
	        
  	    strcpy (param->overlay[l].location, str);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "location = [%s]\n", 
		    param->overlay[l].location);
                fflush (fdebug);
            }

	    sprintf (name, "overlay[%d].text", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) == (char *)NULL) 
	    {
	        strcpy (param->errmsg, "No label text given.");
		return (-1);
	    }
	        
  	    strcpy (param->overlay[l].text, str);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "text= [%s]\n", 
		    param->overlay[l].text);
                fflush (fdebug);
            }
	}
    
    }


/* 
    Write the JSON structure to a file  (to have a copy; we could do all the  
    parameter processing in memory just as well)                             
*/
/*
    if (param->isimcube) {

        sprintf(jsonFile, "%s/%s_current.json", param->directory,
	    param->imcubefile);
    }
    else {
        sprintf(jsonFile, "%s/%s_current.json", 
	    param->directory, param->imageFile);
    }

    fp = fopen(jsonFile, "w+");

    fprintf(fp, "%s", param->jsonOrig);
    fclose(fp);

    if ((debugfile) && (fdebug != (FILE *)NULL)) 
    {
        fprintf (fdebug, "jsonFile  = %s\n", jsonFile);
        fflush (fdebug);
    }
*/

    return (0);
}
