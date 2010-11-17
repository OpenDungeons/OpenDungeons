#include <iostream>
#include <cmath>

#include "RadialVector2.h"

RadialVector2::RadialVector2()
{
	r = 0.0;
	theta = 0.0;
}

RadialVector2::RadialVector2(double x1, double y1, double x2, double y2)
{
	fromCartesian(x1, y1, x2, y2);
}

void RadialVector2::fromCartesian(double x1, double y1, double x2, double y2)
{
	fromCartesian(x2-x1, y2-y1);
}

void RadialVector2::fromCartesian(double dx, double dy)
{
	// Handle the two 'divide by zero' special cases.
	if(fabs(dx) < 0.0000001)
	{
		r = fabs(dy);
		theta = (dy > 0.0) ? (M_PI/2.0) : (3.0*M_PI/2.0);
		return;
	}

	// Handle the other cases.
	r = sqrt(dx*dx + dy*dy);
	theta = atan2(dy, dx);

	if(theta < 0.0)
		theta += 2.0*M_PI;
}

bool RadialVector2::directionIsBetween(RadialVector2 r1, RadialVector2 r2)
{
	//r1.normalizeTheta();
	//r2.normalizeTheta();
	if(r1.theta > r2.theta)
	{
		RadialVector2 tempVector = r1;
		r1 = r2;
		r2 = tempVector;
	}

	// If this vector is simply between the other two, return true, else investigate to see if the two vectors straddle the positive x-axis.
	if(theta >= r1.theta && theta <= r2.theta)
	{
		return true;
	}
	else
	{
		// If the two vectors straddle the positive x-axis check to see if this vector lies between them.
		if(r1.theta <= M_PI/2.0 && r2.theta >= (3.0*M_PI/2.0))
		{
			if(theta <= r1.theta || theta >=r2.theta)
			{
				return true;
			}
		}
	}

	return false;
}

void RadialVector2::normalizeTheta()
{
	//TODO: Optimize this.
	while(theta < 0.0)  theta += 2.0*M_PI;
	while(theta >= 2.0*M_PI)  theta -= 2.0*M_PI;
}

