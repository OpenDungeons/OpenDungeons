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

#include "Vector3i.h"

Vector3i& Vector3i::operator+(Vector3i& vv)
{
    x += vv.x;
    y += vv.y;
    z += vv.z;
    return *this;
}

Vector3i& Vector3i::operator-(Vector3i& vv)
{
    x -= vv.x;
    y -= vv.y;
    z -= vv.z;
    return *this;
}



Vector3i& Vector3i::operator/(int ii)
{
    x /= ii;
    y /= ii;
    z /= ii;
    return *this;
}

Vector3i& Vector3i::operator*(double zz)
{
    x *= zz;
    y *= zz;
    z *= zz;
    return *this;
}



std::ostream& operator<<(std::ostream& ss,Vector3i& vv)
{
    ss << vv.x << " " << vv.y <<" " << vv.z << " ";
    return ss;
}

Vector3i::Vector3i(const Ogre::Vector3& OV)
{
    x = static_cast<int64_t>(Unit * (OV.x + 128.0));
    y = static_cast<int64_t>(Unit * (OV.y + 128.0));
    z = static_cast<int64_t>(Unit * (OV.z + 128.0));
}
