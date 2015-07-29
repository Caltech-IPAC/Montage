#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

extern FILE *fp_debug;

int checkFileExist (char *fname, char *rootname, char *suffix, 
    char *directory, char *filePath)
{
    struct stat buf;

    char   *cptr, str[1024], fname_in[1024];
    int    fileExist, istatus;
    
    int    debugfile = 0;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "checkFileExist: fname= [%s]\n", fname);
        fprintf (fp_debug, "directory= [%s]\n", directory);
        fflush (fp_debug);
    
        fprintf (fp_debug, "rootname= [%s]\n", rootname);
        fflush (fp_debug);
    
        fprintf (fp_debug, "suffix= [%s]\n", suffix);
        fflush (fp_debug);
    }

    strcpy (fname_in, fname);

    if(fname[0] != '/') {
    
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here1\n");
            fflush (fp_debug);
        }
        
	strcpy (filePath, directory);

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "got here2\n");
            fflush (fp_debug);
        }
        
        if (filePath[strlen(directory)-1] != '/')
	    strcat(filePath, "/");
      
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "got here3\n");
            fprintf (fp_debug, "fname= [%s]\n", fname);
            fflush (fp_debug);
        }
        
        strcat(filePath, fname);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here1: fname= [%s]\n", fname);
            fprintf (fp_debug, "filePath= [%s]\n", filePath);
            fflush (fp_debug);
        }
    }
    else {
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
            fprintf (fp_debug, "here2\n");
            fflush (fp_debug);
        }
        
	cptr = (char *)NULL;
        strcpy(filePath, fname);

	strcpy (str, fname);
	cptr = strrchr (str, '/');
	if (cptr != (char *)NULL) {
	    strcpy (fname, cptr+1); 
	}
    }

    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "fname= [%s]\n", fname);
        fprintf (fp_debug, "filePath= [%s]\n", filePath);
        fflush (fp_debug);
    }

/*
    Extract rootname from image filename
*/
    strcpy (str, fname_in);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "str= [%s]\n", str);
        fflush (fp_debug);
    }

        
    cptr = (char *)NULL;
    cptr = strrchr (str, '.');
        
    if (cptr != (char *)NULL) {
        
	strcpy (suffix, cptr+1);
        
        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "surffix= [%s]\n", suffix);
	    fflush (fp_debug);
        }

	*cptr = '\0';
	strcpy (rootname, str);
	if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "rootname= [%s]\n", rootname);
	    fflush (fp_debug);
        }
    }

    fileExist = 0;
    istatus = stat (filePath, &buf);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "istatus= [%d]\n", istatus);
        fflush (fp_debug);
    }

    if (istatus >= 0) {
	fileExist = 1;
    }

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "fileExist= [%d]\n", fileExist);
        fflush (fp_debug);
    } 
    
    return (fileExist);
}

