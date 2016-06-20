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

#include "camera/CullingManager.h"
#include "camera/CameraManager.h"
#include "entities/Creature.h"
#include "gamemap/GameMap.h"
#include "render/RenderManager.h"
#include "utils/VectorInt64.h"
#include "utils/LogManager.h"

#include <OgreVector3.h>
#include <OgreCamera.h>

#include <sstream>
#include <algorithm>

//! Values used to know whether to show and/or hide a mesh

CullingManager::CullingManager(GameMap* gameMap, uint32_t cullingMask):
    mFirstIter(false),
    mGameMap(gameMap),
    mCullingMask(cullingMask),
    mCullTilesFlag(false)
{
    mActivePlanes =(Ogre::Plane(0, 0, 1, 0));
    // init the Ogre vector
    for (unsigned int i = 0; i < 4; ++i)
    {
        mOgreVectorsArray[i].x = static_cast<Ogre::Real>(0.0);
        mOgreVectorsArray[i].y = static_cast<Ogre::Real>(0.0);
        mOgreVectorsArray[i].z = static_cast<Ogre::Real>(0.0);
    }
}

void CullingManager::cullTiles()
{
    mOldWalk = mWalk;
    mWalk.mVertices.mMyArray.clear();
    for (int ii = 0 ; ii < 4 ; ++ii)
        mWalk.mVertices.mMyArray.push_back(VectorInt64(mOgreVectorsArray[ii]));

    // create a slope -- a set of left and right path
    mWalk.convexHull();
    mWalk.buildSlopes();

    OD_LOG_DBG(mOldWalk.debug());
    OD_LOG_DBG(mWalk.debug());

    // reset index pointers to the begging of collections
    mOldWalk.prepareWalk();
    mWalk.prepareWalk();

    newBashAndSplashTiles(SHOW | HIDE);
}

void CullingManager::startTileCulling(Ogre::Camera* camera)
{
    getIntersectionPoints(camera);

    mWalk.mVertices.mMyArray.clear();
    for (int ii = 0 ; ii < 4 ; ++ii)
        mWalk.mVertices.mMyArray.push_back(VectorInt64(mOgreVectorsArray[ii]));

    mWalk.convexHull();
    mWalk.buildSlopes();
    mOldWalk = mWalk;
    mOldWalk.prepareWalk();
    mWalk.prepareWalk();
    hideAllTiles();
    newBashAndSplashTiles(SHOW);

    mCullTilesFlag = true;
}


void CullingManager::stopTileCulling()
{
    mCullTilesFlag = false;
    mOldWalk = mWalk;
    mWalk.mVertices.mMyArray.clear();
    for (int ii = 0 ; ii < 4 ; ++ii)
        mWalk.mVertices.mMyArray.push_back(VectorInt64(mOgreVectorsArray[ii]));

    // create a slope -- a set of left and rigth path
    mWalk.convexHull();
    mWalk.buildSlopes();

    OD_LOG_DBG(mOldWalk.debug());
    OD_LOG_DBG(mWalk.debug());

    // reset index pointers to the begging of collections
    mOldWalk.prepareWalk();
    mWalk.prepareWalk();

    newBashAndSplashTiles(HIDE);
    showAllTiles();
}

void CullingManager::hideAllTiles(void)
{
    for (int jj = 0; jj < mGameMap->getMapSizeY() ; ++jj)
    {
        for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
        {
            Tile* tile = mGameMap->getTile(ii, jj);
            tile->setTileCulling(mCullingMask, false);
        }
    }
}

void CullingManager::showAllTiles(void)
{
    for (int jj = 0; jj < mGameMap->getMapSizeY() ; ++jj)
    {
        for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
        {
            Tile* tile = mGameMap->getTile(ii, jj);
            tile->setTileCulling(mCullingMask, true);
        }
    }
}

void CullingManager::newBashAndSplashTiles(uint32_t mode)
{
    int64_t xxLeftOld = mOldWalk.getTopLeftVertex().x;
    int64_t xxRightOld = mOldWalk.getTopRightVertex().x;
    int64_t xxLeft = mWalk.getTopLeftVertex().x;
    int64_t xxRight = mWalk.getTopRightVertex().x;
    int64_t xxp, yyp;
    std::stringstream ss;
    int64_t bb = ((std::min(mWalk.getBottomLeftVertex().y, mOldWalk.getBottomRightVertex().y) >> VectorInt64::PRECISION_DIGITS) - 2) << VectorInt64::PRECISION_DIGITS;

    for (int64_t yy = ((std::max(mWalk.getTopLeftVertex().y, mOldWalk.getTopRightVertex().y  ) >> VectorInt64::PRECISION_DIGITS) + 2) << VectorInt64::PRECISION_DIGITS; yy >= bb; yy -= VectorInt64::UNIT)
    {
        mOldWalk.notifyOnMoveDown(yy);
        mWalk.notifyOnMoveDown(yy);
        xxLeft = mWalk.getCurrentXLeft(yy);
        xxLeftOld = mOldWalk.getCurrentXLeft(yy);
        xxRight = mWalk.getCurrentXRight(yy);
        xxRightOld = mOldWalk.getCurrentXRight(yy);

        int64_t mm = ((std::min(xxLeft, xxLeftOld) >> VectorInt64::PRECISION_DIGITS) << VectorInt64::PRECISION_DIGITS) ;
        if(std::min(xxLeft, xxLeftOld) < std::max(xxRight,xxRightOld))
        {
            for (int64_t xx = mm ; xx <= std::max(xxRight,xxRightOld); xx += VectorInt64::UNIT)
            {
                bool bash = (xx >= xxLeftOld && xx <= xxRightOld && (yy >= mOldWalk.getBottomLeftVertex().y) && yy <= mOldWalk.getTopLeftVertex().y);
                bool splash = (xx >= xxLeft && xx <= xxRight && (yy >= mWalk.getBottomLeftVertex().y) && yy <= mWalk.getTopLeftVertex().y);

                xxp = (xx >> VectorInt64::PRECISION_DIGITS);
                yyp = (yy >> VectorInt64::PRECISION_DIGITS);
                Tile* tile = mGameMap->getTile(xxp, yyp);
                if(bash && splash && (mode & HIDE) && (mode & SHOW))
                {
                    // Nothing
                }

                else if (bash && (mode & HIDE) && (tile != nullptr))
                {
                    tile->setTileCulling(mCullingMask, false);
                }
                else if (splash && (mode & SHOW) && (tile != nullptr))
                {
                    tile->setTileCulling(mCullingMask, true);
                }
            }
        }
    }
}

bool CullingManager::getIntersectionPoints(Ogre::Camera* camera)
{
    const Ogre::Vector3* cameraVector = camera->getWorldSpaceCorners();
    for(int ii = 0 ; ii < 4; ++ii)
        mActiveRay[ii]= Ogre::Ray (cameraVector[ii], cameraVector[ii+4] - cameraVector[ii]);

    std::pair<bool, Ogre::Real> intersectionResult;

    for(int ii = 0; ii < 4; ++ii)
    {
        intersectionResult =  mActiveRay[ii].intersects(mActivePlanes);
        if(intersectionResult.first)
            mOgreVectorsArray[ii]= (mActiveRay[ii].getPoint(intersectionResult.second));
        else
        {
            OD_LOG_ERR("I didn't find the intersection point for " + Helper::toString(ii) + "th ray ");
        }
    }
    return true;
}

void CullingManager::update(Ogre::Camera* camera)
{
    if(mCullTilesFlag)
        getIntersectionPoints(camera);
    if(mCullTilesFlag)
        cullTiles();
}

/*! \brief Sort two VectorInt64 p1 and p2  to satisfy p1 <= p2 according to
 * the value of X or Y coordinate, which depends on sortByX param.
 */
void CullingManager::sort(VectorInt64& p1, VectorInt64& p2, bool sortByX)
{
    if (sortByX)
    {
        if (p1.x > p2.x)
            std::swap(p1, p2);
    }
    else
    {
        if (p1.y > p2.y)
            std::swap(p1, p2);
    }
}


