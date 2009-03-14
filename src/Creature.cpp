#include "Creature.h"
#include "Defines.h"
#include "Globals.h"
#include "Functions.h"

Creature::Creature()
{
	position = Ogre::Vector3(0,0,0);
	scale = Ogre::Vector3(1,1,1);
	positionTile = gameMap.getTile((int)(position.y), (int)(position.x));
	sightRadius = 10.0;
	digRate = 10.0;
	destinationX = 0;
	destinationY = 0;
}

Creature::Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale)
{
	className = nClassName;
	meshName = nMeshName;
	scale = nScale;
	positionTile = gameMap.getTile((int)(position.y), (int)(position.x));
	sightRadius = 10.0;
	digRate = 10.0;
	destinationX = 0;
	destinationY = 0;
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
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createCreature;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
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
	vector<Tile*> markedTiles;
	Tile *nextStep;
	list<Tile*>walkPath;

	// If we are not standing somewhere on the map, do nothing.
	if(positionTile == NULL)
		return;

	bool loopBack;
	// Look at the surrounding area
	updateVisibleTiles();

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
					destinationX = destinationY = 3;
				}

				/*
				else if(randomDouble(0.0, 1.0) < 0.3)
				{
					loopBack = true;
					currentTask = dig;
				}
				*/
				break;

			case walkTo:
				if(randomDouble(0.0, 1.0) < 0.9)
				{
					// Choose a tile for the next step towards the destination
					//FIXME:  X-Y reversal
					walkPath = gameMap.path((int)position.y, (int)position.x, destinationY, destinationX);
					cout << "\n\n\n\n\nWalk path size = " << walkPath.size() << endl;
					cout.flush();
					if(walkPath.size() == 1)
					{
						nextStep = *(walkPath.begin());
						setPosition(nextStep->x, nextStep->y, position.z);
					}
					else
					{
						if(walkPath.size() >= 2)
						{
						nextStep = *(++(walkPath.begin()));
						setPosition(nextStep->x, nextStep->y, position.z);
						}
						else
						{
							cout << "\n\nCould not find path to destination.";
						}
					}
				}
				else
				{
					currentTask = idle;
				}
				break;

			case dig:
				cout << "Starting dig\n\n\n\n\n";
				cout.flush();

				// Find visible tiles, marked for digging
				for(int i = 0; i < visibleTiles.size(); i++)
				{
					// Check to see if the tile is marked for digging
					if(visibleTiles[i]->getMarkedForDigging())
					{
						markedTiles.push_back(visibleTiles[i]);
					}
				}

				// Dig one out
				if(markedTiles.size() > 0)
					markedTiles[0]->setFullness(markedTiles[0]->getFullness() - digRate);
				break;

			default:
				break;
		}
	} while(loopBack);
}

void Creature::updateVisibleTiles()
{
	int xMin, yMin, xMax, yMax;

	visibleTiles.clear();
	xMin = (int)position.x - sightRadius;
	xMax = (int)position.x + sightRadius;
	yMin = (int)position.y - sightRadius;
	yMax = (int)position.y + sightRadius;

	// Add the circular portion of the visible region
	for(int i = xMin; i < xMax; i++)
	{
		for(int j = yMin; j < yMax; j++)
		{
			int distSQ = powl(position.x - i, 2.0) + powl(position.y - j, 2.0);
			if(distSQ < sightRadius * sightRadius)
			{
				Tile *currentTile = gameMap.getTile(i, j);
				if(currentTile != NULL)
					visibleTiles.push_back(gameMap.getTile(i,j));
			}
		}
	}

	//TODO:  Add the sector shaped region of the visible region
}

void createVisualDebugEntities()
{
}

void destroyVisualDebugEntities()
{
}

