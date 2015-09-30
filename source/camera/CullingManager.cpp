/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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
#include "utils/Vector3i.h"
#include "utils/LogManager.h"
#include <sstream>
#include <algorithm>

using  std::set; using std::swap; using std::max; using std::min;
using  std::cerr; using std::endl;

//! Values used to know whether to show and/or hide a mesh
static const int HIDE =  1;
static const int SHOW =  2;

extern const int mPrecisionDigits ;
extern const int Unit ;


CullingManager::CullingManager(CameraManager* cameraManager):
    mDebug(false),
    mFirstIter(false),
    mCm(cameraManager),
    mCullCreaturesFlag(false),
    mCullTilesFlag(false)
{
    mMyplanes =(Ogre::Plane(0, 0, 1, 0));
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
    oldWalk = mWalk;
    mWalk.myArray[0] =  Vector3i(mOgreVectorsArray[0]);
    mWalk.myArray[1] =  Vector3i(mOgreVectorsArray[1]);
    mWalk.myArray[2] =  Vector3i(mOgreVectorsArray[2]);
    mWalk.myArray[3] =  Vector3i(mOgreVectorsArray[3]);

    
    // create a slope -- a set of left and rigth path
    mWalk.buildSlopes();

    if(mDebug)
    {
        LogManager::getSingleton().logMessage(oldWalk.debug(),LogMessageLevel::NORMAL);
        LogManager::getSingleton().logMessage(mWalk.debug(),LogMessageLevel::NORMAL);
        
    }

    // reset index pointers to the begging of collections
    oldWalk.prepareWalk();
    mWalk.prepareWalk();

    newBashAndSplashTiles(SHOW | HIDE);
    mDebug = false;
}

void CullingManager::startCreatureCulling()
{
    mCullCreaturesFlag = true;
}

void CullingManager::startTileCulling()
{
    getIntersectionPoints();

    mWalk.myArray[0] =  Vector3i(mOgreVectorsArray[0]);
    mWalk.myArray[1] =  Vector3i(mOgreVectorsArray[1]);
    mWalk.myArray[2] =  Vector3i(mOgreVectorsArray[2]);
    mWalk.myArray[3] =  Vector3i(mOgreVectorsArray[3]);
    mWalk.buildSlopes();
    oldWalk = mWalk;
    oldWalk.prepareWalk();
    mWalk.prepareWalk();
    hideAllTiles();
    newBashAndSplashTiles(SHOW);

    mCullTilesFlag = true;
}

void CullingManager::startDebugging()
{
    mDebug = true;
}

void CullingManager::stopCreatureCulling()
{
    mCullCreaturesFlag = false;
}

void CullingManager::stopTileCulling()
{
    // mOldTop = mTop;
    // mOldBottom = mBottom;
    // mOldMiddleLeft = mMiddleLeft;
    // mOldMiddleRight = mMiddleRight;

    oldWalk = mWalk;
    newBashAndSplashTiles(HIDE);
    showAllTiles();
    mCullTilesFlag = false;
}

void CullingManager::hideAllTiles(void)
{
    GameMap* gm = mCm->mGameMap;
    if (!gm)
        return;

    for (int jj = 0; jj < gm->getMapSizeY() ; ++jj)
    {
        for (int ii = 0; ii < gm->getMapSizeX(); ++ii)
        {
            gm->getTile(ii,jj)->hide();
        }
    }
}

void CullingManager::showAllTiles(void)
{
    GameMap* gm = mCm->mGameMap;
    if (!gm)
        return;

    for (int jj = 0; jj < gm->getMapSizeY() ; ++jj)
    {
        for (int ii = 0; ii < gm->getMapSizeX(); ++ii)
        {
            gm->getTile(ii,jj)->show();
        }
    }
}

void CullingManager::newBashAndSplashTiles(int64_t mode){
    int64_t xxLeftOld = oldWalk.getTopLeftVertex().x;
    int64_t xxRightOld= oldWalk.getTopRightVertex().x;
    int64_t xxLeft = mWalk.getTopLeftVertex().x;
    int64_t xxRight= mWalk.getTopRightVertex().x;
    int64_t DxRight, DxLeft;
    int64_t xxp, yyp;
    std::stringstream ss;
    int64_t bb = (( std::min(mWalk.getBottomLeftVertex().y , oldWalk.getBottomRightVertex().y) >> mPrecisionDigits) - 2) << mPrecisionDigits;

    for (int64_t yy = ((std::max(mWalk.getTopLeftVertex().y , oldWalk.getTopRightVertex().y  ) >> mPrecisionDigits) + 2) << mPrecisionDigits;  yy >= bb; yy -= Unit)
    {
        oldWalk.notifyOnMoveDown(yy);
        mWalk.notifyOnMoveDown(yy);
        DxLeft = mWalk.getCurrentXLeft(yy);
        xxLeft = DxLeft;
        xxLeftOld = oldWalk.getCurrentXLeft(yy);
        DxRight = mWalk.getCurrentXRight(yy);
        xxRight = DxRight;
        xxRightOld = oldWalk.getCurrentXRight(yy);
   
        if(mDebug)
        {
            ss << endl;
            ss << "DxLeft, DxRight: " << double(DxLeft)/Unit << " " << double(DxRight)/Unit << " " ;
            LogManager::getSingleton().logMessage( ss.str(),LogMessageLevel::NORMAL);
            ss.str("");
            ss << "xxLeft " << (xxLeft  >> mPrecisionDigits) << "xxRight " << (xxRight  >> mPrecisionDigits );
            LogManager::getSingleton().logMessage( ss.str(),LogMessageLevel::NORMAL);
            ss.str("");
        }
    
        int64_t mm = ((std::min(xxLeft, xxLeftOld) >> mPrecisionDigits) << mPrecisionDigits) ;
        if(std::min(xxLeft, xxLeftOld) < max(xxRight,xxRightOld) )
        {
            for (int64_t xx = mm ; xx <= max(xxRight,xxRightOld) ; xx+= Unit)
            {
                bool bash = (xx >= xxLeftOld && xx <= xxRightOld && (yy >= oldWalk.getBottomLeftVertex().y) && yy <= oldWalk.getTopLeftVertex().y);
                bool splash = (xx >= xxLeft && xx <= xxRight && (yy >= mWalk.getBottomLeftVertex().y) && yy <= mWalk.getTopLeftVertex().y);

                if (mDebug)
                {
                    ss<< " x " <<  (xx >> mPrecisionDigits )<< " y " << (yy >> mPrecisionDigits ) << " " << (splash && (mode & SHOW)) << (bash && (mode & HIDE)) << endl;
            
                }
                xxp = ((xx >>mPrecisionDigits) - 128.0);
                yyp = ((yy >>mPrecisionDigits) - 128.0);
                GameMap* gm = mCm->mGameMap;
                if(bash && splash && (mode & HIDE) && (mode & SHOW))
                {
                    // Nothing
                }

                else if (gm && bash && (mode & HIDE) && xxp>=0 && yyp>= 0 && xxp<mCm->mGameMap->getMapSizeX() && yyp < mCm->mGameMap->getMapSizeY())
                    gm->getTile(xxp, yyp)->hide();
                else if (gm && splash && (mode & SHOW) && xxp>=0 && yyp>= 0 && xxp<mCm->mGameMap->getMapSizeX() && yyp < mCm->mGameMap->getMapSizeY())
                    gm->getTile(xxp, yyp)->show();
            }
        }
    }
}

bool CullingManager::getIntersectionPoints()
{
    const Ogre::Vector3* myvector = mCm->getActiveCamera()->getWorldSpaceCorners();

    mMyRay[0]= Ogre::Ray (myvector[0], myvector[4] - myvector[0]);
    mMyRay[1]= Ogre::Ray (myvector[1], myvector[5] - myvector[1]);
    mMyRay[2]= Ogre::Ray (myvector[2], myvector[6] - myvector[2]);
    mMyRay[3]= Ogre::Ray (myvector[3], myvector[7] - myvector[3]);

    std::pair<bool, Ogre::Real> intersectionResult;

    for(int ii = 0; ii < 4; ++ii)
    {
        intersectionResult =  mMyRay[ii].intersects(mMyplanes);
        if(intersectionResult.first)
            mOgreVectorsArray[ii]= (mMyRay[ii].getPoint(intersectionResult.second));
        else
        {
            cerr<< "I didn't find the intersection point for " << ii <<"th ray"<<endl;
            exit(1);
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

/*! \brief Sort two Vector3i p1 and p2  to satisfy p1 <= p2 according to
 * the value of X or Y coordiante, which depends on sortByX param .
 */
void CullingManager::sort(Vector3i& p1, Vector3i& p2, bool sortByX)
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


