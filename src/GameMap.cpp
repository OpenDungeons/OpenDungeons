#include <iostream>
#include <algorithm>
using namespace std;

#include <math.h>

#include "Functions.h"
#include "Defines.h"
#include "GameMap.h"

void GameMap::createNewMap(int xSize, int ySize)
{
	Tile *tempTile;
	char tempString[255];

	clearTiles();

	for(int j = 0; j < ySize; j++)
	{
		for(int i = 0; i < xSize; i++)
		{
			tempTile = new Tile;
			tempTile->setType(Tile::dirt);
			tempTile->setFullness(100);
			tempTile->x = i;
			tempTile->y = j;

			sprintf(tempString, "Level_%3i_%3i", i, j);
			tempTile->name = tempString;
			tempTile->createMesh();
			tiles.push_back(tempTile);
		}
	}
}

Tile* GameMap::getTile(int x, int y)
{
	for(unsigned int i = 0; i < tiles.size(); i++)
	{
		if(tiles[i]->x == x && tiles[i]->y == y)
		{
			return tiles[i];
		}
	}

	//cerr << "Error: Tile not found: (" << x << ", " << y << ")\n\n\n";
	return NULL;
}

Tile* GameMap::getTile(int index)
{
	return tiles[index];
}

void GameMap::clearAll()
{
	clearTiles();
	clearCreatures();
	clearClasses();
}

void GameMap::clearTiles()
{
	for(int i = 0; i < numTiles(); i++)
	{
		//tiles[i]->destroyMesh();
		tiles[i]->deleteYourself();
	}

	tiles.clear();
}

void GameMap::clearCreatures()
{
	for(int i = 0; i < numCreatures(); i++)
	{
		creatures[i]->deleteYourself();
	}

	creatures.clear();

}

void GameMap::clearClasses()
{
	for(int i = 0; i < numClassDescriptions(); i++)
	{
		delete classDescriptions[i];
	}

	classDescriptions.clear();
}

int GameMap::numTiles()
{
	return tiles.size();
}

void GameMap::addTile(Tile *t)
{
	tiles.push_back(t);
}

void GameMap::addClassDescription(Creature *c)
{
	classDescriptions.push_back( c );
}

void GameMap::addClassDescription(Creature c)
{
	classDescriptions.push_back( new Creature(c) );
}

void GameMap::addCreature(Creature *c)
{
	creatures.push_back(c);
}

Creature* GameMap::getClass(string query)
{
	for(unsigned int i = 0; i < classDescriptions.size(); i++)
	{
		if(classDescriptions[i]->className.compare(query) == 0)
			return classDescriptions[i];
	}

	return NULL;
}

int GameMap::numCreatures()
{
	return creatures.size();
}

int GameMap::numClassDescriptions()
{
	return classDescriptions.size();
}

Creature* GameMap::getCreature(int index)
{
	return creatures[index];
}

Creature* GameMap::getClassDescription(int index)
{
	return classDescriptions[index];
}


void GameMap::createAllEntities()
{
	// Create OGRE entities for map tiles
	for(int i = 0; i < numTiles(); i++)
	{
		getTile(i)->createMesh();
	}

	// Create OGRE entities for the creatures
	for(int i = 0; i < numCreatures(); i++)
	{
		Creature *currentCreature = getCreature(i);

		currentCreature->createMesh();
	}
}

Creature* GameMap::getCreature(string cName)
{
	for(int i = 0; i < numCreatures(); i++)
	{
		if(creatures[i]->name.compare(cName) == 0)
		{
			return creatures[i];
		}
	}

	return NULL;
}

void GameMap::doTurn()
{
	for(int i = 0; i < numCreatures(); i++)
	{
		creatures[i]->doTurn();
	}
}

int GameMap::numCreaturesInHand()
{
	return creaturesInHand.size();
}

Creature* GameMap::getCreatureInHand(int i)
{
	return creaturesInHand[i];
}

void GameMap::addCreatureToHand(Creature *c)
{
	creaturesInHand.push_back(c);
}

void GameMap::removeCreatureFromHand(int i)
{
	vector<Creature*>::iterator curCreature = creaturesInHand.begin();
	while(i > 0 && curCreature != creaturesInHand.end())
	{
		i--;
		curCreature++;
	}

	creaturesInHand.erase(curCreature);
}

// Calculates the walkable path between tiles (x1, y1) and (x2, y2)
// The path returned contains both the starting and ending tiles
list<Tile*> GameMap::path(int x1, int y1, int x2, int y2)
{
	list<Tile*> returnList;
	astarEntry *currentEntry;
	Tile *destination;
	astarEntry *neighbor = new astarEntry;
	list<astarEntry*> openList;
	list<astarEntry*> closedList;

	currentEntry = new astarEntry;
	currentEntry->tile = getTile(x1, y1);

	// If the start tile was not found return an empty path
	if(currentEntry->tile == NULL)
		return returnList;

	// If the end tile was not found return an empty path
	destination = getTile(x2, y2);
	if(destination == NULL)
		return returnList;

	currentEntry->parent = NULL;
	currentEntry->g = 0.0;
	// Use the manhattan distance for the heuristic
	// FIXME:  This is not the only place the heuristic is calculated
	currentEntry->h = fabs(x2-x1) + fabs(y2-y1);
	openList.push_back(currentEntry);

	bool pathFound = false;
	while(true)
	{
		//cout << "\n\nStuck in this loop..." << openList.size();
		//cout.flush();


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

		// Check the tiles surrounding the current square, the squares are numbered like
		//
		//            4    0    6
		//            2    C    3
		//            7    1    5
		//
		// The loop should go to 4 or 8 depending on whether or not you want the creatures
		// to be able to cut corners.  Setting it to 4 does not allow cutting corners, 8 does.
		for(int i = 0; i < 4; i++)
		{
			int tempX, tempY;
			double nDist;

			tempX = currentEntry->tile->x;
			tempY = currentEntry->tile->y;
			switch(i)
			{
				// Adjacent neighbors
				case 0: tempX -= 0;  tempY += 1;  nDist = 1;  break;
				case 1: tempX -= 0;  tempY -= 1;  nDist = 1;  break;
				case 2: tempX -= 1;  tempY += 0;  nDist = 1;  break;
				case 3: tempX += 1;  tempY += 0;  nDist = 1;  break;

				// Corner neighbors
				case 4: tempX -= 1;  tempY += 1;  nDist = sqrt(2.0);  break;
				case 5: tempX += 1;  tempY -= 1;  nDist = sqrt(2.0);  break;
				case 6: tempX += 1;  tempY += 1;  nDist = sqrt(2.0);  break;
				case 7: tempX -= 1;  tempY -= 1;  nDist = sqrt(2.0);  break;

				default:
					cout << "\n\n\nERROR:  Wrong neighbor index in astar search.\n\n\n";
					exit(1);
					break;
			}

			neighbor->tile = getTile(tempX, tempY);
			//cout << endl << tempX << "\t" << tempY;
			//cout.flush();


			//cout << "\n\nF:  " << neighbor->tile->getFullness() << Tile::tileTypeToString(neighbor->tile->getType()) << endl;
			//cout.flush();

			// See if the neighbor tile in question is walkable and empty
			// The "numCreaturesInCell" comaprison sets the max creatures in a cell at a time.
			// note:  this does not prevent two creatures from both walking into an empty cell
			// during the same turn, only one creature moving into the cell occupied by another.
			//FIXME: this creature collision avoidance needs to be updated.  it will cause weird behaviour.
			if(neighbor->tile != NULL && neighbor->tile->getFullness() == 0 && !neighbor->tile->numCreaturesInCell() <= 1)
			{
				// See if the neighbor is in the closed list
				bool neighborFound = false;
				list<astarEntry*>::iterator itr = closedList.begin();
				while(itr != closedList.end() && !neighborFound)
				{
					if(neighbor->tile == (*itr)->tile)
						neighborFound = true;
					else
						itr++;
				}

				// Ignore the neighbor if it is on the closed list
				if(!neighborFound)
				{
					// See if the neighbor is in the open list
					neighborFound = false;
					list<astarEntry*>::iterator itr = openList.begin();
					while(itr != openList.end() && !neighborFound)
					{
						if(neighbor->tile == (*itr)->tile)
							neighborFound = true;
						else
							itr++;
					}

					// If the neighbor is not in the open list
					if(!neighborFound)
					{
						neighbor->g = currentEntry->g + nDist;
						// Use the manhattan distance for the heuristic
						// FIXME:  This is not the only place the heuristic is calculated
						neighbor->h = fabs(x2-tempX) + fabs(y2-tempY);
						neighbor->parent = currentEntry;

						openList.push_back(new astarEntry(*neighbor));
					}
					else
					{
						// If this path to the given neighbor tile is a shorter path than the
						// one already given, make this the new parent.
						if(currentEntry->g + nDist < (*itr)->g)
						{
							(*itr)->g = currentEntry->g + nDist;
							(*itr)->parent = currentEntry;
						}
					}

				}
			}
		}
	}

	//cout << "\n\n\n\n\n\nYEAH\n\n\n\n";
	//cout.flush();
	if(pathFound)
	{
		//Find the destination tile in the closed list
		list<astarEntry*>::iterator itr = closedList.begin();
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

	return returnList;
}

