#include "RoomQuarters.h"
#include "Functions.h"

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
	if(creatureSleepingInTile[t] != NULL)
	{
		Creature *c = creatureSleepingInTile[t];
		if(c != NULL)
		{
			// Loop over all the tiles in this room and if they are slept on by creature c then set them back to NULL.
			for(map<Tile*,Creature*>::iterator itr = creatureSleepingInTile.begin(); itr != creatureSleepingInTile.end(); itr++)
			{
				if(itr->second == c)
					itr->second = NULL;
			}
		}

		RenderRequest *request = new RenderRequest;
		request->type = RenderRequest::destroyBed;
		request->p = t;
		request->p2 = creatureSleepingInTile[t];
		request->p3 = this;

		// Add the request to the queue of rendering operations to be performed before the next frame.
		queueRenderRequest(request);
	}

	creatureSleepingInTile.erase(t);
	Room::removeCoveredTile(t);
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
	int xDim, yDim;

	// Check to see if there is already a creature which has claimed this tile for sleeping.
	if(creatureSleepingInTile[t] == NULL)
	{
		bool normalDirection, spaceIsBigEnough = false;

		// Check to see whether the bed should be situated x-by-y or y-by-x tiles.
		if(tileCanAcceptBed(t, c->bedDim1, c->bedDim2))
		{
			normalDirection = true;
			spaceIsBigEnough = true;
			xDim = c->bedDim1;
			yDim = c->bedDim2;
		}

		if(!spaceIsBigEnough && tileCanAcceptBed(t, c->bedDim2, c->bedDim1))
		{
			normalDirection = false;
			spaceIsBigEnough = true;
			xDim = c->bedDim2;
			yDim = c->bedDim1;
		}

		if(spaceIsBigEnough)
		{
			// Mark all of the affected tiles as having this creature sleeping in them.
			for(int i = 0; i < xDim; i++)
			{
				for(int j = 0; j < yDim; j++)
				{
					Tile *tempTile = gameMap.getTile(t->x + i, t->y + j);
					creatureSleepingInTile[tempTile] = c;
				}
			}

			RenderRequest *request = new RenderRequest;
			request->type = RenderRequest::createBed;
			request->p = t;
			request->p2 = c;
			request->p3 = this;
			request->b = normalDirection;

			// Add the request to the queue of rendering operations to be performed before the next frame.
			queueRenderRequest(request);

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool RoomQuarters::releaseTileForSleeping(Tile *t, Creature *c)
{
	if(creatureSleepingInTile[t] != NULL)
	{
		// Loop over all the tiles in this room and if they are slept on by creature c then set them back to NULL.
		for(map<Tile*,Creature*>::iterator itr = creatureSleepingInTile.begin(); itr != creatureSleepingInTile.end(); itr++)
		{
			if(itr->second == c)
				itr->second = NULL;
		}

		RenderRequest *request = new RenderRequest;
		request->type = RenderRequest::destroyBed;
		request->p = t;
		request->p2 = c;
		request->p3 = this;

		// Add the request to the queue of rendering operations to be performed before the next frame.
		queueRenderRequest(request);

		return true;
	}
	else
	{
		return false;
	}
}

Tile* RoomQuarters::getLocationForBed(int xDim, int yDim)
{
	// Force the dimensions to be positive.
	if(xDim < 0)  xDim *= -1;
	if(yDim < 0)  yDim *= -1;

	// Check to see if there is even enough space available for the bed.
	vector<Tile*> tempVector = getOpenTiles();
	if(tempVector.size() < xDim*yDim)  return NULL;

	// Randomly shuffle the open tiles in tempVector so that the quarters are filled up in a random order.
	std::random_shuffle(tempVector.begin(), tempVector.end());

	// Loop over each of the open tiles in tempVector and for each one, check to see if it 
	for(unsigned int i = 0; i < tempVector.size(); i++)
	{
		if(tileCanAcceptBed(tempVector[i], xDim, yDim))
			return tempVector[i];
	}

	// We got to the end of the open tile list without finding an open tile for the bed so return NULL to indicate failure.
	return NULL;
}

bool RoomQuarters::tileCanAcceptBed(Tile *tile, int xDim, int yDim)
{
	//TODO: This function could be made more efficient by making it take the list of open tiles as an argument so if it is called repeatedly the tempTiles vecotor below only has to be computed once in the calling function rather than N times in this function.

	// Force the dimensions to be positive.
	if(xDim < 0)  xDim *= -1;
	if(yDim < 0)  yDim *= -1;

	// If either of the dimensions is 0 just return true, since the bed takes no space.  This should never really happen anyway.
	if(xDim == 0 || yDim == 0)  return true;

	// If the tile is invalid or not part of this room then the bed cannot be placed in this room.
	if(tile == NULL || tile->getCoveringRoom() != this)  return false;

	// Create a 2 dimensional array of booleans initially all set to false.
	vector< vector<bool> > tileOpen(xDim);
	for(int i = 0; i < xDim; i++)
	{
		tileOpen[i].resize(yDim, false);
	}

	// Now loop over the list of all the open tiles in this quarters.  For each tile, if it falls within
	// the xDim by yDim area from the starting tile we set the corresponding tileOpen entry to true.
	vector<Tile*> tempTiles = getOpenTiles();
	for(unsigned int i = 0; i < tempTiles.size(); i++)
	{
		int xDist = tempTiles[i]->x - tile->x;
		int yDist = tempTiles[i]->y - tile->y;
		if(xDist >= 0 && xDist < xDim && yDist >= 0 && yDist < yDim)
			tileOpen[xDist][yDist] = true;
	}

	// Loop over the tileOpen array and check to see if every value has been set to true, if it has then
	// we can place the a bed of the specified dimensions with its corner at the specified starting tile.
	bool returnValue = true;
	for(int i = 0; i < xDim; i++)
	{
		for(int j = 0; j < yDim; j++)
		{
			returnValue = returnValue && tileOpen[i][j];
		}
	}

	return returnValue;
}

