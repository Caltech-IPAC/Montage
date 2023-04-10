#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <svc.h>

#define STRLEN 1024

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int runScript(char *cmd);

int  debug;


/*************************************************************************/
/*                                                                       */
/*  mHiPSExec                                                            */
/*                                                                       */
/*  Montage supports a lot of variation in making HiPS maps, mainly to   */
/*  account for the wide variety of background issues in astronomical    */
/*  images.  But a lot of datasets fall into a basic pattern, where the  */
/*  backgrounds are matched based on minimizing the image-to-image       */
/*  overlap differences or not modified at all.                          */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int  i, ch, order, nside, nplate;

   int  project_only;
   int  level_only;
   int  scripts_only;

   char dataset [STRLEN];
   char rawdir  [STRLEN];
   char survey  [STRLEN];
   char band    [STRLEN];
   char workdir [STRLEN];
   char tmpdir  [STRLEN];
   char flags   [STRLEN];
   char cmd     [STRLEN];
   char cwd     [STRLEN];
   char tmpstr  [STRLEN];
   char line    [STRLEN];
   char outline [STRLEN];
   char orderstr[STRLEN];

   FILE *ftemplate;
   FILE *findex;

   struct stat buf;

   getcwd(cwd, STRLEN);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   strcpy(dataset, "");
   strcpy(rawdir,  "");
   strcpy(survey,  "");
   strcpy(band,    "");
   strcpy(workdir, "");

   if (argc < 4) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSExec [-p(roject-only)][-s(cripts-only)] dataset workdir survey/band | datadir\"]\n");
      exit(1);
   }

   debug = 0;

   project_only = 0;
   level_only   = 0;
   scripts_only = 0;

   while ((ch = getopt(argc, argv, "dps")) != EOF)                                
   {                                                                            
      switch (ch)                                                               
      {                                                                         
         case 'd':                                                              
            debug = 1;                                                          
            break;                                                              
                                                                                
         case 'l':                                                              
            level_only = 1;                                                          
            break;                                                              
                                                                                
         case 'p':                                                              
            project_only = 1;                                                          
            break;                                                              
                                                                                
         case 's':                                                              
            scripts_only = 1;                                                          
            break;                                                              
                                                                                
         default:                                                               
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSExec [-p(roject-only)][-s(cripts-only)] dataset_name workdir survey/band | datadir\"]\n");
            fflush(stdout);                                                     
            exit(1);                                                            
      }                                                                         
   }                                                                            

   if (argc-optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSExec [-p(roject-only)][-s(cripts-only)] dataset_name workdir survey/band | datadir\"]\n");
      exit(1);
   }
                                                                                
   strcpy(dataset, argv[optind]);                                            
   strcpy(workdir, argv[optind + 1]);                                        
   
   if(argc-optind <= 3)
      strcpy(rawdir,  argv[optind + 2]);                                        
   else
   {
      strcpy(survey,  argv[optind + 2]);                                        
      strcpy(band,    argv[optind + 3]);                                        
   }

   if(strlen(rawdir) > 0 && rawdir[0] != '/')                                                      
   {                                                                            
      strcpy(tmpdir, cwd);                                                      
      strcat(tmpdir, "/");                                                      
      strcat(tmpdir, rawdir);                                                

      strcpy(rawdir, tmpdir);                                                
   }                                         

   if(workdir[0] != '/')                                                      
   {                                                                            
      strcpy(tmpdir, cwd);                                                      
      strcat(tmpdir, "/");                                                      
      strcat(tmpdir, workdir);                                                

      strcpy(workdir, tmpdir);                                                
   }                                         

   if(debug)
   {
      printf("DEBUG> level_only   = %d\n", level_only);
      printf("DEBUG> project_only = %d\n", project_only);
      printf("DEBUG> scripts_only = %d\n", scripts_only);
      printf("\n");

      printf("DEBUG> dataset: [%s]\n", dataset);
      printf("DEBUG> workdir: [%s]\n", workdir);
      printf("DEBUG> rawdir:  [%s]\n", rawdir);
      printf("DEBUG> survey:  [%s]\n", survey);
      printf("DEBUG> band:    [%s]\n", band);
      printf("\n");
      fflush(stdout);
   }


   // Create the working directory (error if it already exists)
   
   if(stat(workdir, &buf) == 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Workspace directory already exists.\"]\n");
      exit(1);
   }


   // Configure the working directory

   sprintf(cmd, "mHiPSSetup -s %s", workdir);
   runCmd(cmd);


   if(strlen(rawdir) > 0)
   {
      // Generate image metadata table for the raw data directory

      sprintf(cmd, "mImgtbl %s %s/images.tbl", rawdir, workdir);
      runCmd(cmd);


      // Based on the pixel scale in the metadata, determine the HPX order
      // and chose the tiling plate number

      sprintf(cmd, "mHPXOrder %s/images.tbl", workdir);
      runCmd(cmd);

      order = atoi(svc_value("order"));

      sprintf(orderstr, "%d", order);

      nside = atoi(svc_value("nside"));

      nplate = 5;

      if(nside >  32768) nplate = 10;
      if(nside > 131072) nplate = 20;
   }
   else
   {
      order  = 9;
      nplate = 20;
   }

   // Make a list of the plates

   sprintf(cmd, "mHPXPlateList -n %d %d %s/platelist.tbl", nplate, order, workdir);
   runCmd(cmd);


   // Now start processing data.  First generate and run the plate
   // mosaicking scripts

   strcpy(flags, "-s");

   if(level_only)
      strcat(flags, " -l");

   if(project_only)
      strcat(flags, " -p");

   if(strlen(rawdir) > 0)
      sprintf(cmd, "mHPXMosaicScripts %s %s/scripts %s/plates/order%d %s/platelist.tbl %s", 
            flags, workdir, workdir, order, workdir, rawdir);
   else
      sprintf(cmd, "mHPXMosaicScripts %s %s/scripts %s/plates/order%d %s/platelist.tbl %s %s", 
            flags, workdir, workdir, order, workdir, survey, band);

   runCmd(cmd);

   if(!scripts_only)
   {
      sprintf(cmd, "%s/scripts/mosaicSubmit.sh", workdir);
      runScript(cmd);
   }


   // Then recursively shrink the plates to get the lower orders

   sprintf(cmd, "mHPXShrinkScripts -s %d %s/scripts %s/plates %s/platelist.tbl", 
         order, workdir, workdir, workdir);
   runCmd(cmd);

   if(!scripts_only)
   {
      sprintf(cmd, "%s/scripts/shrinkSubmit.sh", workdir);
      runScript(cmd);
   }


   // Chop the plates up into HiPS tiles

   sprintf(cmd, "mHiPSTileScripts -s %d 0 %s/scripts %s/plates %s/tiles %s/platelist.tbl", 
         order, workdir, workdir, workdir, workdir);
   runCmd(cmd);

   if(!scripts_only)
   {
      sprintf(cmd, "%s/scripts/tileSubmit.sh", workdir);
      runScript(cmd);
   }


   // Before we can generate PNGS of the tiles, we need the color stretch

   if(!scripts_only)
   {
      sprintf(cmd, "mHPXAllSky %s/tiles", workdir);
      runCmd(cmd);

      sprintf(cmd, "mHistogram -file %s/tiles/Norder3/Allsky.fits min max gaussian-log -out %s/allsky.hist",
            workdir, workdir);
      runCmd(cmd);
   }


   // Now make the PNG versions of the tiles
   
   sprintf(cmd, "mHiPSPNGScripts -s %s/scripts %s/tiles %s/allsky.hist %s/pngs", 
         workdir, workdir, workdir, workdir);
   runCmd(cmd);

   if(!scripts_only)
   {
      sprintf(cmd, "%s/scripts/pngSubmit.sh", workdir);
      runScript(cmd);
   }


   // We need a PNG version of the all-sky image.  For speed, we will 
   // shrink this by a factor of two first
   
   if(!scripts_only)
   {
      sprintf(cmd, "mShrink %s/tiles/Norder3/Allsky.fits %s/tiles/Norder3/Allsky_small.fits 2", 
            workdir, workdir);
      runCmd(cmd);

      sprintf(cmd, "mViewer -nowcs -noflip -brightness 0 -contrast 0 -gray %s/tiles/Norder3/Allsky_small.fits -histfile %s/allsky.hist -png %s/pngs/Norder3/Allsky.png",
            workdir, workdir, workdir);
      runCmd(cmd);
   }


   // Finally, we need to set up the HTML / Javascript / CSS 
   // for serving the data.  The index.html file needs to updated
   // with the dataset name and the max order.

   if(!scripts_only)
   {
      sprintf(cmd, "wget -O %s/pngs/index_template.html     \"http://montage.ipac.caltech.edu/data/HiPS/index_template.html\"",     workdir);
      runScript(cmd);

      sprintf(cmd, "wget -O %s/pngs/js/jquery-1.12.1.min.js \"http://montage.ipac.caltech.edu/data/HiPS/js/jquery-1.12.1.min.js\"", workdir);
      runScript(cmd);

      sprintf(cmd, "wget -O %s/pngs/js/aladin.js            \"http://montage.ipac.caltech.edu/data/HiPS/js/aladin.js\"",            workdir);
      runScript(cmd);

      sprintf(cmd, "wget -O %s/pngs/js/aladin.min.js        \"http://montage.ipac.caltech.edu/data/HiPS/js/aladin.min.js\"",        workdir);
      runScript(cmd);

      sprintf(cmd, "wget -O %s/pngs/css/aladin.min.css      \"http://montage.ipac.caltech.edu/data/HiPS/css/aladin.min.css\"",      workdir);
      runScript(cmd);

      sprintf(cmd, "%s/pngs/index_template.html", workdir);

      ftemplate = fopen(cmd, "r");

      sprintf(cmd, "%s/pngs/index_template.html", workdir);

      findex = fopen(cmd, "w+");

      strcpy(outline, "");

      while(1)
      {
         if(fgets(line, STRLEN, ftemplate) == (char *)NULL)
            break;

         if(line[strlen(line)-1] == '\n'
         || line[strlen(line)-1] == '\r')
            line[strlen(line)-1] == '\0';
         {
            if(strncmp(line + i, "datasetname", 11) == 0)
            {
               strcat(outline, dataset);
               i += 11;
            }

            else if(strncmp(line + i, "order", 6) == 0)
            {
               strcat(outline, orderstr);
               i += 6;
            }

            else
            {
               strcpy(tmpstr, "");

               tmpstr[0] = *(line + i);
               tmpstr[1] = '\0';

               strcat(outline, tmpstr);
            }

            fprintf(findex, "%s\n", outline);
         }
      }

      fclose(ftemplate);
      fclose(findex);
   }
      

   printf("[struct stat=\"OK\", module=\"mHiPSExec\"]\n");
   fflush(stdout);
   exit(0);
}


int runCmd(char *cmd)
{
   char status[STRLEN];

   if(debug)
   {
      printf("\nCMD:    [%s]\n", cmd);                                             
      fflush(stdout);                                                              
   }
                                                                                
   svc_run(cmd);                                                                
                                                                                
   if(debug)
   {
      printf("RETURN: %s\n", svc_value((char *)NULL));                                    
      fflush(stdout);                                                              
   }
                                                                                
   strcpy(status, svc_value("stat"));                                           
                                                                                
   if(strcmp(status, "ERROR") == 0)                                             
      exit(0);                     

   return 0;
}


int runScript(char *cmd)
{
   int  svc;
   char retval[1024];

   if(debug)
   {
      printf("\nCMD:    [%s]\n", cmd);                                             
      fflush(stdout);                                                              
   }

   svc = svc_init(cmd);

   while(1)
   {
      strcpy(retval, svc_receive(svc));

      if(strncmp(retval, "[struct stat=\"ABORT\"", 20) == 0)
         break;

      if(debug)
      {
         printf("RETURN: %s\n", retval);
         fflush(stdout);
      }
   }

   svc_close(svc);

   return 0;
}

