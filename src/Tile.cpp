#include "Defines.h"
#include "Tile.h"
#include "Globals.h"

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

Tile::TileType Tile::getType()
{
	 return type;
}

void Tile::setFullness(int f)
{
	 fullness = f;

	 //FIXME:  This needs to be updated to reflect the allowable fill states for each tile type
	 // This is also where the logic for checking neighboring fullness should go
	 fullnessMeshNumber = 0;
	 if(f > 0 && f <= 25)	fullnessMeshNumber = 25;
	 else if(f > 25 && f <= 50)	fullnessMeshNumber = 50;
	 else if(f > 50 && f <= 75)	fullnessMeshNumber = 75;
	 else if(f >= 75)	fullnessMeshNumber = 104;
	 refreshMesh();

	 if(fullness <= 1 && getMarkedForDigging() == true)
		 setMarkedForDigging(false);
}

int Tile::getFullness()
{
	 return fullness;
}

int Tile::getFullnessMeshNumber()
{
	return fullnessMeshNumber;
}

ostream& operator<<(ostream& os, Tile *t)
{
	os << t->x << "\t" << t->y << "\t" << t->getType() << "\t" << t->getFullness();

	return os;
}

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

Tile::TileType Tile::nextTileType(TileType t)
{
	int currentType = (int)t;
	currentType++;
	currentType %= (int)(nullTileType);

	return (TileType) currentType;
}

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

void Tile::refreshMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::refreshTile;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
}

void Tile::createMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createTile;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
}

void Tile::destroyMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyTile;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
}

void Tile::setSelected(bool s)
{
	Entity *ent;
	char tempString[255];
	char tempString2[255];

	//FIXME:  This code should probably only exectute if it needs to for speed reasons.
	sprintf(tempString, "Level_%3i_%3i_selction_indicator", x, y);
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

bool Tile::getSelected()
{
	return selected;
}

void Tile::setMarkedForDigging(bool s)
{
	Entity *ent;
	char tempString[255];
	char tempString2[255];

	if(markedForDigging != s)
	{
		//FIXME:  This code should probably only exectute if it needs to for speed reasons.
		sprintf(tempString, "Level_%3i_%3i_selction_indicator", x, y);
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

		markedForDigging = s;

		if(markedForDigging)
		{
			ent->setVisible(true);
		}
		else
		{
			ent->setVisible(false);
		}
	}
}

bool Tile::getMarkedForDigging()
{
	return markedForDigging;
}

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

