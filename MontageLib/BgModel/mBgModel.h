#ifndef MBGMODEL_H
#define MBGMODEL_H

/***************************************/
/* Define mBgModel function prototypes */
/***************************************/

int    mBgModel_gaussj      (float **, int, float **, int);
int   *mBgModel_ivector     (int);
void   mBgModel_free_ivector(int *);

#endif
