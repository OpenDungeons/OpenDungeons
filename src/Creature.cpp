#include "Creature.h"
#include "Defines.h"
#include "Globals.h"
#include "Functions.h"
#include "CreatureAction.h"

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
	moveSpeed = 1.0;

	currentTask = idle;
	animationState = NULL;

	if(positionTile() != NULL)
		positionTile()->addCreature(this);

	actionQueue.push_back(CreatureAction(CreatureAction::idle));
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
	moveSpeed = 1.0;

	currentTask = idle;
	animationState = NULL;

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

	// Copy the class based items
	Creature *creatureClass = gameMap.getClass(c->className);
	c->meshName = creatureClass->meshName;
	c->scale = creatureClass->scale;
	c->sightRadius = creatureClass->sightRadius;
	c->digRate = creatureClass->digRate;
	c->hp = creatureClass->hp;
	c->mana = creatureClass->mana;

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

void Creature::setPosition(Ogre::Vector3 v)
{
	setPosition(v.x, v.y, v.z);
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
	list<Tile*>basePath;

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
		int tempX, tempY, baseEndX, baseEndY;
		double diceRoll;
		Tile *neighborTile;
		bool wasANeighbor = false;

		diceRoll = randomDouble(0.0, 1.0);
		if(actionQueue.size() > 0)
		{
			switch(actionQueue.front().type)
			{
				case CreatureAction::idle:
					cout << "idle ";
					setAnimationState("Idle");
					//FIXME: make this into a while loop over a vector of <action, probability> pairs

					// Decide to check for diggable tiles with some probability
					if(diceRoll < 0.4 && digRate > 0.1)
					{
						//loopBack = true;
						//currentTask = dig;
						actionQueue.push_front(CreatureAction(CreatureAction::digTile));
						loopBack = true;

					}

					// Decide to "wander" a short distance
					else if(diceRoll < 0.6)
					{
						//loopBack = true;
						//currentTask = walkTo;
						actionQueue.push_front(CreatureAction(CreatureAction::walkToTile));
						int tempX = position.x + 2.0*gaussianRandomDouble();
						int tempY = position.y + 2.0*gaussianRandomDouble();

						setAnimationState("Walk");
						list<Tile*> result = gameMap.path(positionTile()->x+1, positionTile()->y, tempX, tempY);
						list<Tile*>::iterator itr = result.begin();
						while(itr != result.end())
						{
							addDestination((*itr)->x, (*itr)->y);
							itr++;
						}
					}
					else
					{
						// Remain idle
						//setAnimationState("Idle");
					}

					break;

				case CreatureAction::walkToTile:
					cout << "walkToTile ";
					if(walkQueue.size() == 0)
						actionQueue.pop_front();
					break;

				case CreatureAction::digTile:
					cout << "dig ";

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
					wasANeighbor = false;
					for(unsigned int i = 0; i < markedTiles.size() && !wasANeighbor; i++)
					{
						if(fabs((double)markedTiles[i]->x - position.x) <= 1.55 \
								&& fabs((double)markedTiles[i]->y - position.y) <= 1.55)
						{
							setAnimationState("Dig");
							markedTiles[i]->setFullness(markedTiles[i]->getFullness() - digRate);

							if(markedTiles[i]->getFullness() < 0)
							{
								markedTiles[i]->setFullness(0);
							}

							// If the tile has been dug out, move into that tile and idle
							if(markedTiles[i]->getFullness() == 0)
							{
								addDestination(markedTiles[i]->x, markedTiles[i]->y);
								setAnimationState("Idle");
								currentTask = walkTo;
								actionQueue.push_front(CreatureAction(CreatureAction::walkToTile));
							}

							wasANeighbor = true;
							break;
						}
					}

					if(wasANeighbor)
						break;

					// Try to find a path to one of the marked tiles
					walkPath.clear();
					for(unsigned int i = 0; i < markedTiles.size() && walkPath.size() == 0; i++)
					{
						
						bool destinationFound =  false;
						for(int j = 0; j < 4 && !destinationFound; j++)
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

							// The +1 is a hack to check the neighbor of the marked tile.
							walkPath = gameMap.path(positionTile()->x, positionTile()->y, markedTiles[i]->x+tempX, markedTiles[i]->y+tempY);

							if(walkPath.size() > 2)
							{
								list<Tile*>::iterator itr = walkPath.begin();
								itr++;
								while(itr != walkPath.end())
								{
									addDestination((*itr)->x, (*itr)->y);
									itr++;
								}

								destinationFound = true;
								actionQueue.push_front(CreatureAction(CreatureAction::walkToTile));
								break;
							}
						}
					}

					// If none of our neighbors are marked for digging we got here too late.
					// Finish digging
					actionQueue.pop_front();
					break;
					/*
					cout << "dig ";
					if(digRate > 0.1)
					{
						cout << "Starting dig: rate:  " << digRate << " ";
						//cout.flush();


						// See if any of the tiles is one of our neighbors
						for(unsigned int i = 0; i < markedTiles.size(); i++)
						{
							if(fabs((double)markedTiles[i]->x - position.x) <= 1.55 \
									&& fabs((double)markedTiles[i]->y - position.y) <= 1.55)
							{
								setAnimationState("Dig");
								markedTiles[i]->setFullness(markedTiles[i]->getFullness() - digRate);

								if(markedTiles[i]->getFullness() < 0)
								{
									markedTiles[i]->setFullness(0);
								}

								// If the tile has been dug out, move into that tile and idle
								if(markedTiles[i]->getFullness() == 0)
								{
									destinationX = markedTiles[i]->x;
									destinationY = markedTiles[i]->y;
									setAnimationState("Idle");
									currentTask = walkTo;
								}

								break;
							}
						}

						// If no tiles are next to us, see if we can walk to one to dig it out
						bool destinationFound =  false;
						for(unsigned int i = 0; i < markedTiles.size() && !destinationFound; i++)
						{
							//  j<4 :  Allow standing on adjacent corners only, when digging.
							//  j<8 :  Allow standing on any corners, when digging.
							walkPath.clear();
							basePath.clear();
							for(int j = 0; j < 4 && !destinationFound; j++)
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

								neighborTile = gameMap.getTile(tempX, tempY);
								if(neighborTile != NULL && neighborTile->getFullness() == 0)
								{
									walkPath = gameMap.path(positionTile()->x, positionTile()->y, tempX, tempY);

									// If we found a path to the neighbor tile
									if(walkPath.size() >= 2)
									{
										// Take a step towards the neighbor tile, the second tile is the one we want
										nextStep = *(++(walkPath.begin()));
										//setPosition(nextStep->x, nextStep->y, position.z);
										addDestination(nextStep->x, nextStep->y);
										setAnimationState("Walk");
										destinationFound = true;
										break;
									}
								}
							}
						}
						// If we still can't do anything then give up and idle
						setAnimationState("Idle");
						currentTask = idle;
						loopBack = true;
					}
					else
					{
						// If our dig rate is too low to be of use
						currentTask = idle;
						setAnimationState("Idle");
						loopBack = true;
					}

					break;
					*/

				default:
					cout << "default ";
					break;
			}
		}
	} while(loopBack);
}

// Creates a list of Tile pointers in visibleTiles
void Creature::updateVisibleTiles()
{
	int xMin, yMin, xMax, yMax;

	//cout << "sightrad: " << sightRadius << " ";
	visibleTiles.clear();
	xMin = (int)position.x - sightRadius;
	xMax = (int)position.x + sightRadius;
	yMin = (int)position.y - sightRadius;
	yMax = (int)position.y + sightRadius;

	// Add the circular portion of the visible region
	//TODO:  This does not implement terrain yet, i.e. the creature can see through walls
	for(int i = xMin; i < xMax; i++)
	{
		for(int j = yMin; j < yMax; j++)
		{
			int distSQ = powl(position.x - i, 2.0) + powl(position.y - j, 2.0);
			if(distSQ < sightRadius * sightRadius)
			{
				Tile *currentTile = gameMap.getTile(i, j);
				if(currentTile != NULL)
					visibleTiles.push_back(currentTile);
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

void Creature::setAnimationState(string s)
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::setCreatureAnimationState;
	request->p = this;
	request->str = s;

	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(request);
	sem_post(&renderQueueSemaphore);
}

AnimationState* Creature::getAnimationState()
{
	return animationState;
}

void Creature::addDestination(int x, int y)
{
	cout << "w(" << x << ", " << y << ") ";

	if(walkQueue.size() == 0)
	{
		walkQueue.push_back(Ogre::Vector3(x, y, 0));
		shortDistance = position.distance(walkQueue.front());
		walkDirection = walkQueue.front() - position;

		SceneNode *node = mSceneMgr->getSceneNode(name + "_node");
		Ogre::Vector3 src = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;
		Quaternion quat = src.getRotationTo(walkDirection);
		node->rotate(quat);
	}
	else
	{
		walkQueue.push_back(Ogre::Vector3(x, y, 0));
	}
}
