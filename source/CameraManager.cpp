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
#include "InputManager.h"
#include "CameraManager.h"
#include <OGRE/OgreSceneNode.h>


using namespace std;

template<> CameraManager* Ogre::Singleton<CameraManager>::ms_Singleton = 0;

CameraManager::CameraManager(Ogre::Camera* cam, GameMap* gm ) :
        mCamera(cam),
        gameMap(gm),
        mCamNode(cam->getParentSceneNode()),
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
        oldTop(0),
        oldBottom(0),
        oldMiddleLeft(0),
        oldMiddleRight(0),
        top(0),
        bottom(0),
        middleLeft(0),
        middleRight(0),
	precisionDigits(10),
        worldCoordinatesVector(Ogre::Vector3((double)GameMap::mapSizeX/2,(double)GameMap::mapSizeX/2,0 ))
{
  
  //mCamNode->setPosition(Ogre::Vector3(0,0,200)+ mCamNode->getPosition());
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

    move(zeroRandomRotateY);
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

    // Move the camera to the new location
    mCamNode->setPosition(newPosition);

    SoundEffectsHelper::getSingleton().setListenerPosition(
        newPosition, mCamNode->getOrientation());

    
    updateCameraView();
    gameMap->getMiniMap()->setCamera_2dPosition(getCameraViewTarget());
    gameMode->mouseMoved(
        OIS::MouseEvent(0 ,gameMode->getMouse()->getMouseState()));

    //std::cerr << " x:" << mCamNode->getPosition().x << " y " << mCamNode->getPosition().y <<" z" << mCamNode->getPosition().z << std::endl;
}

/*! \brief Computes a vector whose z-component is 0 and whose x-y coordinates
 * are the position on the floor that the camera is pointed at.
 */
const Ogre::Vector3 CameraManager::getCameraViewTarget()
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

    delete oldTop;
    delete oldBottom;
    delete oldMiddleLeft;
    delete oldMiddleRight;

    oldTop=top ;
    oldBottom=bottom ;
    oldMiddleLeft=middleLeft ;
    oldMiddleRight=middleRight;

    getIntersectionPoints(top, bottom, middleLeft, middleRight);



    // *top=*top +  worldCoordinatesVector;
    // *bottom=*bottom + worldCoordinatesVector;
    // *middleLeft=*middleLeft + worldCoordinatesVector;
    // *middleRight=*middleRight + worldCoordinatesVector;



    // sort the new tiles to form the proper diamod

    sort(bottom, top, false);
    sort(middleLeft, middleRight, false);
    sort(middleRight, top, false);
    sort(bottom, middleLeft, false);
    sort(middleLeft, middleRight, true);


    if(oldTop!=0){
	bashAndSplashTiles(SHOW | HIDE);
    }
    else{





	oldTop=new  Vector3i (*top) ;
	oldBottom=new  Vector3i (*bottom) ;
	oldMiddleLeft=new  Vector3i (*middleLeft) ;
	oldMiddleRight=new  Vector3i (*middleRight);

	bashAndSplashTiles(SHOW);
    }
    return 1;
}



/*! \brief Auxilary function, according to mode flags : SHOW and HIDE will try to show or hide a tile in single pass
 *  In each call there are two processes : one which traces the old camera view ( the one which would hide old Tiles ) , and one 
 *  which traces the new camera view ( the one which would show new Tiles). Mode parameter allows to only activate one of those processes, or activate both or none :)
 *  TODO : IMPLEMENT THE FLOOR AND CEIL MATH FUNCTIONS FOR FRACTURE VALUES, SO THAT THE EDGEING TILES ( IN CAMERA VIEW ) ARE NOT CULLED AWAY 
 */


int CameraManager::bashAndSplashTiles(int mode)
{
    bool bash,splash;

    int xxLeftOld = oldTop->x;
    int xxRightOld= oldTop->x;

    int dxLeftOld1 = (int)(oldMiddleLeft->x - oldTop->x)* (1<<precisionDigits) / (int)(oldTop->y - oldMiddleLeft->y);
    int dxRightOld1 = (int)(oldMiddleRight->x - oldTop->x)* (1<<precisionDigits) / (int)(oldTop->y - oldMiddleRight->y) ;

    int dxLeftOld2 = (int)(oldBottom->x - oldMiddleLeft->x)* (1<<precisionDigits)/ (int)(oldMiddleLeft->y - oldBottom->y );
    int dxRightOld2 =(int)(oldBottom->x - oldMiddleRight->x)* (1<<precisionDigits)/ (int)(oldMiddleRight->y - oldBottom->y) ;


    int xxLeft = top->x;
    int xxRight= top->x;

    int  dxLeft1 = (int)(middleLeft->x - top->x) * (1<<precisionDigits) / (int)(top->y - middleLeft->y);
    int  dxRight1 = (int)(middleRight->x - top->x)* (1<<precisionDigits) / (int)(top->y - middleRight->y) ;

    int  dxLeft2 = (int)(bottom->x - middleLeft->x)* (1<<precisionDigits)/ (int)(middleLeft->y - bottom->y );
    int  dxRight2 =(int)(bottom->x - middleRight->x)* (1<<precisionDigits)/ (int)(middleRight->y - bottom->y) ;


      

    int bb = min(bottom->y,oldBottom->y);

       
    for (int yy =  ((max(top->y,oldTop->y)>>precisionDigits)+1 )<<precisionDigits ; yy >= bb ; yy-=(1<<precisionDigits)) {





	// if(yy == top->y)splashY=!splashY;

	// if(yy == oldTop->y)bashY=!bashY;




	    if ( yy > middleLeft->y && yy<=top->y) {
	      xxLeft += dxLeft1;
	    }
	    else if( yy <= middleLeft->y && yy>bottom->y){
	      xxLeft += dxLeft2;

	    }
	    if ( yy > middleRight->y && yy<=top->y ) {
	      xxRight += dxRight1;
	    }
	    else if( yy <=  middleRight->y && yy>bottom->y ){
	      xxRight += dxRight2;

	    }


	    if ( yy > oldMiddleLeft->y && yy<=oldTop->y) {
		xxLeftOld += dxLeftOld1;
	    }
	    else if( yy <=  oldMiddleLeft->y && yy>oldBottom->y) {
		xxLeftOld += dxLeftOld2;

	    }
	    if ( yy > oldMiddleRight->y && yy<=oldTop->y) {
		xxRightOld += dxRightOld1;
	    }
	    else if( yy <=  oldMiddleRight->y && yy>oldBottom->y){
		xxRightOld += dxRightOld2;
	    }

	int rr =  max(xxRight,xxRightOld);
	for (int xx =  ((min(xxLeft,xxLeftOld)>>precisionDigits) -1)<<precisionDigits  ; xx <= rr ; xx+= (1<<precisionDigits)) {


	    // if(xx <=(int)xxLeft ) splashX=!splashX;
	    // if(xx <=(int)xxLeftOld  && xx >(int)xxRightOld) ) bashX=true;

    

	    splash = (xx >=(int)xxLeft  && xx <=(int)xxRight && (yy >= (int)bottom->y) && yy <= (int)top->y)  ;
	   
	    bash   = (xx >=(int)xxLeftOld  && xx <=(int)xxRightOld && (yy >= (int)oldBottom->y) && yy <= (int)oldTop->y) ;
	    
	    // cerr<< " x" <<  xx  << " y" << yy << " " <<bash<<splash << endl;		    
	      if(bash && splash && (mode & HIDE) && (mode & SHOW) ){}
	      else if (bash && (mode & HIDE))	      gameMap->getTile(xx>>precisionDigits,yy>>precisionDigits)->hide();
	      else if (splash && (mode & SHOW))	      gameMap->getTile(xx>>precisionDigits,yy>>precisionDigits)->show();




	    // if(xx >(int)xxRightOld) bashX=!bashX;
	    // if(xx >(int)xxRight)splashX=!splashX;
	}

	// if(yy < (int)oldBottom->y)bashY=!bashY;
	// if(yy < (int)bottom->y)splashY=!splashY;
    }      
    return 1;        
}

bool CameraManager::getIntersectionPoints( Vector3i*& pp1,Vector3i*& pp2, Vector3i*& pp3, Vector3i*& pp0   ) {

    const Ogre::Vector3*  myvector = getCamera()->getWorldSpaceCorners();

    Ogre::Plane** myplanes =  new  Ogre::Plane*[6];
    Ogre::Ray** myRay = new Ogre::Ray*[4];
    Vector3i** vi = new Vector3i*[4];

    
    myplanes[0]=new Ogre::Plane(0,0,1,0);
    myplanes[1]=new Ogre::Plane(0,0,-1,20);
    myplanes[2]=new Ogre::Plane(0,1,0,-1);
    myplanes[3]=new Ogre::Plane(0,-1,0,395);
    myplanes[4]=new Ogre::Plane(1,0,0,-1);
    myplanes[5]=new Ogre::Plane(-1,0,0,395);



    myRay[0]= new  Ogre::Ray (myvector[0],  myvector[4] - myvector[0] );
    myRay[1]= new  Ogre::Ray (myvector[1],  myvector[5] - myvector[1] );
    myRay[2]= new  Ogre::Ray (myvector[2],  myvector[6] - myvector[2] );
    myRay[3]= new  Ogre::Ray (myvector[3],  myvector[7] - myvector[3] );

    std::pair<bool, Ogre::Real> intersectionResult;
    Ogre::Vector3 pp ;

    //  for(int ii = 0 ; ii <4 ; ii++){
    //  	     intersectionResult =  myRay[ii]->intersects(*myplanes[0]) ;
    //  	     rr++;
    //  	     if(intersectionResult.first)
    //  		 pp= myRay[ii]->getPoint(intersectionResult.second);
    // }



    for(int ii = 0 ; ii <4 ; ii++){
	int rr = 0;
	do
	    {
		intersectionResult =  myRay[ii]->intersects(*myplanes[rr]) ;
		rr++;
		if(intersectionResult.first)
		    pp= myRay[ii]->getPoint(intersectionResult.second);


	    }
	while(!( (intersectionResult.first && pp.x >= 0.0 && pp.x <=396.0 && pp.y >= 0.0 && pp.y <=396.0    ) )   && rr < 6);
	if(intersectionResult.first){
	    vi[ii]=new   Vector3i (pp);

	}
	else{
	    cerr<< "I didn't found the intersection point for " << ii <<"th ray"<<endl;
	    vi[ii]=NULL;
	    exit(1);
	}
    }

    pp0 =  vi[0];
    pp1 =  vi[1]; 
    pp2 =  vi[2]; 
    pp3 =  vi[3]; 

    cerr<<endl << "intersection points" << endl;
    cerr << pp1->x << " " << pp1->y << " " << pp1->z <<endl;
    cerr << pp2->x << " " << pp2->y << " " << pp2->z <<endl;
    cerr << pp3->x << " " << pp3->y << " " << pp3->z <<endl;
    cerr << pp0->x << " " << pp0->y << " " << pp0->z <<endl;

    return true;
}


/*! \brief Sort two Vector3i p1 and p2  to satisfy p1 <= p2 according to  
 * the value of X or Y coordiante, which depends on sortByX param . 
 */
void CameraManager::sort(Vector3i*& p1 , Vector3i*& p2, bool sortByX) {

    if (sortByX) {


        if (p1->x > p2->x) {
	  std::swap(p1,p2);
        }

    }


    else {

        if (p1->y > p2->y) {
	  std::swap(p1,p2);
        }


    }
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
            cameraIsFlying
           );
}

