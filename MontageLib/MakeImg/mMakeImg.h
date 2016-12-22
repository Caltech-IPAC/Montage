#ifndef MFIXNAN_H
#define MFIXNAN_H

void   mMakeImg_printFitsError(int);
int    mMakeImg_readTemplate  (char *filename);
int    mMakeImg_parseLine     (char *line);
double mMakeImg_ltqnorm       (double);
void   mMakeImg_fixxy         (double *x, double *y, int *offscl);
int    mMakeImg_swap          (double *x, double *y);
int    mMakeImg_nextStr       (FILE *fin, char *val);
double mMakeImg_ltqnorm       (double p);

#endif
