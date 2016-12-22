#ifndef MPROJECT_H
#define MPROJECT_H

/*****************************************/
/* Define mProjectPP function prototypes */
/*****************************************/

int    mProjectPP_readFits      (char *filename, char *weightfile);
int    mProjectPP_parseLine     (char *linein, int headerType);
int    mProjectPP_stradd        (char *header, char *card);
int    mProjectPP_readTemplate  (char *filename, int headerType);
int    mProjectPP_BorderSetup   (char *strin);
int    mProjectPP_BorderRange   (int jrow, int maxpix, 
                                int *imin, int *imax);

void   mProjectPP_UpdateBounds  (double oxpix, double oypix,
                                 double *oxpixMin, double *oxpixMax,
                                 double *oypixMin, double *oypixMax);

void   mProjectPP_printFitsError(int);
void   mProjectPP_printError    (char *);

double mProjectPP_computeOverlapPP(double *ix, double *iy,
                                   double minX, double maxX, 
                                   double minY, double maxY,
                                   double pixelArea);

double mProjectPP_polyArea      (int npts, double *nx, double *ny);

int    mProjectPP_rectClip      (int n, double *x, double *y, double *nx, double *ny,
                                 double minX, double minY, double maxX, double maxY);

int    mProjectPP_lineClip      (int n, 
                                 double  *x, double  *y, 
                                 double *nx, double *ny,
                                 double val, int dir);

int    mProjectPP_inPlane       (double test, double divider, int direction);
int    mProjectPP_ptInPoly      (double x, double y, int n, double *xp, double *yp);

#endif
