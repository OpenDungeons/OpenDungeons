#include <iostream>
#include <algorithm>
using namespace std;

#include <math.h>

#include "Functions.h"
#include "Defines.h"
#include "GameMap.h"

/*! \brief Erase all creatures, tiles, etc. from the map and make a new rectangular one.
 *
 * The new map consists entirely of the same kind of tile, with no creature
 * classes loaded, no players, and no creatures.
 */
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

/*! \brief Returns a pointer to the tile at location (x, y).
 *
 * The tile pointers are stored internally in a map so calls to this function
 * have a complexity O(log(N)) where N is the number of tiles in the map.
 */
Tile* GameMap::getTile(int x, int y)
{
	pair<int,int> location(x, y);
	TileMap_t::iterator itr = tiles.find(location);
	if(itr != tiles.end())
		return itr->second;
	else
		return NULL;
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles, creatures, classes, and players in the GameMap.
 *
 */
void GameMap::clearAll()
{
	clearTiles();
	clearCreatures();
	clearClasses();
	clearPlayers();
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles in the GameMap.
 *
 */
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

/*! \brief Clears the mesh and deletes the data structure for all the creatures in the GameMap.
 *
 */
void GameMap::clearCreatures()
{
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		creatures[i]->deleteYourself();
	}

	creatures.clear();

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
	return tiles.size();
}

/*! \brief Adds the address of a new tile to be stored in this GameMap.
 *
 */
void GameMap::addTile(Tile *t)
{
	tiles.insert( pair< pair<int,int>, Tile* >(pair<int,int>(t->x,t->y), t) );
	//tiles.push_back(t);
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
void GameMap::addClassDescription(Creature *c)
{
	classDescriptions.push_back( c );
}

/*! \brief Copies the creature class structure into a newly created structure and stores the address of the new structure in this GameMap.
 *
 */
void GameMap::addClassDescription(Creature c)
{
	classDescriptions.push_back( new Creature(c) );
}

/*! \brief Adds the address of a new creature to be stored in this GameMap.
 *
 */
void GameMap::addCreature(Creature *c)
{
	creatures.push_back(c);
}

/*! \brief Removes the creature from the game map but does not delete its data structure.
 *
 */
void GameMap::removeCreature(Creature *c)
{
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		if(c == creatures[i])
		{
			creatures.erase(creatures.begin()+i);
			return;
		}
	}
}

/*! \brief Returns a pointer to the first class description whose 'name' parameter matches the query string.
 *
 */
Creature* GameMap::getClassDescription(string query)
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
	return creatures.size();
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
	return creatures[index];
}

/*! \brief Gets the i'th class description in this GameMap.
 *
 */
Creature* GameMap::getClassDescription(int index)
{
	return classDescriptions[index];
}

/*! \brief Creates meshes for all the tiles and creatures stored in this GameMap.
 *
 */
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

/*! \brief Returns a pointer to the creature whose name matches cName.
 *
 */
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

/*! \brief Loops over all the creatures and calls their individual doTurn methods.
 *
 */
void GameMap::doTurn()
{
	sem_wait(&creatureAISemaphore);

	cout << "\nStarting creature AI for turn " << turnNumber;
	for(unsigned int i = 0; i < numCreatures(); i++)
	{
		//cout << "\nCreature " << i << "  " << creatures[i]->name << " calling doTurn.\t";
		creatures[i]->doTurn();
	}

	sem_post(&creatureAISemaphore);
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
	list<Tile*> returnList;
	astarEntry *currentEntry;
	Tile *destination;
	//TODO:  make this a local variable, don't forget to remove the delete statement at the end of this function.
	astarEntry *neighbor = new astarEntry;
	list<astarEntry*> openList;
	list<astarEntry*> closedList;
	list<astarEntry*>::iterator itr;

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
						cout << "\n\nERROR:  Trying to find a path through impassable tiles in GameMap::path()\n\n";
						exit(1);
						break;

					default:
						cout << "\n\nERROR:  Unhandled tile type in GameMap::path()\n\n";
						exit(1);
						break;
				}

				if(processNeighbor)
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
	return tiles.begin();
}

/*! \brief Returns an iterator to be used for the purposes of looping over the tiles stored in this GameMap.
 *
 */
TileMap_t::iterator GameMap::lastTile()
{
	return tiles.end();
}

/*! \brief Returns the (up to) 4 nearest neighbor tiles of the tile located at (x, y).
 *
 */
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

/*! \brief Adds a pointer to a player structure to the players stored by this GameMap.
 *
 */
void GameMap::addPlayer(Player *p)
{
	players.push_back(p);
}

/*! \brief Returns a pointer to the i'th player structure stored by this GameMap.
 *
 */
Player* GameMap::getPlayer(int index)
{
	return players[index];
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
				cout << "\n\nERROR:  Unhandled tile type in GameMap::pathIsClear()\n\n";
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

	//cout << "\n\nStarting cutCorners\nCurrentPath is:  ";
	for(list<Tile*>::iterator itr = path.begin(); itr != path.end(); itr++)
	{
		cout << "  (" << (*itr)->x << ", " << (*itr)->y << ") ";
	}

	// Loop t1 over all but the last tile in the path
	while(t1 != path.end()) 
	{
		// Loop t2 from t1 until the end of the path
		t2 = t1;
		t2++;
		//cout << "\nouterloop started t1:  (" << (*t1) << ")\tt2:  (" << (*t2) << ")";
		while(t2 != path.end())
		{
			// If we have a clear line of sight to t2, advance to
			// the next tile else break out of the inner loop
			//cout << "\ninnerloop started t1:  (" << (*t1) << ")\tt2:  (" << (*t2) << ")";
			list<Tile*> lineOfSightPath = lineOfSight( (*t1)->x, (*t1)->y, (*t2)->x, (*t2)->y );

			/*
			cout << "\n\nLine of sight path is:  ";
			for(list<Tile*>::iterator itr = lineOfSightPath.begin(); itr != lineOfSightPath.end(); itr++)
			{
				cout << "  (" << (*itr) << ") ";
			}
			*/


			if( pathIsClear( lineOfSightPath, passability)  )
				t2++;
			else
				break;
		}
		//cout << "\nbroke out of loop.";

		// Delete the tiles 'strictly between' t1 and t2
		t3 = t1;
		t3++;
		if(t3 != t2)
		{
			t4 = t2;
			t4--;
			if(t3 != t4)
			{
				//cout << "\nerasing  (" << (*t3) << ") to (" << (*t4) << ")";
				path.erase(t3, t4);
			}
		}

		t1 = t2;

		secondLast = path.end();
		secondLast--;

		//cout << "\nLoop finished\nCurrentPath is:  ";
		for(list<Tile*>::iterator itr = path.begin(); itr != path.end(); itr++)
		{
			cout << "\n(" << (*itr) << ")";
		}
	}
}

