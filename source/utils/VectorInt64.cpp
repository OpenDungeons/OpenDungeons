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

#include "VectorInt64.h"

#include <iostream>

VectorInt64& VectorInt64::operator+(VectorInt64& vv)
{
    x += vv.x;
    y += vv.y;
    z += vv.z;
    return *this;
}

VectorInt64& VectorInt64::operator-(VectorInt64& vv)
{
    x -= vv.x;
    y -= vv.y;
    z -= vv.z;
    return *this;
}



VectorInt64& VectorInt64::operator/(int ii)
{
    x /= ii;
    y /= ii;
    z /= ii;
    return *this;
}

VectorInt64& VectorInt64::operator*(double zz)
{
    x *= zz;
    y *= zz;
    z *= zz;
    return *this;
}



std::ostream& operator<<(std::ostream& ss,VectorInt64& vv)
{
    ss << vv.x << " " << vv.y <<" " << vv.z << " ";
    return ss;
}

VectorInt64::VectorInt64(const Ogre::Vector3& OV)
{
    x = static_cast<int64_t>(UNIT * OV.x);
    y = static_cast<int64_t>(UNIT * OV.y);
    z = static_cast<int64_t>(UNIT * OV.z);
}
