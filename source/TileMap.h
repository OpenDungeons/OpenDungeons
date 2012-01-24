#ifndef TILEMAP_H
#define TILEMAP_H

#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>

class Tile;

typedef boost::multi_array<Tile*, 2> TileArray;

class TileMap
{

    public:
        TileMap();
        TileMap(size_t sizeX, size_t sizeY, int offsetX, int offsetY);
        void initialise(size_t sizeX, size_t sizeY, int offsetX, int offsetY);
        inline void clear(){ array.reset(); };
        void insertTile(Tile* tile);
        inline Tile* getElement(int x, int y)
        {
            assert(array.get != 0);
            Tile* ret = array.get()[x + offsetX][y + offsetY].get()
            assert(ret != 0);
            return ret;
        }
        virtual ~TileMap();
    private:
        boost::shared_ptr<TileArray> array;
        int offsetX;
        int offsetY;
};

#endif // TILEMAP_H
