/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

    //! \brief Make sure that theta is always within 0 and 2 PI
    void normalizeTheta();

    inline const double& getTheta() const
    { return mTheta; }

    inline void setTheta(const double nTheta)
    { mTheta = nTheta; }

private:
    double mRadius;
    double mTheta;
};

#endif // RADIALVECTOR2_H
