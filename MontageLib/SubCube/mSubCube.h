#ifndef MSUBCUBE_H
#define MSUBCUBE_H

#include <fitsio.h>

#define SKY    0
#define PIX    1
#define HDU    2
#define SHRINK 3

struct mSubCubeParams
{
        int    ibegin;                /* column offset */
        int    iend;                  /* last column */
        int    jbegin;                /* row offset */
        int    jend;                  /* last row */
        int    kbegin;                /* first 'plane' (third dimensional axis) */
        int    kend;                  /* last 'plane' */
        int    lbegin;                /* first 'attribute' (fourth dimensional axis) */
        int    lend;                  /* last 'attribute' */
        char   dConstraint[2][1024];  /* constrains for third dimension */
        int    nrange[2];             /* constraint range info */
        int    range[2][1024][2];
        long   nelements;             /* row length */
        int    nfound;
        int    isDSS;
        double crpix  [10];
        double cnpix  [10];
        long   naxis;
        long   naxes  [10];
        long   naxesin[10];
};


/****************************************/
/* Define mSubCube function prototypes */
/****************************************/

void              mSubCube_fixxy          (double *x, double *y, int *offscl);
struct WorldCoor *mSubCube_getFileInfo    (fitsfile *infptr, char **header, struct mSubCubeParams *params);
int               mSubCube_copyHeaderInfo (fitsfile *infptr, fitsfile *outfptr, struct mSubCubeParams *params);
int               mSubCube_copyData       (fitsfile *infptr, fitsfile *outfptr, struct mSubCubeParams *params);
int               mSubCube_parseSelectList(int ind, struct mSubCubeParams *params);
int               mSubCube_dataRange      (fitsfile *infptr, int *imin, int *imax, int *jmin, int *jmax);
void              mSubCube_printFitsError (int);

#endif
