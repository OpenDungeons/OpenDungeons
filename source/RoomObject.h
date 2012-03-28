#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include <string>
#include <istream>
#include <ostream>

#include "MovableGameEntity.h"

class Room;
class GameMap;

class RoomObject: public MovableGameEntity
{
    public:
        RoomObject(Room* nParentRoom, const std::string& nMeshName);

        Room* getParentRoom();

        //TODO: implment these in a good way
        bool doUpkeep() {return true;}
        void recieveExp(double experience){}
        void takeDamage(double damage, Tile *tileTakingDamage) {}
        double getDefense() const {return 0.0;}
        double getHP(Tile *tile) {return 0;}
        std::vector<Tile*> getCoveredTiles() { return std::vector<Tile*>() ;}

        std::string getOgreNamePrefix();

        static const char* getFormat();
        friend std::ostream& operator<<(std::ostream& os, RoomObject *o);
        friend std::istream& operator>>(std::istream& is, RoomObject *o);

		Ogre::Real x, y;
        Ogre::Real rotationAngle;

    private:
        Room *parentRoom;
};

#endif

