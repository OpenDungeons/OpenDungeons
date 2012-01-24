#include "TileMap.h"
#include "Tile.h"

TileMap::TileMap()
    : offsetX(0),
      offsetY(0)
{
}

TileMap::TileMap( size_t sizeX, size_t sizeY, int offsetX, int offsetY)
{
    initialise(sizeX, sizeY, offsetX, offsetY);
}

void TileMap::initialise ( size_t sizeX, size_t sizeY, int offsetX, int offsetY )
{
    this->offsetX = offsetX;
    this->offsetY = offsetY;
    array = new TileArray(boost::extents[sizeX][sizeY]);
}

void TileMap::insertTile ( Tile* tile )
{
    array.get()[tile->getX() + offsetX][tile->getY() + offsetY] = tile;
}

TileMap::~TileMap()
{

}

