#ifndef ANIMATEDOBJECT_H
#define ANIMATEDOBJECT_H

#include <deque>
#include <iostream>
#include <semaphore.h>

#include <Ogre.h>

class Tile;

class AnimatedObject
{
	public:
		AnimatedObject();

		virtual void setPosition(double x, double y, double z);
		virtual void setPosition(Ogre::Vector3 v);
		virtual Ogre::Vector3 getPosition();

		void addDestination(double x, double y, double z = 0.0);
		bool setWalkPath(std::list<Tile*> path, unsigned int minDestinations, bool addFirstStop);
		void clearDestinations();
		void stopWalking();
		void faceToward(int x, int y);

		virtual double getMoveSpeed();
		virtual void setMoveSpeed(double s);

		virtual void setAnimationState(std::string s, bool loop = true);

		virtual double getAnimationSpeedFactor();
		virtual void setAnimationSpeedFactor(double f);

		std::deque<Ogre::Vector3> walkQueue;

		sem_t positionLockSemaphore;
		sem_t walkQueueLockSemaphore;
		bool walkQueueFirstEntryAdded;
		Ogre::Vector3 walkDirection;
		double shortDistance;

		Ogre::AnimationState *animationState;
		std::string destinationAnimationState;
		Ogre::SceneNode *sceneNode;

		virtual std::string getOgreNamePrefix() = 0;
		virtual std::string getName() = 0;

	protected:
		Ogre::Vector3 position;
		double moveSpeed;
		std::string prevAnimationState;
		bool prevAnimationStateLoop;
		double animationSpeedFactor;
		sem_t animationSpeedFactorLockSemaphore;
};

#endif

