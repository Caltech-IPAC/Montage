#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <montage.h>

#define MAXSTR  256
#define MAXFILE 256

/******************************************************/
/*                                                    */
/*  mMakeImg main routine                             */
/*                                                    */
/******************************************************/


int main(int argc, char **argv)
{
   int       i, debug, index, ifile;
   double    bg1, bg2, bg3, bg4, noise;
   int       haveTemplate, haveOut, region, replace;

   char      template_file[MAXSTR];
   char      output_file  [MAXSTR];
   char    **cat_file;
   char    **image_file;
   char    **colname;
   double    width        [MAXFILE];
   double    tblEpoch     [MAXFILE];
   double    refmag       [MAXFILE];
   char      arrayfile    [MAXSTR];
   
   int       ncat   = 0;
   int       nimage = 0;

   FILE     *montage_status;

   struct mMakeImgReturn *returnStruct;


   montage_status = stdout;

   cat_file   = (char **)malloc(MAXFILE*sizeof(char *));
   image_file = (char **)malloc(MAXFILE*sizeof(char *));
   colname    = (char **)malloc(MAXFILE*sizeof(char *));

   for(i=0; i<MAXFILE; ++i)
   {
      cat_file[i]   = (char *)malloc(MAXSTR*sizeof(char));
      image_file[i] = (char *)malloc(MAXSTR*sizeof(char));
      colname[i]    = (char *)malloc(MAXSTR*sizeof(char));
   }

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
      exit(1);
   }

   debug = 0;

   haveTemplate = 0;
   haveOut      = 0;

   bg1     = 0.;
   bg2     = 0.;
   bg3     = 0.;
   bg4     = 0.;

   noise   = 0.;

   index   = 1;
   replace = 0;
   region  = 0;

   while(1)
   {
      if(index >= argc)
         break;

      if(argv[index][0] == '-')
      {
         if(strlen(argv[index]) < 2)
         {
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
            exit(1);
         }

         switch(argv[index][1])
         {
            case 'd':
               if(argc < index+2)
               {
                  printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
                  exit(1);
               }

               debug = atoi(argv[index+1]);

               index += 2;

               break;


            case 'r':
               replace = 1;

               ++index;

               break;


            case 'f':
               region = 1;

               ++index;

               break;


            case 'a':
               strcpy(arrayfile, argv[index+1]);

               index += 2;

               break;


            case 'n':
               if(argc < index+2)
               {
                  printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
                  exit(1);
               }

               noise = atof(argv[index+1]);

               index += 2;

               break;


            case 'b':
               if(argc < index+5)
               {
                  printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
                  exit(1);
               }

               bg1 = atof(argv[index+1]);
               bg2 = atof(argv[index+2]);
               bg3 = atof(argv[index+3]);
               bg4 = atof(argv[index+4]);

               index += 5;

               break;


            case 't':
               if(argc < index+6)
               {
                  printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
                  exit(1);
               }

               strcpy(cat_file[ncat], argv[index+1]);
               strcpy(colname [ncat], argv[index+2]);

               width   [ncat] = atof(argv[index+3]);
               tblEpoch[ncat] = atof(argv[index+4]);
               refmag  [ncat] = atof(argv[index+5]);

               index += 6;

               ++ncat;

               break;
               


            case 'i':
               if(argc < index+2)
               {
                  printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
                  exit(1);
               }

               strcpy(image_file[nimage], argv[index+1]);

               index += 2;

               ++nimage;

               break;
               
            default:
               break;
         }
      }

      else if(!haveTemplate)
      {
         strcpy(template_file, argv[index]);
         ++index;
         haveTemplate = 1;
      }

      else if(!haveOut)
      {
         strcpy(output_file, argv[index]);
         ++index;
         haveOut = 1;
      }

      else
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-i imagetbl] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
         exit(1);
      }
   }

   if(debug >= 1)
   {
      printf("debug         = %d\n",  debug);
      printf("noise         = %-g\n", noise);
      printf("bg1           = %-g\n", bg1);
      printf("bg2           = %-g\n", bg2);
      printf("bg3           = %-g\n", bg3);
      printf("bg4           = %-g\n", bg4);
      printf("template_file = %s\n",  template_file);
      printf("output_file   = %s\n",  output_file);
      fflush(stdout);

      for(ifile=0; ifile<ncat; ++ifile)
      {
         printf("cat_file[%d] = %s\n",   ifile, cat_file[ifile]);
         printf("colname [%d] = %s\n",   ifile, colname [ifile]);
         printf("width   [%d] = %-g\n",  ifile, width   [ifile]);
         printf("tblEpoch[%d] = %-g\n",  ifile, tblEpoch[ifile]);
         printf("refmag  [%d] = %-g\n",  ifile, refmag  [ifile]);
         fflush(stdout);
      }

      for(ifile=0; ifile<nimage; ++ifile)
      {
         printf("image_file[%d] = %s\n",   ifile, image_file[ifile]);
         fflush(stdout);
      }
   }

   if(!haveOut)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tbl col width epoch refmag] [-f(lat-sources)] [-a array.txt] template.hdr out.fits (-t args can be repeated)\"]\n", argv[0]);
      exit(1);
   }


   /****************************************/
   /* Call the mMakeImg processing routine */
   /****************************************/


   returnStruct = mMakeImg(template_file, output_file, noise, bg1, bg2, bg3, bg4,
                           ncat, cat_file, colname, width, refmag, tblEpoch, region,
                           nimage, image_file, arrayfile, replace, debug);

   if(returnStruct->status == 1)
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      exit(1);
   }
   else
   {   
      fprintf(montage_status, "[struct stat=\"OK\", module=\"mMakeImg\"]\n");
      exit(0);
   }
}             
