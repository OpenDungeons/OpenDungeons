/*!
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

#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <cmath>

namespace Pathfinding
{
    //! \brief Returns the shortest distance (hypotenuse) between tiles located at the two coordinates given.
    template<typename T>
    inline float distanceTile(const T& t1, const T& t2)
    {
        return std::hypotf(t2.getX() - t1.getX(), t2.getY() - t1.getY());
    }

    //! \brief returns squared distance ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1))
    template <typename T>
    inline T squaredDistance(T x1, T x2, T y1, T y2)
    {
        return ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
    }

    //! \brief Returns the squared distance between 2 objects implementing getX/getY
    template <typename T>
    inline auto squaredDistanceTile(const T& ent1, const T& ent2) -> decltype(ent1.getX())
    {
        return squaredDistance(ent1.getX(), ent2.getX(), ent1.getY(), ent2.getY());
    }
}

#endif // PATHFINDING_H
