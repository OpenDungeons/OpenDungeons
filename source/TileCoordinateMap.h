#ifndef TILECOORDINATEMAP_H
#define TILECOORDINATEMAP_H

#include <vector>
#include <utility>

class TileCoordinateData;
class RadialVector2;

/*! \brief A data structure which computes, stores, and lets you query information on relative
 * distance and direction information about tiles.
 */
class TileCoordinateMap
{
    public:
        TileCoordinateMap(const int nRadius);

        void                precomputeMap   (const int sightRadius);
        std::pair<int, int> getCoordinate   (const int i);
        double              getCentralTheta (const int i);
        int                 getRadiusSquared(const int i);

    private:
        static bool dataSortComparitor(TileCoordinateData t1, TileCoordinateData t2);

        void checkIndex(const unsigned int i);

        int                             radius;
        std::vector<TileCoordinateData> data;
};

/*! \brief A helper data structure class which stores the indiviual entries in a TileCoordinateMap.
 *
 */
class TileCoordinateData
{
    public:
        TileCoordinateData(RadialVector2 nvec, int nradiusSquared,
                std::pair<int, int> ncoord) :
                vec             (nvec),
                radiusSquared   (nradiusSquared),
                coord           (ncoord)
        {}

        inline const RadialVector2&         getVec          () const { return vec; }
        inline const int                    getRadiusSquared() const { return radiusSquared; }
        inline const std::pair<int, int>&   getCoord        () const { return coord; }

    private:
        RadialVector2       vec;
        int                 radiusSquared;
        std::pair<int, int> coord;
};

#endif

