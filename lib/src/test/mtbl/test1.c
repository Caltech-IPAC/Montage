#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mtbl.h>

main(argc, argv)
int argc;
char **argv;
{
   int    i, icol, ncol;
   int    nrow, stat;
   char  *name, *value;

   if(argc == 1)
   {
      printf("No table file name given.\n");
      exit(0);
   }

   tsetlen(20000);


   /* Open table */

   ncol = topen(argv[1]);
   if(ncol < 0)
   {
      printf("Error opening table %s\n", argv[1]);
      exit(0);
   }


   /* See if there are any keyword = value lines */

   printf("\nKeyword line count: %d\n\n", tkeycount());


   /* Print out any keywords */

   for(i=0; i<tkeycount(); ++i)
   {
      printf("%3d: [%s] = [%s]\n", i+1, tkeyname(i), tkeyval(i));
   }


   /* See if there is an estimated record count */

   printf("\nEstimated record count: %d\n\n", tlen());


   /* Determine the column names */

   for(i=0; i<ncol; ++i)
   {
      name = tinfo(i);
      printf("%3d: \"%s\"\n", i+1, name);
   }


   /* Read the rows */

   nrow = 0;
   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      ++nrow;
      printf("\nrow %d\n", nrow);

      for(i=0; i<ncol; ++i)
      {
         value = tval(i);
         printf("     %3d: \"%s\"\n", i, value);
      }
   }


   /* Close table */

   tclose();
}

