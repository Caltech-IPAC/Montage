/*
Theme:  This module make an image cutout from an original FITS image.

Written: January 26, 2011 (Mihseh Kong)
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <svc.h>

extern FILE *fdebug;

int subsetImage (char *impath, int ns, int nl, int xflip, int yflip, int nowcs,
    char *subsetPath, double sx, double sy, double ns_subset, double nl_subset, 
    char *errmsg)
{
    char   status[40], cmd[1024];

    int    istatus;
    
    double ss, sl;

    int   debugfile = 0;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "\nFrom subsetImage: impath= [%s]\n", impath);
	fprintf (fdebug, "subsetPath= [%s]\n", subsetPath);
	fprintf (fdebug, 
	    "sx= [%lf] sy= [%lf] ns_subset= [%lf] nl_subset= [%lf]\n", 
	    sx, sy, ns_subset, nl_subset);
	fprintf (fdebug, "xflip= [%d] yflip= [%d]\n", xflip, yflip);
	fprintf (fdebug, "nowcs= [%d]\n", nowcs);
	fflush (fdebug);
    }

    if (xflip) {
        ss = (double)ns - (sx + ns_subset);
    }
    else {
        ss = sx;
    }
    
    if (yflip) {
        sl = (double)nl - (sy + nl_subset);
    }
    else {
        sl = sy;
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, 
	    "ss= [%lf] sl= [%lf]\n", ss, sl);
	fflush (fdebug);
    }

    

/*
    use mSubimage to make cutouts
*/
    if (nowcs) {
        sprintf (cmd, "mSubimage -nowcs -p %s %s %15.8f %15.8f %5.2f %5.2f", 
	    impath, subsetPath, ss, sl, ns_subset, nl_subset);
    }
    else {
        sprintf (cmd, "mSubimage -p %s %s %15.8f %15.8f %5.2f %5.2f", 
	    impath, subsetPath, ss, sl, ns_subset, nl_subset);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "cmd= [%s]\n", cmd);
	fflush (fdebug);
    }
	
    istatus = svc_run(cmd);
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "istatus= [%d]\n", istatus);
	fflush (fdebug);
    }
	
	
    strcpy (status, svc_value("stat"));
        
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "status= [%s]\n", status);
	fflush (fdebug);
    }
	
    if (strcmp(status, "ERROR") == 0) {
        
	strcpy (errmsg, svc_value("msg"));
        return (-1);
    }

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "subimage [%s] successfully created\n", subsetPath);
        fflush(fdebug);
    }

    return (0);
}
