/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
 *
 *
 */


//TODO : CAMERA LOOSE ITS PROPER SENSE OF LEFT, RIGHT, TOP, DOWN WHEN YOU DO MORE THAN QUATER BAREEL VIA CAMERA ROTATE, AKA PTICH > 90 DEG
#include "SoundEffectsHelper.h"
#include "CameraManager.h"
#include "Quadtree.h"
#include "ModeManager.h"
#include "HermiteCatmullSpline.h"
#include "ODApplication.h"
#include "Creature.h"

#include <OGRE/OgrePrerequisites.h>
#include <OGRE/OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>

#include <algorithm>


using  std::set; using  std::swap; using  std::max; using  std::min;
using  std::cerr; using std::endl;



CameraManager::CameraManager(Ogre::SceneManager* tmpSceneManager , GameMap* gm  ) :
    switchedPM(false),
    gameMap(gm),
    cameraIsFlying(false),
    moveSpeed(2.0),
    //NOTE: when changing, also change it in the terminal command 'movespeed'.
    moveSpeedAccel(static_cast<Ogre::Real>(2.0f) * moveSpeed),
    cameraFlightSpeed(70.0),
    rotateSpeed(90),
    swivelDegrees(0.0),
    translateVector(Ogre::Vector3(0.0, 0.0, 0.0)),
    translateVectorAccel(Ogre::Vector3(0.0, 0.0, 0.0)),
    mRotateLocalVector(Ogre::Vector3(0.0, 0.0, 0.0)),
    zChange(0.0),
    mZoomSpeed(7.0),
    circleMode(false),
    catmullSplineMode(false),
    firstIter(true),
    mSceneManager(tmpSceneManager),
    mViewport(0),
    mActiveCamera(0),
    mActiveCameraNode(0)
{


}

/*! \brief Sets up the main camera
 *
 */
void CameraManager::createCamera(std::string ss, double nearClip, double farClip)
{
    Ogre::Camera* tmpCamera = mSceneManager->createCamera(ss);
    tmpCamera->setNearClipDistance(nearClip);
    tmpCamera->setFarClipDistance(farClip);
    tmpCamera->setAutoTracking(false, mSceneManager->getRootSceneNode()
                                ->createChildSceneNode("CameraTarget_" + ss), Ogre::Vector3(gameMap->getMapSizeX()/2,gameMap->getMapSizeY()/2 , 0));

    registeredCameraNames.insert(ss);
}

void CameraManager::createCameraNode(std::string ss,Ogre::Vector3 xyz, Ogre::Degree yaw, Ogre::Degree pitch , Ogre::Degree roll ){
    Ogre::SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode(ss + "_node",xyz);
    Ogre::Camera* tmpCamera = getCamera(ss);
    // node->pitch(Ogre::Degree(25), Ogre::Node::TS_WORLD);
    node->yaw  (yaw ,Ogre::Node::TS_LOCAL);
    node->pitch(pitch, Ogre::Node::TS_LOCAL);
    node->roll (roll, Ogre::Node::TS_LOCAL);
    node->attachObject(tmpCamera);
    node->setPosition(Ogre::Vector3(0,0,2) +  node->getPosition());
    registeredCameraNodeNames.insert(ss);
}


/*! \brief setup the viewports
 *
 */
void CameraManager::createViewport()
{
    mViewport = ODApplication::getSingleton().getWindow()->addViewport(NULL);
    mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

    for(set<string>::iterator m_itr = registeredCameraNames.begin(); m_itr != registeredCameraNames.end() ; m_itr++){
	Ogre::Camera* tmpCamera = getCamera(*m_itr);

    }
}

void CameraManager::setFPPCamera(Creature* cc){
    Ogre::SceneNode* tmpNode = mSceneManager->getSceneNode("FPP_node");
    tmpNode->getParentSceneNode()->removeChild(tmpNode);
    tmpNode->setInheritOrientation(true);
    cc->sceneNode->addChild(tmpNode);
}

Ogre::SceneNode* CameraManager::getActiveCameraNode(){
    return getActiveCamera()->getParentSceneNode();
}

Ogre::SceneNode* CameraManager::setActiveCameraNode(Ogre::String ss){
    return mActiveCameraNode = mSceneManager->getSceneNode(ss + "_node");
}

Ogre::Camera* CameraManager::getCamera(Ogre::String ss){
    return mSceneManager->getCamera(ss);
}

void CameraManager::setActiveCamera(Ogre::String ss){
    mActiveCamera = mSceneManager->getCamera(ss);
    mViewport->setCamera(mActiveCamera);
    mActiveCamera->setAspectRatio(Ogre::Real(mViewport->getActualWidth()) / Ogre::Real(mViewport->getActualHeight()));
}



Ogre::Viewport* CameraManager::getViewport()
{
    return mViewport;
}

Ogre::String CameraManager::switchPolygonMode(){
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
    Ogre::Vector3 newPosition =  getActiveCameraNode()->getPosition();

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
    if (newPosition.z <= 0.5)
    {
        newPosition.z = 0.5;
    }

    // Tilt the camera up or down.
    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_X, Ogre::Degree(mRotateLocalVector.x
								      * frameTime), Ogre::Node::TS_LOCAL);

    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(mRotateLocalVector.y
								      * frameTime), Ogre::Node::TS_LOCAL);

    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_Z, Ogre::Degree(mRotateLocalVector.z
								      * frameTime), Ogre::Node::TS_LOCAL);

    move(zeroRandomRotateY);
    // Swivel the camera to the left or right, while maintaining the same
    // view target location on the ground.
    Ogre::Real deltaX = newPosition.x - viewTarget.x;

    Ogre::Real deltaY = newPosition.y - viewTarget.y;

    Ogre::Real radius = sqrt(deltaX * deltaX + deltaY * deltaY);

    Ogre::Real theta = atan2(deltaY, deltaX) + swivelDegrees.valueRadians() * frameTime;

    newPosition.x = viewTarget.x + radius * cos(theta);

    newPosition.y = viewTarget.y + radius * sin(theta);

    getActiveCameraNode()->rotate(Ogre::Vector3::UNIT_Z, Ogre::Degree(swivelDegrees * frameTime),
				  Ogre::Node::TS_WORLD);

    move(zeroRandomRotateX);
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


    if(circleMode){
	alpha += 0.1 * frameTime;

	newPosition.x = cos(alpha)*radious + centerX;
	newPosition.y = sin(alpha)*radious + centerY;

	if(alpha > 2* 3.145){
	    circleMode = false;

	}
    }


    else if(catmullSplineMode){
	cerr << "catmullSplineMode " << endl;
	alpha += 0.1 * frameTime;

	cerr << "alpha "<< alpha << endl;

	double tempX, tempY;

	tempX = xHCS.evaluate(alpha);
	cerr<< "newPosition.x"<< newPosition.x << endl;
	tempY = yHCS.evaluate(alpha);
	cerr<< "newPosition.y"<< newPosition.y << endl;

	if (tempX < 0.9*gameMap->getMapSizeX() &&  tempX > 10 && tempY < 0.9*gameMap->getMapSizeY() &&  tempY > 10 ) {
	    newPosition.x = tempX;
	    newPosition.y = tempY;

	}

	if(alpha > xHCS.getNN()){
	    catmullSplineMode = false;
	}

    }


    // Move the camera to the new location
    getActiveCameraNode()->setPosition(newPosition);

    SoundEffectsHelper::getSingleton().setListenerPosition(
        newPosition,  getActiveCameraNode()->getOrientation());



    gameMap->getMiniMap()->setCamera_2dPosition(getCameraViewTarget());
    modeManager->getCurrentMode()->mouseMoved(
        OIS::MouseEvent(0 ,modeManager->getCurrentMode()->getMouse()->getMouseState()));

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

/** \brief Starts the camera moving towards a destination position,
 *         it will stop moving when it gets there.
 */
void CameraManager::flyTo(const Ogre::Vector3& destination)
{
    cameraIsFlying = true;
    cameraFlightDestination = destination;
    cameraFlightDestination.z = 0.0;
}

void CameraManager::onMiniMapClick(Ogre::Vector2 cc){

    flyTo(Ogre::Vector3(cc.x, cc.y,  0.0));

}




/*! \brief tells the camera to move/rotate (or stop) in a specific direction
 *
 *  The function combines start and stop in one go. Giving equal momentum in
 *  one direction means either moving there (if resting before) or stoping the
 *  movement (if moving in oppsite direction before)
 */
void CameraManager::move(const Direction direction, double aux  )
{
    switch (direction)
    {

	case stopRight:

	    if (translateVectorAccel.x > 0)
		translateVectorAccel.x = 0;

	    break;

	case stopLeft:
	    if (translateVectorAccel.x < 0)
		translateVectorAccel.x = 0;

	    break;

	case stopBackward:
	    if (translateVectorAccel.y < 0)
		translateVectorAccel.y = 0;

	    break;

	case stopForward:
	    if (translateVectorAccel.y > 0)
		translateVectorAccel.y = 0;

	    break;

	case moveLeft:

	    translateVectorAccel.x -= moveSpeedAccel;

	    break;

	case moveRight:

	    translateVectorAccel.x += moveSpeedAccel;

	    break;

	case moveForward:

	    translateVectorAccel.y += moveSpeedAccel;

	    break;

	case moveBackward:

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

	case randomRotateX:
	    swivelDegrees = Ogre::Degree( 64 * aux);
	    break;


	case zeroRandomRotateX:
	    swivelDegrees = Ogre::Degree( 0.0);

	    break;

	case randomRotateY:
	    mRotateLocalVector.x =  64 * aux;
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
 *
 */
int CameraManager::updateCameraView() {
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
    return (translateVectorAccel.x != 0 ||
            translateVectorAccel.y != 0 ||
            zChange != 0 ||
            swivelDegrees.valueDegrees() != 0 ||
            mRotateLocalVector.x != 0 ||
            cameraIsFlying);
}

bool CameraManager::onFrameStarted (){

    updateCameraView();
    return true;
}

bool CameraManager::onFrameEnded   (){

    if(switchedPM) {
        switchPolygonMode();
        switchedPM = false;
    }
    return true;
}
