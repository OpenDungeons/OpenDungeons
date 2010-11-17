#include "Defines.h"
#include "Functions.h"
#include "Tile.h"
#include "Globals.h"
#include "Creature.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf _snprintf
#endif

Tile::Tile()
{
	selected = false;
	markedForDigging = false;
	location = Ogre::Vector3(0.0, 0.0, 0.0);
	type = dirt;
	setFullness(100);
	rotation = 0.0;
	color = 0;
	colorDouble = 0.0;
	floodFillColor = -1;
	sem_init(&creaturesInCellLockSemaphore, 0, 1);
	coveringRoom = NULL;
}

Tile::Tile(int nX, int nY, TileType nType, int nFullness)
{
	color = 0;
	colorDouble = 0.0;
	floodFillColor = -1;
	selected = false;
	markedForDigging = false;
	x = nX;
	y = nY;
	setType(nType);
	setFullness(nFullness);
	sem_init(&creaturesInCellLockSemaphore, 0, 1);
	coveringRoom = NULL;
}

/*! \brief A mutator to set the type (rock, claimed, etc.) of the tile.
 *
 * In addition to setting the tile type this function also reloads the new mesh
 * for the tile.
 */
void Tile::setType(TileType t)
{
	// If the type has changed from its previous value we need to see if
	// the mesh should be updated
	 if(t != type)
	 {
		type = t;
		refreshMesh();
	 }
}

/*! \brief An accessor which returns the tile type (rock, claimed, etc.).
 *
 */
Tile::TileType Tile::getType()
{
	 return type;
}

/*! \brief A mutator to change how "filled in" the tile is.
 *
 * Additionally this function reloads the proper mesh to display to the user
 * how full the tile is.  It also determines the orientation of the
 * tile to make corners display correctly.  Both of these tasks are
 * accomplished by setting the fullnessMeshNumber variable which is
 * concatenated to the tile's type to determine the mesh to load, e.g.
 * Rock104.mesh for a rocky tile which has all 4 sides shown because it is an
 * "island" with all four sides visible.  Claimed102.mesh would be a fully
 * filled in tile but only two sides are drawn because it borders full tiles on
 * 2 sides.
 */
void Tile::setFullness(int f)
{
	int oldFullnessMeshNumber = fullnessMeshNumber;
	TileClearType oldTilePassability = getTilePassability();
	int tempInt;

	fullness = f;

	// If the tile was marked for digging and has been dug out, unmark it and set its fullness to 0.
	//FIXME:  If other players have it marked for digging, it will not be unmarked for them, we need to write a "setMarkedForDiggingForAllSeats()".
	if(fullness <= 1 && getMarkedForDigging(gameMap.me) == true)
	{
		setMarkedForDigging(false, gameMap.me);
		fullness = 0.0;
	}

	// If we are a sever, the clients need to be told about the change to the tile's fullness.
	if(serverSocket != NULL)
	{
		try
		{
			// Inform the clients that the fullness has changed.
			ServerNotification *serverNotification = new ServerNotification;
			serverNotification->type = ServerNotification::tileFullnessChange;
			serverNotification->tile = this;

			queueServerNotification(serverNotification);
		}
		catch(bad_alloc&)
		{
			cerr << "\n\nERROR:  bad alloc in Tile::setFullness\n\n";
			exit(1);
		}
	}

	// If the passability has changed we may have opened up new paths on the gameMap.
	if(oldTilePassability != getTilePassability())
	{
		// Do a flood fill to update the contiguous region touching the tile.
		gameMap.doFloodFill(x, y);
	}

	// 		4 0 7		    180    
	// 		2 8 3		270  .  90 
	// 		7 1 5		     0     
	//
	bool fillStatus[9];
	Tile *tempTile = gameMap.getTile(x, y+1);
	fillStatus[0] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
	tempTile = gameMap.getTile(x, y-1);
	fillStatus[1] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
	tempTile = gameMap.getTile(x-1, y);
	fillStatus[2] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
	tempTile = gameMap.getTile(x+1, y);
	fillStatus[3] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;

	int fullNeighbors = 0;
	if(fillStatus[0])	fullNeighbors++;
	if(fillStatus[1])	fullNeighbors++;
	if(fillStatus[2])	fullNeighbors++;
	if(fillStatus[3])	fullNeighbors++;

	//FIXME:  This needs to be updated to reflect the allowable fill states for each tile type
	// This is also where the logic for checking neighboring fullness should go
	fullnessMeshNumber = 0;
	if(f > 0 && f <= 25)	fullnessMeshNumber = 25;
	else if(f > 25 && f <= 50)
	{
		fullnessMeshNumber = 50;
		switch(fullNeighbors)
		{
			case 1:
				fullnessMeshNumber = 51;
				if(fillStatus[0])	{rotation = 270; break;}//correct
				if(fillStatus[1])	{rotation = 90; break;}//correct
				if(fillStatus[2])	{rotation = 0; break;}//correct
				if(fillStatus[3])	{rotation = 180; break;}//correct
				break;

			case 2:
				fullnessMeshNumber = 52;
				if(fillStatus[0] && fillStatus[2])	{rotation = 270; break;}//correct
				if(fillStatus[0] && fillStatus[3])	{rotation = 180; break;}//correct
				if(fillStatus[1] && fillStatus[2])	{rotation = 0; break;}//correct
				if(fillStatus[1] && fillStatus[3])	{rotation = 90; break;}//correct

				//TODO:  These next two options are for when the half full tile is in the middle of a wall, the need a separate mesh to be made.
				if(fillStatus[0] && fillStatus[1])	{fullnessMeshNumber = 51; rotation = 0; break;}//correct
				if(fillStatus[2] && fillStatus[3])	{fullnessMeshNumber = 51; rotation = 90; break;}//correct
				break;
		}
	}

	else if(f > 50 && f <= 75)	
	{
		fullnessMeshNumber = 75;
		switch(fullNeighbors)
		{
			case 1:
				if(fillStatus[0])	{rotation = 270; break;}//correct
				if(fillStatus[1])	{rotation = 90; break;}//correct
				if(fillStatus[2])	{rotation = 0; break;}//correct
				if(fillStatus[3])	{rotation = 180; break;}//correct
				break;
		}
	}
	else if(f > 75)
	{
		switch(fullNeighbors)
		{
			//TODO:  Determine the rotation for each of these case statements
			case 0:
				fullnessMeshNumber = 104;
				rotation = 0;
				break;

			case 1:
				fullnessMeshNumber = 103;
				if(fillStatus[0])	{rotation = 180; break;}//correct
				if(fillStatus[1])	{rotation = 0; break;}//correct
				if(fillStatus[2])	{rotation = 270; break;}//correct
				if(fillStatus[3])	{rotation = 90; break;}//correct
				break;

			case 2:
				tempInt = 0;
				if(fillStatus[0])	tempInt += 1;
				if(fillStatus[1])	tempInt += 2;
				if(fillStatus[2])	tempInt += 4;
				if(fillStatus[3])	tempInt += 8;

				switch(tempInt)
				{
					case 5:
						fullnessMeshNumber = 52;
						rotation = 270;
						break;

					case 6:
						fullnessMeshNumber = 52;
						rotation = 0;
						break;

					case 9:
						fullnessMeshNumber = 52;
						rotation = 180;
						break;

					case 10:
						fullnessMeshNumber = 52;
						rotation = 90;
						break;



					case 3:
						fullnessMeshNumber = 102;
						rotation = 0.0;
						break;

					case 12:
						fullnessMeshNumber = 102;
						rotation = 90.0;
						break;

					default:
					cerr << "\n\nERROR:  Unhandled case statement in Tile::setFullness(), exiting.  tempInt = " << tempInt << "\n\n";
					exit(1);
					break;
				}
				break;

			case 3:
				fullnessMeshNumber = 101;  //this is wrong for now it should be 101
				if(!fillStatus[0])	{rotation = 90; break;}//correct
				if(!fillStatus[1])	{rotation = 270; break;}
				if(!fillStatus[2])	{rotation = 180; break;}
				if(!fillStatus[3])	{rotation = 0; break;}
				break;

			case 4:
				fullnessMeshNumber = 100;
				rotation = 0;
				break;

			default:
				cerr << "\n\nERROR:  fullNeighbors != 0 or 1 or 2 or 3 or 4.  This is impossible, exiting program.\n\n";
				exit(1);
				break;
		}

	}

	// If the mesh has changed it means that a new path may have opened up.
	if(oldFullnessMeshNumber != fullnessMeshNumber)
	{
		refreshMesh();
	}
}

/*! \brief An accessor which returns the tile's fullness which should range from 0 to 100.
 *
 */
int Tile::getFullness()
{
	 return fullness;
}

/*! \brief An accessor which returns the tile's fullness mesh number.
 *
 * The fullness mesh number is concatenated to the tile's type to determine the
 * mesh to load to display a given tile type.
 */
int Tile::getFullnessMeshNumber()
{
	return fullnessMeshNumber;
}

/*! \brief Returns the 'passability' state of a tile (impassableTile, walkableTile, etc.).
 *
 * The passability of a tile indicates what type of creatures may move into the
 * given tile.  As an example, no creatures may move into an 'impassableTile'
 * like Dirt100 and only flying creatures may move into a 'flyableTile' like
 * Lava0.
 */
Tile::TileClearType Tile::getTilePassability()
{
	if(fullness > 0.1)
		return impassableTile;

	switch(type)
	{
		case dirt:
		case gold:
		case rock:
		case claimed:
			return walkableTile;
			break;

		case water:
			return walkableTile;
			break;

		case lava:
			return flyableTile;
			break;

		default:
			cerr << "\n\nERROR:  Unhandled tile type in Tile::getTilePassability()\n\n";
			exit(1);
			break;
	}

	// Return something to make the compiler happy.
	// Control should really never reach here because of the exit(1) call in the default switch case above
	cerr << "\n\nERROR:  Control reached the end of Tile::getTilePassability, this should never actually happen.\n\n";
	exit(1);
	return impassableTile;
}

bool Tile::permitsVision()
{
	TileClearType clearType = getTilePassability();
	if(clearType == walkableTile || clearType == flyableTile)
		return true;
	else
		return false;
}
Room* Tile::getCoveringRoom()
{
	return coveringRoom;
}

void Tile::setCoveringRoom(Room *r)
{
	coveringRoom = r;
}

string Tile::getFormat()
{
        return "posX\tposY\ttype\tfullness";
}

/*! \brief The << operator is used for saving tiles to a file and sending them over the net.
 *
 * This operator is used in conjunction with the >> operator to standardize
 * tile format in the level files, as well as sending tiles over the network.
 */
ostream& operator<<(ostream& os, Tile *t)
{
	os << t->x << "\t" << t->y << "\t" << t->getType() << "\t" << t->getFullness();

	return os;
}

/*! \brief The >> operator is used for loading tiles from a file and for receiving them over the net.
 *
 * This operator is used in conjunction with the << operator to standardize
 * tile format in the level files, as well as sending tiles over the network.
 */
istream& operator>>(istream& is, Tile *t)
{
	int tempInt, xLocation, yLocation;
	char tempCellName[255];

	is >> xLocation >> yLocation;
	t->location = Ogre::Vector3(xLocation, yLocation, 0);
	snprintf(tempCellName, sizeof(tempCellName), "Level_%3i_%3i", xLocation, yLocation);
	t->name = tempCellName;
	t->x = xLocation;
	t->y = yLocation;

	is >> tempInt;
	t->setType( (Tile::TileType) tempInt );

	is >> tempInt;
	t->setFullness(tempInt);

	return is;
}

/*! \brief This is a helper function which just converts the tile type enum into a string.
 *
 * This function is used primarily in forming the mesh names to load from disk
 * for the various tile types.  The name returned by this function is
 * concatenated with a fullnessMeshNumber to form the filename, e.g.
 * Dirt104.mesh is a 4 sided dirt mesh with 100% fullness.
 */
string Tile::tileTypeToString(TileType t)
{
	switch(t)
	{
		default:
		case dirt:
			return "Dirt";
			break;

		case rock:
			return "Rock";
			break;

		case gold:
			return "Gold";
			break;

		case water:
			return "Water";
			break;

		case lava:
			return "Lava";
			break;
			
		case claimed:
			return "Claimed";
			break;
	}
}

/*! \brief This is a helper function to scroll through the list of available tile types.
 *
 * This function is used in the map editor when the user presses the button to
 * select the next tile type to be active in the user interface.  The active
 * tile type is the one which is placed when the user clicks the mouse button.
 */
Tile::TileType Tile::nextTileType(TileType t)
{
	int currentType = (int)t;
	currentType++;
	currentType %= (int)(nullTileType);

	return (TileType) currentType;
}

/*! \brief This is a helper function to scroll through the list of available fullness levels.
 *
 * This function is used in the map editor when the user presses the button to
 * select the next tile fullness level to be active in the user interface.  The
 * active fullness level is the one which is placed when the user clicks the
 * mouse button.
 */
int Tile::nextTileFullness(int f)
{

	// Cycle the tile's fullness through the possible values
	switch(f)
	{
		case 0:
			return 25;
			break;

		case 25:
			return 50;
			break;

		case 50:
			return 75;
			break;

		case 75:
			return 100;
			break;

		case 100:
			return 0;
			break;

		default:
			return 0;
			break;
	}
}

/*! \brief This function puts a message in the renderQueue to change the mesh for this tile.
 *
 */
void Tile::refreshMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::refreshTile;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

/*! \brief This function puts a message in the renderQueue to load the mesh for this tile.
 *
 */
void Tile::createMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createTile;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	//FIXME:  this refreshMesh is a test to see if it fixes the hidden tiles bug at load time.
	refreshMesh();
}

/*! \brief This function puts a message in the renderQueue to unload the mesh for this tile.
 *
 */
void Tile::destroyMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyTile;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

/*! \brief This function marks the tile as being selected through a mouse click or drag.
 *
 */
void Tile::setSelected(bool s)
{
	Entity *ent;
	char tempString[255];
	char tempString2[255];

	//FIXME:  This code should probably only execute if it needs to for speed reasons.
	snprintf(tempString, sizeof(tempString), "Level_%3i_%3i_selection_indicator", x, y);
	if(mSceneMgr->hasEntity(tempString))
	{
		ent = mSceneMgr->getEntity(tempString);
	}
	else
	{
		snprintf(tempString2, sizeof(tempString2), "Level_%3i_%3i_node", x, y);
		SceneNode *tempNode = mSceneMgr->getSceneNode(tempString2);

		ent = mSceneMgr->createEntity(tempString, "SquareSelector.mesh");
		tempNode->attachObject(ent);
		ent->setVisible(false);
	}

	if(selected != s)
	{
		selected = s;

		if(selected)
		{
			ent->setVisible(true);
		}
		else
		{
			ent->setVisible(false);
		}
	}
}

/*! \brief This accessor function returns whether or not the tile has been selected.
 *
 */
bool Tile::getSelected()
{
	return selected;
}

/*! \brief This function marks the tile to be dug out by workers, and displays the dig indicator on it.
 *
 */
void Tile::setMarkedForDigging(bool s, Player *p)
{
	// If we are trying to mark a tile that is not dirt or gold, ignore the request.
	if(s && (type != dirt && type != gold))
		return;

	// If we are trying to mark a tile that is already dug out, ignore the request.
	if(s && (getFullness() <= 0))
		return;

	Entity *ent;
	char tempString[255];
	char tempString2[255];

	if(getMarkedForDigging(p) != s)
	{
		bool thisRequestIsForMe = (p == gameMap.me);
		if(thisRequestIsForMe)
		{
			//FIXME:  This code should probably only execute if it needs to for speed reasons.
			//FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest
			snprintf(tempString, sizeof(tempString), "Level_%i_%i_selection_indicator", x, y);
			if(mSceneMgr->hasEntity(tempString))
			{
				ent = mSceneMgr->getEntity(tempString);
			}
			else
			{
				snprintf(tempString2, sizeof(tempString2), "Level_%3i_%3i_node", x, y);
				SceneNode *tempNode = mSceneMgr->getSceneNode(tempString2);

				ent = mSceneMgr->createEntity(tempString, "DigSelector.mesh");
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
				ent->setNormaliseNormals(true);
#endif
				tempNode->attachObject(ent);
			}
		}

		if(s)
		{
			//FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest
			if(thisRequestIsForMe)
			{
				ent->setVisible(true);
			}
			addPlayerMarkingTile(p);
		}
		else
		{
			//FIXME:  This code should be moved over to the rendering thread and called via a RenderRequest
			if(thisRequestIsForMe)
			{
				ent->setVisible(false);
			}
			removePlayerMarkingTile(p);
		}
	}
}

/*! \brief This accessor function returns whether or not the tile has been marked to be dug out by a given Player p.
 *
 */
bool Tile::getMarkedForDigging(Player *p)
{
	bool isMarkedForDigging = false;

	// Loop over any players who have marked this tile and see if 'p' is one of them
	for(unsigned int i = 0; i < playersMarkingTile.size(); i++)
	{
		if(playersMarkingTile[i] == p)
		{
			return true;
		}
	}

	return isMarkedForDigging;
}

/*! \brief This function places a message in the render queue to unload the mesh and delete the tile structure.
 *
 */
void Tile::deleteYourself()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyTile;
	request->p = this;

	RenderRequest *request2 = new RenderRequest;
	request2->type = RenderRequest::deleteTile;
	request2->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
	queueRenderRequest(request2);
}

/*! \brief This function adds a creature to the list of creatures in this tile.
 *
 */
void Tile::addCreature(Creature *c)
{
	sem_wait(&creaturesInCellLockSemaphore);
	creaturesInCell.push_back(c);
	sem_post(&creaturesInCellLockSemaphore);
}

/*! \brief This function removes a creature to the list of creatures in this tile.
 *
 */
void Tile::removeCreature(Creature *c)
{
	sem_wait(&creaturesInCellLockSemaphore);

	// Check to see if the given crature is actually in this tile
	std::vector<Creature*>::iterator itr;
	for(itr = creaturesInCell.begin(); itr != creaturesInCell.end(); itr++)
	{
		if((*itr) == c)
		{
			// Remove the creature from the list
			creaturesInCell.erase(itr);
			break;
		}
	}

	sem_post(&creaturesInCellLockSemaphore);
}

/*! \brief This function returns the count of the number of creatures in the tile.
 *
 */
unsigned int Tile::numCreaturesInCell()
{
	sem_wait(&creaturesInCellLockSemaphore);
	unsigned int tempUnsigned = creaturesInCell.size();
	sem_post(&creaturesInCellLockSemaphore);

	return tempUnsigned;
}

/*! \brief This function returns the i'th creature in the tile.
 *
 */
Creature* Tile::getCreature(int index)
{
	sem_wait(&creaturesInCellLockSemaphore);
	Creature *tempCreature = creaturesInCell[index];
	sem_post(&creaturesInCellLockSemaphore);

	return tempCreature;
}

/*! \brief Add a player to the vector of players who have marked this tile for digging.
 *
 */
void Tile::addPlayerMarkingTile(Player *p)
{
	playersMarkingTile.push_back(p);
}

void Tile::removePlayerMarkingTile(Player *p)
{
	for(unsigned int i = 0; i < playersMarkingTile.size(); i++)
	{
		if(p == playersMarkingTile[i])
		{
			playersMarkingTile.erase(playersMarkingTile.begin()+i);
			return;
		}
	}
}

unsigned int Tile::numPlayersMarkingTile()
{
	return playersMarkingTile.size();
}

Player* Tile::getPlayerMarkingTile(int index)
{
	return playersMarkingTile[index];
}

void Tile::addNeighbor(Tile *n)
{
	neighbors.push_back(n);
}

void Tile::claimForColor(int nColor, double nDanceRate)
{
	if(nColor == color)
	{
		//cout << "\t\tmyTile is My color.";
		colorDouble += nDanceRate;
		if(colorDouble >= 1.0)
		{
			// Claim the tile.
			colorDouble = 1.0;
			setType(Tile::claimed);
		}
	}
	else
	{
		colorDouble -= nDanceRate;
		if(colorDouble <= 0.0)
		{
			// The tile is not yet claimed, but it is now our color.
			colorDouble *= -1.0;
			color = nColor;
		}
	}
}

Tile* Tile::getNeighbor(unsigned int index)
{
	return neighbors[index];
}

std::vector<Tile*> Tile::getAllNeighbors()
{
	return neighbors;
}

