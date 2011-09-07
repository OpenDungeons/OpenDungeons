#ifndef RADIALVECTOR2_H
#define RADIALVECTOR2_H

class RadialVector2
{
    public:
        RadialVector2();
        RadialVector2(const double x1, const double y1, const double x2, const double y2);
        RadialVector2(const double dx, const double dy);

        void fromCartesian      (const double x1, const double y1, const double x2, const double y2);
        void fromCartesian      (const double dx, const double dy);
        bool directionIsBetween (const RadialVector2& r1, const RadialVector2& r2) const;

        void normalizeTheta();

        inline const double&    getTheta() const                { return theta; }
        inline void             setTheta(const double nTheta)  { theta = nTheta; }

    private:
        double r;
        double theta;
};

#endif

