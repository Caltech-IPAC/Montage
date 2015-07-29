/*
Theme: This routine reads the file list in each nightly data directory
    into a memory array.
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include "viewerapp.h"


char *strtrim(char *);

extern FILE *fp_debug;

int readImfileList (char *filelist, int iscolor, struct ViewerApp *param)
{
    FILE  *fp;

    char  line[1024], str[1024], suffix[20];
    char  *cptr;

    int   nim, nfile;
    int   ifile, mod;
    int   n, i, l;
    int   istatus, len;
    
    int   debugfile = 0;
    
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nFrom readImfileList: filelist= [%s]\n", filelist);
        fprintf (fp_debug, "iscolor= [%d]\n", iscolor);
        fprintf (fp_debug, "nim_gray= [%d] nim_color= [%d]\n", 
	    param->nim_gray, param->nim_color);
	fflush (fp_debug);
    }

/*
    read filelist
*/
    fp = (FILE *)NULL;
    fp = fopen (filelist, "r");

    if (fp == (FILE *)NULL) {
	sprintf (param->errmsg, "Failed to open [%s]\n", filelist);
	return (-1);
    }
        

/*
    count number of files
*/
    nfile = 0;
    while (1) 
    {
        cptr = (char *)NULL;
        cptr = fgets (line, 1024, fp);
        
	if (cptr == (char *)NULL)
	    break;
    
        len = (int)strlen (strtrim(line));
	
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "len= [%d] line= [%s]\n", len, line);
	    fflush (fp_debug);
        }

	if ((line[len-1] == '\r') || (line[len-1] == '\n'))
	    line[len-1] = '\0';
	if ((line[len-2] == '\r') || (line[len-2] == '\n'))
	    line[len-2] = '\0';

	strcpy (str, strtrim(line));

/*
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "len= [%d] str= [%s]\n", len, str);
	    fflush (fp_debug);
        }
*/        
	if (len == 0)
	    continue;
        
        cptr = (char *)NULL;
	cptr = strrchr (str, '.');
       
        suffix[0] = '\0';
	if (cptr != (char *)NULL) {
            strcpy (suffix, cptr+1);
	}

/*
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "suffix= [%s]\n", suffix);
	    fflush (fp_debug);
        }
*/


	if ((strcasecmp (suffix, "fits") != 0) &&
	    (strcasecmp (suffix, "fit") != 0) &&
	    (strcasecmp (suffix, "jpg") != 0) &&
	    (strcasecmp (suffix, "jpeg") != 0)) {
	    
	    sprintf (param->errmsg, 
	        "Image files should be FITS file with '.fits' suffix.");
	    return (-1);
        }

	nfile++;
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "nfile= [%d]\n", nfile);
	fflush (fp_debug);
    }

    if (iscolor) {

        mod = nfile % 3;
	if (mod != 0) {
	    sprintf (param->errmsg, 
	      "File Number for color image display should be divisible by 3.");
	    return (-1);
	}

        nim = nfile/3;
        param->nim_color = nim;
    }
    else {
        nim = nfile;
        param->nim_gray = nim;
    }


    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nnfile= [%d] nim= [%d]\n", nfile, nim);
        fprintf (fp_debug, "param->nim_gray= [%d]\n", param->nim_gray);
        fprintf (fp_debug, "param->nim_color= [%d]\n", param->nim_color);
        fprintf (fp_debug, "iscolor= [%d]\n", iscolor);
	fflush (fp_debug);
    }

    if (nfile == 0) {
	sprintf (param->errmsg, "filelist [%s] contains [%d] file.", 
	    filelist, nfile);
	return (-1);
    }

    if (iscolor) {
        
	param->rimfile = (char **)NULL;
        param->rimfile = (char **)malloc (nim*sizeof(char *));
        param->rimpath = (char **)malloc (nim*sizeof(char *));
        for (l=0; l<nim; l++) {
            param->rimfile[l] = (char *)NULL;
            param->rimfile[l] = (char *)malloc (1024*sizeof(char));
            param->rimpath[l] = (char *)NULL;
            param->rimpath[l] = (char *)malloc (1024*sizeof(char));
        }

	param->gimfile = (char **)NULL;
        param->gimfile = (char **)malloc (nim*sizeof(char *));
        param->gimpath = (char **)malloc (nim*sizeof(char *));
        for (l=0; l<nim; l++) {
            param->gimfile[l] = (char *)NULL;
            param->gimfile[l] = (char *)malloc (1024*sizeof(char));
            param->gimpath[l] = (char *)NULL;
            param->gimpath[l] = (char *)malloc (1024*sizeof(char));
        }

	param->bimfile = (char **)NULL;
        param->bimfile = (char **)malloc (nim*sizeof(char *));
        param->bimpath = (char **)malloc (nim*sizeof(char *));
        for (l=0; l<nim; l++) {
            param->bimfile[l] = (char *)NULL;
            param->bimfile[l] = (char *)malloc (1024*sizeof(char));
            param->bimpath[l] = (char *)NULL;
            param->bimpath[l] = (char *)malloc (1024*sizeof(char));
        }
    }
    else {
        param->imfile = (char **)NULL;
        param->imfile = (char **)malloc (nfile*sizeof(char *));
        param->impath = (char **)NULL;
        param->impath = (char **)malloc (nfile*sizeof(char *));
        for (l=0; l<nfile; l++) {
            param->imfile[l] = (char *)NULL;
            param->imfile[l] = (char *)malloc (1024*sizeof(char));
            param->impath[l] = (char *)NULL;
            param->impath[l] = (char *)malloc (1024*sizeof(char));
        }
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "got here\n");
        fflush (fp_debug);
    }
    
    
    istatus = fseek (fp, 0L, SEEK_SET);
    ifile = 0;
    while (1) 
    {
        if (iscolor) {
	    n = 3;
        }
	else {
	    n = 1;
	}

	for (i=0; i<n; i++) {

            cptr = (char *)NULL;
            cptr = fgets (line, 1024, fp);
        
	    if (cptr == (char *)NULL)
	        break;

            if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                fprintf (fp_debug, "ifile= [%d]\n", ifile);
	        fflush (fp_debug);
            }

            len = (int)strlen(line);
            if ((line[len-1] == '\r') || (line[len-1] == '\n')) 
                line[len-1] = '\0';
            if ((line[len-2] == '\r') || (line[len-2] == '\n')) 
            line[len-2] = '\0';

            if (iscolor) {
	        if (i == 0) { 
		    strcpy (param->rimfile[ifile], strtrim(line));
		    sprintf (param->rimpath[ifile], "%s/%s",
		        param->directory, param->rimfile[ifile]);
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "param->rimfile[%d]= [%s]\n", 
		            ifile, param->rimfile[ifile]);
	                fflush (fp_debug);
                    }
		}
		else if (i == 1) { 
		    strcpy (param->gimfile[ifile], strtrim(line));
		    sprintf (param->gimpath[ifile], "%s/%s",
		        param->directory, param->gimfile[ifile]);
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "para->gimfile[%d]= [%s]\n", 
		            ifile, param->gimfile[ifile]);
	                fflush (fp_debug);
                    }
		}
		else if (i == 2) {
		    strcpy (param->bimfile[ifile], strtrim(line));
		    sprintf (param->bimpath[ifile], "%s/%s",
		        param->directory, param->bimfile[ifile]);
		    
		    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                        fprintf (fp_debug, "param->bimfile[%d]= [%s]\n", 
		            ifile, param->bimfile[ifile]);
	                fflush (fp_debug);
                    }
	            ifile++;
	        }	
	    }
	    else {
	        strcpy (param->imfile[ifile], strtrim(line));
		sprintf (param->impath[ifile], "%s/%s",
		    param->directory, param->imfile[ifile]);
	        
		if ((debugfile) && (fp_debug != (FILE *)NULL)) {
                    fprintf (fp_debug, "param->imfile[%d]= [%s]\n", 
		        ifile, param->imfile[ifile]);
	            fflush (fp_debug);
                }
	        ifile++;
            }

        }

	if (ifile == nim)
	    break;
    }
    fclose(fp); 
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "done readImlist\n\n");
        fflush (fp_debug);
    }
    
    return (nfile);
}
