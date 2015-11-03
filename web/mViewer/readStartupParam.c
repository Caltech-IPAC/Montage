/*
Theme: Read initial paramfile to dispparam structure.
	
Date: July 24, 2012 (Mih-seh Kong)
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include "viewerapp.h"

char *strtrim (char *);
int  str2Integer (char *strval, int *intval, char *errmsg);
int  str2Double (char *strval, double *intval, char *errmsg);

int readImfileList (char *filepath, int iscolor, struct ViewerApp *param);
int readFilelist (char *filepath, char ***filename, char *errmsg);

int checkFileExist (char *fname, char *rootname, char *suffix,
    char *directory, char *filePath);
int fileCopy (char *from, char *to, char *errmsg);


extern FILE *fp_debug;

int readStartupParam (struct ViewerApp *param, char *parampath)
{
    FILE   *fp;

    char   *cptr;

    char   datadir[1024];

    char   imcubefile[1024];
    
    char   imfile[1024];
    char   redfile[1024];
    char   grnfile[1024];
    char   bluefile[1024];

    char   line[1024], str[1024];
    char   name[1024], val[1024];
    char   rootname[1024], suffix[1024];
    char   fpath[1024];
    char   errmsg[1024];

    int    fileExist;
    int    istatus, l, i;
    
    int    debugfile = 1;


    fp = (FILE *)NULL;
    fp = fopen (parampath, "r");

    if (fp == (FILE *)NULL) {
	sprintf (param->errmsg, "Failed to open [%s]\n", parampath);
	return (-1);
    }


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nreadStartupParam: parampath= [%s]\n", parampath);
        fprintf (fp_debug, "iscolor= [%d]\n", param->iscolor);
        fflush (fp_debug);
    }
	

    param->canvaswidth = 600;
    param->canvasheight = 600;
    param->refwidth = 0;
    param->refheight = 0;
    param->tblwidth = 0;
    param->tblheight = 0;
    
    param->plotwidth = 0;
    param->plotheight = 0;

    param->plothiststyle = 0;
    
    strcpy (param->plotcolor, "green");
    strcpy (param->plotlinecolor, "green");
    strcpy (param->plotbgcolor, "white");
    strcpy (param->plotsymbol, "ball");
    strcpy (param->plotlinestyle, "none");

    param->plotxaxis[0] = '\0';
    param->plotyaxis[0] = '\0';
    param->plotxlabel[0] = '\0';
    param->plotylabel[0] = '\0';
    param->plottitle[0] = '\0';
    
    param->plotxlabeloffset[0] = '\0';
    param->plotylabeloffset[0] = '\0';

    param->isimcube = 0;
    param->nowcs = -1;
    param->nplaneave = 1;

    param->startplane = 1;
    param->endplane = 1;
    param->centerplane = 1;
    
    
    datadir[0] = '\0';
    param->imlist_gray[0] = '\0';
    param->imlist_color[0] = '\0';
    param->tbllist[0] = '\0';
    param->iminfolist[0] = '\0';
    param->divname[0] = '\0';
    param->imname[0] = '\0';
    
    param->imcubemode[0] = '\0';
    param->imcursormode[0] = '\0';
    param->waveplottype[0] = '\0';
    strcpy (param->showplot, "true");
    strcpy (param->detachplot, "false");

    param->viewtemplate[0] = '\0';
    param->viewhtml[0] = '\0';
    param->helphtml[0] = '\0';
    param->imtypehtml[0] = '\0';
    param->cursorhtml[0] = '\0';

    param->viewcgiurl[0] = '\0';
    param->tblcgiurl[0] = '\0';
    imcubefile[0] = '\0';
    imfile[0] = '\0';
    redfile[0] = '\0';
    grnfile[0] = '\0';
    bluefile[0] = '\0';

    param->ngridMax = 3;
    param->gridcolor = (char **)malloc (param->ngridMax*sizeof(char *));
    param->gridcsys = (char **)malloc (param->ngridMax*sizeof(char *));
    param->gridvis = (int *)malloc (param->ngridMax*sizeof(int));
    
   for (l=0; l<param->ngridMax; l++) {
        param->gridcolor[l] = (char *)malloc (40*sizeof(char));
	param->gridcolor[l][0] = '\0';
        param->gridcsys[l] = (char *)malloc (40*sizeof(char));
	param->gridcsys[l][0] = '\0';
   }

    param->nlabelMax = 20;
    param->labelcolor = (char **)malloc (param->nlabelMax*sizeof(char *));
    param->labeltext = (char **)malloc (param->nlabelMax*sizeof(char *));
    param->labellocstr = (char **)malloc (param->nlabelMax*sizeof(char *));
    param->labelvis = (int *)malloc (param->nlabelMax*sizeof(int));
    
   for (l=0; l<param->nlabelMax; l++) {
        param->labelcolor[l] = (char *)malloc (40*sizeof(char));
	param->labelcolor[l][0] = '\0';
        param->labellocstr[l] = (char *)malloc (100*sizeof(char));
	param->labellocstr[l][0] = '\0';
        param->labeltext[l] = (char *)malloc (200*sizeof(char));
	param->labeltext[l][0] = '\0';
   }

    param->nmarkerMax = 20;
    param->markercolor = (char **)malloc (param->nmarkerMax*sizeof(char *));
    param->markertype = (char **)malloc (param->nmarkerMax*sizeof(char *));
    param->markerlocstr = (char **)malloc (param->nmarkerMax*sizeof(char *));
    param->markersize = (char **)malloc (param->nmarkerMax*sizeof(char *));
    param->markervis = (int *)malloc (param->nmarkerMax*sizeof(int));
            
   for (l=0; l<param->nmarkerMax; l++) {
        param->markercolor[l] = (char *)malloc (40*sizeof(char));
	param->markercolor[l][0] = '\0';
        param->markertype[l] = (char *)malloc (40*sizeof(char));
	param->markertype[l][0] = '\0';
        param->markerlocstr[l] = (char *)malloc (100*sizeof(char));
	param->markerlocstr[l][0] = '\0';
        param->markersize[l] = (char *)malloc (100*sizeof(char));
	param->markersize[l][0] = '\0';
   } 
    
    param->nsrctblMax = 20;
    param->niminfoMax = 20;
    
    
/*
    Read input parameters
*/
    param->ngrid = 0;
    param->nsrctbl = 0;
    param->niminfo = 0;
    param->nlabel = 0;
    param->nmarker = 0;

    while (1) 
    {
        cptr = (char *)NULL;
        cptr = fgets (line, 1024, fp);
        
	if (cptr == (char *)NULL)
	    break;

/*
    empty lines
*/
	if ((int)strlen(line) == 0)
	    continue;
            
	cptr = strchr (line, '=');

	if (cptr == (char *)NULL) {
	    strcpy (name, strtrim(line));
            val[0] = '\0'; 
	}
	else {

	    strcpy (str, cptr+1);
	    *cptr = '\0';
	    strcpy (name, strtrim(line));
	    strcpy (val, strtrim(str));
        }

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "name= [%s] val= [%s]\n", name, val);
            fflush (fp_debug);
        }
	
	if (strcasecmp (name, "canvaswidth") == 0) {
	    istatus = str2Integer (val, &param->canvaswidth, errmsg);
	    if (istatus < 0)
		param->canvaswidth = 600;
	}
	else if (strcasecmp (name, "canvasheight") == 0) {
	    istatus = str2Integer (val, &param->canvasheight, errmsg);
	        if (istatus < 0)
		    param->canvasheight = 600;
	}
	else if (strcasecmp (name, "refwidth") == 0) {
	    istatus = str2Integer (val, &param->refwidth, errmsg);
	    if (istatus < 0)
		param->refwidth = 0;
	}
	else if (strcasecmp (name, "refheight") == 0) {
	    istatus = str2Integer (val, &param->refheight, errmsg);
	    if (istatus < 0)
		param->refheight = 0;
	}
	else if (strcasecmp (name, "iscolor") == 0) {
	    istatus = str2Integer (val, &param->iscolor, errmsg);
	    if (istatus < 0)
		param->iscolor = 0;
	}
	else if (strcasecmp (name, "imcubemode") == 0) {
	    strcpy (param->imcubemode, val);
	}
	else if (strcasecmp (name, "imcursormode") == 0) {
	    strcpy (param->imcursormode, val);
	}
	else if (strcasecmp (name, "waveplottype") == 0) {
	    strcpy (param->waveplottype, val);
	}
	else if (strcasecmp (name, "showplot") == 0) {
	    strcpy (param->showplot, val);
	}
	else if (strcasecmp (name, "detachplot") == 0) {
	    strcpy (param->detachplot, val);
	}
	else if (strcasecmp (name, "isimcube") == 0) {
	    istatus = str2Integer (val, &param->isimcube, errmsg);
	    if (istatus < 0)
		param->isimcube = 0;
	}
	else if (strcasecmp (name, "nplaneave") == 0) {
	    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "XXX read nplaneave parameter\n");
                fflush (fp_debug);
            }

	    istatus = str2Integer (val, &param->nplaneave, errmsg);
	    if (istatus < 0)
		param->nplaneave = 1;
	        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "nplaneave= [%d]\n", param->nplaneave);
                fflush (fp_debug);
            }


/*
	    strcpy (param->nplaneavestr, val);
           
	    if (strcasecmp (param->nplaneavestr, "nplane") == 0) {
                param->nplaneave = param->nfitsplane;
            
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "xxx1: nplaneave= [%d]\n", 
		        param->nplaneave);
                    fflush (fp_debug);
                }
	    
	    }
	    else {
	        istatus = str2Integer (val, &param->nplaneave, errmsg);
	        if (istatus < 0)
		    param->nplaneave = 1;
	        
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "xxx2: nplaneave= [%d]\n", 
		        param->nplaneave);
                    fflush (fp_debug);
                }
	    }
*/

	}
/*
	else if (strcasecmp (name, "implanenum") == 0) {
	    istatus = str2Integer (val, &param->implanenum, errmsg);
	    if (istatus < 0)
		param->implanenum = 1;
	}
*/
	else if (strcasecmp (name, "startplane") == 0) {
	    istatus = str2Integer (val, &param->startplane, errmsg);
	    if (istatus < 0)
		param->startplane = 1;
	}
	else if (strcasecmp (name, "endplane") == 0) {
	    istatus = str2Integer (val, &param->endplane, errmsg);
	    if (istatus < 0)
		param->endplane = 1;
	}
	else if (strcasecmp (name, "centerplane") == 0) {
	    istatus = str2Integer (val, &param->centerplane, errmsg);
	    if (istatus < 0)
		param->centerplane = 1;
	}
	else if (strcasecmp (name, "nowcs") == 0) {
	    istatus = str2Integer (val, &param->nowcs, errmsg);
	    if (istatus < 0)
		param->nowcs = -1;
	}
	else if (strcasecmp (name, "tblwidth") == 0) {
		
	    istatus = str2Integer (val, &param->tblwidth, errmsg);
	    if (istatus < 0)
	        param->tblwidth = 500;
	}
	else if (strcasecmp (name, "tblheight") == 0) {
	    istatus = str2Integer (val, &param->tblheight, errmsg);
	    if (istatus < 0)
	        param->tblheight = 400;
	}
	else if (strcasecmp (name, "gridcolor") == 0) {
            
	    strcpy (param->gridcolor[param->ngrid], strtrim(val));
                
	    if (strcasecmp (param->gridcolor[param->ngrid], "gray") == 0) 
	        strcpy (param->gridcolor[param->ngrid], "grey");
            if (strcasecmp(param->gridcolor[param->ngrid],"lightgray") == 0)
	        strcpy (param->gridcolor[param->ngrid], "lightgrey");
            if (strcasecmp(param->gridcolor[param->ngrid],"darkgray") == 0) 
	        strcpy (param->gridcolor[param->ngrid], "darkgrey");
	        
  	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "gridcolor= [%s]\n", 
		    param->gridcolor[param->ngrid]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "gridcsys") == 0) {
	
	    strcpy (param->gridcsys[param->ngrid], strtrim(val));
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "gridcsys= [%s]\n", 
		    param->gridcsys[param->ngrid]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "showgrid") == 0) {
		
	    istatus = str2Integer 
		(val, &param->gridvis[param->ngrid], errmsg);
	        
	    if (istatus < 0)
		param->gridvis[param->ngrid] = 1;

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "gridvis= [%d]\n", 
		    param->gridvis[param->ngrid]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "endgrid") == 0) {
		
	    param->ngrid++;
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "ngrid= [%d]\n", param->ngrid);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "markertype") == 0) {
	
	    strcpy (param->markertype[param->nmarker], strtrim(val));
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "markertype= [%s]\n", 
		    param->markertype[param->nmarker]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "markercolor") == 0) {
	
	    strcpy (param->markercolor[param->nmarker], strtrim(val));
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "markercolor= [%s]\n", 
		    param->markercolor[param->nmarker]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "markersize") == 0) {
		
	    strcpy (param->markersize[param->nmarker], strtrim(val));
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "markersize= [%s]\n", 
		    param->markersize[param->nmarker]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "markerlocstr") == 0) {
		
	    strcpy (param->markerlocstr[param->nmarker], strtrim(val));
	        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "markerlocstr= [%s]\n", 
		    param->markerlocstr[param->nmarker]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "showmarker") == 0) {
		
	    istatus = str2Integer 
		(val, &param->markervis[param->nmarker], errmsg);
	        
	    if (istatus < 0)
		param->markervis[param->nmarker] = 1;

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "markervis= [%d]\n", 
		    param->markervis[param->nmarker]);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "endmarker") == 0) {
		
	    param->nmarker++;
		
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "nmarker= [%d]\n", param->nmarker);
                fflush (fp_debug);
	    }
	}
	else if (strcasecmp (name, "imcubefile") == 0) {
	    strcpy (imcubefile, val);
	}
	else if (strcasecmp (name, "imfile") == 0) {
	    strcpy (imfile, val);
	}
	else if (strcasecmp (name, "redfile") == 0) {
	    strcpy (redfile, val);
	}
	else if (strcasecmp (name, "grnfile") == 0) {
	    strcpy (grnfile, val);
	}
	else if (strcasecmp (name, "bluefile") == 0) {
	    strcpy (bluefile, val);
	}
	else if (strcasecmp (name, "datadir") == 0) {
	    strcpy (datadir, val);

	    if ((int)strlen(param->datadir) == 0) {
		strcpy (param->datadir, datadir);
	    }
	}
	else if (strcasecmp (name, "divname") == 0) {
	    strcpy (param->divname, val);
	}
	else if (strcasecmp (name, "imname") == 0) {
	    strcpy (param->imname, val);
	}
	else if ((strcasecmp (name, "imlist") == 0) ||
	    (strcasecmp (name, "grayimlist") == 0)) {
	    strcpy (param->imlist_gray, val);
	}
	else if (strcasecmp (name, "colorimlist") == 0) {
	    strcpy (param->imlist_color, val);
	}
	else if (strcasecmp (name, "tbllist") == 0) {
	    strcpy (param->tbllist, val);
	}
	else if (strcasecmp (name, "iminfolist") == 0) {
	    strcpy (param->iminfolist, val);
	}
	else if (strcasecmp (name, "winname") == 0) {
	    strcpy (param->winname, val);
	}
	else if (strcasecmp (name, "viewtemplate") == 0) {
	    strcpy (param->viewtemplate, val);
	}
	else if (strcasecmp (name, "viewhtml") == 0) {
	    strcpy (param->viewhtml, val);
	}
	else if (strcasecmp (name, "helphtml") == 0) {
	    strcpy (param->helphtml, val);
	}
	else if (strcasecmp (name, "imtypehtml") == 0) {
	    strcpy (param->imtypehtml, val);
	}
	else if (strcasecmp (name, "cursorhtml") == 0) {
	    strcpy (param->cursorhtml, val);
	}
	else if (strcasecmp (name, "viewcgiurl") == 0) {
	    strcpy (param->viewcgiurl, val);
	}
	else if (strcasecmp (name, "tblcgiurl") == 0) {
	    strcpy (param->tblcgiurl, val);
	}
	else if (strcasecmp (name, "colortbl") == 0) {
	    strcpy (param->colortbl, val);
	}
	else if (strcasecmp (name, "stretchmode") == 0) {
	    strcpy (param->stretchmode, val);
	}
	else if (strcasecmp (name, "stretchmin") == 0) {
	    strcpy (param->minstretch, val);
	}
	else if (strcasecmp (name, "stretchmax") == 0) {
	    strcpy (param->maxstretch, val);
	}
	else if (strcasecmp (name, "rstretchmode") == 0) {
	    strcpy (param->redstretchmode, val);
	}
	else if (strcasecmp (name, "rstretchmin") == 0) {
	    strcpy (param->redminstretch, val);
	}
	else if (strcasecmp (name, "rstretchmax") == 0) {
	    strcpy (param->redmaxstretch, val);
	}
	else if (strcasecmp (name, "gstretchmode") == 0) {
	    strcpy (param->grnstretchmode, val);
	}
	else if (strcasecmp (name, "gstretchmin") == 0) {
	    strcpy (param->grnminstretch, val);
	}
	else if (strcasecmp (name, "gstretchmax") == 0) {
	    strcpy (param->grnmaxstretch, val);
	}
	else if (strcasecmp (name, "bstretchmode") == 0) {
	    strcpy (param->bluestretchmode, val);
	}
	else if (strcasecmp (name, "bstretchmin") == 0) {
	    strcpy (param->blueminstretch, val);
	}
	else if (strcasecmp (name, "bstretchmax") == 0) {
	    strcpy (param->bluemaxstretch, val);
	}
	else if (strcasecmp (name, "plothiststyle") == 0) {
		
		istatus = str2Integer (val, &param->plothiststyle, errmsg);
	        if (istatus < 0)
		    param->plothiststyle = 0;
	}
	else if (strcasecmp (name, "plothistvalue") == 0) {
		strcpy (param->plothistvalue, val);
	}
	else if (strcasecmp (name, "plotwidth") == 0) {
		
		istatus = str2Integer (val, &param->plotwidth, errmsg);
	        if (istatus < 0)
		    param->plotwidth = 600;
	}
	else if (strcasecmp (name, "plotheight") == 0) {
		istatus = str2Integer (val, &param->plotheight, errmsg);
	        if (istatus < 0)
		    param->plotheight = 400;
	}
	else if (strcasecmp (name, "plotcolor") == 0) {
		strcpy (param->plotcolor, val);
	}
	else if (strcasecmp (name, "plotlinecolor") == 0) {
		strcpy (param->plotlinecolor, val);
	}
	else if (strcasecmp (name, "plotsymbol") == 0) {
		strcpy (param->plotsymbol, val);
	}
	else if (strcasecmp (name, "plotlinestyle") == 0) {
		strcpy (param->plotlinestyle, val);
	}
	else if (strcasecmp (name, "plotxaxis") == 0) {
		strcpy (param->plotxaxis, val);
	}
	else if (strcasecmp (name, "plotyaxis") == 0) {
		strcpy (param->plotyaxis, val);
	}
	else if (strcasecmp (name, "plotxlabel") == 0) {
		strcpy (param->plotxlabel, val);
	}
	else if (strcasecmp (name, "plotylabel") == 0) {
		strcpy (param->plotylabel, val);
	}
	else if (strcasecmp (name, "plottitle") == 0) {
		strcpy (param->plottitle, val);
	}

    }
    fclose (fp);

/*
    if (param->refwidth == 0) 
        param->refwidth = param->canvaswidth/2;
    if (param->refheight == 0) 
        param->refheight = param->canvasheight/2;
*/

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
	fprintf (fp_debug, "divname= [%s]\n", param->divname);
	
	fprintf (fp_debug, "imfile= [%s]\n", imfile);
	fprintf (fp_debug, "redfile= [%s]\n", redfile);
	fprintf (fp_debug, "grnfile= [%s]\n", grnfile);
	fprintf (fp_debug, "bluefile= [%s]\n", bluefile);
	
	fprintf (fp_debug, "imlist_gray= [%s]\n", param->imlist_gray);
	fprintf (fp_debug, "imlist_color= [%s]\n", param->imlist_color);
	fprintf (fp_debug, "tbllist= [%s]\n", param->tbllist);
	fprintf (fp_debug, "iminfolist= [%s]\n", param->iminfolist);
        fprintf (fp_debug, "canvaswidth= [%d]\n", param->canvaswidth);
        fprintf (fp_debug, "canvasheight= [%d]\n", param->canvasheight);
        fprintf (fp_debug, "refwidth= [%d]\n", param->refwidth);
        fprintf (fp_debug, "refheight= [%d]\n", param->refheight);
	fprintf (fp_debug, "tblwidth= [%d]\n", param->tblwidth);
        fprintf (fp_debug, "tblheight= [%d]\n", param->tblheight);
        fprintf (fp_debug, "iscolor= [%d]\n", param->iscolor);
        fprintf (fp_debug, "isimcube= [%d]\n", param->isimcube);
	fprintf (fp_debug, "viewtemplate= [%s]\n", param->viewtemplate);
	fprintf (fp_debug, "viewhtml= [%s]\n", param->viewhtml);
	fprintf (fp_debug, "helphtml= [%s]\n", param->helphtml);
	fflush (fp_debug);
    
        
	fprintf (fp_debug, "colortbl= [%s]\n", param->colortbl);
	fprintf (fp_debug, "stretchmode= [%s]\n", param->stretchmode);
	fprintf (fp_debug, "minstretch= [%s]\n", param->minstretch);
	fprintf (fp_debug, "maxstretch= [%s]\n", param->maxstretch);
	fflush (fp_debug);
    }
    
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here2: imlist_grey= [%s]\n", param->imlist_gray);
        fflush (fp_debug);  
    }


/*
    Read bw imlist and check the existance of files in the list.
*/
    param->nim_gray = 0;
    if ((int)strlen(param->imlist_gray) > 0) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, 
	        "Read imlist: call checkFileExist (gray)\n");
            fflush (fp_debug);  
        }

        fileExist = checkFileExist (param->imlist_gray, rootname, suffix,
            param->directory, fpath);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned checkFileExist: fileExist= [%d]\n",
	        fileExist);
	    fprintf (fp_debug, "directory= [%s]\n", param->directory);
            fflush (fp_debug);  
        }
    
        if (!fileExist) {
            fileExist = checkFileExist (param->imlist_gray, rootname, suffix,
                param->datadir, fpath);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
		    "returned checkFileExist: fileExist= [%d]\n", fileExist);
	        fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
                fflush (fp_debug);  
            }
            
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "call readImfileList: fpath= [%s]\n", fpath);
                fflush (fp_debug);  
            }

            istatus = readImfileList (fpath, 0, param);
        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "returned readImfileList: istatus= [%d]\n", 
		    istatus);
                fflush (fp_debug);  
            }

            if (istatus == -1) {
                return (-1);
            }
        }
    }

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	
	fprintf (fp_debug, "here3: nim_gray= [%d]\n", param->nim_gray);
	for (i=0; i<param->nim_gray; i++) {
	    fprintf (fp_debug, "i= [%d] imfile= [%s]\n", i, param->imfile[i]);
	}
        fflush (fp_debug);  
    }


/* 
    Read color imlist and check the existance of files in the list.
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "imlist_color= [%s]\n", param->imlist_color);
        fflush (fp_debug);  
    }

    param->nim_color = 0;
    if ((int)strlen(param->imlist_color) > 0) {
       
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "Read imlist_color, directory= [%s]\n",
		param->directory);
            fflush (fp_debug);  
        }

        fileExist = checkFileExist (param->imlist_color, rootname, suffix,
            param->directory, fpath);

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned checkFileExist: fileExist= [%d]\n",
		fileExist);
            fflush (fp_debug);  
        }

        if (!fileExist) {
            
	    fileExist = checkFileExist (param->imlist_color, rootname, suffix,
                param->datadir, fpath);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
		    "returned checkFileExist: fileExist= [%d]\n", fileExist);
                fflush (fp_debug);  
            }
        } 

	if (fileExist) {
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "call readImfileList: fpath= [%s]\n", fpath);
                fflush (fp_debug);  
            }
	
	    istatus = readImfileList (fpath, 1, param);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "returned readImfileList: istatus= [%d]\n", 
		    istatus);
                fflush (fp_debug);  
            }

            if (istatus == -1) {
                return (-1);
            }
    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "nim_color= [%d]\n", param->nim_color);
                fflush (fp_debug);  
            }
	}
    } 

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here4: nim_color= [%d]\n", param->nim_color);
	fprintf (fp_debug, "iscolor= [%d]\n", param->iscolor);
        fflush (fp_debug);  
    }

   
    if (!param->iscolor) {
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "here5: nim_gray= [%d]\n", param->nim_gray);
	    fprintf (fp_debug, "imfile= [%s]\n", imfile);
            fflush (fp_debug);  
        }

	if (param->nim_gray == 0) {
	
	    if ((int)strlen(imfile) > 0) {

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "here6\n");
                    fflush (fp_debug);  
                }
                
		
		param->nim_gray = 1;

	        param->imfile = (char **)NULL;
                param->imfile = (char **)malloc(param->nim_gray*sizeof(char *));
                param->impath = (char **)NULL;
                param->impath = (char **)malloc(param->nim_gray*sizeof(char *));
                param->imfile[0] = (char *)NULL;
                param->imfile[0] = (char *)malloc (1024*sizeof(char));
                param->impath[0] = (char *)NULL;
                param->impath[0] = (char *)malloc (1024*sizeof(char));
	    
	        strcpy (param->imfile[0], imfile);
                sprintf (param->impath[0], "%s/%s", param->directory, imfile);
                
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "impath[0]= [%s]\n", param->impath[0]);
                    fflush (fp_debug);  
                }
                
	    }
	}
                
    }
    else {
        if (param->nim_color == 0) {
	    
	    if (((int)strlen(redfile) > 0) && 
	        ((int)strlen(grnfile) > 0) && 
	        ((int)strlen(bluefile) > 0)) { 

	        param->nim_color = 1;
	    
	        param->rimfile = (char **)NULL;
	        param->rimpath = (char **)NULL;
                param->rimfile 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->rimpath 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->rimfile[0] = (char *)NULL;
                param->rimfile[0] = (char *)malloc (1024*sizeof(char));
                param->rimpath[0] = (char *)NULL;
                param->rimpath[0] = (char *)malloc (1024*sizeof(char));

                strcpy (param->rimfile[0], redfile);
                sprintf (param->rimpath[0], 
		    "%s/%s", param->directory, redfile);
	    
	        param->gimfile = (char **)NULL;
                param->gimfile 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->gimpath 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->gimfile[0] = (char *)NULL;
                param->gimfile[0] = (char *)malloc (1024*sizeof(char));
                param->gimpath[0] = (char *)NULL;
                param->gimpath[0] = (char *)malloc (1024*sizeof(char));

                strcpy (param->gimfile[0], grnfile);
                sprintf (param->gimpath[0], 
		    "%s/%s", param->directory, grnfile);
	    
	        param->bimfile = (char **)NULL;
                param->bimfile 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->bimpath 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->bimfile[0] = (char *)NULL;
                param->bimfile[0] = (char *)malloc (1024*sizeof(char));
                param->bimpath[0] = (char *)NULL;
                param->bimpath[0] = (char *)malloc (1024*sizeof(char));
            
                strcpy (param->bimfile[0], bluefile);
                sprintf (param->bimpath[0], 
		    "%s/%s", param->directory, bluefile);
	    }
	}
    }
                
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here7\n");
        fflush (fp_debug);  
    }
                

    
/* 
    Read tbllist and check the existance of files in the list.
*/
    param->nsrctbl = 0;
    if ((int)strlen(param->tbllist) > 0) {
       
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "call checkFileExist\n");
	    fprintf (fp_debug, "tbllist= [%s]\n", param->tbllist);
            fflush (fp_debug);  
        }

        fileExist = checkFileExist (param->tbllist, rootname, suffix,
            param->directory, fpath);


	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned checkFileExist: fileExist= [%d]\n",
		fileExist);
            fflush (fp_debug);  
        }
        
	if (!fileExist) {
            fileExist = checkFileExist (param->tbllist, rootname, suffix,
                param->datadir, fpath);
   
	    if (fileExist) {

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "call readFilelist: fpath= [%s]\n", 
		        fpath);
                    fflush (fp_debug);  
                }
	
                param->nsrctbl = readFilelist (fpath, &param->srctblfile, 
	            param->errmsg);
                
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "returned readFilelist: nsrctbl= [%d]\n",
	                param->nsrctbl);
                    fflush (fp_debug);  
                }

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "returned readFilelist: nsrctbl= [%d]\n",
	                param->nsrctbl);
                    for (l=0; l<param->nsrctbl; l++) 
	                fprintf (fp_debug, "srctblfile[%d]= [%s]\n", 
		            l, param->srctblfile[l]);
                    fflush (fp_debug);  
                }
            }
        }
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here8\n");
        fflush (fp_debug);  
    }
                


    if (param->nsrctbl > 0) {

        param->srctblpath = (char **)malloc (param->nsrctbl*sizeof(char *));
        param->srctblcolor = (char **)malloc (param->nsrctbl*sizeof(char *));
        param->tblsymtype = (char **)malloc (param->nsrctbl*sizeof(char *));
        param->tblsymsize = (double *)malloc (param->nsrctbl*sizeof(double));
        param->tblsymside = (int *)malloc (param->nsrctbl*sizeof(int));
        param->datacol = (char **)malloc (param->nsrctbl*sizeof(char *));
        param->datafluxmode = (char **)malloc (param->nsrctbl*sizeof(char *));
        param->dataref = (double *)malloc (param->nsrctbl*sizeof(double));
        param->srctblvis = (int *)malloc (param->nsrctbl*sizeof(int));
            
        for (l=0; l<param->nsrctbl; l++) {
            param->srctblpath[l] = (char *)NULL;
            param->srctblpath[l] = (char *)malloc (1024*sizeof(char));
	    param->srctblpath[l][0] = '\0';
            param->srctblcolor[l] = (char *)NULL;
            param->srctblcolor[l] = (char *)malloc (40*sizeof(char));
	    param->srctblcolor[l][0] = '\0';
            param->tblsymtype[l] = (char *)NULL;
            param->tblsymtype[l] = (char *)malloc (40*sizeof(char));
	    param->tblsymtype[l][0] = '\0';
            param->datacol[l] = (char *)NULL;
            param->datacol[l] = (char *)malloc (40*sizeof(char));
	    param->datacol[l][0] = '\0';
            param->datafluxmode[l] = (char *)NULL;
            param->datafluxmode[l] = (char *)malloc (40*sizeof(char));
	    param->datafluxmode[l][0] = '\0';
        }
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here9\n");
        fflush (fp_debug);  
    }
                

/*
    Read iminfolist
*/
    param->niminfo = 0;
    if (param->iminfolist[0] != '\0') {
       
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "call checkFileExist\n");
	    fprintf (fp_debug, "iminfolist= [%s]\n", param->iminfolist);
            fflush (fp_debug);  
        }

        fileExist = checkFileExist (param->iminfolist, rootname, suffix,
            param->directory, fpath);


	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned checkFileExist: fileExist= [%d]\n",
		fileExist);
            fflush (fp_debug);  
        }
        
	if (!fileExist) {
            fileExist = checkFileExist (param->iminfolist, rootname, suffix,
                param->datadir, fpath);
        } 
	
	if (fileExist) {
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "call readFilelist\n");
                fflush (fp_debug);  
            }
	
            param->niminfo = readFilelist (fpath, &param->iminfofile, 
	        param->errmsg);
    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "returned readFilelist: niminfo= [%d]\n",
	            param->niminfo);
		for (i=0; i<param->niminfo; i++) {
	            fprintf (fp_debug, "i= [%d] iminfofile= [%s]\n", 
		        i, param->iminfofile[i]);
		}
                fflush (fp_debug);  
            }
        }
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here10\n");
        fflush (fp_debug);  
    }
                


    if (param->niminfo > 0) { 
            
        param->iminfopath = (char **)malloc (param->niminfo*sizeof(char *));
        param->iminfocolor = (char **)malloc (param->niminfo*sizeof(char *));
        param->iminfovis = (int *)malloc (param->niminfo*sizeof(int));
            
        for (l=0; l<param->niminfo; l++) {
            param->iminfopath[l] = (char *)NULL;
            param->iminfopath[l] = (char *)malloc (1024*sizeof(char));
	    param->iminfopath[l][0] = '\0';
            param->iminfocolor[l] = (char *)NULL;
            param->iminfocolor[l] = (char *)malloc (1024*sizeof(char));
	    param->iminfocolor[l][0] = '\0';
        }
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here11\n");
        fflush (fp_debug);  
    }
                
    return (0);
}
