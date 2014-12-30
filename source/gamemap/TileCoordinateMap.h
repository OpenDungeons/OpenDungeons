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

#ifndef TILECOORDINATEMAP_H
#define TILECOORDINATEMAP_H

#include "utils/RadialVector2.h"

#include <vector>
#include <utility>

class TileCoordinateData;

/*! \brief A data structure which computes, stores, and lets you query information on relative
 * distance and direction information about tiles.
 */
class TileCoordinateMap
{
public:
    //! \brief Creates and initializes the map out to the specified radius.
    TileCoordinateMap(const int nRadius);

    //! \brief Computes the distance and direction information of all the ordered pairs x,y
    //! which lie within a circle of the given radius and centered on the origin,
    //! the list is then sorted by the distance to the tile.
    void precomputeMap(const int sightRadius);

    //! \brief Returns the x,y of the ith ordered pair in the sequence of coordinates
    //! in order of increasing distance from the origin.
    std::pair<int, int> getCoordinate(const int i);

    //! \brief Returns the angle (in radians) to the center of the ith tile,
    //! i.e. the the value of the function atan2(yi, xi).
    double getCentralTheta(const int i);

    //! \brief Returns the square of the radius of the ith ordered pair from the origin,
    //! i.e. the value r^2 = xi^2 + yi^2.
    int getRadiusSquared(const int i);

private:
    //! \brief A helper for sorting the list of ordered pairs,
    //! the list will be sorted in order of increasing radiusSquared
    //! for the ordered pairs in the list.
    static bool dataSortComparitor(TileCoordinateData t1, TileCoordinateData t2);

    //! \brief Ensures that a call to get the information for index i will succeed,
    //! this function will recompute the coordinate map to a larger radius
    //! if neccessary to make the call succeed.
    void checkIndex(const unsigned int i);

    int mRadius;
    std::vector<TileCoordinateData> mData;
};

//! \brief A helper data structure class which stores the indiviual entries in a TileCoordinateMap.
class TileCoordinateData
{
public:
    TileCoordinateData(RadialVector2 nvec, int nradiusSquared, std::pair<int, int> ncoord):
        mVec(nvec),
        mRadiusSquared(nradiusSquared),
        mCoord(ncoord)
    {}

    inline const RadialVector2& getVec() const
    { return mVec; }

    inline const int getRadiusSquared() const
    { return mRadiusSquared; }

    inline const std::pair<int, int>& getCoord() const
    { return mCoord; }

private:
    RadialVector2 mVec;
    int mRadiusSquared;
    std::pair<int, int> mCoord;
};

#endif // TILECOORDINATEMAP_H
