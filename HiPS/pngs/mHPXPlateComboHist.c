#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <svc.h>

int main(int argc, char **argv)
{
   int i, j, ii, jj, istat;

   char filename[1024];
   char cmd     [1024];
   char status  [1024];

   struct stat buf;


   for(i=0; i<80; ++i)
   {
      for(j=0; j<80; ++j)
      {
         sprintf(filename, "fits/plate_%02d_%02d.fits", i, j);

         istat = stat(filename, &buf);

         if(istat == 0)
         {
            sprintf(cmd, "mCombineHist -stretch min max gaussian-log -out region_hist/plate_%02d_%02d.hist ", i, j);

            for(ii=i-2; ii<=i+2; ++ii)
            {
               for(jj=j-2; jj<=j+2; ++jj)
               {
                  sprintf(filename, "plate_hist/plate_%02d_%02d.hist", ii, jj);

                  istat = stat(filename, &buf);

                  if(istat == 0)
                  {
                     strcat(cmd, " ");
                     strcat(cmd, filename);
                  }
               }
            }

            svc_run(cmd);

            strcpy(status, svc_value("stat"));

            printf("%s -> %s\n", cmd, status);
            fflush(stdout);
         }
      }
   }

   exit(0);
}
