/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
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

    struct Vector3i{
	Vector3i(const Ogre::Vector3& OV){x = (1<<10) * OV.x ; y = (1<<10) * OV.y; z = (1<<10) *OV.z ; }
	Vector3i(){};

	int x ; int y ; int z;};


    CameraManager( Ogre::SceneManager*   , GameMap*);

    inline void setCircleCenter( int XX, int YY) {centerX = XX ; centerY = YY;} ;
    inline void setCircleRadious(unsigned int rr){ radious = rr;};
    inline void setCircleMode(bool mm){circleMode = mm ; alpha = 0;};
    inline void setCatmullSplineMode(bool mm){catmullSplineMode = mm;  alpha = 0; };
    inline bool switchPM() { switchedPM = true; return true; };


    void setModeManager(ModeManager* mm){modeManager = mm;};
    void setFPPCamera(Creature*);

    //get/set moveSpeed
    inline const Ogre::Real& getMoveSpeed() const {
        return moveSpeed;
	}
    inline void setMoveSpeed(const Ogre::Real& newMoveSpeed) {
        moveSpeed = newMoveSpeed;
	}


    //get/set moveSpeedAccel
    inline const Ogre::Real& getMoveSpeedAccel() const {
        return moveSpeedAccel;
	}
    inline void setMoveSpeedAccel(const Ogre::Real& newMoveSpeedAccel) {
        moveSpeed = newMoveSpeedAccel;
	}

    //get/set rotateSpeed
    inline const Ogre::Degree& getRotateSpeed() const {
        return rotateSpeed;
	}
    inline void setRotateSpeed(const Ogre::Degree& newRotateSpeed) {
        rotateSpeed = newRotateSpeed;
	}

    //get translateVectorAccel
    inline const Ogre::Vector3& getTranslateVectorAccel() const {
        return translateVectorAccel;
	}
    bool getIntersectionPoints(   );


    bool isCamMovingAtAll() const;

    int updateCameraView();


    bool onFrameStarted   ();
    bool onFrameEnded     ();


    void            moveCamera          (const Ogre::Real frameTime);
    const Ogre::Vector3   getCameraViewTarget ();
    void            onMiniMapClick(Ogre ::Vector2 cc);
    void            flyTo               (const Ogre::Vector3& destination);

    void        move        (const Direction direction, double aux = 0.0);
    inline void stopZooming () {
        zChange = 0;
	}


	void createCameraNode(std::string ss, Ogre::Vector3 = Ogre::Vector3(0,0,0),Ogre::Degree = Ogre::Degree(0), Ogre::Degree = Ogre::Degree (0), Ogre::Degree = Ogre::Degree (0) );
    Ogre::SceneNode* getActiveCameraNode();
    Ogre::SceneNode* setActiveCameraNode(std::string ss);

    void createCamera(std::string ss, double nearClip, double farClip);
    void setActiveCamera(std::string ss);

    inline Ogre::Camera* getActiveCamera(){
        return  mActiveCamera ;
	}
    Ogre::Camera* getCamera(std::string ss);

    Ogre::Viewport* getViewport();
    void createViewport();

    private:
    bool switchedPM ;
    std::string switchPolygonMode();
    std::set<string> registeredCameraNames;
    std::set<string> registeredCameraNodeNames;


    bool circleMode;
    bool catmullSplineMode;
    bool firstIter;

    double radious;
    int centerX;
    int centerY;
    double alpha;

    ModeManager* modeManager;
    AbstractApplicationMode* gameMode;



    void sort(Vector3i& p1 , Vector3i& p2, bool sortByX);

    // we use the above variables for the methods below

    int bashAndSplashTiles(int); // set the new tiles
    CameraManager(const CameraManager&);


    Ogre::Camera* mActiveCamera;
    Ogre::SceneNode* mActiveCameraNode;

    GameMap* gameMap;
    bool            cameraIsFlying;
    Ogre::Real      moveSpeed;
    Ogre::Real      moveSpeedAccel;
    Ogre::Real      cameraFlightSpeed;
    Ogre::Degree    rotateSpeed;
    Ogre::Degree    swivelDegrees;
    Ogre::Vector3   translateVector;
    Ogre::Vector3   translateVectorAccel;
    Ogre::Vector3   cameraFlightDestination;
    Ogre::Vector3   mRotateLocalVector;
    Ogre::SceneManager* mSceneManager;
    Ogre::Viewport* mViewport;
    double          zChange;
    float           mZoomSpeed;
};

#endif /* CAMERAMANAGER_H_ */
