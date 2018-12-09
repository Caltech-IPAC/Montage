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
    char     str[1024];
    
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

    int      debugfile = 0;

    
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

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xxx: jsonstr= [%s]\n", param->jsonStr);
        fflush (fdebug);
    }
    

/* 
    Parse the JSON: common parameters 
*/
    param->helphtml[0] = '\0';
   
    str[0] = '\0';
    if (json_val (param->jsonStr, "helphtml", str)  != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            strcpy (param->helphtml, str);
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "helphtml= [%s]\n", param->helphtml);
        fflush (fdebug);
    }
    
    param->imageFile[0] = '\0';
   
    str[0] = '\0';
    if (json_val (param->jsonStr, "image.imagename", str)  != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
	    strcpy (param->imname, str);
	}
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imname= [%s]\n", param->imname);
        fflush (fdebug);
    }
    
    strcpy (param->imageType, "jpeg");
    str[0] = '\0';
    if (json_val (param->jsonStr, "image.imagetype", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            strcpy (param->imageType, str);
	}
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imageType= [%s]\n", param->imageType);
        fflush (fdebug);
    }

    strcpy (param->canvasWidthStr, "");
    strcpy (param->canvasHeightStr, "");

    param->canvasWidth = 512;
    param->canvasHeight = 512;
    param->refWidth = 0;
    param->refHeight = 0;

    str[0] = '\0';
    if (json_val (param->jsonStr, "image.canvaswidth", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            strcpy (param->canvasWidthStr, str);

            istatus = str2Integer (str, &param->canvasWidth, param->errmsg);
            if (istatus == -1) {
                param->canvasWidth = 512;
	    }
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "canvaswidth= [%d]\n", param->canvasWidth);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "image.canvasheight", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->canvasHeightStr, str);
        
            istatus = str2Integer (str, &param->canvasHeight, param->errmsg);
            if (istatus == -1) {
                param->canvasHeight = 512;
	    }
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "canvasheight= [%d]\n", param->canvasHeight);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "image.refwidth", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->refWidthStr, str);

            istatus = str2Integer (str, &param->refWidth, param->errmsg);
            if (istatus == -1) {
                param->refWidth = 0;
	    }
        }	
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "refwidth= [%d]\n", param->refWidth);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "image.refheight", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->refHeightStr, str);

            istatus = str2Integer (str, &param->refHeight, param->errmsg);
            if (istatus == -1) {
                param->refHeight = 0;
	    }
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
	
    if (json_val (param->jsonStr, "image.imagewidth", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->imageWidth, param->errmsg);
            if (istatus == -1) {
                param->imageWidth = 0;
	    }
	}

    }
    
    if (json_val (param->jsonStr, "image.imageheight", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->imageHeight, param->errmsg);
            if (istatus == -1) {
                param->imageHeight = 0;
	    }
	}

    }
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imagewidth= [%d]\n", param->imageWidth);
	fprintf (fdebug, "imageheight= [%d]\n", param->imageHeight);
        fflush (fdebug);
    }
        
    param->nowcs = -1;
    str[0] = '\0';
    
    if (json_val (param->jsonStr, "image.grayfile.nowcs", str) != (char *)NULL) 
    {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "str= [%s]\n", str);
            fflush (fdebug);
        }

	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->nowcs, param->errmsg);
            if (istatus == -1) {
                param->nowcs = -1;
	    }
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
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->srcpick, param->errmsg);
            if (istatus == -1) {
                param->srcpick = 0;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "srcpick= [%d]\n", param->srcpick);
        fflush (fdebug);
    }

   
    param->zoomfactor = 1.;
    str[0] = '\0';
    if (json_val (param->jsonStr, "image.factor", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->zoomfactor, param->errmsg);
            if (istatus == -1) {
                param->zoomfactor = 1.;
	    }
	}

    }


    param->subsetimfile[0] = '\0'; 
    if (json_val (param->jsonStr, "subimage.cutoutfile", str) 
	!= (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            strcpy (param->subsetimfile, str);
	}

    }

    if (json_val (param->jsonStr, "subimage.cutoutwidth", str) 
        != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->cutoutWidth, param->errmsg);
            if (istatus == -1) {
                param->cutoutWidth = 0;
	    }
	}

    }
    
    if (json_val (param->jsonStr, "subimage.cutoutheight", str) 
        != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->cutoutHeight, param->errmsg);
            if (istatus == -1) {
                param->cutoutHeight = 0;
	    }
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
    if (json_val (param->jsonStr, "subimage.ss", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->ss, param->errmsg);
            if (istatus == -1) {
                param->ss = 0.;
	    }
	}

    }
	
    str[0] = '\0';
    if (json_val (param->jsonStr, "subimage.sl", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->sl, param->errmsg);
            if (istatus == -1) {
                param->sl = 0.;
	    }
	}

    }

    param->xmin = 0.;
    param->xmax = 0.;
    param->ymin = 0.;
    param->ymax = 0.;

    str[0] = '\0';
    if (json_val (param->jsonStr, "subimage.xmin", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->xmin, param->errmsg);
            if (istatus == -1) {
                param->xmin = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xminStr= [%s]\n", str);
	fprintf (fdebug, "xmin= [%lf]\n", param->xmin);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "subimage.xmax", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->xmax, param->errmsg);
            if (istatus == -1) {
                param->xmax = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xmaxstr= [%s]\n", str);
	fprintf (fdebug, "xmax= [%lf]\n", param->xmax);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "subimage.ymin", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->ymin, param->errmsg);
            if (istatus == -1) {
                param->ymin = 0.;
	    }
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
    if (json_val (param->jsonStr, "subimage.ymax", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->ymax, param->errmsg);
            if (istatus == -1) {
                param->ymax = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ymaxStr= [%s]\n", str);
	fprintf (fdebug, "ymax= [%lf]\n", param->ymax);
        fflush (fdebug);
    }


    strcpy (param->imcsys, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "image.imsys", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->imcsys, str);
	}

    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "imcsys= [%s]\n", param->imcsys);
        fflush (fdebug);
    }

    strcpy (param->objname, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "objName", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->objname, str);
	}

    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "objname= [%s]\n", param->objname);
        fflush (fdebug);
    }

    strcpy (param->filter, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "filter", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->filter, str);
	}

    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "filter= [%s]\n", param->filter);
        fflush (fdebug);
    }

    strcpy (param->pixscale, "");
    str[0] = '\0';
    if (json_val (param->jsonStr, "pixScale", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->pixscale, str);
	}

    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pixscale= [%s]\n", param->pixscale);
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
    if (json_val (param->jsonStr, "cursor.xs", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->xs, param->errmsg);
            if (istatus == -1) {
                param->xs = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "xsStr= [%s]\n", str);
	    fprintf (fdebug, "xs= [%lf]\n", param->xs);
            fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.xe", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->xe, param->errmsg);
            if (istatus == -1) {
                param->xe = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "xestr= [%s]\n", str);
	    fprintf (fdebug, "xe= [%lf]\n", param->xe);
            fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.ys", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->ys, param->errmsg);
            if (istatus == -1) {
                param->ys = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ysStr= [%s]\n", str);
	fprintf (fdebug, "ys= [%lf]\n", param->ys);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.ye", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->ye, param->errmsg);
            if (istatus == -1) {
                param->ye = 0.;
	    }
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
    if (json_val (param->jsonStr, "cursor.xpick", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->xpick, param->errmsg);
            if (istatus == -1) {
                param->xpick = -1;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "xpick= [%d]\n", param->xpick);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.ypick", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Integer (str, &param->ypick, param->errmsg);
            if (istatus == -1) {
                param->ypick = -1;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ypick= [%d]\n", param->ypick);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.rapick", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->rapick, param->errmsg);
            if (istatus == -1) {
                param->rapick = -1;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "rapick= [%lf]\n", param->rapick);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.decpick", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->decpick, param->errmsg);
            if (istatus == -1) {
                param->decpick = -1;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "decpick= [%lf]\n", param->decpick);
        fflush (fdebug);
    }
    
    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.sexrapick", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->sexrapick, str);
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "sexrapick= [%s]\n", param->sexrapick);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.sexdecpick", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->sexdecpick, str);
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "sexdecpick= [%s]\n", param->sexdecpick);
        fflush (fdebug);
    }

    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.pickvalue", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            istatus = str2Double (str, &param->pickval, param->errmsg);
            if (istatus == -1) {
                param->pickval = 0.;
	    }
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pickval= [%lf]\n", param->pickval);
        fflush (fdebug);
    }
    
    strcpy (param->pickcsys, "eq j2000");
    str[0] = '\0';
    if (json_val (param->jsonStr, "cursor.picksys", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            strcpy (param->pickcsys, str);
	}

    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pickcsysStrr= [%s]\n", str);
	fprintf (fdebug, "pickcsys= [%s]\n", param->pickcsys);
        fflush (fdebug);
    }


/*
    Extract imcubefile info from jsonstr
*/
    param->cubedatadir[0] = '\0';
    param->imdatadir[0] = '\0';
    
    param->isimcube = 0;
    param->imcubefile[0] = '\0';
    strcpy (param->planeavemode, "ave");
    
    param->nfitsplane = 0; 
    param->nplaneave = 1; 
    param->centerplane = 1;
    param->startplane = 1;
    param->endplane = 1;

    if (json_val (param->jsonStr, "imcube", str) != (char *)NULL) 
    {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here0: imcube data\n");
            fflush (fdebug);
        }

	param->isimcube = 1;

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcube.datadir", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->cubedatadir, str);
	    }
        }

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcube.fitsfile", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->imcubefile, str);
	    }
        }

        str[0] = '\0';
        if (json_val (param->jsonStr, "imcube.planeavemode", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->planeavemode, str);
	    }
        }
        
        str[0] = '\0';
	if (json_val (param->jsonStr, "imcube.planenum", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
            
                istatus = str2Integer (str, &param->nfitsplane, param->errmsg);
                if (istatus == -1) {
                    param->nfitsplane = 0;
	        }
	    }

        }
        
        str[0] = '\0';
        if (json_val (param->jsonStr, "imcube.nplaneave", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
            
                istatus = str2Integer (str, &param->nplaneave, param->errmsg);
                if (istatus == -1) {
                    param->nplaneave = 1;
	        }
	    }

        }
        
        str[0] = '\0';
	if (json_val (param->jsonStr, "imcube.centerplane", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
            
                istatus = str2Integer (str, &param->centerplane, param->errmsg);
                if (istatus == -1) {
                    param->centerplane = 1;
	        }
	    }

        }
        
	str[0] = '\0';
	if (json_val (param->jsonStr, "imcube.startplane", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
            
                istatus = str2Integer (str, &param->startplane, param->errmsg);
                if (istatus == -1) {
                    param->startplane = 1;
	        }
	    }

        }
        
	str[0] = '\0';
	if (json_val (param->jsonStr, "imcube.endplane", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
            
                istatus = str2Integer (str, &param->endplane, param->errmsg);
                if (istatus == -1) {
                    param->endplane = 1;
	        }
	    }

        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "cubedatadir= [%s]\n", param->cubedatadir);
	    fprintf (fdebug, "imcubefile= [%s]\n", param->imcubefile);
	    fprintf (fdebug, "planeavemode= [%s]\n", param->planeavemode);
	    
	    fprintf (fdebug, "nfitsplane= [%d]\n", param->nfitsplane);
	    fprintf (fdebug, "nplaneave= [%d]\n", param->nplaneave);
	    fprintf (fdebug, "centerplane= [%d]\n", param->centerplane);
	    fprintf (fdebug, "startplane= [%d]\n", param->startplane);
	    fprintf (fdebug, "endplane= [%d]\n", param->endplane);
	    fflush (fdebug);
        }
    }


/*
    grayFile, redFile, greenFile, blueFile, and imCube specific parameters
*/
    strcpy (param->bunit, "");
    strcpy (param->imdatadir, "");
    
    param->grayFile[0] = '\0'; 
    param->grayPath[0] = '\0'; 
    param->shrunkimfile[0] = '\0'; 
    
    strcpy (param->colorTable, "grayscale"); 
    strcpy (param->stretchMode, "linear");
    strcpy (param->stretchMin, "0.5%");
    strcpy (param->stretchMax, "99.5%");
    
    param->redFile[0] = '\0';
    param->redPath[0] = '\0';
    param->subsetredfile[0] = '\0';
    param->shrunkredfile[0] = '\0';
    strcpy (param->redMode, "linear");
    strcpy (param->redMin, "0.5%");
    strcpy (param->redMax, "99.5%");
    
    param->greenFile[0] = '\0';
    param->greenPath[0] = '\0';
    param->subsetgrnfile[0] = '\0';
    param->shrunkgrnfile[0] = '\0';
    strcpy (param->greenMode, "linear");
    strcpy (param->greenMin, "0.5%");
    strcpy (param->greenMax, "99.5%");
    
    param->blueFile[0] = '\0';
    param->bluePath[0] = '\0';
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
    
    param->iscolor = 0;
    if (json_val (param->jsonStr, "image.type", str) != (char *)NULL) 
    {
	if ((int)strlen(str) > 0) {
            
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "str= [%s]\n", str);
                fflush (fdebug);
            }
        
	    if (strcasecmp (str, "color") == 0)
	        param->iscolor = 1;
	}

    }
   
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "iscolor= [%d]\n", param->iscolor);
        fflush (fdebug);
    }

    if (!param->iscolor) {

        str[0] = '\0';
        if (json_val (param->jsonStr, 
	    "image.grayfile.bunit", str) != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
                strcpy (param->bunit, str);
	    }
        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "bunit= [%s]\n", param->bunit);
            fflush (fdebug);
        }

        if (json_val (param->jsonStr, "image.grayfile.datadir", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->imdatadir, str);
	    }
        }
        
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here1: imdatadir= [%s]\n", param->imdatadir);
            fflush (fdebug);
        }

        if (json_val (param->jsonStr, "image.grayfile.fitsfile", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->grayFile, str);
	    }

        }
        
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here1: grayFile= [%s]\n", param->grayFile);
            fflush (fdebug);
        }

	
        if (json_val (param->jsonStr, "image.grayfile.shrunkfile", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->shrunkimfile, str);
	    }

        }
	
        str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.colortable", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->colorTable, str);
	    }

        }
        
        str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.stretchmode", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->stretchMode, str);
	    }

        }
        
	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.datamin", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->datamin, str);
	    }

        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.datamax", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->datamax, str);
	    }

        }

	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.dispmin", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->minstr, str);
	    }

        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.dispmax", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->maxstr, str);
	    }

        }

	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.percmin", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->percminstr, str);
	    }

        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.percmax", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->percmaxstr, str);
	    }

        }

	
	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.sigmamin", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->sigmaminstr, str);
	    }

        }
	str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.sigmamax", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->sigmamaxstr, str);
	    }

        }


        str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.stretchmin", str) 
            != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->stretchMin, str);
	    }

        }

        str[0] = '\0';
	if (json_val (param->jsonStr, "image.grayfile.stretchmax", str) 
	    != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
                strcpy (param->stretchMax, str);
	    }

        }
        
	str[0] = '\0';
        if (json_val (param->jsonStr, "image.grayfile.xflip", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
                istatus = str2Integer (str, &param->xflip, param->errmsg);
                if (istatus == -1) {
                    param->xflip = 0;
	        }
	    }

        }
    
	str[0] = '\0';
        if (json_val (param->jsonStr, "image.grayfile.yflip", str) 
	    != (char *)NULL) 
        {
	    if ((int)strlen(str) > 0) {
                istatus = str2Integer (str, &param->yflip, param->errmsg);
                if (istatus == -1) {
                    param->yflip = 0;
	        }
	    }

        }
    
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "grayfile= [%s]\n", param->grayFile);
	    fprintf (fdebug, "subsetimfile= [%s]\n", param->subsetimfile);
	    fprintf (fdebug, "shrunkimfile= [%s]\n", param->shrunkimfile);
	
	    fprintf (fdebug, "colortbl= [%s]\n", param->colorTable);
	    fprintf (fdebug, "stretchmode= [%s]\n", param->stretchMode);
	    fprintf (fdebug, "stretchmin= [%s]\n", param->stretchMin);
	    fprintf (fdebug, "stretchmax= [%s]\n", param->stretchMax);
            fflush (fdebug);
        }
    
    }
    else {
        str[0] = '\0';
        if (json_val (param->jsonStr, "image.redfile", str) != (char *)NULL) 
        {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "here1: redfile\n");
                fflush (fdebug);
            }

            str[0] = '\0';
            if (json_val (param->jsonStr, "image.redfile.fitsfile", str) 
	        != (char *)NULL)
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->redFile, str);
	        }

            }


            if (json_val (param->jsonStr, "image.redfile.cutoutfile", str) 
	        != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->subsetredfile, str);
	        }

            }

            if (json_val (param->jsonStr, "image.redfile.shrunkfile", str) 
	        != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->shrunkredfile, str);
	        }

            }

            strcpy (param->redMode, "linear");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.redfile.stretchmode", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->redMode, str);
	        }

            }

            strcpy (param->redMin, "0.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.redfile.stretchmin", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->redMin, str);
	        }

            }

            strcpy (param->redMax, "99.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.redfile.stretchmax", str) 
	        != (char *)NULL) {
	        
		if ((int)strlen(str) > 0) {
                    strcpy (param->redMax, str);
	        }
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
            if (json_val (param->jsonStr, "image.redfile.xflip", str) 
	        != (char *)NULL) 
            {
	        if ((int)strlen(str) > 0) {
                    istatus = str2Integer (str, &param->xflip, param->errmsg);
                    if (istatus == -1) {
                        param->xflip = 0;
	            }
	        }

            }
    
	    str[0] = '\0';
            if (json_val (param->jsonStr, "image.redfile.yflip", str) 
	        != (char *)NULL) 
            {
	        if ((int)strlen(str) > 0) {
                    istatus = str2Integer (str, &param->yflip, param->errmsg);
                    if (istatus == -1) {
                        param->yflip = 0;
	            }
	        }

            }
    
    
            str[0] = '\0';
            if (json_val (param->jsonStr, "image.redfile.bunit", str) 
	        != (char *)NULL) 
            {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->bunit, str);
	        }

            }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "xflip= [%d]\n", param->xflip);
	        fprintf (fdebug, "yflip= [%d]\n", param->yflip);
	        fprintf (fdebug, "bunit= [%s]\n", param->bunit);
                fflush (fdebug);
            }

        }


        str[0] = '\0';
        if (json_val (param->jsonStr, "image.greenfile", str) != (char *)NULL) 
        {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "here2: greenfile\n");
                fflush (fdebug);
            }

            str[0] = '\0';
            if (json_val (param->jsonStr, "image.greenfile.fitsfile", str) 
	        != (char *)NULL) {
	        
		if ((int)strlen(str) > 0) {
                    strcpy (param->greenFile, str);
	        }
            }

            strcpy (param->greenMode, "linear");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.greenfile.stretchmode", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->greenMode, str);
	        }

            }

            strcpy (param->greenMin, "0.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.greenfile.stretchmin", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->greenMin, str);
	        }

            }

            strcpy (param->greenMax, "99.5%");
            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.greenfile.stretchmax", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->greenMax, str);
	        }

            }

            if (json_val (param->jsonStr, "image.greenfile.cutoutfile", str) 
	        != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->subsetimfile, str);
	        }

            }

            if (json_val (param->jsonStr, "image.greenfile.shrunkfile", str) 
	        != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->shrunkimfile, str);
	        }

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
        if (json_val (param->jsonStr, "image.bluefile", str) != (char *)NULL) 
        {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "here3: bluefile\n");
                fflush (fdebug);
            }

            str[0] = '\0';
            if (json_val (param->jsonStr, "image.bluefile.fitsfile", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->blueFile, str);
	        }

            }

            strcpy (param->blueMode, "linear");

            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.bluefile.stretchmode", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->blueMode, str);
	        }

            }

            strcpy (param->blueMin, "0.5%");

            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.bluefile.stretchmin", str) 
	        != (char *)NULL) {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->blueMin, str);
	        }

            }

            strcpy (param->blueMax, "99.5%");

            str[0] = '\0';
	    if (json_val (param->jsonStr, "image.bluefile.stretchmax", str) 
	        != (char *)NULL) {
	        
		if ((int)strlen(str) > 0) {
                    strcpy (param->blueMax, str);
                } 
	    }

            if (json_val (param->jsonStr, "image.bluefile.cutoutfile", str) 
	        != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->subsetimfile, str);
	        }

            }

            if (json_val (param->jsonStr, "image.bluefile.shrunkfile", str) 
	        != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
                    strcpy (param->shrunkimfile, str);
	        }

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


    if (((int)strlen(param->grayFile) == 0) &&
        ((int)strlen(param->redFile) == 0) &&
        ((int)strlen(param->greenFile) == 0) &&
        ((int)strlen(param->blueFile) == 0)) {
         
	 strcpy (param->errmsg, 
	     "Need either single FITS file for grayscale/pseudocolor or "
	     "red/blue files or or red/green/blue files for full color.");
         return (-1);
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


    for (l=0; l<param->noverlay; l++) {

        strcpy (param->overlay[l].type, "");
        strcpy (param->overlay[l].coordSys, "");
        strcpy (param->overlay[l].color, "");
        strcpy (param->overlay[l].dataFile, "");
        strcpy (param->overlay[l].dataPath, "");
        strcpy (param->overlay[l].datadir, "");
        strcpy (param->overlay[l].visible, "");
        strcpy (param->overlay[l].dataCol, "");
        strcpy (param->overlay[l].dataType, "");
        strcpy (param->overlay[l].dataRef, "");
        strcpy (param->overlay[l].symType, "");
        strcpy (param->overlay[l].symSize, "");
        strcpy (param->overlay[l].symSide, "");
        strcpy (param->overlay[l].location, "");
        strcpy (param->overlay[l].text, "");


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

/*
    default color for various overlay
*/
	if (strcasecmp (param->overlay[l].type, "grid") == 0) {
	    strcpy (param->overlay[l].color, "grayscale");
	}
	else if (strcasecmp (param->overlay[l].type, "catalog") == 0) {
	    strcpy (param->overlay[l].color, "green");
	}
	else if (strcasecmp (param->overlay[l].type, "iminfo") == 0) {
	    strcpy (param->overlay[l].color, "yellow");
	}
	else if (strcasecmp (param->overlay[l].type, "mark") == 0) {
	    strcpy (param->overlay[l].color, "red");
	}
	else if (strcasecmp (param->overlay[l].type, "label") == 0) {
	    strcpy (param->overlay[l].color, "red");
	}

	
	sprintf (name, "overlay[%d].color", l);
	
        str[0] = '\0';
	if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	{
	    if ((int)strlen(str) > 0) {
	        strcpy (param->overlay[l].color, str);
	    }

	}

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "color= [%s]\n", param->overlay[l].color);
            fflush (fdebug);
        }

        if (strcasecmp (param->overlay[l].type, "grid") == 0) {

	    strcpy (param->overlay[l].coordSys, "eq j2000");

	    sprintf (name, "overlay[%d].coordsys", l);

            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
	            strcpy (param->overlay[l].coordSys, str);
	        }

	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "grid csys= [%s]\n", 
		    param->overlay[l].coordSys);
                fflush (fdebug);
            }
	}
	else if ((strcasecmp (param->overlay[l].type, "iminfo") == 0) ||
	    (strcasecmp (param->overlay[l].type, "catalog") == 0))  {

	    sprintf (name, "overlay[%d].datadir", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
	            strcpy (param->overlay[l].datadir, str);
	        }

	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "datadir= [%s]\n", 
		    param->overlay[l].datadir);
                fflush (fdebug);
            }

	    sprintf (name, "overlay[%d].datafile", l);
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
	    sprintf (name, "overlay[%d].coordsys", l);

            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
	            strcpy (param->overlay[l].coordSys, str);
	        }

	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "csys= [%s]\n", param->overlay[l].coordSys);
                fflush (fdebug);
            }

/*
    catalog specific parameters
*/
	    if (strcasecmp (param->overlay[l].type, "catalog") == 0) {

	        strcpy (param->overlay[l].symType, "0");
	        sprintf (name, "overlay[%d].symtype", l);

                str[0] = '\0';
	        if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	        {
	            if ((int)strlen(str) > 0) {
	                strcpy (param->overlay[l].symType, str);
	            }

	        }

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "symtype = [%s]\n", 
		        param->overlay[l].symType);
                    fflush (fdebug);
                }

	        strcpy (param->overlay[l].symSide, "3");
	    
	        sprintf (name, "overlay[%d]symside", l);
                str[0] = '\0';
	        if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	        {
	            if ((int)strlen(str) > 0) {
	                strcpy (param->overlay[l].symSide, str);
	            }

	        }

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "symside = [%s]\n", 
		        param->overlay[l].symSide);
                    fflush (fdebug);
                }

	
	        strcpy (param->overlay[l].symSize, "1.0");
	    
	        sprintf (name, "overlay[%d].symsize", l);
                str[0] = '\0';
	        if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	        {
	            if ((int)strlen(str) > 0) {
	                strcpy (param->overlay[l].symSide, str);
	            }

	        }

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "symsize = [%s]\n", 
		        param->overlay[l].symSize);
                    fflush (fdebug);
                }
	
	        strcpy (param->overlay[l].dataCol, "");
	    
	        sprintf (name, "overlay[%d].datacol", l);
                str[0] = '\0';
	        if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	        {
	            if ((int)strlen(str) > 0) {
	                strcpy (param->overlay[l].dataCol, str);
	            }

	        }

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "datacol = [%s]\n", 
		        param->overlay[l].dataCol);
                    fflush (fdebug);
                }
	
	        strcpy (param->overlay[l].dataType, "");
	    
	        sprintf (name, "overlay[%d].datatype", l);
                str[0] = '\0';
	        if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	        {
	            if ((int)strlen(str) > 0) {
	                strcpy (param->overlay[l].dataType, str);
	            }

	        }

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "datatype = [%s]\n", 
		        param->overlay[l].dataType);
                    fflush (fdebug);
                }
	
	        strcpy (param->overlay[l].dataRef, "");
	    
	        sprintf (name, "overlay[%d].dataref", l);
                str[0] = '\0';
	        if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	        {
	            if ((int)strlen(str) > 0) {
	                strcpy (param->overlay[l].dataRef, str);
	            }

	        }

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "dataref = [%s]\n", 
		        param->overlay[l].dataRef);
                    fflush (fdebug);
                }
            }

	}
	else if ((strcasecmp (param->overlay[l].type, "mark") == 0) ||
	    (strcasecmp (param->overlay[l].type, "mark") == 0)) {
         
         
	    sprintf (name, "overlay[%d].symtype", l);
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
	    
	    sprintf (name, "overlay[%d].symsize", l);
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
   
	    strcpy (param->overlay[l].symSize, "9");
	    
	    sprintf (name, "overlay[%d].fontscale", l);
            str[0] = '\0';
	    if (json_val (param->jsonStr, name, str) != (char *)NULL) 
	    {
	        if ((int)strlen(str) > 0) {
	            strcpy (param->overlay[l].symSize, str);
	        }

	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "symsize = [%s]\n", 
		    param->overlay[l].symSize);
                fflush (fdebug);
	    } 
	    
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

    return (0);
}
