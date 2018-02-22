#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mSubimage.h>
#include <montage.h>


/**************************************************************************************/
/*                                                                                    */
/*  mSubimage has two major modes and two "convenience" modes (other parameters       */
/*  omitted for clarity):                                                             */
/*                                                                                    */
/*  mSubimage    [-d][-h hdu] in.fit out.fit racent    deccent   xsize     ysize      */
/*  mSubimage -p [-d][-h hdu] in.fit out.fit xstartpix ystartpix xsize     ysize      */
/*  mSubimage -a [-d] -h hdu  in.fit out.fit (ignored) (ignored) (ignored) (ignored)  */
/*  mSubimage -c [-d][-h hdu] in.fit out.fit (ignored) (ignored) (ignored) (ignored)  */
/*                                                                                    */
/*  mode 0 (SKY):    (xref,yref) are (ra,dec) of center, sizes in degrees             */
/*  mode 1 (PIX):    (xref,yref) are 'start' pixel coords, sizes in pixels            */
/*  mode 2 (HDU):    All the pixels; essentially for picking out an HDU               */
/*  mode 3 (SHRINK): All the pixels with blank edges trimmed off                      */
/*                                                                                    */
/*  HDU and SHRINK are special cases for convenience.  The 'nowcs' flag is a          */
/*  special case, too, and only makes sense in PIX mode.                              */
/*                                                                                    */
/**************************************************************************************/

int main(int argc, char **argv)
{
   int       i, debug, mode, pixmode;
   int       hdu, allPixels, haveHDU, shrinkWrap;
   int       nowcs;

   char      infile [1024];
   char      outfile[1024];
   char      appname[1024];

   char     *end;

   double    ra, dec, xsize, ysize;

   struct mSubimageReturn *returnStruct;

   FILE *montage_status;


   debug      = 0;
   nowcs      = 0;
   pixmode    = 0;
   allPixels  = 0;
   shrinkWrap = 0;
   hdu        = 0;
   haveHDU    = 0;

   montage_status = stdout;

   strcpy(appname, argv[0]);
      
   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-a(ll pixels)][-h hdu][-s statusfile] in.fit out.fit ra dec xsize [ysize] | %s -p [-d][-h hdu][-s statusfile] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize] | %s -c [-d][-h hdu][-s statusfile] in.fit out.fit\"]\n", appname, appname, appname);
      exit(1);
   }

   
   for(i=1; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
      
      if(strcmp(argv[i], "-nowcs") == 0)
         nowcs = 1;
      
      if(strcmp(argv[i], "-a") == 0)
         allPixels = 1;
      
      if(strcmp(argv[i], "-p") == 0)
         pixmode = 1;
      
      if(strcmp(argv[i], "-c") == 0)
         shrinkWrap = 1;

      if(i<argc-1 && strncmp(argv[i], "-h", 2) == 0)
      {
         hdu = strtol(argv[i+1], &end, 10);

         if(end < argv[i+1] + strlen(argv[i+1]) || hdu < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"HDU value (%s) must be a non-negative integer\"]\n",
               argv[i+1]);
            exit(1);
         }

         haveHDU = 1;
         ++i;
      }
      
      if(i<argc-1 && strncmp(argv[i], "-s", 2) == 0)
      {
         if((montage_status = fopen(argv[i+1], "w+")) == (FILE *)NULL)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
               argv[i+1]);
            exit(1);
         }

         ++i;
      }
   }
      
   if(debug)
   {
      printf("Enter mSubimage: debug= %d\n", debug);
      printf("nowcs      = %d\n", nowcs);
      printf("pixmode    = %d\n", pixmode);
      printf("shrinkWrap = %d\n", shrinkWrap);
      printf("allPixels  = %d\n", allPixels);
      fflush(stdout);
   }
  

   if(debug)
   {
      ++argv;
      --argc;
   }

   if(nowcs)
   {
      ++argv;
      --argc;
   }

   if(allPixels)
   {
      ++argv;
      --argc;
   }

   if(shrinkWrap)
   {
      ++argv;
      --argc;
   }


   if(pixmode)
   {
      ++argv;
      --argc;
   }


   if(montage_status != stdout)
   {
      argv += 2;
      argc -= 2;
   }

   if(haveHDU)
   {
      argv += 2;
      argc -= 2;
   }

   if((shrinkWrap || allPixels) && argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-a(ll pixels)][-h hdu][-s statusfile] in.fit out.fit ra dec xsize [ysize] | %s -p [-d][-h hdu][-s statusfile] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize] | %s -c [-d][-h hdu][-s statusfile] in.fit out.fit\"]\n", appname, appname, appname);
      exit(1);
   }

   if (!shrinkWrap && !allPixels && (argc < 6 || (pixmode && argc < 6))) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-a(ll pixels)][-h hdu][-s statusfile] in.fit out.fit ra dec xsize [ysize] | %s -p [-d][-h hdu][-s statusfile] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize] | %s -c [-d][-h hdu][-s statusfile] in.fit out.fit\"]\n", appname, appname, appname);
      exit(1);
   }

   strcpy(infile,  argv[1]);
   strcpy(outfile, argv[2]);

   mode = SKY;

   if(allPixels)
      mode = HDU;

   if(shrinkWrap)
      mode = SHRINK;

   if(pixmode)
      mode = PIX;

   if(allPixels)
   {
      pixmode = 1;

      ra  = 0.;
      dec = 0.;

      xsize = 1.e25;
      ysize = 1.e25;
   }
   else if(!shrinkWrap)
   {
      
         ra  = strtod(argv[3], &end);

         if(end < argv[3] + (int)strlen(argv[3]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Center RA string (%s) cannot be interpreted as a real number\"]\n", 
               argv[3]);
            exit(1);
         }

         dec = strtod(argv[4], &end);

         if(end < argv[4] + (int)strlen(argv[4]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Center Dec string (%s) cannot be interpreted as a real number\"]\n", 
               argv[4]);
            exit(1);
         }

      xsize = strtod(argv[5], &end);
      ysize = xsize;

      if(end < argv[5] + (int)strlen(argv[5]))
      {
         printf("[struct stat=\"ERROR\", msg=\"X size string (%s) cannot be interpreted as a real number\"]\n", 
            argv[5]);
         exit(1);
      }

      if (argc > 6) 
      {
         ysize = strtod(argv[6], &end);

         if(end < argv[6] + (int)strlen(argv[6]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Y size string (%s) cannot be interpreted as a real number\"]\n", 
               argv[6]);
            exit(1);
         }
      }
   }
   
   if(!shrinkWrap)
   {
      if(xsize <= 0.)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid 'x' size\"]\n");
         exit(1);
      }

      if(ysize <= 0.)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid 'y' size\"]\n");
         exit(1);
      }
   }
   
   returnStruct = mSubimage(mode, infile, outfile, ra, dec, xsize, ysize, hdu, nowcs, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mSubimage\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
