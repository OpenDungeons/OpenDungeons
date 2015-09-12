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

#ifndef VECTOR3I_H_
#define VECTOR3I_H_

#include <cstdint>
#include <iostream>
#include <OgreVector3.h>

extern const int mPrecisionDigits;
extern const int Unit;

//! \brief A custom vector used commonly between the CameraManager and the CullingManager classes
class Vector3i
{
public:
    Vector3i():
        x(0),
        y(0),
        z(0)
    {}

    Vector3i(int64_t nx, int64_t ny , int64_t nz):
        x(nx * Unit),
        y(ny * Unit),
        z(nz * Unit)
    {}

    Vector3i& operator+(Vector3i& vv);
    Vector3i& operator-(Vector3i& vv);
    Vector3i& operator/(int ii);
    Vector3i(const Ogre::Vector3& OV);
    Vector3i& operator*(double zz);

    int64_t x;
    int64_t y;
    int64_t z;
};

std::ostream& operator<< (std::ostream& ss,Vector3i& vv);

#endif // VECTOR3I_H_
