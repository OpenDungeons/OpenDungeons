/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
 */

#include "SoundEffectsHelper.h"
#include "InputManager.h"

#include "CameraManager.h"

template<> CameraManager* Ogre::Singleton<CameraManager>::ms_Singleton = 0;

/*! \brief Returns access to the singleton instance of InputManager
 */
CameraManager& CameraManager::getSingleton()
{
    assert(ms_Singleton);
    return(*ms_Singleton);
}

/*! \brief Returns access to the pointer to the singleton instance of Gui
 */
CameraManager* CameraManager::getSingletonPtr()
{
    return ms_Singleton;
}

CameraManager::CameraManager(Ogre::Camera* cam) :
        mCamera(cam),
        mCamNode(cam->getParentSceneNode()),
        cameraIsFlying(false),
        moveSpeed(2.0),
        //NOTE: when changing, also change it in the terminal command 'movespeed'.
        moveSpeedAccel(static_cast<Ogre::Real> (2.0f) * moveSpeed),
        rotateSpeed(90),
        swivelDegrees(0.0),
        cameraFlightSpeed(70.0),
        translateVector(Ogre::Vector3(0.0, 0.0, 0.0)),
        translateVectorAccel(Ogre::Vector3(0.0, 0.0, 0.0)),
        mRotateLocalVector(Ogre::Vector3(0.0, 0.0, 0.0)),
        zChange(0.0),
        mZoomSpeed(7.0)
{
}

/*! \brief Sets the camera to a new location while still satisfying the
 * constraints placed on its movement
 */
void CameraManager::moveCamera(Ogre::Real frameTime)
{
    //before doing all the lengthy calculations we check if it is really needed
    if(checkIfCamMovesAtAll())
    {
        // Carry out the acceleration/deceleration calculations on the camera translation.
        Ogre::Real speed = translateVector.normalise();
        translateVector *= std::max(0.0, speed - (0.75 + (speed / moveSpeed))
                * moveSpeedAccel * frameTime);
        translateVector += translateVectorAccel * (frameTime * 2.0);

        // If we have sped up to more than the maximum moveSpeed then rescale the
        // vector to that length. We use the squaredLength() in this calculation
        // since squaring the RHS is faster than sqrt'ing the LHS.
        if (translateVector.squaredLength() > moveSpeed * moveSpeed)
        {
            speed = translateVector.length();
            translateVector *= moveSpeed / speed;
        }

        // Get the camera's current position.
        Ogre::Vector3 newPosition = mCamNode->getPosition();

        // Get a quaternion which will rotate the "camera relative" x-y values
        // for the translateVector into the global x-y used to position the camera.
        Ogre::Vector3 viewTarget = getCameraViewTarget();
        Ogre::Vector3 viewDirection = viewTarget - newPosition;
        viewDirection.z = 0.0;
        Ogre::Quaternion viewDirectionQuaternion = Ogre::Vector3::UNIT_Y.getRotationTo(
                viewDirection);

        // Adjust the newPosition vector to account for the translation due
        // to the movement keys on the keyboard (the arrow keys and/or WASD).
        newPosition.z += zChange * frameTime * mZoomSpeed;
        Ogre::Real horizontalSpeedFactor = (newPosition.z >= 25.0)
                ? 1.0
                : newPosition.z / (25.0);
        newPosition += horizontalSpeedFactor * (viewDirectionQuaternion * translateVector);

        // Prevent camera from moving down into the tiles.
        if (newPosition.z <= 4.5)
        {
            newPosition.z = 4.5;
        }

        // Tilt the camera up or down.
        mCamNode->rotate(Ogre::Vector3::UNIT_X, Ogre::Degree(mRotateLocalVector.x
                * frameTime), Ogre::Node::TS_LOCAL);
        mCamNode->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(mRotateLocalVector.y
                * frameTime), Ogre::Node::TS_LOCAL);
        mCamNode->rotate(Ogre::Vector3::UNIT_Z, Ogre::Degree(mRotateLocalVector.z
                * frameTime), Ogre::Node::TS_LOCAL);

        // Swivel the camera to the left or right, while maintaining the same
        // view target location on the ground.
        Ogre::Real deltaX = newPosition.x - viewTarget.x;
        Ogre::Real deltaY = newPosition.y - viewTarget.y;
        Ogre::Real radius = sqrt(deltaX * deltaX + deltaY * deltaY);
        Ogre::Real theta = atan2(deltaY, deltaX) + swivelDegrees.valueRadians() * frameTime;
        newPosition.x = viewTarget.x + radius * cos(theta);
        newPosition.y = viewTarget.y + radius * sin(theta);
        mCamNode->rotate(Ogre::Vector3::UNIT_Z, Ogre::Degree(swivelDegrees * frameTime),
                Ogre::Node::TS_WORLD);

        // If the camera is trying to fly toward a destination, move it in that direction.
        if (cameraIsFlying)
        {
            // Compute the direction and distance the camera needs to move
            //to get to its intended destination.
            Ogre::Vector3 flightDirection = cameraFlightDestination - viewTarget;
            radius = flightDirection.normalise();

            // If we are withing the stopping distance of the target, then quit flying.
            // Otherwise we move towards the destination.
            if (radius <= 0.25)
            {
                // We are withing the stopping distance of the target destination
                // so stop flying towards it.
                cameraIsFlying = false;
            }
            else
            {
                // Scale the flight direction to move towards at the given speed
                // (the min function prevents overshooting the target) then add
                // this offset vector to the camera position.
                flightDirection *= std::min(cameraFlightSpeed * frameTime, radius);
                newPosition += flightDirection;
            }
        }

        // Move the camera to the new location
        mCamNode->setPosition(newPosition);
        SoundEffectsHelper::getSingleton().setListenerPosition(
                newPosition, mCamNode->getOrientation());
        InputManager::getSingleton().mouseMoved(
                OIS::MouseEvent(0, InputManager::getSingleton().getMouse()->getMouseState()));
    }
}

/*! \brief Computes a vector whose z-component is 0 and whose x-y coordinates
 * are the position on the floor that the camera is pointed at.
 */
Ogre::Vector3 CameraManager::getCameraViewTarget()
{
    // Get the position of the camera and direction that the camera is facing.
    Ogre::Vector3 position = mCamera->getRealPosition();
    Ogre::Vector3 viewDirection = mCamera->getDerivedDirection();

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

/** \brief Starts the camera moving towards a destination position,
 *         it will stop moving when it gets there.
 */
void CameraManager::flyTo(Ogre::Vector3 destination)
{
    cameraIsFlying = true;
    cameraFlightDestination = destination;
    cameraFlightDestination.z = 0.0;
}

/*! \brief tells the camera to move/rotate (or stop) in a specific direction
 *
 *  The function combines start and stop in one go. Giving equal momentum in
 *  one direction means either moving there (if resting before) or stoping the
 *  movement (if moving in oppsite direction before)
 */
void CameraManager::move(Direction direction)
{
    switch(direction)
    {
        case moveLeft:
        case stopRight:
            translateVectorAccel.x -= moveSpeedAccel;
            break;

        case moveRight:
        case stopLeft:
            translateVectorAccel.x += moveSpeedAccel;
            break;

        case moveForward:
        case stopBackward:
            translateVectorAccel.y += moveSpeedAccel;
            break;

        case moveBackward:
        case stopForward:
            translateVectorAccel.y -= moveSpeedAccel;
            break;

        case moveUp:
        case stopDown:
            zChange += moveSpeed;
            break;

        case moveDown:
        case stopUp:
            zChange -= moveSpeed;
            break;

        case rotateLeft:
        case stopRotRight:
            swivelDegrees += 1.3 * rotateSpeed;
            break;

        case rotateRight:
        case stopRotLeft:
            swivelDegrees -= 1.3 * rotateSpeed;
            break;

        case rotateUp:
        case stopRotDown:
            mRotateLocalVector.x += rotateSpeed.valueDegrees();
            break;

        case rotateDown:
        case stopRotUp:
            mRotateLocalVector.x -= rotateSpeed.valueDegrees();
            break;

        default:
            break;
    }
}

/*! \brief Checks if the camera is moving at all by evaluating all momentums
 *
 *  Sets the variable that is accessible through a public getter method to the
 *  result but also returns the result in one go
 */
bool CameraManager::checkIfCamMovesAtAll()
{
    return (cameraIsMovingAtAll = (
            translateVectorAccel.x != 0 ||
            translateVectorAccel.y != 0 ||
            zChange != 0 ||
            swivelDegrees.valueDegrees() != 0 ||
            mRotateLocalVector.x != 0 ||
            cameraIsFlying
            ));
}
