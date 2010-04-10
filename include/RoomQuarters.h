#ifndef ROOMQUARTERS_H
#define ROOMQUARTERS_H

#include "Room.h"

class RoomQuarters : public Room
{
	public:
		RoomQuarters();

		// Functions overriding virtual functions in the Room base class.
		void doUpkeep();
		void addCoveredTile(Tile* t);
		void removeCoveredTile(Tile* t);
		void clearCoveredTiles();

		// Functions specific to this class.
		vector<Tile*> getOpenTiles();
		bool claimTileForSleeping(Tile *t, Creature *c);
		bool releaseTileForSleeping(Tile *t, Creature *c);

	private:
		map<Tile*,Creature*> creatureSleepingInTile;
};

#endif

