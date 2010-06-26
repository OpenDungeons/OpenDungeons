#if defined(WIN32) || defined(_WIN32)
#define snprintf _snprintf
#endif

#include <iostream>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>
#include <stdlib.h>
using namespace std;

#include "Functions.h"
#include "Defines.h"
#include "GameMap.h"

GameMap::GameMap()
{
	nextUniqueFloodFillColor = 1;
	loadNextLevel = false;
	floodFillEnabled = false;
	numCallsTo_path = 0;
	averageAILeftoverTime = 0.0;
	sem_init(&threadReferenceCountLockSemaphore, 0, 1);
	sem_init(&creaturesLockSemaphore, 0, 1);
	sem_init(&tilesLockSemaphore, 0, 1);
}

/*! \brief Erase all creatures, tiles, etc. from the map and make a new rectangular one.
 *
 * The new map consists entirely of the same kind of tile, with no creature
 * classes loaded, no players, and no creatures.
 */
void GameMap::createNewMap(int xSize, int ySize)
{
	Tile *tempTile;
	char array[255];

	clearAll();

	for(int j = 0; j < ySize; j++)
	{
		for(int i = 0; i < xSize; i++)
		{
			tempTile = new Tile;
			tempTile->setType(Tile::dirt);
			tempTile->setFullness(100);
			tempTile->x = i;
			tempTile->y = j;

			snprintf(array, sizeof(array), "Level_%3i_%3i", i, j);
			tempTile->name = array;
			tempTile->createMesh();
			sem_wait(&tilesLockSemaphore);
			tiles.insert( pair< pair<int,int>, Tile* >(pair<int,int>(i,j), tempTile) );
			sem_post(&tilesLockSemaphore);
		}
	}

	// Loop over all the tiles and force them to examine their
	// neighbors.  This allows them to switch to a mesh with fewer
	// polygons if some are hidden by the neighbors.
	TileMap_t::iterator itr = gameMap.firstTile();
	while(itr != gameMap.lastTile())
	{
		itr->second->setFullness( itr->second->getFullness() );
		itr++;
	}
}

/*! \brief Returns a pointer to the tile at location (x, y).
 *
 * The tile pointers are stored internally in a map so calls to this function
 * have a complexity O(log(N)) where N is the number of tiles in the map.
 */
Tile* GameMap::getTile(int x, int y)
{
	Tile *returnValue = NULL;
	pair<int,int> location(x, y);

	sem_wait(&tilesLockSemaphore);
	TileMap_t::iterator itr = tiles.find(location);
	if(itr != tiles.end())
		returnValue = itr->second;
	else
		returnValue = NULL;
	sem_post(&tilesLockSemaphore);

	return returnValue;
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles, creatures, classes, and players in the GameMap.
 *
 */
void GameMap::clearAll()
{
	clearGoalsForAllSeats();
	clearEmptySeats();
	clearFilledSeats();
	clearPlayers();

	clearCreatures();
	clearClasses();

	clearMapLights();
	clearRooms();
	clearTiles();
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles in the GameMap.
 *
 */
void GameMap::clearTiles()
{
	sem_wait(&tilesLockSemaphore);
	TileMap_t::iterator itr = tiles.begin();
	while(itr != tiles.end())
	{
		itr->second->deleteYourself();
		itr++;
	}

	tiles.clear();
	sem_post(&tilesLockSemaphore);
}

/*! \brief Clears the mesh and deletes the data structure for all the creatures in the GameMap.
 *
 */
void GameMap::clearCreatures()
{
	sem_wait(&creaturesLockSemaphore);
	for(unsigned int i = 0; i < creatures.size(); i++)
		creatures[i]->deleteYourself();

	creatures.clear();
	sem_post(&creaturesLockSemaphore);
}

/*! \brief Deletes the data structure for all the creature classes in the GameMap.
 *
 */
void GameMap::clearClasses()
{
	for(unsigned int i = 0; i < numClassDescriptions(); i++)
	{
		delete classDescriptions[i];
	}

	classDescriptions.clear();
}

/*! \brief Deletes the data structure for all the players in the GameMap.
 *
 */
void GameMap::clearPlayers()
{
	for(unsigned int i = 0; i < numPlayers(); i++)
	{
		delete players[i];
	}

	players.clear();
}

/*! \brief Returns the number of tile pointers currently stored in this GameMap.
 *
 */
unsigned int GameMap::numTiles()
{
	sem_wait(&tilesLockSemaphore);
	unsigned int tempUnsigned = tiles.size();
	sem_post(&tilesLockSemaphore);

	return tempUnsigned;
}

/*! \brief Adds the address of a new tile to be stored in this GameMap.
 *
 */
void GameMap::addTile(Tile *t)
{
	// Notify the neighbor tiles already existing on the GameMap of our existance.
	bool allNeighborsSameColor = true;
	for(unsigned int i = 0; i < 4; i++)
	{
		int tempX = t->x, tempY = t->y;
		switch(i)
		{
			break;
			case 0:  tempX++;  break;
			case 1:  tempY++;  break;
			case 2:  tempX--;  break;
			case 3:  tempY--;  break;

			default:
				 cerr << "\n\n\nERROR:  Unknown neighbor index.\n\n\n";
				 exit(1);
		}

		// If the current neigbor tile exists, add the current tile as one of its
		// neighbors and add it as one of the current tile's neighbors.
		Tile *tempTile = getTile(tempX, tempY);
		if(tempTile != NULL)
		{
			tempTile->addNeighbor(t);
			t->addNeighbor(tempTile);

			allNeighborsSameColor = allNeighborsSameColor && (tempTile->floodFillColor == t->floodFillColor);
		}
	}

	sem_wait(&tilesLockSemaphore);
	tiles.insert( pair< pair<int,int>, Tile* >(pair<int,int>(t->x,t->y), t) );
	sem_post(&tilesLockSemaphore);
}

/** \brief Returns all the valid tiles in the rectangular region specified by the two corner points given.
 *
 */
vector<Tile*> GameMap::rectangularRegion(int x1, int y1, int x2, int y2)
{
	vector<Tile*> returnList;
	Tile *tempTile;

	if(x1 > x2)  swap(x1, x2);
	if(y1 > y2)  swap(y1, y2);

	for(int i = x1; i <= x2; i++)
	{
		for(int j = y1; j <= y2; j++)
		{
			//TODO:  This routine could be sped up by using the neighborTiles function.
			tempTile = getTile(i, j);

			if(tempTile != NULL)
				returnList.push_back(tempTile);
		}
	}

	return returnList;
}

/** \brief Returns all the valid tiles in the curcular region surrounding the given point and extending outward to the specified radius.
 *
 */
vector<Tile*> GameMap::circularRegion(int x, int y, double radius)
{
	vector<Tile*> returnList;
	Tile *tempTile;
	int xDist, yDist, distSquared;
	double radiusSquared = radius*radius;

	if(radius < 0.0)  radius = 0.0;

	for(int i = x-radius; i <= x+radius; i++)
	{
		for(int j = y-radius; j <= y+radius; j++)
		{
			//TODO:  This routine could be sped up by using the neighborTiles function.
			xDist = i - x;
			yDist = j - y;
			distSquared = xDist*xDist + yDist*yDist;
			if(distSquared < radiusSquared)
			{
				tempTile = getTile(i, j);
				if(tempTile != NULL)
					returnList.push_back(tempTile);
			}
		}
	}
	return returnList;
}

/** \brief Returns a vector of all the valid tiles which are a neighbor to one or more tiles in the specified region, i.e. the "perimeter" of the region extended out one tile.
 *
 */
vector<Tile*> GameMap::tilesBorderedByRegion(const vector<Tile*> &region)
{
	vector<Tile*> neighbors, returnList;

	// Loop over all the tiles in the specified region.
	for(unsigned int i = 0; i < region.size(); i++)
	{
		// Get the tiles bordering the current tile and loop over them.
		neighbors = neighborTiles(region[i]);
		for(unsigned int j = 0; j < neighbors.size(); j++)
		{
			bool neighborFound = false;

			// Check to see if the current neighbor is one of the tiles in the region.
			for(unsigned int k = 0; k < region.size(); k++)
			{
				if(region[k] == neighbors[j])
				{
					neighborFound = true;
					break;
				}
			}

			if(!neighborFound)
			{
				// Check to see if the current neighbor is already in the returnList.
				for(unsigned int k = 0; k < returnList.size(); k++)
				{
					if(returnList[k] == neighbors[j])
					{
						neighborFound = true;
						break;
					}
				}
			}

			// If the given neighbor was not already in the returnList, then add it.
			if(!neighborFound)
				returnList.push_back(neighbors[j]);
		}
	}

	return returnList;
}

/*! \brief Adds the address of a new class description to be stored in this GameMap.
 *
 * The class descriptions take the form of a creature data structure with most
 * of the data members filled in.  This class structure is then copied into the
 * data structure of new creatures that are added who are of this class.  The
 * copied members include things like HP, mana, etc, that are the same for all
 * members of that class.  Creature specific things like location, etc. are
 * then filled out for the individual creature.
 */
void GameMap::addClassDescription(CreatureClass *c)
{
	classDescriptions.push_back( c );
}

/*! \brief Copies the creature class structure into a newly created structure and stores the address of the new structure in this GameMap.
 *
 */
void GameMap::addClassDescription(CreatureClass c)
{
	classDescriptions.push_back( new CreatureClass(c) );
}

/*! \brief Adds the address of a new creature to be stored in this GameMap.
 *
 */
void GameMap::addCreature(Creature *c)
{
	sem_wait(&creaturesLockSemaphore);
	creatures.push_back(c);
	sem_post(&creaturesLockSemaphore);

	c->positionTile()->addCreature(c);
}

/*! \brief Removes the creature from the game map but does not delete its data structure.
 *
 */
void GameMap::removeCreature(Creature *c)
{
	sem_wait(&creaturesLockSemaphore);

	// Loop over the creatures looking for creature c
	for(unsigned int i = 0; i < creatures.size(); i++)
	{
		if(c == creatures[i])
		{
			// Creature found
			// Remove the creature from the tile it's in
			c->positionTile()->removeCreature(c);
			creatures.erase(creatures.begin()+i);
			break;
		}
	}

	sem_post(&creaturesLockSemaphore);
}

/** \brief Adds the given creature to the queue of creatures to be deleted in a future turn
  * when it is safe to do so after all references to the creature have been cleared.
  *
*/
void GameMap::queueCreatureForDeletion(Creature *c)
{
	// If the creature has a homeTile where they sleep, their bed needs to be destroyed.
	if(c->homeTile != NULL)
		((RoomQuarters*)c->homeTile->getCoveringRoom())->releaseTileForSleeping(c->homeTile, c);

	// Remove the creature from the GameMap in case the caller forgot to do so.
	removeCreature(c);

	//TODO: This needs to include the turn number that the creature was pushed so proper multithreaded locks can be by the threads to retire the creatures.
	creaturesToDelete[turnNumber.get()].push_back(c);
}

/*! \brief Returns a pointer to the first class description whose 'name' parameter matches the query string.
 *
 */
CreatureClass* GameMap::getClassDescription(string query)
{
	for(unsigned int i = 0; i < classDescriptions.size(); i++)
	{
		if(classDescriptions[i]->className.compare(query) == 0)
			return classDescriptions[i];
	}

	return NULL;
}

/*! \brief Returns the total number of creatures stored in this game map.
 *
 */
unsigned int GameMap::numCreatures()
{
	sem_wait(&creaturesLockSemaphore);
	unsigned int tempUnsigned = creatures.size();
	sem_post(&creaturesLockSemaphore);

	return tempUnsigned;
}

/*! \brief Returns the total number of class descriptions stored in this game map.
 *
 */
unsigned int GameMap::numClassDescriptions()
{
	return classDescriptions.size();
}

/*! \brief Gets the i'th creature in this GameMap.
 *
 */
Creature* GameMap::getCreature(int index)
{
	sem_wait(&creaturesLockSemaphore);
	Creature *tempCreature = creatures[index];
	sem_post(&creaturesLockSemaphore);

	return tempCreature;
}

/*! \brief Gets the i'th class description in this GameMap.
 *
 */
CreatureClass* GameMap::getClassDescription(int index)
{
	return classDescriptions[index];
}

/*! \brief Creates meshes for all the tiles and creatures stored in this GameMap.
 *
 */
void GameMap::createAllEntities()
{
	// Create OGRE entities for map tiles
	sem_wait(&tilesLockSemaphore);
	TileMap_t::iterator itr = tiles.begin();
	while(itr != tiles.end())
	{
		itr->second->createMesh();
		itr++;
	}
	sem_post(&tilesLockSemaphore);

	// Create OGRE entities for the creatures
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		Creature *currentCreature = getCreature(i);
		currentCreature->createMesh();
		currentCreature->weaponL->createMesh();
		currentCreature->weaponR->createMesh();
	}

	// Create OGRE entities for the map lights.
	for(unsigned int i = 0; i < numMapLights(); i++)
	{
		MapLight *currentMapLight = getMapLight(i);
		currentMapLight->createOgreEntity();
	}

	// Create OGRE entities for the rooms
	for(unsigned int i = 0; i < numRooms(); i++)
	{
		Room *currentRoom = getRoom(i);
		currentRoom->createMeshes();
	}
}

/*! \brief Returns a pointer to the creature whose name matches cName.
 *
 */
Creature* GameMap::getCreature(string cName)
{
	Creature *returnValue = NULL;

	sem_wait(&creaturesLockSemaphore);
	for(unsigned int i = 0; i < creatures.size(); i++)
	{
		if(creatures[i]->name.compare(cName) == 0)
		{
			returnValue = creatures[i];
			break; 
		}
	}
	sem_post(&creaturesLockSemaphore);

	return returnValue;
}

/*! \brief Loops over all the creatures and calls their individual doTurn methods, also check goals and do the upkeep.
 *
 */
void GameMap::doTurn()
{
	// Local variables
	Seat *tempSeat;
	Tile *tempTile;
	unsigned int tempUnsigned;

	// Compute the moving window average of how much extra time was left over after the previous doTurn() calls finished.
	averageAILeftoverTime = 0.0;
	for(tempUnsigned = 0; tempUnsigned < previousLeftoverTimes.size(); tempUnsigned++)
		averageAILeftoverTime += previousLeftoverTimes[tempUnsigned];

	if(tempUnsigned > 0)
		averageAILeftoverTime /= (double)tempUnsigned;

	sem_wait(&creatureAISemaphore);

	if(loadNextLevel)
	{
		if(numCreatures() > 0)
		{
			while(numCreatures() > 0)
			{
				sem_wait(&creaturesLockSemaphore);
				Creature *tempCreature = creatures[0];
				sem_post(&creaturesLockSemaphore);

				queueCreatureForDeletion(tempCreature);
			}
		}
		else
		{
			loadNextLevel = false;
			//TODO: The return value from the level load should be checked to make sure it loaded properly.
			readGameMapFromFile(nextLevel);
			createAllEntities();
			me->seat = popEmptySeat();
		}
	}

	cout << "\nStarting creature AI for turn " << turnNumber.get();
	unsigned int numCallsTo_path_atStart = numCallsTo_path;

	processDeletionQueues();

	// Loop over all the filled seats in the game and check all the unfinished goals for each seat.
	// Add any seats with no remaining goals to the winningSeats vector.
	for(unsigned int i = 0; i < numFilledSeats(); i++)
	{
		// Check the previously completed goals to make sure they are still met.
		filledSeats[i]->checkAllCompletedGoals();

		// Check the goals and move completed ones to the completedGoals list for the seat.
		//NOTE: Once seats are placed on this list, they stay there even if goals are unmet.  We may want to change this.
		if(filledSeats[i]->checkAllGoals() == 0 && filledSeats[i]->numFailedGoals() == 0)
			addWinningSeat(filledSeats[i]);

		// Set all the alignment and faction coefficients for this seat to 0, they will be
		// filled up in the loop below which removes the dead creatures from the map.
		filledSeats[i]->numCreaturesControlled = 0;
		filledSeats[i]->factionHumans = 0.0;
		filledSeats[i]->factionCorpars = 0.0;
		filledSeats[i]->factionUndead = 0.0;
		filledSeats[i]->factionConstructs = 0.0;
		filledSeats[i]->factionDenizens = 0.0;
		filledSeats[i]->alignmentAltruism = 0.0;
		filledSeats[i]->alignmentOrder = 0.0;
		filledSeats[i]->alignmentPeace = 0.0;
	}

	// Call the individual creature AI for each creature in this game map
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		sem_wait(&creaturesLockSemaphore);
		Creature *tempCreature = creatures[i];
		sem_post(&creaturesLockSemaphore);

		tempCreature->doTurn();
	}

	// Remove dead creatures from the map and put them into the deletion queue.
	unsigned int count = 0;
	while(count < numCreatures())
	{
		// Check to see if the creature has died.
		//TODO: Add code so creatures lay stunned for a while before they actually die.
		sem_wait(&creaturesLockSemaphore);
		Creature *tempCreature = creatures[count];
		sem_post(&creaturesLockSemaphore);
		if(tempCreature->getHP(tempCreature->positionTile()) < 0.0)
		{
			// Remove the creature from the game map and into the deletion queue, it will be deleted
			// when it is safe, i.e. all other pointers to it have been wiped from the program.
			cout << "\nMoving creature " << tempCreature->name << " from the game map to the deletion list.";
			cout.flush();
			tempCreature->setAnimationState("Die");
			queueCreatureForDeletion(tempCreature);
		}
		else
		{
			// Since the creature is still alive we should add its alignment and faction to
			// its controlling seat to be used in the RoomPortal::spawnCreature routine.
			Player *tempPlayer = tempCreature->getControllingPlayer();
			if(tempPlayer != NULL)
			{
				Seat *tempSeat = tempPlayer->seat;

				tempSeat->numCreaturesControlled++;

				tempSeat->factionHumans += tempCreature->coefficientHumans;
				tempSeat->factionCorpars += tempCreature->coefficientCorpars;
				tempSeat->factionUndead += tempCreature->coefficientUndead;
				tempSeat->factionConstructs += tempCreature->coefficientConstructs;
				tempSeat->factionDenizens += tempCreature->coefficientDenizens;

				tempSeat->alignmentAltruism += tempCreature->coefficientAltruism;
				tempSeat->alignmentOrder += tempCreature->coefficientOrder;
				tempSeat->alignmentPeace += tempCreature->coefficientPeace;
			}

			count++;
		}
	}

	// Determine the number of tiles claimed by each seat.
	// Begin by setting the number of claimed tiles for each seat to 0.
	for(unsigned int i = 0; i < filledSeats.size(); i++)
		filledSeats[i]->numClaimedTiles = 0;

	for(unsigned int i = 0; i < emptySeats.size(); i++)
		emptySeats[i]->numClaimedTiles = 0;

	// Now loop over all of the tiles, if the tile is claimed increment the given seats count.
	sem_wait(&tilesLockSemaphore);
	map< pair<int,int>, Tile*>::iterator currentTile = tiles.begin();
	while(currentTile != tiles.end())
	{
		tempTile = currentTile->second;

		// Check to see if the current tile is claimed by anyone.
		if(tempTile->getType() == Tile::claimed)
		{
			// Increment the count of the seat who owns the tile.
			tempSeat = getSeatByColor(tempTile->color);
			if(tempSeat != NULL)
			{
				tempSeat->numClaimedTiles++;

				// Add a small increment of this player's color to the tiles to allow the claimed area to grow on its own.
				vector<Tile*> neighbors = neighborTiles(currentTile->second);
				for(unsigned int i = 0; i < neighbors.size(); i++)
				{
					if(neighbors[i]->getType() == Tile::dirt && neighbors[i]->getFullness() < 0.1)// && neighbors[i]->colorDouble < 0.8)
						neighbors[i]->claimForColor(tempSeat->color, 0.04);
				}
			}
		}

		currentTile++;
	}
	sem_post(&tilesLockSemaphore);
	
	// Carry out the upkeep on each of the Rooms in the gameMap.
	//NOTE:  The auto-increment on this loop is canceled by a decrement in the if statement, changes to the loop structure will need to keep this consistent.
	for(unsigned int i = 0; i < gameMap.numRooms(); i++)
	{
		Room *tempRoom = gameMap.getRoom(i);
		tempRoom->doUpkeep(tempRoom);

		// Check to see if the room now has 0 covered tiles, if it does we can remove it from the map.
		if(tempRoom->numCoveredTiles() == 0)
		{
			removeRoom(tempRoom);
			tempRoom->deleteYourself();
			i--;  //NOTE:  This decrement is to cancel out the increment that will happen on the next loop iteration.
		}
	}

	// Carry out the upkeep round for each seat.  This means recomputing how much gold is
	// available in their treasuries, how much mana they gain/lose during this turn, etc.
	for(unsigned int i = 0; i < filledSeats.size(); i++)
	{
		tempSeat = filledSeats[i];

		// Add the amount of mana this seat accrued this turn.
		cout << "\nSeat " << i << " has " << tempSeat->numClaimedTiles << " claimed tiles.";
		tempSeat->manaDelta = 50 + tempSeat->numClaimedTiles;
		tempSeat->mana += tempSeat->manaDelta;
		if(tempSeat->mana > 250000)
			tempSeat->mana = 250000;

		// Update the count on how much gold is available in all of the treasuries claimed by the given seat.
		tempSeat->gold = getTotalGoldForColor(tempSeat->color);
	}

	cout << "\nDuring this turn there were " << numCallsTo_path-numCallsTo_path_atStart << " calls to GameMap::path().";

	sem_post(&creatureAISemaphore);
}

bool GameMap::pathExists(int x1, int y1, int x2, int y2, Tile::TileClearType passability)
{
	if(passability == Tile::walkableTile)
		return walkablePathExists(x1, y1, x2, y2);
	else
		return path(x1, y1, x2, y2, passability).size() >= 2;
}

/*! \brief Calculates the walkable path between tiles (x1, y1) and (x2, y2).
 *
 * The search is carried out using the A-star search algorithm.
 * The path returned contains both the starting and ending tiles, and consists
 * entirely of tiles which satify the 'passability' criterion specified in the
 * search.  The returned tiles are also a "manhattan path" meaning that every
 * successive tile is one of the 4 nearest neighbors of the previous tile in
 * the path.  In most cases you will want to call GameMap::cutCorners on the
 * returned path to shorten the number of steps on the path, as well as the
 * actual walking distance along the path.
 */
list<Tile*> GameMap::path(int x1, int y1, int x2, int y2, Tile::TileClearType passability)
{
	numCallsTo_path++;

	//TODO:  Make the openList a priority queue sorted by the cost to improve lookup times on retrieving the next open item.
	list<Tile*> returnList;
	astarEntry *currentEntry;
	Tile *destination;
	list<astarEntry*> openList;
	list<astarEntry*> closedList;
	list<astarEntry*>::iterator itr;

	// If the start tile was not found return an empty path
	if(getTile(x1, y1) == NULL)
		return returnList;

	// If the end tile was not found return an empty path
	destination = getTile(x2, y2);
	if(destination == NULL)
		return returnList;

	// If flood filling is enabled, we can possibly eliminate this path by checking to see if they two tiles are colored differently.
	if(floodFillEnabled && passability == Tile::walkableTile && !walkablePathExists(x1, y1, x2, y2))
		return returnList;

	//TODO:  make this a local variable, don't forget to remove the delete statement at the end of this function.
	astarEntry *neighbor = new astarEntry;

	currentEntry = new astarEntry;
	currentEntry->tile = getTile(x1, y1);
	currentEntry->parent = NULL;
	currentEntry->g = 0.0;
	currentEntry->setHeuristic(x1, y1, x2, y2);
	openList.push_back(currentEntry);

	bool pathFound = false;
	while(true)
	{
		// if the openList is empty we failed to find a path
		if(openList.size() <= 0)
			break;

		// Get the lowest fScore from the openList and move it to the closed list
		list<astarEntry*>::iterator itr = openList.begin(), smallestAstar = openList.begin();
		while(itr != openList.end())
		{
			if((*itr)->fCost() < (*smallestAstar)->fCost())
				smallestAstar = itr;
			itr++;
		}

		currentEntry = *smallestAstar;
		openList.erase(smallestAstar);
		closedList.push_back(currentEntry);

		// We found the path, break out of the search loop
		if(currentEntry->tile == destination)
		{
			pathFound = true;
			break;
		}

		// Check the tiles surrounding the current square
		vector<Tile*>neighbors = neighborTiles(currentEntry->tile);
		bool processNeighbor;
		for(unsigned int i = 0; i < neighbors.size(); i++)
		{
			neighbor->tile = neighbors[i];

			processNeighbor = true;
			if(neighbor->tile != NULL)
			{
				//TODO:  This code is duplicated in GameMap::pathIsClear, it should be moved into a function.
				// See if the neighbor tile in question is passable
				switch(passability)
				{
					case Tile::walkableTile:
						if( !(neighbor->tile->getTilePassability() == Tile::walkableTile) )
						{
							processNeighbor = false;  // skip this tile and go on to the next neighbor tile
						}
						break;

					case Tile::flyableTile:
						if( !(neighbor->tile->getTilePassability() == Tile::walkableTile || neighbor->tile->getTilePassability() == Tile::flyableTile) )
						{
							processNeighbor = false;  // skip this tile and go on to the next neighbor tile
						}
						break;

					case Tile::impassableTile:
						cerr << "\n\nERROR:  Trying to find a path through impassable tiles in GameMap::path()\n\n";
						exit(1);
						break;

					default:
						cerr << "\n\nERROR:  Unhandled tile type in GameMap::path()\n\n";
						exit(1);
						break;
				}

				if(processNeighbor)
				{
					// See if the neighbor is in the closed list
					bool neighborFound = false;
					list<astarEntry*>::iterator itr = closedList.begin();
					while(itr != closedList.end())
					{
						if(neighbor->tile == (*itr)->tile)
						{
							neighborFound = true;
							break;
						}
						else
						{
							itr++;
						}
					}

					// Ignore the neighbor if it is on the closed list
					if(!neighborFound)
					{
						// See if the neighbor is in the open list
						neighborFound = false;
						list<astarEntry*>::iterator itr = openList.begin();
						while(itr != openList.end())
						{
							if(neighbor->tile == (*itr)->tile)
							{
								neighborFound = true;
								break;
							}
							else
							{
								itr++;
							}
						}

						// If the neighbor is not in the open list
						if(!neighborFound)
						{
							// NOTE: This +1 weights all steps the same, diagonal steps
							// should get a greater wieght iis they are included in the future
							neighbor->g = currentEntry->g + 1;

							// Use the manhattan distance for the heuristic
							currentEntry->setHeuristic(x1, y1, neighbor->tile->x, neighbor->tile->y);
							neighbor->parent = currentEntry;

							openList.push_back(new astarEntry(*neighbor));
						}
						else
						{
							// If this path to the given neighbor tile is a shorter path than the
							// one already given, make this the new parent.
							// NOTE: This +1 weights all steps the same, diagonal steps
							// should get a greater wieght iis they are included in the future
							if(currentEntry->g + 1 < (*itr)->g)
							{
								// NOTE: This +1 weights all steps the same, diagonal steps
								// should get a greater wieght iis they are included in the future
								(*itr)->g = currentEntry->g + 1;
								(*itr)->parent = currentEntry;
							}
						}
					}
				}
			}
		}
	}

	if(pathFound)
	{
		//Find the destination tile in the closed list
		//TODO:  Optimize this by remembering this from above so this loop does not need to be carried out.
		itr = closedList.begin();
		while(itr != closedList.end())
		{
			if((*itr)->tile == destination)
				break;
			else
				itr++;
		}

		// Follow the parent chain back the the starting tile
		currentEntry = (*itr);
		do
		{
			if(currentEntry->tile != NULL)
			{
				returnList.push_front(currentEntry->tile);
				currentEntry = currentEntry->parent;
			}

		}while(currentEntry != NULL);
	}

	// Clean up the memory we allocated by deleting the astarEntries in the open and closed lists
	itr = openList.begin();
	while(itr != openList.end())
	{
		delete *itr;
		itr++;
	}

	itr = closedList.begin();
	while(itr != closedList.end())
	{
		delete *itr;
		itr++;
	}

	delete neighbor;

	return returnList;
}

/*! \brief Returns an iterator to be used for the purposes of looping over the tiles stored in this GameMap.
 *
 */
TileMap_t::iterator GameMap::firstTile()
{
	sem_wait(&tilesLockSemaphore);
	TileMap_t::iterator tempItr = tiles.begin();
	sem_post(&tilesLockSemaphore);

	return tempItr;
}

/*! \brief Returns an iterator to be used for the purposes of looping over the tiles stored in this GameMap.
 *
 */
TileMap_t::iterator GameMap::lastTile()
{
	sem_wait(&tilesLockSemaphore);
	TileMap_t::iterator tempItr = tiles.end();
	sem_post(&tilesLockSemaphore);

	return tempItr;
}

/*! \brief Returns the (up to) 4 nearest neighbor tiles of the tile located at (x, y).
 *
 */
vector<Tile*> GameMap::neighborTiles(int x, int y)
{
	vector<Tile*> tempVector;

	Tile *tempTile = getTile(x, y);
	if(tempTile != NULL)
		tempVector = neighborTiles(tempTile);

	return tempVector;
}

vector<Tile*> GameMap::neighborTiles(Tile *t)
{
	return t->getAllNeighbors();
}

/*! \brief Adds a pointer to a player structure to the players stored by this GameMap.
 *
 */
bool GameMap::addPlayer(Player *p)
{
	if(emptySeats.size() > 0)
	{
		p->seat = popEmptySeat();
		players.push_back(p);
		return true;
	}
	else
	{
		return false;
	}
}

/*! \brief Returns a pointer to the i'th player structure stored by this GameMap.
 *
 */
Player* GameMap::getPlayer(int index)
{
	return players[index];
}

//FIXME: This function is deprecated by getSeatByColor() and calls to this should likely be replaced by that function.
Player* GameMap::getPlayerByColour(int colour)
{
	for(unsigned int i = 0; i < players.size(); i++)
	{
		if(players[i]->seat->color == colour)
		{
			return players[i];
		}
	}

	return NULL;
}

/*! \brief Returns a pointer to the player structure stored by this GameMap whose name matches pName.
 *
 */
Player* GameMap::getPlayer(string pName)
{
	for(unsigned int i = 0; i < numPlayers(); i++)
	{
		if(players[i]->nick.compare(pName) == 0)
		{
			return players[i];
		}
	}

	return NULL;
}

/*! \brief Returns the number of player structures stored in this GameMap.
 *
 */
unsigned int GameMap::numPlayers()
{
	return players.size();
}

bool GameMap::walkablePathExists(int x1, int y1, int x2, int y2)
{
	Tile *tempTile1, *tempTile2;
	tempTile1 = getTile(x1, y1);
	if(tempTile1)
	{
		tempTile2 = getTile(x2, y2);
		if(tempTile2)
		{
			return (tempTile1->floodFillColor == tempTile2->floodFillColor);
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

/*! \brief Returns a list of valid tiles along a stright line from (x1, y1) to (x2, y2).
 *
 * This algorithm is from
 * http://en.wikipedia.org/w/index.php?title=Bresenham%27s_line_algorithm&oldid=295047020
 * A more detailed description of how it works can be found there.
 */
list<Tile*> GameMap::lineOfSight(int x0, int y0, int x1, int y1)
{
	list<Tile*> path;

	// Calculate the components of the 'manhattan distance'
	int Dx = x1 - x0;
	int Dy = y1 - y0;

	// Determine if the slope of the line is greater than 1
	int steep = (abs(Dy) >= abs(Dx));
	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
		// recompute Dx, Dy after swap
		Dx = x1 - x0;
		Dy = y1 - y0;
	}

	// Determine whether the x component is increasing or decreasing
	int xstep = 1;
	if (Dx < 0)
	{
		xstep = -1;
		Dx = -Dx;
	}

	// Determine whether the y component is increasing or decreasing
	int ystep = 1;
	if (Dy < 0)
	{
		ystep = -1;
		Dy = -Dy;
	}

	// Loop over the pixels on the line and add them to the return list
	int TwoDy = 2*Dy;
	int TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
	int E = TwoDy - Dx; //2*Dy - Dx
	int y = y0;
	int xDraw, yDraw;
	for (int x = x0; x != x1; x += xstep)
	{
		// Treat a steep line as if it were actually its inverse
		if (steep)
		{
			xDraw = y;
			yDraw = x;
		}
		else
		{
			xDraw = x;
			yDraw = y;
		}
		
		// if the tile exists, add it to the path
		Tile *currentTile = getTile(xDraw, yDraw);
		if(currentTile != NULL)
		{
			path.push_back(currentTile);
		}

		// If the error has accumulated to the next tile, "increment" the y coordinate
		if (E > 0)
		{
			E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
			y = y + ystep;
		}
		else
		{
			E += TwoDy; //E += 2*Dy;
		}
	}

	return path;
}

/*! \brief Determines whether or not you can travel along a path.
 *
 */
bool GameMap::pathIsClear(list<Tile*> path, Tile::TileClearType passability)
{
	list<Tile*>::iterator itr;
	list<Tile*>::iterator last;

	if(path.size() == 0)
		return false;

	last = --path.end();

	// Loop over tile in the path and check to see if it is clear
	bool isClear = true;
	for(itr = path.begin(); itr != path.end() && isClear; itr++)
	{
	 	//TODO:  This code is duplicated in GameMap::path, it should be moved into a function.
		// See if the path tile in question is passable
		switch(passability)
		{
			// Walking creatures can only move through walkableTile's.
			case Tile::walkableTile:
				isClear = (isClear && ((*itr)->getTilePassability() == Tile::walkableTile));
				break;

			// Flying creatures can move through walkableTile's or flyableTile's.
			case Tile::flyableTile:
				isClear = (isClear && ((*itr)->getTilePassability() == Tile::walkableTile || (*itr)->getTilePassability() == Tile::flyableTile));
				break;

			// No creatures can walk through impassableTile's
			case Tile::impassableTile:
				isClear = false;
				break;

			default:
				cerr << "\n\nERROR:  Unhandled tile type in GameMap::pathIsClear()\n\n";
				exit(1);
				break;
		}
	}

	return isClear;
}

/*! \brief Loops over a path an replaces 'manhattan' paths with 'as the crow flies' paths.
 *
 */
void GameMap::cutCorners(list<Tile*> &path, Tile::TileClearType passability)
{
	// Size must be >= 3 or else t3 and t4 can end up pointing at the same value
	if(path.size() <= 3)
		return;

	list<Tile*>::iterator t1 = path.begin();
	list<Tile*>::iterator t2 = t1;
	t2++;
	list<Tile*>::iterator t3;
	list<Tile*>::iterator t4;
	list<Tile*>::iterator secondLast = path.end();
	secondLast--;

	// Loop t1 over all but the last tile in the path
	while(t1 != path.end()) 
	{
		// Loop t2 from t1 until the end of the path
		t2 = t1;
		t2++;

		while(t2 != path.end())
		{
			// If we have a clear line of sight to t2, advance to
			// the next tile else break out of the inner loop
			list<Tile*> lineOfSightPath = lineOfSight( (*t1)->x, (*t1)->y, (*t2)->x, (*t2)->y );

			if( pathIsClear( lineOfSightPath, passability)  )
				t2++;
			else
				break;
		}

		// Delete the tiles 'strictly between' t1 and t2
		t3 = t1;
		t3++;
		if(t3 != t2)
		{
			t4 = t2;
			t4--;
			if(t3 != t4)
			{
				path.erase(t3, t4);
			}
		}

		t1 = t2;

		secondLast = path.end();
		secondLast--;
	}
}

/** \brief Calls the deleteYourself() method on each of the rooms in the game map as well as clearing the vector of stored rooms.
  *
*/ 
void GameMap::clearRooms()
{
	for(unsigned int i = 0; i < numRooms(); i++)
	{
		getRoom(i)->deleteYourself();
	}

	rooms.clear();
}

/** \brief A simple mutator method to add the given Room to the GameMap.
  *
*/ 
void GameMap::addRoom(Room *r)
{
	rooms.push_back(r);
}

void GameMap::removeRoom(Room *r)
{
	for(unsigned int i = 0; i < rooms.size(); i++)
	{
		if(r == rooms[i])
		{
			//TODO:  Loop over the tiles and make any whose coveringRoom variable points to this room point to NULL.
			rooms.erase(rooms.begin()+i);
			break;
		}
	}
}

/** \brief A simple accessor method to return the given Room.
  *
*/ 
Room* GameMap::getRoom(int index)
{
	return rooms[index];
}

/** \brief A simple accessor method to return the number of Rooms stored in the GameMap.
  *
*/ 
unsigned int GameMap::numRooms()
{
	return rooms.size();
}

vector<Room*> GameMap::getRoomsByTypeAndColor(Room::RoomType type, int color)
{
	vector<Room*> returnList;
	for(unsigned int i = 0; i < rooms.size(); i++)
	{
		if(rooms[i]->getType() == type && rooms[i]->color == color)
			returnList.push_back(rooms[i]);
	}

	return returnList;
}

vector<Room*> GameMap::getReachableRooms(const vector<Room*> &vec, Tile *startTile, Tile::TileClearType passability)
{
	vector<Room*> returnVector;

	for(unsigned int i = 0; i < vec.size(); i++)
	{
		if(pathExists(startTile->x, startTile->y, vec[i]->getCoveredTile(0)->x, vec[i]->getCoveredTile(0)->y, passability))
			returnVector.push_back(vec[i]);
	}

	return returnVector;
}

int GameMap::getTotalGoldForColor(int color)
{
	int tempInt = 0;
	vector<Room*> treasuriesOwned = getRoomsByTypeAndColor(Room::treasury, color);
	for(unsigned int i = 0; i < treasuriesOwned.size(); i++)
		tempInt += ((RoomTreasury*)treasuriesOwned[i])->getTotalGold();

	return tempInt;
}

int GameMap::withdrawFromTreasuries(int gold, int color)
{
	// Check to see if there is enough gold available in all of the treasuries owned by the given color.
	int totalGold = getTotalGoldForColor(color);
	if(totalGold < gold)
		return 0;

	// Loop over the treasuries withdrawing gold until the full amount has been withdrawn.
	int goldStillNeeded = gold;
	vector<Room*> treasuriesOwned = getRoomsByTypeAndColor(Room::treasury, color);
	for(unsigned int i = 0; i < treasuriesOwned.size() && goldStillNeeded > 0; i++)
		goldStillNeeded -= ((RoomTreasury*)treasuriesOwned[i])->withdrawGold(goldStillNeeded);

	return gold;
}

void GameMap::clearMapLights()
{
	for(unsigned int i = 0; i < mapLights.size(); i++)
	{
		mapLights[i]->deleteYourself();
	}

	mapLights.clear();
}

void GameMap::clearMapLightIndicators()
{
	for(unsigned int i = 0; i < mapLights.size(); i++)
		mapLights[i]->destroyOgreEntityVisualIndicator();
}

void GameMap::addMapLight(MapLight *m)
{
	mapLights.push_back(m);
}

MapLight* GameMap::getMapLight(int index)
{
	return mapLights[index];
}

MapLight* GameMap::getMapLight(string name)
{
	for(unsigned int i = 0; i < mapLights.size(); i++)
	{
		if(mapLights[i]->getName() == name)
			return mapLights[i];
	}

	return NULL;
}

unsigned int GameMap::numMapLights()
{
	return mapLights.size();
}

/** \brief A simple mutator method to clear the vector of empty Seats stored in the GameMap.
  *
*/ 
void GameMap::clearEmptySeats()
{
	for(unsigned int i = 0; i < numEmptySeats(); i++)
		delete emptySeats[i];

	emptySeats.clear();
}

/** \brief A simple mutator method to add another empty Seat to the GameMap.
  *
*/ 
void GameMap::addEmptySeat(Seat *s)
{
	emptySeats.push_back(s);

	// Add the goals for all seats to this seat.
	for(unsigned int i = 0; i < numGoalsForAllSeats(); i++)
		s->addGoal(getGoalForAllSeats(i));
}

/** \brief A simple accessor method to return the given Seat.
  *
*/ 
Seat* GameMap::getEmptySeat(int index)
{
	return emptySeats[index];
}

/** \brief Removes the first empty Seat from the GameMap and returns a pointer to it, this is used when a Player "sits down" at the GameMap.
  *
*/ 
Seat* GameMap::popEmptySeat()
{
	Seat *s = NULL;
	if(emptySeats.size() > 0)
	{
		s = emptySeats[0];
		emptySeats.erase(emptySeats.begin());
		filledSeats.push_back(s);
	}

	return s;
}

/** \brief A simple accessor method to return the number of empty Seats on the GameMap.
  *
*/ 
unsigned int GameMap::numEmptySeats()
{
	return emptySeats.size();
}

void GameMap::clearFilledSeats()
{
	for(unsigned int i = 0; i < numFilledSeats(); i++)
		delete filledSeats[i];

	filledSeats.clear();
}

void GameMap::addFilledSeat(Seat *s)
{
	filledSeats.push_back(s);

	// Add the goals for all seats to this seat.
	for(unsigned int i = 0; i < numGoalsForAllSeats(); i++)
		s->addGoal(getGoalForAllSeats(i));
}

Seat* GameMap::getFilledSeat(int index)
{
	return filledSeats[index];
}

Seat* GameMap::popFilledSeat()
{
	Seat *s = NULL;
	if(filledSeats.size() > 0)
	{
		s = filledSeats[0];
		filledSeats.erase(filledSeats.begin());
		emptySeats.push_back(s);
	}

	return s;
}

unsigned int GameMap::numFilledSeats()
{
	return filledSeats.size();
}

Seat* GameMap::getSeatByColor(int color)
{
	for(unsigned int i = 0; i < filledSeats.size(); i++)
	{
		if(filledSeats[i]->color == color)
			return filledSeats[i];
	}

	for(unsigned int i = 0; i < emptySeats.size(); i++)
	{
		if(emptySeats[i]->color == color)
			return emptySeats[i];
	}

	return NULL;
}

void GameMap::addWinningSeat(Seat *s)
{
	// Make sure the seat has not already been added.
	for(unsigned int i = 0; i < winningSeats.size(); i++)
	{
		if(winningSeats[i] == s)
			return;
	}

	winningSeats.push_back(s);
}

Seat* GameMap::getWinningSeat(unsigned int index)
{
	return winningSeats[index];
}

unsigned int GameMap::getNumWinningSeats()
{
	return winningSeats.size();
}

bool GameMap::seatIsAWinner(Seat *s)
{
	bool isAWinner = false;
	for(unsigned int i = 0; i < getNumWinningSeats(); i++)
	{
		if(getWinningSeat(i) == s)
		{
			isAWinner = true;
			break;
		}
	}

	return isAWinner;
}

void GameMap::addGoalForAllSeats(Goal *g)
{
	goalsForAllSeats.push_back(g);

	// Add the goal to each of the empty seats currently in the game.
	for(unsigned int i = 0; i < numEmptySeats(); i++)
		emptySeats[i]->addGoal(g);

	// Add the goal to each of the filled seats currently in the game.
	for(unsigned int i = 0; i < numFilledSeats(); i++)
		filledSeats[i]->addGoal(g);
}

Goal* GameMap::getGoalForAllSeats(unsigned int i)
{
	return goalsForAllSeats[i];
}

unsigned int GameMap::numGoalsForAllSeats()
{
	return goalsForAllSeats.size();
}

void GameMap::clearGoalsForAllSeats()
{
	goalsForAllSeats.clear();

	// Add the goal to each of the empty seats currently in the game.
	for(unsigned int i = 0; i < numEmptySeats(); i++)
	{
		emptySeats[i]->clearGoals();
		emptySeats[i]->clearCompletedGoals();
	}

	// Add the goal to each of the filled seats currently in the game.
	for(unsigned int i = 0; i < numFilledSeats(); i++)
	{
		filledSeats[i]->clearGoals();
		filledSeats[i]->clearCompletedGoals();
	}
}

double GameMap::crowDistance(int x1, int x2, int y1, int y2)
{
	const double badValue = -1.0;
	double distance;
	Tile *t1, *t2;

	t1 = getTile(x1, y1);
	if(t1 != NULL)
	{
		t2 = getTile(x2, y2);
		if(t2 != NULL)
		{
			int xDist, yDist;
			xDist = t2->x - t1->x;
			yDist = t2->y - t1->y;
			distance = xDist*xDist + yDist*yDist;
			distance = sqrt(distance);
			return distance;
		}
		else
		{
			return badValue;
		}
	}
	else
	{
		return badValue;
	}

}

int GameMap::uniqueFloodFillColor()
{
	nextUniqueFloodFillColor++;
	return nextUniqueFloodFillColor;
}

unsigned int GameMap::doFloodFill(int startX, int startY, Tile::TileClearType passability, int color)
{
	unsigned int tilesFlooded = 1;

	if(!floodFillEnabled)
		return 0;

	if(color < 0)
		color = uniqueFloodFillColor();

	// Check to see if we should color the current tile.
	Tile *tempTile = gameMap.getTile(startX, startY);
	if(tempTile != NULL)
	{
		// If the tile is walkable, color it.
		//FIXME:  This should be improved to use the "passability" parameter.
		if(tempTile->getTilePassability() == Tile::walkableTile)
			tempTile->floodFillColor = color;
		else
			return 0;
	}

	// Get the current tile's neighbors, loop over each of them.
	vector<Tile*> neighbors = gameMap.neighborTiles(startX, startY);
	for(unsigned int i = 0; i < neighbors.size(); i++)
	{
		if(neighbors[i]->floodFillColor != color)
		{
			tilesFlooded += doFloodFill(neighbors[i]->x, neighbors[i]->y, passability, color);
		}
	}

	return tilesFlooded;
}

void GameMap::disableFloodFill()
{
	floodFillEnabled = false;
}

void GameMap::enableFloodFill()
{
	Tile *tempTile;

	// Carry out a flood fill of the whole level to make sure everything is good.
	// Start by setting the flood fill color for every tile on the map to -1.
	sem_wait(&tilesLockSemaphore);
	map< pair<int,int>, Tile*>::iterator currentTile = tiles.begin();
	while(currentTile != tiles.end())
	{
		tempTile = currentTile->second;
		tempTile->floodFillColor = -1;
		currentTile++;
	}
	sem_post(&tilesLockSemaphore);

	// Loop over the tiles again, this time flood filling when the flood fill color is -1.  This will flood the map enough times to cover the whole map.

	//TODO:  The looping construct here has a potential race condition in that the endTile could change between the time when it is initialized and the end of this loop.  If this happens the loop could continue infinitely.
	floodFillEnabled = true;
	sem_wait(&tilesLockSemaphore);
	currentTile = tiles.begin();
	map< pair<int,int>, Tile*>::iterator endTile = tiles.end();
	sem_post(&tilesLockSemaphore);
	while(currentTile != endTile)
	{
		tempTile = currentTile->second;
		if(tempTile->floodFillColor == -1)
			doFloodFill(tempTile->x, tempTile->y);

		currentTile++;
	}
}

list<Tile*> GameMap::path(Creature *c1, Creature *c2, Tile::TileClearType passability)
{
	return path(c1->positionTile()->x, c1->positionTile()->y, c2->positionTile()->x, c2->positionTile()->y, passability);
}

list<Tile*> GameMap::path(Tile *t1, Tile *t2, Tile::TileClearType passability)
{
	return path(t1->x, t1->y, t2->x, t2->y, passability);
}

double GameMap::crowDistance(Creature *c1, Creature *c2)
{
	//TODO:  This is sub-optimal, improve it.
	Tile *tempTile1 = c1->positionTile(), *tempTile2 = c2->positionTile();
	return crowDistance(tempTile1->x, tempTile1->y, tempTile2->x, tempTile2->y);
}

void GameMap::threadLockForTurn(long int turn)
{
	unsigned int tempUnsigned;

	// Lock the thread reference count map to prevent race conditions.
	sem_wait(&threadReferenceCountLockSemaphore);

	map<long int, ProtectedObject<unsigned int> >::iterator result = threadReferenceCount.find(turn);
	if(result != threadReferenceCount.end())
	{
		(*result).second.lock();
		tempUnsigned = (*result).second.rawGet();
		tempUnsigned++;
		(*result).second.rawSet(tempUnsigned);
		(*result).second.unlock();
	}
	else
	{
		threadReferenceCount[turn].rawSet(1);
	}

	// Unlock the thread reference count map.
	sem_post(&threadReferenceCountLockSemaphore);
}

void GameMap::threadUnlockForTurn(long int turn)
{
	unsigned int tempUnsigned;

	// Lock the thread reference count map to prevent race conditions.
	sem_wait(&threadReferenceCountLockSemaphore);

	map<long int, ProtectedObject<unsigned int> >::iterator result = threadReferenceCount.find(turn);
	if(result != threadReferenceCount.end())
	{
		(*result).second.lock();
		tempUnsigned = (*result).second.rawGet();
		tempUnsigned--;
		(*result).second.rawSet(tempUnsigned);
		(*result).second.unlock();
	}
	else
	{
		cout << "\n\n\nERROR:  Calling threadUnlockForTurn on a turn number which does not have any current locks, bailing out.\n\n\n";
		exit(1);
	}

	// Unlock the thread reference count map.
	sem_post(&threadReferenceCountLockSemaphore);
}

void GameMap::processDeletionQueues()
{
	long int turn = turnNumber.get();

	cout << "\nProcessing deletion queues on turn " << turn << ":\n";
	long int latestTurnToBeRetired = -1;

	// Lock the thread reference count map to prevent race conditions.
	sem_wait(&threadReferenceCountLockSemaphore);

	// Loop over the thread reference count and find the first turn number which has 0 outstanding threads holding references for that turn.
	map<long int, ProtectedObject<unsigned int> >::iterator currentThreadReferenceCount = threadReferenceCount.begin();
	while(currentThreadReferenceCount != threadReferenceCount.end())
	{
		cout << "(" << (*currentThreadReferenceCount).first << ", " << (*currentThreadReferenceCount).second.rawGet() << ")   ";
		if((*currentThreadReferenceCount).second.get() == 0)
		{
			// There are no threads which could be holding references to objects from the current turn so it is safe to retire.
			latestTurnToBeRetired = (*currentThreadReferenceCount).first;
			map<long int, ProtectedObject<unsigned int> >::iterator tempIterator = currentThreadReferenceCount++; 
			threadReferenceCount.erase(tempIterator);
		}
		else
		{
			// There is one or more threads which could still be holding references to objects from the current turn so we cannot retire it.
			break;
		}
	}

	// Unlock the thread reference count map.
	sem_post(&threadReferenceCountLockSemaphore);
	
	cout << "\nThe latest turn to be retired is " << latestTurnToBeRetired;

	// If we did not find any turns which have no threads locking them we are safe to retire this turn.
	if(latestTurnToBeRetired < 0)
		return;

	// Loop over the creaturesToDeleteMap and delete all the creatures in any mapped vector whose
	// key value (the turn those creatures were added) is less than the latestTurnToBeRetired.
	map<long int, vector<Creature*> >::iterator currentTurnForCreatureRetirement = creaturesToDelete.begin();
	while(currentTurnForCreatureRetirement != creaturesToDelete.end() && (*currentTurnForCreatureRetirement).first <= latestTurnToBeRetired)
	{
		long int currentTurnToRetire = (*currentTurnForCreatureRetirement).first;

		// Check to see if any creatures can be deleted.
		while(creaturesToDelete[currentTurnToRetire].size() > 0)
		{
			cout << "\nSending message to delete creature " << (*creaturesToDelete[currentTurnToRetire].begin())->name;
			cout.flush();

			(*creaturesToDelete[currentTurnToRetire].begin())->deleteYourself();
			creaturesToDelete[currentTurnToRetire].erase(creaturesToDelete[currentTurnToRetire].begin());
		}

		currentTurnForCreatureRetirement++;
	}
}

