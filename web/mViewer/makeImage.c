/*
Theme: This routine makes a JPEG image using montage depot's 

    mSubimage -- to cutout the portion of image to zoom,
    mShrink   -- to resize the image to fit the canvas,
    mViewer   -- to make the JPEG image.

Input: 
    mViewer structure containing image parameters for making a JPEG image.

Output:
    a JPEG file 

Date: June 16, 2015 (Mihseh Kong)
*/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <svc.h>

#include "mviewer.h"
#include "fitshdr.h"

int getFitshdr (char *fname, struct FitsHdr *hdr);
int fileCopy (char *frompath, char *toparah, char *errmsg);
int hexLookup (char *color, char *colorstr, char *errmsg);
int str2Integer (char *str, int *intval, char *errmsg);


static char ColortblVal[][30] = {
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


extern FILE *fdebug;

int getColortblIndx (char *color) {

    int    debugfile = 0;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "From getColortblIndx: color= [%s]\n",
	    color);
	fflush (fdebug);
    }

    int l, indx;

    if ((strcasecmp (color, "grayscale") == 0) ||
        (strcasecmp (color, "greyscale") == 0)) {
        strcpy (color, "greyscale");
    }
    else if ((strcasecmp (color, "reversegrayscale") == 0) ||
        (strcasecmp (color, "reversegreyscale") == 0)) { 
        strcpy (color, "reversegreyscale");
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "color= [%s]\n",
	    color);
	fflush (fdebug);
    }

    indx = -1;
    for (l=0; l<9; l++) {
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "l= [%d] ColortblVal= [%s]\n",
	        l, ColortblVal[l]);
	    fflush (fdebug);
        }

	if (strcasecmp (color, ColortblVal[l]) == 0) { 
	    indx = l;

/*
    Skip color table 2 and 3 in mViewer because we don't have 
    names for them.
*/
	    if (indx > 1)
	        indx += 2;

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "found: indx= [%d]\n", indx);
	        fflush (fdebug);
            }

	    break;
        } 
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "before return: indx= [%d]\n", indx);
	fflush (fdebug);
    }

    return (indx);
}


int makeImage (struct Mviewer *param)
{
    struct FitsHdr hdr;

    char   cmd[10000];
    char   paramstr[10000];
    char   refParamstr[10000];
    
    char   prog[1024];
    char   status[20];
    char   filepath[1024];

    char   redpath[1024];
    char   grnpath[1024];
    char   bluepath[1024];


    char   jpgpath[1024];
    char   refjpgpath[1024];

    char   impath[1024];
    char   shrunkimpath[1024];
    char   shrunkrefimpath[1024];

    char   shrunkredpath[1024];
    char   shrunkrefredpath[1024];
    
    char   shrunkgrnpath[1024];
    char   shrunkrefgrnpath[1024];
    
    char   shrunkbluepath[1024];
    char   shrunkrefbluepath[1024];

    char   layertype[40];
    char   color[40];
    char   visible[40];
    
    char   symtype[40];
    char   symside[40];
    
    char   str[1024];


    int    ns;
    int    nl;
    int    colortblIndx;
    int    width, height;
    
    int	   istatus, l;

    double factor, xfactor, yfactor;
    double reffactor;

    struct timeval   tp;
    struct timezone  tzp;
    double           exacttime, exacttime0;

    int    debugfile = 0;
    int    debugtime = 0;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "\nFrom makeImage\n");
	fprintf (fdebug, "iscolor= [%d]\n", param->iscolor);
	fprintf (fdebug, "canvaswidth= [%d] canvasheight= [%d]\n", 
	    param->canvasWidth, param->canvasHeight);
	fprintf (fdebug, "refWidth= [%d] refHeight= [%d]\n", 
	    param->refWidth, param->refHeight);
	fprintf (fdebug, "cmd= [%s]\n", param->cmd);
	fflush (fdebug);
    }

/*
    Rescale the subset image to fit the canvas
*/
    
    if (!param->iscolor) {

	if ((strcasecmp (param->cmd, "init") == 0) ||
	    (strcasecmp (param->cmd, "replaceimage") == 0) ||
	    (strcasecmp (param->cmd, "resetzoom") == 0))
        {
            sprintf (impath, "%s/%s", param->directory, param->grayFile);
	}
        else {
	    if ((int)strlen(param->subsetimfile) > 0) 
	    {
                sprintf (impath, "%s/%s", param->directory, 
		    param->subsetimfile);
	    }
	    else {
                sprintf (impath, "%s/%s", param->directory, param->grayFile);
	    }
	}
    }
    else {
	if ((strcasecmp (param->cmd, "init") == 0) ||
	    (strcasecmp (param->cmd, "replaceimage") == 0) ||
	    (strcasecmp (param->cmd, "resetzoom") == 0))
        {
            sprintf (impath, "%s/%s", param->directory, param->redFile);
	}
        else {
	    if ((int)strlen(param->subsetimfile) > 0) 
	    {
                sprintf (impath, "%s/%s", param->directory, 
		    param->subsetredfile);
	    }
	    else {
                sprintf (impath, "%s/%s", param->directory, param->redFile);
	    }
	}
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "impath= [%s]\n", impath);
	fflush (fdebug);
    }
 
    
    istatus = getFitshdr (impath, &hdr);

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "returned getFitshdr: istatus= [%d]\n", istatus);
	fflush (fdebug);
    }
 
    if (istatus == -1) {
	sprintf (param->errmsg, 
	    "Failed to extract FITS image header, errmsg= [%s]", hdr.errmsg);
	return (-1);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ns= [%d] nl= [%d]\n", hdr.ns, hdr.nl);
	fprintf (fdebug, "csysstr= [%s]\n", hdr.csysstr);
	fprintf (fdebug, "epochstr= [%s]\n", hdr.epochstr);
	fflush (fdebug);
    }

    if (param->nowcs == -1) {
        param->nowcs = hdr.nowcs;
    }

    param->ns = hdr.ns;
    param->nl = hdr.nl;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "ns= [%d] nl= [%d]\n", param->ns, param->nl); 
	fflush (fdebug);
    }



/*
    1. Compute zoom factor based on the image size and canvas size
    2. Run mShrink to make image and refimg 
*/
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "\nrefwidth= [%d] refheight= [%d]\n", 
	    param->refWidth, param->refHeight);
	fflush (fdebug);
    }

    param->nim = 1;
    if ((param->refWidth > 0) && (param->refHeight > 0)) {
        param->nim = 2;
    }

    for (l=0; l<param->nim; l++) {
	
	if (l == 0) {
	    width = param->canvasWidth;
	    height = param->canvasHeight;
	    ns = param->ns;
	    nl = param->nl;
	}
	else {
	    width = param->refWidth;
	    height = param->refHeight;
	    ns = param->imageWidth;
	    nl = param->imageHeight;
	}
   
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "width= [%d] height= [%d]\n", width, height);
	    fprintf (fdebug, "ns= [%d] nl= [%d]\n", ns, nl);
	    fflush (fdebug);
        }

        factor = 1.0;
	xfactor = (double)ns / (double)width;
	yfactor = (double)nl / (double)height;

        factor = xfactor;
	if (yfactor > factor)
	    factor = yfactor;

        if (!param->iscolor) {
	
//	    sprintf (impath, "%s/%s", param->directory, param->subsetimfile);
	    
	    if (l == 0) {
                sprintf (shrunkimpath, "%s/%s", param->directory, 
	            param->shrunkimfile);
                param->zoomfactor = 1./factor;
	    }
	    else {
                sprintf (shrunkimpath, "%s/%s", param->directory, 
	            param->shrunkRefimfile);
                param->refzoomfactor = 1./factor;
	    }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "impath= [%s]\n", impath);
	        fflush (fdebug);
            }
  
            sprintf (cmd, "mShrink %s %s %.6f", impath, shrunkimpath, factor);
           
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "mShrink cmd= [%s]\n", cmd);
	        fflush (fdebug);
            }
  
            istatus = svc_run (cmd);
                
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "returned mShrink: istatus= [%d]\n", istatus);
	        fflush (fdebug);
            }
  
            if (istatus < 0) {
	        sprintf (param->errmsg, 
		    "Failed to run mShrink: cmd= [%s]", cmd);
	        return (-1);
            }
		
	    if (svc_value("stat") == (char *)NULL) {
	        sprintf (param->errmsg, 
	            "Failed to run mShrink: cmd= [%s]", cmd);
	        return (-1);
            }
	            
	    strcpy (status, svc_value("stat"));

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "status= [%s]\n", status);
	        fflush (fdebug);
            }

            if (strcasecmp (status, "ok") != 0) {

	        sprintf (param->errmsg, 
	            "Failed to run mShrink: [%s]\n", svc_value("msg"));
        
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "errmsg= [%s]\n", param->errmsg);
	            fflush (fdebug);
	        }
	        return (-1);
            }

	}
	else {

            if ((int)strlen(param->redFile) > 0) {
	    
		sprintf (redpath, "%s/%s", param->directory, 
		    param->subsetredfile);
	        
		if (l == 0) {
                    sprintf (shrunkredpath, "%s/%s", param->directory, 
	                param->shrunkredfile);
                    param->zoomfactor = 1./factor;
	        }
	        else {
                    sprintf (shrunkredpath, "%s/%s", param->directory, 
	                param->shrunkRefredfile);
                    param->refzoomfactor = 1./factor;
	        }

                sprintf (cmd, "mShrink %s %s %.6f", redpath, shrunkredpath, 
		    factor);
           
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "mShrink cmd= [%s]\n", cmd);
	            fflush (fdebug);
                }
  
                istatus = svc_run (cmd);
                if (istatus < 0) {
	            sprintf (param->errmsg, 
		        "Failed to run mShrink: cmd= [%s]", cmd);
	            return (-1);
                }
		
	        if (svc_value("stat") == (char *)NULL) {
	            sprintf (param->errmsg, 
	                "Failed to run mShrink: cmd= [%s]", cmd);
	            return (-1);
                }
	            
	        strcpy (status, svc_value("stat"));

                if (strcasecmp (status, "ok") != 0) {

	            sprintf (param->errmsg, 
	                "Failed to run mShrink for red image: [%s]\n", 
			    svc_value("msg"));
        
	            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	                fprintf (fdebug, "errmsg= [%s]\n", param->errmsg);
	                fflush (fdebug);
	            }
	            return (-1);
                }
	    }

            if ((int)strlen(param->greenFile) > 0) {
	    
		sprintf (grnpath, "%s/%s", param->directory, 
		    param->subsetgrnfile);
	        
		if (l == 0) {
                    sprintf (shrunkgrnpath, "%s/%s", param->directory, 
	                param->shrunkgrnfile);
                    param->zoomfactor = 1./factor;
	        }
	        else {
                    sprintf (shrunkgrnpath, "%s/%s", param->directory, 
	                param->shrunkRefgrnfile);
                    param->refzoomfactor = 1./factor;
	        }

                sprintf (cmd, "mShrink %s %s %.6f", grnpath, shrunkgrnpath, 
		    factor);
           
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "mShrink cmd= [%s]\n", cmd);
	            fflush (fdebug);
                }
  
                istatus = svc_run (cmd);
                if (istatus < 0) {
	            sprintf (param->errmsg, 
		        "Failed to run mShrink: cmd= [%s]", cmd);
	            return (-1);
                }
		
	        if (svc_value("stat") == (char *)NULL) {
	            sprintf (param->errmsg, 
	                "Failed to run mShrink: cmd= [%s]", cmd);
	            return (-1);
                }
	            
	        strcpy (status, svc_value("stat"));

                if (strcasecmp (status, "ok") != 0) {

	            sprintf (param->errmsg, 
	                "Failed to run mShrink for grn image: [%s]\n", 
			    svc_value("msg"));
        
	            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	                fprintf (fdebug, "errmsg= [%s]\n", param->errmsg);
	                fflush (fdebug);
	            }
	            return (-1);
                }
	    }

            if ((int)strlen(param->blueFile) > 0) {
	    
		sprintf (bluepath, "%s/%s", param->directory, 
		    param->subsetbluefile);
	        
		if (l == 0) {
                    sprintf (shrunkbluepath, "%s/%s", param->directory, 
	                param->shrunkbluefile);
                    param->zoomfactor = 1./factor;
	        }
	        else {
                    sprintf (shrunkbluepath, "%s/%s", param->directory, 
	                param->shrunkRefbluefile);
                    param->refzoomfactor = 1./factor;
	        }

                sprintf (cmd, "mShrink %s %s %.6f", bluepath, shrunkbluepath, 
		    factor);
           
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "mShrink cmd= [%s]\n", cmd);
	            fflush (fdebug);
                }
  
                istatus = svc_run (cmd);
                if (istatus < 0) {
	            sprintf (param->errmsg, 
		        "Failed to run mShrink: cmd= [%s]", cmd);
	            return (-1);
                }
		
	        if (svc_value("stat") == (char *)NULL) {
	            sprintf (param->errmsg, 
	                "Failed to run mShrink: cmd= [%s]", cmd);
	            return (-1);
                }
	            
	        strcpy (status, svc_value("stat"));

                if (strcasecmp (status, "ok") != 0) {

	            sprintf (param->errmsg, 
	                "Failed to run mShrink for blue image: [%s]\n", 
			    svc_value("msg"));
        
	            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	                fprintf (fdebug, "errmsg= [%s]\n", param->errmsg);
	                fflush (fdebug);
	            }
	            return (-1);
                }
	    }
        }
    }


/*
    Contruct mViewer string for both jpg and refjpg
*/
    sprintf (prog, "mViewer ");
    
    paramstr[0] = '\0';
    refParamstr[0] = '\0';
    
    colortblIndx = getColortblIndx (param->colorTable);
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "colortblIndx= [%d]\n", colortblIndx);
	fflush (fdebug);
    }
 
    if (colortblIndx == -1)
        colortblIndx = 0;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "iscolor= [%d]\n", param->iscolor);
	fprintf (fdebug, "nowcs= [%d]\n", param->nowcs);
	fflush (fdebug);
    }

    
    if (!param->iscolor) {
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here1\n");
	    fflush (fdebug);
        }
      
        if (param->nowcs) {
            sprintf (paramstr, "-nowcs -ct %d ", colortblIndx);
            sprintf (refParamstr, "-nowcs -ct %d ", colortblIndx);
	}
	else {
            sprintf (paramstr, "-ct %d ", colortblIndx);
            sprintf (refParamstr, "-ct %d ", colortblIndx);
        }
        
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here2: paramstr= [%s]\n", paramstr);
	    fprintf (fdebug, "refParamstr= [%s]\n", refParamstr);
	    fprintf (fdebug, "\nstretchMin= [%s]\n", param->stretchMin);
	    fprintf (fdebug, "\nstretchMax= [%s]\n", param->stretchMax);
	    fflush (fdebug);
	}
    

	sprintf (str, "-grey %s %s %s %s ", shrunkimpath,
	    param->stretchMin, param->stretchMax, param->stretchMode);
        strcat (paramstr, str);
        
        sprintf (str, "-grey %s %s %s %s ", shrunkrefimpath,
	    param->stretchMin, param->stretchMax, param->stretchMode);
        strcat (refParamstr, str);
    
	
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "paramstr= [%s]\n", paramstr);
	    fprintf (fdebug, "refParamstr= [%s]\n", refParamstr);
	    fflush (fdebug);
	}
    }
    else {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "xxx2\n");
	    fflush (fdebug);
        }
        
	if (param->nowcs) {
            sprintf (paramstr, "-nowcs ");
            sprintf (refParamstr, "-nowcs ");
	}
 
        sprintf (str, "-red %s %s %s %s ", shrunkredpath, 
	    param->redMin, param->redMax, param->redMode);
        strcat (paramstr, str);

        sprintf (str, "-red %s %s %s %s ", shrunkrefredpath, 
	    param->redMin, param->redMax, param->redMode);
        strcat (refParamstr, str);

        sprintf (str, "-green %s %s %s %s ", shrunkgrnpath, 
	    param->greenMin, param->greenMax, param->greenMode);
        strcat (paramstr, str);
        
	sprintf (str, "-green %s %s %s %s ", shrunkrefgrnpath, 
	    param->greenMin, param->greenMax, param->greenMode);
        strcat (refParamstr, str);

        sprintf (str, "-blue %s %s %s %s ", shrunkbluepath, 
	    param->blueMin, param->blueMax, param->blueMode);
        strcat (paramstr, str);
        
	sprintf (str, "-blue %s %s %s %s ", shrunkrefbluepath, 
	    param->blueMin, param->blueMax, param->blueMode);
        strcat (refParamstr, str);
	
    }
   
 
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	
	fprintf (fdebug, "xxx4: paramstr= [%s]\n", paramstr);
	fprintf (fdebug, "refParamstr= [%s]\n", refParamstr);
	fprintf (fdebug, "noverlay= [%d]\n", param->noverlay);
	fprintf (fdebug, "nim= [%d]\n", param->nim);
	fflush (fdebug);
    }

    if (param->noverlay > 0) {
            
	for (l=0; l<param->noverlay; l++) {
		
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
	
		fprintf (fdebug, "l= [%d] type= [%s] visible= [%s]\n", 
		    l, param->overlay[l].type, param->overlay[l].visible);
	        fprintf (fdebug, "color= [%s] coordSys= [%s]\n", 
		    param->overlay[l].color, param->overlay[l].coordSys);
	        fflush (fdebug);

	        if ((strcasecmp (param->overlay[l].type, "catalog") == 0) ||
	            (strcasecmp (param->overlay[l].type, "iminfo") == 0)) {
        
	            fprintf (fdebug, "dataFile= [%s]\n", 
		        param->overlay[l].dataFile);
	        } 
		
		if (strcasecmp (param->overlay[l].type, "catalog") == 0)
		{
	            fprintf (fdebug, "catalog: symType= [%s]\n", 
	                param->overlay[l].symType);
	            fprintf (fdebug, "symSide= [%s]\n", 
	                param->overlay[l].symSide);
	            fprintf (fdebug, "symSize= [%s]\n", 
	                param->overlay[l].symSize);
	            fprintf (fdebug, "dataCol= [%s]\n", 
	                param->overlay[l].dataCol);
	            fprintf (fdebug, "dataType= [%s]\n", 
	                param->overlay[l].dataType);
	            fprintf (fdebug, "dataRef= [%s]\n", 
	                param->overlay[l].dataRef);
                }

		if ((strcasecmp (param->overlay[l].type, "marker") == 0) ||
		    (strcasecmp (param->overlay[l].type, "mark") == 0)) 
		{ 
	            fprintf (fdebug, "marker: symType= [%s]\n", 
	                param->overlay[l].symType);
	            fprintf (fdebug, "location= [%s]\n", 
	                param->overlay[l].location);
		}
		
		if (strcasecmp (param->overlay[l].type, "label") == 0) 
		{ 
	            fprintf (fdebug, "text= [%s]\n", 
	                param->overlay[l].text);
	            fprintf (fdebug, "location= [%s]\n", 
	                param->overlay[l].location);
		}

            }

/*
    Add layers
*/
	
	    strcpy (layertype, param->overlay[l].type);
	    strcpy (visible, param->overlay[l].visible);

	    if ((strcasecmp(visible, "true") != 0) && 
		(strcasecmp(visible, "yes") != 0) && 
		(strcasecmp(visible, "1") != 0)) 
	    {
	        continue;
	    }

		    
	    istatus = hexLookup (param->overlay[l].color,  color,
		    param->errmsg);
	        
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "returned hexLookup: istatus= [%d]\n", 
		    istatus);
	        fprintf (fdebug, "color= [%s]\n", color);
	            fflush (fdebug);
            }
           
            if (istatus == -1) {
/*
    Cannot find the color from iceView list, use the string as is
*/
		strcpy (color, param->overlay[l].color);
            }

	    if (strcasecmp (layertype, "grid") == 0) {
        
	        sprintf (str, "-color %s ", color);
                strcat (paramstr, str);
                strcat (refParamstr, str);

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "gridcsys= [%s]\n", 
			param->overlay[l].coordSys);
	            fprintf (fdebug, "gridvis= [%s]\n", 
			param->overlay[l].visible);
	            fflush (fdebug);
                }
	                
		sprintf (str, "-grid %s ", param->overlay[l].coordSys);
                strcat (paramstr, str);
                strcat (refParamstr, str);
	    }
	    else if (strcasecmp (layertype, "catalog") == 0) {

		sprintf (filepath, "%s/%s", param->directory, 
		    param->overlay[l].dataFile);
		        
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "filepath= [%s]\n", filepath);
	            fflush (fdebug);
                }
            
		sprintf (str, "-csys %s ", param->overlay[l].coordSys);
                strcat (paramstr, str);
                strcat (refParamstr, str);
			
                sprintf (str, "-color %s ", color);
                strcat (paramstr, str);
                strcat (refParamstr, str);
	   
	        strcpy (symtype, param->overlay[l].symType);
	        strcpy (symside, param->overlay[l].symSide);
                
		if ((int)strlen(symside) == 0) {
		    strcpy (symside, "3");
		}

		if (strcasecmp (symtype, "polygon") == 0) {
	            strcpy (symtype, "0");
		}
                else if (strcasecmp (symtype, "starred") == 0) {
	            strcpy (symtype, "1");
		}
                else if (strcasecmp (symtype, "skeletal") == 0) {
	            strcpy (symtype, "2");
		}
                else if ((strcasecmp (symtype, "box") == 0) ||
		    (strcasecmp (symtype, "square") == 0)) {
	            
		    strcpy (symtype, "0");
		    strcpy (symside, "4");
		}

	        sprintf (str, "-symbol %s %s %s ", 
		    param->overlay[l].symSize, symtype, symside);
	                
                strcat (paramstr, str);
                strcat (refParamstr, str);
	    
 
	        if ((int)strlen(param->overlay[l].dataCol) == 0) {
			    
		    sprintf (str, "-catalog %s ", filepath);
                }
	        else {
	            sprintf (str, "-catalog %s %s %s %s ", filepath, 
		        param->overlay[l].dataCol, 
			param->overlay[l].dataRef, 
			param->overlay[l].dataType);
	        }
                
		strcat (paramstr, str);
                strcat (refParamstr, str);
	    }
	    else if (strcasecmp (param->overlay[l].type, "iminfo") == 0) {
	                
		if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "here8: iminfo\n");
	            fflush (fdebug);
                }
			
		sprintf (filepath, "%s/%s", param->directory, 
		    param->overlay[l].dataFile);

	        sprintf (str, "-csys %s ", param->overlay[l].coordSys);
                
		strcat (paramstr, str);
                strcat (refParamstr, str);
			
                sprintf (str, "-color %s ", color);
                strcat (paramstr, str);
                strcat (refParamstr, str);
	    
	        sprintf (str, "-imginfo %s ", filepath);
                strcat (paramstr, str);
                strcat (refParamstr, str);
            }
	    else if ((strcasecmp (layertype, "marker") == 0) || 
	        (strcasecmp (layertype, "mark") == 0)) {


                sprintf (str, "-color %s ", color);
                strcat (paramstr, str);
                strcat (refParamstr, str);
	   
		sprintf (str, "-symbol %s %s -mark %s ", 
		    param->overlay[l].symSize, param->overlay[l].symType, 
		    param->overlay[l].location);
                
		strcat (paramstr, str);
                strcat (refParamstr, str);
	    }
	    else if (strcasecmp (layertype, "label") == 0) {

                sprintf (str, "-color %s ", color);
                strcat (paramstr, str);
                strcat (refParamstr, str);
	   
		sprintf (str, "-label %s \"%s\"", 
		    param->overlay[l].location, 
		    param->overlay[l].text); 
                
		strcat (paramstr, str);
                strcat (refParamstr, str);
	    }

	}
    }


/*
    Run mViewer
*/
    for (l=0; l<param->nim; l++) {
	
        if (l == 0) { 
	    
            sprintf (param->jpgfile, "%s.jpg", param->imageFile);
            sprintf (jpgpath, "%s/%s", param->directory, param->jpgfile);

	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "(l=0): jpgfile= [%s]\n", param->jpgfile);
	        fprintf (fdebug, "jpgpath= [%s]\n", jpgpath);
	        fflush (fdebug);
            }
	        
	    sprintf (cmd, "%s %s -out %s", prog, paramstr, jpgpath);
	}
	else {
            sprintf (param->refjpgfile, "%s_ref.jpg", param->imageFile);
            sprintf (refjpgpath, "%s/%s", param->directory, param->refjpgfile);
	    
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "(l=1): refjpgfile= [%s]\n", 
		    param->refjpgfile);
	        fprintf (fdebug, "refjpgpath= [%s]\n", refjpgpath);
	        fflush (fdebug);
            }
	        
	    sprintf (cmd, "%s %s -out %s", prog, refParamstr, refjpgpath);
        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "Run mViewer: cmd= [%s]\n", cmd);
	    fflush (fdebug);
        }
  
//        svc_debug (fdebug);

        if (debugtime) {
            gettimeofday (&tp, &tzp);
            exacttime0 = (double)tp.tv_sec + (double)tp.tv_usec/1000000.;
        }

        istatus = svc_run (cmd);

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "returned svc_run: istatus= [%d]\n", istatus);
	    fflush (fdebug);
        }
 

	strcpy (status, svc_value("stat"));

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "status= [%s]\n", status);
	    fflush (fdebug);
	}

        if (strcasecmp (status, "error") == 0) {

	    sprintf (param->errmsg, "Failed to run mViewer: %s.", 
	        svc_value("msg"));
        
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "errmsg= [%s]\n", param->errmsg);
	        fflush (fdebug);
	    }

	    return (-1);
        }
    
            if (debugtime) {
                gettimeofday (&tp, &tzp);
                exacttime = (double)tp.tv_sec + (double)tp.tv_usec/1000000.;
                fprintf (fdebug, "time (mViewer): %.6f sec\n", (
	            exacttime-exacttime0));
            }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here11: l= [%d]\n", l);
	    fflush (fdebug);
	}
        
	
	if (l == 0) {
/*
    Extract values from return structure
*/
	    param->xflip = 0;
            if (svc_value("xflip") != (char *)NULL) {
	        strcpy (param->xflipstr, svc_value("xflip"));
            }
	    istatus = str2Integer (param->xflipstr, &param->xflip, 
	        param->errmsg);
	    if (istatus != 0)
	        param->xflip = 0;

	    param->yflip = 1;
            if (svc_value("yflip") != (char *)NULL) {
	        strcpy (param->yflipstr, svc_value("yflip"));
            }
	    istatus = str2Integer (param->yflipstr, &param->yflip, 
	        param->errmsg);
	    if (istatus != 0)
	        param->yflip = 0;

	    if (param->iscolor) {

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "here12\n");
	            fflush (fdebug);
	        }
        
                param->blueminstr[0] = '\0';
                if (svc_value("bmin") != (char *)NULL) {
	            strcpy (param->blueminstr, svc_value("bmin"));
                }
                param->bluepercminstr[0] = '\0';
                if (svc_value("bminpercent") != (char *)NULL) {
	            strcpy (param->bluepercminstr, svc_value("bminpercent"));
                }
                param->bluesigmaminstr[0] = '\0';
                if (svc_value("bminsigma") != (char *)NULL) {
	            strcpy (param->bluesigmaminstr, svc_value("bminsigma"));
                }
                param->bluemaxstr[0] = '\0';
                if (svc_value("bmax") != (char *)NULL) {
	            strcpy (param->bluemaxstr, svc_value("bmax"));
                }
                param->bluepercmaxstr[0] = '\0';
                if (svc_value("bmaxpercent") != (char *)NULL) {
	            strcpy (param->bluepercmaxstr, svc_value("bmaxpercent"));
                }
                param->bluesigmamaxstr[0] = '\0';
                if (svc_value("bmaxsigma") != (char *)NULL) {
	            strcpy (param->bluesigmamaxstr, svc_value("bmaxsigma"));
                }
            
            
	        param->bluedatamin[0] = '\0';
                if (svc_value("bdatamin") != (char *)NULL) {
	            strcpy (param->bluedatamin, svc_value("bdatamin"));
                }
                param->bluedatamax[0] = '\0';
                if (svc_value("bdatamax") != (char *)NULL) {
	            strcpy (param->bluedatamax, svc_value("bdatamax"));
                }
    
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "bluedatamin= [%s] bluedatamax= [%s]\n", 
	                param->bluedatamin, param->bluedatamax);
	            fprintf (fdebug, "blueminstr= [%s] bluemaxstr= [%s]\n", 
	                param->blueminstr, param->bluemaxstr);
	            fprintf (fdebug, 
		        "bluepercminstr= [%s] bluepercmaxstr= [%s]\n", 
	                param->bluepercminstr, param->bluepercmaxstr);
	            fprintf (fdebug, 
		        "bluesigmaminstr= [%s] bluesigmamaxstr= [%s]\n", 
	                param->bluesigmaminstr, param->bluesigmamaxstr);
	            fflush (fdebug);
                }
    
                param->grnminstr[0] = '\0';
                if (svc_value("gmin") != (char *)NULL) {
	            strcpy (param->grnminstr, svc_value("gmin"));
                }
                param->grnpercminstr[0] = '\0';
                if (svc_value("gminpercent") != (char *)NULL) {
	            strcpy (param->grnpercminstr, svc_value("gminpercent"));
                }
                param->grnsigmaminstr[0] = '\0';
                if (svc_value("gminsigma") != (char *)NULL) {
	            strcpy (param->grnsigmaminstr, svc_value("gminsigma"));
                }
                param->grnmaxstr[0] = '\0';
                if (svc_value("gmax") != (char *)NULL) {
	            strcpy (param->grnmaxstr, svc_value("gmax"));
                }
                param->grnpercmaxstr[0] = '\0';
                if (svc_value("gmaxpercent") != (char *)NULL) {
	            strcpy (param->grnpercmaxstr, svc_value("gmaxpercent"));
                }
                param->grnsigmamaxstr[0] = '\0';
                if (svc_value("gmaxsigma") != (char *)NULL) {
	            strcpy (param->grnsigmamaxstr, svc_value("gmaxsigma"));
                }
	    
	        param->grndatamin[0] = '\0';
                if (svc_value("gdatamin") != (char *)NULL) {
	            strcpy (param->grndatamin, svc_value("gdatamin"));
                }
                param->grndatamax[0] = '\0';
                if (svc_value("gdatamax") != (char *)NULL) {
	            strcpy (param->grndatamax, svc_value("gdatamax"));
                    }
    
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "grndatamin= [%s] grndatamax= [%s]\n", 
	                param->grndatamin, param->grndatamax);
	            fprintf (fdebug, "grnminstr= [%s] grnmaxstr= [%s]\n", 
	                param->grnminstr, param->grnmaxstr);
	            fprintf (fdebug, 
		        "grnpercminstr= [%s] grnpercmaxstr= [%s]\n", 
	                param->grnpercminstr, param->grnpercmaxstr);
	            fflush (fdebug);
                }
    
                param->redminstr[0] = '\0';
                if (svc_value("rmin") != (char *)NULL) {
	            strcpy (param->redminstr, svc_value("rmin"));
                }
                param->redpercminstr[0] = '\0';
                if (svc_value("rminpercent") != (char *)NULL) {
	            strcpy (param->redpercminstr, svc_value("rminpercent"));
                }
                param->redsigmaminstr[0] = '\0';
                if (svc_value("rminsigma") != (char *)NULL) {
	            strcpy (param->redsigmaminstr, svc_value("rminsigma"));
                }
                param->redmaxstr[0] = '\0';
                if (svc_value("rmax") != (char *)NULL) {
	            strcpy (param->redmaxstr, svc_value("rmax"));
                }
                param->redpercmaxstr[0] = '\0';
                if (svc_value("rmaxpercent") != (char *)NULL) {
	            strcpy (param->redpercmaxstr, svc_value("rmaxpercent"));
                }
                param->redsigmamaxstr[0] = '\0';
                if (svc_value("rmaxsigma") != (char *)NULL) {
	            strcpy (param->redsigmamaxstr, svc_value("rmaxsigma"));
                }
            
	        param->reddatamin[0] = '\0';
                if (svc_value("rdatamin") != (char *)NULL) {
	            strcpy (param->reddatamin, svc_value("rdatamin"));
                }
                param->reddatamax[0] = '\0';
                if (svc_value("rdatamax") != (char *)NULL) {
	            strcpy (param->reddatamax, svc_value("rdatamax"));
                }
    
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "reddatamin= [%s] reddatamax= [%s]\n", 
	                param->reddatamin, param->reddatamax);
	            fprintf (fdebug, "redminstr= [%s] redmaxstr= [%s]\n", 
	                param->redminstr, param->redmaxstr);
	            fprintf (fdebug, 
		        "redpercminstr= [%s] redpercmaxstr= [%s]\n", 
	                param->redpercminstr, param->redpercmaxstr);
	            fflush (fdebug);
                }
    
            }
            else {
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "xxx5\n");
	            fflush (fdebug);
	        }
        
                param->datamin[0] = '\0';
                if (svc_value("datamin") != (char *)NULL) {
	            strcpy (param->datamin, svc_value("datamin"));
                }
                param->datamax[0] = '\0';
                if (svc_value("datamax") != (char *)NULL) {
	            strcpy (param->datamax, svc_value("datamax"));
                }
            
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "datamin= [%s] datamax= [%s]\n",
		        param->datamin, param->datamax);
	            fflush (fdebug);
	        }
        
    
                param->minstr[0] = '\0';
                if (svc_value("min") != (char *)NULL) {
	            strcpy (param->minstr, svc_value("min"));
                }
                param->percminstr[0] = '\0';
                if (svc_value("minpercent") != (char *)NULL) {
	            strcpy (param->percminstr, svc_value("minpercent"));
                }
                param->sigmaminstr[0] = '\0';
                if (svc_value("minsigma") != (char *)NULL) {
	            strcpy (param->sigmaminstr, svc_value("minsigma"));
                }
    
                param->maxstr[0] = '\0';
                if (svc_value("max") != (char *)NULL) {
	            strcpy (param->maxstr, svc_value("max"));
                }
                param->percmaxstr[0] = '\0';
                if (svc_value("maxpercent") != (char *)NULL) {
	            strcpy (param->percmaxstr, svc_value("maxpercent"));
                }
                param->sigmamaxstr[0] = '\0';
                if (svc_value("maxsigma") != (char *)NULL) {
	            strcpy (param->sigmamaxstr, svc_value("maxsigma"));
                }
	    
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "xxx6\n");
	            fflush (fdebug);
	        }
        
            }
	}
    
    }


    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "zoomfactor= [%lf] refzoomfactor= [%lf]\n", 
	    param->zoomfactor, param->refzoomfactor);
	fprintf (fdebug, "prog= [%s]\n", prog);
	fflush (fdebug);
    }


/*
    Compute zoomxmin, zoomxmax, zoomymin, zoomymin to draw on refimg.
*/
    if ((strcasecmp (param->cmd, "init") == 0) ||
        (strcasecmp (param->cmd, "replaceimage") == 0) ||
        (strcasecmp (param->cmd, "replaceimcubeplane") == 0)) {
        
	param->zoomxmin = 0;
	param->zoomymin = 0;
	param->zoomxmax = (int)((double)param->ns*param->refzoomfactor+0.5);
	param->zoomymax = (int)((double)param->nl*param->refzoomfactor+0.5);
    }
    else {
        reffactor = param->refzoomfactor;

/*
	param->zoomxmin = (int)(param->ss*reffactor+0.5);
	param->zoomymin = (int)(param->sl*reffactor+0.5);
	param->zoomxmax = param->zoomxmin 
	    + (int)((double)param->ns*reffactor+0.5);
	param->zoomymax = param->zoomymin 
	    + (int)((double)param->nl*reffactor+0.5);
*/
	
	param->zoomxmin = (int)(param->xmin*reffactor+0.5);
	param->zoomymin = (int)(param->ymin*reffactor+0.5);
	param->zoomxmax = (int)((double)param->xmax*reffactor+0.5);
	param->zoomymax = (int)((double)param->ymax*reffactor+0.5);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "zoomfactor= [%lf] reffactor= [%lf]\n", 
	    param->zoomfactor, reffactor);
	fprintf (fdebug, "xmin= [%lf] xmax= [%lf]\n", 
	    param->xmin, param->xmax);
	fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", 
	    param->ymin, param->ymax);
	fprintf (fdebug, "zoomxmin= [%d] zoomxmax= [%d]\n", 
	    param->zoomxmin, param->zoomxmax);
	fprintf (fdebug, "zoomymin= [%d] zoomymax= [%d]\n", 
	    param->zoomymin, param->zoomymax);
	fflush (fdebug);
    }
        

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "here13\n");
	fprintf (fdebug, "prog= [%s]\n", prog);
	fflush (fdebug);
    }


    return (0);
}


