#ifndef MISSILEOBJECT_H
#define MISSILEOBJECT_H

#include <deque>
#include <string>

#include <Ogre.h>
#include <semaphore.h>

#include "ActiveObject.h"

class MissileObject : public ActiveObject, public AnimatedObject
{
	public:
		MissileObject();
		MissileObject(std::string nMeshName, Ogre::Vector3 nPosition);
		void initialize();

		void setPosition(double x, double y, double z);
		void setPosition(Ogre::Vector3 v);
		Ogre::Vector3 getPosition();

		void createMesh();
		void destroyMesh();
		void deleteYourself();

		std::string meshName;
		string getOgreNamePrefix() {return "";}
		std::string getName() {return name;}
		std::string name;


	private:
		std::deque<Ogre::Vector3> walkQueue;
		Ogre::Vector3 position;
		sem_t positionLockSemaphore;
		bool meshesExist;
};

#endif

