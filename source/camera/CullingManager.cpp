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

#include <OgreVector3.h>
#include <OgreCamera.h>

#include "camera/CullingManager.h"
#include "camera/CameraManager.h"
#include "gamemap/GameMap.h"

#include "entities/Creature.h"
#include "utils/VectorInt64.h"
#include "utils/LogManager.h"
#include "render/RenderManager.h"
#include <sstream>
#include <algorithm>

//! Values used to know whether to show and/or hide a mesh

CullingManager::CullingManager(CameraManager* cameraManager):
    mFirstIter(false),
    mCm(cameraManager),
    mCullTilesFlag(false)
{
    mActivePlanes =(Ogre::Plane(0, 0, 1, 0));
    // init the Ogre vector
    for (unsigned int i = 0; i < 8; ++i)
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
    for (int ii = 0 ; ii < 8 ; ++ii)
        mWalk.mVertices.mMyArray.push_back(VectorInt64(mOgreVectorsArray[ii]));

    // create a slope -- a set of left and rigth path
    mWalk.convex_hull();
    mWalk.buildSlopes();


    OD_LOG_DBG( mOldWalk.debug());
    OD_LOG_DBG( mWalk.debug());

    // reset index pointers to the begging of collections
    mOldWalk.prepareWalk();
    mWalk.prepareWalk();

    newBashAndSplashTiles(SHOW | HIDE);
}

void CullingManager::startTileCulling()
{
    getIntersectionPoints();

    mWalk.mVertices.mMyArray.clear();
    for (int ii = 0 ; ii < 8 ; ++ii)
        mWalk.mVertices.mMyArray.push_back(VectorInt64(mOgreVectorsArray[ii]));
    mWalk.convex_hull();
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
    for (int ii = 0 ; ii < 8 ; ++ii)
        mWalk.mVertices.mMyArray.push_back(VectorInt64(mOgreVectorsArray[ii]));

    // create a slope -- a set of left and rigth path
    mWalk.convex_hull();
    mWalk.buildSlopes();

    OD_LOG_DBG( mOldWalk.debug());
    OD_LOG_DBG( mWalk.debug());        

    // reset index pointers to the begging of collections
    mOldWalk.prepareWalk();
    mWalk.prepareWalk();

    newBashAndSplashTiles(HIDE);
    showAllTiles();
}

void CullingManager::hideAllTiles(void)
{
    GameMap* gm = mCm->getGameMap();
    if (!gm)
        return;

    for (int jj = 0; jj < gm->getMapSizeY() ; ++jj)
    {
        for (int ii = 0; ii < gm->getMapSizeX(); ++ii)
        {
            RenderManager::getSingleton().rrDetachEntity(gm->getTile(ii, jj));
        }
    }
}

void CullingManager::showAllTiles(void)
{
    GameMap* gm = mCm->getGameMap();
    if (!gm)
        return;

    for (int jj = 0; jj < gm->getMapSizeY() ; ++jj)
    {
        for (int ii = 0; ii < gm->getMapSizeX(); ++ii)
        {
            RenderManager::getSingleton().rrAttachEntity(gm->getTile(ii, jj));
        }
    }
}

void CullingManager::newBashAndSplashTiles(uint32_t mode){
    int64_t xxLeftOld = mOldWalk.getTopLeftVertex().x;
    int64_t xxRightOld= mOldWalk.getTopRightVertex().x;
    int64_t xxLeft = mWalk.getTopLeftVertex().x;
    int64_t xxRight= mWalk.getTopRightVertex().x;
    int64_t xxp, yyp;
    std::stringstream ss;
    int64_t bb = (( std::min(mWalk.getBottomLeftVertex().y , mOldWalk.getBottomRightVertex().y) >> VectorInt64::PRECISION_DIGITS) - 2) << VectorInt64::PRECISION_DIGITS;

    for (int64_t yy = ((std::max(mWalk.getTopLeftVertex().y , mOldWalk.getTopRightVertex().y  ) >> VectorInt64::PRECISION_DIGITS) + 2) << VectorInt64::PRECISION_DIGITS;  yy >= bb; yy -= VectorInt64::UNIT)
    {
        mOldWalk.notifyOnMoveDown(yy);
        mWalk.notifyOnMoveDown(yy);
        xxLeft = mWalk.getCurrentXLeft(yy);
        xxLeftOld = mOldWalk.getCurrentXLeft(yy);
        xxRight = mWalk.getCurrentXRight(yy);
        xxRightOld = mOldWalk.getCurrentXRight(yy);
   
        int64_t mm = ((std::min(xxLeft, xxLeftOld) >> VectorInt64::PRECISION_DIGITS) << VectorInt64::PRECISION_DIGITS) ;
        if(std::min(xxLeft, xxLeftOld) < std::max(xxRight,xxRightOld) )
        {
            for (int64_t xx = mm ; xx <= std::max(xxRight,xxRightOld) ; xx+= VectorInt64::UNIT)
            {
                bool bash = (xx >= xxLeftOld && xx <= xxRightOld && (yy >= mOldWalk.getBottomLeftVertex().y) && yy <= mOldWalk.getTopLeftVertex().y);
                bool splash = (xx >= xxLeft && xx <= xxRight && (yy >= mWalk.getBottomLeftVertex().y) && yy <= mWalk.getTopLeftVertex().y);

                xxp = ((xx >>VectorInt64::PRECISION_DIGITS));
                yyp = ((yy >>VectorInt64::PRECISION_DIGITS));
                GameMap* gm = mCm->getGameMap();
                if(bash && splash && (mode & HIDE) && (mode & SHOW))
                {
                    // Nothing
                }

                else if (gm && bash && (mode & HIDE) && xxp>=0 && yyp>= 0 && xxp<mCm->getGameMap()->getMapSizeX() && yyp < mCm->getGameMap()->getMapSizeY())
                {
                    RenderManager::getSingleton().rrDetachEntity(gm->getTile(xxp, yyp));
                }
                else if (gm && splash && (mode & SHOW) && xxp>=0 && yyp>= 0 && xxp<mCm->getGameMap()->getMapSizeX() && yyp < mCm->getGameMap()->getMapSizeY())
                {
                    RenderManager::getSingleton().rrAttachEntity(gm->getTile(xxp, yyp));
                }
            }
        }
    }
}

bool CullingManager::getIntersectionPoints()
{
    const Ogre::Vector3* activeCameraVector = mCm->getActiveCamera()->getWorldSpaceCorners();
    const Ogre::Vector3* activeCameraVector2 = mCm->getCamera("miniMapCam")->getWorldSpaceCorners();
    for(int ii = 0 ; ii < 4; ++ii)
        mActiveRay[ii]= Ogre::Ray (activeCameraVector[ii], activeCameraVector[ii+4] - activeCameraVector[ii]);

    for(int ii = 4; ii < 8; ++ii)
        mActiveRay[ii]= Ogre::Ray (activeCameraVector2[ii-4], activeCameraVector2[ii] - activeCameraVector2[ii-4]);

    std::pair<bool, Ogre::Real> intersectionResult;

    for(int ii = 0; ii < 8; ++ii)
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

bool CullingManager::onFrameStarted()
{
    if(mCullTilesFlag)
        getIntersectionPoints();
    if(mCullTilesFlag)
        cullTiles();
    return true;
}

bool CullingManager::onFrameEnded()
{
    return true;
}

/*! \brief Sort two VectorInt64 p1 and p2  to satisfy p1 <= p2 according to
 * the value of X or Y coordiante, which depends on sortByX param .
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


