#ifndef MSUBSET_H
#define MSUBSET_H

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;


/**************************************/
/* Define mSubset function prototypes */
/**************************************/

void   mSubset_printError  (char *msg);
int    mSubset_swap        (double *x, double *y);
int    mSubset_stradd      (char *header, char *card);
int    mSubset_readTemplate(char *filename);
int    mSubset_parseLine   (char *line);
void   mSubset_fixxy       (double *x, double *y, int *offscl);
int    mSubset_Cross       (Vec *a, Vec *b, Vec *c);
double mSubset_Dot         (Vec *a, Vec *b);

#endif

