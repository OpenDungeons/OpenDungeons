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

#include "camera/HermiteCatmullSpline.h"

#include "gamemap/GameMap.h"
#include "modes/AbstractApplicationMode.h"

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>

#include <OgreString.h>

#include <set>
#include <vector>
#include <map>

// The min/max camera height in tile size
const Ogre::Real MIN_CAMERA_Z = 3.0;
const Ogre::Real MAX_CAMERA_Z = 20.0;

class CameraManager
{
friend class Console;

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

    CameraManager(Ogre::SceneManager* sceneManager, GameMap* gameMap, Ogre::RenderWindow* renderWindow);
    ~CameraManager()
    {}

    inline void setCircleCenter(int x, int y)
    {
        mCenterX = x;
        mCenterY = y;
    }

    inline void setCircleRadius(unsigned int radius)
    { mRadius = radius; }

    inline void setCircleMode(bool set_mode)
    {
        mCircleMode = set_mode;
        mAlpha = 0;
    }

    inline void setCatmullSplineMode(bool set_mode)
    {
        mCatmullSplineMode = set_mode;
        mAlpha = 0;
    }

    void setFPPCamera(Creature*);

    inline const Ogre::Vector3& getTranslateVectorAccel() const
    {
        return mTranslateVectorAccel;
    }

    bool onFrameStarted()
    { return true; }
    bool onFrameEnded()
    { return true; }

    /*! \brief Sets the camera to a new location while still satisfying the
    * constraints placed on its movement
    */
    void updateCameraFrameTime(const Ogre::Real frameTime);

    /*! \brief Computes a vector whose z-component is 0 and whose x-y coordinates
    * are the position on the floor that the camera is pointed at.
    */
    const Ogre::Vector3 getCameraViewTarget();

    void onMiniMapClick(Ogre ::Vector2 cc);

    /** \brief Starts the camera moving towards a destination position,
     *  it will stop moving when it gets there.
     */
    void flyTo(const Ogre::Vector3& destination);

    //! \brief Directly set the new camera position
    void setCameraPosition(const Ogre::Vector3& position);

    /*! \brief tells the camera to move/rotate (or stop) in a specific direction
    *
    *  The function combines start and stop in one go. Giving equal momentum in
    *  one direction means either moving there (if resting before) or stoping the
    *  movement (if moving in oppsite direction before)
    */
    void move(const Direction direction, double aux = 0.0);

    void createCameraNode(const Ogre::String& ss, Ogre::Vector3 = Ogre::Vector3(0,0,0),
                          Ogre::Degree = Ogre::Degree(0),
                          Ogre::Degree = Ogre::Degree (0),
                          Ogre::Degree = Ogre::Degree (0));

    Ogre::SceneNode* getActiveCameraNode();
    Ogre::SceneNode* setActiveCameraNode(const Ogre::String& ss);

    //! \brief Sets up the main camera
    void createCamera(const Ogre::String& ss, double nearClip, double farClip);

    void setActiveCamera(const Ogre::String& ss);

    inline Ogre::Camera* getActiveCamera()
    {
        return  mActiveCamera;
    }

    Ogre::Camera* getCamera(const Ogre::String& ss);

    Ogre::Viewport* getViewport()
    { return mViewport; }

    void resetHCSNodes(int nodeValue)
    {
        mXHCS.resetNodes(nodeValue);
        mYHCS.resetNodes(nodeValue);
    }

    void addHCSNodes(int XNode, int YNode)
    {
        mXHCS.addNode(XNode);
        mYHCS.addNode(YNode);
    }

private:
    //! \brief HermiteCatmullSpline members for each axices.
    HermiteCatmullSpline mXHCS;
    HermiteCatmullSpline mYHCS;

    std::set<Ogre::String> mRegisteredCameraNames;
    std::set<Ogre::String> mRegisteredCameraNodeNames;

    bool mCircleMode;
    bool mCatmullSplineMode;
    bool mFirstIter;

    double mRadius;
    int mCenterX;
    int mCenterY;
    double mAlpha;

    Ogre::Camera* mActiveCamera;
    Ogre::SceneNode* mActiveCameraNode;

    GameMap* mGameMap;

    bool            mCameraIsFlying;
    Ogre::Real      mZChange;
    Ogre::Degree    mSwivelDegrees;
    Ogre::Vector3   mTranslateVector;
    Ogre::Vector3   mTranslateVectorAccel;
    Ogre::Vector3   mCameraFlightDestination;
    Ogre::Vector3   mRotateLocalVector;
    Ogre::SceneManager* mSceneManager;
    Ogre::Viewport* mViewport;

    //! \brief setup the viewport
    void createViewport(Ogre::RenderWindow* renderWindow);

    //! \brief Checks if the camera is moving at all by evaluating all momentums
    //! This permits to avoid useless computations when the camera doesn't move.
    bool isCameraMovingAtAll() const;
};

#endif // CAMERAMANAGER_H_
