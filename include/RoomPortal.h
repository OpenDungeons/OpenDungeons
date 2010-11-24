#ifndef ROOMPORTAL_H
#define ROOMPORTAL_H

#include "Room.h"
#include "CreatureClass.h"

class RoomPortal : public Room
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

		std::vector< pair<CreatureClass*,double> > classProbabilities;
		double xCenter, yCenter;
};

#endif

