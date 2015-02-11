/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
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

#include "camera/CameraManager.h"

#include "gamemap/GameMap.h"
#include "gamemap/MiniMap.h"
#include "render/ODFrameListener.h"
#include "sound/SoundEffectsManager.h"
#include "utils/LogManager.h"

#include "ODApplication.h"

#include <OgrePrerequisites.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreRenderWindow.h>

#include <algorithm>

//! The camera base moving speed.
const Ogre::Real MOVE_SPEED = 2.0;
const Ogre::Real MOVE_SPEED_ACCELERATION = 2.0 * MOVE_SPEED;

//! The camera moving speed factor on Z axis.
const Ogre::Real ZOOM_SPEED = 4.0;

//! Camera speed when clicking on the minimap or pushing the home key.
const Ogre::Real FLIGHT_SPEED = 70.0;

//! Camera rotation speed in degrees.
const Ogre::Degree ROTATION_SPEED = Ogre::Degree(90);

CameraManager::CameraManager(Ogre::SceneManager* sceneManager, GameMap* gm, Ogre::RenderWindow* renderWindow) :
    mCircleMode(false),
    mCatmullSplineMode(false),
    mFirstIter(true),
    mRadius(0.0),
    mCenterX(0),
    mCenterY(0),
    mAlpha(0.0),
    mActiveCamera(nullptr),
    mActiveCameraNode(nullptr),
    mGameMap(gm),
    mCameraIsFlying(false),
    mCameraFlightDestination(Ogre::Vector3(0.0, 0.0, 0.0)),
    mCameraIsRotating(false),
    mCameraPitchDestination(0.0),
    mCameraRollDestination(0.0),
    mZChange(0.0),
    mSwivelDegrees(0.0),
    mTranslateVector(Ogre::Vector3(0.0, 0.0, 0.0)),
    mTranslateVectorAccel(Ogre::Vector3(0.0, 0.0, 0.0)),
    mRotateLocalVector(Ogre::Vector3(0.0, 0.0, 0.0)),
    mSceneManager(sceneManager),
    mViewport(nullptr)
{
    createViewport(renderWindow);
    createCamera("RTS", 0.02, 300.0);
    createCameraNode("RTS", Ogre::Vector3(static_cast<Ogre::Real>(10.0),
                                              static_cast<Ogre::Real>(10.0),
                                              MAX_CAMERA_Z / 2.0),
                                              Ogre::Degree(0.0), Ogre::Degree(30.0));

    createCamera("FPP", 0.02, 30.0);
    createCameraNode("FPP", Ogre::Vector3(), Ogre::Degree(0), Ogre::Degree(75), Ogre::Degree(0));

    setActiveCamera("RTS");
    setActiveCameraNode("RTS");

    LogManager::getSingleton().logMessage("Created camera manager");
}

void CameraManager::createCamera(const Ogre::String& ss, double nearClip, double farClip)
{
    Ogre::Camera* tmpCamera = mSceneManager->createCamera(ss);
    tmpCamera->setNearClipDistance(static_cast<Ogre::Real>(nearClip));
    tmpCamera->setFarClipDistance(static_cast<Ogre::Real>(farClip));
    tmpCamera->setAutoTracking(false, mSceneManager->getRootSceneNode()
                                ->createChildSceneNode("CameraTarget_" + ss),
                                    Ogre::Vector3(static_cast<Ogre::Real>(mGameMap->getMapSizeX() / 2),
                                                  static_cast<Ogre::Real>(mGameMap->getMapSizeY() / 2),
                                                  static_cast<Ogre::Real>(0)));

    mRegisteredCameraNames.insert(ss);
    LogManager::getSingleton().logMessage("Creating " + ss + " camera...", Ogre::LML_NORMAL);
}

void CameraManager::createCameraNode(const Ogre::String& ss, Ogre::Vector3 xyz,
                                     Ogre::Degree yaw, Ogre::Degree pitch, Ogre::Degree roll)
{
    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode(ss + "_node", xyz);
    Ogre::Camera* tmpCamera = getCamera(ss);
    node->yaw(yaw, Ogre::Node::TS_LOCAL);
    node->pitch(pitch, Ogre::Node::TS_LOCAL);
    node->roll(roll, Ogre::Node::TS_LOCAL);
    node->attachObject(tmpCamera);
    node->setPosition(Ogre::Vector3(0,0,2) +  node->getPosition());
    mRegisteredCameraNodeNames.insert(ss);

    LogManager::getSingleton().logMessage("Creating " + ss + "_node camera node...", Ogre::LML_NORMAL);
}

void CameraManager::setDefaultIsometricView()
{
    RotateTo(40.0, 45.0);
}

void CameraManager::setDefaultOrthogonalView()
{
    RotateTo(0.0, 0.0);
}

void CameraManager::RotateTo(Ogre::Real pitch, Ogre::Real roll)
{
    mCameraIsRotating = true;
    mCameraPitchDestination = pitch;
    mCameraRollDestination = roll;
}

void CameraManager::createViewport(Ogre::RenderWindow* renderWindow)
{
    mViewport = renderWindow->addViewport(nullptr);
    mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

// TODO: Update registered camera if needed.
//    for(set<string>::iterator m_itr = mRegisteredCameraNames.begin(); m_itr != mRegisteredCameraNames.end() ; ++m_itr){
//        Ogre::Camera* tmpCamera = getCamera(*m_itr);
//    }
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating viewport...", Ogre::LML_NORMAL);
}
/*
void CameraManager::setFPPCamera(Creature* cc)
{
    // That should be done in the RenderManager
#if 0
    Ogre::SceneNode* tmpNode = mSceneManager->getSceneNode("FPP_node");
    tmpNode->getParentSceneNode()->removeChild(tmpNode);
    tmpNode->setInheritOrientation(true);
    cc->getEntityNode()->addChild(tmpNode);
#endif // 0
}*/

Ogre::SceneNode* CameraManager::getActiveCameraNode()
{
    return getActiveCamera()->getParentSceneNode();
}

Ogre::SceneNode* CameraManager::setActiveCameraNode(const Ogre::String& ss)
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Setting active camera node to " + ss + "_node ...", Ogre::LML_NORMAL);

    return mActiveCameraNode = mSceneManager->getSceneNode(ss + "_node");
}

Ogre::Camera* CameraManager::getCamera(const Ogre::String& ss)
{
    return mSceneManager->getCamera(ss);
}

void CameraManager::setActiveCamera(const Ogre::String& ss)
{
    mActiveCamera = mSceneManager->getCamera(ss);
    mViewport->setCamera(mActiveCamera);
    mActiveCamera->setAspectRatio(Ogre::Real(mViewport->getActualWidth()) / Ogre::Real(mViewport->getActualHeight()));

    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Setting Active Camera to " + ss + " ...", Ogre::LML_NORMAL);
}

void CameraManager::updateCameraFrameTime(const Ogre::Real frameTime)
{
    if (!isCameraMovingAtAll())
        return;

    // Update the minimap view
    MiniMap* minimap = ODFrameListener::getSingleton().getMiniMap();
    minimap->updateCameraInfos(getCameraViewTarget(), getActiveCameraNode()->getOrientation().getRoll().valueRadians());

    // Carry out the acceleration/deceleration calculations on the camera translation.
    Ogre::Real speed = mTranslateVector.normalise();
    mTranslateVector *= static_cast<Ogre::Real>(std::max(0.0, speed - (0.75 + (speed / MOVE_SPEED))
                        * MOVE_SPEED_ACCELERATION * frameTime));
    mTranslateVector += mTranslateVectorAccel * static_cast<Ogre::Real>(frameTime * 2.0);

    // If we have sped up to more than the maximum moveSpeed then rescale the
    // vector to that length. We use the squaredLength() in this calculation
    // since squaring the RHS is faster than sqrt'ing the LHS.
    if (mTranslateVector.squaredLength() > MOVE_SPEED * MOVE_SPEED)
    {
        speed = mTranslateVector.length();
        mTranslateVector *= MOVE_SPEED / speed;
    }

    // Get the camera's current position.
    Ogre::Vector3 newPosition =  getActiveCameraNode()->getPosition();

    // Get a quaternion which will rotate the "camera relative" x-y values
    // for the translateVector into the global x-y used to position the camera.
    Ogre::Vector3 viewTarget = getCameraViewTarget();

    Ogre::Vector3 viewDirection = viewTarget - newPosition;

    viewDirection.z = 0.0;

    Ogre::Quaternion viewDirectionQuaternion = Ogre::Vector3::UNIT_Y.getRotationTo(viewDirection);

    // Adjust the newPosition vector to account for the translation due
    // to the movement keys on the keyboard (the arrow keys and/or WASD).
    if (mZChange != 0)
    {
        newPosition.z += static_cast<Ogre::Real>(mZChange * frameTime * ZOOM_SPEED);
        // We also stow and stop the movement here, as the keyboard release events
        // and mouse wheel event are otherwise colliding on handling the zoom.
        if (mZChange > 0)
            mZChange -= MOVE_SPEED;
        else if (mZChange < 0)
            mZChange += MOVE_SPEED;
    }

    // Update the position for the other axices.
    newPosition += (viewDirectionQuaternion * mTranslateVector);

    // Prevent camera from moving down into the tiles or too high.
    if (newPosition.z <= MIN_CAMERA_Z)
        newPosition.z = MIN_CAMERA_Z;
    else if (newPosition.z >= MAX_CAMERA_Z)
        newPosition.z = MAX_CAMERA_Z;

    // Prevent the tilting to show a reversed world or looking too high.
    if (mRotateLocalVector.x != 0)
    {
        Ogre::Real currentPitch = getActiveCameraNode()->getOrientation().getPitch().valueDegrees();
        if ((currentPitch >= 0.0 && mRotateLocalVector.x < 0)
            || (currentPitch <= 50.0 && mRotateLocalVector.x > 0))
        {
            // Tilt the camera up or down.
            getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_X,
                                          Ogre::Degree(mRotateLocalVector.x * frameTime),
                                          Ogre::Node::TS_LOCAL);
        }
    }

    // Swivel the camera to the left or right, while maintaining the same
    // view target location on the ground.
    Ogre::Real deltaX = newPosition.x - viewTarget.x;
    Ogre::Real deltaY = newPosition.y - viewTarget.y;

    Ogre::Real radius = sqrt(deltaX * deltaX + deltaY * deltaY);
    Ogre::Real theta = atan2(deltaY, deltaX) + mSwivelDegrees.valueRadians() * frameTime;

    newPosition.x = viewTarget.x + radius * cos(theta);
    newPosition.y = viewTarget.y + radius * sin(theta);

    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_Z,
                                  Ogre::Degree(mSwivelDegrees * frameTime),
                                  Ogre::Node::TS_WORLD);

    // If the camera is trying to fly toward a destination, move it in that direction.
    if (mCameraIsFlying)
    {
        // Compute the direction and distance the camera needs to move
        //to get to its intended destination.
        Ogre::Vector3 flightDirection = mCameraFlightDestination - viewTarget;
        radius = flightDirection.normalise();

        // If we are withing the stopping distance of the target, then quit flying.
        // Otherwise we move towards the destination.

        if (radius <= 0.25)
        {
            // We are withing the stopping distance of the target destination
            // so stop flying towards it.
            mCameraIsFlying = false;
        }
        else
        {
            // Scale the flight direction to move towards at the given speed
            // (the min function prevents overshooting the target) then add
            // this offset vector to the camera position.
            flightDirection *= std::min(FLIGHT_SPEED * frameTime, radius);
            newPosition += flightDirection;
        }
    }

    if (mCameraIsRotating && frameTime > 0.0)
    {
        Ogre::Quaternion orientationQuat = getActiveCameraNode()->getOrientation();

        // Tilting - X-Axis - Looking up or down
        Ogre::Real pitch = orientationQuat.getPitch().valueDegrees();
        Ogre::Real pitchUpdate = ROTATION_SPEED.valueDegrees();
        Ogre::Real pitchDiff = std::abs(pitch - mCameraPitchDestination);
        Ogre::Real pitchChange = ((pitchUpdate * frameTime) > pitchDiff) ? pitchDiff / frameTime : pitchUpdate;
        bool pitchDone = false;
        if (pitchDiff < 0.5)
        {
            mRotateLocalVector.x = 0.0;
            pitchDone = true;
        }
        else if (pitch > mCameraPitchDestination)
            mRotateLocalVector.x = -pitchChange;
        else if (pitch < mCameraPitchDestination)
            mRotateLocalVector.x = pitchChange;

        // Roll - Z-Axis - Left or right
        Ogre::Real roll = orientationQuat.getRoll().valueDegrees();
        Ogre::Real rollUpdate = 1.3 * ROTATION_SPEED.valueDegrees();
        Ogre::Real rollDiff = std::abs(roll - mCameraRollDestination);
        Ogre::Real rollChange = ((rollUpdate * frameTime) > rollDiff) ? rollDiff / frameTime : rollUpdate;
        bool rollDone = false;
        if (std::abs(roll - mCameraRollDestination) < 0.5)
        {
            mSwivelDegrees = 0.0;
            rollDone = true;
        }
        else if (roll < mCameraRollDestination)
            mSwivelDegrees = rollChange;
        else if (roll > mCameraRollDestination)
            mSwivelDegrees = -rollChange;

        if (pitchDone && rollDone)
            mCameraIsRotating = false;

        /* Useful to debug... or test.
        std::cout << "Current pitch: " << pitch << ", desired pitch: " << mCameraPitchDestination << std::endl;
        std::cout << "Pitch update: " << pitchChange * frameTime << std::endl;
        std::cout << "Current roll: " << roll << ", desired roll: " << mCameraRollDestination << std::endl;
        std::cout << "Roll update: " << rollChange * frameTime << std::endl;
        */
    }

    if(mCircleMode)
    {
        mAlpha += 0.1 * frameTime;

        newPosition.x = static_cast<Ogre::Real>(cos(mAlpha) * mRadius + mCenterX);
        newPosition.y = static_cast<Ogre::Real>(sin(mAlpha) * mRadius + mCenterY);

        if(mAlpha > 2.0 * 3.145)
            mCircleMode = false;
    }
    else if(mCatmullSplineMode)
    {
        mAlpha += 0.1 * frameTime;

        //std::cout << "alpha "<< mAlpha << std::endl;

        double tempX = mXHCS.evaluate(mAlpha);
        //std::cout << "newPosition.x: " << newPosition.x << std::endl;
        double tempY = mYHCS.evaluate(mAlpha);
        //std::cout << "newPosition.y: "<< newPosition.y << std::endl;

        if (tempX < 0.9 * mGameMap->getMapSizeX() &&
                tempX > 10 &&
                tempY < 0.9 * mGameMap->getMapSizeY() &&
                tempY > 10 ) {
            newPosition.x = static_cast<Ogre::Real>(tempX);
            newPosition.y = static_cast<Ogre::Real>(tempY);
        }

        if(mAlpha > mXHCS.getNN())
            mCatmullSplineMode = false;
    }

    // Move the camera to the new location
    getActiveCameraNode()->setPosition(newPosition);

    SoundEffectsManager::getSingleton().setListenerPosition(
        newPosition,  getActiveCameraNode()->getOrientation());
}

const Ogre::Vector3 CameraManager::getCameraViewTarget()
{
    // Get the position of the camera and direction that the camera is facing.
    Ogre::Vector3 position = mActiveCamera->getRealPosition();
    Ogre::Vector3 viewDirection = mActiveCamera->getDerivedDirection();

    // Compute the offset, this is how far you would move in the x-y plane if
    // you follow along the view direction vector until you get to z = 0.
    viewDirection.normalise();
    viewDirection /= fabs(viewDirection.z);
    Ogre::Vector3 offset = position.z * viewDirection;
    offset.z = 0.0;

    // The location we are looking at is then simply the camera's positon plus
    // the view offset computed above.  We zero the z-value on the target for
    // consistency.
    Ogre::Vector3 target = position + offset;
    target.z = 0.0;

    return target;
}

void CameraManager::setCameraPosition(const Ogre::Vector3& position)
{
    getActiveCameraNode()->setPosition(position);
}


void CameraManager::flyTo(const Ogre::Vector3& destination)
{
    mCameraIsFlying = true;
    mCameraFlightDestination = destination;
}

void CameraManager::onMiniMapClick(Ogre::Vector2 cc)
{
    flyTo(Ogre::Vector3(cc.x, cc.y, 0.0));
}

void CameraManager::move(const Direction direction, double aux)
{
    // NOTE : The camera loses the desired sense of left, right, top, down
    // when the camera pitch is more than 90 degrees.
    // So we invert the panning in that case.
    Ogre::Real currentPitch = getActiveCameraNode()->getOrientation().getPitch().valueDegrees();

    switch (direction)
    {
    case moveRight:
        if (currentPitch > 0.0)
            mTranslateVectorAccel.x += MOVE_SPEED_ACCELERATION;
        else
            mTranslateVectorAccel.x -= MOVE_SPEED_ACCELERATION;
        break;

    case stopRight:
        mTranslateVectorAccel.x = 0;
        break;

    case moveLeft:
        if (currentPitch > 0.0)
            mTranslateVectorAccel.x -= MOVE_SPEED_ACCELERATION;
        else
            mTranslateVectorAccel.x += MOVE_SPEED_ACCELERATION;
        break;

    case stopLeft:
        mTranslateVectorAccel.x = 0;
        break;

    case moveBackward:
        if (currentPitch > 0.0)
            mTranslateVectorAccel.y -= MOVE_SPEED_ACCELERATION;
        else
            mTranslateVectorAccel.y += MOVE_SPEED_ACCELERATION;
        break;

    case stopBackward:
            mTranslateVectorAccel.y = 0;
        break;

    case moveForward:
        if (currentPitch > 0.0)
            mTranslateVectorAccel.y += MOVE_SPEED_ACCELERATION;
        else
            mTranslateVectorAccel.y -= MOVE_SPEED_ACCELERATION;
        break;

    case stopForward:
        mTranslateVectorAccel.y = 0;
        break;

    case moveUp:
        mZChange += MOVE_SPEED_ACCELERATION;
        break;

    case stopUp:
        break;

    case moveDown:
        mZChange -= MOVE_SPEED_ACCELERATION;
        break;

    case stopDown:
        break;

    case rotateLeft:
        mSwivelDegrees += 1.3 * ROTATION_SPEED;
        break;

    case stopRotRight:
        mSwivelDegrees = 0;
        break;

    case rotateRight:
        mSwivelDegrees -= 1.3 * ROTATION_SPEED;
        break;

    case stopRotLeft:
        mSwivelDegrees = 0;
        break;

    case rotateUp:
        mRotateLocalVector.x += ROTATION_SPEED.valueDegrees();
        break;

    case stopRotDown:
        mRotateLocalVector.x = 0;
        break;

    case rotateDown:
        mRotateLocalVector.x -= ROTATION_SPEED.valueDegrees();
        break;

    case stopRotUp:
        mRotateLocalVector.x = 0;
        break;

    case randomRotateX:
        mSwivelDegrees = Ogre::Degree(static_cast<Ogre::Real>(64 * aux));
        break;

    case zeroRandomRotateX:
        mSwivelDegrees = Ogre::Degree(0.0);
        break;

    case randomRotateY:
        mRotateLocalVector.x = static_cast<Ogre::Real>(64 * aux);
        break;

    case zeroRandomRotateY:
        mRotateLocalVector.x =  0.0;
        break;

    default:
        break;
    }
}

bool CameraManager::isCameraMovingAtAll() const
{
    return (mTranslateVectorAccel.x != 0 ||
            mTranslateVectorAccel.y != 0 ||
            mTranslateVector.x != 0 ||
            mTranslateVector.y != 0 ||
            mZChange != 0 ||
            mSwivelDegrees.valueDegrees() != 0 ||
            mRotateLocalVector.x != 0 ||
            mCameraIsFlying ||
            mCameraIsRotating);
}
