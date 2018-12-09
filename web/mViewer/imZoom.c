/*
Theme:  Subroutines to handle image zoom:

    zoombox (input box from main image),
    movebox (input box from refimage),
    zoomin (button),
    zoomout (button)
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>

#include <config.h>
#include <www.h>
#include <cmd.h>


#include "mviewer.h"

int fileCopy (char *fromfile, char *tofile, char *errmsg);
int checkFileExist (char *filename, char *rootname, char *suffix, 
    char *directory, char *filepath); 

int subsetImage (char *imPath, int ns_orig, int nl_orig, int xflip, int yflip,
    int nowcs, char *subsetimpath, double ss, double sl, double ns, double nl, 
    char *errmsg);


extern FILE *fdebug;

    
int  imZoom (struct Mviewer *param)
{
    char     subsetimpath[1024];
    char     subsetredpath[1024];
    char     subsetgrnpath[1024];
    char     subsetbluepath[1024];
    
    char     graypath[1024];
    char     redpath[1024];
    char     grnpath[1024];
    char     bluepath[1024];
    
    int      istatus;
    
    int      ns_subset, nl_subset;
    int      diffx, diffy;

    int      xdim, ydim;


    double   xmin, ymin, xmax, ymax;

    double   dx, xc, dy, yc;
    double   factor, reffactor, new_factor;


    struct timeval   tp;
    struct timezone  tzp;
    double           exacttime, exacttime0;

    int      debugfile = 0;
    int      debugtime = 0;
   
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	
	fprintf (fdebug, "\nFrom imZoom: cmd= [%s]\n", param->cmd); 
	fprintf (fdebug, "xmin= [%lf] xmax= [%lf]\n", 
	    param->xmin, param->xmax);
        fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", 
	    param->ymin, param->ymax);
        fflush (fdebug);

	fprintf (fdebug, "imageWidth= [%d] imageHeight= [%d]\n", 
	    param->imageWidth, param->imageHeight);
	fprintf (fdebug, "nowcs= [%d]\n", param->nowcs);
	fflush (fdebug);
    }

    
    factor = param->zoomfactor;
    reffactor = param->refzoomfactor;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "factor= [%lf] reffactor= [%lf]\n", 
	    factor, reffactor);
        fprintf (fdebug, "ss= [%.1f] sl= [%.1f]\n", param->ss, param->sl);
        fprintf (fdebug, "xflip= [%d] yflip= [%d]\n", 
	    param->xflip, param->yflip);
        fflush (fdebug);
    }


/*
    "movebox" input are from refimg pixel coord.
*/
    if (strcasecmp (param->cmd, "movebox") == 0) {
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "case movebox\n");
            fflush (fdebug);
        }
	    
	xmin = param->xmin/reffactor; 
	xmax = param->xmax/reffactor; 
	ymin = param->ymin/reffactor; 
	ymax = param->ymax/reffactor; 
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "xmin= [%lf] xmax= [%lf]\n", xmin, xmax); 
            fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", ymin, ymax); 
	    fflush (fdebug);
        }

	param->xmin = xmin;
	param->xmax = xmax;
	param->ymin = ymin;
	param->ymax = ymax;
    }
    else if (strcasecmp (param->cmd, "zoombox") == 0) {

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "case zoombox: factor= [%lf]\n", factor);
            fprintf (fdebug, "image:xmin= [%lf] ymin= [%lf]\n", 
	        param->xmin, param->ymin);
            fprintf (fdebug, "xmax= [%lf] ymax= [%lf]\n", 
	        param->xmax, param->ymax);
            fprintf (fdebug, "param->ss= [%lf] sl= [%lf]\n", 
	        param->ss, param->sl);
            fprintf (fdebug, "param->xs= [%lf] xe= [%lf]\n", 
	        param->xs, param->xe);
            fprintf (fdebug, "param->ys= [%lf] ye= [%lf]\n", 
	        param->ys, param->ye);
            fflush (fdebug);
        }

	xmin = param->xs/factor + param->ss; 
	xmax = param->xe/factor + param->ss; 
	ymin = param->ys/factor + param->sl; 
	ymax = param->ye/factor + param->sl; 
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "screen: xmin= [%lf] ymin= [%lf]\n", xmin, ymin);
            fprintf (fdebug, "xmax= [%lf] ymax= [%lf]\n", xmax, ymax);
            fflush (fdebug);
        }

/*
    Adjust xmax/ymax to make it a square
*/
        xdim = xmax - xmin;
	ydim = ymax - ymin;

        if (xdim > ydim) {
	    ydim = xdim;
	}
	else if (ydim > xdim) {
	    xdim = ydim;
	}
	
        xmax = xmin + xdim;
        ymax = ymin + ydim;

        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "adjusted: xmin= [%lf] ymin= [%lf]\n", xmin, ymin);
            fprintf (fdebug, "xmax= [%lf] ymax= [%lf]\n", xmax, ymax);
            fflush (fdebug);
        }

        if ((fabs(xmax-xmin) < 5) && (fabs(ymax-ymin) < 5)) {
   
            strcpy (param->errmsg, 
	        "Zoom box is less than 5 real image pixels in either directions"
	        " -- too small an area for zoom operation."); 
	    return (-1);
        }

	param->xmin = xmin;
	param->xmax = xmax;
	param->ymin = ymin;
	param->ymax = ymax;

    }
    else if (strcasecmp (param->cmd, "zoomin") == 0) {
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "cmd= [%s]\n", param->cmd); 
            fprintf (fdebug, "ss= [%.1f] sl= [%.1f]\n", 
	        param->ss, param->sl); 
            fprintf (fdebug, "cutoutWidth= [%d] cutoutHeight= [%d]\n", 
	        param->cutoutWidth, param->cutoutHeight); 
            fprintf (fdebug, "imageWidth= [%d] imageHeight= [%d]\n", 
	        param->imageWidth, param->imageHeight); 
            fprintf (fdebug, "canvasWith= [%d] canvasHtigh= [%d]\n", 
	        param->canvasWidth, param->canvasWidth); 
            fflush (fdebug);
        }

        if (param->cutoutWidth > 0)
	    param->ns = param->cutoutWidth;
	else
	    param->ns = param->imageWidth;

        if (param->cutoutHeight > 0)
	    param->nl = param->cutoutHeight;
	else
	    param->nl = param->imageHeight;

	xmin = param->ss;
	ymin = param->sl;
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "ns= [%d] nl= [%d]\n", param->ns, param->nl);
            fprintf (fdebug, "xmin= [%lf] ymin= [%lf]\n", xmin, ymin);
            fflush (fdebug);
        }

	if (strcasecmp (param->cmd, "zoomin") == 0) {
	    new_factor = factor * sqrt(2.);
	}
	else if (strcasecmp (param->cmd, "zoomout") == 0) {
	    new_factor = factor / sqrt(2.);
        }
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "new factor: factor= [%lf] reffactor= [%lf]\n",
	        new_factor, reffactor);
            fflush (fdebug);
        }

        xdim = param->ns;
	ydim = param->nl;

        if (xdim > ydim) {
	    ydim = xdim;
	}
	else if (ydim > xdim) {
	    xdim = ydim;
	}

	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "xdim= [%d] ydim= [%d]\n", xdim, ydim);
            fprintf (fdebug, "xdim*new_factor= [%d] ydim*new_factor= [%d]\n", 
	        (int)(xdim*new_factor), (int)(ydim*new_factor));
            fflush (fdebug);
            fflush (fdebug);
        }

/*
    If the 'dimension*new_factor' still fits the canvas, don't change the xmin, xmax
*/
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "canvasWidth= [%d] xdim*new_factor= [%d]\n",
		param->canvasWidth, (int)(xdim*new_factor));
            fflush (fdebug);
        }
 
        if ((int)(param->ns*new_factor) > param->canvasWidth) {

/*
    dx for computing the center of cutout image
*/
	    dx = (double)param->ns/ 2.0;
	    xc = xmin + dx;

/*
    adjust dy for cutout size
*/
	    dx = (double)xdim / 2.0;

	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "current dx= [%lf] xc= [%lf]\n", dx, xc);
                fflush (fdebug);
            }
            
	    dx = dx*factor/new_factor;
	    xmin = xc - dx;
	    xmax = xc + dx;
	    
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "new dx= [%lf]\n", dx);
                fprintf (fdebug, "new xmin= [%lf] xmax= [%lf]\n", xmin, xmax);
                fflush (fdebug);
            }
	
	    if (xmin < 0.)
	        xmin = 0.;

	    if (xmax > (double)param->imageWidth-1)
	        xmax = (double)param->imageWidth-1;
	
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "adjusted by the imageWidth xmin= [%lf] xmax= [%lf]\n",
		    xmin, xmax);
                fflush (fdebug);
            }
        }
	else {
	    xmax = (double)param->ns-1;
	}

        if ((int)(param->nl*new_factor) > param->canvasHeight) {

/*
    dy for computing the center of cutout image
*/
	    dy = (double)param->nl/ 2.0;
	    yc = ymin + dy;

/*
    adjust dy for cutout size
*/
	    dy = (double)ydim / 2.0;

	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "current dy= [%lf] yc= [%lf]\n", dy, yc);
                fflush (fdebug);
            }
	
            dy = dy*factor/new_factor;
	    ymin = yc - dy;
	    ymax = yc + dy;

	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "new dy= [%lf] yc= [%lf]\n", dy, yc);
                fflush (fdebug);
                fprintf (fdebug, "new ymin= [%lf] ymax= [%lf]\n", ymin, ymax);
                fflush (fdebug);
            }

	    if (ymin < 0.)
	        ymin = 0.;

	    if (ymax > (double)param->imageHeight-1)
	        ymax = (double)param->imageHeight-1;
	    
	    if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "adjusted by the imageHeight ymin= [%lf] ymax= [%lf]\n",
		    ymin, ymax);
                fflush (fdebug);
            }
        }
	else {
	    ymax = (double)param->nl-1;
	}

        param->xmin = xmin;
        param->xmax = xmax;
        param->ymin = ymin;
        param->ymax = ymax;

    }
    else if (strcasecmp (param->cmd, "zoomout") == 0) {
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "cmd= [%s]\n", param->cmd); 
            fprintf (fdebug, "ss= [%.1f] sl= [%.1f]\n", 
	        param->ss, param->sl); 
            fprintf (fdebug, "ns= [%d] nl= [%d]\n", 
	        param->cutoutWidth, param->cutoutWidth); 
            fflush (fdebug);
        }

        if (param->cutoutWidth > 0)
	    param->ns = param->cutoutWidth;
	else
	    param->ns = param->imageWidth;

        if (param->cutoutHeight > 0)
	    param->nl = param->cutoutHeight;
	else
	    param->nl = param->imageHeight;

	xmin = param->ss;
	ymin = param->sl;
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "ns= [%d] nl= [%d]\n", param->ns, param->nl);
            fprintf (fdebug, "xmin= [%lf] ymin= [%lf]\n", xmin, ymin);
            fflush (fdebug);
        }

	if (strcasecmp (param->cmd, "zoomin") == 0) {
	    new_factor = factor * sqrt(2.);
	}
	else if (strcasecmp (param->cmd, "zoomout") == 0) {
	    new_factor = factor / sqrt(2.);
        }
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "new factor: factor= [%lf] reffactor= [%lf]\n",
	        new_factor, reffactor);
            fflush (fdebug);
        }

        xdim = param->ns;
	ydim = param->nl;

        if (xdim > ydim) {

	    ydim = xdim;
	}
	else if (ydim > xdim) {
	    xdim = ydim;
	}
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "xdim= [%d] ydim= [%d]\n", xdim, ydim);
            fflush (fdebug);
        }
        
	dx = (double)xdim / 2.0;
        xc = xmin + dx;
	
	dy = (double)ydim / 2.0;
        yc = ymin + dy;
	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "dx= [%lf] xc= [%lf]\n", dx, xc);
            fprintf (fdebug, "dy= [%lf] yc= [%lf]\n", dy, yc);
            fflush (fdebug);
        }
	
        dx = dx*factor/new_factor;
        dy = dy*factor/new_factor;

	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "new xc and dx: dx= [%lf] xc= [%lf]\n", dx, xc);
            fprintf (fdebug, "dy= [%lf] yc= [%lf]\n", dy, yc);
            fflush (fdebug);
        }
	
	xmin = xc - dx;
	xmax = xc + dx;
	ymin = yc - dy;
	ymax = yc + dy;

	if (xmin < 0.)
	    xmin = 0.;

	if (xmax > (double)param->imageWidth-1)
	    xmax = (double)param->imageWidth-1;

	if (ymin < 0.)
	    ymin = 0.;

	if (ymax > (double)param->imageHeight-1)
	    ymax = (double)param->imageHeight-1;


        param->xmin = xmin;
        param->xmax = xmax;
        param->ymin = ymin;
        param->ymax = ymax;

    }
    else if (strcasecmp (param->cmd, "resetzoom") == 0) { 
        
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "cmd= [%s]\n", param->cmd); 
            fprintf (fdebug, "ss= [%.1f] sl= [%.1f]\n", 
	        param->ss, param->sl); 
            fprintf (fdebug, "ns= [%d] nl= [%d]\n", param->ns, param->nl); 
            fflush (fdebug);
        }

        param->ns = param->imageWidth;
        param->nl = param->imageHeight;

	xmin = 0.;
	xmax = (double)param->ns-1;

        ymin = 0.;
	ymax = (double)param->nl-1;
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "resetzoom: xmin= %lf xmax= %lf\n", xmin, xmax);
	    fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", ymin, ymax);
	    fflush (fdebug);
        }
    }
    else if (strcasecmp (param->cmd, "replaceimplane") == 0) { 

        if (param->cutoutWidth > 0)
	    param->ns = param->cutoutWidth;
	else
	    param->ns = param->imageWidth;

        if (param->cutoutHeight > 0)
	    param->nl = param->cutoutHeight;
	else
	    param->nl = param->imageHeight;

	
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
            fprintf (fdebug, "factor= [%lf] reffactor= [%lf]\n",
	        factor, reffactor);
            fflush (fdebug);
        }

        xmin = param->ss;
	xmax = xmin + param->ns-1;
	
	ymin = param->sl;
	ymax = ymin + param->nl-1;

	if (xmin < 0.)
	    xmin = 0.;

	if (xmax > (double)param->imageWidth-1)
	    xmax = (double)param->imageWidth-1;

	if (ymin < 0.)
	    ymin = 0.;

	if (ymax > (double)param->imageHeight-1)
	    ymax = (double)param->imageHeight-1;


        param->xmin = xmin;
        param->xmax = xmax;
        param->ymin = ymin;
        param->ymax = ymax;
    } 

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "xmin= [%lf] xmax= [%lf]\n", xmin, xmax); 
        fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", ymin, ymax); 
	fflush (fdebug);
    }
   

/*
    Adjust xmin, xmax, ymin, ymax if they exceed image dimensions
*/
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "\n Adjust xmin, xmax, ymin, ymax: \n");
	fprintf (fdebug, "xmin= [%lf] xmax= [%lf]\n", xmin, xmax);
	fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", ymin, ymax);
	fprintf (fdebug, "xflip= [%d] yflip= [%d]\n", 
	    param->xflip, param->yflip);
	fflush (fdebug);
    }

    if ((int)xmin < 0) {
        dx = -xmin;
        xmin = 0.;
	xmax += dx;
    }

    if ((int)xmax > param->imageWidth) {
        
        dx = xmax - param->imageWidth;
	xmax = (double)param->imageWidth-1;

	xmin -= dx;
	if (xmin < 0)
	    xmin = 0.;
    }

	
    if ((int)ymin < 0) {
	dy = -ymin;
        ymin = 0.;
	ymax += dy;
    }
    
    if ((int)ymax > param->imageHeight) {
        
        dy = ymax - param->imageHeight;
	ymax = (double)param->imageHeight-1;
    
        if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "here1: ymin= [%lf] ymax= [%lf]\n", ymin, ymax);
	    fprintf (fdebug, "dy= [%lf]\n", dy);
	    fflush (fdebug);
        }

	ymin -= dy;
        
	if ((debugfile) && (fdebug != (FILE *)NULL)) {
	    fprintf (fdebug, "ymin= [%lf]\n", ymin);
	    fflush (fdebug);
        }
	if (ymin < 0)
	    ymin = 0.;
    }
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "\nadjusted: xmin= [%lf] xmax= [%lf]\n", 
	    xmin, xmax); 
        fprintf (fdebug, "ymin= [%lf] ymax= [%lf]\n", ymin, ymax); 
	fflush (fdebug);
    }
   
    ns_subset = (int)(xmax - xmin);
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "\nns_subset= [%d]\n", ns_subset); 
        fprintf (fdebug, "imageWidth= [%d]\n", param->imageWidth); 
	fflush (fdebug);
    }
   
    diffx = (int)xmin + ns_subset - param->imageWidth;
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "diffx= [%d]\n", diffx); 
	fflush (fdebug);
    }
   
    if (diffx > 0) 
        ns_subset -= diffx;
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "adjusted ns_subset= [%d]\n", ns_subset); 
	fflush (fdebug);
    }

    nl_subset = (int)(ymax - ymin);
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "\nnl_subset= [%d]\n", nl_subset); 
        fprintf (fdebug, "imageHeight= [%d]\n", param->imageHeight); 
	fflush (fdebug);
    }
   
    diffy = (int)ymin + nl_subset - param->imageHeight;
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "diffy= [%d]\n", diffy); 
	fflush (fdebug);
    }
   
    if (diffy > 0) 
        nl_subset -= diffy;

    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "adjusted nl_subset= [%d]\n", nl_subset); 
	fflush (fdebug);
    }


    if ((debugfile) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "ns_subset= [%d] nl_subset= ]%d]\n", 
            ns_subset, nl_subset);    
	fflush (fdebug);
    }
	
    if ((strcasecmp (param->cmd, "zoomin") == 0) &&
        ((ns_subset < 5) || (nl_subset < 5))) {
   
        strcpy (param->errmsg, "New zoom area is less than 5x5 pixels "
	    "-- too small for zoom in operation."); 
	return (-1);
    }



/*
    make subset image 
*/
    if (strcasecmp (param->cmd, "resetzoom") != 0) { 
        
        if (param->iscolor) { 

/*
            if ((int)strlen(param->imdatadir) > 0) {

                sprintf (redpath, "%s/%s", param->imdatadir, param->redFile);
                sprintf (grnpath, "%s/%s", param->imdatadir, param->greenFile);
                sprintf (bluepath, "%s/%s", param->imdatadir, param->blueFile);
	    }
	    else {
                sprintf (redpath, "%s/%s", param->directory, param->redFile);
                sprintf (grnpath, "%s/%s", param->directory, param->greenFile);
                sprintf (bluepath, "%s/%s", param->directory, param->blueFile);
            }
*/
            
	    strcpy (redpath, param->redPath);
	    strcpy (grnpath, param->greenPath);
	    strcpy (bluepath, param->bluePath);

	    if ((int)strlen(param->subsetredfile) == 0) {
		sprintf (param->subsetredfile, "%s_cutout_%s", 
	            param->imageFile, param->redFile);
	    }
	    sprintf (subsetredpath, "%s/%s", 
	        param->directory, param->subsetredfile);
	    
	    if ((int)strlen(param->subsetgrnfile) == 0) {
		sprintf (param->subsetgrnfile, "%s_cutout_%s", 
	            param->imageFile, param->greenFile);
	    }
            sprintf (subsetgrnpath, "%s/%s", 
	        param->directory, param->subsetgrnfile);
	    
	    if ((int)strlen(param->subsetbluefile) == 0) {
		sprintf (param->subsetbluefile, "%s_cutout_%s", 
	            param->imageFile, param->blueFile);
	    }
            sprintf (subsetbluepath, "%s/%s", 
	        param->directory, param->subsetbluefile);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                
		fprintf (fdebug, "redpath= [%s]\n", redpath);
		fprintf (fdebug, "grnpath= [%s]\n", grnpath);
		fprintf (fdebug, "bluepath= [%s]\n", bluepath);
	        
		fprintf (fdebug, "subsetredpath= [%s]\n", subsetredpath);
		fprintf (fdebug, "subsetgrnpath= [%s]\n", subsetgrnpath);
		fprintf (fdebug, "subsetbluepath= [%s]\n", subsetbluepath);
		fflush (fdebug);
            }


            istatus = subsetImage (redpath, param->imageWidth, 
	        param->imageHeight, param->xflip,
	        param->yflip, param->nowcs, subsetredpath, xmin, ymin, 
		(double)ns_subset, (double)nl_subset, param->errmsg); 	

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned subsetImage (red): istatus= [%d]\n", istatus);
	        fflush (fdebug);
            }
	   
            if (istatus == -1) {
	        return (-1);
            }
        
	    istatus = subsetImage (grnpath, param->imageWidth, 
	        param->imageHeight, param->xflip,
	        param->yflip, param->nowcs, subsetgrnpath, xmin, ymin, 
		(double)ns_subset, (double)nl_subset, param->errmsg); 	

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned subsetImage (grn): istatus= [%d]\n", istatus);
	        fflush (fdebug);
            }
	   
            if (istatus == -1) {
	        return (-1);
            }
        
	    istatus = subsetImage (bluepath, param->imageWidth, 
	        param->imageHeight, param->xflip,
	        param->yflip, param->nowcs, subsetbluepath, xmin, ymin, 
		(double)ns_subset, (double)nl_subset, param->errmsg); 	

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, 
		    "returned subsetImage (blue): istatus= [%d]\n", istatus);
	        fflush (fdebug);
            }
	   
            if (istatus == -1) {
	        return (-1);
            }
        }
        else {
/*
            if ((int)strlen(param->imdatadir) > 0) {
	        sprintf (graypath, "%s/%s", param->imdatadir, param->grayFile);
	    }
	    else {
	        sprintf (graypath, "%s/%s", param->directory, param->grayFile);
            }
*/
	        
	    strcpy (graypath, param->grayPath);

	    if ((int)strlen(param->subsetimfile) == 0) {
		
		sprintf (param->subsetimfile, "%s_cutout.fits", 
	            param->imageFile);
	    }

	    sprintf (subsetimpath, "%s/%s", 
	        param->directory, param->subsetimfile);

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
            
	        if (debugtime) {
                    gettimeofday (&tp, &tzp);
                    exacttime0 = (double)tp.tv_sec 
		        + (double)tp.tv_usec/1000000.;
                }

                fprintf (fdebug, "call subsetImage\n");
                fprintf (fdebug, "graypath= [%s]\n", graypath);
                fprintf (fdebug, "subsetimpath= [%s]\n", subsetimpath);
	        fflush (fdebug);
            }
            
            istatus = subsetImage (graypath, param->imageWidth, 
	        param->imageHeight, param->xflip,
	        param->yflip, param->nowcs, subsetimpath, xmin, ymin, 
		(double)ns_subset, (double)nl_subset, param->errmsg); 	

            if ((debugfile) && (fdebug != (FILE *)NULL)) {
                fprintf (fdebug, "returned subsetImage: istatus= [%d]\n", 
	            istatus);
	        fflush (fdebug);
            }
	   
            if (istatus == -1) {
	        return (-1);
            }
    
            if ((debugfile) && (fdebug != (FILE *)NULL)) {
            
	        if (debugtime) {
                    gettimeofday (&tp, &tzp);
                    exacttime = (double)tp.tv_sec + (double)tp.tv_usec/1000000.;
                    fprintf (fdebug, "time (subsetImage): %.6f sec\n", 
	            (exacttime-exacttime0));
                }
            }
        }
    }

    param->zoomfactor = factor;
    
    if ((debugfile) && (fdebug != (FILE *)NULL)) {
	fprintf (fdebug, "zoomfactor= %lf\n", param->zoomfactor);
        fflush (fdebug);
    }
    
    param->ss = xmin;
    param->sl = ymin;
    
    return (0);
}

