#include "TileContainer.h"

TileContainer::TileContainer():
    mapSizeX(0),
    mapSizeY(0),
    rr(0),
    tiles(NULL)
{
    sem_init(&tilesLockSemaphore, 0, 1);
}

TileContainer::~TileContainer()
{
    clearTiles();
    if (tiles)
        delete [] tiles;
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles in the TileContainer.
 *
 */
void TileContainer::clearTiles()
{
    sem_wait(&tilesLockSemaphore);

    for (int jj = 0; jj < mapSizeY; ++jj)
    {
        for (int ii = 0; ii < mapSizeX; ++ii)
        {
            tiles[ii][jj].destroyMesh();
            tiles[ii][jj].deleteYourself();
        }
    }

    sem_post(&tilesLockSemaphore);
}


bool TileContainer::addTile(const Tile& t)
{
    if (t.x < getMapSizeX() && t.y < getMapSizeY() && t.x >= 0 && t.y >= 0)
    {
        tiles[t.x][t.y] = t;
        return true;
    }

    return false;
}

/*! \brief Adds the address of a new tile to be stored in this TileContainer.
 *
 */

void TileContainer::setTileNeighbors(Tile *t){
    for (unsigned int i = 0; i < 2; ++i)
    {
        int tempX = t->x, tempY = t->y;
        switch (i)
        {

        case 0:
            --tempX;
            break;
        case 1:
            --tempY;
            break;

        default:
            std::cerr << "\n\n\nERROR:  Unknown neighbor index.\n\n\n";
            exit(1);
        }

        // If the current neigbor tile exists, add the current tile as one of its
        // neighbors and add it as one of the current tile's neighbors.
        if (tempX >=0 && tempY >= 0)
	    {

		Tile *tempTile = getTile(tempX, tempY);
		tempTile->addNeighbor(t);
		t->addNeighbor(tempTile);
	    }
    }
}

/*! \brief Returns a pointer to the tile at location (x, y) (const version).
 *
 * The tile pointers are stored internally in a map so calls to this function
 * have a complexity O(log(N)) where N is the number of tiles in the map.
 * This function does not lock.
 */
Tile* TileContainer::getTile(int xx, int yy) const
{
    Tile *returnValue = NULL;
    // std::pair<int, int> location(x, y);

    // sem_wait(&tilesLockSemaphore);
    // const TileMap_t& constTiles = tiles;
    // TileMap_t::const_iterator itr = constTiles.find(location);
    // returnValue = (itr != tiles.end()) ? itr->second : NULL;
    // sem_post(&tilesLockSemaphore);
    if (xx < getMapSizeX() && yy < getMapSizeY() && xx >= 0 && yy >= 0 )
        return returnValue = &(tiles[xx][yy]);
    else {
        // std :: cerr << " invalid x,y coordinates to getTile" << std :: endl;
        return NULL;
    }
}

Tile::TileType* TileContainer::getNeighborsTypes( Tile *curTile, Tile::TileType   *neighbors){

    int xx = curTile->getX();
    int yy = curTile->getY();

    neighbors[0] = getSafeTileType(getTile(xx-1,yy) );
    neighbors[1] = getSafeTileType(getTile(xx-1,yy+1) );
    neighbors[2] = getSafeTileType(getTile(xx,yy+1));
    neighbors[3] = getSafeTileType(getTile(xx+1,yy+1));
    neighbors[4] = getSafeTileType(getTile(xx+1,yy) );
    neighbors[5] = getSafeTileType(getTile(xx+1,yy-1));
    neighbors[6] = getSafeTileType(getTile(xx,yy-1));
    neighbors[7] = getSafeTileType(getTile(xx-1,yy-1));



    return neighbors;

}


bool* TileContainer::getNeighborsFullness( Tile *curTile, bool *neighborsFullness){

    int xx = curTile->getX();
    int yy = curTile->getY();

    neighborsFullness[0] = getSafeTileFullness(getTile(xx-1,yy) );
    neighborsFullness[1] = getSafeTileFullness(getTile(xx-1,yy+1) );
    neighborsFullness[2] = getSafeTileFullness(getTile(xx,yy+1));
    neighborsFullness[3] = getSafeTileFullness(getTile(xx+1,yy+1));
    neighborsFullness[4] = getSafeTileFullness(getTile(xx+1,yy) );
    neighborsFullness[5] = getSafeTileFullness(getTile(xx+1,yy-1));
    neighborsFullness[6] = getSafeTileFullness(getTile(xx,yy-1));
    neighborsFullness[7] = getSafeTileFullness(getTile(xx-1,yy-1));

    return neighborsFullness;
}


/*! \brief Returns the number of tile pointers currently stored in this TileContainer.
 *
 */
unsigned int TileContainer::numTiles()
{
    // sem_wait(&tilesLockSemaphore);
    // unsigned int tempUnsigned = tiles.size();
    // sem_post(&tilesLockSemaphore);

    return mapSizeX * mapSizeY;
}

Tile* TileContainer::firstTile()
{
    return &tiles[0][0];
}

/*! \brief Returns an iterator to be used for the purposes of looping over the tiles stored in this GameMap.
 *
 */
Tile* TileContainer::lastTile()
{
    return &tiles[getMapSizeX()][getMapSizeY()];
}


bool TileContainer::allocateMapMemory(int xSize, int ySize)
{
    if (xSize <= 0 || ySize <= 0)
    {
        std::cerr << "Invalid map size given. Couldn't allocate map memory" << std::endl;
        return false;
    }

    // Set map size
    mapSizeX = xSize;
    mapSizeY = ySize;

    // Clear memory usage first
    if (tiles)
        delete [] tiles;

    tiles = new Tile *[mapSizeX];
    if(!tiles)
    {
        std::cerr << "Failed to allocate map memory" << std::endl;
        return false;
    }

    for (int i = 0; i < mapSizeX; ++i)
    {
        tiles[i] = new Tile[mapSizeY];
    }

    return true;
}

Tile::TileType TileContainer::getSafeTileType(Tile* tt )
{
    return (tt == NULL) ? Tile::nullTileType : tt->getType();
}

bool  TileContainer::getSafeTileFullness(Tile* tt)
{
    return (tt == NULL) ? false : (tt->getFullness() > 0 );
}

std::vector<Tile*> TileContainer::rectangularRegion(int x1, int y1, int x2, int y2)
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
            tempTile = getTile(ii, jj);

            if (tempTile != NULL)
                returnList.push_back(tempTile);
        }
    }

    return returnList;
}

std::vector<Tile*> TileContainer::circularRegion(int x, int y, double radius) const
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

std::vector<Tile*> TileContainer::tilesBorderedByRegion(const std::vector<Tile*> &region)
{
    std::vector<Tile*> neighbors, returnList;

    // Loop over all the tiles in the specified region.
    for (unsigned int i = 0; i < region.size(); ++i)
    {
        // Get the tiles bordering the current tile and loop over them.
        neighbors = region[i]->getAllNeighbors();
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

std::vector<Tile*> TileContainer::neighborTiles(int x, int y)
{
    std::vector<Tile*> tempVector;

    Tile *tempTile = getTile(x, y);
    if (tempTile != NULL)
        tempVector = tempTile->getAllNeighbors();

    return tempVector;
}
