/* Module: checkWCS.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.7      John Good        24Feb06  The CD matrix constraints were too strict
1.6      John Good        02Dec03  Change naxes to naxis to match
                                   change ins WCS library.
1.5      John Good        25Aug03  Implement status file output
1.4      John Good        15Apr03  Allow for LON,LAT transpose
1.3      John Good        19Mar03  Renamed file / function from wcsCheck
                                   to checkWCS for consistency
1.2      John Good        19Mar03  Modified bad WCS error message  
1.1      John Good        18Mar03  Corrected error with action flag
1.0      John Good        13Mar03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wcs.h>

#include <montage.h>

static char montage_msgstr[1024];

int wcs_debug = 0;


/*************************************************************************/
/*                                                                       */
/*  checkWCS                                                             */
/*                                                                       */
/*  This routine check a WCS structure to determine if the information   */
/*  contained is internally consistent.                                  */
/*                                                                       */
/*************************************************************************/

char *montage_checkWCS(struct WorldCoor *wcs)
{
   int i;

   if(wcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "No WCS information (or not FITS header)");
      return montage_msgstr;
   }

   if(wcs_debug)
   {
      printf("prjcode    = %d\n", wcs->prjcode);
      printf("ptype      = [%s]\n", wcs->ptype);
      printf("nxpix      = %-g\n", wcs->nxpix);
      printf("nypix      = %-g\n", wcs->nypix);
      printf("ctype[0]   = [%s]\n", wcs->ctype[0]);
      printf("ctype[1]   = [%s]\n", wcs->ctype[1]);
      printf("naxis      = %d\n", wcs->naxis);

      for(i=0; i<wcs->naxis; ++i)
        printf("crval[%d]   = %-g\n", i, wcs->crval[i]);

      printf("xref       = %-g\n", wcs->xref);
      printf("yref       = %-g\n", wcs->yref);

      for(i=0; i<wcs->naxis; ++i)
        printf("crpix[%d]   = %-g\n", i, wcs->crpix[i]);

      printf("xrefpix    = %-g\n", wcs->xrefpix);
      printf("yrefpix    = %-g\n", wcs->yrefpix);

      if(wcs->rotmat)
      {
         for(i=0; i<4; ++i)
           printf("cd[%d]      = %-g\n", i, wcs->cd[i]);
      }
      else
      {
         for(i=0; i<wcs->naxis; ++i)
           printf("cdelt[%d]   = %-g\n", i, wcs->cdelt[i]);
      }

      printf("xinc       = %-g\n", wcs->xinc);
      printf("yinc       = %-g\n", wcs->yinc);
      printf("rot        = %-g\n", wcs->rot);

      printf("equinox    = %-g\n", wcs->equinox);
      printf("epoch      = %-g\n", wcs->epoch);

      for(i=0; i<16; ++i)
        printf("pc[%2d]     = %-g\n", i, wcs->pc[i]);

      for(i=0; i<10; ++i)
        printf("projp[%2d]  = %-g\n", i, wcs->projp[i]);

      printf("longpole   = %-g\n", wcs->longpole);
      printf("latpole    = %-g\n", wcs->latpole);
   }


   /* Check NAXIS */

   if(wcs->naxis < 2)
   {
      sprintf(montage_msgstr, "Must have at least two (n>1) dimensions");
      return montage_msgstr;
   }


   /* Check the projection */

   if(wcs->prjcode <= 0)
   {
      sprintf(montage_msgstr, "Invalid projection");
      return montage_msgstr;
   }


   /* Check NAXIS1 and NAXIS2 */

   if(wcs->nxpix <= 0)
   {
      sprintf(montage_msgstr, "Invalid NAXIS1");
      return montage_msgstr;
   }

   if(wcs->nypix <= 0)
   {
      sprintf(montage_msgstr, "Invalid NAXIS2");
      return montage_msgstr;
   }


   /* Check the coordinate system (from CTYPE1 and CTYPE2) */

   if(strncmp(wcs->ctype[0], "RA", 2) == 0)
   {
      if(strncmp(wcs->ctype[1], "DEC", 3) != 0)
      {
         sprintf(montage_msgstr, "CTYPE1 and CTYPE2 don't match");
         return montage_msgstr;
      }
   }
   else if(strncmp(wcs->ctype[0], "DEC", 3) == 0)
   {
      if(strncmp(wcs->ctype[1], "RA", 2) != 0)
      {
         sprintf(montage_msgstr, "CTYPE1 and CTYPE2 don't match");
         return montage_msgstr;
      }
   }
   else if(strncmp(wcs->ctype[0], "GLON", 4) == 0)
   {
      if(strncmp(wcs->ctype[1], "GLAT", 4) != 0)
      {
         sprintf(montage_msgstr, "CTYPE1 and CTYPE2 don't match");
         return montage_msgstr;
      }
   }
   else if(strncmp(wcs->ctype[0], "GLAT", 4) == 0)
   {
      if(strncmp(wcs->ctype[1], "GLON", 4) != 0)
      {
         sprintf(montage_msgstr, "CTYPE1 and CTYPE2 don't match");
         return montage_msgstr;
      }
   }
   else if(strncmp(wcs->ctype[0], "ELON", 4) == 0)
   {
      if(strncmp(wcs->ctype[1], "ELAT", 4) != 0)
      {
         sprintf(montage_msgstr, "CTYPE1 and CTYPE2 don't match");
         return montage_msgstr;
      }
   }
   else if(strncmp(wcs->ctype[0], "ELAT", 4) == 0)
   {
      if(strncmp(wcs->ctype[1], "ELON", 4) != 0)
      {
         sprintf(montage_msgstr, "CTYPE1 and CTYPE2 don't match");
         return montage_msgstr;
      }
   }
   else
   {
      sprintf(montage_msgstr, "Invalid CTYPE1");
      return montage_msgstr;
   }


   /* Check the CD matrix or CDELT1, CDELT2 */

   if(wcs->rotmat)
   {
      if((wcs->cd[0] == 0. && wcs->cd[1] == 0.)
      || (wcs->cd[2] == 0. && wcs->cd[3] == 0.))
      {
         sprintf(montage_msgstr, "Invalid CD matrix");
         return montage_msgstr;
      }
   }
   else
   {
      if(wcs->xinc == 0.)
      {
         sprintf(montage_msgstr, "Invalid CDELT1");
         return montage_msgstr;
      }

      if(wcs->yinc == 0.)
      {
         sprintf(montage_msgstr, "Invalid CDELT2");
         return montage_msgstr;
      }
   }

   return (char *)NULL;
}
