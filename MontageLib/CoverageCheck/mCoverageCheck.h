#ifndef MCOVERAGECHECK_H
#define MCOVERAGECHECK_H

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;

#define POINTS  0
#define BOX     1
#define CIRCLE  2
#define POINT   3
#define HEADER  4
#define CUTOUT  5


/*********************************************/
/* Define mCoverageCheck function prototypes */
/*********************************************/

int     mCoverageCheck_Cross               (Vec *a, Vec *b, Vec *c);
double  mCoverageCheck_Dot                 (Vec *a, Vec *b);
double  mCoverageCheck_Normalize           (Vec *a);
void    mCoverageCheck_Reverse             (Vec *a);
int     mCoverageCheck_SegSegIntersect     (Vec *a, Vec *b, Vec *c, Vec *d, 
                                            Vec *e, Vec *f, Vec *p, Vec *q);
int     mCoverageCheck_Between             (Vec *a, Vec *b, Vec *c);

int     mCoverageCheck_swap                (double *x, double *y);
int     mCoverageCheck_stradd              (char *header, char *card);

#endif
