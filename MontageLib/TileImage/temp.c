#include <stdio.h>
#include <svc.h>

int main(int argc, char **argv)
{
   int i, j;

   int naxis1 = 14000;
   int naxis2 = 14000;

   int nx = 26;
   int ny = 26;

   int xpad = 256;
   int ypad = 256;

   int xsize, ysize;
   int xoffset, yoffset;

   xsize = naxis1/nx;
   ysize = naxis2/ny;

   for(i=0; i<nx; ++i)
   {
      for(j=0; j<ny; ++j)
      {
         xoffset = i * xsize - xpad;
         yoffset = j * ysize - ypad;

         printf("%d %d: mSubimage -p in.fits out.fits %d %d %d %d\n",
               i, j, xoffset, yoffset, xsize+2*xpad, ysize+2*ypad);
         fflush(stdout);
      }
   }
}


