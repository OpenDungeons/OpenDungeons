/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
 *
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

#ifndef CAMERAMANAGER_H_
#define CAMERAMANAGER_H_

#include "GameMap.h"
#include "AbstractApplicationMode.h"
#include "HermiteCatmullSpline.h"

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>

#include <OgreString.h>

#include <set>
#include <vector>
#include <map>

class ModeManager;
class Console;
class Viewport;

using std::vector; using std::pair; using std::map;

// A custom vector used commonly between the CameraManager and the CullingManager classes
struct Vector3i
{
    Vector3i(const Ogre::Vector3& OV)
    {
        x = (int)((1 << 10) * OV.x);
        y = (int)((1 << 10) * OV.y);
        z = (int)((1 << 10) * OV.z);
    }

    Vector3i():
        x(0),
        y(0),
        z(0)
    {}

    int x;
    int y;
    int z;
};

class CameraManager
{
    friend class Console;
    friend class CullingManager;

public:
    enum Direction
    {
        moveLeft, moveRight, moveForward, moveBackward, moveUp, moveDown,
        rotateLeft, rotateRight, rotateUp, rotateDown,

        stopLeft, stopRight, stopForward, stopBackward, stopUp, stopDown,
        stopRotLeft, stopRotRight, stopRotUp, stopRotDown,

        randomRotateX,	zeroRandomRotateX,
        randomRotateY,	zeroRandomRotateY,
        fullStop
    };

    HermiteCatmullSpline xHCS;
    HermiteCatmullSpline yHCS;

    CameraManager(Ogre::SceneManager*, GameMap*);
    ~CameraManager();

    inline void setCircleCenter( int XX, int YY)
    {
        mCenterX = XX;
        mCenterY = YY;
    }

    inline void setCircleRadius(unsigned int rr)
    { mRadius = rr; }

    inline void setCircleMode(bool mm)
    {
        mCircleMode = mm;
        mAlpha = 0;
    }

    inline void setCatmullSplineMode(bool mm)
    {
        mCatmullSplineMode = mm;
        mAlpha = 0;
    }

    inline bool switchPM()
    {
        mSwitchedPM = true;
        return true;
    }

    void setModeManager(ModeManager* mm)
    { mModeManager = mm; }

    void setFPPCamera(Creature*);

    //get/set moveSpeed
    inline const Ogre::Real& getMoveSpeed() const
    {
        return mMoveSpeed;
    }

    inline void setMoveSpeed(const Ogre::Real& newMoveSpeed)
    {
        mMoveSpeed = newMoveSpeed;
    }

    //get/set moveSpeedAccel
    inline const Ogre::Real& getMoveSpeedAccel() const
    {
        return mMoveSpeedAccel;
    }

    inline void setMoveSpeedAccel(const Ogre::Real& newMoveSpeedAccel)
    {
        mMoveSpeed = newMoveSpeedAccel;
    }

    //get/set rotateSpeed
    inline const Ogre::Degree& getRotateSpeed() const
    {
        return mRotateSpeed;
    }

    inline void setRotateSpeed(const Ogre::Degree& newRotateSpeed)
    {
        mRotateSpeed = newRotateSpeed;
    }

    //get translateVectorAccel
    inline const Ogre::Vector3& getTranslateVectorAccel() const
    {
        return mTranslateVectorAccel;
    }

    bool getIntersectionPoints();

    bool isCamMovingAtAll() const;

    int updateCameraView();

    bool onFrameStarted();
    bool onFrameEnded();

    void moveCamera(const Ogre::Real frameTime);
    const Ogre::Vector3 getCameraViewTarget();
    void onMiniMapClick(Ogre ::Vector2 cc);

    /** \brief Starts the camera moving towards a destination position,
     *  it will stop moving when it gets there.
     */
    void flyTo(const Ogre::Vector3& destination);

    //! \brief Directly set the new camera position
    void setCameraPosition(const Ogre::Vector3& position);

    void move(const Direction direction, double aux = 0.0);

    inline void stopZooming ()
    {
        mZChange = 0;
    }

    void createCameraNode(const Ogre::String& ss, Ogre::Vector3 = Ogre::Vector3(0,0,0),
                          Ogre::Degree = Ogre::Degree(0),
                          Ogre::Degree = Ogre::Degree (0),
                          Ogre::Degree = Ogre::Degree (0));

    Ogre::SceneNode* getActiveCameraNode();
    Ogre::SceneNode* setActiveCameraNode(const Ogre::String& ss);

    void createCamera(const Ogre::String& ss, double nearClip, double farClip);
    void setActiveCamera(const Ogre::String& ss);

    inline Ogre::Camera* getActiveCamera()
    {
        return  mActiveCamera ;
    }

    Ogre::Camera* getCamera(const Ogre::String& ss);

    Ogre::Viewport* getViewport();
    void createViewport();

private:
    bool mSwitchedPM;
    std::set<Ogre::String> mRegisteredCameraNames;
    std::set<Ogre::String> mRegisteredCameraNodeNames;

    bool mCircleMode;
    bool mCatmullSplineMode;
    bool mFirstIter;

    double mRadius;
    int mCenterX;
    int mCenterY;
    double mAlpha;

    ModeManager* mModeManager;
    CullingManager* mCullingManager;

    Ogre::Camera* mActiveCamera;
    Ogre::SceneNode* mActiveCameraNode;

    GameMap* mGameMap;
    bool            mCameraIsFlying;
    Ogre::Real      mMoveSpeed;
    Ogre::Real      mMoveSpeedAccel;
    Ogre::Real      mCameraFlightSpeed;
    Ogre::Degree    mRotateSpeed;
    Ogre::Degree    mSwivelDegrees;
    Ogre::Vector3   mTranslateVector;
    Ogre::Vector3   mTranslateVectorAccel;
    Ogre::Vector3   mCameraFlightDestination;
    Ogre::Vector3   mRotateLocalVector;
    Ogre::SceneManager* mSceneManager;
    Ogre::Viewport* mViewport;
    double          mZChange;
    float           mZoomSpeed;

    std::string switchPolygonMode();
    void sort(Vector3i& p1 , Vector3i& p2, bool sortByX);
};

#endif /* CAMERAMANAGER_H_ */
