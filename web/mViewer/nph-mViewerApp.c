/*
Theme:  This program sets up the workspace and makes mViewer's  
    initial HTML page from a template.

Input:

-- workspace (optional),

-- image file,

-- input param file: a keyword-value pair parameter file defining the
   initial paramters of the image display, including:
   
   ** the datadir,
   
   ** image div name,

   ** a required HTML template containing the image viewer's <div> and 
      the javascript code for controls and callbacks of viewer's operations
      (e.g. zoom, intensity stretch etc..).

   ** optional initial display parameters (default will be provided 
      if not given).

-- project name (to set the cookie name)


Date: August 18, 2015 (Mihseh Kong)
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <password.h>


#include <time.h>
#include <math.h>

#include <config.h>
#include <www.h>
#include <cmd.h>

#include "viewerapp.h"
#include "fitshdr.h"


#define MAXSTR 1024
#define MAXTBL   64


char *strtrim (char *);
int str2Integer (char *strval, int *intval, char *errmsg);
int str2Double (char *strval, double *dblval, char *errmsg);

void printerr (char *msg);

int checkFileExist (char *fname, char *rootname, char *suffix,
    char *directory, char *filePath);

int extractStartupParam (struct ViewerApp *param);

int getFitshdr (char *fromFile, struct FitsHdr *hdr);

int writeJsonFile (struct ViewerApp *param);
int writePlotJson (struct ViewerApp *param);

int makeStartupHtml (struct ViewerApp *param, struct FitsHdr *hdr); 
int readStartupParam (struct ViewerApp *param, char *parampath); 


FILE *fp_debug = (FILE *)NULL;

int main (int argc, char *argv[], char *envp[])
{
    FILE      *fp;

    struct ViewerApp param;

    struct FitsHdr hdr;
    
    char   *cptr;
    char   cmd[1024];
    char   str[1024];
    
    char   imroot[1024];

    char   debugfname[1024];
 
    int    pid;

    int    nkey;
    int    istatus;

    int    debugfile = 0;
   
/* 
    Get ISIS.conf parameters 
*/
    config_init ((char *)NULL);

    nkey = keyword_init (argc, argv);
    pid = getpid();


    if (debugfile) {
        
        sprintf (debugfname, "/tmp/viewerapp_%d.debug", pid);

	fp_debug = fopen (debugfname, "w+");
	if (fp_debug == (FILE *)NULL) {
	    printf ("Cannot create debug file: %s\n", debugfname);
	    fflush (stdout);
	    exit (0);
	}
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nnph-viewerApp starts: nkey= [%d]\n", nkey);
        fprintf (fp_debug, "call extractStartupParam\n");
        fflush (fp_debug);
    }

/*
    strcpy (param.cookiename, "NIRC2");
*/

    istatus = extractStartupParam (&param);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "returned extractStartupParam: istatus= [%d]\n",
	    istatus);
	fprintf (fp_debug, "imname= [%s]\n", param.imname);

        fprintf (fp_debug, "isimcube= [%d]\n", param.isimcube);
	fprintf (fp_debug, "gridvis= [%d]\n", param.gridvis[0]);

        fprintf (fp_debug, "nim_gray= [%d] nim_color= [%d]\n", 
	    param.nim_gray, param.nim_color);
        fprintf (fp_debug, "nsrctbl= [%d] niminfo= [%d]\n", 
	    param.nsrctbl, param.niminfo);
        fprintf (fp_debug, "viewhtml= [%s] viewtemplate= [%s]\n", 
	    param.viewhtml, param.viewtemplate);
    }

    if (istatus == -1) {
	printerr (param.errmsg);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
      
        fprintf (fp_debug, "\nhttp_srvr = [%s]\n", param.http_srvr);
        fprintf (fp_debug, "baseurl   = [%s]\n", param.baseurl);
        fprintf (fp_debug, "workdir   = [%s]\n", param.workdir);
        fprintf (fp_debug, "workspace = [%s]\n", param.workspace);
        fprintf (fp_debug, "directory = [%s]\n", param.directory);
        fprintf (fp_debug, "datadir = [%s]\n", param.datadir);
        
	fprintf (fp_debug, "cmd       = [%s]\n", param.cmd);
	fprintf (fp_debug, "mode  = [%s]\n", param.mode);
	fprintf (fp_debug, "divname  = [%s]\n", param.divname);
	
	fprintf (fp_debug, "imroot = [%s]\n", param.imroot);
	fprintf (fp_debug, "grayfile    = [%s]\n", param.grayfile);
	
        fprintf (fp_debug, "canvasWidth = [%d]\n", param.canvaswidth);
        fprintf (fp_debug, "canvasHeigh = [%d]\n", param.canvasheight);
        fflush (fp_debug);
	
	
        fprintf (fp_debug, "ngridMax = [%d]\n", param.ngridMax);
        fprintf (fp_debug, "niminfoMax = [%d]\n", param.niminfoMax);
        fprintf (fp_debug, "nim_gray = [%d]\n", param.nim_gray);
        fprintf (fp_debug, "nim_color = [%d]\n", param.nim_color);
        fprintf (fp_debug, "nsrctbl = [%d]\n", param.nsrctbl);
        fprintf (fp_debug, "niminfo = [%d]\n", param.niminfo);
	fprintf (fp_debug, "ngrid = [%d]\n", param.ngrid);
        fprintf (fp_debug, "iscolor = [%d]\n", param.iscolor);
        fprintf (fp_debug, "isimcube = [%d]\n", param.isimcube);
        fprintf (fp_debug, "nowcs = [%d]\n", param.nowcs);
        fflush (fp_debug);
    }
    

/*
    Extract imname from filename
*/
    if ((int)strlen(param.imname) == 0) {
    
      if (!param.iscolor) {
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "xxx1\n");
            fflush (fp_debug);
        }
        

        str[0] = '\0';
        if (param.isimcube) {

            if ((int)strlen (param.imcubefile) > 0) {
		strcpy (str, param.imcubefile);
	    }
	}
	else {
	    if ((int)strlen (param.grayfile) > 0) {
		strcpy (str, param.grayfile);
	    }
	}
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "str= [%s]\n", str);
            fflush (fp_debug);
        }
          
	imroot[0] = '\0';
	if ((int)strlen (str) > 0) {
	        
	    cptr = (char *)NULL;
            cptr = strrchr (str, '/');
		
	    if (cptr != (char *)NULL) {
		strcpy (imroot, cptr+1);
	    }
	    else {
		strcpy (imroot, str);
	    }
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imroot= [%s]\n", imroot);
                fflush (fp_debug);
            }

	    cptr = (char *)NULL;
            cptr = strrchr (imroot, '.');
	    if (cptr != (char *)NULL) {
		*cptr = '\0';
            }
        }
           
	strcpy (param.imname, imroot);
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "cmd= startup: imname= [%s]\n", param.imname );
	    fflush (fp_debug);
        }
	
      }
      else {
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "xxx2\n");
            fflush (fp_debug);
        }
        
        str[0] = '\0';
        if (param.isimcube) {

            if ((int)strlen (param.redcubefile) > 0) {
		strcpy (str, param.redcubefile);
	    }
	}
	else {
	    if ((int)strlen (param.redfile) > 0) {
		strcpy (str, param.redfile);
	    }
	}
          
	imroot[0] = '\0';
	if ((int)strlen (str) > 0) {
	        
	    cptr = (char *)NULL;
            cptr = strrchr (str, '/');
		
	    if (cptr != (char *)NULL) {
		strcpy (imroot, cptr+1);
	    }
	    else {
		strcpy (imroot, str);
	    }

	    cptr = (char *)NULL;
            cptr = strrchr (str, '.');
	    if (cptr != (char *)NULL) {
		*cptr = '\0';
            }
        }
           
	strcpy (param.imname, imroot);
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "imname= [%s]\n", param.imname );
	    fflush (fp_debug);
        }
      }
    } 
	
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "jsonfile= [%s]\n", param.jsonfile);
	fflush (fp_debug);
    }


/*
    Read FitsHdr to extract special keywords: objname, filter, and pixscale
    for display.
    
    If it is a fits cube, find out nfitsplane too.
    Note: Only deal with grayfile at present.
*/
    param.nfitsplane = 0;

    if (param.isimcube) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "here1: isimcube\n");
            fflush (fp_debug);
        }

	istatus = getFitshdr (param.imcubepath, &hdr); 
	    
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned getFitshdr: istatus= [%d]\n", istatus);
            fflush (fp_debug);
        }
	   

        if (istatus != 0) {
	    sprintf (param.errmsg, 
	        "Failed to extract Fits Header from input imcube file [%s]\n",
	        param.imcubepath);
            printerr (param.errmsg);
	}
            
	param.nfitsplane = hdr.nplane;
            
	strcpy (param.filter, hdr.filter);
	strcpy (param.objname, hdr.objname);
	strcpy (param.pixscale, hdr.pixscale);

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        
	    fprintf (fp_debug, "isimcube= [%d]\n", param.isimcube);
	    fprintf (fp_debug, "nfitsplane= [%d]\n", param.nfitsplane);
	        
	    fprintf (fp_debug, "objname= [%s]\n", param.objname);
	    fprintf (fp_debug, "pixscale= [%s]\n", param.pixscale);
	    fprintf (fp_debug, "filter= [%s]\n", param.filter);
		
	    fprintf (fp_debug, "axisIndx[2]= [%d]\n", hdr.axisIndx[2]);
	    fprintf (fp_debug, "cdelt[2]= [%lf]\n", 
	        hdr.cdelt[hdr.axisIndx[2]]);
	    fprintf (fp_debug, "crval[2]= [%lf]\n", 
	        hdr.crval[hdr.axisIndx[2]]);
            fflush (fp_debug);
        }

        param.cdelt3 = hdr.cdelt[hdr.axisIndx[2]]; 
        param.crval3 = hdr.crval[hdr.axisIndx[2]]; 
	
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "cdelt3= [%lf]\n", param.cdelt3);
	    fprintf (fp_debug, "crval3= [%lf]\n", param.crval3);
            fflush (fp_debug);
        }
       
    }
    else {
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "here2: imfile: %s\n", param.graypath);
            fflush (fp_debug);
        }

	istatus = getFitshdr (param.graypath, &hdr); 
            
	strcpy (param.filter, hdr.filter);
	strcpy (param.objname, hdr.objname);
	strcpy (param.pixscale, hdr.pixscale);

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned getFitshdr: istatus= [%d]\n", istatus);
		
	    fprintf (fp_debug, "objname= [%s]\n", param.objname);
	    fprintf (fp_debug, "pixscale= [%s]\n", param.pixscale);
	    fprintf (fp_debug, "filter= [%s]\n", param.filter);
            fflush (fp_debug);
        }
	   

        if (istatus != 0) {
	        sprintf (param.errmsg, 
	          "Failed to extract Fits Header from input image file [%s]\n",
		  param.graypath);
                
		printerr (param.errmsg);
	}
    }


/*
    If input jsonfile doesn't exist, create jsonfile
*/
    if ((int)strlen(param.jsonfile) == 0) {
    
        sprintf (param.jsonfile, "%s.json", param.imname);
        sprintf (param.jsonpath, "%s/%s", param.directory, param.jsonfile);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "call writeJsonFile\n");
            fprintf (fp_debug, "jansonfile= [%s]\n", param.jsonfile);
            fprintf (fp_debug, "jansonpath= [%s]\n", param.jsonpath);
            fflush (fp_debug);
        }

        istatus = writeJsonFile (&param); 

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returning writeJsonFile: istatus= [%d]\n",
                istatus);
            fflush (fp_debug);
	}

        if (istatus < 0) {
	    printerr (param.errmsg);
        }
    }

/*
    If plot required but input plotjsonfile doesn't exist, 
        create plotjsonfile
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "waveplottype= [%s]\n", param.waveplottype);
        fprintf (fp_debug, "plotjsonfile= [%s]\n", param.plotjsonfile);
	fflush (fp_debug);
    }

    if (((int)strlen(param.waveplottype) > 0) &&
        ((int)strlen(param.plotjsonfile) == 0)) {
   
        sprintf (param.plotjsonfile, "%s_plot.json", param.imname);
        sprintf (param.plotjsonpath, "%s/%s", 
	    param.directory, param.plotjsonfile);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "call writeJsonFile\n");
            fprintf (fp_debug, "plotjansonfile= [%s]\n", param.plotjsonfile);
            fprintf (fp_debug, "plotjansonpath= [%s]\n", param.plotjsonpath);
            fflush (fp_debug);
        }

        istatus = writePlotJson (&param); 

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "returning writePlotJson: istatus= [%d]\n",
                istatus);
            fflush (fp_debug);
	}

        if (istatus < 0) {
	    printerr (param.errmsg);
        }
    }

       
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "call makeStartupHtml\n");
        fflush (fp_debug);
    }


    istatus = makeStartupHtml (&param, &hdr); 

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "returning makeStartupHtml: istatus= [%d]\n",
            istatus);
        fprintf (fp_debug, "viewhtmlpath= [%s]\n", param.viewhtmlpath);
        fflush (fp_debug);
    }

    if (istatus < 0) {
	printerr (param.errmsg);
    }
       

    fp = (FILE *)NULL;
    fp = fopen (param.viewhtmlpath, "r");
    if (fp == (FILE *)NULL) {
        printf("Can't open HTML file: [%s].<br>\n", param.viewhtmlpath);
        printf("</html>\n");
        fflush(stdout);
        return(-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here3\n");
        fflush (fp_debug);
    }


    expires (7.0, param.timeout);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "timeout= [%s]\n", param.timeout);
        fprintf (fp_debug, "cookiename= [%s]\n", param.cookiename);
        fprintf (fp_debug, "cookiestr= [%s]\n", param.cookiestr);
        fflush (fp_debug);
    }

        
    printf("HTTP/1.0 200 OK\r\n");
    printf ("Set-Cookie: %s=%s;path=/;expires=%s\r\n", 
        param.cookiename, param.cookiestr, param.timeout);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "Set-Cookie called\n");
        fflush (fp_debug);
    }
    
    printf ("Content-type: text/html\r\n\r\n");
    fflush (stdout);

    while(1) {
        if(fgets(cmd, 1024, fp) == (char *)NULL)
            break;

        if(cmd[strlen(cmd) - 1] == '\n')
            cmd[strlen(cmd) - 1]  = '\0';

        puts(cmd);
    }
    fclose (fp);
    fflush (stdout);


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "done\n");
        fflush (fp_debug);
    }

    
    return (0);
}
