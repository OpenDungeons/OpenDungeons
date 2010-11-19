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
	r = sqrt(dx*dx + dy*dy);
	theta = atan2(dy, dx);
}

bool RadialVector2::directionIsBetween(RadialVector2 r1, RadialVector2 r2)
{
	double tempTheta = theta + 2.0*M_PI;
	double tempTheta1 = r1.theta + 2.0*M_PI;
	double tempTheta2 = r2.theta + 2.0*M_PI;

	if(tempTheta1 < tempTheta2)
		return (tempTheta1 <= tempTheta && tempTheta <= tempTheta2);
	else
		return (tempTheta2 <= tempTheta && tempTheta <= tempTheta1);
}

void RadialVector2::normalizeTheta()
{
	//TODO: Optimize this.
	while(theta < 0.0)  theta += 2.0*M_PI;
	while(theta >= 2.0*M_PI)  theta -= 2.0*M_PI;
}

