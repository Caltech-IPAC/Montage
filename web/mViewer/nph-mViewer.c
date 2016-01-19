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

int rewriteFitsCube (char *cubefile, char *outcubefile, char *errmsg);

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

    struct stat     buf;

    char   *cptr;
    char   str[1024];
    char   suffix[40];
    char   cmd[1024];
    
    char   imroot[1024];
    char   redroot[1024];
    char   grnroot[1024];
    char   blueroot[1024];
    
    char   impath[1024];
    char   redpath[1024];
    char   grnpath[1024];
    char   bluepath[1024];
    
    char   refJpgpath[1024];

    char   imcubefile[1024];
    char   imcubepath[1024];

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
//        sprintf (debugfname, 
//	    "/koa/cm/ws/mihseh/montage/web/mViewer/mviewer.debug");
        
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
    
/*    
    if (cmd[0] == '\0') {
    
        strcpy (param.errmsg, 
	    "Required input parameter \'cmd\' is missing.");

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "errmsg= [%s]\n", param.errmsg);
            fflush (fdebug);
        }
    
        printError (param.errmsg);
    }
*/

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
    Check if input imfile exists
*/
    if (!param.iscolor) {
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "iscolor=0\n");
            fflush (fdebug);
        }
        
        if (param.isimcube) {
        
/*
    Rewrite imcubefile if necessary,
    first check if the input cubefile is a full path name
*/
            strcpy (str, param.imcubefile);
	    
            cptr = (char *)NULL;
            cptr = strrchr (str, '/');

            if (cptr != (char *)NULL) {
	        strcpy (imcubepath, param.imcubefile);
	        strcpy (imcubefile, cptr+1);
            }
            else {
	        strcpy (imcubefile, param.imcubefile);
	        sprintf (imcubepath, "%s/%s", param.directory, imcubefile);
            } 
	    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                fprintf (fp_debug, "imcubefile= [%s]\n", imcubefile);
	        fflush (fp_debug);
            }

	    if (strcasecmp (param.cmd, "init") == 0) {
                
		fileExist = stat (imcubepath, &buf);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "fileExist (ws)= [%d]\n", fileExist); 
	            fflush (fp_debug);
                }

                if (fileExist < 0) {
	            sprintf (param.errmsg, 
	                "Error: input image cube file [%s] doesn't exist.", 
		        param.imcubefile);
                    printError (param.errmsg);
                }

/*
    getFitshdr to analyze if the cube data needs to be re-arranged
*/
                if (debugfile) {
                    fprintf (fp_debug, "call getFitshdr\n");
                    fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                    fflush (fp_debug);
                }
           
                istatus = getFitshdr (imcubepath, &hdr, param.isimcube);

                if (istatus == -1) {
                    printError (param.errmsg);
                }

                if (debugfile) {
                    fprintf (fp_debug, "returned getFitshdr, istatus= [%d]\n", 
		        istatus);
                    fprintf (fp_debug, "naxes[0]= [%d] axisIndx[0]= [%d]\n", 
                        hdr.naxes[0], hdr.axisIndx[0]);
                    fprintf (fp_debug, "naxes[1]= [%d] axisIndx[1]= [%d]\n", 
                        hdr.naxes[1], hdr.axisIndx[1]);
                    fprintf (fp_debug, "naxes[2]= [%d] axisIndx[2]= [%d]\n", 
                        hdr.naxes[2], hdr.axisIndx[2]);
                    fflush (fp_debug);
                }


                if ((hdr.axisIndx[0] == 0) && 
                    (hdr.axisIndx[1] == 1) && 
                    (hdr.axisIndx[2] == 2)) {
/*
    Use original cube file
*/
                    strcpy (param.imcubefile, imcubefile);
                    strcpy (param.imcubepath, imcubepath);
                    
		    strcpy (param.imcubepathOrig, "");
		}
		else {
                    strcpy (str, imcubefile);
	    
                    cptr = (char *)NULL;
                    cptr = strrchr (str, '.');

                    if (cptr != (char *)NULL) {
                        *cptr = '\0';
                    }

                    sprintf (param.imcubefile, "%s_dbl.fits", str);            
                    sprintf (param.imcubepath, "%s/%s_dbl.fits", 
		        param.directory, str);            
             
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                        fprintf (fp_debug, "outcubepath= [%s]\n", 
			    param.imcubepath);
	                fflush (fp_debug);
                    }

		    strcpy (param.imcubepathOrig, imcubepath);
                    
		    istatus = rewriteFitsCube (imcubepath, 
		        param.imcubepath, param.errmsg);

                    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                        fprintf (fdebug, 
			    "returned rewriteFitsCube: istatus= [%d]\n",
		            istatus);
                        fflush (fdebug);
                    }
       
                    if (istatus < 0) {
                        printError (param.errmsg);
	            }
		}
	    
/*
    getFitshdr again after cube has been re-arranged
*/
                if (debugfile) {
                    fprintf (fp_debug, "call getFitshdr after re-arrange\n");
                    fprintf (fp_debug, "imcubepath= [%s]\n", param.imcubepath);
                    fflush (fp_debug);
                }
           
                istatus = getFitshdr (param.imcubepath, &hdr, param.isimcube);

                if (istatus == -1) {
                    printError (param.errmsg);
                }

                if (debugfile) {
                    fprintf (fp_debug, "returned getFitshdr, istatus= [%d]\n", 
		        istatus);
                    fprintf (fp_debug, "naxes[0]= [%d] axisIndx[0]= [%d]\n", 
                        hdr.naxes[0], hdr.axisIndx[0]);
                    fprintf (fp_debug, "naxes[1]= [%d] axisIndx[1]= [%d]\n", 
                        hdr.naxes[1], hdr.axisIndx[1]);
                    fprintf (fp_debug, "naxes[2]= [%d] axisIndx[2]= [%d]\n", 
                        hdr.naxes[2], hdr.axisIndx[2]);
                    fflush (fp_debug);
                }
	   
/*
    adjust nplaneave if it exceeds the number of plane
*/
                param.nfitsplane = hdr.naxes[2];
               

                strcpy (param.objname, hdr.objname);
		strcpy (param.filter, hdr.filter);
                strcpy (param.pixscale, hdr.pixscale);

		strcpy (param.ctype3, hdr.ctype[2]);
                param.crval3 = hdr.crval[2];
                param.cdelt3 = hdr.cdelt[2];

		if (debugfile) {
                    fprintf (fp_debug, "nfitsplane= [%d]\n", param.nfitsplane);
                    
		    fprintf (fp_debug, "objname= [%s]\n", param.objname);
                    fprintf (fp_debug, "filter= [%s]\n", param.filter);
                    fprintf (fp_debug, "pixscale= [%s]\n", param.pixscale);
                    
		    fprintf (fp_debug, "ctype3= [%s]\n", param.ctype3);
                    fprintf (fp_debug, "crval3= [%lf]\n", param.crval3);
                    fprintf (fp_debug, "cdelt3= [%lf]\n", param.cdelt3);
                }

                if (param.nplaneave > param.nfitsplane)
		    param.nplaneave = param.nfitsplane;
                if (param.centerplane > param.nfitsplane)
		    param.centerplane = (param.startplane + param.nplaneave)/2;
                
		if (debugfile) {
                    fprintf (fp_debug, "nplaneave adjusted: nplaneave= [%d]\n",
		        param.nplaneave);
                    fprintf (fp_debug, "centerplane= [%d]\n", 
		        param.centerplane);
                    fprintf (fp_debug, "startplane= [%d]\n", param.startplane);
                    fflush (fp_debug);
                }
           
	    }
	    else {
                strcpy (param.imcubefile, imcubefile);
                strcpy (param.imcubepath, imcubepath);
	    }

            if (debugfile) {
                fprintf (fp_debug, "imcubepath= [%s]\n", param.imcubepath);
                fprintf (fp_debug, "startplane= [%d]\n", param.startplane);
                fprintf (fp_debug, "nplaneave adjusted: nplaneave= [%d]\n",
		    param.nplaneave);
                fprintf (fp_debug, "centerplane= [%d]\n", param.centerplane);
                fflush (fp_debug);
            }
	   
/*
    extract one implane
*/
	    strcpy (param.grayFile, "implane.fits");
            sprintf (impath, "%s/%s", param.directory, param.grayFile);
	    
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "impath= [%s]\n", impath);
            }

	    if ((strcasecmp (param.cmd, "init") == 0) ||
	        (strcasecmp (param.cmd, "replaceimplane") == 0))  
	    {
                if (strcasecmp (param.imcubemode, "ave") == 0) {

		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "call extractAvePlane\n");
		        fflush (fp_debug);
                    }
		    
		    istatus = extractAvePlane (param.imcubepath, 
		        impath, param.centerplane, param.nplaneave, 
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
		        impath, param.centerplane, param.nplaneave,
		        &param.startplane, &param.endplane, param.errmsg);
            
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, 
		            "returned generateMedianPlane: istatus= [%d]\n",
		            istatus);
                    }

                    if (istatus == -1) {
                        printError (param.errmsg);
                    }

                }
            }
	    else {
	        fileExist = checkFileExist (param.grayFile, imroot, 
	            suffix, param.directory, impath);

                if ((debugfile) && (fdebug != (FILE *)NULL)) {
                    fprintf (fdebug, 
		        "returned checkFileExist (ws): fileExist[%s]= [%d]\n", 
	                param.grayFile, fileExist);
                }
   
                if (!fileExist) {
	            sprintf (param.errmsg, 
	            "Cannot find required FITS image file in workspace [%s].",
	                param.directory);
	            printError (param.errmsg);
	        }
            }
        }
	else {
	    fileExist = checkFileExist (param.grayFile, imroot, 
	        suffix, param.directory, impath);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned checkFileExist (ws): fileExist[%s]= [%d]\n", 
	            param.grayFile, fileExist);
            }
   
            if (!fileExist) {
	        sprintf (param.errmsg, 
	            "Cannot find required FITS image file in workspace [%s].",
	            param.directory);
	        printError (param.errmsg);
	    }
        } 
    }
    else {
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "iscolor=1\n");
            fflush (fdebug);
        }
        
        redExist = checkFileExist (param.redFile, redroot, 
	    suffix, param.directory, redpath);
            
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "returned checkFileExist: redExist= [%d]\n", 
	        redExist);
            fflush (fdebug);
        }
        
	grnExist = checkFileExist (param.greenFile, grnroot, 
            suffix, param.directory, grnpath);

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "returned checkFileExist: grnExist= [%d]\n", 
	        grnExist);
            fflush (fdebug);
        }
        
        blueExist = checkFileExist (param.blueFile, blueroot, 
	    suffix, param.directory, bluepath);

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "returned checkFileExist: blueExist= [%d]\n", 
	        blueExist);
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
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "here2\n");
        fflush (fdebug);
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
	    (strcasecmp (param.cmd, "replaceimplane") == 0) ||
	    (strcasecmp (param.cmd, "resetzoom") == 0))
        {	
	    strcpy (param.subsetimfile, "");
        }

        if ((int)strlen(param.shrunkimfile) == 0) {
	    sprintf (param.shrunkimfile, "%s_shrunk_%s",  
	        param.imageFile, param.grayFile);
        }
	
	sprintf (param.shrunkRefimfile, "%s_shrunkref_%s",  
	    param.imageFile, param.grayFile);
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "subsetimfile= [%s]\n", param.subsetimfile);
	    fprintf (fdebug, "shrunkimfile= [%s]\n", param.shrunkimfile);
	    fprintf (fdebug, "shrunkRefimfile= [%s]\n", param.shrunkRefimfile);
            fflush (fdebug);
	}
    } 
    else {
	if ((strcasecmp (param.cmd, "init") == 0) ||
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
    else if (strcasecmp (param.cmd, "waveplot") == 0) { 
        
        if (strcasecmp (param.waveplottype, "pix") == 0) {
             
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "call pick routine\n");
                fflush (fdebug);
            }
      
	    istatus = pick (&param);
	
	    if (istatus == -1) {
                printError (param.errmsg);
            }
   	
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	        fprintf (fdebug, "returned pick: istatus= [%d]\n", istatus);
                fflush (fdebug);
            }

        }

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "waveplot cmd\n");
            fflush (fdebug);
        }
      
	istatus = extractWaveSpectra (&param);
	    
	if (istatus == -1) {
            printError (param.errmsg);
        }
   	
   	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "returned extractWaveSpectra: istatus= [%d]\n", 
	        istatus);
            fflush (fdebug);
        }
    }
    else {
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "XXX here0\n");
            fflush (fdebug);
        }
   
	if (strcasecmp (param.cmd, "init") == 0) 
        {     
/*
    First look at image header to check if WCS info exists
*/

            impath[0] = '\0';
            if (!param.iscolor) {
                sprintf (impath, "%s/%s", param.directory, param.grayFile);
            }
            else {
                sprintf (impath, "%s/%s", param.directory, param.redFile);
            }

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


/*
    Don't need to subset file 
*/
/*
            if (!param.iscolor) {
                strcpy (param.subsetimfile, param.grayFile); 
             
                if ((debugfile) && (fdebug != (FILE *)NULL)) {
	            fprintf (fdebug, "subsetimfile= [%s]\n", 
		        param.subsetimfile);
                    fflush (fdebug);
	        }
            } 
            else {
                strcpy (param.subsetredfile, param.redFile);
                strcpy (param.subsetgrnfile, param.greenFile);
                strcpy (param.subsetbluefile, param.blueFile);
            }
*/


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
	    fflush (fdebug);
        }
    
    }


    istatus = constructRetjson (&param);

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "returned from constructRetjson\n");
	fprintf (fdebug, "\nretjsonstr= [%s]\n", param.jsonStr);
	fflush (fdebug);
    }

    printRetval (param.jsonStr);
   
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "returned printRetval\n");
        fflush (fdebug);
    }        
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fclose (fdebug);
    } 
    
    return (0);
}
