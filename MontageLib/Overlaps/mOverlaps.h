#ifndef MOVERLAPS_H
#define MOVERLAPS_H

/* Vector stuff */

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;


/****************************************/
/* Define mOverlaps function prototypes */
/****************************************/

int     mOverlaps_stradd         (char *header, char *card);
void    mOverlaps_fixxy          (int id, double *x, double *y, int *offscl);
char   *mOverlaps_fileName       (char *fname);

int     mOverlaps_SegSegIntersect(Vec *a, Vec *b, Vec *c, Vec *d, 
                                  Vec *e, Vec *f, Vec *p, Vec *q);
int     mOverlaps_Cross          (Vec *a, Vec *b, Vec *c);
double  mOverlaps_Dot            (Vec *a, Vec *b);
double  mOverlaps_Normalize      (Vec *a);
void    mOverlaps_Reverse        (Vec *a);
int     mOverlaps_Between        (Vec *a, Vec *b, Vec *c);

#endif
