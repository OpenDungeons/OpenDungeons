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

			sprintf(tempString, "Level_%3i_%3i", i, j);
			tempTile->name = tempString;
			tempTile->createMesh();
			//tiles.push_back(tempTile);
			tiles.insert( pair< pair<int,int>, Tile* >(pair<int,int>(i,j), tempTile) );
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

Tile* GameMap::getTile(int x, int y)
{
	pair<int,int> location(x, y);
	TileMap_t::iterator itr = tiles.find(location);
	if(itr != tiles.end())
		return itr->second;
	else
		return NULL;

	/*
	for(unsigned int i = 0; i < tiles.size(); i++)
	{
		if(tiles[i]->x == x && tiles[i]->y == y)
		{
			return tiles[i];
		}
	}
	*/

	//cerr << "Error: Tile not found: (" << x << ", " << y << ")\n\n\n";
	return NULL;
}

/*
//FIXME:  looping from over this function is now quadratic rather than linear and this should be fixed
Tile* GameMap::getTile(int index)
{
	//return tiles[index];
	TileMap_t::iterator itr = tiles.begin();
	for(int i = 0; i < index; i++)
		itr++;

	return itr->second;
}
*/

void GameMap::clearAll()
{
	clearTiles();
	clearCreatures();
	clearClasses();
	clearPlayers();
}

void GameMap::clearTiles()
{
	TileMap_t::iterator itr = tiles.begin();
	while(itr != tiles.end())
	{
		itr->second->deleteYourself();
		itr++;
	}

	tiles.clear();
}

void GameMap::clearCreatures()
{
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		creatures[i]->deleteYourself();
	}

	creatures.clear();

}

void GameMap::clearClasses()
{
	for(unsigned int i = 0; i < numClassDescriptions(); i++)
	{
		delete classDescriptions[i];
	}

	classDescriptions.clear();
}

void GameMap::clearPlayers()
{
	for(unsigned int i = 0; i < numPlayers(); i++)
	{
		delete players[i];
	}

	players.clear();
}

unsigned int GameMap::numTiles()
{
	return tiles.size();
}

void GameMap::addTile(Tile *t)
{
	tiles.insert( pair< pair<int,int>, Tile* >(pair<int,int>(t->x,t->y), t) );
	//tiles.push_back(t);
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

Creature* GameMap::getClassDescription(string query)
{
	for(unsigned int i = 0; i < classDescriptions.size(); i++)
	{
		if(classDescriptions[i]->className.compare(query) == 0)
			return classDescriptions[i];
	}

	return NULL;
}

unsigned int GameMap::numCreatures()
{
	return creatures.size();
}

unsigned int GameMap::numClassDescriptions()
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
	TileMap_t::iterator itr = tiles.begin();
	while(itr != tiles.end())
	{
		itr->second->createMesh();
		itr++;
	}

	// Create OGRE entities for the creatures
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		Creature *currentCreature = getCreature(i);

		currentCreature->createMesh();
	}
}

Creature* GameMap::getCreature(string cName)
{
	for(unsigned int i = 0; i < numCreatures(); i++)
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
	cout << "\nStarting creature AI for turn " << turnNumber;
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		cout << "\nCreature " << i << "  " << creatures[i]->name << " calling doTurn.\t";
		creatures[i]->doTurn();
	}
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

		// Check the tiles surrounding the current square
		vector<Tile*>neighbors = neighborTiles(currentEntry->tile->x, currentEntry->tile->y);
		for(unsigned int i = 0; i < neighbors.size(); i++)
		{
			neighbor->tile = neighbors[i];

			// See if the neighbor tile in question is walkable and empty
			// The "numCreaturesInCell" comaprison sets the max creatures in a cell at a time.
			// note:  this does not prevent two creatures from both walking into an empty cell
			// during the same turn, only one creature moving into the cell occupied by another.
			if(neighbor->tile != NULL && neighbor->tile->getFullness() == 0)
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
						// NOTE: This +1 weights all steps the same, diagonal steps
						// should get a greater wieght iis they are included in the future
						neighbor->g = currentEntry->g + 1;

						// Use the manhattan distance for the heuristic
						// FIXME:  This is not the only place the heuristic is calculated
						neighbor->h = fabs(x2-neighbor->tile->x) + fabs(y2-neighbor->tile->y);
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

TileMap_t::iterator GameMap::firstTile()
{
	return tiles.begin();
}

TileMap_t::iterator GameMap::lastTile()
{
	return tiles.end();
}

vector<Tile*> GameMap::neighborTiles(int x, int y)
{
	Tile *t = getTile(x, y);
	Tile *neighbor;
	vector<Tile*> neighbors;

	if(t != NULL)
	{
		for(int i = 0; i < 4; i++)
		{
			int tempX, tempY;

			tempX = x;
			tempY = y;
			switch(i)
			{
				// Adjacent neighbors
				case 0: tempX -= 0;  tempY += 1;  break;
				case 1: tempX -= 0;  tempY -= 1;  break;
				case 2: tempX -= 1;  tempY += 0;  break;
				case 3: tempX += 1;  tempY += 0;  break;

				default:
					cout << "\n\n\nERROR:  Wrong neighbor index in astar search.\n\n\n";
					exit(1);
					break;
			}

			neighbor = getTile(tempX, tempY);
			if(neighbor != NULL)
				neighbors.push_back(neighbor);
		}
	}

	return neighbors;
}

void GameMap::addPlayer(Player *p)
{
	players.push_back(p);
}

Player* GameMap::getPlayer(int index)
{
	return players[index];
}

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

unsigned int GameMap::numPlayers()
{
	return players.size();
}

list<Tile*> GameMap::lineOfSight(int x0, int y0, int x1, int y1)
{
	list<Tile*> path;

	int Dx = x1 - x0;
	int Dy = y1 - y0;
	int steep = (abs(Dy) >= abs(Dx));
	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
		// recompute Dx, Dy after swap
		Dx = x1 - x0;
		Dy = y1 - y0;
	}
	int xstep = 1;
	if (Dx < 0)
	{
		xstep = -1;
		Dx = -Dx;
	}
	int ystep = 1;
	if (Dy < 0)
	{
		ystep = -1;
		Dy = -Dy;
	}
	int TwoDy = 2*Dy;
	int TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
	int E = TwoDy - Dx; //2*Dy - Dx
	int y = y0;
	int xDraw, yDraw;
	for (int x = x0; x != x1; x += xstep)
	{
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

		// next
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

bool GameMap::pathIsClear(list<Tile*> path)
{
	list<Tile*>::iterator itr;

	// Loop over each space and check to see if it is clear
	bool isClear = true;
	for(itr = path.begin(); itr != path.end() && isClear; itr++)
	{
		isClear = (isClear && (*itr)->getFullness() <= 1.0);
	}

	return isClear;
}

void GameMap::cutCorners(list<Tile*> &path)
{
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
	while(t1 != secondLast && t1 != path.end()) 
	{
		// Loop t2 from t1 until the end of the path
		t2 = t1;
		t2++;
		while(t2 != path.end())
		{
			// If we have a clear line of sight to t2, advance to
			// the next tile else break out of the inner loop
			if( pathIsClear( lineOfSight( (*t1)->x, (*t1)->y, (*t2)->x, (*t2)->y )) )
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
				path.erase(t3, t4);
		}

		t1 = t2;

		secondLast = path.end();
		secondLast--;
	}
}

