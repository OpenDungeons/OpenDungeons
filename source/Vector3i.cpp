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
    this->x+=vv.x;
    this->y+=vv.y;
    this->z+=vv.z;

    return *this;
}

Vector3i& Vector3i::operator/(int ii)
{
    // FIXME: Should check for division by 0.
    this->x/=ii;
    this->y/=ii;
    this->z/=ii;

    return *this;
}

Vector3i::Vector3i(const Ogre::Vector3& OV)
{
    x = (int64_t)OV.x;
    y = (int64_t)OV.y;
    z = (int64_t)OV.z;
}

// Vector3i::Vector3i(const Ogre::Vector3& OV)
// {
//     x = (int64_t)((1 << mPrecisionDigits) * OV.x);
//     y = (int64_t)((1 << mPrecisionDigits) * OV.y);
//     z = (int64_t)((1 << mPrecisionDigits) * OV.z);
// }

std::ostream& operator<<(std::ostream& ss, Vector3i& vv)
{
    ss << vv.x << " " << vv.y << " " << vv.z << " ";
    return ss;
}
