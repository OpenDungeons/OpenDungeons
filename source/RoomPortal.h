#ifndef ROOMPORTAL_H
#define ROOMPORTAL_H

#include <vector>
#include <map> //For pair

#include "CreatureDefinition.h"
#include "Room.h"

class RoomPortal: public Room
{
    public:
        RoomPortal();

        // Functions overriding virtual functions in the Room base class.
        void createMeshes();
        void addCoveredTile(Tile* t, double nHP = Room::defaultTileHP);
        void removeCoveredTile(Tile* t);
        bool doUpkeep(Room *r);

        // Functions specific to this class.
        void spawnCreature();

    private:
        void recomputeClassProbabilities();
        void recomputeCenterPosition();

        int spawnCreatureCountdown;

        std::vector<std::pair<CreatureDefinition*, double> > classProbabilities;
        double xCenter, yCenter;

        RoomObject *portalObject;
};

#endif

