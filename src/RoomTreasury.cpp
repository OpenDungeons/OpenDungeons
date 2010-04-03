#include "RoomTreasury.h"

RoomTreasury::RoomTreasury()
	: Room()
{
	return;
}

void RoomTreasury::doUpkeep()
{
	// Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
	Room::doUpkeep();
}

void RoomTreasury::addCoveredTile(Tile* t)
{
	Room::addCoveredTile(t);
	goldInTile[t] = 0;
}

void RoomTreasury::removeCoveredTile(Tile* t)
{
	Room::removeCoveredTile(t);
	goldInTile.erase(t);
}

void RoomTreasury::clearCoveredTiles()
{
	Room::clearCoveredTiles();
	goldInTile.clear();
}

int RoomTreasury::getTotalGold()
{
	int tempInt = 0;

	for(map<Tile*,int>::iterator itr = goldInTile.begin(); itr != goldInTile.end(); itr++)
		tempInt += (*itr).second;

	return tempInt;
}

void RoomTreasury::depositGold(int gold, Tile *tile)
{
	goldInTile[tile] += gold;
}

