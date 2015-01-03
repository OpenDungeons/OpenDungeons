/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
 *
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

#ifndef CAMERAMANAGER_H_
#define CAMERAMANAGER_H_

#include "camera/HermiteCatmullSpline.h"

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include <OgreString.h>

#include <cstdint>
#include <set>

class TileContainer;

// The min/max camera height in tile size
const Ogre::Real MIN_CAMERA_Z = 3.0;
const Ogre::Real MAX_CAMERA_Z = 16.0;

//! \brief The default views enum, used to cycle between them.
enum class ViewModes : uint16_t
{
    defaultView = 0,
    isometricView,
    orthogonalView
};

class CameraManager
{
public:
    enum Direction
    {
        moveLeft, moveRight, moveForward, moveBackward, moveUp, moveDown,
        rotateLeft, rotateRight, rotateUp, rotateDown,

        stopLeft, stopRight, stopForward, stopBackward, stopUp, stopDown,
        stopRotLeft, stopRotRight, stopRotUp, stopRotDown,

        randomRotateX, zeroRandomRotateX,
        randomRotateY, zeroRandomRotateY,
        fullStop
    };

    CameraManager(Ogre::SceneManager* sceneManager, TileContainer* gameMap, Ogre::RenderWindow* renderWindow);
    ~CameraManager()
    {}

    inline void circleAround(int x, int y, unsigned int radius)
    {
        mCenterX = x;
        mCenterY = y;
        mRadius = radius;
        mCircleMode = true;
        mAlpha = 0;
    }

    inline void setCatmullSplineMode(bool set_mode)
    {
        mCatmullSplineMode = set_mode;
        mAlpha = 0;
    }

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
    Ogre::Vector3 getCameraViewTarget() const;

    void onMiniMapClick(Ogre ::Vector2 cc);

    /** \brief Starts the camera moving towards a destination position,
     *  it will stop moving when it gets there.
     */
    void flyTo(const Ogre::Vector3& destination);

    //! \brief Makes the camera rotate to the given orientation (in degrees).
    //! Pitch=X axis, Yaw=y axis, Roll=z axis. Right handed Ogre 3D scale.
    //! X-Axis: Looking up or down from the user point of view.
    //! Z-Axis: Left or right from a user point of view.
    void RotateTo(Ogre::Real pitch, Ogre::Real roll);

    //! \brief Directly set the new camera position
    void resetCamera(const Ogre::Vector3& position);

    /*! \brief tells the camera to move/rotate (or stop) in a specific direction
    *
    *  The function combines start and stop in one go. Giving equal momentum in
    *  one direction means either moving there (if resting before) or stoping the
    *  movement (if moving in oppsite direction before)
    */
    void move(const Direction direction, double aux = 0.0);

    void createCameraNode(const std::string& name);

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

    //! \brief Makes the RTS camera use the corresponding default viewpoint.
    void setDefaultView();
    void setDefaultIsometricView();
    void setDefaultOrthogonalView();

    //! \brief Calls the next default view in the ViewModes enum order.
    void setNextDefaultView();

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

    TileContainer* mGameMap;

    //! \brief Is true when a camera is flying to a given position.
    bool            mCameraIsFlying;

    //! \brief The camera destination when the camera is "flying".
    Ogre::Vector3   mCameraFlightDestination;

    //! \brief Is true when the camera is rotating to a given point of view.
    bool            mCameraIsRotating;

    //! \brief The camer pitch, yaw and roll destination rotation.
    Ogre::Real      mCameraPitchDestination; // X-Axis: Looking up or down from the user point of view.
    Ogre::Real      mCameraRollDestination; // Z-Axis: Left or right from a user point of view.

    //! \brief The current (or last) default view mode requested.
    ViewModes       mCurrentDefaultViewMode;

    //! \brief The user height change value.
    Ogre::Real      mZChange;

    //! \brief The Z-axis rotation, left or right from the user point of view.
    Ogre::Degree    mSwivelDegrees;

    //! \brief Carry out the acceleration/deceleration calculations on the camera translation.
    Ogre::Vector3   mTranslateVector;
    Ogre::Vector3   mTranslateVectorAccel;

    //! \brief The X-axis rotation vector, tilting the point of view (look down or up).
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
