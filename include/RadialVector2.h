#ifndef RADIALVECTOR2_H
#define RADIALVECTOR2_H

class RadialVector2
{
    public:
        double r, theta;

        RadialVector2();
        RadialVector2(double x1, double y1, double x2, double y2);
        RadialVector2(double dx, double dy);

        void fromCartesian(double x1, double y1, double x2, double y2);
        void fromCartesian(double dx, double dy);
        bool directionIsBetween(RadialVector2 r1, RadialVector2 r2);

        void normalizeTheta();
};

#endif

