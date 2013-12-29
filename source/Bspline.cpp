/*********************************************************************

 Simple b-spline curve algorithm

 Copyright 1994 by Keith Vertanen (vertankd@cda.mrs.umn.edu)

 Released to the public domain (your mileage may vary)

**********************************************************************/

// #include <graphics.h>
#include <stdlibc>
#include "bspline.h"


/*********************************************************************

Parameters:
  n          - the number of control points minus 1
  t          - the degree of the polynomial plus 1
  control    - control point array made up of point stucture
  output     - array in which the calculate spline points are to be put
  num_output - how many points on the spline are to be calculated

Pre-conditions:
  n+2>t  (no curve results if n+2<=t)
  control array contains the number of points specified by n
  output array is the proper size to hold num_output point structures


**********************************************************************/
void bspline(int n, int t, point *control, point *output, int num_output)

{
  int *u;
  double increment,interval;
  point calcxyz;
  int output_index;

  u=new int[n+t+1];
  compute_intervals(u, n, t);

  increment=(double) (n-t+2)/(num_output-1);  // how much parameter goes up each time
  interval=0;

  for (output_index=0; output_index<num_output-1; output_index++)
  {
    compute_point(u, n, t, interval, control, &calcxyz);
    output[output_index].x = calcxyz.x;
    output[output_index].y = calcxyz.y;
    output[output_index].z = calcxyz.z;
    interval=interval+increment;  // increment our parameter
  }
  output[num_output-1].x=control[n].x;   // put in the last point
  output[num_output-1].y=control[n].y;
  output[num_output-1].z=control[n].z;

  delete u;
}

// void main()
// {
//   int *u;
//   int n,t,i;
//   n=7;          // number of control points = n+1
//   t=4;           // degree of polynomial = t-1

//   point *pts;          // allocate our control point array
//   pts=new point[n+1];
// /*
//   randomize();
//   for (i=0; i<=n; i++)  // assign the control points randomly
//   {
//       (pts[i].x)=random(100)+(i*600/n);
//       (pts[i].y)=random(500);
//       (pts[i].z)=random(500);
//   }
// */
//   pts[0].x=10;  pts[0].y=100;  pts[0].z=0;
//   pts[1].x=200;  pts[1].y=100;  pts[1].z=0;
//   pts[2].x=345;  pts[2].y=300;  pts[2].z=0;
//   pts[3].x=400;  pts[3].y=250;  pts[3].z=0;
//   pts[4].x=500;  pts[4].y=550;  pts[4].z=0;
//   pts[5].x=550;  pts[5].y=150;  pts[5].z=0;
//   pts[6].x=570;  pts[6].y=50;   pts[6].z=0;
//   pts[7].x=600;  pts[7].y=100;  pts[7].z=0;

//   int resolution = 100;  // how many points our in our output array
//   point *out_pts;
//   out_pts = new point[resolution];

//   bspline(n, t, pts, out_pts, resolution);
//   if (set_graph())
//   {
//     setcolor(69);
//     for (i=0; i<=n; i++)
//       circle(pts[i].x,pts[i].y,2); // put circles at control points
//     circle(pts[0].x,pts[0].y,0);  // drop the pen down at first control point
//     for (i=0; i<resolution; i++)
//     {
//       setcolor(i);   // have a little fun with the colors
//       putpixel(out_pts[i].x,out_pts[i].y,WHITE);
//     }
//   }
// }

double blend(int k, int t, int *u, double v)  // calculate the blending value
{
  double value;

  if (t==1)			// base case for the recursion
  {
    if ((u[k]<=v) && (v<u[k+1]))
      value=1;
    else
      value=0;
  }
  else
  {
    if ((u[k+t-1]==u[k]) && (u[k+t]==u[k+1]))  // check for divide by zero
      value = 0;
    else
    if (u[k+t-1]==u[k]) // if a term's denominator is zero,use just the other
      value = (u[k+t] - v) / (u[k+t] - u[k+1]) * blend(k+1, t-1, u, v);
    else
    if (u[k+t]==u[k+1])
      value = (v - u[k]) / (u[k+t-1] - u[k]) * blend(k, t-1, u, v);
    else
      value = (v - u[k]) / (u[k+t-1] - u[k]) * blend(k, t-1, u, v) +
	      (u[k+t] - v) / (u[k+t] - u[k+1]) * blend(k+1, t-1, u, v);
  }
  return value;
}

void compute_intervals(int *u, int n, int t)   // figure out the knots
{
  int j;

  for (j=0; j<=n+t; j++)
  {
    if (j<t)
      u[j]=0;
    else
    if ((t<=j) && (j<=n))
      u[j]=j-t+1;
    else
    if (j>n)
      u[j]=n-t+2;  // if n-t=-2 then we're screwed, everything goes to 0
  }
}

void compute_point(int *u, int n, int t, double v, point *control,
			point *output)
{
  int k;
  double temp;

  // initialize the variables that will hold our outputted point
  output->x=0;
  output->y=0;
  output->z=0;

  for (k=0; k<=n; k++)
  {
    temp = blend(k,t,u,v);  // same blend is used for each dimension coordinate
    output->x = output->x + (control[k]).x * temp;
    output->y = output->y + (control[k]).y * temp;
    output->z = output->z + (control[k]).z * temp;
  }
}


// int set_graph(void)
//   {
// 	int graphdriver = DETECT, graphmode, error_code;

// 	//Initialize graphics system; must be EGA or VGA
// 	initgraph(&graphdriver, &graphmode, "c:\\borlandc\\bgi");
// 	error_code = graphresult();
// 	if (error_code != grOk)
// 		return(-1);               // No graphics hardware found
// 	if ((graphdriver != EGA) && (graphdriver != VGA))
// 	{
// 		closegraph();
// 		return 0;
// 	}
// 	return(1);                   // Graphics OK, so return "true"
// }
