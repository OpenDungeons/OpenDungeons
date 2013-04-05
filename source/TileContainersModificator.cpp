#include "TileContainersModificator.h"
#include "Tile.h"

/** \brief Returns all the valid tiles in the rectangular region specified by the two corner points given.
 *
 */
std::vector<Tile*> TilesContainersModificator::rectangularRegion(int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> returnList;
    Tile *tempTile;

    if (x1 > x2)
        std::swap(x1, x2);
    if (y1 > y2)
        std::swap(y1, y2);

    for (int ii = x1; ii <= x2; ++ii)
    {
        for (int jj = y1; jj <= y2; ++jj)
        {
            //TODO:  This routine could be sped up by using the neighborTiles function.
            tempTile = getTile(ii, jj);

            if (tempTile != NULL)
                returnList.push_back(tempTile);
        }
    }

    return returnList;
}

/** \brief Returns all the valid tiles in the curcular region surrounding the given point and extending outward to the specified radius.
 *
 */
std::vector<Tile*> TilesContainersModificator::circularRegion(int x, int y, double radius)
{
    std::vector<Tile*> returnList;
    Tile *tempTile;
    int xDist, yDist, distSquared;
    double radiusSquared = radius * radius;

    if (radius < 0.0)
        radius = 0.0;

    for (int i = x - radius; i <= x + radius; ++i)
    {
        for (int j = y - radius; j <= y + radius; ++j)
        {
            //TODO:  This routine could be sped up by using the neighborTiles function.
            xDist = i - x;
            yDist = j - y;
            distSquared = xDist * xDist + yDist * yDist;
            if (distSquared < radiusSquared)
            {
                tempTile = getTile(i, j);
                 if (tempTile != NULL)
                   returnList.push_back(tempTile);
            }
        }
    }
    return returnList;
}

/** \brief Returns a vector of all the valid tiles which are a neighbor to one or more tiles in the specified region, i.e. the "perimeter" of the region extended out one tile.
 *
 */
std::vector<Tile*> TilesContainersModificator::tilesBorderedByRegion(
    const std::vector<Tile*> &region)
{
    std::vector<Tile*> neighbors, returnList;

    // Loop over all the tiles in the specified region.
    for (unsigned int i = 0; i < region.size(); ++i)
    {
        // Get the tiles bordering the current tile and loop over them.
        neighbors = neighborTiles(region[i]);
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            bool neighborFound = false;

            // Check to see if the current neighbor is one of the tiles in the region.
            for (unsigned int k = 0; k < region.size(); ++k)
            {
                if (region[k] == neighbors[j])
                {
                    neighborFound = true;
                    break;
                }
            }

            if (!neighborFound)
            {
                // Check to see if the current neighbor is already in the returnList.
                for (unsigned int k = 0; k < returnList.size(); ++k)
                {
                    if (returnList[k] == neighbors[j])
                    {
                        neighborFound = true;
                        break;
                    }
                }
            }

            // If the given neighbor was not already in the returnList, then add it.
            if (!neighborFound)
                returnList.push_back(neighbors[j]);
        }
    }

    return returnList;
}
