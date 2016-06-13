/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef VECTOR3I_H_
#define VECTOR3I_H_

#include <cstdint>
#include <iostream>
#include <OgreVector3.h>




//! \brief A custom vector used commonly between the CameraManager and the CullingManager classes
class VectorInt64
{
public:
    static const int PRECISION_DIGITS = 20;
    static const int UNIT = (1 << PRECISION_DIGITS);

    VectorInt64():
        x(0),
        y(0),
        z(0)
    {}

    VectorInt64(int64_t nx, int64_t ny , int64_t nz):
        x(nx * UNIT),
        y(ny * UNIT),
        z(nz * UNIT)
    {}

    VectorInt64& operator+(VectorInt64& vv);
    VectorInt64& operator-(VectorInt64& vv);
    VectorInt64& operator/(int ii);
    VectorInt64(const Ogre::Vector3& OV);
    VectorInt64& operator*(double zz);

    int64_t x;
    int64_t y;
    int64_t z;
};

std::ostream& operator<< (std::ostream& ss,VectorInt64& vv);

#endif // VECTOR3I_H_
