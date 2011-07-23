/*!
 * \file   CameraManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  Handles the camera movements
 */

#ifndef CAMERAMANAGER_H_
#define CAMERAMANAGER_H_

class CameraManager : public Ogre::Singleton<CameraManager>
{
    public:
        enum Direction
        {
            moveLeft, moveRight, moveForward, moveBackward, moveUp, moveDown,
            rotateLeft, rotateRight, rotateUp, rotateDown,

            stopLeft, stopRight, stopForward, stopBackward, stopUp, stopDown,
            stopRotLeft, stopRotRight, stopRotUp, stopRotDown,

            fullStop
        };

        CameraManager(Ogre::Camera* cam);

        //get/set moveSpeed
        inline const Ogre::Real& getMoveSpeed() const{return moveSpeed;}
        inline void setMoveSpeed(const Ogre::Real& newMoveSpeed){moveSpeed = newMoveSpeed;}

        //get/set moveSpeedAccel
        inline const Ogre::Real& getMoveSpeedAccel() const{return moveSpeedAccel;}
        inline void setMoveSpeedAccel(const Ogre::Real& newMoveSpeedAccel){moveSpeed = newMoveSpeedAccel;}

        //get/set rotateSpeed
        inline const Ogre::Degree& getRotateSpeed() const{return rotateSpeed;}
        inline void setRotateSpeed(const Ogre::Degree& newRotateSpeed){rotateSpeed = newRotateSpeed;}

        //get translateVectorAccel
        inline const Ogre::Vector3& getTranslateVectorAccel() const{return translateVectorAccel;}

        //get camera
        inline Ogre::Camera* getCamera() const{return mCamera;}

        bool isCamMovingAtAll();

        void moveCamera(Ogre::Real frameTime);
        Ogre::Vector3 getCameraViewTarget();
        void flyTo(Ogre::Vector3 destination);

        void move(Direction direction);
        inline void stopZooming() {zChange = 0;}

    private:
        CameraManager(const CameraManager&);

        Ogre::Camera* mCamera;
        Ogre::SceneNode* mCamNode;

        bool cameraIsFlying;
        Ogre::Real moveSpeed;
        Ogre::Real moveSpeedAccel;
        Ogre::Real cameraFlightSpeed;
        Ogre::Degree rotateSpeed;
        Ogre::Degree swivelDegrees;
        Ogre::Vector3 translateVector;
        Ogre::Vector3 translateVectorAccel;
        Ogre::Vector3 cameraFlightDestination;
        Ogre::Vector3 mRotateLocalVector;
        double zChange;
        float mZoomSpeed;
};

#endif /* CAMERAMANAGER_H_ */
