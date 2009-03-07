#include "Defines.h"
#include "Tile.h"

Tile::Tile()
{
	location = Ogre::Vector3(0.0, 0.0, 0.0);
	type = dirt;
	setFullness(100);
	rotation = 0.0;

}

Tile::Tile(int nX, int nY, TileType nType, int nFullness)
{
	x = nX;
	y = nY;
	setType(nType);
	setFullness(nFullness);
}

Tile::~Tile()
{
	destroyMesh();
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
	int tempInt;

	is >> t->x >> t->y;
	is >> tempInt;
	t->type = (Tile::TileType) tempInt;
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
	 char meshName[255];
	 Entity *ent;

	 if(mSceneMgr->hasSceneNode( (name + "_node").c_str() ) )
	 {
		  mSceneMgr->getSceneNode( (name + "_node").c_str() )->detachObject(name.c_str());
		  mSceneMgr->destroyEntity(name.c_str());

		  string tileTypeString = tileTypeToString(type);
		  sprintf(meshName, "%s%i.mesh", tileTypeString.c_str(), fullnessMeshNumber);
		  ent = mSceneMgr->createEntity(name.c_str(), meshName);

		  mSceneMgr->getSceneNode((name + "_node").c_str())->attachObject(ent);
	 }
}

void Tile::createMesh()
{
	char meshName[255];
	char tempString[255];
	Entity *ent;
	SceneNode *node;

	string tileTypeString = Tile::tileTypeToString(getType());

	sprintf(meshName, "%s%i.mesh", tileTypeString.c_str(), getFullnessMeshNumber());
	ent = mSceneMgr->createEntity(name.c_str(), meshName);

	sprintf(tempString, "%s_node", name.c_str());
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode(tempString);
	node->setPosition(Ogre::Vector3(y/BLENDER_UNITS_PER_OGRE_UNIT, x/BLENDER_UNITS_PER_OGRE_UNIT, 0));

	node->attachObject(ent);
}

void Tile::destroyMesh()
{
	if(mSceneMgr->hasEntity(name.c_str()))
	{
		cout << endl << "destroying mesh " << name << endl;
		cout.flush();

		char tempString[255];
		Entity *ent;
		SceneNode *node;

		ent = mSceneMgr->getEntity(name.c_str());
		node = mSceneMgr->getSceneNode((name + "_node").c_str());
		//mSceneMgr->getRootSceneNode()->detachObject((name + "_node").c_str());
		node->detachAllObjects();
		mSceneMgr->destroySceneNode((name + "_node").c_str());
		mSceneMgr->destroyEntity(ent);
	}
}

void Tile::setSelected(bool s)
{
	Entity *ent;
	char tempString[255];
	char tempString2[255];
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

