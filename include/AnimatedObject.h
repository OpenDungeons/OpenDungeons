#ifndef ANIMATEDOBJECT_H
#define ANIMATEDOBJECT_H

#include <deque>

#include <Ogre.h>

#include "Tile.h"

class AnimatedObject
{
	public:
		AnimatedObject();

		void addDestination(int x, int y);
		bool setWalkPath(std::list<Tile*> path, unsigned int minDestinations, bool addFirstStop);
		void clearDestinations();
		void stopWalking();
		void faceToward(int x, int y);

		std::deque<Ogre::Vector3> walkQueue;
		string name;			// The creature's unique name

		sem_t positionLockSemaphore;
		sem_t walkQueueLockSemaphore;
		bool walkQueueFirstEntryAdded;
		Ogre::Vector3 walkDirection;
		double shortDistance;

	protected:
		Ogre::Vector3 position;
};

#endif

