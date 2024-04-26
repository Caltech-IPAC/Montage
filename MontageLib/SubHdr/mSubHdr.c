#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mSubHdr.h>
#include <montage.h>


/**************************************************************************************/
/*                                                                                    */
/*  mSubHdr has two major modes and two "convenience" modes (other parameters         */
/*  omitted for clarity):                                                             */
/*                                                                                    */
/*  mSubHdr    in.hdr out.hdr racent    deccent   xsize ysize                         */
/*  mSubHdr -p in.hdr out.hdr xstartpix ystartpix xsize ysize                         */
/*                                                                                    */
/*  mode 0 (SKY):    (xref,yref) are (ra,dec) of center, sizes in degrees             */
/*  mode 1 (PIX):    (xref,yref) are 'start' pixel coords, sizes in pixels            */
/*                                                                                    */
/*  The 'nowcs' flag is a special case and only makes sense in PIX mode.              */
/*  The 'shift' flag tells the program to try to shift the center if the region goes  */
/*  off the image.                                                                    */
/*                                                                                    */
/**************************************************************************************/

int main(int argc, char **argv)
{
   int       i, debug, mode, pixmode, shift;
   int       nowcs;

   char      infile [1024];
   char      outfile[1024];
   char      appname[1024];

   char     *end;

   double    ra, dec;
   char      xsize[1024];
   char      ysize[1024];

   struct mSubHdrReturn *returnStruct;

   FILE *montage_status;


   debug      = 0;
   nowcs      = 0;
   pixmode    = 0;
   shift      = 0;

   montage_status = stdout;

   strcpy(appname, argv[0]);
      
   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(hift)] in.fit out.fit ra dec xsize [ysize] | %s -p [-d][-s(hift)] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize]\"]\n", appname, appname);
      exit(1);
   }

   
   for(i=1; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
      
      if(strcmp(argv[i], "-nowcs") == 0)
         nowcs = 1;
      
      if(strcmp(argv[i], "-s") == 0)
         shift = 1;
      
      if(strcmp(argv[i], "-p") == 0)
         pixmode = 1;
   }
      
   if(debug)
   {
      printf("Enter mSubHdr: debug= %d\n", debug);
      printf("nowcs      = %d\n", nowcs);
      printf("pixmode    = %d\n", pixmode);
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

   if(shift)
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

   if (pixmode && argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(hift)] in.fit out.fit ra dec xsize [ysize] | %s -p [-d][-s(hift)] in.fit out.fit xstartpix ystartpix xpixsize [ypixsize]\"]\n", appname, appname);
      exit(1);
   }

   strcpy(infile,  argv[1]);
   strcpy(outfile, argv[2]);

   mode = SKY;

   if(pixmode)
      mode = PIX;

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

   strcpy(xsize, argv[5]);
   strcpy(ysize, xsize);

   if (argc > 6) 
      strcpy(ysize, argv[6]);
  
   returnStruct = mSubHdr(infile, outfile, ra, dec, xsize, ysize, mode, nowcs, shift, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mSubHdr\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
