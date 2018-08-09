#ifndef MEXAMINE_H
#define MEXAMINE_H

#define ALL     0
#define REGION  1
#define APPHOT  2


/***************************************/
/* Define mExamine function prototypes */
/***************************************/

int mExamine_getPlanes(char *, int *);
int mExamine_radCompare(const void *p1, const void *p2);

#endif
