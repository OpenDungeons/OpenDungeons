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

        void createMesh();
        void destroyMesh();
        void deleteYourself();

        bool doUpkeep() {return true;}

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

