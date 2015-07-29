/*
Theme: This routine creates the options text in the HTML to feed
    the '@incfile' paramter in the htmlgen.

Input: 

Output:
    option text file.

Date: October 12, 2012 (Mihseh Kong)
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>


extern FILE *fp_debug;

int writeOptionList (char *filepath, int ncnt, char **arr,
    int selectedIndex, char *errmsg)
{
    FILE   *fp; 
    
    int    l;
    int    debugfile = 0;

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "\nwriteOptionList: filepath= [%s]\n", filepath);
	fflush (fp_debug);
    }

    if ((fp = fopen (filepath, "w+")) == (FILE *)NULL) {
	sprintf (errmsg, "Failed to open [%s] for write", filepath);
        return(-1);
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "ncnt= [%d]\n", ncnt);
	fflush (fp_debug);
    }

    for (l=0; l<ncnt; l++) {
   
        if (l == selectedIndex) {
            fprintf (fp, 
	        "<option value=\"%s\" selected=\"selected\">%s</option>\n",
	        arr[l], arr[l]);
        }
	else {
            fprintf (fp, "<option value=\"%s\">%s</option>\n", 
	        arr[l], arr[l]);
	}
    }

    fflush (fp);
    fclose (fp);

    return (0);
}

