#include "Creature.h"
#include "Defines.h"
#include "Globals.h"
#include "Functions.h"

Creature::Creature()
{
	position = Ogre::Vector3(0,0,0);
	scale = Ogre::Vector3(1,1,1);
	positionTile = gameMap.getTile((int)(position.y), (int)(position.x));
}

Creature::Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale)
{
	className = nClassName;
	meshName = nMeshName;
	scale = nScale;
	positionTile = gameMap.getTile((int)(position.y), (int)(position.x));
}

ostream& operator<<(ostream& os, Creature *c)
{
	os << c->className << "\t" << c->name << "\t";
	os << c->position.x << "\t" << c->position.y << "\t" << c->position.z << "\n";

	return os;
}

istream& operator>>(istream& is, Creature *c)
{
	static int uniqueNumber = 1;
	double xLocation = 0.0, yLocation = 0.0, zLocation = 0.0;
	string tempString;
	is >> c->className;

	is >> tempString;

	if(tempString.compare("autoname") == 0)
	{
		char tempArray[255];
		sprintf(tempArray, "%s_%04i", c->className.c_str(), uniqueNumber);
		tempString = string(tempArray);
	}

	c->name = tempString;

	is >> xLocation >> yLocation >> zLocation;
	c->position = Ogre::Vector3(xLocation, yLocation, zLocation);

	uniqueNumber++;
	return is;
}

void Creature::createMesh()
{
	Entity *ent;
	SceneNode *node;

	ent = mSceneMgr->createEntity( ("Creature_" + name).c_str(), meshName.c_str());
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode( (name + "_node").c_str() );
	//node->setPosition(position/BLENDER_UNITS_PER_OGRE_UNIT);
	node->setPosition(position);
	//FIXME: Something needs to be done about the caling issue here.
	//node->setScale(1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT);
	node->setScale(scale);
	node->attachObject(ent);
}

void Creature::destroyMesh()
{
	Entity *ent;
	SceneNode *node;

	ent = mSceneMgr->getEntity( ("Creature_" + name).c_str() );
	node = mSceneMgr->getSceneNode( (name + "_node").c_str() );
	mSceneMgr->getRootSceneNode()->removeChild( node );
	node->detachObject( ent );
	mSceneMgr->destroyEntity( ent );
	mSceneMgr->destroySceneNode( (name + "_node") );
}

void Creature::setPosition(double x, double y, double z)
{
	SceneNode *creatureSceneNode = mSceneMgr->getSceneNode(name + "_node");

	//FIXME: X-Y reversal issue.
	//creatureSceneNode->setPosition(y/BLENDER_UNITS_PER_OGRE_UNIT, x/BLENDER_UNITS_PER_OGRE_UNIT, z/BLENDER_UNITS_PER_OGRE_UNIT);
	creatureSceneNode->setPosition(y, x, z);
	gameMap.getCreature(name)->position = Ogre::Vector3(y, x, z);
	positionTile = gameMap.getTile((int)(position.y), (int)(position.x));
}

Ogre::Vector3 Creature::getPosition()
{
	return position;
}

void Creature::doTurn()
{
	// If we are not standing somewhere on the map, do nothing.
	if(positionTile == NULL)
		return;

	bool loopBack;
	// Look at the surrounding area

	// If the current task was 'idle' and it changed to something else, start doing the next
	// thing during this turn instead of waiting unitl the next turn.
	do
	{
		loopBack = false;

		// Carry out the current task
		int tempX, tempY;
		switch(currentTask)
		{
			case idle:
				//FIXME: make this into a while loop over a vector of <action, probability> pairs
				if(randomDouble(0.0, 1.0) < 0.3)
				{
					loopBack = true;
					currentTask = walkTo;
				}

				else if(randomDouble(0.0, 1.0) < 0.3)
				{
					loopBack = true;
					currentTask = dig;
				}
				break;

			case walkTo:
				/*
				if(randomDouble(0.0, 1.0) < 0.7)
				{
					// Choose a tile for the next step towards the destination, and make sure we can walk on it.
					// loopCount sets the number of times to try to pick a tile before giving up
					int loopCount = 9;
					do
					{
						if(randomDouble(0.0, 1.0) < 0.5)
							tempX = 1;
						else
							tempX = -1;

						if(randomDouble(0.0, 1.0) < 0.5)
							tempY = 1;
						else
							tempY = -1;

						loopCount--;
						cout << tempX << "\t" << tempY << endl;
						cout.flush();

					} while(gameMap.getTile((int)(position.x) + tempX, (int)(position.y) + tempY)->getFullness() != 0 && gameMap.getTile((int)(position.x) + tempX, (int)(position.y) + tempY)->getType() == Tile::lava && loopCount > 0);

					setPosition(position.x + tempX, position.y + tempY, position.z);
				}
				else
				{
					currentTask = idle;
				}
				*/
				break;

			case dig:

			default:
				break;
		}
	} while(loopBack);
}

void Creature::updateVisibleTiles()
{
}

void createVisualDebugEntities()
{
}

void destroyVisualDebugEntities()
{
}

