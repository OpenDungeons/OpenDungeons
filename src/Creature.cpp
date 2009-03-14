#include "Creature.h"
#include "Defines.h"
#include "Globals.h"
#include "Functions.h"

Creature::Creature()
{
	position = Ogre::Vector3(0,0,0);
	scale = Ogre::Vector3(1,1,1);
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
	//creatureSceneNode->setPosition(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, z/BLENDER_UNITS_PER_OGRE_UNIT);
	creatureSceneNode->setPosition(x, y, z);
	gameMap.getCreature(name)->position = Ogre::Vector3(x, y, z);
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
	if(positionTile() == NULL)
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

				if(randomDouble(0.0, 1.0) < 0.7)
				{
					loopBack = true;
					currentTask = dig;
					break;
				}

				/*
				if(randomDouble(0.0, 1.0) < 0.3)
				{
					loopBack = true;
					currentTask = walkTo;
					destinationX = 5;
					destinationY = 3;
					break;
				}
				*/
				break;

			case walkTo:
				if(randomDouble(0.0, 1.0) < 0.7)
				{
					// Choose a tile for the next step towards the destination
					//FIXME:  X-Y reversal
					walkPath = gameMap.path((int)position.x, (int)position.y, destinationX, destinationY);
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

				// See if any of the tiles is one of our neighbors
				for(int i = 0; i < markedTiles.size(); i++)
				{
					//FIXME:  X-Y reversal
					if(fabs((double)markedTiles[i]->x - position.x) <= 1.55 \
							&& fabs((double)markedTiles[i]->y - position.y) <= 1.55)
					{
						markedTiles[i]->setFullness(markedTiles[i]->getFullness() - digRate);
						break;
					}
				}

				// If no tiles are next to us, see if we can walk to one to dig it out
				for(int i = 0; i < markedTiles.size(); i++)
				{
					for(int j = 0; j < 8; j++)
					{
						tempX = markedTiles[i]->x;
						tempY = markedTiles[i]->y;
						switch(j)
						{
							case 0:  tempX += -1;  tempY += -1;  break;
							case 1:  tempX += -1;  tempY += -0;  break;
							case 2:  tempX += -1;  tempY += 1;  break;
							case 3:  tempX += 0;  tempY += -1;  break;
							case 4:  tempX += 0;  tempY += 1;  break;
							case 5:  tempX += 1;  tempY += -1;  break;
							case 6:  tempX += 1;  tempY += 0;  break;
							case 7:  tempX += 1;  tempY += 1;  break;
						}

						//FIXME:  X-Y reversal
						walkPath = gameMap.path(positionTile()->x, positionTile()->y, tempX, tempY);

						if(walkPath.size() >= 2)
						{
							nextStep = *(++(walkPath.begin()));
							//FIXME:  X-Y reversal
							setPosition(nextStep->x, nextStep->y, position.z);
							break;
						}
					}
				}

				// If we still can't do anything then give up and idle
				currentTask = idle;
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
	//TODO:  fill in this stub method.
}

void destroyVisualDebugEntities()
{
	//TODO:  fill in this stub method.
}

Tile* Creature::positionTile()
{
	return gameMap.getTile((int)(position.x), (int)(position.y));
}

