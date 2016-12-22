#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <montage.h>

int main(int argc, char **argv)
{
   int  status, debug;

   char infile  [1024];
   char outfile [1024];
   char template[1024];

   strcpy(infile,   argv[1]);
   strcpy(template, argv[3]);

   debug = atoi(argv[4]);


   sprintf(outfile, "%s", argv[2]);

   status = mProject  (infile, 0, outfile, template, "", 1., 0., "", 0., 1., 0, 0, 0, debug);

   if(status)
      printf("mProject ERROR: %s\n", montage_msgstr);
   else
      printf("mProject done.\n");


   sprintf(outfile, "PP%s", argv[2]);

   status = mProjectPP(infile, 0, outfile, template, "", 1., 0., "", "", "", 0., 1., 0, 0, 999);

   if(status)
      printf("mProjectPP ERROR: %s\n", montage_msgstr);
   else
      printf("PP done.\n");


   sprintf(outfile, "QL%s", argv[2]);

   status = mProjectQL(infile, 0, outfile, template, 0, "", 1., 0., "", 1., 0, 0, 0, debug);

   if(status)
      printf("mProjectQL ERROR: %s\n", montage_msgstr);
   else
      printf("QL done.\n");


   exit(0);
}
