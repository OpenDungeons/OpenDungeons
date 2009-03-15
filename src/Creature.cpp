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

	hp = 10;
	mana = 10;
	sightRadius = 10;
	digRate = 10;

	if(positionTile() != NULL)
		positionTile()->addCreature(this);
}

Creature::Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale, int nHP, int nMana, double nSightRadius, double nDigRate)
{
	className = nClassName;
	meshName = nMeshName;
	scale = nScale;
	destinationX = 0;
	destinationY = 0;

	hp = nHP;
	mana = nMana;
	sightRadius = nSightRadius;
	digRate = nDigRate;

	if(positionTile() != NULL)
		positionTile()->addCreature(this);
}

ostream& operator<<(ostream& os, Creature *c)
{
	os << c->className << "\t" << c->name << "\t";
	os << c->position.x << "\t" << c->position.y << "\t" << c->position.z << "\t";
	os << c->color << "\n";

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
		uniqueNumber++;
	}

	c->name = tempString;

	is >> xLocation >> yLocation >> zLocation;
	c->position = Ogre::Vector3(xLocation, yLocation, zLocation);
	is >> c->color;

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
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyCreature;
	request->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
}

void Creature::setPosition(double x, double y, double z)
{
	SceneNode *creatureSceneNode = mSceneMgr->getSceneNode(name + "_node");

	//creatureSceneNode->setPosition(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, z/BLENDER_UNITS_PER_OGRE_UNIT);
	creatureSceneNode->setPosition(x, y, z);
	Tile *oldPositionTile = positionTile();
	position = Ogre::Vector3(x, y, z);

	if(oldPositionTile != NULL)
		oldPositionTile->removeCreature(this);

	if(positionTile() != NULL)
		positionTile()->addCreature(this);
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
		double diceRoll;
		diceRoll = randomDouble(0.0, 1.0);
		switch(currentTask)
		{
			case idle:
				//FIXME: make this into a while loop over a vector of <action, probability> pairs

				if(diceRoll < 0.6)
				{
					loopBack = true;
					currentTask = dig;
				}

				else if(diceRoll < 0.9)
				{
					loopBack = true;
					currentTask = walkTo;
					destinationX = position.x + 2.0*gaussianRandomDouble();
					destinationY = position.y + 2.0*gaussianRandomDouble();
				}
				else
				{
					// Remain idle
				}

				break;

			case walkTo:
				if((int)positionTile()->x != destinationX || (int)positionTile()->y != destinationY)
				{
					// Choose a tile for the next step towards the destination
					walkPath = gameMap.path((int)position.x, (int)position.y, destinationX, destinationY);
					//cout << "\n\nWalk path size = " << walkPath.size() << endl;
					//cout.flush();

					if(walkPath.size() >= 2)
					{
						// The second tile is the one we want
						nextStep = *(++(walkPath.begin()));
						setPosition(nextStep->x, nextStep->y, position.z);
					}
					else
					{
						currentTask = idle;
						loopBack = true;
						//cout << "\n\nCould not find path to destination.";
					}
				}
				else
				{
					currentTask = idle;
				}
				break;

			case dig:
				if(digRate > 0.1)
				{
					//cout << "Starting dig\nDR:  " << digRate << "\n\n";
					//cout.flush();

					// Find visible tiles, marked for digging
					for(unsigned int i = 0; i < visibleTiles.size(); i++)
					{
						// Check to see if the tile is marked for digging
						if(visibleTiles[i]->getMarkedForDigging())
						{
							markedTiles.push_back(visibleTiles[i]);
						}
					}

					// See if any of the tiles is one of our neighbors
					for(unsigned int i = 0; i < markedTiles.size(); i++)
					{
						if(fabs((double)markedTiles[i]->x - position.x) <= 1.55 \
								&& fabs((double)markedTiles[i]->y - position.y) <= 1.55)
						{
							markedTiles[i]->setFullness(markedTiles[i]->getFullness() - digRate);
							break;
						}
					}

					// If no tiles are next to us, see if we can walk to one to dig it out
					for(unsigned int i = 0; i < markedTiles.size(); i++)
					{
						//  j<4 :  Allow standing on adjacent corners only, when digging.
						//  j<8 :  Allow standing on any corners, when digging.
						for(int j = 0; j < 4; j++)
						{
							tempX = markedTiles[i]->x;
							tempY = markedTiles[i]->y;
							switch(j)
							{
								// Adjacent tiles
								case 0:  tempX += -1;  tempY += -0;  break;
								case 1:  tempX += 0;  tempY += -1;  break;
								case 2:  tempX += 0;  tempY += 1;  break;
								case 3:  tempX += 1;  tempY += 0;  break;

								 // Corner tiles
								case 4:  tempX += -1;  tempY += -1;  break;
								case 5:  tempX += -1;  tempY += 1;  break;
								case 6:  tempX += 1;  tempY += -1;  break;
								case 7:  tempX += 1;  tempY += 1;  break;
							}

							if(gameMap.getTile(tempX, tempY)->getFullness() == 0)
							{
								walkPath = gameMap.path(positionTile()->x, positionTile()->y, tempX, tempY);

								if(walkPath.size() >= 2)
								{
									// The second tile is the one we want
									nextStep = *(++(walkPath.begin()));
									setPosition(nextStep->x, nextStep->y, position.z);
									break;
								}
							}
						}
					}
					// If we still can't do anything then give up and idle
					currentTask = idle;
				}
				else
				{
					// If our di rate is too low to be of use
					currentTask = idle;
					loopBack = true;
				}

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

void Creature::deleteYourself()
{
	if(positionTile() != NULL)
		positionTile()->removeCreature(this);

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyCreature;
	request->p = this;

	RenderRequest *request2 = new RenderRequest;
	request2->type = RenderRequest::deleteCreature;
	request2->p = this;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	renderQueue.push_back(request2);
	sem_post(&renderQueueSemaphore);
}

