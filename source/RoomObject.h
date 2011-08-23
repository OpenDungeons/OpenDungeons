#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include "ActiveObject.h"
#include "AnimatedObject.h"
#include <string>
#include <istream>
#include <ostream>

class Room;
class GameMap;

class RoomObject: public ActiveObject, public AnimatedObject
{
    public:
        RoomObject(Room* nParentRoom, const std::string& nMeshName);

        const std::string& getName() const {return name;}
        const std::string& getMeshName(){return meshName;}

        Room* getParentRoom();

        void createMesh();
        void destroyMesh();
        void deleteYourself();

        std::string getOgreNamePrefix();

        static const char* getFormat();
        friend std::ostream& operator<<(std::ostream& os, RoomObject *o);
        friend std::istream& operator>>(std::istream& is, RoomObject *o);

		Ogre::Real x, y;
        Ogre::Real rotationAngle;

    private:
        Room *parentRoom;
        bool meshExists;
        std::string name, meshName;
};

#endif

