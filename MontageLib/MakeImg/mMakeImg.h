#ifndef MFIXNAN_H
#define MFIXNAN_H

#define JSONFILE 0
#define JSONMODE 1
#define CMDMODE  2
  
#define STRLEN  1024
#define MAXFILE 256
#define MAXJSON 800000

void   mMakeImg_parseCoordStr (char *coordStr, int *csys, double *epoch);
void   mMakeImg_printFitsError(int);
int    mMakeImg_readTemplate  (char *filename);
int    mMakeImg_parseLine     (char *line);
double mMakeImg_ltqnorm       (double);
void   mMakeImg_fixxy         (double *x, double *y, int *offscl);
int    mMakeImg_swap          (double *x, double *y);
int    mMakeImg_nextStr       (FILE *fin, char *val);
double mMakeImg_ltqnorm       (double p);
void   mMakeImg_cleanup       ();

#endif
