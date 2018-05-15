/* Module: mArchiveGet.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.0      John Good        08Dec17  Total rewrite to use WGET/CURL
1.1      John Good        14Feb05  Got rid of IRSA-specific code
1.0      John Good        15Dec04  Baseline code

*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <bzlib.h>

#include <montage.h>
#include <mArchiveGet.h>


/*-*****************************************************************************/
/*                                                                             */
/* Unfortunately, simple URL access to some archive data has become more       */
/* complex.  It used to be that most data was available via a simple socket    */
/* connection to an HTTP server.  Now many sites use secure sockets (even      */
/* when they don't need to), redirection (often because of the secure sockets) */
/* and so on.                                                                  */
/*                                                                             */
/* We don't want to recreate all the infrastructure in programs like wget      */
/* and curl that deals with all this, so we have opted for using utilities     */
/* like wget or curl.                                                          */
/*                                                                             */
/* Since one or the other of these are default on almost all Linux and OSX     */
/* platforms (though neither is there by default on both platforms), we have   */
/* opted to do the following: First try a fork/exec of wget and parse the      */
/* output string (limited by the -nv flag to be minimal information).  If      */
/* this fails, try the same with curl (limiting the output string here as      */
/* well to just the output file size).  An added twist is that wget sends      */
/* its output to stderr and curl to stdout but we can take care of this        */
/* by being specific about which stream is redirected back to the parent       */
/* process.                                                                    */
/*                                                                             */
/*******************************************************************************/
   
struct mArchiveGetReturn *mArchiveGet(char *url, char *datafile, int timeout, int debug)
{
   int    i, ch, status, nnew, imgsize, retcode, child, waitstatus;

   char  *begin, *end, *endptr;

   char   timestr[1024];

   char **cmdv;

   int    fdpipe[2];

   FILE  *fromexec;

   char   retval[32768];

   struct mArchiveGetReturn *returnStruct;

   sprintf(timestr, "%d", timeout);

   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mArchiveGetReturn *)malloc(sizeof(struct mArchiveGetReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   // Both wget and curl will take five command-line
   // arguments.  For execvp(), the sixth argument must
   // be (char *)NULL.

   cmdv = (char **)malloc(16 * sizeof(char *));

   for(i=0; i<16; ++i)
      cmdv[i] = (char *)NULL;

   for(i=0; i<7; ++i)
      cmdv[i] = (char *)malloc(1024 * sizeof(char));



   // WGET

   // Set up arguments for wget

   strcpy(cmdv[0], "wget");
   strcpy(cmdv[1], "-nv");
   strcpy(cmdv[2], "-T");
   strcpy(cmdv[3], timestr);
   strcpy(cmdv[4], "-O");
   strcpy(cmdv[5], datafile);
   strcpy(cmdv[6], url);

   if(debug)
   {
      printf("DEBUG> [%s] -> [%s] (timeout:%s)\n", url, datafile, timestr);
      fflush(stdout);
   }


   // Create a pipe to allow the child (wget) to communicate
   // its stderr output back to the parent (this process).

   pipe(fdpipe);
   

   // Fork/exec wget making sure that it's stderr gets redirected
   // (via dup2()) to our pipe. For simplicity we open the output
   // end of the pipe in the parent as a stream.

   if((child = fork()) == 0)      // Child (wget)
   {
      close(fdpipe[0]);

      (void) dup2(fdpipe[1], 2);  // Redirect stderr

      execvp(cmdv[0], cmdv);

      exit(0);  // Execvp() overlays the process so we should 
                // never get to this statement.
   }
   else                 // Parent (this process)
   {
      close(fdpipe[1]);

      fromexec = fdopen(fdpipe[0], "r");
   }


   // Read what comes back from wget.  If there is 
   // nothing here (or we can't find the file size)
   // we'll assume wget failed (or didn't exist).

   i=0;

   while(1)
   {
      ch = fgetc(fromexec);

      if(ch == EOF)
         break;

      if(i == 32768)
         break;

      retval[i] = ch;

      ++i;
   }
   
   retval[i] = '\0';

   if(debug)
   {
      printf("DEBUG> wget return value: [%s]\n", retval);
      fflush(stdout);
   }

   fclose(fromexec);
   close(fdpipe[0]);

   waitpid(child, &waitstatus, WNOHANG | WUNTRACED | WCONTINUED);

   begin = strstr(retval, " ERROR ");

   if(begin != (char *)NULL)
   {
      begin += 7;

      retcode = atoi(begin);

      if(retcode > 0 && retcode != 200)
      {
         for(i=0; i<16; ++i)
         {
            if(cmdv[i])
               free(cmdv[i]);
         }

         free(cmdv);

         sprintf(returnStruct->msg, "Retrieval failed.  HTTP return code: %d.", retcode);
         return returnStruct;
      }
   }

   if(strlen(retval) > 0)
   {
      // Check for errors

      if(strstr(retval, "Permission denied") != (char *)NULL)
      {
         for(i=0; i<16; ++i)
         {
            if(cmdv[i])
               free(cmdv[i]);
         }

         free(cmdv);

         strcpy(returnStruct->msg, "Cannot write to output file.");
         return returnStruct;
      }


      if(strstr(retval, "unable to resolve") != (char *)NULL)
      {
         for(i=0; i<16; ++i)
         {
            if(cmdv[i])
               free(cmdv[i]);
         }

         free(cmdv);

         strcpy(returnStruct->msg, "Unable to resolve URL.");
         return returnStruct;
      }


      // Parse out the file size

      begin = retval;
      
      while(*begin != '[' && *begin != '\0')
         ++begin;

      if(*begin == '[')
         ++begin;

      end = begin;

      while(*end != ']' && *end != '/' && *end != '\0')
         ++end;

      *end = '\0';

      imgsize = strtol(begin, &endptr, 10);

      if(strlen(begin) > 0 && endptr == end)
      {
         for(i=0; i<16; ++i)
         {
            if(cmdv[i])
               free(cmdv[i]);
         }

         free(cmdv);

         nnew = mArchiveGet_bunzip(datafile, debug);

         if(nnew > 0)
            imgsize = nnew;

         returnStruct->status = 0;

         sprintf(returnStruct->msg,    "count=%d",     imgsize);
         sprintf(returnStruct->json, "{\"count\":%d}", imgsize);
     
         returnStruct->count  = imgsize;

         return returnStruct;
      }
   }

   

   // CURL

   // If we need to, now try curl.

   cmdv[7] = (char *)malloc(1024 * sizeof(char));
   cmdv[8] = (char *)malloc(1024 * sizeof(char));
   cmdv[9] = (char *)malloc(1024 * sizeof(char));

   strcpy(cmdv[0], "curl");
   strcpy(cmdv[1], "-s");
   strcpy(cmdv[2], "-L");
   strcpy(cmdv[3], "-m");
   strcpy(cmdv[4], timestr);
   strcpy(cmdv[5], "-w");
   strcpy(cmdv[6], "%{size_download}:%{http_code}");
   strcpy(cmdv[7], "-o");
   strcpy(cmdv[8], datafile);
   strcpy(cmdv[9], url);


   // Create a pipe to allow the child (wget) to communicate
   // its stderr output back to the parent (this process).

   pipe(fdpipe);


   // Fork/exec wget making sure that it's stderr gets redirected
   // (via dup2()) to our pipe. For simplicity we open the output
   // end of the pipe in the parent as a stream.

   if(fork() == 0)      // Child (wget)
   {
      close(fdpipe[0]);

      (void) dup2(fdpipe[1], 1);  // Redirect stdout

      execvp(cmdv[0], cmdv);

      exit(0);  // Execvp() overlays the process so we should 
                // never get to this statement.
   }
   else                 // Parent (this process)
   {
      close(fdpipe[1]);

      fromexec = fdopen(fdpipe[0], "r");
   }


   // Read what comes back from wget.  If there is 
   // nothing here (or we can't find the file size)
   // we'll assume wget failed (or didn't exist).

   i=0;

   while(1)
   {
      ch = fgetc(fromexec);

      if(ch == EOF)
         break;

      if(i == 32768)
         break;

      retval[i] = ch;

      ++i;
   }
   
   retval[i] = '\0';

   end = retval;

   if(debug)
   {
      printf("DEBUG> curl return value: [%s]\n", retval);
      fflush(stdout);
   }

   while(*end != ':' && *end != '\0')
      ++end;

   begin = end;

   if(*begin == ':')
      ++begin;

   *end = '\0';

   imgsize = strtol(retval, &endptr, 10);

   retcode = atoi(begin);

   if(retcode > 0 && retcode != 200)
   {
      sprintf(returnStruct->msg, "Retrieval failed.  HTTP return code: %d.", retcode);
      return returnStruct;
   }

   if(strlen(retval) > 0 && endptr == end)
   {
      close(fdpipe[0]);

      for(i=0; i<16; ++i)
         if(cmdv[i])
            free(cmdv[i]);

      free(cmdv);

      if(imgsize == 0)
      {
         strcpy(returnStruct->msg, "Retrieval failed.  Check URL and file permissions.");
         return returnStruct;
      }
      else
      {
         nnew = mArchiveGet_bunzip(datafile, debug);

         if(nnew > 0)
            imgsize = nnew;

         returnStruct->status = 0;

         sprintf(returnStruct->msg,    "count=%d",     imgsize);
         sprintf(returnStruct->json, "{\"count\":%d}", imgsize);
     
         returnStruct->count  = imgsize;

         return returnStruct;
      }
   }



   // We only get here if both wget and curl failed

   close(fdpipe[0]);

   for(i=0; i<16; ++i)
      if(cmdv[i])
         free(cmdv[i]);

   free(cmdv);

   strcpy(returnStruct->msg, "Need either wget or curl executables in your path.");
   return returnStruct;
}




int mArchiveGet_bunzip(char *infile, int debug)
{
   int     bzError, nread;
   size_t  nwritten, ntot;

   char    buf    [4096];
   char    outfile[4096];

   FILE   *fin;
   FILE   *fout;
   BZFILE *bzfile;

   if(strlen(infile) < 5)
      return 0;

   if(strcmp(infile+strlen(infile)-4, ".bz2") != 0)
      return 0;

   strcpy(outfile, infile);

   outfile[strlen(outfile)-4] = '\0';

   if(debug)
   {
      printf("DEBUG> bunzip [%s] -> [%s]\n", infile, outfile);
      fflush(stdout);
   }

   fin  = fopen(infile,  "r");
   fout = fopen(outfile, "w+");


   bzfile = BZ2_bzReadOpen(&bzError, fin, 0, 0, NULL, 0);

   if (bzError != BZ_OK) 
      return -1;


   ntot = 0;

   while (bzError == BZ_OK) 
   {
      nread = BZ2_bzRead(&bzError, bzfile, buf, sizeof buf);

      if (bzError == BZ_OK || bzError == BZ_STREAM_END) 
      {
         nwritten = fwrite(buf, 1, nread, fout);

         ntot += nwritten;

         if (nwritten != (size_t) nread) 
            return -1;
      }
   }


   if (bzError != BZ_STREAM_END) 
      return -1;


   BZ2_bzReadClose(&bzError, bzfile);

   unlink(infile);

   if(debug)
   {
      printf("DEBUG> bunzip done\n");
      fflush(stdout);
   }

   return ntot;
}
