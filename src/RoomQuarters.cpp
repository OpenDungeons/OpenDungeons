#include "RoomQuarters.h"

RoomQuarters::RoomQuarters()
	: Room()
{
	return;
}

void RoomQuarters::doUpkeep()
{
	// Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
	Room::doUpkeep();
}

void RoomQuarters::addCoveredTile(Tile* t)
{
	Room::addCoveredTile(t);
	creatureSleepingInTile[t] = NULL;
}

void RoomQuarters::removeCoveredTile(Tile* t)
{
	Room::removeCoveredTile(t);
	creatureSleepingInTile.erase(t);
}

void RoomQuarters::clearCoveredTiles()
{
	Room::clearCoveredTiles();
	creatureSleepingInTile.clear();
}

vector<Tile*> RoomQuarters::getOpenTiles()
{
	vector<Tile*> returnVector;

	for(map<Tile*,Creature*>::iterator itr = creatureSleepingInTile.begin(); itr != creatureSleepingInTile.end(); itr++)
	{
		if(itr->second == NULL)
			returnVector.push_back(itr->first);
	}

	return returnVector;
}

bool RoomQuarters::claimTileForSleeping(Tile *t, Creature *c)
{
	// Check to see if there is already a creature which has claimed this tile for sleeping.
	if(creatureSleepingInTile[t] == NULL)
	{
		creatureSleepingInTile[t] = c;
		return true;
	}
	else
	{
		return false;
	}
}

