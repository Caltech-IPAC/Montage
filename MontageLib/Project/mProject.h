#ifndef MPROJECT_H
#define MPROJECT_H

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;


/***************************************/
/* Define mProject function prototypes */
/***************************************/

void    mProject_UpdateBounds        (double oxpix, double oypix,
                                      double *oxpixMin, double *oxpixMax,
                                      double *oypixMin, double *oypixMax);
 
int     mProject_BorderSetup         (char *strin);
int     mProject_BorderRange         (int jrow, int maxpix, int *imin, int *imax);

int     mProject_parseLine           (char *linein);
int     mProject_stradd              (char *header, char *card);
int     mProject_readTemplate        (char *filename);
void    mProject_cleanup             ();
int     mProject_readFits            (char *filename, char *weightfile);
void    mProject_fixHPX              (double *x, double *y, int *offscl);
void    mProject_fix360              (double *x, double *y, int *offscl);

double  mProject_computeOverlap      (double *ilon, double *ilat,
                                      double *olon, double *olat,
                                      int energyMode, double refArea, double *areaRatio);

int     mProject_DirectionCalculator (Vec *a, Vec *b, Vec *c);
int     mProject_SegSegIntersect     (Vec *a, Vec *b, Vec *c, Vec *d, 
                                      Vec *e, Vec *f, Vec *p, Vec *q);
int     mProject_Between             (Vec *a, Vec *b, Vec *c);
int     mProject_Cross               (Vec *a, Vec *b, Vec *c);
double  mProject_Dot                 (Vec *a, Vec *b);
double  mProject_Normalize           (Vec *a);
void    mProject_Reverse             (Vec *a);
void    mProject_SaveVertex          (Vec *a);
void    mProject_SaveSharedSeg       (Vec *p, Vec *q);
void    mProject_PrintPolygon        ();

void    mProject_ComputeIntersection (Vec *P, Vec *Q);

int     mProject_UpdateInteriorFlag  (Vec *p, int interiorFlag, 
                                      int pEndpointFromQdir,
                                      int qEndpointFromPdir);

int     mProject_Advance             (int i, int *i_advances, 
                                      int n, int inside, Vec *v);

double  mProject_Girard();
void    mProject_RemoveDups();

int     mProject_printDir            (char *point, char *vector, int dir);

void    mProject_printFitsError      (int);
void    mProject_printError          (char *);

#endif
