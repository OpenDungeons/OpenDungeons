#ifndef MISSILEOBJECT_H
#define MISSILEOBJECT_H

#include <deque>
#include <string>

#include <Ogre.h>
#include <semaphore.h>

#include "MovableGameEntity.h"

class GameMap;

class MissileObject: public MovableGameEntity
{
    public:
        MissileObject(const std::string& nMeshName, const Ogre::Vector3& nPosition, GameMap& gameMap);

        void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);
        void setPosition(const Ogre::Vector3& v);
        Ogre::Vector3 getPosition();

        void createMesh();
        void destroyMesh();
        void deleteYourself();

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
        GameMap& gameMap;
        std::deque<Ogre::Vector3> walkQueue;
        Ogre::Vector3 position;
        sem_t positionLockSemaphore;
};

#endif

