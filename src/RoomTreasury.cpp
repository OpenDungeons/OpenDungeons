#include "RoomTreasury.h"
#include "Functions.h"

RoomTreasury::RoomTreasury()
	: Room()
{
	return;
}

void RoomTreasury::absorbRoom(Room *r)
{
	// Start by deleting the Ogre meshes associated with both rooms.
	destroyMeshes();
	destroyGoldMeshes();
	r->destroyMeshes();

	// Copy over the information about the gold that is stored in the other treasury before we remove its rooms.
	for(unsigned int i = 0; i < r->numCoveredTiles(); i++)
	{
		Tile *tempTile = r->getCoveredTile(i);
		goldInTile[tempTile] = ((RoomTreasury*)r)->goldInTile[tempTile];
		fullnessOfTile[tempTile] = ((RoomTreasury*)r)->fullnessOfTile[tempTile];
	}

	// Use the superclass function to copy over the covered tiles to this room and get rid of them in the other room.
	Room::absorbRoom(r);

	// Recreate the meshes for this new room which contains both rooms.
	createMeshes();

	// Recreate the gold indicators which were destroyed when the meshes were cleared.
	createGoldMeshes();
}

void RoomTreasury::doUpkeep(Room *r)
{
	// Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
	Room::doUpkeep(this);
}

void RoomTreasury::addCoveredTile(Tile* t, double nHP)
{
	Room::addCoveredTile(t, nHP);

	// Only initialize the tile to empty if it has not already been set by the absorbRoom() function.
	if(goldInTile.find(t) == goldInTile.end())
	{
		goldInTile[t] = 0;
		fullnessOfTile[t] = empty;
	}
}

void RoomTreasury::removeCoveredTile(Tile* t)
{
	Room::removeCoveredTile(t);
	goldInTile.erase(t);
	fullnessOfTile.erase(t);
}

void RoomTreasury::clearCoveredTiles()
{
	Room::clearCoveredTiles();
	goldInTile.clear();
	fullnessOfTile.clear();
}

int RoomTreasury::getTotalGold()
{
	int tempInt = 0;

	for(map<Tile*,int>::iterator itr = goldInTile.begin(); itr != goldInTile.end(); itr++)
		tempInt += (*itr).second;

	return tempInt;
}

int RoomTreasury::emptyStorageSpace()
{
	return numCoveredTiles()*maxGoldWhichCanBeStoredInAChest - getTotalGold();
}

int RoomTreasury::depositGold(int gold, Tile *tile)
{
	int goldDeposited, goldToDeposit = gold, emptySpace;

	// Start by trying to deposit the gold in the requested tile.
	emptySpace = maxGoldWhichCanBeStoredInAChest - goldInTile[tile];
	goldDeposited = min(emptySpace, goldToDeposit);
	goldInTile[tile] += goldDeposited;
	goldToDeposit -= goldDeposited;
	updateMeshesForTile(tile);

	// If there is still gold left to deposit after the first tile, loop over all of the tiles and see if we can put the gold in another tile.
	for(map<Tile*,int>::iterator itr = goldInTile.begin(); itr != goldInTile.end() && goldToDeposit > 0; itr++)
	{
		// Store as much gold as we can in this tile.
		emptySpace = maxGoldWhichCanBeStoredInAChest - itr->second;
		goldDeposited = min(emptySpace, goldToDeposit);
		itr->second += goldDeposited;
		goldToDeposit -= goldDeposited;
		updateMeshesForTile(itr->first);
	}

	// Return the amount we were actually able to deposit (i.e. the amount we wanted to deposit minus the amount we were unable to deposit).
	return gold - goldToDeposit;
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
			updateMeshesForTile(itr->first);
			break;
		}
		else
		{
			// There is not enough to satisfy the request so take everything there is and move on to the next tile.
			withdrawlAmount += itr->second;
			itr->second = 0;
			updateMeshesForTile(itr->first);
		}
	}

	return withdrawlAmount;
}

RoomTreasury::TreasuryTileFullness RoomTreasury::getTreasuryTileFullness(int gold)
{
	if(gold <= 0)
		return empty;

	if(gold <= maxGoldWhichCanBeStoredInABag)
		return bag;

	if(gold <= maxGoldWhichCanBeStoredInAChest)
		return chest;

	return overfull;
}

string RoomTreasury::getMeshNameForTreasuryTileFullness(TreasuryTileFullness fullness)
{
	switch(fullness)
	{
		// The empty case should really never happen since we shouldn't be creating meshes for an empty tile anyway.
		case empty:        return "TreasuryTileEmptyError";     break;
		case bag:          return "GoldBag";                    break;
		case chest:        return "GoldChest";                  break;
		case overfull:     return "TreasuryTileOverfullError";  break;
	}

	return "TreasuryTileFullnessMeshError";
}

void RoomTreasury::updateMeshesForTile(Tile *t)
{
	TreasuryTileFullness newFullness = getTreasuryTileFullness(goldInTile[t]);

	// If the mesh has not changed we do not need to do anything.
	if(fullnessOfTile[t] == newFullness)
		return;

	// Since the fullness level has changed we need to destroy the existing indicator mesh (if it exists) and create a new one.
	destroyMeshesForTile(t);
	fullnessOfTile[t] = newFullness;
	createMeshesForTile(t);
}

void RoomTreasury::createMeshesForTile(Tile *t)
{
	// If the tile is empty, there is no indicator mesh to create so we just return.
	if(fullnessOfTile[t] == empty)
		return;

	string indicatorMeshName = getMeshNameForTreasuryTileFullness(fullnessOfTile[t]);

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createTreasuryIndicator;
	request->p = t;
	request->p2 = this;
	request->str = indicatorMeshName;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

void RoomTreasury::destroyMeshesForTile(Tile *t)
{
	// If the tile is empty, there is no indicator mesh to destroy so we just return.
	if(fullnessOfTile[t] == empty)
		return;

	string indicatorMeshName = getMeshNameForTreasuryTileFullness(fullnessOfTile[t]);

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyTreasuryIndicator;
	request->p = t;
	request->p2 = this;
	request->str = indicatorMeshName;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

void RoomTreasury::createGoldMeshes()
{
	for(unsigned int i = 0; i < numCoveredTiles(); i++)
		createMeshesForTile(getCoveredTile(i));
}

void RoomTreasury::destroyGoldMeshes()
{
	for(unsigned int i = 0; i < numCoveredTiles(); i++)
		destroyMeshesForTile(getCoveredTile(i));
}

