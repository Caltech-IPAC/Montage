#ifndef MMASK_H
#define MMASK_H

#include <fitsio.h>

struct mMaskParams
{
   int ibegin;          /* column offset */
   int iend;            /* last column */
   int jbegin;          /* row offset */
   int jend;            /* last row */
   long nelements;      /* row length */
   int nfound;
   long naxis;
   long naxes[10];
};


/****************************************/
/* Define mMask function prototypes */
/****************************************/

int  mMask_getFileInfo    (fitsfile *infptr, char **header, struct mMaskParams *params);
int  mMask_copyHeaderInfo (fitsfile *infptr, fitsfile *outfptr, struct mMaskParams *params);
int  mMask_copyData       (fitsfile *infptr, fitsfile *outfptr, struct mMaskParams *params);
void mMask_printFitsError (int);

#endif
