#include "TileArray.h"
#include "Tile.h"

TileArray::TileArray()
    : offsetX(0),
      offsetY(0)
{
}

TileArray::TileArray( size_t sizeX, size_t sizeY, int offsetX, int offsetY)
{
    initialise(sizeX, sizeY, offsetX, offsetY);
}

void TileArray::initialise ( size_t sizeX, size_t sizeY, int offsetX, int offsetY )
{
    this->offsetX = offsetX;
    this->offsetY = offsetY;
    array = new TileMultiArray_t(boost::extents[sizeX][sizeY]);
}

void TileArray::insertTile ( Tile* tile )
{
    array.get()[tile->getX() + offsetX][tile->getY() + offsetY] = tile;
}

TileArray::~TileArray()
{

}

