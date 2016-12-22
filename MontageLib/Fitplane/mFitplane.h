#ifndef MPROJECT_H
#define MPROJECT_H

/****************************************/
/* Define mFitplane function prototypes */
/****************************************/

int    mFitplane_gaussj        (double **, int, double **, int);
void   mFitplane_nrerror       (char *);
int   *mFitplane_ivector       (int);
void   mFitplane_free_ivector  (int *);
void   mFitplane_printFitsError(int);

#endif
