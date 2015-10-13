/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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
#include "camera/CameraManager.h"
#include "camera/SlopeWalk.h"

#include "utils/Vector3i.h"

#include <OgreRay.h>
#include <OgrePlane.h>
#include <set>

class CameraManager;
class GameMap;

/*! \brief The CullingMangaer class is a class to effectivly 
 *  manage culling methods used in game. So far there is only
 *  one algorithm included : it is supposed to cull the Tiles.
 *  It should be started with the method startTileCulling.
 *  In future I plan to add creature's culling as well. 
 
 * Currently I try to impment the most general polygon rasterizing algorithm , that is :
 * 1. We choose the max and min points due to their Y value .
 * 2. We sort all the points by the angle value in it's polar representation 
 * ( that is going , by visiting them clockwise due to the center of the polygon )
 * 3. Now we have two paths from which we can walk from top to down vertices : call them left and right .
 * 4. Between each following pair of vertices we can establish a >> slope << ,
 * which is just the the "a" in the eq. of linear form y = ax + b
 * 5. Now start drawing our polygon Row by Row From top to bottom . 
 * Each Row has given the most left and rightmost tile due to use of both paths prepared before 
 * -- Left path for tracing the most Leftmost Tile , Right path teh most Rightmost Tile in each .
 */
class CullingManager
{
    friend class GameMap;

public:
    CullingManager(CameraManager*);

    void startCreatureCulling();
    void startTileCulling();
    void startDebugging();

    void stopCreatureCulling();
    void stopTileCulling();

    bool getIntersectionPoints();

    void hideAllTiles();
    void showAllTiles();

    int cullCreatures();
    void cullTiles();

    bool onFrameStarted();
    bool onFrameEnded();

    // set the new tiles
    void newBashAndSplashTiles(int64_t);

    void sort(Vector3i& p1, Vector3i& p2, bool sortByX);

    //! \brief Activate/deactivate debug output.
    bool mDebug;

private:

    // Objects representing past and present walk around the polygon
    SlopeWalk mWalk;
    SlopeWalk mOldWalk;

    // Array of Vector3's for keeping the intersection points of camera viewfrustrum 
    // and the XY plane
    Ogre::Vector3 mOgreVectorsArray[4];
 
    bool mFirstIter;
    CameraManager* mCm;

    Ogre::Plane mMyplanes;
    Ogre::Ray mMyRay[4];

    bool mCullCreaturesFlag;
    bool mCullTilesFlag;


};

#endif // CULLINGMANAGER_H_
