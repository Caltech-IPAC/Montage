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


char *strtrim(char *);

extern FILE *fp_debug;

int readFilelist (char *filelist, char ***filename, char *errmsg)
{
    FILE  *fp;

    char  *cptr, **fname, line[1024];
    
    int   nfile, istatus, l, len, ifile;
    
    int   debugfile = 0;
    
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nFrom readFilelist: filelist= [%s]\n", filelist);
	fflush (fp_debug);
    }

/*
    read filelist
*/
    fp = (FILE *)NULL;
    fp = fopen (filelist, "r");

    if (fp == (FILE *)NULL) {
	sprintf (errmsg, "Failed to open [%s]\n", filelist);
	return (-1);
    }
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here1\n");
	    fflush (fp_debug);
        }


/*
    count number of files
*/
    nfile = 0;
    while (1) 
    {
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here2\n");
	    fflush (fp_debug);
        }

        cptr = (char *)NULL;
        cptr = fgets (line, 1024, fp);
        
	if (cptr == (char *)NULL)
	    break;
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here3: line= [%s]\n", line);
	    fflush (fp_debug);
        }


        len = (int)strlen (line);
	
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "len= [%d]\n", len);
	    fflush (fp_debug);
        }

	
	if ((line[len-1] == '\r') || (line[len-1] == '\n'))
	    line[len-1] = '\0';
	if ((line[len-2] == '\r') || (line[len-2] == '\n'))
	    line[len-2] = '\0';

        len = (int)strlen (line);
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "len= [%d]\n", len);
	    fflush (fp_debug);
        }

	nfile++;
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "\nnfile= [%d]\n", nfile);
	fflush (fp_debug);
    }

    if (nfile == 0) {
	sprintf (errmsg, "filelist [%s] contains [%d] file.", 
	    filelist, nfile);
	return (nfile);
    }


    istatus = fseek (fp, 0L, SEEK_SET);

    fname = (char **)NULL;
    fname = (char **)malloc (nfile*sizeof(char *));
    for (l=0; l<nfile; l++) {
        fname[l] = (char *)NULL;
        fname[l] = (char *)malloc (1024*sizeof(char));
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "here1\n");
	fflush (fp_debug);
    }


    ifile = 0;
    while (1) 
    {
        cptr = (char *)NULL;
        cptr = fgets (line, 1024, fp);
        
	if (cptr == (char *)NULL)
	    break;

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "ifile= [%d] line= [%s]\n", ifile, line);
	    fflush (fp_debug);
        }

        len = (int)strlen(line);
        if ((line[len-1] == '\r') || (line[len-1] == '\n')) 
            line[len-1] = '\0';
        if ((line[len-2] == '\r') || (line[len-2] == '\n')) 
            line[len-2] = '\0';

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here2: line= [%s]\n", line);
	    fflush (fp_debug);
        }
	    
	strcpy (fname[ifile], strtrim(line));
        
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here4: fname[%d]= [%s]\n", ifile, fname[ifile]);
	    fflush (fp_debug);
        }

	ifile++;
	
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here5: ifile= [%d] nfile= [%d]\n", 
	        ifile, nfile);
	    fflush (fp_debug);
        }


	if (ifile == nfile)
	    break;
    }
    fclose(fp); 

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here6\n");
	fflush (fp_debug);
    }
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	for (l=0; l<nfile; l++) {
	    fprintf (fp_debug, "fname[%d]= [%s]\n", l, fname[l]);
	    fflush (fp_debug);
	}
    }

    *filename = fname;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "here7: returning\n");
	fflush (fp_debug);
    }
    
    return (nfile);
}
