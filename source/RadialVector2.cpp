/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "RadialVector2.h"

#include "Helper.h"

#include <iostream>
#include <cmath>

RadialVector2::RadialVector2() :
        mRadius(0.0),
        mTheta(0.0)
{
}

RadialVector2::RadialVector2(const double x1, const double y1,
                             const double x2, const double y2)
{
    fromCartesian(x1, y1, x2, y2);
}

RadialVector2::RadialVector2(const double dx, const double dy)
{
    fromCartesian(dx, dy);
}

void RadialVector2::fromCartesian(const double x1, const double y1,
                                  const double x2, const double y2)
{
    fromCartesian(x2 - x1, y2 - y1);
}

void RadialVector2::fromCartesian(const double dx, const double dy)
{
    mRadius = sqrt(dx * dx + dy * dy); // Never actually used ??
    mTheta = atan2(dy, dx);
}

bool RadialVector2::directionIsBetween(const RadialVector2& r1, const RadialVector2& r2) const
{
    double tempTheta = mTheta + 2.0 * PI;
    double tempTheta1 = r1.mTheta + 2.0 * PI;
    double tempTheta2 = r2.mTheta + 2.0 * PI;

    return (tempTheta1 < tempTheta2)
            ? (tempTheta1 <= tempTheta && tempTheta <= tempTheta2)
            : (tempTheta2 <= tempTheta && tempTheta <= tempTheta1);
}

void RadialVector2::normalizeTheta()
{
    mTheta = fmod(mTheta, 2.0 * PI);
    if (mTheta < 0.0)
    {
        mTheta += 2.0 * PI;
    }
}
