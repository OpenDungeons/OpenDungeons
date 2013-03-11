#ifndef MISSILEOBJECT_H
#define MISSILEOBJECT_H

#include <deque>
#include <string>

#include <OgreVector3.h>
#include <semaphore.h>

#include "MovableGameEntity.h"

class GameMap;

class MissileObject: public MovableGameEntity
{
    public:
        MissileObject(const std::string& nMeshName, const Ogre::Vector3& nPosition, GameMap* gameMap);

        void setPosition(const Ogre::Vector3& v);

        std::string getOgreNamePrefix()
        {
            return "";
        }
        
        virtual bool doUpkeep();
        virtual void stopWalking();

        //TODO: implment these in a good way
        void recieveExp(double experience){}
        void takeDamage(double damage, Tile *tileTakingDamage) {}
        double getDefense() const {return 0.0;}
        double getHP(Tile *tile) {return 0;}
        std::vector<Tile*> getCoveredTiles() { return std::vector<Tile*>() ;}

        static sem_t missileObjectUniqueNumberLockSemaphore;

    private:
        std::deque<Ogre::Vector3> walkQueue;
        sem_t positionLockSemaphore;
};

#endif

