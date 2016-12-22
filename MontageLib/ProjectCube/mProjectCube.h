#ifndef MPROJECTCUBE_H
#define MPROJECTCUBE_H

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;


/*******************************************/
/* Define mProjectCube function prototypes */
/*******************************************/

void    mProjectCube_UpdateBounds        (double oxpix, double oypix,
                                          double *oxpixMin, double *oxpixMax,
                                          double *oypixMin, double *oypixMax);
 
int     mProjectCube_BorderSetup         (char *strin);
int     mProjectCube_BorderRange         (int jrow, int maxpix, int *imin, int *imax);

int     mProjectCube_parseLine           (char *linein);
int     mProjectCube_stradd              (char *header, char *card);
int     mProjectCube_readTemplate        (char *filename);
int     mProjectCube_readFits            (char *filename, char *weightfile);
void    mProjectCube_fixxy               (double *x, double *y, int *offscl);

double  mProjectCube_computeOverlap      (double *ilon, double *ilat,
                                          double *olon, double *olat,
                                          int energyMode, double refArea, double *areaRatio);

int     mProjectCube_DirectionCalculator (Vec *a, Vec *b, Vec *c);
int     mProjectCube_SegSegIntersect     (Vec *a, Vec *b, Vec *c, Vec *d, 
                                          Vec *e, Vec *f, Vec *p, Vec *q);
int     mProjectCube_Between             (Vec *a, Vec *b, Vec *c);
int     mProjectCube_Cross               (Vec *a, Vec *b, Vec *c);
double  mProjectCube_Dot                 (Vec *a, Vec *b);
double  mProjectCube_Normalize           (Vec *a);
void    mProjectCube_Reverse             (Vec *a);
void    mProjectCube_SaveVertex          (Vec *a);
void    mProjectCube_SaveSharedSeg       (Vec *p, Vec *q);
void    mProjectCube_PrintPolygon        ();

void    mProjectCube_ComputeIntersection (Vec *P, Vec *Q);

int     mProjectCube_UpdateInteriorFlag  (Vec *p, int interiorFlag, 
                                          int pEndpointFromQdir,
                                          int qEndpointFromPdir);

int     mProjectCube_Advance             (int i, int *i_advances, 
                                          int n, int inside, Vec *v);

double  mProjectCube_Girard();
void    mProjectCube_RemoveDups();

int     mProjectCube_printDir            (char *point, char *vector, int dir);

void    mProjectCube_printFitsError      (int);
void    mProjectCube_printError          (char *);

#endif
