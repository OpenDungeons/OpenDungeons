/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
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

#include "CameraManager.h"

#include "SoundEffectsHelper.h"
#include "CullingQuad.h"
#include "ModeManager.h"
#include "HermiteCatmullSpline.h"
#include "ODApplication.h"
#include "Creature.h"
#include "LogManager.h"
#include "CullingManager.h"

#include <OGRE/OgrePrerequisites.h>
#include <OGRE/OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>

#include <algorithm>

using  std::set; using  std::swap; using  std::max; using  std::min;
using  std::cerr; using std::endl;

CameraManager::CameraManager(Ogre::SceneManager* tmpSceneManager, GameMap* gm) :
    mSwitchedPM(false),
    mCircleMode(false),
    mCatmullSplineMode(false),
    mFirstIter(true),
    mRadius(0.0),
    mCenterX(0),
    mCenterY(0),
    mAlpha(0.0),
    mModeManager(NULL),
    mCullingManager(NULL),
    mActiveCamera(NULL),
    mActiveCameraNode(NULL),
    mGameMap(gm),
    mCameraIsFlying(false),
    mMoveSpeed(2.0),
    //NOTE: when changing, also change it in the terminal command 'movespeed'.
    mMoveSpeedAccel(static_cast<Ogre::Real>(2.0f) * mMoveSpeed),
    mCameraFlightSpeed(70.0),
    mRotateSpeed(90),
    mSwivelDegrees(0.0),
    mTranslateVector(Ogre::Vector3(0.0, 0.0, 0.0)),
    mTranslateVectorAccel(Ogre::Vector3(0.0, 0.0, 0.0)),
    mCameraFlightDestination(Ogre::Vector3(0.0, 0.0, 0.0)),
    mRotateLocalVector(Ogre::Vector3(0.0, 0.0, 0.0)),
    mSceneManager(tmpSceneManager),
    mViewport(NULL),
    mZChange(0.0),
    mZoomSpeed(7.0)
{
    mCullingManager = new CullingManager(this);

    gm->setCullingManger(mCullingManager);

    createViewport();
    createCamera("RTS", 0.02, 300.0);
    createCameraNode("RTS", Ogre::Vector3((Ogre::Real)10.0,
                                              (Ogre::Real)10.0,
                                              MAX_CAMERA_Z / 2.0),
                                              Ogre::Degree(0.0), Ogre::Degree(30.0));

    createCamera("FPP", 0.02, 30.0);
    createCameraNode("FPP", Ogre::Vector3(), Ogre::Degree(0), Ogre::Degree(75), Ogre::Degree(0));

    setActiveCamera("RTS");
    setActiveCameraNode("RTS");

    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Created camera manager");
}

CameraManager::~CameraManager()
{
    delete mCullingManager;
}

//! \brief Sets up the main camera
void CameraManager::createCamera(const Ogre::String& ss, double nearClip, double farClip)
{
    Ogre::Camera* tmpCamera = mSceneManager->createCamera(ss);
    tmpCamera->setNearClipDistance((Ogre::Real)nearClip);
    tmpCamera->setFarClipDistance((Ogre::Real)farClip);
    tmpCamera->setAutoTracking(false, mSceneManager->getRootSceneNode()
                                ->createChildSceneNode("CameraTarget_" + ss),
                                    Ogre::Vector3((Ogre::Real)(mGameMap->getMapSizeX() / 2),
                                                  (Ogre::Real)(mGameMap->getMapSizeY() / 2),
                                                  (Ogre::Real)0));

    mRegisteredCameraNames.insert(ss);
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating " + ss + " camera...", Ogre::LML_NORMAL);
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

    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating " + ss + "_node camera node...", Ogre::LML_NORMAL);
}


//! \brief setup the viewports
void CameraManager::createViewport()
{
    mViewport = ODApplication::getSingleton().getWindow()->addViewport(NULL);
    mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

// TODO: Update registered camera if needed.
//    for(set<string>::iterator m_itr = mRegisteredCameraNames.begin(); m_itr != mRegisteredCameraNames.end() ; ++m_itr){
//        Ogre::Camera* tmpCamera = getCamera(*m_itr);
//    }
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating viewport...", Ogre::LML_NORMAL);
}

void CameraManager::setFPPCamera(Creature* cc)
{
    Ogre::SceneNode* tmpNode = mSceneManager->getSceneNode("FPP_node");
    tmpNode->getParentSceneNode()->removeChild(tmpNode);
    tmpNode->setInheritOrientation(true);
    cc->mSceneNode->addChild(tmpNode);
}

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

Ogre::Viewport* CameraManager::getViewport()
{
    return mViewport;
}

Ogre::String CameraManager::switchPolygonMode()
{
    Ogre::String newVal;
    Ogre::PolygonMode pm;

    switch (getActiveCamera()->getPolygonMode())
    {
        case Ogre::PM_SOLID:
            newVal = "Wireframe";
            pm = Ogre::PM_WIREFRAME;
            break;
        case Ogre::PM_WIREFRAME:
            newVal = "Points";
            pm = Ogre::PM_POINTS;
            break;
        default:
            newVal = "Solid";
            pm = Ogre::PM_SOLID;
    }

    getActiveCamera()->setPolygonMode(pm);

    return newVal;
}

/*! \brief Sets the camera to a new location while still satisfying the
 * constraints placed on its movement
 */
void CameraManager::moveCamera(const Ogre::Real frameTime)
{
    // Carry out the acceleration/deceleration calculations on the camera translation.
    Ogre::Real speed = mTranslateVector.normalise();
    mTranslateVector *= (Ogre::Real)std::max(0.0, speed - (0.75 + (speed / mMoveSpeed))
                                * mMoveSpeedAccel * frameTime);
    mTranslateVector += mTranslateVectorAccel * (Ogre::Real)(frameTime * 2.0);

    // If we have sped up to more than the maximum moveSpeed then rescale the
    // vector to that length. We use the squaredLength() in this calculation
    // since squaring the RHS is faster than sqrt'ing the LHS.

    if (mTranslateVector.squaredLength() > mMoveSpeed * mMoveSpeed)
    {
        speed = mTranslateVector.length();
        mTranslateVector *= mMoveSpeed / speed;
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
    newPosition.z += (Ogre::Real)(mZChange * frameTime * mZoomSpeed);

    Ogre::Real horizontalSpeedFactor =
        (newPosition.z >= 16.0) ? (Ogre::Real)1.0 : (Ogre::Real)(newPosition.z / (16.0));

    newPosition += horizontalSpeedFactor * (viewDirectionQuaternion * mTranslateVector);

    // Prevent camera from moving down into the tiles or too high.
    if (newPosition.z <= MIN_CAMERA_Z)
        newPosition.z = MIN_CAMERA_Z;
    else if (newPosition.z >= MAX_CAMERA_Z)
        newPosition.z = MAX_CAMERA_Z;

    // Prevent the tilting to show a reversed world or looking too high.
    if (mRotateLocalVector.x != 0)
    {
        // NOTE : The camera loses the desired sense of left, right, top, down
        // when the camera pitch is more than 90 degrees.
        Ogre::Real pitch = getActiveCameraNode()->getOrientation().getPitch().valueRadians();
        if ((pitch >= 0.5 && mRotateLocalVector.x < 0)
            || (pitch <= 0.8 && mRotateLocalVector.x > 0))
        {
            // Tilt the camera up or down.
            getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_X,
                                          Ogre::Degree(mRotateLocalVector.x * frameTime),
                                          Ogre::Node::TS_LOCAL);
        }
    }

    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_Y,
                                  Ogre::Degree(mRotateLocalVector.y * frameTime),
                                  Ogre::Node::TS_LOCAL);

    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_Z,
                                  Ogre::Degree(mRotateLocalVector.z * frameTime),
                                  Ogre::Node::TS_LOCAL);

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
            flightDirection *= std::min(mCameraFlightSpeed * frameTime, radius);
            newPosition += flightDirection;
        }
    }


    if(mCircleMode)
    {
        mAlpha += 0.1 * frameTime;

        newPosition.x = (Ogre::Real)(cos(mAlpha) * mRadius + mCenterX);
        newPosition.y = (Ogre::Real)(sin(mAlpha) * mRadius + mCenterY);

        if(mAlpha > 2.0 * 3.145)
            mCircleMode = false;
    }
    else if(mCatmullSplineMode)
    {
        //cerr << "catmullSplineMode " << endl;
        mAlpha += 0.1 * frameTime;

        //cerr << "alpha "<< mAlpha << endl;

        double tempX, tempY;

        tempX = xHCS.evaluate(mAlpha);
        //cerr<< "newPosition.x"<< newPosition.x << endl;
        tempY = yHCS.evaluate(mAlpha);
        //cerr<< "newPosition.y"<< newPosition.y << endl;

        if (tempX < 0.9 * mGameMap->getMapSizeX() &&
                tempX > 10 &&
                tempY < 0.9 * mGameMap->getMapSizeY() &&
                tempY > 10 ) {
            newPosition.x = (Ogre::Real)tempX;
            newPosition.y = (Ogre::Real)tempY;
        }

        if(mAlpha > xHCS.getNN())
            mCatmullSplineMode = false;
    }

    // Move the camera to the new location
    getActiveCameraNode()->setPosition(newPosition);

    SoundEffectsHelper::getSingleton().setListenerPosition(
        newPosition,  getActiveCameraNode()->getOrientation());

    mGameMap->getMiniMap()->updateCameraInfos(getCameraViewTarget(), getActiveCameraNode()->getOrientation().getRoll().valueRadians());
    mModeManager->getCurrentMode()->mouseMoved(
        OIS::MouseEvent(0 ,mModeManager->getCurrentMode()->getMouse()->getMouseState()));

    //std::cerr << " x:" <<  getActiveCameraNode()->getPosition().x << " y " <<  getActiveCameraNode()->getPosition().y <<" z" <<  getActiveCameraNode()->getPosition().z << std::endl;
}

/*! \brief Computes a vector whose z-component is 0 and whose x-y coordinates
 * are the position on the floor that the camera is pointed at.
 */
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
    mCameraFlightDestination.z = 0.0;
}

void CameraManager::onMiniMapClick(Ogre::Vector2 cc)
{
    flyTo(Ogre::Vector3(cc.x, cc.y, (Ogre::Real)0.0));
}

/*! \brief tells the camera to move/rotate (or stop) in a specific direction
 *
 *  The function combines start and stop in one go. Giving equal momentum in
 *  one direction means either moving there (if resting before) or stoping the
 *  movement (if moving in oppsite direction before)
 */
void CameraManager::move(const Direction direction, double aux)
{
    switch (direction)
    {
    case stopRight:
        if (mTranslateVectorAccel.x > 0)
            mTranslateVectorAccel.x = 0;
        break;

    case stopLeft:
        if (mTranslateVectorAccel.x < 0)
            mTranslateVectorAccel.x = 0;
        break;

    case stopBackward:
        if (mTranslateVectorAccel.y < 0)
            mTranslateVectorAccel.y = 0;
        break;

    case stopForward:
        if (mTranslateVectorAccel.y > 0)
            mTranslateVectorAccel.y = 0;
        break;

    case moveLeft:
        mTranslateVectorAccel.x -= mMoveSpeedAccel;
        break;

    case moveRight:
        mTranslateVectorAccel.x += mMoveSpeedAccel;
        break;

    case moveForward:
        mTranslateVectorAccel.y += mMoveSpeedAccel;
        break;

    case moveBackward:
        mTranslateVectorAccel.y -= mMoveSpeedAccel;
        break;

    case moveUp:
    case stopDown:
        mZChange += mMoveSpeed;
        break;

    case moveDown:
    case stopUp:
        mZChange -= mMoveSpeed;
        break;

    case rotateLeft:
    case stopRotRight:
        mSwivelDegrees += 1.3 * mRotateSpeed;
        break;

    case rotateRight:
    case stopRotLeft:
        mSwivelDegrees -= 1.3 * mRotateSpeed;
        break;

    case rotateUp:
    case stopRotDown:
        mRotateLocalVector.x += mRotateSpeed.valueDegrees();
        break;

    case rotateDown:
    case stopRotUp:
        mRotateLocalVector.x -= mRotateSpeed.valueDegrees();
        break;

    case randomRotateX:
        mSwivelDegrees = Ogre::Degree((Ogre::Real)(64 * aux));
        break;

    case zeroRandomRotateX:
        mSwivelDegrees = Ogre::Degree(0.0);
        break;

    case randomRotateY:
        mRotateLocalVector.x = (Ogre::Real)(64 * aux);
        break;

    case zeroRandomRotateY:
        mRotateLocalVector.x =  0.0;
        break;

    default:
        break;
    }

    // We inform about the Camera position's change  in terems of X Y  coordinates
}

/*! \brief By tracking the four camera corners uses the bashAndSplash function to
 *  show or hide the tiles according to camera view
 */
int CameraManager::updateCameraView()
{
    return 0;
}

//TODO: This check is not used currently, because there's a bug with the cam
//movement: The cam will "remember" the last state when stopping, this leads
//to strange behavior -> try to rewrite camera movement from scratch (prevent
//the lengthy calculations from being executed if there's no movement at all)
/*! \brief Checks if the camera is moving at all by evaluating all momentums
 */
bool CameraManager::isCamMovingAtAll() const
{
    return (mTranslateVectorAccel.x != 0 ||
            mTranslateVectorAccel.y != 0 ||
            mZChange != 0 ||
            mSwivelDegrees.valueDegrees() != 0 ||
            mRotateLocalVector.x != 0 ||
            mCameraIsFlying);
}

bool CameraManager::onFrameStarted()
{
    updateCameraView();
    mCullingManager->onFrameStarted();

    return true;
}

bool CameraManager::onFrameEnded()
{
    if(mSwitchedPM) {
        switchPolygonMode();
        mSwitchedPM = false;
    }

    mCullingManager->onFrameEnded();
    return true;
}
