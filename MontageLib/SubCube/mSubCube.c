#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mSubCube.h>
#include <montage.h>

#define STRLEN 32768


/*************************************************************************************/
/*                                                                                   */
/*  mSubCube has two major modes and two "convenience" modes (other parameters       */
/*  omitted for clarity):                                                            */
/*                                                                                   */
/*  mSubCube    [-d][-h hdu] in.fit out.fit racent    deccent   xsize     ysize      */
/*  mSubCube -p [-d][-h hdu] in.fit out.fit xstartpix ystartpix xsize     ysize      */
/*  mSubCube -a [-d] -h hdu  in.fit out.fit (ignored) (ignored) (ignored) (ignored)  */
/*  mSubCube -c [-d][-h hdu] in.fit out.fit (ignored) (ignored) (ignored) (ignored)  */
/*                                                                                   */
/*  mode 0 (SKY):    (xref,yref) are (ra,dec) of center, sizes in degrees            */
/*  mode 1 (PIX):    (xref,yref) are 'start' pixel coords, sizes in pixels           */
/*  mode 2 (HDU):    All the pixels; essentially for picking out an HDU              */
/*  mode 3 (SHRINK): All the pixels with blank edges trimmed off                     */
/*                                                                                   */
/*  HDU and SHRINK are special cases for convenience.  The 'nowcs' flag is a         */
/*  special case, too, and only makes sense in PIX mode.                             */
/*                                                                                   */
/*************************************************************************************/

int main(int argc, char **argv)
{
   int       i, debug, mode, pixmode;
   int       hdu, allPixels, haveHDU, shrinkWrap;
   int       havePlane, haveD3, haveD4;
   int       d3begin, d3end;
   int       nowcs;

   char      infile      [STRLEN];
   char      outfile     [STRLEN];
   char      appname     [STRLEN];
   char      d3constraint[STRLEN];
   char      d4constraint[STRLEN];

   char     *end;

   double    ra, dec, xsize, ysize;

   struct mSubCubeReturn *returnStruct;

   FILE *montage_status;


   debug      = 0;
   nowcs      = 0;
   pixmode    = 0;
   allPixels  = 0;
   shrinkWrap = 0;
   hdu        = 0;
   haveHDU    = 0;
   havePlane  = 0;
   haveD3     = 0;
   haveD4     = 0;

   montage_status = stdout;

   strcpy(d3constraint, "");
   strcpy(d4constraint, "");


   strcpy(appname, argv[0]);
      
   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-D3 selection-list][-D4 selection-list][-d][-a(ll pixels)][-h hdu][-s statusfile] in.fit out.fit ra dec xsize [ysize] | %s -p [-D3 selection-list][-D4 selection-list][-d][-h hdu][-s statusfile] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize] | %s -c [-D3 selection-list][-D4 selection-list][-d][-h hdu][-s statusfile] in.fit out.fit\"]\n", appname, appname, appname);
      exit(1);
   }

   for(i=1; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
      
      else if(strcmp(argv[i], "-nowcs") == 0)
         nowcs = 1;
      
      else if(strcmp(argv[i], "-a") == 0)
         allPixels = 1;
      
      else if(strcmp(argv[i], "-p") == 0)
         pixmode = 1;
      
      else if(strcmp(argv[i], "-c") == 0)
         shrinkWrap = 1;

      else if(i<argc-1 && strncmp(argv[i], "-h", 2) == 0)
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
      
      else if(i<argc-2 && strncmp(argv[i], "-P", 2) == 0)
      {
         d3begin = -1;
         d3end   = -1;
         
         d3begin = strtol(argv[i+1], &end, 10);

         if(end < argv[i+1] + strlen(argv[i+1]) || d3begin < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Starting plane value (%s) must be a non-negative integer\"]\n",
               argv[i+1]);
            exit(1);
         }

         d3end = strtol(argv[i+2], &end, 10);

         if(end < argv[i+2] + strlen(argv[i+2]) || d3end < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Ending plane value (%s) must be a non-negative integer\"]\n",
               argv[i+1]);
            exit(1);
         }

         sprintf(d3constraint, "%d:%d", d3begin, d3end);
         
         havePlane = 1;
         i+=2;
      }
      
      else if(i<argc-1 && strncmp(argv[i], "-D3", 3) == 0)
      {
         strcpy(d3constraint, argv[i+1]);

         haveD3 = 1;
         i+=1;
      }
      
      else if(i<argc-1 && strncmp(argv[i], "-D4", 3) == 0)
      {
         strcpy(d4constraint, argv[i+1]);

         haveD4 = 1;
         i+=1;
      }
      
      else if(i<argc-1 && strncmp(argv[i], "-s", 2) == 0)
      {
         if((montage_status = fopen(argv[i+1], "w+")) == (FILE *)NULL)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
               argv[i+1]);
            exit(1);
         }

         ++i;
      }

      else if(argv[i][0] == '-')
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid flag '%s'.\"]\n", argv[i]);
         exit(1);
      }

      else
         break;
   }
      
   if (havePlane && haveD3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot mix -P and -D3 constraint syntaxes.\"]\n");
      exit(1);
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

   if(havePlane)
   {
      argv += 3;
      argc -= 3;
   }

   if(haveD3)
   {
      argv += 2;
      argc -= 2;
   }

   if(haveD4)
   {
      argv += 2;
      argc -= 2;
   }

   if((shrinkWrap || allPixels) && argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-D3 selection-list][-D4 selection-list][-d][-a(ll pixels)][-h hdu][-s statusfile] in.fit out.fit ra dec xsize [ysize] | %s -p [-D3 selection-list][-D4 selection-list][-d][-h hdu][-s statusfile] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize] | %s -c [-D3 selection-list][-D4 selection-list][-d][-h hdu][-s statusfile] in.fit out.fit\"]\n", appname, appname, appname);
      exit(1);
   }

   if (!shrinkWrap && !allPixels && (argc < 6 || (pixmode && argc < 6))) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-D3 selection-list][-D4 selection-list][-d][-a(ll pixels)][-h hdu][-s statusfile] in.fit out.fit ra dec xsize [ysize] | %s -p [-D3 selection-list][-D4 selection-list][-d][-h hdu][-s statusfile] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize] | %s -c [-D3 selection-list][-D4 selection-list][-d][-h hdu][-s statusfile] in.fit out.fit\"]\n", appname, appname, appname);
      exit(1);
   }

   if(argv[1][0] == '-')
   {  
      printf("[struct stat=\"ERROR\", msg=\"Invalid flag '%s'.\"]\n", argv[1]);
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
         fflush(stdout);
         exit(1);
      }

      dec = strtod(argv[4], &end);

      if(end < argv[4] + (int)strlen(argv[4]))
      {
         printf("[struct stat=\"ERROR\", msg=\"Center Dec string (%s) cannot be interpreted as a real number\"]\n", 
            argv[4]);
         fflush(stdout);
         exit(1);
      }

      xsize = strtod(argv[5], &end);
      ysize = xsize;

      if(end < argv[5] + (int)strlen(argv[5]))
      {
         printf("[struct stat=\"ERROR\", msg=\"X size string (%s) cannot be interpreted as a real number\"]\n", 
            argv[5]);
         fflush(stdout);
         exit(1);
      }

      if (argc > 6) 
      {
         ysize = strtod(argv[6], &end);

         if(end < argv[6] + (int)strlen(argv[6]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Y size string (%s) cannot be interpreted as a real number\"]\n", 
               argv[6]);
            fflush(stdout);
            exit(1);
         }
      }
   }
   
   if(!shrinkWrap)
   {
      if(xsize <= 0.)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid 'x' size\"]\n");
         fflush(stdout);
         exit(1);
      }

      if(ysize <= 0.)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid 'y' size\"]\n");
         fflush(stdout);
         exit(1);
      }
   }


   returnStruct = mSubCube(mode, infile, outfile, ra, dec, xsize, ysize, hdu, nowcs, d3constraint, d4constraint, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mSubCube\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
