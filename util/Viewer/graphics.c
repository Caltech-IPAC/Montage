#include <stdio.h>
#include <math.h>
#include <ft2build.h>
#include <freetype/freetype.h>

void labeled_curve (char *face_path, int fontsize, int showLine,
                    double *xcurve, double *ycurve, int npt,  
                    char *text, double offset, 
                    double red, double green, double blue);

void curve         (double *xcurve, double *ycurve, int npt,  
                    double red, double green, double blue);

double frac        (double x);
double invfrac     (double x);

void smooth_line   (double x1, double y1, 
                    double x2, double y2,
                    double red, double green, double blue);

void draw_bitmap   (FT_Bitmap * bitmap, int x, int y, double red, double green, double blue, int fontsize );

int  setPixel      (int i, int j, double brightness, double red, double green, double blue, int force);
int  getPixel      (int i, int j, int color);

double label_length(char *face_path, int fontsize, char *text);


/****************************************************/
/*                                                  */
/* Routine to draw a labeled curve on image.        */
/* The text is written along the curve of the data. */
/* Before and after the text, the curve is drawn.   */
/*                                                  */
/****************************************************/

void labeled_curve( char *face_path, int fontsize, int showLine,
                    double *xcurve, double *ycurve, int npt,  
                    char *text, double offset, 
                    double red, double green, double blue)
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
   double path_length1_ang, path_length2_ang;

   double xchar, ychar;
   double delx, dely;
   double xadvance;
   double angle, dl, frac, length;
   double xoff, yoff;
   double dtr;

   int xpix, ypix;
   int ii, jj;

   dtr = atan(1.)/45.;

   if(npt < 2)
      return;


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
         smooth_line(x1, y1, x2, y2, red, green, blue);
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
      smooth_line(x1, y1, xchar, ychar, red, green, blue);
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

         path_length1_ang  = path_length2_ang;
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


      /* Now, draw to our target surface */

      draw_bitmap( &face->glyph->bitmap,
                      xpix + face->glyph->bitmap_left,
                      ypix + face->glyph->bitmap_top,
                      red, green, blue, fontsize);


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

      xpix = floor(xchar);
      ypix = floor(ychar);


      /* Draw character reference point (for debugging purposes) **

      for(ii=xpix-1; ii<=xpix+1; ++ii)
         for(jj=ypix-1; jj<=ypix+1; ++jj)
            setPixel(ii, jj, 1., 0., 0., 1., 1);

      ***/

      xpix = floor(xchar+xoff);
      ypix = floor(ychar+yoff);

      pen.x = (xchar + xoff - (double)xpix) * 64.;
      pen.y = (ychar + yoff - (double)ypix) * 64.;

      xoff =  0.4 * (double)fontsize * sin(angle);
      yoff = -0.4 * (double)fontsize * cos(angle);


      /* record current glyph index (needed for kerning next glyph) */

      previous = glyph_index;
   }


   /* Free the face and the library objects */

   FT_Done_Face    ( face );
   FT_Done_FreeType( library );


   /* Draw the subsegment of the curve following the last character */

   if(showLine && npath < npt)
   {
      smooth_line(xchar, ychar, x2, y2, red, green, blue);
   }


   /* Finally, draw the remainder of the curve */

   while(1)
   {
      if(showLine)
      {
         smooth_line(x1, y1, x2, y2, red, green, blue);
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

void curve(double *xcurve, double *ycurve, int npt,  
           double red, double green, double blue)
{
   int    i;

   if(npt < 2)
      return;

   i = 1;

   while(1)
   {
      if(fabs(xcurve[i]-xcurve[i-1]) < 10.)
      smooth_line(xcurve[i-1], ycurve[i-1], xcurve[i], ycurve[i], red, green, blue);

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

void draw_bitmap( FT_Bitmap * bitmap, int x, int y, double red, double green, double blue, int fontsize)
{
   int    i, j;
   double temp;

   for(j=-2; j<fontsize+3; j++)
   {
      for(i=-2; i<bitmap->width+3; i++)
      {
         setPixel(x + i, y - j, 0., 0., 0., 0., 0);
      }
   }

   for(j=1; j<bitmap->rows+1; j++)
   {
      for(i=1; i< bitmap->width+1; i++)
      {
         temp = (double)(bitmap->buffer[(j-1)*bitmap->width + (i-1)] )/255.0;

         if(temp)
              setPixel(x + i, y - j, temp, red, green, blue, 1);
         else
            setPixel(x + i, y - j, 0., 0., 0., 0., 0);
      }
   }

   /* Draw character bounding box (for debugging purposes **

   for(j=0; j<bitmap->rows+2; j++)
   {
      setPixel(x,                     y - j, 1., 1., 1., 1., 1);
      setPixel(x + bitmap->width + 1, y - j, 1., 1., 1., 1., 1);
   }

   for(i=0; i<bitmap->width+2; i++)
   {
      setPixel(x + i, y,                    1., 1., 1., 1., 1);
      setPixel(x + i, y - bitmap->rows - 1, 1., 1., 1., 1., 1);
   }

   ***/
}



/**************************************************/
/*                                                */
/* Anti-aliased line drawing routine              */
/*                                                */
/**************************************************/

void smooth_line(double x1, double y1, 
                 double x2, double y2,
                 double red, double green, double blue)
{
   int    x, y, ix1, ix2, iy1, iy2;

   double grad, xd, yd, temp;
   double xend, yend, xf, yf;
   double brightness1, brightness2;


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

      brightness1 = invfrac(yend);
      brightness2 =    frac(yend);

      setPixel(ix1, iy1,   brightness1, red, green, blue, 0);
      setPixel(ix1, iy1+1, brightness2, red, green, blue, 0);

      yf = yend + grad;


      /* Second end point */

      xend = floor(x2 + 0.5);
      yend = y2 + grad * (xend-x2);

      ix2 = (int)(floor(xend));
      iy2 = (int)(floor(yend));

      brightness1 = invfrac(yend);
      brightness2 =    frac(yend);

      setPixel(ix2, iy2,   brightness1, red, green, blue, 0);
      setPixel(ix2, iy2+1, brightness2, red, green, blue, 0);


      /* Loop over intermediate pixels */

      for(x=ix1+1; x<ix2; ++x)
      {
         brightness1 = invfrac(yf);
         brightness2 =    frac(yf);

         setPixel(x, floor(yf),   brightness1, red, green, blue, 0);
         setPixel(x, floor(yf)+1, brightness2, red, green, blue, 0);

         yf += grad; 
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

      brightness1 = invfrac(xend);
      brightness2 =    frac(xend);

      setPixel(ix1,   iy1, brightness1, red, green, blue, 0);
      setPixel(ix1+1, iy1, brightness2, red, green, blue, 0);

      xf = xend + grad;


      /* Second end point */

      yend = floor(y2 + 0.5);
      xend = x2 + grad * (yend-y2);

      ix2 = (int)(floor(xend));
      iy2 = (int)(floor(yend));

      brightness1 = invfrac(xend);
      brightness2 =    frac(xend);

      setPixel(ix2,   iy2, brightness1, red, green, blue, 0);
      setPixel(ix2+1, iy2, brightness2, red, green, blue, 0);


      /* Loop over intermediate pixels */

      for(y=iy1+1; y<iy2; ++y)
      {
         brightness1 = invfrac(xf);
         brightness2 =    frac(xf);

         setPixel(floor(xf),   y, brightness1, red, green, blue, 0);
         setPixel(floor(xf)+1, y, brightness2, red, green, blue, 0);

         xf += grad; 
      }
   }
}



double frac(double x)
{
   double val;

   val = x - floor(x);

   return(val);
}


double invfrac(double x)
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

double label_length( char *face_path, int fontsize, char *text)
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
