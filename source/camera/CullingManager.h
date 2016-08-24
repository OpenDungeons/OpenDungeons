/*
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

#ifndef CULLINGMANAGER_H_
#define CULLINGMANAGER_H_

#include "camera/SlopeWalk.h"
#include "entities/Tile.h"

#include "utils/VectorInt64.h"

#include <OgreRay.h>
#include <OgrePlane.h>
#include <set>

class GameMap;

namespace Ogre
{
class Camera;
};

namespace CullingType
{
    //! \brief bit array to know if the entity should be displayed/hidden
    const uint32_t HIDE = 0;
    const uint32_t SHOW_MAIN_WINDOW = 0x01;
    const uint32_t SHOW_MINIMAP = 0x02;

    const uint32_t SHOW_ALL = 0x03;
};

/*! \brief The CullingManager class is a class to effectively
 *  manage culling methods used in game. So far there is only
 *  one algorithm included : it is supposed to cull the Tiles.
 *  It should be started with the method startTileCulling.

 * Currently I try to implement the most general polygon rasterizing algorithm , that is :
 * 1. We choose the max and min points due to their Y value .
 * 2. We sort all the points by the angle value in it's polar representation
 * ( that is going , by visiting them clockwise due to the center of the polygon )
 * 3. Now we have two paths from which we can walk from top to down vertices : call them left and right .
 * 4. Between each following pair of vertices we can establish a >> slope << ,
 * which is just the the "a" in the eq. of linear form y = ax + b
 * 5. Now start drawing our polygon Row by Row From top to bottom .
 * Each Row has given the most left and rightmost tile due to use of both paths prepared before
 * -- Left path for tracing the most Leftmost Tile , Right path the most Rightmost Tile in each .
 */
class CullingManager
{
public:
    static const uint32_t HIDE =  1;
    static const uint32_t SHOW =  2;

    CullingManager(GameMap* gameMap, uint32_t cullingMask);

    void startTileCulling(Ogre::Camera* camera, const std::vector<Ogre::Vector3>& ogreVectors);

    void stopTileCulling(const std::vector<Ogre::Vector3>& ogreVectors);

    void update(Ogre::Camera* camera, const std::vector<Ogre::Vector3>& ogreVectors);

    //! \brief Computes the intersection points from the camera with the XY plane. The corresponding
    //! vectors are put in ogreVectors
    bool computeIntersectionPoints(Ogre::Camera* camera, std::vector<Ogre::Vector3>& ogreVectors);

private:

    void cullTiles(const std::vector<Ogre::Vector3>& ogreVectors);

    void hideAllTiles();
    void showAllTiles();

    // set the new tiles
    void newBashAndSplashTiles(uint32_t);

    void sort(VectorInt64& p1, VectorInt64& p2, bool sortByX);

    // Objects representing past and present walk around the polygon
    SlopeWalk mWalk;
    SlopeWalk mOldWalk;

    bool mFirstIter;
    GameMap* mGameMap;

    uint32_t mCullingMask;

    bool mCullTilesFlag;
};

#endif // CULLINGMANAGER_H_
