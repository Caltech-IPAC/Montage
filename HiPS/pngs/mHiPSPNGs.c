/* Module: mHiPSPNGs.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        14Jul19  Baseline code
1.1      John Good        27Aug19  Added color image support

*/

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
#include <mtbl.h>
#include <errno.h>

#define MAXSTR 4096

//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 12->6

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3, 4};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2, 4};

int  mHiPSPNGs_getFiles      (char *pathname);
void mHiPSPNGs_printFitsError(int status);
int  mHiPSPNGs_mkdir         (const char *path);
void mHiPSPNGs_transform     (int order, int tile, int *xtile, int *ytile);
void mHPXHiPSPNGs_splitIndex (long index, int level, int *x, int *y);

int  nimage, nerror;

double brightness, contrast;
double brightness_val, contrast_val;

double bbrightness, bcontrast;
double gbrightness, gcontrast;
double rbrightness, rcontrast;

int    ibbrightness, ibcontrast;
int    igbrightness, igcontrast;
int    irbrightness, ircontrast;

int    ncols, ncont, iorder, torder;

char directory1[1024];
char directory2[1024];
char directory3[1024];
char histfile1 [1024];
char histfile2 [1024];
char histfile3 [1024];
char outdir    [1024];
char contfile  [1024];

int  no_transpose, update_only, histfile_is_dir, ct;
int  order, nplate;
int  len1, lenout;

int  color = 0;

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int  debug;


/*************************************************************************/
/*                                                                       */
/*  mHiPSPNGs                                                            */
/*                                                                       */
/*  This program recursively finds all the FITS files in a directory     */
/*  tree and generates a matching PNG (grayscale) using the supplied     */
/*  histogram.                                                           */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int c, i;

   struct stat buf;

   brightness      =  0;
   contrast        =  0;
   no_transpose    =  0;
   update_only     =  0;
   histfile_is_dir =  0;
   ct              = -1;
   order           = -1;
   nplate          = -1;

   strcpy(contfile, "");

   while((c = getopt(argc, argv, "dnub:c:C:t:o:p:")) != EOF)
   {
      switch(c)
      {
         case 'd':
            debug = 1;
            svc_debug(stdout);
            break;

         case 'n':
            no_transpose = 1;
            break;

         case 'u':
            update_only = 1;
            break;

         case 'b':
            brightness = atof(optarg);
            break;

         case 'c':
            contrast = atof(optarg);
            break;

         case 'C':
            strcpy(contfile, optarg);
            break;

         case 't':
            ct = atoi(optarg);
            break;

         case 'o':
            order = atoi(optarg);
            break;

         case 'p':
            nplate = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs [-d][-n(o-transpose)][-b brightness][-c contrast][-t color-table][-o order][-p nplate] directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
            exit(1);
            break;
      }
   }

   if(argc-optind < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs [-d][-n(o-transpose)][-b brightness][-c contrast][-t color-table][-o order][-p nplate] directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
      exit(1);
   }

   if(ct < 0)
      ct = 0;

   if(order >= 0 || nplate > 0)
   {
      if(order < 0 || nplate <= 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"If either is given, order and nplate must be positive integers.\"]\n");
         exit(1);
      }
   }

   strcpy(directory1, argv[optind]);
   strcpy(histfile1,  argv[optind+1]);

   if(directory1[strlen(directory1)-1] != '/')
      strcat(directory1, "/");

   if (stat(histfile1, &buf) == 0) 
   {
      if (S_ISDIR(buf.st_mode) == 1)
         histfile_is_dir = 1;
   }

   if(histfile_is_dir && histfile1[strlen(histfile1)-1] != '/')
      strcat(histfile1, "/");

   len1 = strlen(directory1);

   strcpy(outdir, argv[optind+2]);

   if(outdir[strlen(outdir)-1] != '/')
      strcat(outdir, "/");


   if(argc-optind > 3)
   {
      if(argc-optind < 7)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs [-d][-n(o-transpose)][-b brightness][-c contrast][-t color-table][-o order][-p nplate] directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
         exit(1);
      }

      strcpy(directory2, argv[optind+2]);
      strcpy(histfile2,  argv[optind+3]);

      if(histfile_is_dir && histfile2[strlen(histfile2)-1] != '/')
         strcat(histfile2, "/");

      strcpy(directory3, argv[optind+4]);
      strcpy(histfile3,  argv[optind+5]);

      if(histfile_is_dir && histfile3[strlen(histfile3)-1] != '/')
         strcat(histfile3, "/");

      strcpy(outdir, argv[optind+6]);

      if(directory2[strlen(directory2)-1] != '/')
         strcat(directory2, "/");

      if(directory3[strlen(directory3)-1] != '/')
         strcat(directory3, "/");

      if(outdir[strlen(outdir)-1] != '/')
         strcat(outdir, "/");

      lenout = strlen(outdir);

      color = 1;
   }

   nimage = 0;
   nerror = 0;


   // Read through the brightness/contrast table, if given,
   // to find the values for this order

   bbrightness = brightness;
   gbrightness = brightness;
   rbrightness = brightness;

   bcontrast   = contrast;
   gcontrast   = contrast;
   rcontrast   = contrast;

   if(strlen(contfile) > 0)
   {
      ncols = topen(contfile);

      if(ncols > 0)
      {
         iorder = tcol("order");

         ibbrightness = tcol("bbrightness");
         igbrightness = tcol("gbrightness");
         irbrightness = tcol("rbrightness");

         ibcontrast   = tcol("bcontrast");
         igcontrast   = tcol("gcontrast");
         ircontrast   = tcol("rcontrast");

         if(iorder       < 0
         || ibbrightness < 0
         || igbrightness < 0
         || irbrightness < 0
         || ibcontrast   < 0
         || igcontrast   < 0
         || ircontrast   < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"brightness/contrast table is missing required columns.\"]\n");
            exit(1);
         }

         while(1)
         {
            if(tread() < 0)
               break;

            torder = atoi(tval(iorder));

            if(torder < 0 || torder > 15)
            {
               printf("[struct stat=\"ERROR\", msg=\"brightness/contrast has an invalid HiPS order entry.\"]\n");
               exit(1);
            }

            if(torder != order)
               continue;

            bbrightness = atof(tval(ibbrightness));
            gbrightness = atof(tval(igbrightness));
            rbrightness = atof(tval(irbrightness));

            bcontrast   = atof(tval(ibcontrast));
            gcontrast   = atof(tval(igcontrast));
            rcontrast   = atof(tval(ircontrast));

            ++ncont;
         }
      }
   }

   if(debug)
   {
      printf("\n");
      printf("DEBUG> contfile:    [%s]\n",  contfile);
      printf("\n");
      printf("DEBUG> ibbrightness = %d\n",  ibbrightness);
      printf("DEBUG> igbrightness = %d\n",  igbrightness);
      printf("DEBUG> irbrightness = %d\n",  irbrightness);
      printf("DEBUG> ibcontrast   = %d\n",  ibcontrast);
      printf("DEBUG> igcontrast   = %d\n",  igcontrast);
      printf("DEBUG> ircontrast   = %d\n",  ircontrast);
      printf("\n");
      printf("DEBUG> bbrightness  = %-g\n", bbrightness);
      printf("DEBUG> gbrightness  = %-g\n", gbrightness);
      printf("DEBUG> rbrightness  = %-g\n", rbrightness);
      printf("DEBUG> bcontrast    = %-g\n", bcontrast);
      printf("DEBUG> gcontrast    = %-g\n", gcontrast);
      printf("DEBUG> rcontrast    = %-g\n", rcontrast);
      printf("\n");
      fflush(stdout);
   }


   // Find the right brightness/contrast for this order


   

   mHiPSPNGs_getFiles(directory1);

   printf("[struct stat=\"OK\", module=\"mHiPSPNGs\", directory=\"%s\", nimage=%d, nerror=%d]\n", 
         directory1, nimage, nerror);
   fflush(stdout);
   exit(0);
}



/*******************************/
/*                             */
/*  Step through the directory */
/*                             */
/*******************************/

int mHiPSPNGs_getFiles (char *pathname)
{
   int             len, i, ntile, iplate, jplate;

   char            filename1 [MAXSTR];
   char            filename2 [MAXSTR];
   char            filename3 [MAXSTR];
   char            transfile1[MAXSTR];
   char            transfile2[MAXSTR];
   char            transfile3[MAXSTR];
   char            hist1     [MAXSTR];
   char            hist2     [MAXSTR];
   char            hist3     [MAXSTR];
   char            pngfile   [MAXSTR];
   char            newdir    [MAXSTR];
   char            cmd       [MAXSTR];

   char            status[32];

   char           *ptr;

   int             tileID, xtile, ytile;

   DIR            *dp;
   struct dirent  *entry;
   struct stat     buf;

   dp = opendir (pathname);

   if(debug)
   {
      printf("DEBUG> Opening path    [%s]\n", pathname);
      fflush(stdout);
   }

   if (dp == NULL) 
      return 0;


   // Histogram file logic:  If we are given a single histogram file
   // for the image (or first image set if there are three), we use that.
   // If what we are given turns out to be a directory, we assume it
   // contains one histogram file per image, with the same base name.
   //
   // The most complicated histogram setup supported is where histograms
   // are generated on a per-"plate" basis, in which case we need to 
   // look them up by translating the tile ID in the file name (e.g.,
   // "Npix3451.fits") into pixel coordinates and then into plate coords.
   // These histograms have to follow a naming convention 
   // (e.g, "plate_03_05.hist").
   
   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(debug)
      {
         printf("\nDEBUG>  entry [%s]\n", entry->d_name);
         fflush(stdout);
      }

      sprintf (filename1, "%s%s", pathname, entry->d_name);

      if(color)
      {
         sprintf(filename2, "%s%s", directory2, filename1+len1);
         sprintf(filename3, "%s%s", directory3, filename1+len1);
      }

      if(debug)
      {
         printf("DEBUG> file/directory: [%s]\n", filename1);
         fflush(stdout);
      }


      if (stat(filename1, &buf) == 0) 
      {
         if (S_ISDIR(buf.st_mode) == 1)
         {
            // If directory, recurse
            
            if((strcmp(entry->d_name, "." ) != 0)
            && (strcmp(entry->d_name, "..") != 0))
            {
               if(debug)
               {
                  printf("DEBUG> Found directory [%s]\n", filename1);
                  fflush(stdout);
               }

               strcat(filename1, "/");

               if(mHiPSPNGs_getFiles (filename1) > 1)
                  return 1;
            }
         }

         else
         {
            // If regular file, process
            
            sprintf(pngfile, "%s%s", outdir, filename1+len1);

            pngfile[strlen(pngfile)-5] = '\0';

            strcat(pngfile, ".png");


            // If 'histfile1' is a directory, set the histogram file name
            // based on the FITS file name.
            
            if(histfile_is_dir)
            {
               sprintf(hist1, "%s%s", histfile1, entry->d_name);
               *(hist1+strlen(hist1)-5) = '\0';
               strcat(hist1, ".hist");

               if(color)
               {
                  sprintf(hist2, "%s%s", histfile2, entry->d_name);
                  *(hist2+strlen(hist2)-5) = '\0';
                  strcat(hist2, ".hist");

                  sprintf(hist3, "%s%s", histfile3, entry->d_name);
                  *(hist3+strlen(hist3)-5) = '\0';
                  strcat(hist3, ".hist");
               }
            }

            // Otherwise the histogram file is the argument given
            
            else
            {
               strcpy(hist1, histfile1);
               strcpy(hist2, histfile2);
               strcpy(hist3, histfile3);
            }
          

            // However, if we were given order and plate scale, assume
            // the "histogram file" is indeed a directory and that it 
            // contains plate histograms.
            
            /*
            if(order >= 0)
            {
               tileID = atoi(strstr(filename1, "Npix") + 4);

               mHiPSPNGs_transform(order, tileID, &xtile, &ytile);

               ntile = pow(2., (double)order) * 5.;

               iplate = xtile * nplate / ntile;
               jplate = ytile * nplate / ntile;

               if(histfile_is_dir)
               {
                  sprintf(hist1, "%splate_%02d_%02d.hist", histfile1, iplate, jplate);

                  if(color)
                  {
                     sprintf(hist2, "%splate_%02d_%02d.hist", histfile2, iplate, jplate);
                     sprintf(hist3, "%splate_%02d_%02d.hist", histfile3, iplate, jplate);
                  }
               }
            }

            if(order < 0 && color)
            {
               sprintf(hist2, "%s%s", histfile2, entry->d_name);
               *(hist2+strlen(hist2)-5) = '\0';
               strcat(hist2, ".hist");

               sprintf(hist3, "%s%s", histfile3, entry->d_name);
               *(hist3+strlen(hist3)-5) = '\0';
               strcat(hist3, ".hist");
            }
            */


            // We've got everything lined up but we really need to check if this
            // is a FITS file.

            if (strncmp(filename1+strlen(filename1)-5, ".fits", 5) == 0)
            {
               if(debug)
               {
                  printf("DEBUG> Found FITS file [%s]\n", filename1);
                  fflush(stdout);
               }

               if(debug)
               {
                  printf("DEBUG> directory1: [%s]\n", directory1);
                  printf("DEBUG> len1:        %d \n", len1);
                  printf("DEBUG> directory2: [%s]\n", directory2);
                  printf("DEBUG> directory3: [%s]\n", directory3);
                  printf("DEBUG> filename1:  [%s]\n", filename1);
                  printf("DEBUG> filename2:  [%s]\n", filename2);
                  printf("DEBUG> filename3:  [%s]\n", filename3);
                  printf("DEBUG> hist1:      [%s]\n", hist1);
                  printf("DEBUG> hist2:      [%s]\n", hist2);
                  printf("DEBUG> hist3:      [%s]\n", hist3);
                  printf("DEBUG> pngfile:    [%s]\n", pngfile);
                  fflush(stdout);
               }


               // Sometimes we only want to add a few files without redoing all the others.
               // Here we check if the output file already exists and skip it if it does.
               
               if(update_only)
               {
                  if(stat(pngfile, &buf) == 0) 
                     continue;
               }


               // Run mTranspose and mViewer
                
               if(color)
               {
                  strcpy(newdir, pngfile);

                  ptr = (char *)NULL;

                  for(i=0; i<strlen(newdir); ++i)
                  {
                     if(newdir[i] == '/')
                        ptr = newdir + i;
                  }

                  if(ptr == (char *)NULL)
                  {
                     printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs -d directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
                     exit(1);
                  }

                  *ptr = '\0';

                  if(debug)
                  {
                     printf("DEBUG> Recursive mkdir: [%s]\n", newdir);
                     fflush(stdout);
                  }

                  mHiPSPNGs_mkdir(newdir);

                  sprintf(transfile1, "%s_trans", filename1);
                  sprintf(transfile2, "%s_trans", filename2);
                  sprintf(transfile3, "%s_trans", filename3);

                  if(!no_transpose)
                  {
                     sprintf(cmd, "mTranspose %s %s 2 1", filename1, transfile1);

                     if(debug)
                     {
                        printf("DEBUG> Command: [%s]\n", cmd);
                        fflush(stdout);
                     }

                     svc_run(cmd);

                     sprintf(cmd, "mTranspose %s %s 2 1", filename2, transfile2);

                     if(debug)
                     {
                        printf("DEBUG> Command: [%s]\n", cmd);
                        fflush(stdout);
                     }

                     svc_run(cmd);

                     sprintf(cmd, "mTranspose %s %s 2 1", filename3, transfile3);

                     if(debug)
                     {
                        printf("DEBUG> Command: [%s]\n", cmd);
                        fflush(stdout);
                     }
                  }
                  else
                  {
                     strcpy(transfile1, filename1);
                     strcpy(transfile2, filename2);
                     strcpy(transfile3, filename3);
                  }

                  svc_run(cmd);

                  sprintf(cmd, "mViewer -bbrightness %-g -bcontrast %-g -gbrightness %-g -gcontrast %-g -rbrightness %-g -rcontrast %-g -blue %s -histfile %s -green %s -histfile %s -red %s -histfile %s -png %s",
                     bbrightness, bcontrast, gbrightness, gcontrast, rbrightness, rcontrast,
                     transfile1, hist1, transfile2, hist2, transfile3, hist3, pngfile);

                  if(debug)
                  {
                     printf("DEBUG> Command: [%s]\n", cmd);
                     fflush(stdout);
                  }

                  svc_run(cmd);

                  strcpy(status, svc_value("stat"));
                  
                  if(strcmp(status, "ERROR") == 0)
                     ++nerror;

                  svc_closeall();

                  if(!no_transpose)
                  {
                     unlink(transfile1);
                     unlink(transfile2);
                     unlink(transfile3);
                  }
               }

               else
               {
                  sprintf(transfile1, "%s_trans", filename1);

                  if(!no_transpose)
                  {
                     sprintf(cmd, "mTranspose %s %s 2 1", filename1, transfile1);

                     if(debug)
                     {
                        printf("DEBUG> Command: [%s]\n", cmd);
                        fflush(stdout);
                     }
                  }
                  else
                     strcpy(transfile1, filename1);

                  svc_run(cmd);

                  sprintf(cmd, "mViewer -ct %d -brightness %-g -contrast %-g -gray %s -histfile %s -png %s",
                     ct, brightness, contrast, transfile1, hist1, pngfile);

                  if(debug)
                  {
                     printf("DEBUG> Command: [%s]\n", cmd);
                     fflush(stdout);
                  }

                  svc_run(cmd);

                  strcpy(status, svc_value("stat"));
                  
                  if(strcmp(status, "ERROR") == 0)
                     ++nerror;

                  if(!no_transpose)
                     unlink(transfile1);
               }

               ++nimage;
            }
         }
      }
   }

   closedir(dp);
   return 0;
}



int mHiPSPNGs_mkdir(const char *path)
{
   const size_t len = strlen(path);

   char _path[32768];

   char *p; 

   errno = 0;

   if (len > sizeof(_path)-1) 
   {
      errno = ENAMETOOLONG;
      return -1; 
   }   

   strcpy(_path, path);

   /* Iterate the string */
   for (p = _path + 1; *p; p++) 
   {
      if (*p == '/') 
      {
         *p = '\0';

         if (mkdir(_path, S_IRWXU) != 0) 
         {
            if (errno != EEXIST)
               return -1; 
         }

         *p = '/';
      }
   }   

   if (mkdir(_path, S_IRWXU) != 0) 
   {
      if (errno != EEXIST)
         return -1; 
   }   

   return 0;
}


/*************************************************************************/
/*                                                                       */
/*  mHiPSPNGs_transform                                                  */
/*                                                                       */
/*  Given a HiPS tile order and tile ID within that order, determine     */
/*  the x, y HPX location for the tile.  See: util/mHPXtile2xy.c         */
/*                                                                       */
/*************************************************************************/

void mHiPSPNGs_transform(int order, int tile, int *xtile, int *ytile)
{
   int  baseTile, nside;
   long index;
   int  i, x, y;

   if(debug)
   {
      printf("\nDEBUG TILE> tile order   = %ld\n", order);
      printf("DEBUG TILE> tile         = %ld (input)\n", tile);
      fflush(stdout);
   }


   nside = pow(2., (double)order);

   if(debug)
   {
      printf("\nDEBUG TILE> nside(tile)  = %ld\n", nside);
      fflush(stdout);
   }


   baseTile = (int)tile/(nside*nside);

   if(debug)
   {
      printf("\nDEBUG TILE> Base tile    = %d (offset %d %d)\n",
         baseTile, xoffset[baseTile], yoffset[baseTile]);
      fflush(stdout);
   }


   index = tile - baseTile * nside * nside;

   if(debug)
   {
      printf("DEBUG TILE> Tile index   = %ld (i.e., index inside base tile)\n", index);
      fflush(stdout);
   }


   mHPXHiPSPNGs_splitIndex(index, order, &x, &y);

   x = nside - 1 - x;

   if(debug)
   {
      printf("\nDEBUG TILE> Relative tile: X = %7d, Y = %7d (of %d) [X was originally %d]\n", x, y, nside, nside - 1 - x);
      fflush(stdout);
   }


   x = x + xoffset[baseTile]*nside;
   y = y + yoffset[baseTile]*nside;

   if(debug)
   {
      printf("DEBUG TILE> Absolute:      X = %7d, Y = %7d (order %ld)\n\n", x, y, order);
      fflush(stdout);
   }

   *xtile = x;
   *ytile = y;

   return;
}


/***************************************************/
/*                                                 */
/* mHPXtile2xy_splitIndex()                        */
/*                                                 */
/* Cell indices are Z-order binary constructs.     */
/* The x and y pixel offsets are constructed by    */
/* extracting the pattern made by every other bit  */
/* to make new binary numbers.                     */
/*                                                 */
/***************************************************/

void mHPXHiPSPNGs_splitIndex(long index, int level, int *x, int *y)
{
   int  i;
   long val;

   val = index;

   *x = 0;
   *y = 0;

   for(i=0; i<level; ++i)
   {
      *x = *x + (((val >> (2*i))   & 1) << i);
      *y = *y + (((val >> (2*i+1)) & 1) << i);
   }

   return;
}
