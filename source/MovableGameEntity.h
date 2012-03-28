#ifndef MOVABLEGAMEENTITY_H
#define MOVABLEGAMEENTITY_H

#include <deque>
#include <list>
#include <vector>
#include <semaphore.h>

#include <OgreVector3.h>
#include <OgreAnimationState.h>
#include <OgreSceneNode.h>

#include "GameEntity.h"

class Tile;

class MovableGameEntity : public GameEntity
{
    public:
        MovableGameEntity();
        virtual ~MovableGameEntity() {};

        void addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z = 0.0);
        bool setWalkPath(std::list<Tile*> path, unsigned int minDestinations,
                bool addFirstStop);
        void clearDestinations();
        virtual void stopWalking();
        void faceToward(int x, int y);

        virtual double getMoveSpeed();
        virtual void setMoveSpeed(double s);

        virtual void setAnimationState(const std::string& s, bool loop = true);

        virtual double getAnimationSpeedFactor();
        virtual void setAnimationSpeedFactor(double f);

        std::deque<Ogre::Vector3> walkQueue;

        sem_t walkQueueLockSemaphore;
        bool walkQueueFirstEntryAdded;
        Ogre::Vector3 walkDirection;
        double shortDistance;

        Ogre::AnimationState *animationState;
        std::string destinationAnimationState;
        Ogre::SceneNode *sceneNode;

        virtual std::string getOgreNamePrefix() = 0;

    protected:
        double moveSpeed;
        std::string prevAnimationState;
        bool prevAnimationStateLoop;
        double animationSpeedFactor;
        sem_t animationSpeedFactorLockSemaphore;
};

#endif

