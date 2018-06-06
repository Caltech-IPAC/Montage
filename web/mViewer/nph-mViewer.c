/*
Theme:  Main program for mViewer image interaction

command:   
    
    init      -- make initial image display.
       
    zoomin    -- zoom each image dimension by sqrt(2.)
    zoomin    -- shrink each image dimension by sqrt(2.)
    
    zoombox   -- make a zoomed image of the input zoom box area
    movebox   -- move the zoomed image to the input zoom box area
    
    resetzoom -- reset to the initial image display

    update    -- make new image display based on the updated image parameters 
      
    pick      -- convert a screen pixel to sky coordinate, 
                 find pixel value, and the closest table source.
    
    tblrow    -- Find the image pixel that correspond to the selected table row

Written: June 15,2015 (Mihseh Kong)

Comment: Consolidate the old iceViewer program with JCG's mViewer program.

John's program works but it is too long so I separated into routines but 
kept the variable names the same when possible so he doesn't have to 
change his code.


-- extractViewParam,
-- init,
-- pick,
-- zoom,
-- update,
-- constructRetjson


Modified: May25, 2017 (Mihseh Kong)

Rearrange the program to remove project specific functionality such as 
extracting KOA's FITS keywords (instrument, objname, filter etc.) and 
re-arranging OSIRIS cube data, and making OSIRIS waveplot.

Project related code will be included in the project software. 
*/

/*************************************************************************/
/*                                                                       */
/*  The service receives an image display specification (JSON structure) */
/*  and uses it to generate a JPEG/PNG image of the data.  Really, all   */
/*  it does is run the mViewer program and manage the result file.       */
/*                                                                       */
/*************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <time.h>
#include <math.h>

#include <config.h>
#include <www.h>
#include <cmd.h>
#include <json.h>

#include "mviewer.h"
#include "fitshdr.h"


#define MAXSTR 1024
#define MAXTBL   64


char *strtrim (char *);
int str2Integer (char *strval, int *intval, char *errmsg);
int str2Double (char *strval, double *dblval, char *errmsg);

int printRetval (char *msg);

int checkFileExist (char *fname, char *rootname, char *suffix,
    char *directory, char *filePath);

int extractViewParam (struct Mviewer *param);

int getFitshdr (char *fname, struct FitsHdr *hdr, int iscube);
int imZoom (struct Mviewer *param);

int subsetImage (char *imPath, int ns_orig, int nl_orig, int xflip, int yflip,
    int nowcs, char *subsetimpath, double ss, double sl, double ns, double nl, 
    char *errmsg);

int makeImage (struct Mviewer *param);
int pick (struct Mviewer *param);

int constructRetjson (struct Mviewer *param);

int extractAvePlane (char *imcubepath, char *graypath, int implanenum,
    int nplaneave, int *splaneave, int *eplaneave, char *errmsg);
int generateMedianPlane (char *imcubepath, char *graypath, int implanenum,
    int nplaneave, int *splaneave, int *eplaneave, char *errmsg);

int extractWaveSpectra (struct Mviewer *param);
   	

/*
   print error message in the form of JSON structure
   for an AJAX call
*/
void printError(char *errmsg)
{
   printf("HTTP/1.1 200 OK\r\n");
   printf("Content-type: text/plain\r\n\r\n");
   fflush(stdout);

   printf ("{\"error\" : \"%s\"}", errmsg);
   fflush (stdout);

   exit(0);
}


FILE *fdebug = (FILE *)NULL;
FILE *fp_debug = (FILE *)NULL;



int main (int argc, char *argv[], char *envp[])
{

    struct timeval   tp;
    struct timezone  tzp;
    double exacttime, exacttime0;
    
    int     pid;
  
/*
    FILE      *fp;
    time_t    curtime;
    struct tm *loctime;
    
    int    yr, mo, day, pid;
    double start;
    
    char   *cptr;
    char   str[1024];
    char   buffer[256];
    char   id[MAXSTR];
*/


    struct Mviewer param;
    struct FitsHdr hdr;

    char   *cptr;
    char   str[1024];
    char   suffix[40];
    char   cmd[1024];
    
    char   imroot[1024];
    char   redroot[1024];
    char   grnroot[1024];
    char   blueroot[1024];

    char   impath[1024];

/*
    char   redpath[1024];
    char   grnpath[1024];
    char   bluepath[1024];
*/

    char   refJpgpath[1024];

    char   errmsg[256];
    char   debugfname[1024];
   
    int    nkey;
    int    istatus;
    
    int    fileExist;
    int    redExist;
    int    blueExist;
    int    grnExist;

    int    debugfile = 0;
    int    debugtime = 0;
   
/* 
    Get ISIS.conf parameters 
*/
    config_init ((char *)NULL);

    nkey = keyword_init (argc, argv);
    pid = getpid();
    param.nkey = nkey;

    if (debugfile) {
        sprintf (debugfname, "/tmp/mviewer_%d.debug", pid);
        
        fdebug = fopen (debugfname, "w+");
        if (fdebug == (FILE *)NULL) {
	    printf ("Cannot create debug file: %s\n", debugfname);
	    fflush (stdout);
	    exit (0);
        }
    }

    fp_debug = fdebug;

    cmd[0] = '\0';
    if(keyword_exists("cmd")) {
        if (keyword_value("cmd") != (char *)NULL) {
            strcpy(cmd, strtrim(keyword_value("cmd")));
        }
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "\nnph-mViewer starts: nkey= [%d]\n", param.nkey);
        fprintf (fdebug, "cmd= [%s]\n", cmd);
        fflush (fdebug);
    }
    
    strcpy (param.cmd , cmd);
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "call extractViewParam\n");
        fflush (fdebug);
    }


    if (debugtime) {
        gettimeofday (&tp, &tzp);
        exacttime0 = (double)tp.tv_sec + (double)tp.tv_usec/1000000.;
    }

    istatus = extractViewParam (&param);

    if (debugtime) {
        gettimeofday (&tp, &tzp);
        exacttime = (double)tp.tv_sec + (double)tp.tv_usec/1000000.;
        fprintf (fdebug, "time (extractViewParam): %.6f sec\n", 
	    (exacttime-exacttime0));
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "returned extractViewParam: istatus= [%d]\n",
	    istatus);
        fflush (fdebug);
    }

    if (istatus == -1) {
        printError (param.errmsg);
    }


    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "here1\n");
        fprintf (fdebug, "isimcube= [%d]\n", param.isimcube);
        fflush (fdebug);
    }


/*
    Check if input image file exists
*/
    if (!param.iscolor) {
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "iscolor=0\n");
            fflush (fdebug);
        }
        
        if (param.isimcube) {
/*
    Case: data cube
*/
	    fileExist = 0;

            if ((int)strlen(param.cubedatadir) > 0) {


                if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, "check cubefile in cubedatadir= %s\n",
		        param.cubedatadir);
		    fflush (fp_debug);
                }
   
	        fileExist = checkFileExist (param.imcubefile, imroot, 
	            suffix, param.cubedatadir, param.imcubepath);
            
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, 
	            "returned checkFileExist(cubedatadir): fileExist= [%d]\n",
	                fileExist);
		    fflush (fp_debug);
                }
                
   
            }
	    else {
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, "check imcubefile in workdir= %s\n",
		        param.directory);
		    fflush (fp_debug);
                }
   
	        fileExist = checkFileExist (param.imcubefile, imroot, 
	            suffix, param.directory, param.imcubepath);
            
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, 
		        "returned checkFileExist (ws): fileExist= [%d]\n", 
	                fileExist);
		    fflush (fp_debug);
                }
		
            }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "fileExist= [%d]\n", fileExist);
            }
   
            if (!fileExist) {
	        sprintf (param.errmsg, "Cannot find required FITS image file "
		    "[%s] in workspace or data directory.", param.imcubefile);
	        printError (param.errmsg);
	    }

/*    
    if implane name is not defined, supply the default name
*/
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", param.imcubepath);
                fprintf (fp_debug, "imcubefile= [%s]\n", param.imcubefile);
                fprintf (fp_debug, "grayFile= [%s]\n", param.grayFile);
                
		fprintf (fp_debug, "planeavemode= [%s]\n", param.planeavemode);
		fprintf (fp_debug, "nplaneave adjusted: nplaneave= [%d]\n",
		    param.nplaneave);
                fprintf (fp_debug, "centerplane= [%d]\n", param.centerplane);
                fflush (fp_debug);
            }

            if ((int)strlen(param.grayFile) == 0) {
		strcpy (param.grayFile, "implane.fits");
            }

            sprintf (param.grayPath, "%s/%s", param.directory, param.grayFile);
	    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "grayPath= [%s]\n", param.grayPath);
            }

	    if ((strcasecmp (param.cmd, "initjsonfile") == 0) ||
	        (strcasecmp (param.cmd, "initjsondata") == 0) ||
	        (strcasecmp (param.cmd, "replaceimplane") == 0))  
	    {
/*
    New implane: extract implane from cube
*/
                if (strcasecmp (param.planeavemode, "ave") == 0) {

		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "call extractAvePlane\n");
		        fflush (fp_debug);
                    }
		    
		    istatus = extractAvePlane (param.imcubepath, 
		        param.grayPath, param.centerplane, param.nplaneave, 
		        &param.startplane, &param.endplane, param.errmsg);


                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, 
		            "returned extractAvePlane: istatus= [%d]\n", 
			    istatus);
		        fflush (fp_debug);
                    }

                    if (istatus == -1) {
                        printError (param.errmsg);
                    }

                }
	        else {
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "call generateMedianPlane\n");
                        fprintf (fp_debug, 
			    "centerplane= [%d] nplaneave= [%d]\n",
		            param.centerplane, param.nplaneave);
		        fflush (fp_debug);
                    }

	            istatus = generateMedianPlane (param.imcubepath, 
		        param.grayPath, param.centerplane, param.nplaneave,
		        &param.startplane, &param.endplane, param.errmsg);
            
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, 
		            "returned generateMedianPlane: istatus= [%d]\n",
		            istatus);
		        fflush (fp_debug);
                    }

                    if (istatus == -1) {
                        printError (param.errmsg);
                    }

                }
            }
	    else {
/*
    Same implane: grayFile should have already existed in workdir 
*/
	        fileExist = checkFileExist (param.grayFile, imroot, 
	            suffix, param.directory, param.grayPath);

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, 
		        "returned checkFileExist (ws): fileExist= [%d]\n", 
	                fileExist);
		    fflush (fp_debug);
                }
   
                if (!fileExist) {
	            sprintf (param.errmsg, 
	                "Cannot find image plane file in work directory [%s].",
	                param.directory);
	            printError (param.errmsg);
	        }
            }
	
	}
	else {
/*
    Case: 2D imageD
*/
	    fileExist = 0;

            if ((int)strlen(param.imdatadir) > 0) {

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, "check file in imdatadir= %s\n",
		        param.imdatadir);
		    fflush (fp_debug);
                }
   
	        fileExist = checkFileExist (param.grayFile, imroot, 
	            suffix, param.imdatadir, param.grayPath);
            
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, 
		    "returned checkFileExist(imdatadir): fileExist= [%d]\n",
	                fileExist);
		    fflush (fp_debug);
                }
   
            }
	    else {
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, "check file in workdir= %s\n",
		        param.directory);
		    fflush (fp_debug);
                }
   
	        fileExist = checkFileExist (param.grayFile, imroot, 
	            suffix, param.directory, param.grayPath);
            
	        if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, 
		        "returned checkFileExist (ws): fileExist= [%d]\n", 
	                fileExist);
		    fflush (fp_debug);
                }
   
            }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "fileExist= [%d]\n", fileExist);
		fflush (fp_debug);
            }
   
            if (!fileExist) {
	        sprintf (param.errmsg, "Cannot find required FITS image file "
		    "in either workdir or datadir.");
	        printError (param.errmsg);
	    }
            
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "grayFile= [%s]\n", param.grayFile);
                fprintf (fdebug, "grayPath= [%s]\n", param.grayPath);
		fflush (fp_debug);
            }
   
        } 
            
	strcpy (impath, param.grayPath);
	
    }
    else {
/*
    Case: color image
*/
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "iscolor=1\n");
            fflush (fdebug);
        }
	    
	redExist = 0;
        if ((int)strlen(param.imdatadir) > 0) {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "check file in imdatadir= %s\n",
		    param.imdatadir);
		fflush (fp_debug);
            }
   
	    redExist = checkFileExist (param.redFile, imroot, 
	        suffix, param.imdatadir, param.redPath);
            
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist(imdatadir): redExist= [%d]\n",
	            redExist);
		fflush (fp_debug);
            }
   
        }
	else {
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "check file in workdir= %s\n",
		    param.directory);
		fflush (fp_debug);
            }
   
            redExist = checkFileExist (param.redFile, redroot, 
	        suffix, param.directory, param.redPath);
            
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist(ws): redExist= [%d]\n", redExist);
                fflush (fdebug);
            }
	}
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "redExist= [%d]\n", redExist);
            fflush (fdebug);
	}
        
	
	grnExist = 0;
        if ((int)strlen(param.imdatadir) > 0) {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "check file in imdatadir= %s\n",
		    param.imdatadir);
		fflush (fp_debug);
            }
   
	    grnExist = checkFileExist (param.greenFile, imroot, 
	        suffix, param.imdatadir, param.greenPath);
            
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist(imdatadir): grnExist= [%d]\n",
	            grnExist);
		fflush (fp_debug);
            }
   
        }
	else {
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "check file in workdir= %s\n",
		    param.directory);
		fflush (fp_debug);
            }
	
	    grnExist = checkFileExist (param.greenFile, grnroot, 
                suffix, param.directory, param.greenPath);

	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist(ws): grnExist= [%d]\n", grnExist);
                fflush (fdebug);
            }
	}
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "grnExist= [%d]\n", grnExist);
            fflush (fdebug);
	}
        
        
	blueExist = 0;
        if ((int)strlen(param.imdatadir) > 0) {

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "check file in imdatadir= %s\n",
		    param.imdatadir);
		fflush (fp_debug);
            }
   
	    blueExist = checkFileExist (param.blueFile, imroot, 
	        suffix, param.imdatadir, param.bluePath);
            
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist(imdatadir): blueExist= [%d]\n",
	            blueExist);
		fflush (fp_debug);
            }
   
        }
	else {
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "check file in workdir= %s\n",
		    param.directory);
		fflush (fp_debug);
            }
   
        blueExist = checkFileExist (param.blueFile, blueroot, 
	    suffix, param.directory, param.bluePath);

	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist(ws): blueExist= [%d]\n", 
	            blueExist);
                fflush (fdebug);
            }
	}
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "blueExist= [%d]\n", blueExist);
            fflush (fdebug);
	}
        
    
        if ((!redExist) || 
            (!blueExist)) { 
	
	    strcpy (errmsg, "Either red or blue FITS image files for");
	    sprintf (param.errmsg, 
	        "%s color image planes is missing in workspace [%s].",
	        errmsg, param.directory);
	    printError (param.errmsg);
        }    
	
	strcpy (impath, param.redPath);
	
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "here2: impath= [%s]\n", impath);
	fflush (fp_debug);
    }


    if ((int)strlen(param.imageFile) == 0) {
        
	if (!param.iscolor) {
            strcpy (str, param.grayFile);
	}
	else {
            strcpy (str, param.redFile);
	}
	    
	cptr = (char *)NULL;
	cptr = strrchr (str, '.');
	if (cptr != (char *)NULL) {
            *cptr = '\0';
	}

	strcpy (param.imageFile, str);

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "param.imageFile= [%s]\n", param.imageFile);
            fflush (fdebug);
        }
    }

    
/*
    Use imname to construct subset, shrunk, jpg, and imparam files.
*/
/*
    curtime = time (NULL);
    loctime = localtime (&curtime);

    strftime(buffer, 256, "%Y", loctime);
    yr = atoi(buffer);
    strftime(buffer, 256, "%m", loctime);
    mo = atoi(buffer);
    strftime(buffer, 256, "%d", loctime);
    day = atoi(buffer);

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "pid= [%d]\n", pid);
	fprintf (fdebug, "yr= [%d] mo= [%d] day= [%d]\n", yr, mo, day);
	fprintf (fdebug, "loctime->tm_isdst= [%d]\n", loctime->tm_isdst);
        fflush (fdebug);
    }

    sprintf (id, "%04d.%02d.%02d.%d", yr, mo, day, pid);
 
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "id= [%s]\n", id);
	fprintf (fdebug, "imroot= [%s]\n", param.imroot);
        fflush (fdebug);
    }
*/

    sprintf (param.jpgfile, "%s.jpg", param.imageFile);
    sprintf (param.refjpgfile, "%s_ref.jpg", param.imageFile);
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "param.cmd= [%s]\n", param.cmd);
	fprintf (fdebug, "jpgfile= [%s]\n", param.jpgfile);
	fprintf (fdebug, "refjpgfile= [%s]\n", param.refjpgfile);
        fflush (fdebug);
    }
    
    
    if (!param.iscolor) {

	if ((strcasecmp (param.cmd, "init") == 0) ||
	    (strcasecmp (param.cmd, "initjsonfile") == 0) ||
	    (strcasecmp (param.cmd, "initjsondata") == 0) ||
	    (strcasecmp (param.cmd, "replaceim") == 0) ||
	    (strcasecmp (param.cmd, "replaceimplane") == 0) ||
	    (strcasecmp (param.cmd, "resetzoom") == 0))
        {	
	    strcpy (param.subsetimfile, "");
        }

        if ((int)strlen(param.shrunkimfile) == 0) {
	    
	    sprintf (param.shrunkimfile, "%s_shrunk.fits",  
	        param.imageFile);
        }
	
	sprintf (param.shrunkRefimfile, "%s_shrunkref.fits",  
	    param.imageFile);

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "subsetimfile= [%s]\n", param.subsetimfile);
	    fprintf (fdebug, "shrunkimfile= [%s]\n", param.shrunkimfile);
	    fprintf (fdebug, "shrunkRefimfile= [%s]\n", param.shrunkRefimfile);
            fflush (fdebug);
	}
    } 
    else {
	if ((strcasecmp (param.cmd, "init") == 0) ||
	    (strcasecmp (param.cmd, "initjsonfile") == 0) ||
	    (strcasecmp (param.cmd, "initjsondata") == 0) ||
	    (strcasecmp (param.cmd, "replaceim") == 0) ||
	    (strcasecmp (param.cmd, "replaceimplane") == 0) ||
	    (strcasecmp (param.cmd, "resetzoom") == 0))
        {	
	    strcpy (param.subsetredfile, "");
	    strcpy (param.subsetgrnfile, "");
	    strcpy (param.subsetbluefile, "");
        }

        if ((int)strlen(param.shrunkredfile) == 0) {
	    sprintf (param.shrunkredfile, "%s_shrunk_%s", 
	        param.imageFile, param.redFile);
        }
        sprintf (param.shrunkRefredfile, "%s_shrunkref_%s", 
	    param.imageFile, param.redFile);

        if ((int)strlen(param.shrunkgrnfile) == 0) {
            sprintf (param.shrunkgrnfile, "%s_shrunk_%s", 
	        param.imageFile, param.greenFile);
        }
        sprintf (param.shrunkRefgrnfile, "%s_shrunkref_%s", 
	    param.imageFile, param.greenFile);
        
        if ((int)strlen(param.shrunkbluefile) == 0) {
            sprintf (param.shrunkbluefile, "%s_shrunk_%s", 
	        param.imageFile, param.blueFile);
        }
        sprintf (param.shrunkRefbluefile, "%s_shrunkref_%s", 
	    param.imageFile, param.blueFile);
	
    }


/*
    Process input command:
    
    init (or new image), zoom, update all need to call makeImage 
    but in addition, 

    "init/new image" -- needs to extract some info (ns, nl, imcsys) 
    from the FITS header, and info (zoomfactor and xflip, yflip)
    from the return structure of running "mViewer" program to add 
    to the JSON struct. 

    "zoom"  needs to compute the sub-image before calling makeImage. 

    "update" has all the necessary info in the input JSON struct
    to just call makeImage.

    "pick" doesn't need to call makeImage, just extract the pixel value 
    and the coordinate.
    
    "waveplot" doesn't need to call makeImage, just extract the wavelength
    spectra into a plot file.
*/
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "XXX\n");
        fflush (fdebug);
    }

    if (strcasecmp (param.cmd, "impick") == 0) { 

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "pick cmd\n");
            fflush (fdebug);
        }
      
	istatus = pick (&param);
   	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "returned pick: istatus= [%d]\n", istatus);
            fflush (fdebug);
        }
                    
	if (istatus == -1) {
            printError (param.errmsg);
        }

    }
    else {
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "XXX here0\n");
            fflush (fdebug);
        }
   
	if ((strcasecmp (param.cmd, "init") == 0) ||
	    (strcasecmp (param.cmd, "initjsonfile") == 0) ||
	    (strcasecmp (param.cmd, "initjsondata") == 0) ||
	    (strcasecmp (param.cmd, "replaceim") == 0)) 
        {     
/*
    First look at image header to check if WCS info exists
*/
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "call getFitshdr\n"); 
                fflush (fdebug);
            }

            istatus = getFitshdr (impath, &hdr, 0);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "returned getFitshdr: istatus= [%d]\n", 
		    istatus); 
	        fflush (fdebug);
            }

            if (istatus == -1) {
                sprintf (param.errmsg, 
                    "Failed to extract FITS image header, errmsg= [%s]", 
		    hdr.errmsg);
                printError (param.errmsg);
            }

            if (param.nowcs == -1) {
                param.nowcs = hdr.nowcs;
            }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "nowcs= [%d]\n", param.nowcs);
	        fflush (fdebug);
            }

            param.imageWidth = hdr.ns;
            param.imageHeight = hdr.nl;
	    strcpy (param.bunit, hdr.bunit);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "imageWidth= [%d] imageHeight= [%d]\n", 
	            param.imageWidth, param.imageHeight);
                fflush (fdebug);
            }

            param.xmin = 0.;
            param.ymin = 0.;
            param.xmax = (double)hdr.ns;
            param.ymax = (double)hdr.nl;


            if (!param.nowcs) {

                if (((int)strlen(hdr.csysstr) > 0) && 
	            ((int)strlen(hdr.epochstr) > 0)) {
                    sprintf (param.imcsys, "%s %s", hdr.csysstr, hdr.epochstr);
                }
                else {
                    strcpy (param.imcsys, "eq j2000");
                }
            }

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "imcsys= [%s]\n", param.imcsys);
                fflush (fdebug);
            }

        }
        else if ((strcasecmp (param.cmd, "movebox") == 0) ||
            (strcasecmp (param.cmd, "zoombox") == 0) ||
            (strcasecmp (param.cmd, "zoomin") == 0) ||
            (strcasecmp (param.cmd, "zoomout") == 0) ||
            (strcasecmp (param.cmd, "replaceimplane") == 0) ||
            (strcasecmp (param.cmd, "resetzoom") == 0)) 
	{
        
        
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "call imZoom\n");
                fflush (fdebug);
            }
    
            istatus = imZoom (&param);
	
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "returned imZoom, istatus= [%d]\n", istatus);
                fprintf (fdebug, "zoomfactor= [%lf] zoomfacorstr= [%s]\n", 
		    param.zoomfactor, param.zoomfactorstr);
                fflush (fdebug);
            }

	    if (istatus == -1) {
                printError (param.errmsg);
            }
   	
	    strcpy (refJpgpath, "");
        }
        

/*
    Make Jpeg image
*/
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "call makeImage\n");
	    fflush (fdebug);
        }
    
        istatus = makeImage (&param);
    
        if (istatus == -1) {
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "errmsg= [%s]\n", param.errmsg);
                fflush (fdebug);
            }
        }

	if (istatus == -1) {
            printError (param.errmsg);
        }
   	
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "returned makeImage: istatus= [%d]\n", istatus);
            fprintf (fdebug, "zoomfactor= [%lf] zoomfacorstr= [%s]\n", 
		param.zoomfactor, param.zoomfactorstr);
	    fflush (fdebug);
        }
    
    }


    istatus = constructRetjson (&param);

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "returned from constructRetjson: retjsonstr=\n");
	fprintf (fdebug, "%s\n", param.jsonStr);
	fflush (fdebug);
	fprintf (fdebug, "\n\n");
	fflush (fdebug);
    }

    printRetval (param.jsonStr);
   
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "returned printRetval\n");
        fflush (fdebug);
    }        
    
    return (0);
}
