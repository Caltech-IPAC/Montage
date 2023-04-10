#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <svc.h>
#include <mtbl.h>

int main(int argc, char **argv)
{
   int    row, ncols, icdelt1, icdelt2, order;
   int    i, level;
   double nside;
   double cdelt1, cdelt2, cdelt;
   char   tblname[1024];

   double orderCdelt[32];

   for(i=0; i<25; ++i)
   {
      level = i + 9;

      nside = pow(2., (double)level);

      orderCdelt[i] = 90.0 / nside / sqrt(2.0);

      // printf("%d: %-g (%-g)\n", i, orderCdelt[i], orderCdelt[i]*3600.);
      // fflush(stdout);
   }


   // Process the command line.
   
   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXOrder tblfile\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(tblname, argv[1]);


   // Open the image list.  We only need the file name and HDU number.
   
   ncols = topen(tblname);

   icdelt1 = tcol("cdelt1");
   icdelt2 = tcol("cdelt2");


   // Loop over the image list, finding the smallest cdelt1/cdelt2.

   cdelt = 1.0e99;

   row = 0;

   while(tread() >= 0)
   {
      ++row;

      cdelt1 = fabs(atof(tval(icdelt1)));
      cdelt2 = fabs(atof(tval(icdelt2)));

      if(cdelt1 < cdelt)  cdelt = cdelt1;
      if(cdelt2 < cdelt)  cdelt = cdelt2;

      if(cdelt <= 0.)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid CDELT value in table row %d.\"]\n", row);
         fflush(stdout);
         exit(0);
      }
   }

   tclose();

   for(i=0; i<25; ++i)
      if(orderCdelt[i] < cdelt)
         break;

   level = i + 9;

   nside = pow(2., (double)level);

   printf("[struct stat=\"OK\", cdelt=%-g, order=%d, order_cdelt=%-g, nside=%.0f]\n", 
         cdelt, i, orderCdelt[i], nside);
   fflush(stdout);
   exit(0);
}
