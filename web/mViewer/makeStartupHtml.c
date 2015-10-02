/*
Theme:  Routine for making the viewer HTML pages from a template.

Date: August 18, 2015 (Mihseh Kong)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <time.h>
#include <math.h>

#include <config.h>
#include <varcmd.h>
#include <www.h>
#include <cmd.h>
#include <svc.h>

#include "viewerapp.h"


char strtrim (char *);
int str2Integer (char *strval, int *intval, char *errmsg);
int str2Double (char *strval, double *dblval, char *errmsg);

int checkFileExist (char *fname, char *rootname, char *suffix,
    char *directory, char *filePath);

int fileCopy (char *fromFile, char *toFile, char *errmsg);
int writeOptionList (char *filepath, int narr, char **optionarr,
    int selectedIndx, char *errmsg);


extern FILE *fp_debug;


int makeStartupHtml (struct ViewerApp *param)
{
    FILE           *fp;

    char           helphtmlpath[1024];
    char           viewhtmlpath[1024];
    char           viewtemplatepath[1024];
    
    char           planeoptionpath[1024];
    char           **arr;

    char           imfile[1024];
    char           fpath[1024];
    char           str[1024];
    char           *cptr;

    char           imcontroltemplatepath[1024];
    
    char           graylistoptionPath[1024];
    
    char           redlistoptionPath[1024];
    char           grnlistoptionPath[1024];
    char           bluelistoptionPath[1024];

    char           srctbloptionPath[1024];
    char           iminfooptionPath[1024];

    char           rootname[128];
    
    char           suffix[20];
    char           varstr[32768];
    char           status[32];

    int            l;
    int            istatus;
    int            selectedIndx;
    int            fileExist;
    int            viewhtmlExist;
    int            helphtmlExist;
   
   
    double         cdelt3;
    double         crval3;

    int            debugfile = 1;

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "From makeStartupHtml\n");
	fprintf (fp_debug, "isimcube= [%d]\n", param->isimcube);
	fprintf (fp_debug, "gridvis[0]= [%d]\n", param->gridvis[0]);

	fprintf (fp_debug, "redfile= [%s] grnfile= [%s] bluefile= [%s]\n",
	    param->redfile, param->grnfile, param->bluefile);

	fprintf (fp_debug, "viewhtml= [%s]\n", param->viewhtml);
	fprintf (fp_debug, "directory= [%s]\n", param->directory);
	fprintf (fp_debug, "Make viewhtml from template\n");
        fflush (fp_debug);
    }
     

/*
    Check if viewhtml is in workspace or datadir.
*/
    
    if ((int)strlen(param->helphtml) > 0) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "\nhelphtml specified in inparam file\n");
            fflush (fp_debug);
        }
     
        helphtmlExist = 0;
        helphtmlExist = checkFileExist (param->helphtml, rootname, suffix,
	    param->directory, helphtmlpath);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "helphtmlExist (directory)= [%d]\n", 
	        helphtmlExist);
	    fprintf (fp_debug, "helphtmlpath= [%s]\n", helphtmlpath);
            fflush (fp_debug);
        }

	if (!helphtmlExist) { 

            helphtmlExist = checkFileExist (param->helphtml, rootname, suffix,
	        param->datadir, fpath);
    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
	            "checkFileExist(helphtml): helphtmlExist= [%d]\n", 
		    helphtmlExist);
	        fprintf (fp_debug, "fpath= [%s]\n", fpath);
                fflush (fp_debug);
            }

	    if (helphtmlExist) {
                istatus = fileCopy (fpath, helphtmlpath, param->errmsg); 
            
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, 
		        "helphtml copied from datadir to directory");
                    fflush (fp_debug);
                }
	    }
	}

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "helphtmlpath= [%s]\n", helphtmlpath);
            fflush (fp_debug);
        }

    }
    
    if ((int)strlen(param->viewhtml) > 0) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "\nviewhtml specified in inparam file\n");
            fflush (fp_debug);
        }
     
        viewhtmlExist = 0;
        viewhtmlExist = checkFileExist (param->viewhtml, rootname, suffix,
	    param->directory, viewhtmlpath);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "viewhtmlExist (directory)= [%d]\n", 
	        viewhtmlExist);
	    fprintf (fp_debug, "viewhtmlpath= [%s]\n", viewhtmlpath);
            fflush (fp_debug);
        }

	if (!viewhtmlExist) { 

            viewhtmlExist = checkFileExist (param->viewhtml, rootname, suffix,
	        param->datadir, fpath);
    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
	            "checkFileExist(viewhtml): viewhtmlExist= [%d]\n", 
		    viewhtmlExist);
	        fprintf (fp_debug, "fpath= [%s]\n", fpath);
                fflush (fp_debug);
            }

	    if (viewhtmlExist) {
                istatus = fileCopy (fpath, viewhtmlpath, param->errmsg); 
            
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, 
		        "viewhtml copied from datadir to directory");
                    fflush (fp_debug);
                }
	    }
	}

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "viewhtmlpath= [%s]\n", viewhtmlpath);
            fflush (fp_debug);
        }

    }
    else {
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "Make viewhtml from template\n");
            fflush (fp_debug);
        }
     
        strcpy (str, param->viewtemplate);

        cptr = (char *)NULL;
        cptr = strrchr (str, '.');
	if (cptr != (char *)NULL) {
            *cptr = '\0';
	}

	sprintf (param->viewhtml, "%s.html", str);
	sprintf (param->viewhtmlpath, "%s/%s", param->directory, 
	    param->viewhtml);

	fileExist = checkFileExist (param->viewtemplate, rootname, suffix,
	    param->directory, viewtemplatepath);
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "fileExist= [%d] viewtemplate in workdir=[%s]\n",
	        fileExist, param->directory);
	    fprintf (fp_debug, "viewtemplatepath= [%s]\n", viewtemplatepath);
            fflush (fp_debug);
        }

        if (!fileExist) {
	    fileExist = checkFileExist (param->viewtemplate, rootname, suffix,
	        param->datadir, viewtemplatepath);
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
		    "viewtemplate fileExist= [%d] in datadir= [%s])\n", 
	            fileExist, param->datadir);
	        fprintf (fp_debug, "viewtemplatepath= [%s]\n", 
		    viewtemplatepath);
                fflush (fp_debug);
            }

        }


        if (!fileExist) {
	    strcpy (param->errmsg, 
	      "Failed to find view template in either datadir\n");
            return (-1);
        }
	   
/*
    Read FitsHdr to extract special keywords: objname, filter, and pixscale
    for display.
    
    If it is a fits cube, find out nfitsplane too.
    Note: Only deal with grayfile at present.
*/
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "nfitsplane= [%d]\n", param->nfitsplane);
            fflush (fp_debug);
        }


        if (param->isimcube) {

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "here1: isimcube\n");
                fflush (fp_debug);
            }

            if (param->nfitsplane > 0) {
            
                sprintf (planeoptionpath, "%s/planeoption.txt", 
		    param->directory);

		arr = (char **)malloc (param->nfitsplane*sizeof(char *));
		for (l=0; l<param->nfitsplane; l++) {
		    
		    arr[l] = (char *)malloc (10*sizeof(char));
                    sprintf (arr[l], "%d", (l+1));
		}
                
		selectedIndx = 0;

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "call writeOptionList\n");
                    fflush (fp_debug);
                }
	   
                istatus = writeOptionList (planeoptionpath,
		    param->nfitsplane, arr, selectedIndx, param->errmsg);
                
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "returned writeOptionList\n");
                    fflush (fp_debug);
                }
	   
	    }
	}


/*
    If nim_gray > 1, make imlistoption interface for replace image
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "nim_gray= [%d]\n", param->nim_gray);
            fflush (fp_debug);
        }

        if (param->nim_gray > 1) {

            sprintf (graylistoptionPath, "%s/graylistoption.txt", 
	        param->directory); 
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "graylistoptionPath= [%s]\n", 
	            graylistoptionPath);
                fflush (fp_debug);
            }

            fp = (FILE *)NULL;
	    param->errmsg[0] = '\0';
	    if ((fp = fopen(graylistoptionPath, "w+")) == (FILE *)NULL) {
                sprintf (param->errmsg, 
		    "Failed to create file [%s] in workspace", 
		    graylistoptionPath);
	        return (-1);
	    }

	    
	    fprintf (fp, "<option vallue=\"\" selected=\"selected\">\n");
	    fprintf (fp, "</option>\n");

            for (l=0; l<param->nim_gray; l++) {
	
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "l= [%d] imfile= [%s]\n", 
		        l, param->imfile[l]);
                    fflush (fp_debug);
                }

	        fprintf (fp, "<option vallue=\"%s\">%s</option>\n", 
		    param->imfile[l], param->imfile[l]);
	    }

	    fclose (fp);
        }
        else {
            redlistoptionPath[0] = '\0';
            grnlistoptionPath[0] = '\0';
            bluelistoptionPath[0] = '\0';
        }


/*
    If nsrctbl > 1, make srctbloption interface for adding srctbl
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
            fflush (fp_debug);
        }

        if (param->nsrctbl > 0) {

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "\nwrite srctblOption file for html page\n");
                fflush (fp_debug);
            }

            sprintf (srctbloptionPath, "%s/srctbloption.txt", param->directory); 
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "srctbloptionPath= [%s]\n", 
		    srctbloptionPath);
                fflush (fp_debug);
            }

            fp = (FILE *)NULL;
	    param->errmsg[0] = '\0';
	    if ((fp = fopen(srctbloptionPath, "w+")) == (FILE *)NULL) {
                sprintf (param->errmsg, 
		    "Failed to create file [%s] in workspace",
		    srctbloptionPath);
	        return (-1);
	    }
    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "here2\n");
                fflush (fp_debug);
            }


	    fprintf (fp, "<option vallue=\"\" selected=\"selected\">\n");
	    fprintf (fp, "</option>\n");
        
	    for (l=0; l<param->nsrctbl; l++) {
	
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "l= [%d] srctblfile= [%s]\n", 
		        l, param->srctblfile[l]);
                    fflush (fp_debug);
                }

	        fprintf (fp, "<option vallue=\"%s\">%s</option>\n", 
		    param->srctblfile[l], param->srctblfile[l]);
	    }

	    fclose (fp);
        }


/*
    If niminfo >= 1, make iminfooption interface for adding iminfo
*/
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
            fflush (fp_debug);
        }

        if (param->niminfo > 0) {

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "here3\n");
                fflush (fp_debug);
            }

            sprintf (iminfooptionPath, "%s/iminfooption.txt", param->directory); 
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "iminfooptionPath= [%s]\n", 
		    iminfooptionPath);
                fflush (fp_debug);
            }

            fp = (FILE *)NULL;
	    param->errmsg[0] = '\0';
	    if ((fp = fopen(iminfooptionPath, "w+")) == (FILE *)NULL) {
                sprintf (param->errmsg, 
		    "Failed to create file [%s] in workspace",
	            iminfooptionPath);
	        return (-1);
	    }

	    fprintf (fp, "<option vallue=\"\" selected=\"selected\">\n");
	    fprintf (fp, "</option>\n");
        
	    for (l=0; l<param->niminfo; l++) {
	
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "l= [%d] iminfofile= [%s]\n", 
		        l, param->iminfofile[l]);
                    fflush (fp_debug);
                }

	        fprintf (fp, "<option vallue=\"%s\">%s</option>\n", 
		    param->iminfofile[l], param->iminfofile[l]);
	    }

	    fclose (fp);
        }

/*
    Make viewer html from template -- always requires a template
*/
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "here4\n");
            fflush (fp_debug);
        }

        if (param->isimcube) {
	    strcpy (imfile, param->imcubepath);
	}
	else {
	    strcpy (imfile, param->grayfile);
	}
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "isimcube= [%d]\n", param->isimcube);
	    fprintf (fp_debug, "imfile= [%s]\n", imfile);
	    
	    fprintf (fp_debug, "cdelt3= [%lf]\n", param->cdelt3);
	    fprintf (fp_debug, "crval3= [%lf]\n", param->crval3);
            fflush (fp_debug);
        }
       
	varcmd (varstr, 32768,
            "htmlgen",
	                        "%s",              viewtemplatepath,
	                        "%s",              param->viewhtmlpath,
	    "winname",          "%s",              param->winname,
	    "workspace",        "%s",              param->workspace,
	    "httpurl",	        "%s",              param->baseurl,
	    "isimcube",		"%d",	           param->isimcube,
	    "nplane",           "%d",              param->nfitsplane,
	    "cdelt3",           "%lf",             param->cdelt3,
	    "crval3",           "%lf",             param->crval3,
	    "imfile",           "%s",              imfile,
	    "grayfile",         "%s",              param->grayfile,
	    "imcubepath",       "%s",              param->imcubepath,
	    "redfile",          "%s",              param->redfile,
	    "grnfile",          "%s",              param->grnfile,
	    "bluefile",         "%s",              param->bluefile,
	    "jsonfile",         "%s",              param->jsonfile,
	    "planeoption",      "%s",              planeoptionpath,
	    "title",	        "%s",              param->divname,
	    "imname",	        "%s",              param->imname,
	    "divname",	        "%s",              param->divname,
	    "viewdiv",	        "%sview",          param->divname,
	    "refdiv",	        "%sref",           param->divname,
	    "canvaswidth",      "%d",              param->canvaswidth,
	    "canvasheight",     "%d",              param->canvasheight,
	    "refwidth",         "%d",              param->refwidth,
	    "refheight",        "%d",              param->refheight,
	    "viewcgiurl",       "%s",              param->viewcgiurl,
	    "tblcgiurl",        "%s",              param->tblcgiurl,
	    "tblwidth",         "%d",              param->tblwidth,
	    "tblheight",        "%d",              param->tblheight,
	    "graylistoption",   "%s",              graylistoptionPath,
	    "srctbloption",     "%s",              srctbloptionPath,
	    "iminfooption",     "%s",              iminfooptionPath,
	    "objname",     	"%s",              param->objname,
	    "filter",     	"%s",              param->filter,
	    "pixscale",     	"%s",              param->pixscale,
	    "END_PARM");


        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "varstr (viewhtml)= [%s]\n", varstr);
            fflush (fp_debug);
        }
   
/*
    Add imlist, srctbl and iminfo list elements
*/
        if (param->nim_gray > 0) {
            
	    for (l=0; l<param->nim_gray; l++) {
	        sprintf (str, " \"imfile%d\" \"%s\"", l, param->imfile[l]);
		strcat (varstr, str); 
            }
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "varstr (viewhtml)= [%s]\n", varstr);
	        fflush (fp_debug);
	    }
	}
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here5\n");
            fflush (fp_debug);
        }
   

        if (param->nim_color > 0) {
            
	    for (l=0; l<param->nim_color; l++) {
	        sprintf (str, " \"rimfile%d\" \"%s\"", l, param->rimfile[l]);
		strcat (varstr, str); 
	        sprintf (str, " \"gimfile%d\" \"%s\"", l, param->gimfile[l]);
		strcat (varstr, str); 
	        sprintf (str, " \"bimfile%d\" \"%s\"", l, param->bimfile[l]);
		strcat (varstr, str); 
            }
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "varstr (viewhtml)= [%s]\n", varstr);
                fflush (fp_debug);
            }
	}
	
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here6\n");
            fflush (fp_debug);
        }
   

        if (param->nsrctbl > 0) {
            
	    for (l=0; l<param->nsrctbl; l++) {
	        sprintf (str, " \"srctblfile%d\" \"%s\"", 
		    l, param->srctblfile[l]);
		strcat (varstr, str); 
            }
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "varstr (viewhtml)= [%s]\n", varstr);
	        fflush (fp_debug);
	    }
	}
	
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here7\n");
            fflush (fp_debug);
        }
   

        if (param->niminfo > 0) {
           
	    for (l=0; l<param->niminfo; l++) {
	        sprintf (str, " \"iminfofile%d\" \"%s\"", 
		    l, param->iminfofile[l]);
		strcat (varstr, str); 
            }
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "varstr (viewhtml)= [%s]\n", varstr);
	        fflush (fp_debug);
	    }
	}
	    
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "Run varstr (viewhtml)= [%s]\n", varstr);
	    fflush (fp_debug);
	}

        istatus = svc_run (varstr);
      
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returned svc_run: istatus= [%d]\n", istatus);
            fflush (fp_debug);
        }
    
        strcpy( status, svc_value( "stat" ));

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "status= [%s]\n", status);
            fflush (fp_debug);
        }
    
        if (strcasecmp( status, "error") == 0) {
	    strcpy (param->errmsg, svc_value("msg"));
	    return (-1);
        }
    }



/*
    If input paramfile provides neither tbldisphtml nor tbldisptemplate,
    return error.
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "nsrctbl= [%d] niminfo= [%d]\n", 
	    param->nsrctbl, param->niminfo);
        fflush (fp_debug);
    }

    return (0);
}
