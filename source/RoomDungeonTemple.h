#ifndef ROOMDUNGEONTEMPLE_H
#define ROOMDUNGEONTEMPLE_H

#include "Room.h"

class RoomDungeonTemple: public Room
{
    public:
        RoomDungeonTemple();

        void createMesh();
        void destroyMesh();

        // Functions specific to this class.
        void produceKobold();

    private:
        int waitTurns;
};

#endif

