#include <algorithm>

#include "RadialVector2.h"

#include "TileCoordinateMap.h"

/*! \brief Creates and initializes the map out to the specified radius.
 *
 */
TileCoordinateMap::TileCoordinateMap(const int nRadius) :
        radius(nRadius)
{
    precomputeMap(nRadius);
}

/*! \brief Computes the distance and direction information of all the ordered pairs x,y which lie within a circle of the given radius and centered on the origin, the list is then sorted by the distance to the tile.
 *
 */
void TileCoordinateMap::precomputeMap(const int sightRadius)
{
    data.clear();

    //TODO: This loop can be made to list the visible region in a spiral pattern so that all of the tiles appear in the tileQueue already sorted.
    int sightRadiusSquared = sightRadius * sightRadius;
    for (int i = -1 * sightRadius; i <= sightRadius; ++i)
    {
        for (int j = -1 * sightRadius; j <= sightRadius; ++j)
        {
            int rSquared = i * i + j * j;
            if (rSquared > sightRadiusSquared)
                continue;

            data.push_back(TileCoordinateData(RadialVector2(i, j), rSquared,
                    std::pair<int, int>(i, j)));
        }
    }

    // Sort the tile queue so that if we start at any point in the tile queue and iterate forward from that point, every successive tile will be as far away from, or farther away from the target tile point.
    sort(data.begin(), data.end(), TileCoordinateMap::dataSortComparitor);
}

/*! \brief Returns the x,y of the ith ordered pair in the sequence of coordinates in order of increasing distance from the origin.
 *
 */
std::pair<int, int> TileCoordinateMap::getCoordinate(const int i)
{
    checkIndex(i);
    return data[i].getCoord();
}

/*! \brief Returns the angle (in radians) to the center of the ith tile, i.e. the the value of the function atan2(yi, xi).
 *
 */
double TileCoordinateMap::getCentralTheta(const int i)
{
    checkIndex(i);
    return data[i].getVec().getTheta();
}

/*! \brief Returns the square of the radius of the ith ordered pair from the origin, i.e. the value r^2 = xi^2 + yi^2.
 *
 */
int TileCoordinateMap::getRadiusSquared(const int i)
{
    checkIndex(i);
    return data[i].getRadiusSquared();
}

/*! \brief A helper for sorting the list of ordered pairs, the list will be sorted in order of increasing radiusSquared for the ordered pairs in the list.
 *
 */
bool TileCoordinateMap::dataSortComparitor(TileCoordinateData t1, TileCoordinateData t2)
{
    return t1.getRadiusSquared() < t2.getRadiusSquared();
}

/*! \brief Ensures that a call to get the information for index i will succeed, this function will recompute the coordinate map to a larger radius if neccessary to make the call succeed.
 *
 */
void TileCoordinateMap::checkIndex(const unsigned int i)
{
    if (i >= data.size())
    {
        int newRadius = radius * 2;
        precomputeMap(newRadius);
        radius = newRadius;
    }
}
