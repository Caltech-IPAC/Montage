#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <svc.h>
#include <errno.h>

#define MAXSTR 4096

int  nimg[10] = {12, 48, 192, 768, 3072, 12288, 49152, 196608, 786432, 3145728};

int  debug;


int main(int argc, char **argv)
{
   int i, order, dirno;

   char dirname[MAXSTR];

   for(order=0; order<=9; ++order)
   {
      for(i=0; i<350; ++i)
      {
         dirno = i * 10000;

         if(dirno > nimg[order])
            continue;

         sprintf(dirname, "Norder%d/Dir%d", order, dirno);

         printf("echo \"%s\"\n", dirname);
         printf("ls -l %s | wc\n", dirname);
      }
   }

   exit(0);
}
