/*
Theme: This routine use copyfile svc programt o copy a file.

Synopsis: 

Date: May 11, 2009 (Mihseh Kong)
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <mtbl.h>
#include <svc.h>

extern FILE *fp_debug;

int fileCopy (char *fromfile, char *tofile, char *errmsg)
{
    char  cmd[1024], status[20];
    
    int   istatus;

    int   debugfile = 0;
    int   svcdebug = 0;


    sprintf (cmd, "copyfile %s %s", fromfile, tofile);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "cmd= [%s]\n", cmd);
	fflush (fp_debug);
    }
  
    if (svcdebug) {
	svc_debug (fp_debug);
    }

    istatus = svc_run (cmd);

    if (svc_value("stat") == (char *)NULL) {

	sprintf (errmsg, "Failed to copyfile: cmd= [%s]", cmd);
	return (-1);
    }
    else {
	strcpy (status, svc_value("stat"));

        if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	    fprintf (fp_debug, "status= [%s]\n", status);
	    fflush (fp_debug);
	}
    }

    if (strcasecmp (status, "error") == 0) {

	sprintf (errmsg, "Failed to copyfile: cmd= [%s]", cmd);
	return (-1);
    }

    return (0);
}


