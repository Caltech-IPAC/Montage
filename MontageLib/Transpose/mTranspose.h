#ifndef MTRANSPOSE_H
#define MTRANSPOSE_H

#include <fitsio.h>

void  mTranspose_printFitsError  (int);
void  mTranspose_printError      (char *);
char *mTranspose_checkKeyword    (char *keyname, char *card, long naxis);
int   mTranspose_initTransform   (long *naxis, long *NAXIS);
void  mTranspose_transform       (int i, int j, int k, int l, int *it, int *jt, int *kt, int *lt);
int   mTranspose_analyzeCTYPE    (fitsfile *inFptr);

#endif
