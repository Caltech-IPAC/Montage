#include <stdio.h>
#include <math.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

void  mViewer_labeledCurve (char *face_path, int fontsize, int showLine,
                            double *xcurve, double *ycurve, int npt,  
                            char *text, double offset, 
                            double red, double green, double blue,
                            double linewidth);

double mViewer_frac        (double x);
double mViewer_invfrac     (double x);

void   mViewer_smooth_line (double x1, double y1, 
                            double x2, double y2,
                            double red, double green, double blue,
                            double linewidth);

void   mViewer_thick_line  (double x1, double y1, 
                            double x2, double y2,
                            double red, double green, double blue,
                            double width);

void   mViewer_draw_bitmap (FT_Bitmap * bitmap, int x, int y, double red, double green, double blue, int fontsize, double angle);

int    mViewer_setPixel    (int i, int j, double brightness, double red, double green, double blue, int replace);
int    mViewer_lockPixel   (int i, int j);
int    mViewer_getPixel    (int i, int j, int color);

void   mViewer_convexHull  (int n, int *xarray, int *yarray, int *nhull, int *xhull, int *yhull);

void   mViewer_boundingbox (int n, int *x, int *y, double *a, double *b, double *c, double *xbox, double *ybox);


double mViewer_label_length(char *face_path, int fontsize, char *text);

int mViewer_drawing_debug = 0;


/****************************************************/
/*                                                  */
/* Routine to draw a labeled curve on image.        */
/* The text is written along the curve of the data. */
/* Before and after the text, the curve is drawn.   */
/*                                                  */
/****************************************************/

void mViewer_labeledCurve( char *face_path, int fontsize, int showLine,
                           double *xcurve, double *ycurve, int npt,  
                           char *text, double offset, 
                           double red, double green, double blue,
                           double linewidth)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   FT_Bool use_kerning;
   FT_UInt previous = 0;

   int    npath;
   double x1, y1;
   double x2, y2;
   double path_length1, path_length2;

   int    npath_ang;
   double x1_ang, y1_ang;
   double x2_ang, y2_ang;
   double path_length2_ang;

   double xchar, ychar;
   double delx, dely;
   double xadvance;
   double angle, dl, frac, length;
   double xoff, yoff;
   double xcenter, ycenter, sgn[4];

   int    xpix, ypix, i, inext, j, k, inside;

   int    ncorner;
   int    xcorner[32768];
   int    ycorner[32768];

   int    nhull;
   int    xhull[1024];
   int    yhull[1024];

   double a[4], b[4], c[4];
   double xbox[4], ybox[4];
   double xmin, ymin, xmax, ymax, val;



   if(npt < 2)
      return;
   
   if(mViewer_drawing_debug)
   {
      printf("LABEL> [%s]\n", text);
      fflush(stdout);
   }


   /* Draw the curve up to the text starting offset */

   x1 = xcurve[0];
   y1 = ycurve[0];

   x2 = xcurve[1];
   y2 = ycurve[1];

   path_length1 = 0;
   path_length2 = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));

   npath = 1;

   while(path_length2 < offset)
   {
      if(showLine)
      {
         mViewer_smooth_line(x1, y1, x2, y2, red, green, blue, linewidth);
      }

      ++npath;
      if(npath >= npt)
         break;

      x1 = x2;
      y1 = y2;

      x2 = xcurve[npath];
      y2 = ycurve[npath];

      path_length1  = path_length2;
      path_length2 += sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
   }


   /* Find the angle for the text */

   angle = atan2((y2-y1), (x2-x1));

      xoff =  0.4 * (double)fontsize * sin(angle);
      yoff = -0.4 * (double)fontsize * cos(angle);


   /* Interpolate the last curve segment before the text and draw a subsegment */

   frac = (offset - path_length1) / (path_length2 - path_length1);

   xchar = x1 + frac * (x2 - x1);
   ychar = y1 + frac * (y2 - y1);
      
   if(frac > 0 && showLine)
   {
      mViewer_smooth_line(x1, y1, xchar, ychar, red, green, blue, linewidth);
   }

   xpix = floor(xchar + xoff);
   ypix = floor(ychar + yoff);

   pen.x = (xchar + xoff - (double)xpix) * 64.;
   pen.y = (ychar + yoff - (double)ypix) * 64.;


   /* Now draw characters, each one oriented based on the slope and location */
   /* of the curve at that point.  If we run out of curve before we run out  */
   /* of characters, extrapolate based on the last slope/location            */

   /* Place starting coordinates in adequate form. */

   length = 0.;


   /*Count the length of the string */

   int num_bytes=0;
   while(text[num_bytes]!=0)
        num_bytes++;


   //The array of ucs4 glyph indexes, which will by at most the number of bytes in the utf-8 file.

   long * ucs4text;

   ucs4text = malloc((num_bytes+1) * sizeof(long));

   unsigned char u,v,w,x,y,z;

   int num_chars=0;

   long iii=0;

   while(iii<num_bytes)
   {
      z = text[iii];

      if(z<=127)
      {
         ucs4text[num_chars] = z;
      }

      if((192<=z)&&(z<=223))
      {
         iii++; y = text[iii];
         ucs4text[num_chars] = (z-192)*64 + (y -128);
      }

      if((224<=z)&&(z<=239))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         ucs4text[num_chars] = (z-224)*4096 + (y -128)*64 + (x-128);
      }

      if((240<=z)&&(z<=247))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         iii++; w = text[iii];
         ucs4text[num_chars] = (z-240)*262144 + (y -128)*4096 + (x-128)*64 + (w-128);
      }

      if((248<=z)&&(z<=251))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         iii++; w = text[iii];
         iii++; v = text[iii];
         ucs4text[num_chars] = (z-248)*16777216 + (y -128)*262144 + (x-128)*4096 + (w-128)*64 +(v-128);
      }

      if((252==z)||(z==253))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         iii++; w = text[iii];
         iii++; v = text[iii];
         u = text[iii];
         ucs4text[num_chars] = (z-252)*1073741824 + (y -128)*16777216   + (x-128)*262144 + (w-128)*4096 +(v-128)*64 + (u-128);
      }

      if((z==254)||(z==255))
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem with character: invalid UTF-8 data.\"]\n");
         exit(1);
      }
      
      iii++;
      num_chars++;
   }


   // num_chars now contains the number of characters in the string.
   

   /* Initialize FT Library object */

   error = FT_Init_FreeType( &library );

   if (error)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Could not init Library.\"]\n");
      exit(1);
   }


   /* Initialize FT face object */

   error = FT_New_Face( library, face_path, 0, &face );

   if (error == FT_Err_Unknown_File_Format)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Font was opened, but type not supported.\"]\n");
      exit(1);
   }
   else if (error)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Could not find or load font file.\"]\n");
      exit(1);
   }


   /* Set the Char size */

   error = FT_Set_Char_Size( face,          /* handle to face object           */
                             0,             /* char_width in 1/64th of points  */
                             fontsize*64,   /* char_height in 1/64th of points */
                             100,           /* horizontal device resolution    */
                             100 );         /* vertical device resolution      */
   if (error)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Set char size error.\"]\n");
      exit(1);
   }


   /* Does the font file support kerning? */

   use_kerning = FT_HAS_KERNING( face );

   int n;

   ncorner = 0;

   for ( n = 0; n < num_chars; n++ )
   {
      /* Convert character code to glyph index */

      glyph_index = FT_Get_Char_Index( face, ucs4text[n] );


      /* Get non-rotated glyph offset (X advance value) */

      matrix.xx = (FT_Fixed)(1.0*0x10000);
      matrix.xy = (FT_Fixed)(0.0*0x10000);
      matrix.yx = (FT_Fixed)(0.0*0x10000);
      matrix.yy = (FT_Fixed)(1.0*0x10000);

      FT_Set_Transform( face, &matrix, &pen );
      FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );

      xadvance = face->glyph->advance.x;


      /* Find the angle for this character (calculated at center of character) */

      npath_ang = npath;

      x1_ang = x1;
      y1_ang = y1;

      x2_ang = x2;
      y2_ang = y2;

      path_length2_ang = path_length2;

      while(path_length2_ang < offset+length+xadvance/64./2.)
      {
         ++npath_ang;
         if(npath_ang >= npt)
            break;

         x1_ang = x2_ang;
         y1_ang = y2_ang;

         x2_ang = xcurve[npath_ang];
         y2_ang = ycurve[npath_ang];

         path_length2_ang += sqrt((x2_ang-x1_ang)*(x2_ang-x1_ang) + (y2_ang-y1_ang)*(y2_ang-y1_ang));
      }

      angle = atan2((y2_ang-y1_ang), (x2_ang-x1_ang));


      /* Retrieve kerning distance (if any) and move pen position */

      delx = 0.;
      dely = 0.;

      if ( use_kerning && previous && glyph_index )
      {
         FT_Vector  delta;
         FT_Get_Kerning( face,
                         previous,
                         glyph_index,
                         ft_kerning_default, //FT_KERNING_DEFAULT,
                         &delta );


         /* Transform this kerning distance into rotated space */

         pen.x += (int) (((double) delta.x)*cos(angle));
         pen.y += (int) (((double) delta.x)*sin(angle));

         if(pen.x >= 64)
         {
            ++xpix;
            pen.x -= 64;
         }

         if(pen.y >= 64)
         {
            ++ypix;
            pen.y -= 64;
         }

         delx = ((double) delta.x)*cos(angle);
         dely = ((double) delta.y)*sin(angle);
      }


      /* Set transform */

      matrix.xx = (FT_Fixed)( cos(angle)*0x10000);
      matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);
      matrix.yx = (FT_Fixed)( sin(angle)*0x10000);
      matrix.yy = (FT_Fixed)( cos(angle)*0x10000);

      FT_Set_Transform( face, &matrix, &pen );


      /* Retrieve glyph index from character code */

      glyph_index = FT_Get_Char_Index( face, ucs4text[n] );


      /* Load glyph image (erase previous one) */

      error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );

      if (error)
      {
         printf("[struct stat=\"ERROR\", msg=\"FreeType: Could not load glyph (in loop).\"]\n");
         exit(1);
      }


      /* Convert to an anti-aliased bitmap */

      error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );

      if (error)
      {
         printf("[struct stat=\"ERROR\", msg=\"FreeType: Render glyph error.\"]\n");
         exit(1);
      }


      /* Gather the four corners of the character */

      xcorner[ncorner] = xpix + face->glyph->bitmap_left;
      ycorner[ncorner] = ypix + face->glyph->bitmap_top;
      ++ncorner;

      xcorner[ncorner] = xpix + face->glyph->bitmap_left + face->glyph->bitmap.width + 1;
      ycorner[ncorner] = ypix + face->glyph->bitmap_top;
      ++ncorner;

      xcorner[ncorner] = xpix + face->glyph->bitmap_left + face->glyph->bitmap.width + 1;
      ycorner[ncorner] = ypix + face->glyph->bitmap_top - face->glyph->bitmap.rows - 1;
      ++ncorner;

      xcorner[ncorner] = xpix + face->glyph->bitmap_left;
      ycorner[ncorner] = ypix + face->glyph->bitmap_top - face->glyph->bitmap.rows - 1;
      ++ncorner;


      /* Now, draw to our target surface */

      mViewer_draw_bitmap(&face->glyph->bitmap,
                           xpix + face->glyph->bitmap_left,
                           ypix + face->glyph->bitmap_top,
                           red, green, blue, fontsize, angle);


      /* Advance to the next position */

      delx += face->glyph->advance.x;
      dely += face->glyph->advance.y;

      dl = sqrt((delx/64.)*(delx/64.) + (dely/64.)*(dely/64.));

      length += dl;


      /* Find the start location for the next character */

      while(path_length2 < offset+length)
      {
         ++npath;
         if(npath >= npt)
            break;

         x1 = x2;
         y1 = y2;

         x2 = xcurve[npath];
         y2 = ycurve[npath];

         path_length1  = path_length2;
         path_length2 += sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
      }


      /* Interpolate the current curve segment */
   
      frac = (offset+length - path_length1) / (path_length2 - path_length1);
   
      xchar = x1 + frac * (x2 - x1);
      ychar = y1 + frac * (y2 - y1);

      xpix = floor(xchar+xoff);
      ypix = floor(ychar+yoff);

      pen.x = (xchar + xoff - (double)xpix) * 64.;
      pen.y = (ychar + yoff - (double)ypix) * 64.;

      xoff =  0.4 * (double)fontsize * sin(angle);
      yoff = -0.4 * (double)fontsize * cos(angle);


      /* record current glyph index (needed for kerning next glyph) */

      previous = glyph_index;
   }


   /* The character corners collected above form a set for which we can determine a convex hull */

   mViewer_convexHull(ncorner, xcorner, ycorner, &nhull, xhull, yhull);

   mViewer_boundingbox(nhull, xhull, yhull, a, b, c, xbox, ybox);

   if(mViewer_drawing_debug)
   {
      printf("\n");
      printf("data = [\n");

      for(i=0; i<ncorner; ++i)
      {
         if(i==ncorner-1)
            printf("[%d, %d]];\n", xcorner[i], ycorner[i]);
         else
            printf("[%d, %d],\n", xcorner[i], ycorner[i]);
      }

      printf("\n");
      printf("hull = [\n");

      for(i=0; i<nhull; ++i)
      {
         if(i==nhull-1)
            printf("[ %d, %d]];\n", xhull[i], yhull[i]);
         else
            printf("[ %d, %d],\n", xhull[i], yhull[i]);
      }

      printf("\n");
      printf("box = [\n");

      for(i=0; i<4; ++i)
      {
         if(i==3)
            printf("[ %d, %d]];\n", (int)(xbox[i]+0.5), (int)(ybox[i]+0.5));
         else
            printf("[ %d, %d],\n", (int)(xbox[i]+0.5), (int)(ybox[i]+0.5));
      }

      printf("\n");
      fflush(stdout);
   }


   if(mViewer_drawing_debug)
   {
      for(i=0; i<4; ++i)
      {
         inext = (i+1)%4;

         mViewer_smooth_line (xbox[i], ybox[i], xbox[inext], ybox[inext], red, green, blue, 9.);
      }
   }

   xmin = xbox[0];
   ymin = ybox[0];
   xmax = xbox[0];
   ymax = ybox[0];

   for(k=0; k<4; ++k)
   {
      if(xbox[k] < xmin) xmin = xbox[k];
      if(ybox[k] < ymin) ymin = ybox[k];
      if(xbox[k] > xmax) xmax = xbox[k];
      if(ybox[k] > ymax) ymax = ybox[k];
   }

   xcenter = (xbox[0] + xbox[1] + xbox[2] + xbox[3])/4.;
   ycenter = (ybox[0] + ybox[1] + ybox[2] + ybox[3])/4.;

   for(k=0; k<4; ++k)
      sgn[k] = a[k]*xcenter + b[k]*ycenter + c[k];

   for(j=ymin; j<=ymax; ++j)
   {
      for(i=xmin; i<=xmax; ++i)
      {
         inside = 1;

         for(k=0; k<4; ++k)
         {
            val = a[k]*i + b[k]*j + c[k];

            if(sgn[k] * val <= 0.)
            {
               inside = 0;
               break;
            }
         }

         if (inside)
            mViewer_lockPixel(i, j);
      }
   }


   /* Free the face and the library objects */

   FT_Done_Face    ( face );
   FT_Done_FreeType( library );


   /* Draw the subsegment of the curve following the last character */

   if(showLine && npath < npt)
   {
      mViewer_smooth_line(xchar, ychar, x2, y2, red, green, blue, linewidth);
   }


   /* Finally, draw the remainder of the curve */

   while(1)
   {
      if(showLine)
      {
         mViewer_smooth_line(x1, y1, x2, y2, red, green, blue, linewidth);
      }

      ++npath;
      if(npath >= npt)
         break;

      x1 = x2;
      y1 = y2;

      x2 = xcurve[npath];
      y2 = ycurve[npath];
   }

   free(ucs4text);
}


/***************************************************/
/*                                                 */
/* Routine to draw a simple curve on image.        */
/*                                                 */
/***************************************************/

void mViewer_curve(double *xcurve, double *ycurve, int npt,  
                   double red, double green, double blue, double linewidth)
{
   int    i;

   if(npt < 2)
      return;

   i = 1;

   while(1)
   {
      // if(fabs(xcurve[i]-xcurve[i-1]) < 10.)
         mViewer_smooth_line(xcurve[i-1], ycurve[i-1], xcurve[i], ycurve[i], red, green, blue, linewidth);

      ++i;
      if(i >= npt)
         break;
   }
}



/********************************************************/
/*                                                      */
/* Use a single font bitmap to update the image overlay */
/*                                                      */
/********************************************************/

void mViewer_draw_bitmap( FT_Bitmap * bitmap, int x, int y, double red, double green, double blue, int fontsize, double angle)
{
   int    i, j, size;
   double temp;

   size = fontsize;

   if(bitmap->width > size) 
      size = bitmap->width;

   if(bitmap->rows > size) 
      size = bitmap->rows;

   size = (double)size * 1.415/2.;


   // Turn on the pixels in the image covered by the (non-zero) bitmap locations

   for(j=1; j<bitmap->rows+1; j++)
   {
      for(i=1; i<bitmap->width+1; i++)
      {
         temp = (double)(bitmap->buffer[(j-1)*bitmap->width + (i-1)] )/255.0;

         if(temp)
         {
            mViewer_setPixel(x + i, y - j, temp, red, green, blue, 1);
         }
      }
   }


   // Lock the area covered by the bitmap so that
   // other drawings (mainly the coordinated grid lines)
   // do not overwrite them.
   
   for(j=0; j<bitmap->rows+5; j++)
   {
      for(i=0; i<bitmap->width+5; i++)
         mViewer_lockPixel(x+i-2, y-j-2);
   }

   
   // Draw character bounding box (for debugging purposes 
   // and the character bounding circle                  

   if(mViewer_drawing_debug)
   {
      for(j=0; j<bitmap->rows+2; j++)
      {
         mViewer_setPixel(x,                     y - j, 1., 1., 1., 1., 1);
         mViewer_setPixel(x + bitmap->width + 1, y - j, 1., 1., 1., 1., 1);
      }

      for(i=0; i<bitmap->width+2; i++)
      {
         mViewer_setPixel(x + i, y,                    1., 1., 1., 1., 1);
         mViewer_setPixel(x + i, y - bitmap->rows - 1, 1., 1., 1., 1., 1);
      }
   }
}


/**************************************************/
/*                                                */
/* Anti-aliased thin line drawing routine         */
/*                                                */
/**************************************************/


void mViewer_smooth_line(double x1, double y1, 
                         double x2, double y2,
                         double red, double green, double blue,
                         double linewidth)
{
   int    x, y, ix1, ix2, iy1, iy2;

   double grad, xd, yd, temp;
   double xend, yend, xf, yf;
   double brightness1, brightness2;

   if(linewidth != 1.)
   {
      mViewer_thick_line(x1, y1, x2, y2, red, green, blue, linewidth);
      return;
   }


   /* Extent of line in X and Y */

   xd = x2 - x1;
   yd = y2 - y1;


   /* 'Horizontal' lines */

   if(fabs(xd) > fabs(yd))
   {
      /* Make sure X is increasing */

      if(x1 > x2)
      {
         temp = x1; x1 = x2; x2 = temp;
         temp = y1; y1 = y2; y2 = temp;

         xd = -xd;
         yd = -yd;
      }

      grad = yd / xd;


      /* First end point */

      xend = floor(x1 + 0.5);
      yend = y1 + grad * (xend-x1);

      ix1 = (int)(floor(xend));
      iy1 = (int)(floor(yend));

      brightness1 = mViewer_invfrac(yend);
      brightness2 =    mViewer_frac(yend);

      mViewer_setPixel(ix1, iy1,   brightness1, red, green, blue, 0);
      mViewer_setPixel(ix1, iy1+1, brightness2, red, green, blue, 0);

      yf = yend + grad;


      /* Second end point */

      xend = floor(x2 + 0.5);
      yend = y2 + grad * (xend-x2);

      ix2 = (int)(floor(xend));
      iy2 = (int)(floor(yend));

      brightness1 = mViewer_invfrac(yend);
      brightness2 =    mViewer_frac(yend);

      mViewer_setPixel(ix2, iy2,   brightness1, red, green, blue, 0);
      mViewer_setPixel(ix2, iy2+1, brightness2, red, green, blue, 0);


      /* Loop over intermediate pixels */

      if(abs(ix2-ix1) < 5 && abs(iy2-iy1) < 5)
      {
         for(x=ix1+1; x<ix2; ++x)
         {
            brightness1 = mViewer_invfrac(yf);
            brightness2 =    mViewer_frac(yf);

            mViewer_setPixel(x, floor(yf),   brightness1, red, green, blue, 0);
            mViewer_setPixel(x, floor(yf)+1, brightness2, red, green, blue, 0);

            yf += grad; 
         }
      }
   }


   /* 'Vertical' lines */

   else
   {
      /* Make sure Y is increasing */

      if(y1 > y2)
      {
         temp = x1; x1 = x2; x2 = temp;
         temp = y1; y1 = y2; y2 = temp;

         xd = -xd;
         yd = -yd;
      }

      grad = xd / yd;


      /* First end point */

      yend = floor(y1 + 0.5);
      xend = x1 + grad * (yend-y1);

      ix1 = (int)(floor(xend));
      iy1 = (int)(floor(yend));

      brightness1 = mViewer_invfrac(xend);
      brightness2 =    mViewer_frac(xend);

      mViewer_setPixel(ix1,   iy1, brightness1, red, green, blue, 0);
      mViewer_setPixel(ix1+1, iy1, brightness2, red, green, blue, 0);

      xf = xend + grad;


      /* Second end point */

      yend = floor(y2 + 0.5);
      xend = x2 + grad * (yend-y2);

      ix2 = (int)(floor(xend));
      iy2 = (int)(floor(yend));

      if(abs(ix2-ix1) < 5 && abs(iy2-iy1) < 5)
      {
         brightness1 = mViewer_invfrac(xend);
         brightness2 =    mViewer_frac(xend);

         mViewer_setPixel(ix2,   iy2, brightness1, red, green, blue, 0);
         mViewer_setPixel(ix2+1, iy2, brightness2, red, green, blue, 0);


         /* Loop over intermediate pixels */

         for(y=iy1+1; y<iy2; ++y)
         {
            brightness1 = mViewer_invfrac(xf);
            brightness2 =    mViewer_frac(xf);

            mViewer_setPixel(floor(xf),   y, brightness1, red, green, blue, 0);
            mViewer_setPixel(floor(xf)+1, y, brightness2, red, green, blue, 0);

            xf += grad; 
         }
      }
   }
}



/*************************************************/
/*                                               */
/* Thick anti-aliased line drawing routine       */
/*                                               */
/* We are going to use a full analytic solution  */
/* based on a filled polygon algorithm.          */
/*                                               */
/*************************************************/

void mViewer_thick_line(double x1, double y1, 
                        double x2, double y2,
                        double red, double green, double blue,
                        double width)
{
   int    isub, jsub;
   int    i, j, jstart, jend, istart, iend;
   int    horizontal, ncrossing, is, ie;

   double grad, xd, yd, temp, fs, fe;
   double theta, deltax, deltay;

   double x1off1, y1off1, y2off1;
   double x1off2, y1off2, y2off2;

   double dy, dx2, jy;

   double xcrossing[8];

   char   **array;
   double **farray;

   int    nxorig, nyorig;
   int    nxfine, nyfine;

   int    nsamp, narray;

   nsamp = 3;

   narray = nsamp * nsamp;


   if(x1 == x2 && y1 == y2)
      return;


   /* Make sure X is increasing */

   if(x1 > x2)
   {
      temp = x1; x1 = x2; x2 = temp;
      temp = y1; y1 = y2; y2 = temp;
   }


   /* Our thick line is a box of the given width with the          */
   /* points (x1,y1) and (x2,y2) at the mid-points of the          */
   /* "short" (width length) sides and two circles of diameter     */
   /* 'width' centered on these two points.  In reality, it        */
   /* is possible that the length of the segment is smaller        */
   /* than the width but this has no operational effect.           */
   /* We only worry about the half circles extending beyond        */
   /* the box and the two sides of the box running the             */
   /* 'length' of the segment (the other parts are interior        */
   /* to the segment and don't represent region edge crossings.    */

   /* The 'region of interest' (set of horizontal lines that might */
   /* intersect with the segment) are y-values between y1-width/2. */
   /* and y2+witdh/2. (or the same with opposite sign depending    */
   /* on the slope of the line).                                   */

   istart = (int)(x1 - width/2.)-2;
   iend   = (int)(x2 + width/2.)+2;

   if(istart < 0)
      istart = 0;

   if(y1 >= y2)
   { 
      jstart = (int)(y2 - width/2.)-2;
      jend   = (int)(y1 + width/2.)+2;
   }
   else
   { 
      jstart = (int)(y1 - width/2.)-2;
      jend   = (int)(y2 + width/2.)+2;
   }

   if(jstart < 0)
      jstart = 0;

   nxorig = abs(iend - istart + 2);
   nyorig = abs(jend - jstart + 2);

   nxfine = nsamp * abs(iend - istart + 2);
   nyfine = nsamp * abs(jend - jstart + 2);

   array = (char **)malloc(nyfine * sizeof(char *));

   for(j=0; j<nyfine; ++j)
   {
      array[j] = (char *)malloc((nxfine+1)*sizeof(char));

      for(i=0; i<nxfine; ++i)
         array[j][i] = '.';

      array[j][nxfine] = '\n';
   }


   farray = (double **)malloc(nyorig * sizeof(double *));

   for(j=0; j<nyorig; ++j)
   {
      farray[j] = (double *)malloc((nxorig+1)*sizeof(double));

      for(i=0; i<nxorig; ++i)
         farray[j][i] = 0.;
   }


   // Direction angle for width offset

   xd = x2 - x1;
   yd = y2 - y1;

   theta = atan2(yd, xd);

   deltax = -width/2. * sin(theta);
   deltay =  width/2. * cos(theta);


   // Line segments along two box edges

   x1off1 = x1 + deltax;
   y1off1 = y1 + deltay;

   y2off1 = y2 + deltay;

   x1off2 = x1 - deltax;
   y1off2 = y1 - deltay;

   y2off2 = y2 - deltay;


   // Seee if the segment is more 'horizontal' or 'vertical'

   horizontal = 0;

   if(fabs(x2-x1) > fabs(y2-y1))
      horizontal = 1;

   if(horizontal)
      grad = yd / xd;
   else
      grad = xd / yd;


   /* For each horizontal line ... */

   for(j=0; j<nyfine; ++j)
   {
      /* Check the "left" circle.  We keep both crossings though the second will */
      /* often be deleted later if there are crossings of the box.               */

      jy = (j + 0.5)/(double)nsamp + jstart - 0.5;

      ncrossing = 0;

      if(jy >= y1-width/2. && jy <= y1+width/2.)
      {
         dy = jy - y1;

         dx2 = width*width/4.- dy*dy;

         if(dx2 > 0.)
         {
            xcrossing[0] = x1 - sqrt(dx2);
            xcrossing[1] = x1 + sqrt(dx2);

            ncrossing = 2;
         }
      }


      /* Check the box, but just the "length" edges.  If either of the other */
      /* two edges were crossed, the corresponding circle would be as well.  */

      if((y1off1 > y2off1 && jy <= y1off1 && jy >= y2off1)
      || (y1off1 < y2off1 && jy >= y1off1 && jy <= y2off1))
      {
         if(horizontal)
            xcrossing[ncrossing] = (jy - y1off1) / grad + x1off1;
         else
            xcrossing[ncrossing] = (jy - y1off1) * grad + x1off1;

         ++ncrossing; 
      }

      if((y1off2 > y2off2 && jy <= y1off2 && jy >= y2off2)
      || (y1off2 < y2off2 && jy >= y1off2 && jy <= y2off2))
      {
         if(horizontal)
            xcrossing[ncrossing] = (jy - y1off2) / grad + x1off2;
         else
            xcrossing[ncrossing] = (jy - y1off2) * grad + x1off2;

         ++ncrossing;
      }


      /* Check the "right" circle.  We keep both crossings though the second will */
      /* often be deleted later if there are crossings of the box.               */

      if(jy >= y2-width/2. && jy <= y2+width/2.)
      {
         dy = jy - y2;

         dx2 = width*width/4. - dy*dy;

         if(dx2 > 0.)
         {
            xcrossing[ncrossing] = x2 - sqrt(dx2);
            ++ncrossing;

            xcrossing[ncrossing] = x2 + sqrt(dx2);
            ++ncrossing;
         }
      }

      if(ncrossing < 2)
         continue;
   
      fs = xcrossing[0];
      fe = xcrossing[0];

      for(i=0; i<ncrossing; ++i)
      {
         if(xcrossing[i] < fs) fs = xcrossing[i];
         if(xcrossing[i] > fe) fe = xcrossing[i];
      }

      is = nsamp * (fs - istart + 0.5) - 0.5;
      ie = nsamp * (fe - istart + 0.5) - 0.5;

      if(is < 0) is = 0;
      if(ie < 0) ie = 0;

      if(is >= nxfine) is = nxfine;
      if(ie >= nxfine) ie = nxfine;

      for(i=is; i<=ie; ++i)
         array[j][i] = '+';
   }


   for(i=0; i<nxorig; ++i)
   {
      for(j=0; j<nyorig; ++j)
      {
         for(isub=0; isub<nsamp; ++isub)
         {
            for(jsub=0; jsub<nsamp; ++jsub)
            {
               if(array[j*nsamp+jsub][i*nsamp+isub] == '+')
                  farray[j][i] = farray[j][i] + 1./narray;
            }
         }
      }
   }

   for(j=0; j<nyfine; ++j)
      free(array[j]);

   free(array);

   for(j=nyorig-1; j>=0; --j)
   {
      for(i=0; i<nxorig; ++i)
      {
         if(farray[j][i] > 0.)
         {
            mViewer_setPixel(i+istart, j+jstart, farray[j][i], red, green, blue, 0);
         }
      }
   }

   for(j=0; j<nyorig; ++j)
      free(farray[j]);

   free(farray);
}



double mViewer_frac(double x)
{
   double val;

   val = x - floor(x);

   return(val);
}


double mViewer_invfrac(double x)
{
   double val;

   val = x - floor(x);

   val = 1.0 - val;

   return(val);
}



/*****************************************************/
/*                                                   */
/* Routine to get the length of a string in pixels.  */
/*                                                   */
/*****************************************************/

double mViewer_label_length( char *face_path, int fontsize, char *text)
{
   FT_Library  library;
   FT_Face     face;
   FT_Matrix   matrix;      // transformation matrix
   FT_Vector   pen;

   FT_UInt glyph_index;
   FT_Error error;

   double string_length;
   double xadvance;


   /*Count the length of the string */

   int num_bytes=0;
   while(text[num_bytes]!=0)
        num_bytes++;


   //The array of ucs4 glyph indexes, which will by at most the number of bytes in the utf-8 file.

   long * ucs4text;

   ucs4text = malloc((num_bytes+1) * sizeof(long));

   unsigned char u,v,w,x,y,z;

   int num_chars=0;

   long iii=0;

   while(iii<num_bytes)
   {
      z = text[iii];

      if(z<=127)
      {
         ucs4text[num_chars] = z;
      }

      if((192<=z)&&(z<=223))
      {
         iii++; y = text[iii];
         ucs4text[num_chars] = (z-192)*64 + (y -128);
      }

      if((224<=z)&&(z<=239))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         ucs4text[num_chars] = (z-224)*4096 + (y -128)*64 + (x-128);
      }

      if((240<=z)&&(z<=247))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         iii++; w = text[iii];
         ucs4text[num_chars] = (z-240)*262144 + (y -128)*4096 + (x-128)*64 + (w-128);
      }

      if((248<=z)&&(z<=251))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         iii++; w = text[iii];
         iii++; v = text[iii];
         ucs4text[num_chars] = (z-248)*16777216 + (y -128)*262144 + (x-128)*4096 + (w-128)*64 +(v-128);
      }

      if((252==z)||(z==253))
      {
         iii++; y = text[iii];
         iii++; x = text[iii];
         iii++; w = text[iii];
         iii++; v = text[iii];
         u = text[iii];
         ucs4text[num_chars] = (z-252)*1073741824 + (y -128)*16777216   + (x-128)*262144 + (w-128)*4096 +(v-128)*64 + (u-128);
      }

      if((z==254)||(z==255))
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem with character: invalid UTF-8 data.\"]\n");
         exit(1);
      }
      
      iii++;
      num_chars++;
   }


   // num_chars now contains the number of characters in the string.
   

   /* Initialize FT Library object */

   error = FT_Init_FreeType( &library );

   if (error)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Could not init Library.\"]\n");
      exit(1);
   }


   /* Initialize FT face object */

   error = FT_New_Face( library, face_path, 0, &face );

   if (error == FT_Err_Unknown_File_Format)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Font was opened, but type not supported.\"]\n");
      exit(1);
   }
   else if (error)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Could not find or load font file.\"]\n");
      exit(1);
   }


   /* Set the Char size */

   error = FT_Set_Char_Size( face,          /* handle to face object           */
                             0,             /* char_width in 1/64th of points  */
                             fontsize*64,   /* char_height in 1/64th of points */
                             100,           /* horizontal device resolution    */
                             100 );         /* vertical device resolution      */
   if (error)
   {
      printf("[struct stat=\"ERROR\", msg=\"FreeType: Set char size error.\"]\n");
      exit(1);
   }


   int n;

   string_length = 0.;

   for ( n = 0; n < num_chars; n++ )
   {
      /* Convert character code to glyph index */

      glyph_index = FT_Get_Char_Index( face, ucs4text[n] );


      /* Get non-rotated glyph offset (X advance value) */

      matrix.xx = (FT_Fixed)(1.0*0x10000);
      matrix.xy = (FT_Fixed)(0.0*0x10000);
      matrix.yx = (FT_Fixed)(0.0*0x10000);
      matrix.yy = (FT_Fixed)(1.0*0x10000);

      FT_Set_Transform( face, &matrix, &pen );
      FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );

      xadvance = face->glyph->advance.x;

      string_length += xadvance/64.;
   }


   /* Free the face and the library objects */

   FT_Done_Face    ( face );
   FT_Done_FreeType( library );

   free(ucs4text);

   return(string_length);
}
