/*
theme:  

extract input keywords from cgi stream, read parameters from an init param file,
and read filenames from imlist and tbllist

make jsonfile from the init param file content.

date: march 12, 2015 (mihseh kong)
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
#include <password.h>
#include <cmd.h>
#include <svc.h>
#include <varcmd.h>

#include "viewerapp.h"
#include "fitshdr.h"


void upper (char *);

char *strtrim (char *);
int str2Integer (char *strval, int *intval, char *errmsg);
int str2Double (char *strval, double *dblval, char *errmsg);

int readStartupParam (struct ViewerApp *param, char *initparampath);

int checkFileExist (char *fname, char *rootname, char *suffix,
    char *directory, char *filepath);
int fileCopy (char *from, char *to, char *errmsg);

int getFitshdr (char *fromfile, struct FitsHdr *hdr);

int rewriteFitsCube (char *cubefile, char *outcubefile, char *errmsg);


extern FILE *fp_debug;

int extractStartupParam (struct ViewerApp *param)
{
    struct FitsHdr hdr;
    
    struct stat     buf;

    char     *cptr;

    char     varstr[32768];
    char     status[32];

    char     rootname[1024];
    char     suffix[1024];
    char     baseurl[1024];
    char     directory[1024];
    char     http_port[1024];
    char     http_srvr[1024];
   
    char     jsonfile[1024];
    char     jsonpath[1024];

    char     paramfile[1024];
    char     parampath[1024];
    char     str[1024];
   
    char     paramtemplate[1024];
    char     paramtemplatepath[1024];
    
    int      paramfileexist;
    int      paramtemplateexist;

    char     imcubefile_in[1024];
    char     imcubefile[1024];
    char     imcubepath[1024];
    
    char     grayimfile_in[1024];
    char     grayimfile[1024];
    char     grayimpath[1024];
    
    char     redfile[1024];
    char     grnfile[1024];
    char     bluefile[1024];


/*
    char     fpath[1024];
    char     fpath2[1024];
*/

    int      fileexist;

    int      istatus;

    int      debugfile = 0;

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "from extractStartupParam\n");
        fflush (fp_debug); 
    }

/*
    extract configuration keywords
*/
    baseurl[0] = '\0';
    directory[0] = '\0';
    if (config_value("ISIS_WORKURL") != (char *)NULL) {
        strcpy(baseurl, config_value("ISIS_WORKURL"));
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "baseurl= [%s]\n", baseurl);
            fflush (fp_debug); 
        }
    }
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
   
    http_port[0] = '\0';
    strcpy (http_srvr, "http://");

    if (config_value("HTTP_URL") != (char *)NULL)
        strcat(http_srvr, config_value("HTTP_URL"));
    else { 
        strcpy (param->errmsg, 
	    "HTTP_URL configuration variable is undefined!");
	return (-1);
    }
   
    if (config_value("HTTP_PORT") != (char *)NULL)
        strcat(http_port, config_value("HTTP_PORT"));
    else { 
        strcpy (param->errmsg, 
	    "HTTP_PORT configuration variable is undefined!");
	return (-1);
    }
 
    if(strcmp(http_port, "80") != 0) {
        strcat (http_srvr, ":");
        strcat (http_srvr, http_port);
    }

    strcpy(param->http_srvr, http_srvr);

    
    param->project[0] = '\0';
    if(keyword_exists("project")) {
        if (keyword_value("project") != (char *)NULL) 
	    strcpy (param->project, strtrim(keyword_value("project")));
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "param->project= [%s]\n", param->project);
        fflush (fp_debug); 
    }

    upper (param->project);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "param->project= [%s]\n", param->project);
        fflush (fp_debug); 
    }
    
    if ((int)strlen(param->project) == 0) {
        strcpy (param->cookiename, "iceviewapp");
    }
    else {
        strcpy (param->cookiename, param->project);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "param->cookiename= [%s]\n", param->cookiename);
        fflush (fp_debug); 
    }


/* 
    extract input workspace name
*/
    param->workspace[0] = '\0';
    if(keyword_exists("workspace")) {
        if (keyword_value("workspace") != (char *)NULL) 
	    strcpy (param->workspace, strtrim(keyword_value("workspace")));
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "param->workspace= [%s]\n", param->workspace);
        fflush (fp_debug); 
    }

    if ((int)strlen(param->workspace) == 0) {

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "here1: cookiename= [%s]\n", param->cookiename);
            fflush (fp_debug); 
        }

        param->cookiestr[0] = '\0';
        if (cookie_string (param->cookiename) == (char *)NULL) {
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "here2\n");
                fflush (fp_debug);
            }
	}
	else {
            strcpy (param->cookiestr, cookie_string (param->cookiename));
        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "here3: param->cookiestr= [%s]\n", 
		    param->cookiestr);
                fflush (fp_debug);
            }
        }

	if ((int)strlen(param->cookiestr) > 0) {

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "here4\n");
                fflush (fp_debug);
            }
    
	    strcpy (param->workspace, param->cookiestr);
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "workspace retrieved from cookie= [%s]\n", 
		    param->workspace);
                fflush (fp_debug);
            }
	}
	else {
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "here5\n");
                fflush (fp_debug);
            }
    
	    strcpy (param->workspace, (char *)tmplogin());
    
            strcpy (param->cookiestr, param->workspace);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "workspace created by tmplogin= [%s]\n", 
		    param->workspace);
                fflush (fp_debug);
            }
	}
    
    }

    sprintf (param->directory, "%s/%s", directory, param->workspace);
    sprintf (param->baseurl, "%s/%s", baseurl, param->workspace);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "param->baseurl= [%s]\n", param->baseurl);
        fflush (fp_debug); 
    }


/*
    check if workspace exists, if not create a new workspace 
*/
    sprintf (param->directory, "%s/%s", directory, param->workspace);

    fileexist = stat (param->directory, &buf);
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "fileexist= [%d]\n", fileexist);
        fflush (fp_debug); 
    }

    if (fileexist < 0) {
            
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here6\n");
            fflush (fp_debug);
        }
        
	strcpy (param->workspace, (char *)tmplogin());
        strcpy (param->cookiestr, param->workspace);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "new workspace created by tmplogin= [%s]\n", 
		param->workspace);
            fflush (fp_debug);
        }
    }


    sprintf (param->directory, "%s/%s", directory, param->workspace);
    sprintf (param->baseurl, "%s/%s", baseurl, param->workspace);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "param->workspace= [%s]\n", param->workspace);
	fprintf (fp_debug, "param->cookiestr= [%s]\n", param->cookiestr);
	fprintf (fp_debug, "param->directory= [%s]\n", param->directory);
	fprintf (fp_debug, "param->baseurl= [%s]\n", param->baseurl);
        fflush (fp_debug); 
    }


/* 
    input parameter file or template
*/
    param->datadir[0] = '\0';
    if(keyword_exists("datadir")) {
        if (keyword_value("datadir") != (char *)NULL) 
            strcpy (param->datadir, strtrim(keyword_value("datadir")));
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
        fflush (fp_debug); 
    }

    paramtemplate[0] = '\0';
    if(keyword_exists("initparamtemplate")) {
        if (keyword_value("initparamtemplate") != (char *)NULL) 
            strcpy (paramtemplate, 
	        strtrim(keyword_value("initparamtemplate")));
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "paramtemplate= [%s]\n", paramtemplate);
        fflush (fp_debug); 
    }

    paramfile[0] = '\0';
    if(keyword_exists("initparamfile")) {
        if (keyword_value("initparamfile") != (char *)NULL) 
            strcpy (paramfile, strtrim(keyword_value("initparamfile")));
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "paramfile= [%s]\n", paramfile);
        fflush (fp_debug); 
    }

    jsonfile[0] = '\0';
    param->jsonfile[0] = '\0';
    if(keyword_exists("jsonfile")) {
        if (keyword_value("jsonfile") != (char *)NULL) 
            strcpy (jsonfile, strtrim(keyword_value("jsonfile")));
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "jsonfile= [%s]\n", jsonfile);
        fflush (fp_debug); 
    }

    param->iscolor = 0;
    if(keyword_exists("iscolor")) {
        if (keyword_value("iscolor") != (char *)NULL) { 
	    
	    strcpy (str, strtrim(keyword_value("iscolor")));
            if ((int)strlen(str) > 0) {
	        istatus = str2Integer (str, &param->iscolor, param->errmsg);
	    }
	}
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "iscolor= [%d]\n", param->iscolor);
        fflush (fp_debug); 
    }

    param->isimcube = 0;
    if(keyword_exists("isimcube")) {
        if (keyword_value("isimcube") != (char *)NULL) { 
	    
	    strcpy (str, strtrim(keyword_value("isimcube")));
            if ((int)strlen(str) > 0) {
	        istatus = str2Integer (str, &param->isimcube, param->errmsg);
	    }
	}
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "isimcube= [%d]\n", param->isimcube);
        fflush (fp_debug); 
    }


    param->startplane = 1;
    if(keyword_exists("startplane")) {
        if (keyword_value("startplane") != (char *)NULL) { 
	    
	    strcpy (str, strtrim(keyword_value("startplane")));
            if ((int)strlen(str) > 0) {
	        istatus = str2Integer (str, &param->startplane, param->errmsg);
		if (istatus == -1)
		    param->startplane = 1;
	    }
	}
    }
    
    param->nplaneavestr[0] = '\0';
    if(keyword_exists("nplaneave")) {
        if (keyword_value("nplaneave") != (char *)NULL) { 
	    
	    strcpy (str, strtrim(keyword_value("nplaneave")));
            if ((int)strlen(str) > 0) {
	        strcpy (param->nplaneavestr, str);
	    }
	}
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "startplane= [%d]\n", param->startplane);
	fprintf (fp_debug, "nplaneavestr= [%s]\n", param->nplaneavestr);
        fflush (fp_debug); 
    }


/*
    input image file
*/
    imcubefile[0] = '\0';
    imcubefile_in[0] = '\0';
    if (keyword_exists("imcubefile")) {
        if (keyword_value("imcubefile") != (char *)NULL) 
	    strcpy (imcubefile_in, strtrim(keyword_value("imcubefile")));
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "imcubefile_in= [%s]\n", imcubefile_in);
        fflush (fp_debug); 
    }

    grayimfile_in[0] = '\0';
    grayimfile[0] = '\0';
    if (keyword_exists("imfile")) {
        if (keyword_value("imfile") != (char *)NULL) 
	    strcpy (grayimfile_in, strtrim(keyword_value("imfile")));
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "grayimfile_in= [%s]\n", grayimfile_in);
        fflush (fp_debug); 
    }

    redfile[0] = '\0';
    if(keyword_exists("redfile")) {
        if (keyword_value("redfile") != (char *)NULL) 
	    strcpy (redfile, strtrim(keyword_value("redfile")));
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "redfile= [%s]\n", redfile);
        fflush (fp_debug); 
    }

    grnfile[0] = '\0';
    if(keyword_exists("grnfile")) {
        if (keyword_value("grnfile") != (char *)NULL) 
	    strcpy (grnfile, strtrim(keyword_value("grnfile")));
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "grnfile= [%s]\n", grnfile);
        fflush (fp_debug); 
    }

    bluefile[0] = '\0';
    if(keyword_exists("bluefile")) {
        if (keyword_value("bluefile") != (char *)NULL) 
	    strcpy (bluefile, strtrim(keyword_value("bluefile")));
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "bluefile= [%s]\n", bluefile);
	fprintf (fp_debug, "parampath= [%s]\n", param->parampath);
        fflush (fp_debug); 
    }


/*
    if it is imcube, extract nplane frome fits header
*/
    param->nfitsplane = 1;

    if (param->isimcube) {
            
        strcpy (str, imcubefile_in);
	    
	cptr = (char *)NULL;
	cptr = strrchr (str, '/');

        if (cptr != (char *)NULL) {
/*
    input cubefile is a full path; 
    check if file exists
*/
	    strcpy (imcubefile, cptr+1);
	    strcpy (imcubepath, imcubefile_in);
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                fprintf (fp_debug, "imcubefile= [%s]\n", imcubefile);
	        fflush (fp_debug);
            }

            fileexist = stat (imcubepath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (fullpath)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }
            
	    if (fileexist < 0) {
	        sprintf (param->errmsg, 
		    "error: input image cube file [%s] doesn't exist.", 
		    imcubefile_in);
	        return (-1); 
            } 

/*
	    strcpy (param->imcubepath, imcubepath);
	    strcpy (param->imcubefile, imcubefile);
*/

	}
	else {
/*
    check if file exists in the workspace directory
*/
	    strcpy (imcubefile, imcubefile_in);
	    sprintf (imcubepath, "%s/%s", param->directory, imcubefile);
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                fprintf (fp_debug, "imcubefile= [%s]\n", imcubefile);
	        fflush (fp_debug);
            }

            fileexist = stat (imcubepath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (ws)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }

/*
	    strcpy (param->imcubefile, imcubefile);
*/

	    if (fileexist >= 0) {
//	        strcpy (param->imcubepath, imcubepath);
	    }
	    else {
/*
    not in ws directory: check datadir for imcubefile
*/
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
	            fflush (fp_debug);
                }

	        sprintf (imcubepath, "%s/%s", param->datadir, imcubefile);
	        
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
		    fflush (fp_debug);
                }
	    
                fileexist = stat (imcubepath, &buf);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "fileexist (datadir)= [%d]\n", 
		        fileexist); 
		    fflush (fp_debug);
                }
                
	        if (fileexist >= 0) {
		   
/*
		    sprintf (param->imcubepath, "%s/%s", 
		        param->directory, imcubefile);
*/
		    sprintf (str, "%s/%s", param->directory, imcubefile);

		    istatus = fileCopy (imcubepath, str, param->errmsg);  
                    
		    if (istatus < 0) {
		        return (-1);
		    }
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "copy [%s] to [%s]\n",
		            imcubepath, str);
                        fflush (fp_debug);  
                    }
		   
		    strcpy (imcubepath, str);

		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                        fflush (fp_debug);  
                    }
	        }
	        else {
	            sprintf (param->errmsg, 
		        "error: input image cube file [%s] doesn't exist.", 
		        imcubefile);
	            return (-1); 
                }     
	    }
	} 

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
            fprintf (fp_debug, "imcubefile= [%s]\n", imcubefile);
	    fflush (fp_debug);
        }

	
	istatus = getFitshdr (imcubepath, &hdr); 
	    
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "returned getFitshdr: istatus= [%d]\n",
	        istatus);
            fprintf (fp_debug, "naxes[0]= [%d] axisIndx[0]= [%d]\n", 
                hdr.naxes[0], hdr.axisIndx[0]);
            fprintf (fp_debug, "naxes[1]= [%d] axisIndx[1]= [%d]\n", 
                hdr.naxes[1], hdr.axisIndx[1]);
            fprintf (fp_debug, "naxes[2]= [%d] axisIndx[2]= [%d]\n", 
                hdr.naxes[2], hdr.axisIndx[2]);
	    fflush (fp_debug);
        }

        if (istatus != 0) {
	    sprintf (param->errmsg, 
	        "failed to extract fits header from input image file [%s]\n",
		param->grayfile);
            return (-1);
	}
            
	param->nfitsplane = hdr.nplane;
            
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "nfitsplane= [%d]\n", param->nfitsplane);
            fflush (fp_debug);
        }
           
	strcpy (param->imcubefile, imcubefile);
        strcpy (param->imcubepath, imcubepath);
       

/*
    Cube re-arrange is needed
*/
/*
	if ((hdr.axisIndx[0] == 0) &&
	    (hdr.axisIndx[1 == 1]) &&
	    (hdr.axisIndx[2] == 2))
	{
           strcpy (param->imcubefile, imcubefile);
           strcpy (param->imcubepath, imcubepath);
        }
	else {
            strcpy (str, imcubefile);
	    
            cptr = (char *)NULL;
            cptr = strrchr (str, '.');

            if (cptr != (char *)NULL) {
                *cptr = '\0';
            }

            sprintf (param->imcubefile, "%s_dbl.fits", str);            
            sprintf (param->imcubepath, "%s/%s", param->directory, 
	        param->imcubefile);            
             
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                fprintf (fp_debug, "param->imcubepath= [%s]\n", 
		    param->imcubepath);
	        fflush (fp_debug);
            }

            istatus = rewriteFitsCube (imcubepath, param->imcubepath, 
	        param->errmsg);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, 
		    "returned rewriteFitsCube: istatus= [%d]\n", istatus);
                fprintf (fp_debug, "param->imcubefile= [%s]\n", 
		    param->imcubefile);
                fprintf (fp_debug, "param->imcubepath= [%s]\n", 
		    param->imcubepath);
                fflush (fp_debug);
            }
       
            if (istatus < 0) {
                return (-1);
	    }
        }   
*/


    }
    else {
        strcpy (str, grayimfile_in);
	    
	cptr = (char *)NULL;
	cptr = strrchr (str, '/');

        if (cptr != (char *)NULL) {
/*
    input grayimfile_in is a full path; 
    check if file exists
*/
	    strcpy (grayimfile, cptr+1);
	    strcpy (grayimpath, grayimfile_in);
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "input grayimfile is full path= [%s]\n", 
		    grayimfile_in);
                fprintf (fp_debug, "grayimpath= [%s]\n", grayimpath);
                fprintf (fp_debug, "grayimfile= [%s]\n", grayimfile);
	        fflush (fp_debug);
            }

            fileexist = stat (grayimpath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (fullpath)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }
            
	    if (fileexist < 0) {
	        sprintf (param->errmsg, 
		    "error: input image file [%s] doesn't exist.", 
		    grayimfile_in);
	        return (-1); 
            } 

	    strcpy (param->graypath, grayimpath);
	    strcpy (param->grayfile, grayimfile);
	
	}
	else {
/*
    check if file exists in the workspace directory
*/
	    strcpy (grayimfile, grayimfile_in);
	    sprintf (grayimpath, "%s/%s", param->directory, grayimfile);
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "input grayimfile is not full path= [%s]\n", 
		    grayimfile_in);
                fprintf (fp_debug, "grayimpath= [%s]\n", grayimpath);
                fprintf (fp_debug, "grayimfile= [%s]\n", grayimfile);
	        fflush (fp_debug);
            }

            fileexist = stat (grayimpath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (ws)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }

	    strcpy (param->grayfile, grayimfile);
	    
	    if (fileexist >= 0) {
	        strcpy (param->graypath, grayimpath);
	    }
	    else {
/*
    not in ws directory: check datadir for grayimfile
*/
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
	            fflush (fp_debug);
                }

	        sprintf (grayimpath, "%s/%s", param->datadir, grayimfile);
	        
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "grayimpath= [%s]\n", grayimpath);
		    fflush (fp_debug);
                }
	    
                fileexist = stat (grayimpath, &buf);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "fileexist (datadir)= [%d]\n", 
		        fileexist); 
		    fflush (fp_debug);
                }
                
	        if (fileexist >= 0) {
		    
		    sprintf (param->graypath, "%s/%s", 
		        param->directory, grayimfile);
	        
		    istatus = fileCopy (grayimpath, param->graypath, 
		        param->errmsg);  
                    
		    if (istatus < 0) {
		        return (-1);
		    }
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "copy [%s] to [%s]\n",
		            grayimpath, param->graypath);
                        fflush (fp_debug);  
                    }
	        }
	        else {
	            sprintf (param->errmsg, 
		        "error: input image  file [%s] doesn't exist.", 
		        grayimfile);
	            return (-1); 
                }     
	    }
	} 

    }
       

/*
    jsonfile
*/
    if ((int)strlen(jsonfile) > 0) {
        
	strcpy (str, jsonfile);
	    
	cptr = (char *)NULL;
	cptr = strrchr (str, '/');

        if (cptr != (char *)NULL) {
/*
    input jsonfile is a full path; 
    check if file exists
*/
	    strcpy (param->jsonfile, cptr+1);
	    strcpy (param->jsonpath, jsonfile);
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "input jsonfile is full path= [%s]\n", 
		    jsonfile);
                fprintf (fp_debug, "jsonpath= [%s]\n", param->jsonpath);
                fprintf (fp_debug, "jsonfile= [%s]\n", param->jsonfile);
	        fflush (fp_debug);
            }

            fileexist = stat (param->jsonpath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (fullpath)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }
            
	    if (fileexist < 0) {
	        sprintf (param->errmsg, 
		    "error: input jsonfile [%s] doesn't exist.", jsonfile);
	        return (-1); 
            } 
	}
	else {
/*
    check if jsonfile exists in the workspace directory
*/
	    strcpy (param->jsonfile, jsonfile);
	    
	    sprintf (jsonpath, "%s/%s", param->directory, jsonfile);
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "input jsonfile is not full path= [%s]\n", 
		    jsonfile);
                fprintf (fp_debug, "jsonpath= [%s]\n", jsonpath);
	        fflush (fp_debug);
            }

            fileexist = stat (jsonpath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (ws)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }

	    
	    if (fileexist >= 0) {
	        strcpy (param->jsonpath, jsonpath);
	    }
	    else {
/*
    not in ws directory: check datadir for grayimfile
*/
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
	            fflush (fp_debug);
                }

	        sprintf (jsonpath, "%s/%s", param->datadir, jsonfile);
	        
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "jsonpath= [%s]\n", jsonpath);
		    fflush (fp_debug);
                }
	    
                fileexist = stat (jsonpath, &buf);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "fileexist (datadir)= [%d]\n", 
		        fileexist); 
		    fflush (fp_debug);
                }
                
	        if (fileexist >= 0) {
		    
		    sprintf (param->jsonpath, "%s/%s", 
		        param->directory, jsonfile);
	        
		    istatus = fileCopy (jsonpath, param->jsonpath, 
		        param->errmsg);  
                    
		    if (istatus < 0) {
		        return (-1);
		    }
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "copy [%s] to [%s]\n",
		            jsonpath, param->jsonpath);
                        fflush (fp_debug);  
                    }
	        }
	        else {
	            sprintf (param->errmsg, 
		        "error: input jsonfile [%s] doesn't exist.", jsonfile); 
                }
	    }
        } 
    }


/*
    if paramfile exists, read init paramfile; 
*/

    if ((int)strlen(paramfile) > 0) {
    
        paramfileexist = 0;
        
	strcpy (str, paramfile);
	    
        cptr = (char *)NULL;
        cptr = strrchr (str, '/');

        if (cptr != (char *)NULL) {
/*
    input paramfile is a full path; 
    check if file exists
*/
	    strcpy (paramfile, cptr+1);
	    strcpy (parampath, paramfile);
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "parampath= [%s]\n", parampath);
                fprintf (fp_debug, "paramfile= [%s]\n", paramfile);
	        fflush (fp_debug);
            }

            fileexist = stat (parampath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (fullpath paramfile)= [%d]\n", 
	            fileexist); 
	        fflush (fp_debug);
            }
            
	    if (fileexist < 0) {
	        sprintf (param->errmsg, 
		    "error: input param file [%s] doesn't exist.", 
		    paramfile);
	        return (-1); 
            }     
    
            paramfileexist = 1;

	    strcpy (param->paramfile, paramfile);
	    strcpy (param->parampath, parampath);
    
        }
	else {
/*
    check if file exists in workspace directory
*/
            paramfileexist = 0;

	    paramfileexist = checkFileExist (paramfile, rootname, suffix,
                param->directory, parampath);
        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "paramfileexist= [%d]\n", paramfileexist);
                fflush (fp_debug); 
            }

    
            if (paramfileexist) {
	        strcpy (param->paramfile, paramfile);
                sprintf (param->parampath, "%s/%s", 
	            param->directory, param->paramfile);
            }
	    else {
/*
    check datadir directory
*/
  	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "datadir= [%s]\n", param->datadir);
	            fprintf (fp_debug, "directory= [%s]\n", param->directory);
                    fflush (fp_debug); 
                }

	        if (((int)strlen(param->datadir) > 0) &&
	            (strcmp (param->datadir, param->directory) != 0)) {

	            paramfileexist = checkFileExist (paramfile, rootname, 
		        suffix, param->datadir, parampath);
        
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "paramfileexist= [%d]\n", 
			    paramfileexist);
                        fflush (fp_debug); 
                    }
            
                    if (!paramfileexist) {
	                sprintf (param->errmsg, 
		            "error: cannot find param file [%s].", paramfile);
                        return (-1);
                    }

/*
    paramfile in datadir, copy from datadir to workspace
*/
	            strcpy (param->paramfile, paramfile);
	            sprintf (param->parampath, "%s/%s", param->directory, 
		        paramfile);
	    
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "paramfile= [%s]\n", paramfile);
	                fprintf (fp_debug, "copy [%s] to [%s]\n", 
		            parampath, param->parampath);
                        fflush (fp_debug); 
                    }
	
	            istatus = fileCopy (parampath, param->parampath, 
		        param->errmsg);  
	    
	            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, 
			    "istatus (paramfile fileCopy)= [%d]\n", istatus);
                        fflush (fp_debug); 
                    }

                    if (istatus < 0) {
                        return (-1);
	            }
	        }
            }

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "paramfile= [%s]\n", param->paramfile);
	        fprintf (fp_debug, "parampath= [%s]\n", param->parampath);
                fflush (fp_debug); 
            }
	}
    }
    else if ((int)strlen(paramtemplate) > 0) {
    
        paramtemplateexist = 0;
       
        strcpy (str, paramtemplate);
	    
        cptr = (char *)NULL;
        cptr = strrchr (str, '/');

        if (cptr != (char *)NULL) {
/*
    input paramtemplate is a full path; 
    check if template file exists
*/
	    strcpy (paramtemplate, cptr+1);
	    strcpy (paramtemplatepath, paramtemplate);
	
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "xxx1: paramtemplatepath= [%s]\n", 
		    paramtemplatepath);
                fprintf (fp_debug, "paramtemplate= [%s]\n", paramtemplate);
	        fflush (fp_debug);
            }

            fileexist = stat (paramtemplatepath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, 
		    "fileexist (fullpath paramtemplate)= [%d]\n", fileexist); 
	        fflush (fp_debug);
            }
            
	    if (fileexist < 0) {
	        sprintf (param->errmsg, 
		    "error: input param template [%s] doesn't exist.", 
		    paramtemplate);
	        return (-1); 
            }     
    
            paramtemplateexist = 1;
        }
	else {
/*
    check if file exists in workspace directory
*/
	    paramtemplateexist = checkFileExist (paramtemplate, rootname, 
	        suffix, param->directory, paramtemplatepath);
        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "xxx2: paramtemplateexist= [%d]\n", 
		    paramtemplateexist);
	        fprintf (fp_debug, "paramtemplate= [%s]\n", paramtemplate);
	        fprintf (fp_debug, "paramtemplatepath= [%s]\n", 
		    paramtemplatepath);
                fflush (fp_debug); 
            }

    
            if (!paramtemplateexist) {
/*
    check datadir directory
*/
  	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "xxx3: datadir= [%s]\n", param->datadir);
	            fprintf (fp_debug, "directory= [%s]\n", param->directory);
                    fflush (fp_debug); 
                }

	        if (((int)strlen(param->datadir) > 0) &&
	            (strcmp (param->datadir, param->directory) != 0)) {

	            paramtemplateexist = checkFileExist (paramtemplate, 
		        rootname, suffix, param->datadir, paramtemplatepath);
        
                    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "paramtemplateexist= [%d]\n", 
			    paramtemplateexist);
	                fprintf (fp_debug, "paramtemplate= [%s]\n", 
			    paramtemplate);
	                fprintf (fp_debug, "paramtemplatepath= [%s]\n", 
			    paramtemplatepath);
                        fflush (fp_debug); 
                    }
            
                    if (!paramtemplateexist) {
	                sprintf (param->errmsg, 
		            "error: cannot find param template file [%s].", 
			    paramtemplate);
                        return (-1);
                    }
		}
	    }

	}
  	        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "paramtemplate= [%s]\n", paramtemplate);
            fprintf (fp_debug, "paramtemplatepath= [%s]\n", paramtemplatepath);
            fflush (fp_debug); 
        }


/*
    Make paramfile from paramtemplate
*/
        if ((int)strlen(param->paramfile) == 0) {
        
	    strcpy (str, paramtemplate);
	
	    cptr = (char *)NULL;
	    cptr = strrchr (str, '.');
	    if (cptr != (char *)NULL) {
                *cptr = '\0';
	    }
       
            strcpy (param->paramfile, str);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, "paramfile= [%s]\n", param->paramfile);
                fflush (fp_debug); 
            }

	    cptr = (char *)NULL;
	    cptr = strrchr (str, '.');
	    if (cptr != (char *)NULL) {
                strcpy (suffix, cptr+1);
	    }

	    if (strcasecmp (suffix, "inparam") != 0) {
                sprintf (param->paramfile, "%s.inparam", param->paramfile);
	    }
	    sprintf (param->parampath, "%s/%s", 
	        param->directory, param->paramfile);
        }

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "paramtemplatepath= [%s]\n", paramtemplatepath);
	    fprintf (fp_debug, "paramfile= [%s]\n", param->paramfile);
	    fprintf (fp_debug, "parampath= [%s]\n", param->parampath);
	    fprintf (fp_debug, "nfitsplane= [%d]\n", param->nfitsplane);
	    fprintf (fp_debug, "startplane= [%d]\n", param->startplane);
	    fprintf (fp_debug, "centerplane= [%d]\n", param->centerplane);
	    fprintf (fp_debug, "nplaneave= [%d]\n", param->nplaneave);
            fflush (fp_debug); 
        }
    
/*
    Make input paramfile from input param template file 
*/   
        varcmd (varstr, 32768, 
            "htmlgen",
	    "%s",                                paramtemplatepath,
	    "%s",                                param->parampath,
	    "datadir",	          "%s",          param->datadir,
	    "nplane",             "%d",          param->nfitsplane,
	    "startplane",         "%d",          param->startplane,
	    "END_PARM");

    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "varstr= [%s]\n", varstr);
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
            
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "errmsg= [%s]\n", param->errmsg);
                fflush (fp_debug);
            }
    
	    return (-1);
        }
    
    }


/*
    read init param file
*/
    fileexist = stat (param->parampath, &buf);
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "fileexist= [%d]\n", fileexist);
        fflush (fp_debug); 
    }

    if (fileexist < 0) {
	sprintf (param->errmsg, 
	    "failed to find required input parameter file [%s].",
	param->parampath);
	return (-1);
    }


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "\ncall readStartupParam: iscolor= [%d]\n", 
	    param->iscolor);
        fflush (fp_debug); 
    }

    istatus = readStartupParam (param, param->parampath);
        
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "\nreturned readStartupParam: istatus= [%d]\n", 
	    istatus);
	fprintf (fp_debug, "imname= [%s]\n", param->imname);
        fflush (fp_debug); 
    }

    if (istatus == -1) {
        return (-1);
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
      
        fprintf (fp_debug, "datadir = [%s]\n", param->datadir);
	fprintf (fp_debug, "imlist_gray   = [%s]\n", param->imlist_gray);
	fprintf (fp_debug, "imlist_color   = [%s]\n", param->imlist_color);
	fprintf (fp_debug, "tbllist   = [%s]\n", param->tbllist);
	fprintf (fp_debug, "nim_gray= [%d]\n", param->nim_gray);
	fprintf (fp_debug, "nim_color= [%d]\n", param->nim_color);
	fprintf (fp_debug, "nsrctbl= [%d]\n", param->nsrctbl);
	fprintf (fp_debug, "niminfo= [%d]\n", param->niminfo);
	fprintf (fp_debug, "isimcube= [%d]\n", param->isimcube);
	
	fprintf (fp_debug, "iscolor= [%d]\n", param->iscolor);
	fprintf (fp_debug, "ngrid= [%d]\n", param->ngrid);
	fprintf (fp_debug, "gridvis[0]= [%d]\n", param->gridvis[0]);
	fprintf (fp_debug, "gridcolor[0]= [%s]\n", param->gridcolor[0]);
        fflush (fp_debug);  
    }


/* 
    check if imfiles exist in ws, if not copy them to ws.
    
*/
    if (!param->iscolor) {

        if (param->isimcube) {
/*            
            strcpy (str, imcubefile);
	    
	    cptr = (char *)NULL;
	    cptr = strrchr (str, '/');

            if (cptr != (char *)NULL) {
		strcpy (imcubepath, imcubefile);
		strcpy (imcubefile, cptr+1);
	    }
	    else {
		sprintf (imcubepath, "%s/%s", param->directory, imcubefile);
	    } 
	    
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
                fprintf (fp_debug, "imcubefile= [%s]\n", imcubefile);
		fflush (fp_debug);
            }

            fileexist = stat (imcubepath, &buf);

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "fileexist (ws)= [%d]\n", fileexist); 
		fflush (fp_debug);
            }

	    strcpy (param->imcubefile, imcubefile);
	    
	    if (fileexist >= 0) {
	        strcpy (param->imcubepath, imcubepath);
	    }
	    else {
*/
/*
    check datadir for imcubefile
*/
/*
		sprintf (imcubepath, "%s/%s", param->datadir, imcubefile);
	        
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "imcubepath= [%s]\n", imcubepath);
		    fflush (fp_debug);
                }
	    
                fileexist = stat (imcubepath, &buf);

                if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "fileexist (datadir)= [%d]\n", 
		        fileexist); 
		    fflush (fp_debug);
                }
                
	        if (fileexist >= 0) {
		    
		    sprintf (param->imcubepath, "%s/%s", 
		        param->directory, imcubefile);
	        
		    istatus = fileCopy (imcubepath, param->imcubepath, 
		        param->errmsg);  
                    
		    if (istatus < 0) {
		        return (-1);
		    }
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	                fprintf (fp_debug, "copy [%s] to [%s]\n",
		            imcubepath, param->imcubepath);
                        fflush (fp_debug);  
                    }
	        }
		else {
	            sprintf (param->errmsg, 
		        "error: input image cube file [%s] doesn't exist.", 
		        imcubefile);
	            return (-1); 
               } 
	    }

	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "imcubepath= [%s]\n", param->imcubepath);
                fprintf (fp_debug, "imcubefile= [%s]\n", param->imcubefile);
	        fflush (fp_debug);
            }
*/

	    strcpy (grayimfile, "implane.fits");
            
            sprintf (param->graypath, "%s/%s", param->directory, grayimfile);
            
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "graypath= [%s]\n", param->graypath);
            }

	}

	
	if ((int)strlen(grayimfile) > 0) {
	        
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "here1: strlen(grayimfile) > 0\n");
            }

	    strcpy (param->grayfile, grayimfile);
	    
            if (param->nim_gray == 0) {
    
	        param->nim_gray = 1;

	        param->imfile = (char **)NULL;
                param->imfile = (char **)malloc(param->nim_gray*sizeof(char *));
                param->impath = (char **)NULL;
                param->impath = (char **)malloc(param->nim_gray*sizeof(char *));
                param->imfile[0] = (char *)NULL;
                param->imfile[0] = (char *)malloc (1024*sizeof(char));
                param->impath[0] = (char *)NULL;
                param->impath[0] = (char *)malloc (1024*sizeof(char));
	    
	        strcpy (param->imfile[0], grayimfile);
                sprintf (param->impath[0], 
		    "%s/%s", param->directory, grayimfile);
	    }
	}
	else {
            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, 
		    "imlist defined but grayfile is not defined\n");
            }

	    if (param->nim_gray == 0) {
	
	        strcpy (param->errmsg, 
	            "required image file or image list not found.\n");
        
	        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "%s\n", param->errmsg); 
                    fflush (fp_debug);  
                }
	        return (-1);
            }
	    
	    param->grayfile[0] = '\0';
            strcpy (param->grayfile, param->imfile[0]);
        }

        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "nim_gray= [%d]\n", param->nim_gray);
	    fprintf (fp_debug, "grayfile= [%s]\n", param->grayfile);
            fflush (fp_debug);  
        }
    
    }
    else {
	if (param->nim_color == 0) {
	
	    if (((int)strlen(redfile) > 0) &&
	        ((int)strlen(grnfile) > 0) &&
	        ((int)strlen(bluefile) > 0)) {
            
	        param->nim_color = 1;

	        param->rimfile = (char **)NULL;
                param->rimfile 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->rimpath = (char **)NULL;
                param->rimpath 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->rimfile[0] = (char *)NULL;
                param->rimfile[0] = (char *)malloc (1024*sizeof(char));
                param->rimpath[0] = (char *)NULL;
                param->rimpath[0] = (char *)malloc (1024*sizeof(char));
	    
	        strcpy (param->rimfile[0], redfile);
                sprintf (param->rimpath[0], "%s/%s", param->directory, redfile);
	
	        param->gimfile = (char **)NULL;
                param->gimfile 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->gimpath = (char **)NULL;
                param->gimpath 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->gimfile[0] = (char *)NULL;
                param->gimfile[0] = (char *)malloc (1024*sizeof(char));
                param->gimpath[0] = (char *)NULL;
                param->gimpath[0] = (char *)malloc (1024*sizeof(char));
	    
	        strcpy (param->gimfile[0], grnfile);
                sprintf (param->gimpath[0], "%s/%s", param->directory, grnfile);
	
	        param->bimfile = (char **)NULL;
                param->bimfile 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->bimpath = (char **)NULL;
                param->bimpath 
		    = (char **)malloc (param->nim_color*sizeof(char *));
                param->bimfile[0] = (char *)NULL;
                param->bimfile[0] = (char *)malloc (1024*sizeof(char));
                param->bimpath[0] = (char *)NULL;
                param->bimpath[0] = (char *)malloc (1024*sizeof(char));
	    
	        strcpy (param->bimfile[0], bluefile);
                sprintf (param->bimpath[0], 
		    "%s/%s", param->directory, bluefile);
	
	        strcpy (param->redfile, param->rimfile[0]);
	        strcpy (param->grnfile, param->gimfile[0]);
	        strcpy (param->bluefile, param->bimfile[0]);
	    }
	}
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "nim_color= [%d]\n", param->nim_color);
            fflush (fp_debug);  
        }
    
    }

    if ((param->nim_gray == 0) && (param->nim_color == 0)) {

	strcpy (param->errmsg, "required input imfile ");
	strcat (param->errmsg, "(grayfile/redfile,grnfile,bluefile)");
	strcat (param->errmsg, 
	    "or imlist (gray imlist/color imlist) is missing.");
	    return (-1);
    }


/* 
    make html files from templates
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "\nhere2\n");
	fprintf (fp_debug, "viewhtml= [%s]\n", param->viewhtml);
        fflush (fp_debug);  
    }
    
/*
    param->viewhtmlpath[0] = '\0';
    fileexist = 0;
    if ((int)strlen(param->viewhtml) > 0) {
       
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "\nviewhtml is specified\n");
            fflush (fp_debug);  
        }
        
	
	fileexist = checkFileExist (param->viewhtml, rootname, suffix,
            param->directory, param->viewhtmlpath);

	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "checkFileExist(viewhtml in workdir): = [%d]\n",
		fileexist);
	    fprintf (fp_debug, "param->viewhtmlpath= [%s]\n", 
	        param->viewhtmlpath);
            fflush (fp_debug);  
        }

        if (!fileexist) {
            
	    fileexist = checkFileExist (param->viewhtml, rootname, suffix,
                param->datadir, fpath);
        
	    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	        fprintf (fp_debug, 
		    "checkFileExist (viewhtml in datadir)= [%d]\n", fileexist);
                fflush (fp_debug);  
            }
		    
	    if (fileexist) {
	    
		istatus = fileCopy (fpath, param->viewhtmlpath, param->errmsg);
		if (istatus < 0) {
		    return (-1);
	        }
		    
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	            fprintf (fp_debug, "copy [%s] to [%s]\n",
		        fpath, param->viewhtmlpath);
                    fflush (fp_debug);  
                }
           }
	}
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here3\n");
        fflush (fp_debug);  
    }
*/
    
    
    if ((int)strlen(param->viewtemplate) == 0) {
        
	if ((int)strlen(param->divname) > 0){
	    sprintf (param->viewtemplate, "%s.template", param->divname);
        }
	else {
	    strcpy (param->viewtemplate, "iceview.template");
	}
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "viewtemplate= [%s]\n", param->viewtemplate);
        fflush (fp_debug);  
    }
    
    return (0);
}

