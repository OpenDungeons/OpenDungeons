#include <math.h>
#include <algorithm>
using namespace std;

#include <Ogre.h>

#include "Creature.h"
#include "Defines.h"
#include "Globals.h"
#include "Functions.h"
#include "CreatureAction.h"
#include "Network.h"
#include "Field.h"
#include "Weapon.h"

Creature::Creature()
{
	sem_init(&meshCreationFinishedSemaphore, 0, 0);
	sem_init(&meshDestructionFinishedSemaphore, 0, 0);
	hasVisualDebuggingEntities = false;
	position = Ogre::Vector3(0,0,0);
	scale = Ogre::Vector3(1,1,1);
	sightRadius = 10.0;
	digRate = 10.0;
	danceRate = 0.35;
	destinationX = 0;
	destinationY = 0;

	hp = 10;
	mana = 10;
	sightRadius = 10;
	digRate = 10;
	moveSpeed = 1.0;
	tilePassability = Tile::walkableTile;

	weaponL = NULL;
	weaponR = NULL;

	animationState = NULL;

	actionQueue.push_back(CreatureAction(CreatureAction::idle));
	battleField = new Field("autoname");
}

Creature::Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale, int nHP, int nMana, double nSightRadius, double nDigRate, double nMoveSpeed)
{
	// This constructor is meant to be used to initialize a creature class so no creature specific stuff should be set
	className = nClassName;
	meshName = nMeshName;
	scale = nScale;

	hp = nHP;
	mana = nMana;
	sightRadius = nSightRadius;
	digRate = nDigRate;
	moveSpeed = nMoveSpeed;
	tilePassability = Tile::walkableTile;
}

/*! \brief A matched function to transport creatures between files and over the network.
 *
 */
ostream& operator<<(ostream& os, Creature *c)
{
	os << c->className << "\t" << c->name << "\t";
	os << c->position.x << "\t" << c->position.y << "\t" << c->position.z << "\t";
	os << c->color << "\t";
	os << c->weaponL << "\t" << c->weaponR;

	return os;
}

/*! \brief A matched function to transport creatures between files and over the network.
 *
 */
istream& operator>>(istream& is, Creature *c)
{
	static int uniqueNumber = 1;
	double xLocation = 0.0, yLocation = 0.0, zLocation = 0.0;
	string tempString;
	is >> c->className;

	is >> tempString;

	if(tempString.compare("autoname") == 0)
	{
		tempString = c->className + Ogre::StringConverter::toString(uniqueNumber);
		uniqueNumber++;
	}

	c->name = tempString;

	is >> xLocation >> yLocation >> zLocation;
	c->position = Ogre::Vector3(xLocation, yLocation, zLocation);
	is >> c->color;

	c->weaponL = new Weapon;
	is >> c->weaponL;
	c->weaponL->parentCreature = c;
	c->weaponL->handString = "L";

	c->weaponR = new Weapon;
	is >> c->weaponR;
	c->weaponR->parentCreature = c;
	c->weaponR->handString = "R";

	// Copy the class based items
	Creature *creatureClass = gameMap.getClassDescription(c->className);
	if(creatureClass != NULL)
	{
		c->meshName = creatureClass->meshName;
		c->scale = creatureClass->scale;
		c->sightRadius = creatureClass->sightRadius;
		c->digRate = creatureClass->digRate;
		c->hp = creatureClass->hp;
		c->mana = creatureClass->mana;
		c->moveSpeed = creatureClass->moveSpeed;
	}

	return is;
}

/*! \brief Allocate storage for, load, and inform OGRE about a mesh for this creature.
 *
 *  This function is called after a creature has been loaded from hard disk,
 *  received from a network connection, or created during the game play by the
 *  game engine itself.
 */
void Creature::createMesh()
{
	//NOTE: I think this line is redundant since the a sem_wait on any previous destruction should return the sem to 0 anyway but this takes care of it in case it is forgotten somehow
	sem_init(&meshCreationFinishedSemaphore, 0, 0);
	sem_init(&meshDestructionFinishedSemaphore, 0, 0);

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createCreature;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	//FIXME:  This function needs to wait until the render queue has processed the request before returning.  This should fix the bug where the client crashes loading levels with lots of creatures.  Other create mesh routines should have a similar wait statement.  It currently breaks the program since this function gets called from the rendering thread causing the thread to wait for itself to do something.
	//sem_wait(&meshCreationFinishedSemaphore);

}


/*! \brief Free the mesh and inform the OGRE system that the mesh has been destroyed.
 *
 *  This function is primarily a helper function for other methods.
 */
void Creature::destroyMesh()
{
	weaponL->destroyMesh();
	weaponR->destroyMesh();

	//NOTE: I think this line is redundant since the a sem_wait on any previous creation should return the sem to 0 anyway but this takes care of it in case it is forgotten somehow
	//sem_init(&meshCreationFinishedSemaphore, 0, 0);

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyCreature;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	sem_wait(&meshDestructionFinishedSemaphore);
}

/*! \brief Changes the creatures position to a new position.
 *
 *  This is an overloaded function which just calls Creature::setPosition(double x, double y, double z).
 */
void Creature::setPosition(Ogre::Vector3 v)
{
	setPosition(v.x, v.y, v.z);
}

/*! \brief Changes the creatures position to a new position.
 *
 *  Moves the creature to a new location in 3d space.  This function is
 *  responsible for informing OGRE anything it needs to know, as well as
 *  maintaining the list of creatures in the individual tiles.
 */
void Creature::setPosition(double x, double y, double z)
{
	// If we are on the gameMap we may need to update the tile we are in
	if(gameMap.getCreature(name) != NULL)
	{
		// We are on the map
		// Move the creature relative to its parent scene node.  We record the
		// tile the creature is in before and after the move to properly
		// maintain the results returned by the positionTile() function.
		Tile *oldPositionTile = positionTile();
		position = Ogre::Vector3(x, y, z);
		Tile *newPositionTile = positionTile();

		if(oldPositionTile != newPositionTile)
		{
			if(oldPositionTile != NULL)
				oldPositionTile->removeCreature(this);

			if(positionTile() != NULL)
				positionTile()->addCreature(this);
		}
	}
	else
	{
		// We are not on the map
		position = Ogre::Vector3(x, y, z);
	}

	// Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::moveSceneNode;
	request->str = name + "_node";
	request->vec = position;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

/*! \brief A simple accessor function to get the creature's current position in 3d space.
 *
 */
Ogre::Vector3 Creature::getPosition()
{
	return position;
}

/*! \brief The main AI routine which decides what the creature will do and carries out that action.
 *
 * The doTurn routine is the heart of the Creature AI subsystem.  The other,
 * higher level, functions such as GameMap::doTurn() ultimately just call this
 * function to make the creatures act.
 *
 * The function begins in a pre-cognition phase which prepares the creature's
 * brain state for decision making.  This involves generating lists of known
 * about creatures, either through sight, hearing, keeper knowledge, etc, as
 * well as some other bookkeeping stuff.
 *
 * Next the function enters the cognition phase where the creature's current
 * state is examined and a decision is made about what to do.  The state of the
 * creature is in the form of a queue, which is really used more like a stack.
 * At the beginning of the game the 'idle' action is pushed onto each
 * creature's actionQueue, this action is never removed from the tail end of
 * the queue and acts as a "last resort" for when the creature completely runs
 * out of things to do.  Other actions such as 'walkToTile' or 'attackCreature'
 * are then pushed onto the front of the queue and will determine the
 * creature's future behavior.  When actions are complete they are popped off
 * the front of the action queue, causing the creature to revert back into the
 * state it was in when the actions was placed onto the queue.  This allows
 * actions to be carried out recursively, i.e. if a creature is trying to dig a
 * tile and it is not nearby it can begin walking toward the tile as a new
 * action, and when it arrives at the tile it will revert to the 'digTile'
 * action.
 *
 * In the future there should also be a post-cognition phase to do any
 * additional checks after it tries to move, etc.
 */
void Creature::doTurn()
{
	vector<Tile*> markedTiles;
	list<Tile*>walkPath;
	list<Tile*>basePath;
	list<Tile*>::iterator tileListItr;
	vector< list<Tile*> > possiblePaths;
	vector< list<Tile*> > shortPaths;
	bool loopBack;
	int tempInt;
	unsigned int tempUnsigned;
	CreatureAction tempAction;

	// If we are not standing somewhere on the map, do nothing.
	if(positionTile() == NULL)
		return;

	// Look at the surrounding area
	updateVisibleTiles();
	visibleEnemies = getVisibleEnemies();
	reachableEnemies = getReachableCreatures(visibleEnemies);
	enemiesInRange = getEnemiesInRange(visibleEnemies);
	visibleAllies = getVisibleAllies();
	if(digRate > 0.1)
		markedTiles = getVisibleMarkedTiles();

	// If the creature can see enemies that are reachable.
	if(reachableEnemies.size() > 0)
	{
		/*
		cout << "\nCreature sees enemies:  " << visibleEnemies.size() << "   " << name;
		cout << "\nvisibleEnemies:\n";

		for(unsigned int i = 0; i < visibleEnemies.size(); i++)
		{
			cout << visibleEnemies[i] << endl;
		}
		*/

		// If we are not already fighting with a creature or maneuvering then start doing so.
		tempAction = actionQueue.front();
		if(tempAction.type != CreatureAction::attackCreature || tempAction.type != CreatureAction::maneuver)
		{
			tempAction.type = CreatureAction::maneuver;
			actionQueue.push_front(tempAction);
		}
	}

	// The loopback variable allows creatures to begin processing a new
	// action immediately after some other action happens.
	do
	{
		loopBack = false;

		// Carry out the current task
		double diceRoll;
		double tempDouble;
		Tile *neighborTile;
		vector<Tile*>neighbors, neighbors2, creatureNeighbors, claimableTiles;
		bool wasANeighbor = false;
		Player *tempPlayer;
		Tile *tempTile, *tempTile2, *myTile;
		list<Tile*> tempPath;
		pair<LocationType, double> min;

		diceRoll = randomDouble(0.0, 1.0);
		if(actionQueue.size() > 0)
		{
			switch(actionQueue.front().type)
			{
				case CreatureAction::idle:
					//cout << "idle ";
					setAnimationState("Idle");
					//FIXME: make this into a while loop over a vector of <action, probability> pairs

					// Decide to check for clamiable tiles
					if(diceRoll < 0.2 && digRate > 0.1)
					{
						loopBack = true;
						actionQueue.push_front(CreatureAction(CreatureAction::claimTile));
					}

					// Decide to check for diggable tiles
					else if(diceRoll < 0.4 && digRate > 0.1)
					{
						loopBack = true;
						actionQueue.push_front(CreatureAction(CreatureAction::digTile));
					}

					// Decide to "wander" a short distance
					else if(diceRoll < 0.6)
					{
						loopBack = true;
						actionQueue.push_front(CreatureAction(CreatureAction::walkToTile));
						int tempX = position.x + 2.0*gaussianRandomDouble();
						int tempY = position.y + 2.0*gaussianRandomDouble();

						Tile *tempPositionTile = positionTile();
						list<Tile*> result;
						if(tempPositionTile != NULL)
						{
							result = gameMap.path(tempPositionTile->x, tempPositionTile->y, tempX, tempY, tilePassability);
						}

						gameMap.cutCorners(result, tilePassability);
						setWalkPath(result, 2, true);
					}
					else
					{
						// Remain idle
						//setAnimationState("Idle");
					}

					break;

				case CreatureAction::walkToTile:
					//TODO: Peek at the item that caused us to walk
					if(actionQueue[1].type == CreatureAction::digTile)
					{
						tempPlayer = getControllingPlayer();
						// Check to see if the tile is still marked for digging
						int index = walkQueue.size();
						Tile *currentTile = gameMap.getTile((int)walkQueue[index].x, (int)walkQueue[index].y);
						if(currentTile != NULL)
						{
							// If it is not marked
							if(tempPlayer != NULL && !currentTile->getMarkedForDigging(tempPlayer))
							{
								// Clear the walk queue
								clearDestinations();
							}
						}
					}

					//cout << "walkToTile ";
					if(walkQueue.size() == 0)
					{
						actionQueue.pop_front();
						loopBack = true;
					}
					break;

				case CreatureAction::claimTile:
					myTile = positionTile();
					//NOTE:  This is a workaround for the problem with the positionTile() function,
					// it can be removed when that issue is resolved.
					if(myTile == NULL)
					{
						actionQueue.pop_front();
						goto claimTileBreakStatement;
					}

					// Randomly decide to stop claiming with a small probability
					if(randomDouble(0.0, 1.0) < 0.1 + 0.2*markedTiles.size())
					{
						loopBack = true;
						actionQueue.pop_front();

						// If there are any visible tiles marked for digging start working on that.
						if(markedTiles.size() > 0)
							actionQueue.push_front(CreatureAction(CreatureAction::digTile));

						break;
					}

					// See if the tile we are standing on can be claimed
					if(myTile->color != color || myTile->colorDouble < 1.0)
					{
						//cout << "\nTrying to claim the tile I am standing on.";
						// Check to see if one of the tile's neighbors is claimed for our color
						neighbors = gameMap.neighborTiles(myTile->x, myTile->y);
						for(unsigned int j = 0; j < neighbors.size(); j++)
						{
							// Check to see if the current neighbor is already claimed
							tempTile = neighbors[j];
							if(tempTile->color == color && tempTile->colorDouble >= 1.0)
							{
								//cout << "\t\tFound a neighbor that is claimed.";
								// If we found a neighbor that is claimed for our side than we
								// can start dancing on this tile
								if(myTile->color == color)
								{
									//cout << "\t\tmyTile is My color.";
									myTile->colorDouble += danceRate;
									if(myTile->colorDouble >= 1.0)
									{
										// Claim the tile and finish claiming
										//FIXME:  Change Dig to Dance
										setAnimationState("Dig");
										myTile->colorDouble = 1.0;
										myTile->setType(Tile::claimed);
									}
								}
								else
								{
									myTile->colorDouble -= danceRate;
									if(myTile->colorDouble <= 0.0)
									{
										myTile->colorDouble *= -1.0;
										myTile->color = color;
									}
								}

								// Since we danced on a tile we are done for this turn
								goto claimTileBreakStatement;
							}
						}
					}

					//cout << "\nLooking at the neighbor tiles to see if I can claim a tile.";
					// The tile we are standing on is already claimed or is not currently
					// claimable, find candidates for claiming.
					// Start by checking the neighbor tiles of the one we are already in
					neighbors = gameMap.neighborTiles(myTile->x, myTile->y);
					while(neighbors.size() > 0)
					{
						// If the current neigbor is claimable, walk into it and skip to the end of this turn
						tempInt = randomUint(0, neighbors.size()-1);
						tempTile = neighbors[tempInt];
						if(tempTile != NULL && tempTile->getTilePassability() == Tile::walkableTile && (tempTile->color != color || tempTile->colorDouble < 1.0))
						{
							// The neighbor tile is a potential candidate for claiming, to be an actual candidate
							// though it must have a neighbor of its own that is already claimed for our side.
							neighbors2 = gameMap.neighborTiles(tempTile->x, tempTile->y);
							for(unsigned int i = 0; i < neighbors2.size(); i++)
							{
								tempTile2 = neighbors2[i];
								if(tempTile2->color == color && tempTile2->colorDouble >= 1.0)
								{
									clearDestinations();
									addDestination(tempTile->x, tempTile->y);
									setAnimationState("Walk");
									goto claimTileBreakStatement;
								}
							}
						}

						neighbors.erase(neighbors.begin()+tempInt);
					}

					//cout << "\nLooking at the visible tiles to see if I can claim a tile.";
					// If we still haven't found a tile to claim, check the rest of the visible tiles
					for(unsigned int i = 0; i < visibleTiles.size(); i++)
					{
						// if this tile is not fully claimed yet or the tile is of another player's color
						tempTile = visibleTiles[i];
						if(tempTile != NULL && tempTile->getTilePassability() == Tile::walkableTile && (tempTile->colorDouble < 1.0 || tempTile->color != color))
						{
							// Check to see if one of the tile's neighbors is claimed for our color
							neighbors = gameMap.neighborTiles(visibleTiles[i]->x, visibleTiles[i]->y);
							for(unsigned int j = 0; j < neighbors.size(); j++)
							{
								tempTile = neighbors[j];
								if(tempTile->color == color && tempTile->colorDouble >= 1.0)
								{
									claimableTiles.push_back(tempTile);
								}
							}
						}
					}

					//cout << "  I see " << claimableTiles.size() << " tiles I can claim.";
					// Randomly pick a claimable tile, plot a path to it and walk to it
					while(claimableTiles.size() > 0)
					{
						// Randomly find a "good" tile to claim.  A good tile is one that has many neighbors
						// already claimed, this makes the claimed are more "round" and less jagged.
						tempUnsigned = 0;
						do
						{
							int numNeighborsClaimed;

							// Start by randomly picking a candidate tile.
							tempTile = claimableTiles[randomUint(0, claimableTiles.size()-1)];

							// Count how many of the candidate tile's neighbors are already claimed.
							neighbors = gameMap.neighborTiles(tempTile->x, tempTile->y);
							numNeighborsClaimed = 0;
							for(unsigned int i = 0; i < neighbors.size(); i++)
							{
								if(neighbors[i]->color == color && neighbors[i]->colorDouble >= 1.0)
									numNeighborsClaimed++;
							}

							// Pick a random number in [0:1], if this number is high enough, than use this tile to claim.  The
							// bar for success approaches 0 as numTiles approaches N so this will be guaranteed to succeed at,
							// or before the time we get to the last unclaimed tile.  The bar for success is also lowered
							// according to how many neighbors are already claimed.
							//NOTE: The bar can be negative, when this happens we are guarenteed to use this candidate tile.
							double bar;
							bar = 1.0 - (numNeighborsClaimed/4.0) - (tempUnsigned/(double)(claimableTiles.size()-1));
							if(randomDouble(0.0, 1.0) >= bar)
								break;

							// Safety catch to prevent infinite loop in case the bar for success is too high and is never met.
							if(tempUnsigned >= claimableTiles.size()-1)
								break;

							// Increment the counter indicating how many candidate tiles we have rejected so far.
							tempUnsigned++;
						} while(true);

						if(tempTile != NULL)
						{
							// If we find a valid path to the tile start walking to it and break
							tempPath = gameMap.path(myTile->x, myTile->y, tempTile->x, tempTile->y, tilePassability);
							gameMap.cutCorners(tempPath, tilePassability);
							if(setWalkPath(tempPath, 2, true))
							{
								//loopBack = true;
								actionQueue.push_back(CreatureAction::walkToTile);
								goto claimTileBreakStatement;
							}
						}

						// If we got to this point, the tile we randomly picked cannot be gotten to via a
						// valid path.  Delete it from the claimable tiles vector and repeat the outer
						// loop to try to find another valid tile.
						for(unsigned int i = 0; i < claimableTiles.size(); i++)
						{
						        if(claimableTiles[i] == tempTile)
						        {
							    claimableTiles.erase(claimableTiles.begin() + i);
							    break;  // Break out of this for loop.
						        }
						}
					}

					// We couldn't find a tile to try to claim so we stop trying
					actionQueue.pop_front();
claimTileBreakStatement:
					break;

				case CreatureAction::digTile:
					tempPlayer = getControllingPlayer();
					//cout << "dig ";

					// Randomly decide to stop digging with a small probability
					if(randomDouble(0.0, 1.0) < 0.5 - 0.2*markedTiles.size())
					{
						loopBack = true;
						actionQueue.pop_front();
						goto claimTileBreakStatement;
					}

					// See if any of the tiles is one of our neighbors
					wasANeighbor = false;
					creatureNeighbors = gameMap.neighborTiles(position.x, position.y);
					for(unsigned int i = 0; i < creatureNeighbors.size() && !wasANeighbor; i++)
					{
						if(tempPlayer != NULL && creatureNeighbors[i]->getMarkedForDigging(tempPlayer))
						{
							setAnimationState("Dig");
							creatureNeighbors[i]->setFullness(creatureNeighbors[i]->getFullness() - digRate);

							// Force all the neighbors to recheck their meshes as we may have exposed
							// a new side that was not visible before.
							neighbors = gameMap.neighborTiles(creatureNeighbors[i]->x, creatureNeighbors[i]->y);
							for(unsigned int j = 0; j < neighbors.size(); j++)
							{
								neighbors[j]->setFullness(neighbors[j]->getFullness());
							}

							if(creatureNeighbors[i]->getFullness() < 0)
							{
								creatureNeighbors[i]->setFullness(0);
							}

							// If the tile has been dug out, move into that tile and idle
							if(creatureNeighbors[i]->getFullness() == 0)
							{
								addDestination(creatureNeighbors[i]->x, creatureNeighbors[i]->y);
								setAnimationState("Walk");

								// Remove the dig action and replace it with
								// walking to the newly dug out tile.
								actionQueue.pop_front();
								actionQueue.push_front(CreatureAction(CreatureAction::walkToTile));
							}

							wasANeighbor = true;
							break;
						}
					}

					if(wasANeighbor)
						break;

					// Randomly decide to stop digging with a larger probability
					if(randomDouble(0.0, 1.0) < 0.3)
					{
						loopBack = true;
						actionQueue.pop_front();
						goto claimTileBreakStatement;
					}

					// Find paths to all of the neighbor tiles for all of the marked visible tiles.
					possiblePaths.clear();
					for(unsigned int i = 0; i < markedTiles.size(); i++)
					{
						neighbors = gameMap.neighborTiles(markedTiles[i]->x, markedTiles[i]->y);
						for(unsigned int j = 0; j < neighbors.size(); j++)
						{
							neighborTile = neighbors[j];
							if(neighborTile != NULL && neighborTile->getFullness() == 0)
								possiblePaths.push_back(gameMap.path(positionTile()->x, positionTile()->y, neighborTile->x, neighborTile->y, tilePassability));

						}
					}

					// Find the shortest path and start walking toward the tile to be dug out
					if(possiblePaths.size() > 0)
					{
						// Find the N shortest valid paths, see if there are any valid paths shorter than this first guess
						shortPaths.clear();
						for(unsigned int i = 0; i < possiblePaths.size(); i++)
						{
							// If the current path is long enough to be valid
							unsigned int currentLength = possiblePaths[i].size();
							if(currentLength >= 2)
							{
								shortPaths.push_back(possiblePaths[i]);

								// If we already have enough short paths
								if(shortPaths.size() > 5)
								{
									unsigned int longestLength, longestIndex;

									// Kick out the longest
									longestLength = shortPaths[0].size();
									longestIndex = 0;
									for(unsigned int j = 1; j < shortPaths.size(); j++)
									{
										if(shortPaths[j].size() > longestLength)
										{
											longestLength = shortPaths.size();
											longestIndex = j;
										}
									}

									shortPaths.erase(shortPaths.begin() + longestIndex);
								}
							}
						}

						// Randomly pick a short path to take
						unsigned int numShortPaths = shortPaths.size();
						if(numShortPaths > 0)
						{
							unsigned int shortestIndex;
							shortestIndex = randomUint(0, numShortPaths-1);
							walkPath = shortPaths[shortestIndex];

							// If the path is a legitimate path, walk down it to the tile to be dug out
							gameMap.cutCorners(walkPath, tilePassability);
							if(setWalkPath(walkPath, 2, false))
							{
								actionQueue.push_front(CreatureAction(CreatureAction::walkToTile));
								break;
							}
						}
					}

					// If none of our neighbors are marked for digging we got here too late.
					// Finish digging
					if(actionQueue.front().type == CreatureAction::digTile)
					{
						actionQueue.pop_front();
						loopBack = true;
					}
					break;

				case CreatureAction::attackCreature:
					setAnimationState("Attack1");

					// If there are no more enemies which are reachable, stop attacking
					if(reachableEnemies.size() == 0)
					{
						actionQueue.pop_front();
						loopBack = true;
						break;
					}

					myTile = positionTile();

					// Find the first enemy close enough to hit and attack it
					if(enemiesInRange.size() > 0)
					{
						//FIXME: We should only do as much damage as is allowed by the weapon ranges in case one is in range and one is not.
						double damageDone = weaponL->damage + weaponR->damage;
						damageDone = randomInt(0, (int)damageDone);
						enemiesInRange[0]->hp -= damageDone;
						cout << "\n" << name << " did " << damageDone << " damage to " << enemiesInRange[0]->name;
						cout << " who now has " << enemiesInRange[0]->hp << "hp";

						// Randomly decide to start maneuvering again so we don't just stand still and fight.
						if(randomDouble(0.0, 1.0) <= 0.6)
							actionQueue.pop_front();

						break;
					}

					// There is not an enemy within range, begin maneuvering to try to get near an enemy, or out of the combat situation.
					actionQueue.push_front(CreatureAction(CreatureAction::maneuver));
					loopBack = true;
					break;

				case CreatureAction::maneuver:
					setAnimationState("Walk");

					// If there are no more enemies which are reachable, stop maneuvering.
					if(reachableEnemies.size() == 0)
					{
						actionQueue.pop_front();
						loopBack = true;
						break;
					}

					// If there is an enemy within range, stop maneuvering and attack it.
					if(enemiesInRange.size() > 0)
					{
						actionQueue.pop_front();
						loopBack = true;

						// If the next action down the stack is not an attackCreature action, add it.
						if(actionQueue.front().type != CreatureAction::attackCreature)
							actionQueue.push_front(CreatureAction(CreatureAction::attackCreature));

						break;
					}

					// Prepare the battlefield so we can decide where to move.
					computeBattlefield();

					// Move to the desired location on the battlefield, a minumum if we are
					// trying to "attack" and a maximum if we are trying to "retreat".
					clearDestinations();
				       	min = battleField->min();  // Currently always attack, never retreat.
					tempDouble = 4;
					tempPath = gameMap.path(positionTile()->x, positionTile()->y, min.first.first + randomDouble(-1.0*tempDouble,tempDouble), min.first.second + randomDouble(-1.0*tempDouble, tempDouble), tilePassability);

					// Walk a maximum of 5 tiles before recomputing the destination since we are in combat.
					if(tempPath.size() >= 5)
						tempPath.resize(5);

					gameMap.cutCorners(tempPath, tilePassability);
					setWalkPath(tempPath, 2, true);

					// This is a debugging statement, it produces a visual display of the battlefield as seen by the first creature.
					if(battleField->name.compare("field_1") == 0)
					{
						//battleField->refreshMeshes(0.0);
					}
					break;

				default:
					cerr << "\n\nERROR:  Unhandled action type in Creature::doTurn().\n\n";
					exit(1);
					break;
			}
		}
		else
		{
			cerr << "\n\nERROR:  Creature has empty action queue in doTurn(), this should not happen.\n\n";
			exit(1);
		}

	} while(loopBack);

	// Update the visual debugging entities
	if(hasVisualDebuggingEntities)
	{
		// if we are standing in a different tile than we were last turn
		Tile *currentPositionTile = positionTile();
		if(currentPositionTile != previousPositionTile)
		{
			//TODO:  This destroy and re-create is kind of a hack as its likely only a few tiles will actually change.
			destroyVisualDebugEntities();
			createVisualDebugEntities();
		}
	}
}

/*! \brief Creates a list of Tile pointers in visibleTiles
 *
 * The tiles are currently determined to be visible or not, according only to
 * the distance they are away from the creature.  Because of this they can
 * currently see through walls, etc.
*/
void Creature::updateVisibleTiles()
{
	//int xMin, yMin, xMax, yMax;
	const double sightRadiusSquared = sightRadius * sightRadius;
	Tile *tempPositionTile = positionTile();
	Tile *currentTile;
	int xBase = tempPositionTile->x;
	int yBase = tempPositionTile->y;
	int xLoc, yLoc;

	visibleTiles.clear();

	// Add the tile the creature is standing in
	if(tempPositionTile != NULL)
	{
		visibleTiles.push_back(tempPositionTile);
	}

	// Add the 4 principle axes rays
	for(int i = 1; i < sightRadius; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			switch(j)
			{
				case 0:  xLoc = xBase+i;   yLoc = yBase;  break;
				case 1:  xLoc = xBase-i;   yLoc = yBase;  break;
				case 2:  xLoc = xBase;   yLoc = yBase+i;  break;
				case 3:  xLoc = xBase;   yLoc = yBase-i;  break;
			}

			currentTile = gameMap.getTile(xLoc, yLoc);

			if(currentTile != NULL)
			{
				// Check if we can actually see the tile in question
				// or if it is blocked by terrain
				if(tempPositionTile != NULL && gameMap.pathIsClear(gameMap.lineOfSight(tempPositionTile->x, tempPositionTile->y, xLoc, yLoc), Tile::flyableTile))
				{
					visibleTiles.push_back(currentTile);
				}
				else
				{
					// If we cannot see this tile than we cannot see any tiles farther away 
					// than this one (in this direction) so move on to the next direction.
					continue;
				}
			}
		}
	}

	// Fill in the 4 pie slice shaped sectors
	for(int i = 1; i < sightRadius; i++)
	{
		for(int j = 1; j < sightRadius; j++)
		{
			// Check to see if the current tile is actually close enough to be visible
			int distSQ = i*i + j*j;
			if(distSQ < sightRadiusSquared)
			{
				for(int k = 0; k < 4; k++)
				{
					switch(k)
					{
						case 0:  xLoc = xBase+i;   yLoc = yBase+j;  break;
						case 1:  xLoc = xBase+i;   yLoc = yBase-j;  break;
						case 2:  xLoc = xBase-i;   yLoc = yBase+j;  break;
						case 3:  xLoc = xBase-i;   yLoc = yBase-j;  break;
					}

					currentTile = gameMap.getTile(xLoc, yLoc);
					
					if(currentTile != NULL)
					{
						// Check if we can actually see the tile in question
						// or if it is blocked by terrain
						if(tempPositionTile != NULL && gameMap.pathIsClear(gameMap.lineOfSight(tempPositionTile->x, tempPositionTile->y, xLoc, yLoc), Tile::flyableTile))
						{
							visibleTiles.push_back(currentTile);
						}
					}
				}
			}
			else
			{
				// If this tile is too far away then any tile with a j value greater than this
				// will also be too far away.
				break;
			}
		}
	}

	//TODO:  Add the sector shaped region of the visible region
}

/*! \brief Loops over the visibleTiles and adds all enemy creatures in each tile to a list which it returns.
 *
*/
vector<Creature*> Creature::getVisibleEnemies()
{
	return getVisibleForce(color, true);
}

/*! \brief Loops over creaturesToCheck and returns a vector containing all the ones which can be reached via a valid path.
 *
*/
vector<Creature*> Creature::getReachableCreatures(const vector<Creature*> &creaturesToCheck)
{
	vector<Creature*> tempVector;
	Tile *myTile = positionTile(), *creatureTile;
	list<Tile*> tempPath;

	// Loop over the vector of creatures we are supposed to check.
	for(unsigned int i = 0; i < creaturesToCheck.size(); i++)
	{
		// Try to find a valid path from the tile this creature is in to the tile where the current target creature is standing.
		creatureTile = creaturesToCheck[i]->positionTile();
		tempPath = gameMap.path(myTile->x, myTile->y, creatureTile->x, creatureTile->y, tilePassability);

		// If the path we found is valid, then add the creature to the ones we return.
		if(tempPath.size() >= 2)
			tempVector.push_back(creaturesToCheck[i]);
	}

	return tempVector;
}

/*! \brief Loops over the enemiesToCheck vector and adds all enemy creatures within weapons range to a list which it returns.
 *
*/
vector<Creature*> Creature::getEnemiesInRange(const vector<Creature*> &enemiesToCheck)
{
	vector<Creature*> tempVector;

	// If there are no enemies to check we are done.
	if(enemiesToCheck.size() == 0)
		return tempVector;

	// Find our location and calculate the square of the max weapon range we have.
	Tile *myTile = positionTile();
	double weaponRangeSquared = max(weaponL->range, weaponR->range);
	weaponRangeSquared *= weaponRangeSquared;

	// Loop over the enemiesToCheck and add any within range to the tempVector.
	for(unsigned int i = 0; i < enemiesToCheck.size(); i++)
	{
		Tile *tempTile = enemiesToCheck[i]->positionTile();
		double rSquared = powl(myTile->x - tempTile->x, 2.0) + powl(myTile->y - tempTile->y, 2.0);
		if(rSquared < weaponRangeSquared)
			tempVector.push_back(enemiesToCheck[i]);
	}

	return tempVector;
}

/*! \brief Loops over the visibleTiles and adds all allied creatures in each tile to a list which it returns.
 *
*/
vector<Creature*> Creature::getVisibleAllies()
{
	return getVisibleForce(color, false);
}

/*! \brief Loops over the visibleTiles and adds any which are marked for digging to a vector which it returns.
 *
*/
vector<Tile*> Creature::getVisibleMarkedTiles()
{
	vector<Tile*> tempVector;
	Player *tempPlayer = getControllingPlayer();

	// Loop over all the visible tiles.
	for(unsigned int i = 0; i < visibleTiles.size(); i++)
	{
		// Check to see if the tile is marked for digging.
		if(tempPlayer != NULL && visibleTiles[i]->getMarkedForDigging(tempPlayer))
			tempVector.push_back(visibleTiles[i]);
	}
	
	return tempVector;
}

/*! \brief Loops over the visibleTiles and returns any creatures in those tiles whose color matches (or if invert is true, does not match) the given color parameter.
 *
*/
vector<Creature*> Creature::getVisibleForce(int color, bool invert)
{
	vector<Creature*> returnList;

	// Loop over the visible tiles
	vector<Tile*>::iterator itr;
	for(itr = visibleTiles.begin(); itr != visibleTiles.end(); itr++)
	{
		// Loop over the creatures in the given tile
		for(unsigned int i = 0; i < (*itr)->numCreaturesInCell(); i++)
		{
			Creature *tempCreature = (*itr)->getCreature(i);
			// If it is an enemy
			if(tempCreature != NULL)
			{
				if(invert)
				{
					if(tempCreature->color != color)
					{
						// Add the current creature
						returnList.push_back(tempCreature);
					}
				}
				else
				{
					if(tempCreature->color == color)
					{
						// Add the current creature
						returnList.push_back(tempCreature);
					}
				}
			}
		}
	}

	return returnList;
}

/*! \brief Displays a mesh on all of the tiles visible to the creature.
 *
*/
void Creature::createVisualDebugEntities()
{
	hasVisualDebuggingEntities = true;
	visualDebugEntityTiles.clear();

	Tile *currentTile = NULL;
	updateVisibleTiles();
	for(unsigned int i = 0; i < visibleTiles.size(); i++)
	{
		currentTile = visibleTiles[i];

		if(currentTile != NULL)
		{
			// Create a render request to create a mesh for the current visible tile.
			RenderRequest *request = new RenderRequest;
			request->type = RenderRequest::createCreatureVisualDebug;
			request->p = currentTile;
			request->p2 = this;

			// Add the request to the queue of rendering operations to be performed before the next frame.
			queueRenderRequest(request);

			visualDebugEntityTiles.push_back(currentTile);

		}
	}
}

/*! \brief Destroy the meshes created by createVisualDebuggingEntities().
 *
*/
void Creature::destroyVisualDebugEntities()
{
	hasVisualDebuggingEntities = false;

	Tile *currentTile = NULL;
	updateVisibleTiles();
	list<Tile*>::iterator itr;
	for(itr = visualDebugEntityTiles.begin(); itr != visualDebugEntityTiles.end(); itr++)
	{
		currentTile = *itr;

		if(currentTile != NULL)
		{
			// Destroy the mesh for the current visible tile
			RenderRequest *request = new RenderRequest;
			request->type = RenderRequest::destroyCreatureVisualDebug;
			request->p = currentTile;
			request->p2 = this;

			// Add the request to the queue of rendering operations to be performed before the next frame.
			queueRenderRequest(request);
		}
	}

}

/*! \brief Returns a pointer to the tile the creature is currently standing in.
 *
*/
Tile* Creature::positionTile()
{
	return gameMap.getTile((int)(position.x), (int)(position.y));
}

/*! \brief Completely destroy this creature, including its OGRE entities, scene nodes, etc.
 *
*/
void Creature::deleteYourself()
{
	weaponL->destroyMesh();
	weaponR->destroyMesh();

	if(positionTile() != NULL)
		positionTile()->removeCreature(this);

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyCreature;
	request->p = this;

	RenderRequest *request2 = new RenderRequest;
	request2->type = RenderRequest::deleteCreature;
	request2->p = this;

	// Add the requests to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
	queueRenderRequest(request2);
}

/*! \brief Sets a new animation state from the creature's library of animations.
 *
*/
void Creature::setAnimationState(string s)
{
	string tempString;
	stringstream tempSS;
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::setCreatureAnimationState;
	request->p = this;
	request->str = s;

	if(serverSocket != NULL)
	{
		try
		{
			// Place a message in the queue to inform the clients about the new animation state
			ServerNotification *serverNotification = new ServerNotification;
			serverNotification->type = ServerNotification::creatureSetAnimationState;
			serverNotification->str = s;
			serverNotification->cre = this;

			sem_wait(&serverNotificationQueueLockSemaphore);
			serverNotificationQueue.push_back(serverNotification);
			sem_post(&serverNotificationQueueLockSemaphore);

			sem_post(&serverNotificationQueueSemaphore);
		}
		catch(bad_alloc&)
		{
			cerr << "\n\nERROR:  bad alloc in Creature::setAnimationState\n\n";
			exit(1);
		}
	}

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

/*! \brief Returns the creature's currently active animation state.
 *
*/
AnimationState* Creature::getAnimationState()
{
	return animationState;
}

/*! \brief Adds a position in 3d space to the creature's walk queue and, if necessary, starts it walking.
 *
 * This function also places a message in the serverNotificationQueue so that
 * relevant clients are informed about the change.
*/
void Creature::addDestination(int x, int y)
{
	//cout << "w(" << x << ", " << y << ") ";
	Ogre::Vector3 destination(x, y, 0);

	// if there are currently no destinations in the walk queue
	if(walkQueue.size() == 0)
	{
		// Add the destination and set the remaining distance counter
		walkQueue.push_back(destination);
		shortDistance = position.distance(walkQueue.front());

		// Rotate the creature to face the direction of the destination
		walkDirection = walkQueue.front() - position;
		walkDirection.normalise();

		//TODO:  this is OGRE rendering code and it should be moved to the RenderRequest system
		SceneNode *node = mSceneMgr->getSceneNode(name + "_node");
		Ogre::Vector3 src = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;

		// Work around 180 degree quaternion rotation quirk
		if ((1.0f + src.dotProduct(walkDirection)) < 0.0001f)
		{
			node->roll(Degree(180));
		}
		else
		{
			Quaternion quat = src.getRotationTo(walkDirection);
			node->rotate(quat);
		}
	}
	else
	{
		// Add the destination
		walkQueue.push_back(destination);
	}

	if(serverSocket != NULL)
	{
		try
		{
			// Place a message in the queue to inform the clients about the new destination
			ServerNotification *serverNotification = new ServerNotification;
			serverNotification->type = ServerNotification::creatureAddDestination;
			serverNotification->str = name;
			serverNotification->vec = destination;

			sem_wait(&serverNotificationQueueLockSemaphore);
			serverNotificationQueue.push_back(serverNotification);
			sem_post(&serverNotificationQueueLockSemaphore);

			sem_post(&serverNotificationQueueSemaphore);
		}
		catch(bad_alloc&)
		{
			cerr << "\n\nERROR:  bad alloc in Creature::addDestination\n\n";
			exit(1);
		}
	}
}

/*! \brief Replaces a creature's current walk queue with a new path.
 *
 * This replacement is done if, and only if, the new path is at least minDestinations
 * long; if addFirstStop is false the new path will start with the second entry in path.
*/
bool Creature::setWalkPath(list<Tile*> path, unsigned int minDestinations, bool addFirstStop)
{
	// Remove any existing stops from the walk queue.
	clearDestinations();

	// Verify that the given path is long enough to be considered valid.
	if(path.size() >= minDestinations)
	{
		setAnimationState("Walk");
		list<Tile*>::iterator itr = path.begin();

		// If we are not supposed to add the first tile in the path to the destination queue, then we skip over it.
		if(!addFirstStop)
			itr++;

		// Loop over the path adding each tile as a destination in the walkQueue.
		while(itr != path.end())
		{
			addDestination((*itr)->x, (*itr)->y);
			itr++;
		}

		return true;
	}
	else
	{
		setAnimationState("Idle");
		return false;
	}

	return true;
}

/*! \brief Clears all future destinations from the walk queue, stops the creature where it is, and sets its animation state.
 *
*/
void Creature::clearDestinations()
{
	walkQueue.clear();
	stopWalking();

	if(serverSocket != NULL)
	{
		// Place a message in the queue to inform the clients about the clear
		ServerNotification *serverNotification = new ServerNotification;
		serverNotification->type = ServerNotification::creatureClearDestinations;
		serverNotification->cre = this;

		sem_wait(&serverNotificationQueueLockSemaphore);
		serverNotificationQueue.push_back(serverNotification);
		sem_post(&serverNotificationQueueLockSemaphore);

		sem_post(&serverNotificationQueueSemaphore);
	}
}

/*! \brief Stops the creature where it is, and sets its animation state.
 *
*/
void Creature::stopWalking()
{
	walkDirection = Ogre::Vector3::ZERO;
	setAnimationState("Idle");
}

/*! \brief An accessor to return whether or not the creature has OGRE entities for its visual debugging entities.
 *
*/
bool Creature::getHasVisualDebuggingEntities()
{
	return hasVisualDebuggingEntities;
}

/*! \brief Returns the first player whose color matches this creature's color.
 *
*/
Player* Creature::getControllingPlayer()
{
	Player *tempPlayer;

	if(gameMap.me->seat->color == color)
	{
		return gameMap.me;
	}

	// Try to find and return a player with color equal to this creature's
	for(unsigned int i = 0; i < gameMap.numPlayers(); i++)
	{
		tempPlayer = gameMap.getPlayer(i);
		if(tempPlayer->seat->color == color)
		{
			return tempPlayer;
		}
	}

	// No player found, return NULL
	return NULL;
}

/*! \brief Clears the action queue, except for the Idle action at the end.
 *
*/
void Creature::clearActionQueue()
{
	actionQueue.clear();
	actionQueue.push_back(CreatureAction(CreatureAction::idle));
}

/** \brief This function loops over the visible tiles and computes a score for each one indicating how
  * frindly or hostile that tile is and stores it in the battleField variable.
  *
*/
void Creature::computeBattlefield()
{
	Tile *myTile, *tempTile, *tempTile2;
	double rSquared;

	// Loop over the tiles in this creature's battleField and compute their value.
	// The creature will then walk towards the tile with the minimum value to
	// attack or towards the maximum value to retreat.
	myTile = positionTile();
	battleField->clear();
	for(unsigned int i = 0; i < visibleTiles.size(); i++)
	{
		tempTile = visibleTiles[i];
		double tileValue = 0.0;// - sqrt(rSquared)/sightRadius;

		// Enemies
		for(unsigned int j = 0; j < reachableEnemies.size(); j++)
		{
			Tile *tempTile2 = reachableEnemies[j]->positionTile();

			// Compensate for how close the creature is to me
			//rSquared = powl(myTile->x - tempTile2->x, 2.0) + powl(myTile->y - tempTile2->y, 2.0);
			//double factor = 1.0 / (sqrt(rSquared) + 1.0);

			// Subtract for the distance from the enemy creature to r
			rSquared = powl(tempTile->x - tempTile2->x, 2.0) + powl(tempTile->y - tempTile2->y, 2.0);
			tileValue += sqrt(rSquared);
		}

		// Allies
		for(unsigned int j = 0; j < visibleAllies.size(); j++)
		{
			Tile *tempTile2 = visibleAllies[j]->positionTile();

			// Compensate for how close the creature is to me
			//rSquared = powl(myTile->x - tempTile2->x, 2.0) + powl(myTile->y - tempTile2->y, 2.0);
			//double factor = 1.0 / (sqrt(rSquared) + 1.0);

			rSquared = powl(tempTile->x - tempTile2->x, 2.0) + powl(tempTile->y - tempTile2->y, 2.0);
			tileValue += 15.0 / (sqrt(rSquared+1.0));
		}

		double jitter = 0.05;
		double tileScaleFactor = 0.05;
		battleField->set(tempTile->x, tempTile->y, (tileValue + randomDouble(-1.0*jitter, jitter))*tileScaleFactor);
	}
}

