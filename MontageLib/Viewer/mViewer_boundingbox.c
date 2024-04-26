#include <stdio.h> 
#include <stdlib.h> 
#include <math.h> 

int mViewer_bb_debug = 0;


typedef struct xy 
{ 
	int x, y; 
} 
   Point; 


// A global point needed for sorting points with reference 
// to the first point Used in compare function of qsort() 

Point p0; 


// Stack of points and push/pop functions

Point S[1024];
int   ns;

void mViewer_push(Point p)
{
   S[ns].x = p.x;
   S[ns].y = p.y;

   ++ns;
}


Point mViewer_pop()
{
   Point p;

   p.x = S[ns-1].x;
   p.y = S[ns-1].y;

   --ns;

   return p;
}


Point mViewer_top()
{
   Point p;

   p.x = S[ns-1].x;
   p.y = S[ns-1].y;

   return p;
}


Point mViewer_nextToTop() 
{ 
	Point p; 

   p.x = S[ns-2].x;
   p.y = S[ns-2].y;
	
	return p; 
} 


// A utility function to swap two points 

void mViewer_swap(Point p1, Point p2) 
{ 
	Point temp; 

   temp.x = p1.x;
   temp.y = p1.y;

	p1.x = p2.x; 
	p1.y = p2.y; 
	
	p2.x = temp.x; 
	p2.y = temp.y; 
} 


// A utility function to return square of distance 
// between p1 and p2 

int mViewer_distSq(Point p1, Point p2) 
{ 
	return (p1.x - p2.x) * (p1.x - p2.x) + 
   		 (p1.y - p2.y) * (p1.y - p2.y); 
} 



// To find orientation of ordered triplet (p, q, r). 
// The function returns following values 

// 0 --> p, q and r are colinear 
// 1 --> Clockwise 
// 2 --> Counterclockwise 

int mViewer_orientation(Point p, Point q, Point r) 
{ 
	int val = (q.y - p.y) * (r.x - q.x) - 
			    (q.x - p.x) * (r.y - q.y); 

	if (val == 0)
      return 0; // colinear 

	return (val > 0)? 1: 2; // clock or counterclock wise 
} 


// A function used by library function qsort() to sort an array of 
// points with respect to the first point 

int mViewer_compare(const void *vp1, const void *vp2) 
{ 
   Point *p1 = (Point *)vp1; 
   Point *p2 = (Point *)vp2; 


   // Find orientation 

   int o = mViewer_orientation(p0, *p1, *p2); 

   if (o == 0) 
      return (mViewer_distSq(p0, *p2) >= mViewer_distSq(p0, *p1))? -1 : 1; 

   return (o == 2)? -1: 1; 
} 


// Finds convex hull for a set of n points. 

void mViewer_convexHull(int n, int *xarray, int *yarray, int *nhull, int *xhull, int *yhull) 
{ 
   int i;

   Point points[32768];

   for(i=0; i<n; ++i)
   {
      points[i].x = xarray[i];
      points[i].y = yarray[i];
   } 


   // Find the bottommost point 

   int ymin = points[0].y, min = 0; 

   for (i = 1; i < n; i++) 
   { 
      int y = points[i].y; 


      // Pick the bottom-most or chose the left 
      // most point in case of tie 

      if ((y < ymin) || (ymin == y && points[i].x < points[min].x)) 
      {
         ymin = points[i].y;
         min = i; 
      }
   } 


   // Place the bottom-most point at first position 

   mViewer_swap(points[0], points[min]); 


   // Sort n-1 points with respect to the first point. 
   // A point p1 comes before p2 in sorted ouput if p2 
   // has larger polar angle (in counterclockwise 
   // direction) than p1 

   p0 = points[0]; 

   qsort(&points[1], n-1, sizeof(Point), mViewer_compare); 


   // If two or more points make same angle with p0, 
   // Remove all but the one that is farthest from p0 
   // Remember that, in above sorting, our criteria was 
   // to keep the farthest point at the end when more than 
   // one points have same angle. 

   int m = 1; // Initialize size of modified array 

   for (i=1; i<n; i++) 
   { 
      // Keep removing i while angle of i and i+1 is same 
      // with respect to p0 

      while (i < n-1 && mViewer_orientation(p0, points[i], 
               points[i+1]) == 0) 
         i++; 


      points[m] = points[i]; 

      m++; // Update size of modified array 
   } 


   // If modified array of points has less than 3 points, 
   // convex hull is not possible 

   if (m < 3) return; 


   // "Empty" the stack and push first three points 
   // to it. 

   ns = 0;

   mViewer_push(points[0]); 
   mViewer_push(points[1]); 
   mViewer_push(points[2]); 


   // Process remaining n-3 points 

   for (i = 3; i < m; i++) 
   { 
      // Keep removing top while the angle formed by 
      // points next-to-top, top, and points[i] makes 
      // a non-left turn 

      while (mViewer_orientation(mViewer_nextToTop(), mViewer_top(), points[i]) != 2) 
         mViewer_pop(); 

      mViewer_push(points[i]); 
   } 


   // Now stack has the output points, save them to the output arrays

   *nhull = 0;

   while (ns > 0) 
   { 
      Point p = mViewer_top(); 

      xhull[*nhull] = p.x;
      yhull[*nhull] = p.y;
      ++*nhull;

      mViewer_pop(); 
   } 
} 


void mViewer_boundingbox(int n, int *x, int *y, 
                         double *a, double *b, double *c,
                         double *xcorner, double *ycorner)
{
   int    i, j, inext, iref, imax = 0;
   
   double a0, at;
   double b0, bt;
   double c0, ct;

   double d, denom, dmax, lmin, pad;
   double dleft, dright;
   int    ileft, iright;
   double area, minarea = 0.;

   double sgn, temp;


   // The x,y values are points on a convex hull
   // for the original data.  That is, the minimum
   // bounding polygon for those original points.

   // Here we walk around the convex hull, looking 
   // for the minimum bounding box.  It can be shown
   // that one side of this box will contain one of
   // the segments from the hull, so we try each one.

   // The first step is the find the point on the
   // hull farthest from this initial line, as that
   // will define the other side of the box.

   if(mViewer_bb_debug)
   {
      for(i=0; i<n; ++i)
         printf("%2d: %d %d\n", i, x[i], y[i]);

      fflush(stdout);
   }

   for(i=0; i<n; ++i)
   {
      if(mViewer_bb_debug)
      {
         printf("\nStarting point: %d\n", i);
         fflush(stdout);
      }

      inext = (i+1)%n;


      // The line connecting two adjacent points
      // on the hull is "a0*x + b0*y + c0 = 0"

      a0 =   y[inext] - y[i];
      b0 = -(x[inext] - x[i]);

      c0 = (x[inext]-x[i]) * y[i] - (y[inext]-y[i]) * x[i];

      if(mViewer_bb_debug)
      {
         printf("Line: %-gX + %-gY + %-g = 0\n", a0, b0, c0);
         printf("Confirm point %d (%d,%d): %-g\n", i, x[i], y[i], a0*x[i] + b0*y[i] + c0);
         printf("Confirm point %d (%d,%d): %-g\n", inext, x[inext], y[inext], a0*x[inext] + b0*y[inext] + c0);
         fflush(stdout);
      }


      // Loop over the other n-2 points

      dmax = 0.;

      denom = sqrt(a0*a0 + b0*b0);

      for(j=0; j<n-2; ++j)
      {
         iref = (i+2+j)%n;

         // The parameter 'd' is the distance from the
         // point to our first line.  We want the point
         // with the largest value

         d = fabs(a0 * x[iref] + b0 * y[iref] + c0) / denom;

         if(mViewer_bb_debug)
         {
            printf("Checking point %d (%d,%d) for max distance from line. d = %-g\n", iref, x[iref], y[iref], d);
            fflush(stdout);
         }

         if(d > dmax)
         {
            if(mViewer_bb_debug)
            {
               printf("NEW MAXIMUM DISTANCE\n");
               fflush(stdout);
            }

            dmax = d;
            imax = iref;
         }
      }


      // Now draw a line from the original line through
      // this new point.  The third and fourth sides are
      // going to be defined by the points on the hull 
      // farthest from this new line on either side.

      at = -b0;
      bt =  a0;
      ct = -at*x[imax] - bt*y[imax];

      denom = sqrt(at*at + bt*bt);

      for(j=0; j<n; ++j)
      {
         iref = (i+j)%n;

         // The parameter 'd' is now the (signed) distance 
         // from the point to our second (perpendicular) line.
         // We want the min/max values of 'd'.

         d = (at * x[iref] + bt * y[iref] + ct) / denom;

         if(j==0)
         {
            dleft = d;
            ileft = iref;

            dright = d;
            iright = iref;
         }

         if(d > dleft)
         {
            dleft = d;
            ileft = iref;
         }

         if(d < dright)
         {
            dright = d;
            iright = iref;
         }
      }

      if(mViewer_bb_debug)
      {
         printf("Left/right points: %d (%-g) and %d (%-g)\n", ileft, dleft, iright, dright);
         fflush(stdout);
      }


      // The area for this box is dmax * fabs(dleft - dright).
      // Our ultimate goal is to find the box with the smallest
      // area.

      // What we need at the end are the four corners for the
      // minimum area box, so each time we find a new minimum
      // we calculate these.

      lmin = fabs(dleft - dright);

      area =  dmax * lmin;

      if(mViewer_bb_debug)
      {
         printf("Area: %-g\n", area);
         fflush(stdout);
      }

      if(i==0 || area < minarea)
      {
         if(mViewer_bb_debug)
         {
            printf("NEW MINIMUM AREA\n");
            fflush(stdout);
         }

         minarea  = area;


         // Save line 0

         a[0] = a0;
         b[0] = b0;
         c[0] = c0;


         // Line 2 is parallel to line 0 but going through
         // the perpendicular point.  We will use this general
         // approach for all four corners: slope from a reference
         // line and the 'c' value derived from the offset point.

         a[2] = a[0];
         b[2] = b[0];
         c[2] = -a[2]*x[imax] - b[2]*y[imax];

         
         // Lines one and three are parallel to the perpendicular
         // line but going through points ileft and iright.

         a[1] = at;
         b[1] = bt;
         c[1] = -at*x[ileft] - bt*y[ileft];

         a[3] = at;
         b[3] = bt;
         c[3] = -at*x[iright] - bt*y[iright];


         // Apply an offset to each of the lines equal to half
         // the box "height"

         pad = dmax;

         if(lmin < dmax)
            pad = lmin;

         pad = pad/4.;

         c[0] = c[0] + sqrt(a[0]*a[0] + b[0]*b[0]) * pad;
         c[1] = c[1] - sqrt(a[1]*a[1] + b[1]*b[1]) * pad;
         c[2] = c[2] - sqrt(a[2]*a[2] + b[2]*b[2]) * pad;
         c[3] = c[3] + sqrt(a[3]*a[3] + b[3]*b[3]) * pad;


         // The intersections between line0/line1, line1/line2,
         // line2/line3, and line3/line0 are the four corners
         // of our box.

         xcorner[0] =  (c[1]*b[0] - c[0]*b[1])/(a[0]*b[1] - a[1]*b[0]);
         ycorner[0] = -(c[1]*a[0] - c[0]*a[1])/(a[0]*b[1] - a[1]*b[0]);

         xcorner[1] =  (c[2]*b[1] - c[1]*b[2])/(a[1]*b[2] - a[2]*b[1]);
         ycorner[1] = -(c[2]*a[1] - c[1]*a[2])/(a[1]*b[2] - a[2]*b[1]);

         xcorner[2] =  (c[3]*b[2] - c[2]*b[3])/(a[2]*b[3] - a[3]*b[2]);
         ycorner[2] = -(c[3]*a[2] - c[2]*a[3])/(a[2]*b[3] - a[3]*b[2]);

         xcorner[3] =  (c[0]*b[3] - c[3]*b[0])/(a[3]*b[0] - a[0]*b[3]);
         ycorner[3] = -(c[0]*a[3] - c[3]*a[0])/(a[3]*b[0] - a[0]*b[3]);


         // Make sure the corners are arranged clockwise.  The quantity 
         // we calculate is the length of the z-axis of the vector cross
         // product of the vectors derived from corners 0->1 and corners
         // 0->2.  If this quantity is positive, the corners are arranged
         // clockwise.  Otherwise, swap corners 1 and 3.

         sgn = (xcorner[1]-xcorner[0]) * (ycorner[2]-ycorner[0])
             - (ycorner[1]-ycorner[0]) * (xcorner[2]-xcorner[0]);

         if(sgn > 0)
         {
            temp       = xcorner[1];
            xcorner[1] = xcorner[3];
            xcorner[3] = temp;

            temp       = ycorner[1];
            ycorner[1] = ycorner[3];
            ycorner[3] = temp;
         }
      }
   }
}
