#ifndef MSUBHDR_H
#define MSUBHDR_H

#include <fitsio.h>

#define SKY    0
#define PIX    1

/**************************************/
/* Define mSubHdr function prototypes */
/**************************************/

void              mSubHdr_fixxy          (double *x, double *y, int *offscl);
int               mSubHdr_readTemplate   (char *filename);
int               mSubHdr_parseLine      (char *line);
void              mSubHdr_printFitsError (int);

#endif
