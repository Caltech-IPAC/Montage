#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include <www.h>
#include <password.h>
#include <config.h>
#include <svc.h>
#include <json.h>

#define MAXSTR 32768
#define STRLEN 4096

#define APPNAME "mViewer"

void processLocation(char *location, char *ra, char *dec);

void printError(char *errmsg);

int debug = 0;

FILE *fdebug;


/*************************************************************************/
/*                                                                       */
/*  The service receives an image display specification (JSON structure) */
/*  and uses it to generate a JPEG/PNG image of the data.  Really, all   */
/*  it does is run the mViewer program and manage the result file.       */
/*                                                                       */
/*************************************************************************/


int main(int argc, char **argv)
{
   int  i, istat, offset, nkey, pid;
   int  ixmin, ixmax, iymin, iymax;
   int  ixsize, iysize;

   double canvasWidth, canvasHeight;
   double xmin,        xmax;
   double ymin,        ymax;
   double imageWidth,  imageHeight;
   double xfactor,     yfactor;
   double xsize,       ysize;
   double xcenter,     ycenter;
   double canvasRatio;
   double regionRatio;
   double tmp;

   double wscale, wrotation;
   double wra, wdec, wxsize, wysize;

   char cmd            [STRLEN];
   char subcmd         [STRLEN];
   char tmpstr         [STRLEN];
   char jsonStr        [MAXSTR];
   char jsonOrig       [MAXSTR];
   char workDir        [STRLEN];
   char directory      [STRLEN];
   char baseURL        [STRLEN];
   char jsonFile       [STRLEN];

   char elementName    [STRLEN];

   char workspace      [STRLEN];
   char imageFile      [STRLEN];
   char imageType      [STRLEN];
   char canvasWidthStr [STRLEN];
   char canvasHeightStr[STRLEN];

   char xminStr        [STRLEN];
   char xmaxStr        [STRLEN];
   char yminStr        [STRLEN];
   char ymaxStr        [STRLEN];

   char grayFile       [STRLEN];
   char colorTable     [STRLEN];
   char stretchMin     [STRLEN];
   char stretchMax     [STRLEN];
   char stretchMode    [STRLEN];

   char blueFile       [STRLEN];
   char blueMin        [STRLEN];
   char blueMax        [STRLEN];
   char blueMode       [STRLEN];
 
   char greenFile      [STRLEN];
   char greenMin       [STRLEN];
   char greenMax       [STRLEN];
   char greenMode      [STRLEN];

   char redFile        [STRLEN];
   char redMin         [STRLEN];
   char redMax         [STRLEN];
   char redMode        [STRLEN];

   char min            [STRLEN];
   char minpercent     [STRLEN];
   char minsigma       [STRLEN];
   char max            [STRLEN];
   char maxpercent     [STRLEN];
   char maxsigma       [STRLEN];
   char datamin        [STRLEN];
   char datamax        [STRLEN];

   char bmin           [STRLEN];
   char bminpercent    [STRLEN];
   char bminsigma      [STRLEN];
   char bmax           [STRLEN];
   char bmaxpercent    [STRLEN];
   char bmaxsigma      [STRLEN];
   char bdatamin       [STRLEN];
   char bdatamax       [STRLEN];

   char gmin           [STRLEN];
   char gminpercent    [STRLEN];
   char gminsigma      [STRLEN];
   char gmax           [STRLEN];
   char gmaxpercent    [STRLEN];
   char gmaxsigma      [STRLEN];
   char gdatamin       [STRLEN];
   char gdatamax       [STRLEN];

   char rmin           [STRLEN];
   char rminpercent    [STRLEN];
   char rminsigma      [STRLEN];
   char rmax           [STRLEN];
   char rmaxpercent    [STRLEN];
   char rmaxsigma      [STRLEN];
   char rdatamin       [STRLEN];
   char rdatamax       [STRLEN];

   char xflip          [STRLEN];
   char yflip          [STRLEN];
   char bunit          [STRLEN];
   char wwturl         [STRLEN];
   char colortable     [STRLEN];

   char cra            [STRLEN];
   char cdec           [STRLEN];

   char status         [32];

   char wfile          [STRLEN];

   struct 
   {
      char type        [STRLEN];
      char coordSys    [STRLEN]; 
      char color       [STRLEN];
      char dataFile    [STRLEN];
      char dataCol     [STRLEN];
      char dataType    [STRLEN];
      char dataRef     [STRLEN];
      char symType     [STRLEN];
      char symSize     [STRLEN];
      char location    [STRLEN];
      char text        [STRLEN];
      char visible     [STRLEN];
   }
   overlay[50];

   int  nOverlay = 0;
   
   FILE *fp;


   /********************/
   /* Config variables */
   /********************/

   config_init((char *)NULL);

   if(config_exists("ISIS_WORKDIR"))
      strcpy(workDir, config_value("ISIS_WORKDIR"));
   else
      printError("No workspace directory.");


   if(config_exists("ISIS_WORKURL"))
      strcpy(baseURL, config_value("ISIS_WORKURL"));
   else
      printError("No workspace URL.");

   pid = getpid();



   /****************************************/
   /* Keywords (workspace and JSON string) */
   /****************************************/

   nkey = keyword_init(argc, argv);

   if(nkey <= 0)
      printError("No keywords found.");

   if(!debug && keyword_exists("debug"))
      debug = atoi(keyword_value("debug"));

   fdebug = stdout;

   if(debug > 1)
   {
      sprintf (tmpstr, "/tmp/mViewer.debug_%d", pid);

      fdebug = fopen (tmpstr, "w+");
   }
 
   if(debug)
      svc_debug(fdebug);

   if(keyword_exists("json"))
      strcpy(jsonStr, keyword_value("json"));
   else
      printError("No JSON structure.");

   strcpy(jsonOrig, jsonStr);

   if(keyword_exists("workspace"))
      strcpy(workspace, keyword_value("workspace"));
   else
      printError("No workspace specified.");

   if(debug)
   {
      fprintf(fdebug, "Parameters:");

      fprintf(fdebug, "DEBUG> ISIS_WORKDIR = [%s]\n", workDir);
      fprintf(fdebug, "DEBUG> ISIS_WORKURL = [%s]\n", baseURL);
      fprintf(fdebug, "DEBUG> workspace    = [%s]\n", workspace);
      fprintf(fdebug, "DEBUG> jsonStr      = [%s]\n", jsonStr);
      fflush(fdebug);
   }


   /**********************/
   /* Find the workspace */
   /**********************/

   strcpy(directory, workDir);
   strcat(directory, "/");
   strcat(directory, workspace);

   strcat(baseURL,   "/");
   strcat(baseURL,   workspace);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> directory    = %s\n", directory);
      fprintf(fdebug, "DEBUG> baseURL      = %s\n", baseURL);
      fflush(fdebug);
   }

   istat = chdir(directory);

   if(istat != 0)
      printError("Cannot chdir to work directory.");


   /******************************************************/
   /* Compress any newline/carriage return characters    */
   /* out of the JSON (our parser below is very simple). */
   /******************************************************/

   offset = 0;

   for(i=0; i<strlen(jsonStr); ++i)
   {
      if(jsonStr[i] == '\r' 
      || jsonStr[i] == '\n')
         ++offset;

      else
         jsonStr[i-offset] = jsonStr[i];
   }

   jsonStr[i] = '\0';


   /******************/
   /* Parse the JSON */
   /******************/

   strcpy( grayFile, "");
   strcpy(  redFile, "");
   strcpy(greenFile, "");
   strcpy( blueFile, "");


   if(json_val(jsonStr, "imageFile",    imageFile)       == (char *)NULL) printError("No image file name given.");
   if(json_val(jsonStr, "imageType",    imageType)       == (char *)NULL) printError("No image file type given.");
   if(json_val(jsonStr, "canvasWidth",  canvasWidthStr)  == (char *)NULL) printError("No canvas width given.");
   if(json_val(jsonStr, "canvasHeight", canvasHeightStr) == (char *)NULL) printError("No canvas height given.");

   xmin = 0.;
   xmax = 0.;
   ymin = 0.;
   ymax = 0.;

   if(json_val(jsonStr, "xmin", xminStr) != (char *)NULL) xmin = atof(xminStr);
   if(json_val(jsonStr, "xmax", xmaxStr) != (char *)NULL) xmax = atof(xmaxStr);
   if(json_val(jsonStr, "ymin", yminStr) != (char *)NULL) ymin = atof(yminStr);
   if(json_val(jsonStr, "ymax", ymaxStr) != (char *)NULL) ymax = atof(ymaxStr);

   if(xmin > xmax)
   {
      tmp = xmin;
      xmin = xmax;
      xmax = tmp;
   }

   if(ymin > ymax)
   {
      tmp = ymin;
      ymin = ymax;
      ymax = tmp;
   }


   if(json_val(jsonStr, "grayFile.fitsFile",  grayFile) != (char *)NULL)
   {
      if(json_val(jsonStr, "grayFile.colorTable",  colorTable)   == (char *)NULL) strcpy(colorTable,  "1");
      if(json_val(jsonStr, "grayFile.stretchMin",  stretchMin)   == (char *)NULL) strcpy(stretchMin,  "-2s");
      if(json_val(jsonStr, "grayFile.stretchMax",  stretchMax)   == (char *)NULL) strcpy(stretchMax,  "max");
      if(json_val(jsonStr, "grayFile.stretchMode", stretchMode)  == (char *)NULL) strcpy(stretchMode, "gaussian-log");
   }
   else
   {
      if(json_val(jsonStr, "redFile.fitsFile",   redFile  ) == (char *)NULL
      || json_val(jsonStr, "blueFile.fitsFile",  blueFile ) == (char *)NULL)
         printError("Need either single FITS file for grayscale/pseudocolor or red/blue files or or red/green/blue files for full color.");

      if(json_val(jsonStr, "redFile.stretchMin",    redMin)    == (char *)NULL) strcpy(redMin,    "-2s");
      if(json_val(jsonStr, "redFile.stretchMax",    redMax)    == (char *)NULL) strcpy(redMax,    "max");
      if(json_val(jsonStr, "redFile.stretchMode",   redMode)   == (char *)NULL) strcpy(redMode,   "gaussian-log");

      json_val(jsonStr, "greenFile.fitsFile", greenFile );
      
      if(json_val(jsonStr, "greenFile.stretchMin",  greenMin)  == (char *)NULL) strcpy(greenMin,  "-2s");
      if(json_val(jsonStr, "greenFile.stretchMax",  greenMax)  == (char *)NULL) strcpy(greenMax,  "max");
      if(json_val(jsonStr, "greenMode", greenMode) == (char *)NULL) strcpy(greenMode, "gaussian-log");
      
      if(json_val(jsonStr, "blueFile.stretchMin",   blueMin)   == (char *)NULL) strcpy(blueMin,   "-2s");
      if(json_val(jsonStr, "blueFile.stretchMax",   blueMax)   == (char *)NULL) strcpy(blueMax,   "max");
      if(json_val(jsonStr, "blueFile.stretchMode",  blueMode)  == (char *)NULL) strcpy(blueMode,  "gaussian-log");
   }

   i = 0;

   while(1)
   {
      strcpy(overlay[i].coordSys, ""); 
      strcpy(overlay[i].color,    "");
      strcpy(overlay[i].dataFile, "");
      strcpy(overlay[i].dataCol,  "");
      strcpy(overlay[i].dataRef,  "");
      strcpy(overlay[i].dataType, "");
      strcpy(overlay[i].symType,  "");
      strcpy(overlay[i].symSize,  "");
      strcpy(overlay[i].location, "");
      strcpy(overlay[i].text,     "");

      sprintf(elementName, "overlay[%d]", i);

      if(debug)
      {
         fprintf(fdebug, "overlay[%d]:\n", i);
         fflush(fdebug);
      }

      if(json_val(jsonStr, elementName, tmpstr) == (char *)NULL) 
      {
         if(debug)
         {
            fprintf(fdebug, "Not found, exiting loop.\n");
            fflush(fdebug);
         }

         break;
      }

      sprintf(elementName, "overlay[%d].type", i);

      if(json_val(jsonStr, elementName, overlay[i].type) == (char *)NULL) 
         printError("No type given for overlay.");

      if(debug)
      {
         fprintf(fdebug, "  type:     [%s]\n", overlay[i].type);
         fflush(fdebug);
      }

      sprintf(elementName, "overlay[%d].coordSys", i); json_val(jsonStr, elementName, overlay[i].coordSys); 
      sprintf(elementName, "overlay[%d].color",    i); json_val(jsonStr, elementName, overlay[i].color);
      sprintf(elementName, "overlay[%d].dataFile", i); json_val(jsonStr, elementName, overlay[i].dataFile);
      sprintf(elementName, "overlay[%d].dataCol",  i); json_val(jsonStr, elementName, overlay[i].dataCol);
      sprintf(elementName, "overlay[%d].dataRef",  i); json_val(jsonStr, elementName, overlay[i].dataRef);
      sprintf(elementName, "overlay[%d].dataType", i); json_val(jsonStr, elementName, overlay[i].dataType);
      sprintf(elementName, "overlay[%d].symType",  i); json_val(jsonStr, elementName, overlay[i].symType);
      sprintf(elementName, "overlay[%d].symSize",  i); json_val(jsonStr, elementName, overlay[i].symSize);
      sprintf(elementName, "overlay[%d].location", i); json_val(jsonStr, elementName, overlay[i].location);
      sprintf(elementName, "overlay[%d].text",     i); json_val(jsonStr, elementName, overlay[i].text);

      sprintf(elementName, "overlay[%d].visible", i);

      if(json_val(jsonStr, elementName, overlay[i].visible) == (char *)NULL) 
         strcpy(overlay[i].visible, "true");
      else
      {
         if(strcasecmp(overlay[i].visible, "true"   ) == 0
         || strcasecmp(overlay[i].visible, "visible") == 0
         || strcasecmp(overlay[i].visible, "yes"    ) == 0
         || strcasecmp(overlay[i].visible, "on"     ) == 0
         || strcasecmp(overlay[i].visible, "1"      ) == 0)
            strcpy(overlay[i].visible, "true");
         else
            strcpy(overlay[i].visible, "false");
      }

      if(debug)
      {
         fprintf(fdebug, "  visible:  [%s]\n", overlay[i].visible);
         fflush(fdebug);
      }

      ++i;
   }

   nOverlay = i;

   if(debug)
   {
      fprintf(fdebug, "nOverlay = %d\n", nOverlay);
      fflush(fdebug);
   }


   /*****************************************/
   /* Write the JSON structure to a file    */
   /* (to have a copy; we could do all the  */
   /* parameter processing in memory just   */
   /* as well)                              */
   /*****************************************/

   sprintf(jsonFile, "%s/%s_current.json", directory, imageFile);

   fp = fopen(jsonFile, "w+");

   fprintf(fp, "%s", jsonOrig);
   fclose(fp);

   if(debug)
   {
      fprintf(fdebug, "jsonFile  = %s\n", jsonFile);
      fflush(fdebug);
   }


   /***************************************************/
   /* Cutout and rescale the image to fit the display */
   /***************************************************/

   canvasWidth  = atof(canvasWidthStr);
   canvasHeight = atof(canvasHeightStr);

   if(strlen(grayFile) > 0)
      sprintf(cmd, "imginfo %s", grayFile);
   else
      sprintf(cmd, "imginfo %s", redFile);

   svc_run(cmd);

   strcpy(status, svc_value( "stat" ));

   if(strcmp( status, "ERROR") == 0)
   {
      strcpy(tmpstr, svc_value( "msg" ));
      printError(tmpstr);
   }

   imageWidth  = atof(svc_value("naxis1"));
   imageHeight = atof(svc_value("naxis2"));



   // SUBIMAGE

   // Special case, like when neither value
   // is given at all

   if(xmin == xmax)
   {
      xmin = 0.;
      xmax = imageWidth;
   }

   if(ymin == ymax)
   {
      ymin = 0.;
      ymax = imageHeight;
   }

   if(debug)
   {
      fprintf(fdebug, "Subimage request    x: %.6f -> %.6f, y: %.6f -> %.6f\n", 
         xmin, xmax, ymin, ymax);
      fflush(fdebug);
   }


   // Adjust the region to match the canvas aspect
   // ratio as much as possible

   if(xmin < 0.)
      xmin = 0.;

   if(xmax > imageWidth)
      xmax = imageWidth;

   if(ymin < 0.)
      ymin = 0.;

   if(ymin > imageHeight)
      ymin = imageHeight;

   if(debug)
   {
      fprintf(fdebug, "Subimage trimmed:  xmin = %.6f, xmax = %.6f, ymin = %.6f, ymax = %.6f\n", 
         xmin, xmax, ymin, ymax);
      fflush(fdebug);
   }


   // Find the canvas and region aspect ratios

   canvasRatio = canvasHeight / canvasWidth;
   regionRatio = (ymax-ymin)  / (xmax-xmin);

   if(debug)
   {
      fprintf(fdebug, "Ratios: canvasRatio = %.6f, regionRatio = %.6f\n", 
         canvasRatio, regionRatio);
      fflush(fdebug);
   }


   // Adjust one of the region sides size 
   // so that the region has the same 
   // proportions as the canvas

   xsize = xmax - xmin;
   ysize = ymax - ymin;

   if(debug)
   {
      fprintf(fdebug, "Size (before aspect correction):  xsize = %.6f, ysize = %.6f\n", 
         xsize, ysize);
      fflush(fdebug);
   }


   if(regionRatio < canvasRatio)
      ysize = xsize * canvasRatio;
   else
      xsize = ysize / canvasRatio;

   if(debug)
   {
      fprintf(fdebug, "Size (after):  xsize = %.6f, ysize = %.6f\n", 
         xsize, ysize);
      fflush(fdebug);
   }


   // Find new xmin...ymax based on
   // original center and zoomed canvas
   // size

   xcenter = (xmin + xmax)/2.;
   ycenter = (ymin + ymax)/2.;

   if(debug)
   {
      fprintf(fdebug, "Center:  xcenter = %.6f, ycenter = %.6f\n", 
         xcenter, ycenter);
      fflush(fdebug);
   }

   xmin = xcenter - xsize / 2.;
   xmax = xcenter + xsize / 2.;
   ymin = ycenter - ysize / 2.;
   ymax = ycenter + ysize / 2.;

   if(debug)
   {
      fprintf(fdebug, "Cutout box:  xmin = %.6f, xmax = %.6f, ymin = %.6f, ymax = %.6f\n", 
         xmin, xmax, ymin, ymax);
      fflush(fdebug);
   }


   // Since we expanded one dimension, we may
   // be offscale again. Shift anything that is.
   // We may shift both ways but this will only 
   // happen if we're bigger than the image.

   if(xmax > imageWidth)
   {
      xmin -= xmax - imageWidth;
      xmax = imageWidth;
   }
  
   if(xmin < 0.)
   {
      xmax = xmax - xmin;
      xmin = 0.;
   }
  
   if(xmax > imageWidth)
      xmax = imageWidth;


   if(ymax > imageHeight)
   {
      ymin -= ymax - imageHeight;
      ymax = imageHeight;
   }
  
   if(ymin < 0.)
   {
      ymax = ymax - ymin;
      ymin = 0.;
   }
  
   if(ymax > imageHeight)
      ymax = imageHeight;

   if(debug)
   {
      fprintf(fdebug, "Edge shifted:  xmin = %.6f, xmax = %.6f, ymin = %.6f, ymax = %.6f\n", 
         xmin, xmax, ymin, ymax);
      fflush(fdebug);
   }


   // Integer range for actual cutout
   
   ixmin = (int)floor(xmin);
   ixmax = (int)ceil (xmax);
   iymin = (int)floor(ymin);
   iymax = (int)ceil (ymax);

   ixsize = ixmax - ixmin;
   iysize = iymax - iymin;

   if(ixsize < 10) ixsize = 10;
   if(iysize < 10) iysize = 10;

   if(strlen(grayFile) > 0)
   {
      sprintf(cmd, "mSubimage -p %s %s_cutout_%s %d %d %d %d", grayFile,   imageFile, grayFile, ixmin, iymin, ixsize, iysize); svc_run(cmd);
   }
   else
   {
      if(strlen(redFile) > 0)
      {
         sprintf(cmd, "mSubimage -p %s %s_cutout_%s %d %d %d %d", redFile,    imageFile, redFile,   ixmin, iymin, ixsize, iysize); 
         svc_run(cmd);
      }

      if(strlen(greenFile) > 0)
      {
         sprintf(cmd, "mSubimage -p %s %s_cutout_%s %d %d %d %d", greenFile,  imageFile, greenFile, ixmin, iymin, ixsize, iysize);
         svc_run(cmd);
      }

      if(strlen(blueFile) > 0)
      {
         sprintf(cmd, "mSubimage -p %s %s_cutout_%s %d %d %d %d", blueFile,   imageFile, blueFile,  ixmin, iymin, ixsize, iysize);
         svc_run(cmd);
      }
   }

   if(strlen(grayFile) > 0)
      sprintf(cmd, "imginfo %s_cutout_%s", imageFile, grayFile);
   else
      sprintf(cmd, "imginfo %s_cutout_%s", imageFile, redFile);

   svc_run(cmd);

   strcpy(status, svc_value( "stat" ));

   if(strcmp( status, "ERROR") == 0)
   {
      strcpy(tmpstr, svc_value( "msg" ));
      printError(tmpstr);
   }

   imageWidth  = atof(svc_value("naxis1"));
   imageHeight = atof(svc_value("naxis2"));

   wscale = fabs(atof(svc_value("yscale")));

   wrotation = fabs(atof(svc_value("rotation"))) + 180;

   while(wrotation > 360.) wrotation -= 360.;
   while(wrotation <   0.) wrotation += 360.;

   wra  = fabs(atof(svc_value("rac")));
   wdec = fabs(atof(svc_value("decc")));
   
   wxsize = fabs(atof(svc_value("xrefpix")));
   wysize = fabs(atof(svc_value("yrefpix")));
   

   // SHRINK

   xfactor = imageWidth  / canvasWidth;
   yfactor = imageHeight / canvasHeight;

   if(yfactor > xfactor)
      xfactor = yfactor;

   if(strlen(grayFile) > 0)
   {
      sprintf(cmd, "mShrink %s_cutout_%s %s_shrunken_%s %.6f", imageFile, grayFile,  imageFile, grayFile,  xfactor); 
      svc_run(cmd);
   }
   else
   {
      if(strlen(redFile) > 0)
      {
         sprintf(cmd, "mShrink %s_cutout_%s %s_shrunken_%s %.6f", imageFile, redFile,   imageFile, redFile,   xfactor);
         svc_run(cmd);
      }

      if(strlen(greenFile) > 0)
      {
         sprintf(cmd, "mShrink %s_cutout_%s %s_shrunken_%s %.6f", imageFile, greenFile, imageFile, greenFile, xfactor);
         svc_run(cmd);
      }

      if(strlen(blueFile) > 0)
      {
         sprintf(cmd, "mShrink %s_cutout_%s %s_shrunken_%s %.6f", imageFile, blueFile,  imageFile, blueFile,  xfactor);
         svc_run(cmd);
      }
   }




   /*********************************/
   /* Now build the mViewer command */
   /*********************************/

   strcpy(cmd, "mViewer");

   if(debug)
   {
      fprintf(fdebug, "nOverlay  = %d\n", nOverlay);
      fflush(fdebug);
   }

   for(i=0; i<nOverlay; ++i)
   {
      if(debug)
      {
         fprintf(fdebug, "overlay[%d] = \"%s\" [visible: %s]\n", i, overlay[i].type, overlay[i].visible);
         fflush(fdebug);
      }


      // GRID

      if(strcmp(overlay[i].type, "grid") == 0 && strcmp(overlay[i].visible, "true") == 0)
      {
         sprintf(subcmd, " -color %s -grid %s", overlay[i].color, overlay[i].coordSys);
         strcat(cmd, subcmd);
      }


      // CATALOG

      else if(strcmp(overlay[i].type, "catalog") == 0 && strcmp(overlay[i].visible, "true") == 0)
      {
         sprintf(subcmd, " -color %s -symbol %s %s -catalog %s %s %s %s", 
            overlay[i].color, overlay[i].symSize, overlay[i].symType, overlay[i].dataFile, 
            overlay[i].dataCol, overlay[i].dataRef, overlay[i].dataType);
         strcat(cmd, subcmd);
      }


      // IMAGE METADATA

      else if(strcmp(overlay[i].type, "imginfo") == 0 && strcmp(overlay[i].visible, "true") == 0)
      {
         sprintf(subcmd, " -color %s -imginfo %s", 
            overlay[i].color, overlay[i].dataFile);
         strcat(cmd, subcmd);
      }


      // MARK

      else if(strcmp(overlay[i].type, "mark") == 0 && strcmp(overlay[i].visible, "true") == 0)
      {
         processLocation(overlay[i].location, cra, cdec);

         sprintf(subcmd, " -color %s -symbol %s %s -mark %s %s", 
            overlay[i].color, overlay[i].symSize, overlay[i].symType, cra, cdec);

         strcat(cmd, subcmd);
      }


      // LABEL

      else if(strcmp(overlay[i].type, "label") == 0 && strcmp(overlay[i].visible, "true") == 0)
      {
         processLocation(overlay[i].location, cra, cdec);

         sprintf(subcmd, " -color %s -label %s %s \"%s\"", 
            overlay[i].color, cra, cdec, overlay[i].text);
         strcat(cmd, subcmd);
      }

      if(debug)
      {
         fprintf(fdebug, "cmd -> [%s]\n", cmd);
         fflush(fdebug);
      }
   }


   // FITS FILE(S)

   if(strlen(grayFile) > 0)
   {
      sprintf(subcmd, " -ct %s -gray %s_shrunken_%s %s %s %s", 
         colorTable, imageFile, grayFile, stretchMin, stretchMax, stretchMode);
      strcat(cmd, subcmd);
   }
   else
   {
      if(strlen(blueFile) > 0)
      {
         sprintf(subcmd, " -blue %s_shrunken_%s %s %s %s", 
            imageFile, blueFile, blueMin, blueMax, blueMode);
         strcat(cmd, subcmd);
      }

      if(strlen(greenFile) > 0)
      {
         sprintf(subcmd, " -green %s_shrunken_%s %s %s %s", 
            imageFile, greenFile, greenMin, greenMax, greenMode);
         strcat(cmd, subcmd);
      }

      if(strlen(redFile) > 0)
      {
         sprintf(subcmd, " -red %s_shrunken_%s %s %s %s", 
            imageFile, redFile, redMin, redMax, redMode);
         strcat(cmd, subcmd);
      }
   }


   // OUTPUT FILE
      
   if(strcmp(imageType, "png") == 0)
      sprintf(subcmd, " -%s %s.png", imageType, imageFile);
   else
      sprintf(subcmd, " -%s %s.jpg", imageType, imageFile);

   strcat(cmd, subcmd);


   /****************************/
   /* Create the PNG plot file */
   /****************************/

   svc_run(cmd);

   strcpy(status, svc_value( "stat" ));

   if(strcmp( status, "ERROR") == 0)
   {
      strcpy(tmpstr, svc_value( "msg" ));
      printError(tmpstr);
   }

   if(svc_value("colortable") != (char *)NULL)
   {
      strcpy(colortable,  svc_value("colortable"));
      strcpy(min,         svc_value("min"));
      strcpy(minpercent,  svc_value("minpercent"));
      strcpy(minsigma,    svc_value("minsigma"));
      strcpy(max,         svc_value("max"));
      strcpy(maxpercent,  svc_value("maxpercent"));
      strcpy(maxsigma,    svc_value("maxsigma"));
      strcpy(datamin,     svc_value("datamin"));
      strcpy(datamax,     svc_value("datamax"));
   }
   else
   {
      strcpy(colortable, "");
      strcpy(bmin,        svc_value("bmin"));
      strcpy(bminpercent, svc_value("bminpercent"));
      strcpy(bminsigma,   svc_value("bminsigma"));
      strcpy(bmax,        svc_value("bmax"));
      strcpy(bmaxpercent, svc_value("bmaxpercent"));
      strcpy(bmaxsigma,   svc_value("bmaxsigma"));
      strcpy(bdatamin,    svc_value("bdatamin"));
      strcpy(bdatamax,    svc_value("bdatamax"));

      strcpy(gmin,        svc_value("gmin"));
      strcpy(gminpercent, svc_value("gminpercent"));
      strcpy(gminsigma,   svc_value("gminsigma"));
      strcpy(gmax,        svc_value("gmax"));
      strcpy(gmaxpercent, svc_value("gmaxpercent"));
      strcpy(gmaxsigma,   svc_value("gmaxsigma"));
      strcpy(gdatamin,    svc_value("gdatamin"));
      strcpy(gdatamax,    svc_value("gdatamax"));

      strcpy(rmin,        svc_value("rmin"));
      strcpy(rminpercent, svc_value("rminpercent"));
      strcpy(rminsigma,   svc_value("rminsigma"));
      strcpy(rmax,        svc_value("rmax"));
      strcpy(rmaxpercent, svc_value("rmaxpercent"));
      strcpy(rmaxsigma,   svc_value("rmaxsigma"));
      strcpy(rdatamin,    svc_value("rdatamin"));
      strcpy(rdatamax,    svc_value("rdatamax"));
   }

   strcpy(xflip,      svc_value("xflip"));
   strcpy(yflip,      svc_value("yflip"));
   strcpy(bunit,      svc_value("bunit"));

   strcpy(wfile, imageFile);

   if(strcmp(imageType, "png") == 0)
      strcat(wfile, ".png");
   else
      strcat(wfile, ".jpg");

   sprintf(wwturl, "http://www.worldwidetelescope.org/wwtweb/ShowImage.aspx?scale=%.3f&rotation=%.4f&ra=%.7f&dec=%.7f&y=%.3f&x=%.3f&thumb=%s%s&imageurl=%s%s&name=%s", wscale, wrotation, wra, wdec, wysize, wxsize, baseURL, wfile, baseURL, wfile, imageFile); 
   

   printf("HTTP/1.1 200 OK\r\n");
   printf("Content-type: text/plain\r\n\r\n");
   fflush(stdout);
      
   if(strlen(colortable) > 0)
   {
      if(strcmp(imageType, "png") == 0)
         printf("{\"file\": \"%s/%s.png\", \"xmin\": %d, \"xmax\": %d, \"ymin\": %d, \"ymax\": %d, \"scale\": %.10f, \"min\":\"%s\", \"minpercent\":\"%s\", \"minsigma\":\"%s\", \"max\":\"%s\", \"maxpercent\":\"%s\", \"maxsigma\":\"%s\", \"datamin\":\"%s\", \"datamax\":\"%s\", \"xflip\":\"%s\", \"yflip\":\"%s\", \"bunit\":\"%s\", \"WWTURL\": \"%s\", \"colortable\":\"%s\"}",
            baseURL, imageFile, ixmin, ixmax, iymin, iymax, xfactor, min, minpercent, minsigma, max, maxpercent, maxsigma, datamin, datamax, xflip, yflip, bunit, wwturl, colortable);

      else
         printf("{\"file\": \"%s/%s.jpg\", \"xmin\": %d, \"xmax\": %d, \"ymin\": %d, \"ymax\": %d, \"scale\": %.10f, \"min\":\"%s\", \"minpercent\":\"%s\", \"minsigma\":\"%s\", \"max\":\"%s\", \"maxpercent\":\"%s\", \"maxsigma\":\"%s\", \"datamin\":\"%s\", \"datamax\":\"%s\", \"xflip\":\"%s\", \"yflip\":\"%s\", \"bunit\":\"%s\", \"WWTURL\": \"%s\", \"colortable\":\"%s\"}",
            baseURL, imageFile, ixmin, ixmax, iymin, iymax, xfactor, min, minpercent, minsigma, max, maxpercent, maxsigma, datamin, datamax, xflip, yflip, bunit, wwturl, colortable);
   }
   else
   {
      if(strcmp(imageType, "png") == 0)
         printf("{\"file\": \"%s/%s.png\", \"xmin\": %d, \"xmax\": %d, \"ymin\": %d, \"ymax\": %d, \"scale\": %.10f, ",
            baseURL, imageFile, ixmin, ixmax, iymin, iymax, xfactor);
      else
         printf("{\"file\": \"%s/%s.jpg\", \"xmin\": %d, \"xmax\": %d, \"ymin\": %d, \"ymax\": %d, \"scale\": %.10f, ",
            baseURL, imageFile, ixmin, ixmax, iymin, iymax, xfactor);

      printf("\"bmin\":\"%s\", \"bminpercent\":\"%s\", \"bminsigma\":\"%s\", \"bmax\":\"%s\", \"bmaxpercent\":\"%s\", \"bmaxsigma\":\"%s\", \"bdatamin\":\"%s\", \"bdatamax\":\"%s\", ",
         bmin, bminpercent, bminsigma, bmax, bmaxpercent, bmaxsigma, bdatamin, bdatamax);

      printf("\"gmin\":\"%s\", \"gminpercent\":\"%s\", \"gminsigma\":\"%s\", \"gmax\":\"%s\", \"gmaxpercent\":\"%s\", \"gmaxsigma\":\"%s\", \"gdatamin\":\"%s\", \"gdatamax\":\"%s\", ",
         gmin, gminpercent, gminsigma, gmax, gmaxpercent, gmaxsigma, gdatamin, gdatamax);

      printf("\"rmin\":\"%s\", \"rminpercent\":\"%s\", \"rminsigma\":\"%s\", \"rmax\":\"%s\", \"rmaxpercent\":\"%s\", \"rmaxsigma\":\"%s\", \"rdatamin\":\"%s\", \"rdatamax\":\"%s\", ",
         rmin, rminpercent, rminsigma, rmax, rmaxpercent, rmaxsigma, rdatamin, rdatamax);

      printf("\"xflip\":\"%s\", \"yflip\":\"%s\", \"bunit\":\"%s\", \"WWTURL\": \"%s\"}",
         xflip, yflip, bunit, wwturl);
   }

   fflush(stdout);

   exit(0);
}


/**********************************************/
/* Convert a location string into coordinates */
/**********************************************/

void processLocation(char *location, char *ra, char *dec)
{
   int    ispix;
   double tmpval;
   char   status[16];
   char   cmd[STRLEN];

   ispix = 0;

   if(strstr(location, "p") != 0)
      ispix = 1;

   if(ispix)
   {
      sscanf(location, "%s %s", ra, dec);

      tmpval = atof(ra);

      sprintf(ra, "%.0fp", tmpval);

      tmpval = atof(dec);

      sprintf(dec, "%.0fp", tmpval);

      return;
   }

   sprintf(cmd, "lookup -s %s", location);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
      printError("Invalid location");

   svc_run(cmd);

   strcpy(ra,  svc_value("lon"));
   strcpy(dec, svc_value("lat"));
   
   return;
}


/**********************/
/* HTML Error message */
/**********************/

void printError(char *errmsg)
{
   printf("HTTP/1.1 200 OK\r\n");
   printf("Content-type: text/plain\r\n\r\n");
   fflush(stdout);

   printf ("{\"error\" : \"%s\"}", errmsg);
   fflush (stdout);

   exit(0);
}
