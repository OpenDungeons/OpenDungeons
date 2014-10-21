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

#include "camera/CullingManager.h"

#include "camera/MortuaryQuad.h"

#include "entities/Creature.h"
#include "utils/Vector3i.h"

#include <algorithm>

using  std::set; using std::swap; using std::max; using std::min;
using  std::cerr; using std::endl;

//! Values used to know whether to show and/or hide a mesh
static const int HIDE =  1;
static const int SHOW =  2;

extern const int mPrecisionDigits ;
extern const int Unit ;


CullingManager::CullingManager(CameraManager* cameraManager):
    mCurrentVisibleCreatures(&mCreaturesSet[0]),
    mPreviousVisibleCreatures(&mCreaturesSet[1]),
    mFirstIter(false),
    mCm(cameraManager),
    mCullCreaturesFlag(false),
    mCullTilesFlag(false),
    mDebug(false)
{
    mMyplanes[0]=(Ogre::Plane(0, 0, 1, 0));
    mMyplanes[1]=(Ogre::Plane(0, 0, -1, 20));
    mMyplanes[2]=(Ogre::Plane(0, 1, 0, -1));
    mMyplanes[3]=(Ogre::Plane(0, -1, 0, 395));
    mMyplanes[4]=(Ogre::Plane(1, 0, 0, -1));
    mMyplanes[5]=(Ogre::Plane(-1, 0, 0, 395));
    mMyCullingQuad.setRadius(256);
    mMyCullingQuad.setCenter(200, 200);

    // init the Ogre vector
    for (unsigned int i = 0; i < 4; ++i)
    {
        mOgreVectorsArray[i].x = (Ogre::Real)0.0;
        mOgreVectorsArray[i].y = (Ogre::Real)0.0;
        mOgreVectorsArray[i].z = (Ogre::Real)0.0;
    }
}

void CullingManager::cullTiles()
{
    // mOldTop = mTop ;
    // mOldBottom = mBottom ;
    // mOldMiddleLeft = mMiddleLeft ;
    // mOldMiddleRight = mMiddleRight;

    // mTop = Vector3i(mOgreVectorsArray[0]);
    // mMiddleLeft = Vector3i(mOgreVectorsArray[1]);
    // mBottom = Vector3i(mOgreVectorsArray[2]);
    // mMiddleRight = Vector3i(mOgreVectorsArray[3]);

    // sort(mBottom, mTop, false);
    // sort(mMiddleLeft, mMiddleRight, false);
    // sort(mMiddleRight, mTop, false);
    // sort(mBottom, mMiddleLeft, false);
    // sort(mMiddleLeft, mMiddleRight, true);

    oldWalk = mWalk;
    mWalk.myArray[0] =  Vector3i(mOgreVectorsArray[0]);
    mWalk.myArray[1] =  Vector3i(mOgreVectorsArray[1]);
    mWalk.myArray[2] =  Vector3i(mOgreVectorsArray[2]);
    mWalk.myArray[3] =  Vector3i(mOgreVectorsArray[3]);
    mWalk.buildSlopes();

    oldWalk.prepareWalk();
    mWalk.prepareWalk();

    newBashAndSplashTiles(SHOW | HIDE);
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
    // mTop = Vector3i(mOgreVectorsArray[0]);
    // mMiddleLeft = Vector3i(mOgreVectorsArray[1]);
    // mBottom = Vector3i(mOgreVectorsArray[2]);
    // mMiddleRight = Vector3i(mOgreVectorsArray[3]);

    // mOldTop = mTop;
    // mOldBottom = mBottom;
    // mOldMiddleLeft = mMiddleLeft;
    // mOldMiddleRight = mMiddleRight;

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


int CullingManager::cullCreatures()
{
    //cerr << "countnodes " << myCullingQuad.countNodes() <<endl;
    mMyCullingQuad.holdRootSemaphore();
    MortuaryQuad tmpQuad(mMyCullingQuad);

    mMyCullingQuad.releaseRootSemaphore();

    tmpQuad.cut(Segment(mOgreVectorsArray[1], mOgreVectorsArray[0]));
    tmpQuad.cut(Segment(mOgreVectorsArray[0], mOgreVectorsArray[3]));
    tmpQuad.cut(Segment(mOgreVectorsArray[3], mOgreVectorsArray[2]));
    tmpQuad.cut(Segment(mOgreVectorsArray[2], mOgreVectorsArray[1]));

    //cerr << "tmpQuad.countNodes() " << tmpQuad.countNodes() <<endl;

    std::swap(mCurrentVisibleCreatures, mPreviousVisibleCreatures);
    mCurrentVisibleCreatures = tmpQuad.returnCreaturesSet(mCurrentVisibleCreatures);
    //cerr << "currentVisibleCreatures  " << currentVisibleCreatures->size() <<endl;

    std::set<Creature*> intersection;
    std::set<Creature*> ascendingCreatures;
    std::set<Creature*> descendingCreatures;
    std::set_intersection(mPreviousVisibleCreatures->begin(), mPreviousVisibleCreatures->end(),
                          mCurrentVisibleCreatures->begin(), mCurrentVisibleCreatures->end(),
                          std::inserter(intersection, intersection.end()));

    std::set_difference(mCurrentVisibleCreatures->begin(), mCurrentVisibleCreatures->end(),
                        intersection.begin(), intersection.end(),
                        std::inserter(ascendingCreatures, ascendingCreatures.end()));

    std::set_difference(mPreviousVisibleCreatures->begin(), mPreviousVisibleCreatures->end(),
                        intersection.begin(), intersection.end(),
                        std::inserter(descendingCreatures, descendingCreatures.end()));

    std::set_difference(mPreviousVisibleCreatures->begin(), mPreviousVisibleCreatures->end(),
                        intersection.begin(), intersection.end(),
                        std::inserter(descendingCreatures, descendingCreatures.end()));

    for(std::vector<Creature*>::iterator it = tmpQuad.mMortuaryQuad.begin(); it != tmpQuad.mMortuaryQuad.end(); ++it)
        descendingCreatures.erase(*it);

    //cerr << "ascendingCreatures  " << ascendingCreatures.size() <<endl;
    //cerr << "descendingCreatures " << descendingCreatures.size()<<endl;

    // sort the new tiles to form the proper diamod
    for(std::set<Creature*>::iterator it = ascendingCreatures.begin(); it != ascendingCreatures.end(); ++it)
        (*it)->show();

    for(std::set<Creature*>::iterator it = descendingCreatures.begin(); it != descendingCreatures.end(); ++it)
        (*it)->hide();

    return 1;
}


void CullingManager::newBashAndSplashTiles(int64_t mode){
    int64_t xxLeftOld = oldWalk.getTopLeftVertex().x;
    int64_t xxRightOld= oldWalk.getTopRightVertex().x;
    int64_t xxLeft = mWalk.getTopLeftVertex().x;
    int64_t xxRight= mWalk.getTopRightVertex().x;

    if(mDebug)
    {
        mWalk.printState();
        mDebug = false;
    }
    int64_t bb = (( std::min(mWalk.getBottomLeftVertex().y , oldWalk.getBottomLeftVertex().y) >> mPrecisionDigits) - 2) << mPrecisionDigits;

    for (int64_t yy = ((std::max(mWalk.getTopLeftVertex().y , oldWalk.getTopLeftVertex().y  ) >> mPrecisionDigits) + 2) << mPrecisionDigits;  yy >= bb; yy -= Unit)
    {
	mWalk.notifyOnMoveDown(yy);
	oldWalk.notifyOnMoveDown(yy);
	xxLeft -= mWalk.getCurrentDxLeft();
	xxLeftOld -= oldWalk.getCurrentDxLeft();
	xxRight -= mWalk.getCurrentDxRight();
	xxRightOld -= oldWalk.getCurrentDxRight();

	// cerr << (xxLeft  >> mPrecisionDigits )<< " " << (xxLeftOld  >> mPrecisionDigits )<< " " << (xxRight  >> mPrecisionDigits )<< " " << (xxRightOld  >> mPrecisionDigits )<< " " <<endl ;
	// cerr << "DxLeft, DxRight: " << mWalk.getCurrentDxLeft() << " " << mWalk.getCurrentDxRight() << " " << endl;

	int64_t mm = ((std::min(xxLeft, xxLeftOld) >> mPrecisionDigits) << mPrecisionDigits) ;
	if(std::min(xxLeft, xxLeftOld) < max(xxRight,xxRightOld) )
        for (int64_t xx = mm ; xx <= max(xxRight,xxRightOld) ; xx+= Unit){
            bool bash = (xx >= xxLeftOld && xx <= xxRightOld && (yy >= oldWalk.getBottomLeftVertex().y) && yy <= oldWalk.getTopLeftVertex().y);
            bool splash = (xx >= xxLeft && xx <= xxRight && (yy >= mWalk.getBottomLeftVertex().y) && yy <= mWalk.getTopLeftVertex().y);


	    // cerr<<"CullingManager "   << " x" <<  (xx >> mPrecisionDigits )<< " y" << (yy >> mPrecisionDigits ) << " " << (splash && (mode & SHOW)) << (bash && (mode & HIDE)) << endl;
            GameMap* gm = mCm->mGameMap;
            if(bash && splash && (mode & HIDE) && (mode & SHOW))
            {
                // Nothing
            }
            else if (gm && bash && (mode & HIDE))
                gm->getTile(xx >> mPrecisionDigits, yy >> mPrecisionDigits)->hide();
            else if (gm && splash && (mode & SHOW))
                gm->getTile(xx >> mPrecisionDigits, yy >> mPrecisionDigits)->show();
	}

    }
}

/*! \brief Auxilary function, according to mode flags : SHOW and HIDE will try to show or hide a tile in single pass
 *  In each call there are two processes : one which traces the old camera view ( the one which would hide old Tiles ) , and one
 *  which traces the new camera view ( the one which would show new Tiles). Mode parameter allows to only activate one of those processes, or activate both or none :)
 *  TODO : IMPLEMENT THE FLOOR AND CEIL MATH FUNCTIONS FOR FRACTURE VALUES, SO THAT THE EDGEING TILES ( IN CAMERA VIEW ) ARE NOT CULLED AWAY
 */
int64_t CullingManager::bashAndSplashTiles(int64_t mode)
{
    int64_t xxLeftOld = mOldTop.x;
    int64_t xxRightOld= mOldTop.x;

    int64_t dxLeftOld1 = (int64_t)(mOldMiddleLeft.x - mOldTop.x) * (1 << mPrecisionDigits) / (int64_t)(mOldTop.y - mOldMiddleLeft.y);
    int64_t dxRightOld1 = (int64_t)(mOldMiddleRight.x - mOldTop.x) * (1 << mPrecisionDigits) / (int64_t)(mOldTop.y - mOldMiddleRight.y);

    int64_t dxLeftOld2 = (int64_t)(mOldBottom.x - mOldMiddleLeft.x) * (1 << mPrecisionDigits) / (int64_t)(mOldMiddleLeft.y - mOldBottom.y);
    int64_t dxRightOld2 =(int64_t)(mOldBottom.x - mOldMiddleRight.x) * (1 << mPrecisionDigits) / (int64_t)(mOldMiddleRight.y - mOldBottom.y);

    int64_t xxLeft = mTop.x;
    int64_t xxRight= mTop.x;

    int64_t  dxLeft1 = (int64_t)(mMiddleLeft.x - mTop.x) * (1 << mPrecisionDigits) / (int64_t)(mTop.y - mMiddleLeft.y);
    int64_t  dxRight1 = (int64_t)(mMiddleRight.x - mTop.x) * (1 << mPrecisionDigits) / (int64_t)(mTop.y - mMiddleRight.y);

    int64_t  dxLeft2 = (int64_t)(mBottom.x - mMiddleLeft.x) * (1 << mPrecisionDigits) / (int64_t)(mMiddleLeft.y - mBottom.y);
    int64_t  dxRight2 =(int64_t)(mBottom.x - mMiddleRight.x) * (1 << mPrecisionDigits) / (int64_t)(mMiddleRight.y - mBottom.y);

    int64_t bb = std::min(mBottom.y, mOldBottom.y);

    for (int64_t yy = ((std::max(mTop.y, mOldTop.y) >> mPrecisionDigits) + 1) << mPrecisionDigits;
         yy >= bb; yy -= (1 << mPrecisionDigits))
    {
    //  if(yy == top.y)splashY=!splashY;

    //  if(yy == oldTop.y)bashY=!bashY;

        if (yy > mMiddleLeft.y && yy <= mTop.y)
        {
            xxLeft += dxLeft1;
        }
        else if(yy <= mMiddleLeft.y && yy > mBottom.y)
        {
            xxLeft += dxLeft2;
        }

        if (yy > mMiddleRight.y && yy <= mTop.y)
        {
            xxRight += dxRight1;
        }
        else if(yy <= mMiddleRight.y && yy > mBottom.y)
        {
            xxRight += dxRight2;
        }

        if (yy > mOldMiddleLeft.y && yy <= mOldTop.y)
        {
            xxLeftOld += dxLeftOld1;
        }
        else if(yy <= mOldMiddleLeft.y && yy > mOldBottom.y)
        {
            xxLeftOld += dxLeftOld2;
        }

        if ( yy > mOldMiddleRight.y && yy <= mOldTop.y)
        {
            xxRightOld += dxRightOld1;
        }
        else if( yy <= mOldMiddleRight.y && yy > mOldBottom.y)
        {
            xxRightOld += dxRightOld2;
        }

        int64_t rr =  max(xxRight,xxRightOld);

        for (int64_t xx = ((std::min(xxLeft, xxLeftOld) >> mPrecisionDigits) - 1) << mPrecisionDigits;
             xx <= rr; xx+= (1 << mPrecisionDigits))
        {
        //  if(xx <=(int64_t)xxLeft ) splashX=!splashX;
        //  if(xx <=(int64_t)xxLeftOld  && xx >(int64_t)xxRightOld) ) bashX=true;

            bool splash = (xx >= (int64_t)xxLeft && xx <= (int64_t)xxRight && (yy >= (int64_t)mBottom.y) && yy <= (int64_t)mTop.y)  ;
            bool bash = (xx >= (int64_t)xxLeftOld && xx <= (int64_t)xxRightOld && (yy >= (int64_t)mOldBottom.y) && yy <= (int64_t)mOldTop.y);

        //  cerr<< " x" <<  xx  << " y" << yy << " " <<bash<<splash << endl;
            GameMap* gm = mCm->mGameMap;
            if(bash && splash && (mode & HIDE) && (mode & SHOW))
            {
                // Nothing
            }
            else if (gm && bash && (mode & HIDE))
                gm->getTile(xx >> mPrecisionDigits, yy >> mPrecisionDigits)->hide();
            else if (gm && splash && (mode & SHOW))
                gm->getTile(xx >> mPrecisionDigits, yy >> mPrecisionDigits)->show();

        //  if(xx >(int64_t)xxRightOld) bashX=!bashX;
        //  if(xx >(int64_t)xxRight)splashX=!splashX;
        }

    //  if(yy < (int64_t)oldBottom.y)bashY=!bashY;
    //  if(yy < (int64_t)bottom.y)splashY=!splashY;
    }
    return 1;
}

bool CullingManager::getIntersectionPoints()
{
    const Ogre::Vector3* myvector = mCm->getActiveCamera()->getWorldSpaceCorners();

    mMyRay[0]= Ogre::Ray (myvector[0], myvector[4] - myvector[0]);
    mMyRay[1]= Ogre::Ray (myvector[1], myvector[5] - myvector[1]);
    mMyRay[2]= Ogre::Ray (myvector[2], myvector[6] - myvector[2]);
    mMyRay[3]= Ogre::Ray (myvector[3], myvector[7] - myvector[3]);

    std::pair<bool, Ogre::Real> intersectionResult;

    //  Ogre::Vector3* pp = NULL;
    //  for(int ii = 0 ; ii <4 ; ii++)
    //  {
    //      intersectionResult = mMyRay[ii]->intersects(*mMyplanes[0]);
    //      ++rr;
    //      if(intersectionResult.first)
    //          pp= myRay[ii]->getPoint(intersectionResult.second);
    //  }

    for(int ii = 0; ii < 4; ++ii)
    {
        int rr = 0;

        do
        {
            intersectionResult =  mMyRay[ii].intersects(mMyplanes[rr]);
            ++rr;
            if(intersectionResult.first)
                mOgreVectorsArray[ii]= (mMyRay[ii].getPoint(intersectionResult.second));
        }
        while(!((intersectionResult.first && mOgreVectorsArray[ii].x >= 0.0
                 && mOgreVectorsArray[ii].x <= 396.0 && mOgreVectorsArray[ii].y >= 0.0 && mOgreVectorsArray[ii].y <= 396.0))
              && rr < 6);

        if(intersectionResult.first)
        {
        }
        else
        {
            //cerr<< "I didn't find the intersection point for " << ii <<"th ray"<<endl;
            //exit(1);
        }
    }

    //cerr << endl << "intersection points" << endl;
    //cerr << mOgreVectorsArray[1].x << " " << mOgreVectorsArray[1].y << " " << mOgreVectorsArray[1].z <<endl;
    //cerr << mOgreVectorsArray[2].x << " " << mOgreVectorsArray[2].y << " " << mOgreVectorsArray[2].z <<endl;
    //cerr << mOgreVectorsArray[3].x << " " << mOgreVectorsArray[3].y << " " << mOgreVectorsArray[3].z <<endl;
    //cerr << mOgreVectorsArray[0].x << " " << mOgreVectorsArray[0].y << " " << mOgreVectorsArray[0].z <<endl;

    return true;
}


bool CullingManager::onFrameStarted()
{
    if(mCullTilesFlag || mCullCreaturesFlag)
        getIntersectionPoints();
    if(mCullTilesFlag)
        cullTiles();
    if(mCullCreaturesFlag)
        cullCreatures();
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


