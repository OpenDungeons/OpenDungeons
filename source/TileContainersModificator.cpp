#include "TileContainersModificator.h"
#include <algorithm> 

/** \brief Returns all the valid tiles in the rectangular region specified by the two corner points given.
 *
 */
std::vector<Tile*> TileContainersModificator::rectangularRegion(int x1, int y1, int x2, int y2)
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
            tempTile = getContainer()->getTile(ii, jj);

            if (tempTile != NULL)
                returnList.push_back(tempTile);
        }
    }

    return returnList;
}

/** \brief Returns all the valid tiles in the curcular region surrounding the given point and extending outward to the specified radius.
 *
 */
std::vector<Tile*> TileContainersModificator::circularRegion(int x, int y, double radius)
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
                tempTile = getContainer()->getTile(i, j);
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
std::vector<Tile*> TileContainersModificator::tilesBorderedByRegion(
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

/*! \brief Returns the (up to) 4 nearest neighbor tiles of the tile located at (x, y).
 *
 */
std::vector<Tile*> TileContainersModificator::neighborTiles(int x, int y)
{
    std::vector<Tile*> tempVector;

    Tile *tempTile = getContainer()->getTile(x, y);
    if (tempTile != NULL)
        tempVector = neighborTiles(tempTile);

    return tempVector;
}

std::vector<Tile*> TileContainersModificator::neighborTiles(Tile *t)
{
    return t->getAllNeighbors();
}


TileContainer* TileContainersModificator::create(int x , int y , Tile::TileType tt ){

    TileContainer *tc = new TileContainer();
    

}


TileContainer* TileContainersModificator::copy(TileContainer* tc1 , TileContainer* tc2 ){
    if(tc1 == NULL || tc2 == NULL)
	return NULL; 

    for(int ii = 0 ; ii < tc1->getMapSizeX() ; ii++){
	for(int jj = 0 ; jj < tc2->getMapSizeY(); jj++){
	    // tc2->getTile(ii,jj) = tc1->getTile(ii,jj);

	}
	return tc2;
    }
}

TileContainer* TileContainersModificator::embed(TileContainer* tc1 , TileContainer* tc2 ){
    if(tc1 == NULL )
	return NULL;   
    delete tc1;
    return tc2;
}

TileContainer* TileContainersModificator::rotate90(TileContainer* tc ){
    // std::swap(tc1->getMapSizeX(),tc1->getMapSizeY());
    if(tc == NULL )
	return NULL;   



    return tc;

}



TileContainer* TileContainersModificator::reflectX(TileContainer* tc){
    if(tc == NULL )
	return NULL;   
    tc->symetry = !tc->symetry;
    tc->rr++;
    tc->rr%=4;
    return tc; 
}



TileContainer* TileContainersModificator::reflectY(TileContainer* tc){
    if(tc == NULL )
	return NULL;  
    tc->symetry = !tc->symetry;
    return tc; 
}


TileContainer* TileContainersModificator::tilePermute(TileContainer* tc, int x , int y , Tile::TileType tt ){
    if(tc == NULL )
	return NULL;  

    return tc;

}



TileContainer* TileContainersModificator::tileReplace( TileContainer* tc , Tile::TileType tt1, Tile::TileType tt2  ){
    if(tc == NULL )
	return NULL;  


    for(int ii = 0 ; ii < tc->getMapSizeX() ; ii++){
	for(int jj = 0 ; jj < tc->getMapSizeY(); jj++){
	    if(tc->getTile(ii,jj)->type == tt1  ){
		tc->getTile(ii,jj)->type = tt2 ;
	    }

	}
    }

    return tc;
}





