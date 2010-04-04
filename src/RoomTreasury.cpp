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

int RoomTreasury::depositGold(int gold, Tile *tile)
{
	//TODO: Make this enforce limits on how much gold can be deposited in the treasury.
	goldInTile[tile] += gold;
	return gold;
}

int RoomTreasury::withdrawGold(int gold)
{
	int withdrawlAmount = 0;
	for(map<Tile*,int>::iterator itr = goldInTile.begin(); itr != goldInTile.end(); itr++)
	{
		// Check to see if the current room tile has enough gold in it to fill the amount we still need to pick up.
		int goldStillNeeded = gold - withdrawlAmount;
		if(itr->second > goldStillNeeded)
		{
			// There is enough to satisfy the request so we do so and exit the loop.
			withdrawlAmount += goldStillNeeded;
			itr->second -= goldStillNeeded;
			break;
		}
		else
		{
			// There is not enough to satisfy the request so take everything there is and move on to the next tile.
			withdrawlAmount += itr->second;
			itr->second = 0;
		}
	}

	return withdrawlAmount;
}

