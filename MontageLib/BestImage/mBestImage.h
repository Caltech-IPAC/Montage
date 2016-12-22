#ifndef MBESTIMAGE_H
#define MBESTIMAGE_H

/* Vector stuff */

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;


int    mBestImage_Cross    (Vec *a, Vec *b, Vec *c);
double mBestImage_Dot      (Vec *a, Vec *b);
double mBestImage_Normalize(Vec *a);

int    mBestImage_stradd(char *header, char *card);

#endif
