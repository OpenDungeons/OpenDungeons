#include "Defines.h"
#include "Tile.h"
#include "Globals.h"
#include "Creature.h"

Tile::Tile()
{
	selected = false;
	markedForDigging = false;
	location = Ogre::Vector3(0.0, 0.0, 0.0);
	type = dirt;
	setFullness(100);
	rotation = 0.0;
}

Tile::Tile(int nX, int nY, TileType nType, int nFullness)
{
	selected = false;
	markedForDigging = false;
	x = nX;
	y = nY;
	setType(nType);
	setFullness(nFullness);
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
 * how full the tile is.  In the future this function will also be responsible
 * for determining the orientation of the tile to make corners display
 * correctly.  Both of these tasks are accomplished by setting the
 * fullnessMeshNumber variable which is concatenated to the tile's type to
 * determine the mesh to load, e.g. Rock104.mesh for a rocky tile which has all
 * 4 sides shown because it is an "island" with all four sides visible.
 */
void Tile::setFullness(int f)
{
	fullness = f;

	//FIXME:  This needs to be updated to reflect the allowable fill states for each tile type
	// This is also where the logic for checking neighboring fullness should go
	fullnessMeshNumber = 0;
	if(f > 0 && f <= 25)	fullnessMeshNumber = 25;
	else if(f > 25 && f <= 50)	fullnessMeshNumber = 50;
	else if(f > 50 && f <= 75)	fullnessMeshNumber = 75;
	else if(f > 75)
	{
		// If  all 4 neighbors exist and are also in the "full"
		// category, then we don't draw the sides on this tile.
		Tile *top = gameMap.getTile(x, y+1);
		Tile *bottom = gameMap.getTile(x, y-1);
		Tile *left = gameMap.getTile(x-1, y);
		Tile *right = gameMap.getTile(x+1, y);
		if(top != NULL && bottom != NULL && left != NULL && right != NULL)
		{
			if(top->getFullness() > 75 && bottom->getFullness() > 75 && \
			left->getFullness() > 75 && right->getFullness() > 75)
			{
				fullnessMeshNumber = 100;
			}
			else
			{
				fullnessMeshNumber = 104;
			}
		}
		else
		{
			fullnessMeshNumber = 104;
		}
	}

	refreshMesh();

	if(fullness <= 1 && getMarkedForDigging(gameMap.me) == true)
		setMarkedForDigging(false, gameMap.me);

	if(serverSocket != NULL)
	{
		try
		{
			// Inform the clients that the fullness has changed.
			ServerNotification *serverNotification = new ServerNotification;
			serverNotification->type = ServerNotification::tileFullnessChange;
			serverNotification->tile = this;

			sem_wait(&serverNotificationQueueLockSemaphore);
			serverNotificationQueue.push_back(serverNotification);
			sem_post(&serverNotificationQueueLockSemaphore);

			sem_post(&serverNotificationQueueSemaphore);
		}
		catch(bad_alloc&)
		{
			cout << "\n\nERROR:  bad alloc in Tile::setFullness\n\n";
			exit(1);
		}
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
	if(fullnessMeshNumber != 0)
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
			cout << "\n\nERROR:  Unhandled tile type in Tile::getTilePassability()\n\n";
			exit(1);
			break;
	}

	// Return something to make the compiler happy.
	// Control should really never reach here because of the exit(1) call in the default switch case above
	cout << "\n\nERROR:  Control reached the end of Tile::getTilePassability, this should never actually happen.\n\n";
	exit(1);
	return impassableTile;
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
	sprintf(tempCellName, "Level_%3i_%3i", xLocation, yLocation);
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

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
}

/*! \brief This function puts a message in the renderQueue to load the mesh for this tile.
 *
 */
void Tile::createMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createTile;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);

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

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
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
	sprintf(tempString, "Level_%3i_%3i_selection_indicator", x, y);
	if(mSceneMgr->hasEntity(tempString))
	{
		ent = mSceneMgr->getEntity(tempString);
	}
	else
	{
		sprintf(tempString2, "Level_%3i_%3i_node", x, y);
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
			sprintf(tempString, "Level_%i_%i_selection_indicator", x, y);
			if(mSceneMgr->hasEntity(tempString))
			{
				ent = mSceneMgr->getEntity(tempString);
			}
			else
			{
				sprintf(tempString2, "Level_%3i_%3i_node", x, y);
				cout << "\n\nTempstring2:  " << tempString2;
				cout.flush();
				SceneNode *tempNode = mSceneMgr->getSceneNode(tempString2);

				ent = mSceneMgr->createEntity(tempString, "DigSelector.mesh");
				ent->setNormaliseNormals(true);
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

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	renderQueue.push_back(request2);
	sem_post(&renderQueueSemaphore);
}

/*! \brief This function adds a creature to the list of creatures in this tile.
 *
 */
void Tile::addCreature(Creature *c)
{
	creaturesInCell.push_back(c);
}

/*! \brief This function removes a creature to the list of creatures in this tile.
 *
 */
void Tile::removeCreature(Creature *c)
{
	// Check to see if the given crature is actually in this tile
	vector<Creature*>::iterator itr;
	for(itr = creaturesInCell.begin(); itr != creaturesInCell.end(); itr++)
	{
		if((*itr) == c)
		{
			// Remove the creature from the list
			creaturesInCell.erase(itr);
			return;
		}
	}
}

/*! \brief This function returns the count of the number of creatures in the tile.
 *
 */
int Tile::numCreaturesInCell()
{
	return creaturesInCell.size();
}

/*! \brief This function returns the i'th creature in the tile.
 *
 */
Creature* Tile::getCreature(int index)
{
	return creaturesInCell[index];
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

int Tile::numPlayersMarkingTile()
{
	return playersMarkingTile.size();
}

Player* Tile::getPlayerMarkingTile(int index)
{
	return playersMarkingTile[index];
}

