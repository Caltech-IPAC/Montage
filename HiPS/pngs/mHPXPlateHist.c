#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <svc.h>

int main(int argc, char **argv)
{
   int i, j, istat;

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
            sprintf(cmd, "mHistogram -file fits/plate_%02d_%02d.fits min max gaussian-log -out plate_hist/plate_%02d_%02d.hist",
               i, j, i, j);

            svc_run(cmd);

            strcpy(status, svc_value("stat"));

            printf("%s -> %s\n", cmd, status);
         }
      }
   }

   exit(0);
}
