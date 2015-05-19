#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include <www.h>
#include <mtbl.h>
#include <password.h>
#include <config.h>
#include <svc.h>

#define MAXSTR 32768
#define STRLEN 4096

#define APPNAME "mViewer"

#define DEMO         1
#define UPLOAD       2
#define SEARCHRESULT 3
#define EXTERNAL     4

void printError(char *errmsg);
void link_file (char *fname, char *set, char *directory);

int debug = 0;

FILE *fdebug;


/******************************************************************/
/*                                                                */
/*  The service just sets up a workspace for an mViewer instance, */
/*  populates it in various ways (demo data / user upload / data  */
/*  from another service) and points mViewer at it.               */
/*                                                                */
/******************************************************************/


int main(int argc, char *argv[], char *envp[])
{
   int   nkey, pid, mode;

   char  tmpstr     [MAXSTR];
   char  cmd        [STRLEN];
   char  wspace     [STRLEN];
   char  directory  [STRLEN];
   char  workDir    [STRLEN];
   char  userDir    [STRLEN];
   char  workspace  [STRLEN];
   char  template   [STRLEN];
   char  indexfile  [STRLEN];
   char  form       [STRLEN];
   char  set        [STRLEN];
   char  inwork     [STRLEN];
   char  modeStr    [STRLEN];


   char *cookie;

   char  status     [32];

   FILE  *fp;


   /* Various time value variables */

   char    buffer[256];

   int     yr, mo, day, hr, min, sec;

   time_t     curtime;
   struct tm *loctime;


  /*********************************************************/
   /* Get the current time and convert to a datetime string */
   /*********************************************************/

   curtime = time (NULL);
   loctime = localtime (&curtime);

   strftime(buffer, 256, "%Y", loctime);
   yr = atoi(buffer);

   strftime(buffer, 256, "%m", loctime);
   mo = atoi(buffer);

   strftime(buffer, 256, "%d", loctime);
   day = atoi(buffer);

   strftime(buffer, 256, "%H", loctime);
   hr = atoi(buffer);

   strftime(buffer, 256, "%M", loctime);
   min = atoi(buffer);

   strftime(buffer, 256, "%S", loctime);
   sec = atoi(buffer);

   pid = getpid();

   sprintf(wspace, "%04d.%02d.%02d_%02d.%02d.%02d_%06d",
       yr, mo, day, hr, min, sec, pid);


   /********************/
   /* Config variables */
   /********************/

   config_init((char *)NULL);

   if(config_exists("ISIS_WORKDIR"))
      strcpy(workDir, config_value("ISIS_WORKDIR"));
   else
      printError("No workspace directory.");


   /************/
   /* Keywords */
   /************/

   nkey = keyword_init(argc, argv);

   if(nkey <= 0)
      printError("No keywords found.");

   if(!debug && keyword_exists("debug"))
      debug = atoi(keyword_value("debug"));

   fdebug = stdout;

   if(debug > 1)
   {
      sprintf (tmpstr, "/tmp/mViewerInit.debug_%d", pid);

      fdebug = fopen (tmpstr, "w+");
   }
 
   if(debug)
      svc_debug(fdebug);

   strcpy(modeStr, "demo");

   if(keyword_exists("mode"))
      strcpy(modeStr, keyword_value("mode"));

        if(strcmp(modeStr, "demo"         ) == 0) mode = DEMO;
   else if(strcmp(modeStr, "upload"       ) == 0) mode = UPLOAD;
   else if(strcmp(modeStr, "searchresults") == 0) mode = SEARCHRESULT;
   else if(strcmp(modeStr, "external"     ) == 0) mode = EXTERNAL;
   else
      printError("Invalid processing mode.");

   strcpy(form, "template");
   if(keyword_exists("form"))
      strcpy(form, keyword_value("form"));

   strcpy(set, "m51");
   if(keyword_exists("set"))
      strcpy(set, keyword_value("set"));

   strcpy(inwork, "");
   if(keyword_exists("workspace"))
      strcpy(inwork, keyword_value("workspace"));
   if(keyword_exists("ws"))
      strcpy(inwork, keyword_value("ws"));

   if(debug)
   {
      fprintf(fdebug, "<H2>Config parameters:</H2>");

      fprintf(fdebug, "<pre>\n");
      fprintf(fdebug, "DEBUG> MY_DATA_DIR  = [%s]\n", MY_DATA_DIR);
      fprintf(fdebug, "DEBUG> ISIS_WORKDIR = [%s]\n", workDir);
      fprintf(fdebug, "DEBUG> mode         =  %d \n", mode);
      fprintf(fdebug, "DEBUG> set          = [%s]\n", set);
      fprintf(fdebug, "DEBUG> inwork       = [%s]\n", inwork);
      fprintf(fdebug, "</pre><hr/>\n");
      fflush(fdebug);
   }


   if(mode == EXTERNAL)
   {
      /************************************************************/
      /* We will work in the space the driver application created */
      /************************************************************/

      strcpy(workspace, inwork);

      strcpy(directory, workDir);
      strcat(directory, "/");
      strcat(directory, workspace);
   }
   else
   {
      /***********************/
      /* Get the ISIS cookie */
      /***********************/

      cookie = cgiworkspace();


      /*********************************************************************/
      /* Create a workspace directory (and associated JobID subdirectory). */
      /*********************************************************************/

      strcpy(userDir, cookie);

      strcpy(directory, workDir);
      strcat(directory, "/");
      strcat(directory, userDir);

      strcpy(workspace, userDir);

      if(mkdir(directory, 0775) < 0)
      {
         if(errno != EEXIST)
            printError("Cannot create user workspace subdirectory.");
      }

      strcat(directory, "/");
      strcat(directory, APPNAME);
      strcat(workspace, "/");
      strcat(workspace, APPNAME);

      if(mkdir(directory, 0775) < 0)
      {
         if(errno != EEXIST)
         {
            sprintf(tmpstr, "Cannot create %s workspace subdirectory.", APPNAME);
            printError(tmpstr);
         }
      }

      strcat(directory, "/");
      strcat(directory, wspace);
      strcat(workspace, "/");
      strcat(workspace, wspace);

      if(mkdir(directory, 0775) < 0)
      {
         if(errno != EEXIST)
            printError("Cannot create JobID workspace subdirectory.");
      }

      if(debug)
      {
         fprintf(fdebug, "<pre>\n");
         fprintf(fdebug, "wspace    = %s\n", wspace);
         fprintf(fdebug, "userDir   = %s\n", userDir);
         fprintf(fdebug, "directory = %s\n", directory);
         fprintf(fdebug, "workspace = %s\n", workspace);
         fprintf(fdebug, "</pre><hr/>\n");
      }
   }


   /***********************************************************/
   /* There are three modes of this program:                  */
   /*                                                         */
   /*    A specific "set" (DEMO data),                        */
   /*                                                         */
   /*    A a set of uploaded files (UPLOAD), or               */
   /*                                                         */
   /*    A reference to a workspace that contains params.json */
   /*    and associated files made by another application     */
   /*    (EXTERNAL).                                          */
   /*                                                         */
   /***********************************************************/

   switch(mode)
   {
      // One of the demo datasets

      case DEMO:
         
         /* Copy/link M51 demo data */

         if(strcmp(set, "m51_small") == 0)
         {
            link_file("viewer.json",       set, directory);
            link_file("sdss_g_small.fits", set, directory);
         }

         else if(strcmp(set, "m51") == 0)
         {
            link_file("viewer.json", set, directory);
            link_file("sdss_g.fits", set, directory);
         }

         else if(strcmp(set, "m51_color") == 0)
         {
            link_file("viewer.json",    set, directory);
            link_file("sdss_u2.fits",   set, directory);
            link_file("sdss_g2.fits",   set, directory);
            link_file("sdss_r2.fits",   set, directory);
            link_file("iracmap.tbl",    set, directory);
            link_file("iracmap_pc.tbl", set, directory);
            link_file("irsmap.tbl",     set, directory);
            link_file("irspeakup.tbl",  set, directory);
            link_file("irsstare.tbl",   set, directory);
            link_file("mipssed.tbl",    set, directory);
            link_file("mipsscan.tbl",   set, directory);
            link_file("fp_2mass.tbl",   set, directory);
         }

         else if(strcmp(set, "m51_color_small") == 0)
         {
            link_file("viewer.json",       set, directory);
            link_file("sdss_u_small.fits", set, directory);
            link_file("sdss_g_small.fits", set, directory);
            link_file("sdss_r_small.fits", set, directory);
            link_file("iracmap.tbl",       set, directory);
            link_file("iracmap_pc.tbl",    set, directory);
            link_file("irsmap.tbl",        set, directory);
            link_file("irspeakup.tbl",     set, directory);
            link_file("irsstare.tbl",      set, directory);
            link_file("mipssed.tbl",       set, directory);
            link_file("mipsscan.tbl",      set, directory);
            link_file("fp_2mass.tbl",      set, directory);
         }

         else if(strcmp(set, "spitzer") == 0)
         {
            link_file("viewer.json",        set, directory);
            link_file("spitzer_north.fits", set, directory);
            link_file("iracmap.tbl",        set, directory);
            link_file("iracmap_pc.tbl",     set, directory);
            link_file("irsmap.tbl",         set, directory);
            link_file("irspeakup.tbl",      set, directory);
            link_file("irsstare.tbl",       set, directory);
            link_file("mipssed.tbl",        set, directory);
            link_file("mipsscan.tbl",       set, directory);
            link_file("fp_2mass.tbl",       set, directory);
         }

         else if(strcmp(set, "FinderChart") == 0)
         {
            link_file("viewer.json",             set, directory);
            link_file("fc_m51_dssdss2blue.fits", set, directory);
         }

         else
            printError("Invalid demo set name.");

      break;


      // Process the input into a table file with column datatypes

      case UPLOAD:
         
      break;


      // Data was properly generated by another application;
      // We don't move anything around.
      
      case EXTERNAL:

         // We could check for the right files here
         // but since mViewer will do that too, 
         // we won't bother.

      break;


      default:
         printError("Invalid processing mode");
      break;
   }


   /************************************************/
   /* Create the index.html file from the template */
   /************************************************/

   strcpy(template, MY_DATA_DIR "/");

   strcat(template, form);
   strcat(template, ".html");

   sprintf(indexfile, "%s/mViewer.html", directory);

   sprintf(cmd, "htmlgen %s %s workspace %s json \"%s\"",
      template, indexfile, workspace, "viewer.json");

   svc_run(cmd);

   strcpy(status, svc_value( "stat" ));

   if(strcmp( status, "ERROR") == 0)
   {
      strcpy(tmpstr, svc_value( "msg" ));
      printError(tmpstr);
   }


   /**************************************************/
   /* Copy the generated index.html file to the user */
   /**************************************************/

   fp = fopen(indexfile, "r");

   if(fp == (FILE *)NULL)
      printError("Cannot open mViewer.html file.");

   if(debug)
   {
      fprintf(fdebug, "<hr/>\n<H2>Result Page:</H2>");

      fprintf(fdebug, "<pre>\n");
   }

   if(debug != 1)
   {
      printf("HTTP/1.1 200 OK\r\n");
      printf("Content-type: text/html\r\n\r\n");
      fflush(stdout);
   }

   while(1)
   {
      if(fgets(tmpstr, MAXSTR, fp) == (char *)NULL)
         break;

      fputs(tmpstr, stdout);
      fflush(stdout);
   }

   fclose(fp);

   if(debug)
      fprintf(fdebug, "</pre><hr/>\n");

   fflush(fdebug);
   exit(0);
}



/**********************/
/* HTML Error message */
/**********************/

void printError(char *errmsg)
{
    printf("HTTP/1.1 200 OK\n");
    printf("Content-type: text/html\r\n");
    printf ("\r\n");

    printf ("%s\n", errmsg);
    fflush (fdebug);

   exit(0);
}



/****************************************/
/* Sym link demo files into a workspace */
/****************************************/

void link_file(char *fname, char *set, char *directory)
{
   char from[STRLEN];
   char to  [STRLEN];

   sprintf(from, "%s/%s/%s", MY_DATA_DIR, set, fname);
   sprintf(to,   "%s/%s", directory,   fname);

   symlink(from, to);

   return;
}
