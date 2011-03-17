#include <cmath>
#include <algorithm>


#include <CEGUIWindow.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>

#include "Creature.h"
#include "Defines.h"
#include "Globals.h"
#include "Functions.h"
#include "CreatureAction.h"
#include "Network.h"
#include "Field.h"
#include "Weapon.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "SoundEffectsHelper.h"
#include "CreatureSound.h"
#include "Player.h"
#include "Seat.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf _snprintf
#endif

using CEGUI::UDim;
using CEGUI::UVector2;

Creature::Creature()
{
    sem_init(&hpLockSemaphore, 0, 1);
    sem_init(&manaLockSemaphore, 0, 1);
    sem_init(&isOnMapLockSemaphore, 0, 1);
    sem_init(&actionQueueLockSemaphore, 0, 1);
    sem_init(&statsWindowLockSemaphore, 0, 1);

    sem_wait(&statsWindowLockSemaphore);
    statsWindow = NULL;
    sem_post(&statsWindowLockSemaphore);

    sem_wait(&isOnMapLockSemaphore);
    isOnMap = false;
    sem_post(&isOnMapLockSemaphore);

    hasVisualDebuggingEntities = false;

    scale = Ogre::Vector3(1, 1, 1);
    sightRadius = 10.0;
    digRate = 10.0;
    exp = 0.0;
    level = 1;
    danceRate = 0.35;
    destinationX = 0;
    destinationY = 0;

    setHP(10.0);
    setMana(10.0);

    maxHP = 10;
    maxMana = 10;
    hpPerLevel = 0.0;
    manaPerLevel = 0.0;
    gold = 0;
    sightRadius = 10;
    digRate = 10;
    moveSpeed = 1.0;
    tilePassability = Tile::walkableTile;
    homeTile = NULL;
    trainingDojo = NULL;
    trainWait = 0;

    weaponL = NULL;
    weaponR = NULL;

    sceneNode = NULL;

    pushAction(CreatureAction::idle);

    battleField = new Field("autoname");
    battleFieldAgeCounter = 0;

    meshesExist = false;

    //static int uniqueId = 0;

    //Create sound object
    sound = SoundEffectsHelper::getSingleton().createCreatureSound(getName());

    awakeness = 100.0;
    deathCounter = 10;

    prevAnimationState = "";
    prevAnimationStateLoop = true;
}

/*  This function causes a segfault in Creature::doTurn() when computeBattlefield() is called.
 Creature::~Creature()
 {
 if(battleField != NULL)
 {
 delete battleField;
 battleField = NULL;
 }
 }
 */

/** \brief A function which returns a string describing the IO format of the << and >> operators.
 *
 */
std::string Creature::getFormat()
{
    //NOTE:  When this format changes changes to RoomPortal::spawnCreature() may be necessary.
    string tempString = "className\tname\tposX\tposY\tposZ\tcolor\tweaponL";
    tempString += Weapon::getFormat();
    tempString += "\tweaponR";
    tempString += Weapon::getFormat();
    tempString += "\tHP\tmana";

    return tempString;
}

/*! \brief A matched function to transport creatures between files and over the network.
 *
 */
std::ostream& operator<<(std::ostream& os, Creature *c)
{
    os << c->className << "\t" << c->name << "\t";

    sem_wait(&c->positionLockSemaphore);
    os << c->position.x << "\t" << c->position.y << "\t" << c->position.z
            << "\t";
    sem_post(&c->positionLockSemaphore);

    os << c->color << "\t";
    os << c->weaponL << "\t" << c->weaponR << "\t";

    os << c->getHP(NULL) << "\t";

    os << c->getMana();

    return os;
}

/*! \brief A matched function to transport creatures between files and over the network.
 *
 */
std::istream& operator>>(std::istream& is, Creature *c)
{
    double xLocation = 0.0, yLocation = 0.0, zLocation = 0.0;
    double tempDouble;
    std::string tempString;

    is >> c->className;
    is >> tempString;

    if (tempString.compare("autoname") == 0)
        tempString = c->getUniqueCreatureName();

    c->name = tempString;

    is >> xLocation >> yLocation >> zLocation;
    c->setPosition(xLocation, yLocation, zLocation);

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
    CreatureClass *creatureClass = gameMap.getClassDescription(c->className);
    if (creatureClass != NULL)
    {
        *c = *creatureClass;
    }

    is >> tempDouble;
    c->setHP(tempDouble);

    is >> tempDouble;
    c->setMana(tempDouble);

    return is;
}

Creature Creature::operator=(CreatureClass c2)
{
    creatureJob = c2.creatureJob;
    className = c2.className;
    meshName = c2.meshName;
    scale = c2.scale;
    sightRadius = c2.sightRadius;
    digRate = c2.digRate;
    danceRate = c2.danceRate;
    hpPerLevel = c2.hpPerLevel;
    manaPerLevel = c2.manaPerLevel;
    setMoveSpeed(c2.getMoveSpeed());
    maxHP = c2.maxHP;
    maxMana = c2.maxMana;
    bedMeshName = c2.bedMeshName;
    bedDim1 = c2.bedDim1;
    bedDim2 = c2.bedDim2;

    return *this;
}

/*! \brief Allocate storage for, load, and inform OGRE about a mesh for this creature.
 *
 *  This function is called after a creature has been loaded from hard disk,
 *  received from a network connection, or created during the game play by the
 *  game engine itself.
 */
void Creature::createMesh()
{
    if (meshesExist)
        return;

    meshesExist = true;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::createCreature;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

/*! \brief Free the mesh and inform the OGRE system that the mesh has been destroyed.
 *
 *  This function is primarily a helper function for other methods.
 */
void Creature::destroyMesh()
{
    destroyStatsWindow();

    if (!meshesExist)
        return;

    meshesExist = false;
    weaponL->destroyMesh();
    weaponR->destroyMesh();

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyCreature;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

/*! \brief Changes the creature's position to a new position.
 *
 *  This is an overloaded function which just calls Creature::setPosition(double x, double y, double z).
 */
void Creature::setPosition(Ogre::Vector3 v)
{
    setPosition(v.x, v.y, v.z);
}

/*! \brief Changes the creature's position to a new position.
 *
 *  Moves the creature to a new location in 3d space.  This function is
 *  responsible for informing OGRE anything it needs to know, as well as
 *  maintaining the list of creatures in the individual tiles.
 */
void Creature::setPosition(double x, double y, double z)
{
    // If we are on the gameMap we may need to update the tile we are in
    sem_wait(&isOnMapLockSemaphore);
    bool flag = isOnMap;
    sem_post(&isOnMapLockSemaphore);
    if (flag)
    {
        // We are on the map
        // Move the creature relative to its parent scene node.  We record the
        // tile the creature is in before and after the move to properly
        // maintain the results returned by the positionTile() function.
        Tile *oldPositionTile = positionTile();
        sem_wait(&positionLockSemaphore);
        position = Ogre::Vector3(x, y, z);
        sem_post(&positionLockSemaphore);
        Tile *newPositionTile = positionTile();

        if (oldPositionTile != newPositionTile)
        {
            if (oldPositionTile != NULL)
                oldPositionTile->removeCreature(this);

            if (positionTile() != NULL)
                positionTile()->addCreature(this);
        }
    }
    else
    {
        // We are not on the map
        sem_wait(&positionLockSemaphore);
        position = Ogre::Vector3(x, y, z);
        sem_post(&positionLockSemaphore);
    }

    //attackSound->setPosition(x, y, z);

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = name + "_node";
    request->vec = position;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

void Creature::setHP(double nHP)
{
    sem_wait(&hpLockSemaphore);
    hp = nHP;
    sem_post(&hpLockSemaphore);

    updateStatsWindow();
}

double Creature::getHP(Tile *tile)
{
    sem_wait(&hpLockSemaphore);
    double tempDouble = hp;
    sem_post(&hpLockSemaphore);

    return tempDouble;
}

void Creature::setMana(double nMana)
{
    sem_wait(&manaLockSemaphore);
    mana = nMana;
    sem_post(&manaLockSemaphore);

    updateStatsWindow();
}

double Creature::getMana()
{
    sem_wait(&manaLockSemaphore);
    double tempDouble = mana;
    sem_post(&manaLockSemaphore);

    return tempDouble;
}

double Creature::getMoveSpeed()
{
    return moveSpeed;
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
 * out of things to do.  Other actions such as 'walkToTile' or 'attackObject'
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
    std::vector<Tile*> markedTiles;
    std::list<Tile*> walkPath;
    std::list<Tile*> basePath;
    std::list<Tile*>::iterator tileListItr;
    std::vector<std::list<Tile*> > possiblePaths;
    std::vector<std::list<Tile*> > shortPaths;
    bool loopBack;
    bool stopUsingDojo;
    bool tempBool;
    int tempInt;
    unsigned int tempUnsigned;
    double tempDouble;
    //Creature *tempCreature;
    AttackableObject *tempAttackableObject;
    CreatureAction tempAction;
    Ogre::Vector3 tempVector;
    Ogre::Quaternion tempQuat;

    // Heal.
    sem_wait(&hpLockSemaphore);
    hp += 0.1;
    if (hp > maxHP)
        hp = maxHP;
    sem_post(&hpLockSemaphore);

    // Regenrate mana.
    sem_wait(&manaLockSemaphore);
    mana += 0.45;
    if (mana > maxMana)
        mana = maxMana;
    sem_post(&manaLockSemaphore);

    awakeness -= 0.15;

    // Check to see if we have earned enough experience to level up.
    while (exp >= 5 * level + 5 * powl(level / 3.0, 2))
        doLevelUp();

    // If we are not standing somewhere on the map, do nothing.
    if (positionTile() == NULL)
        return;

    // Look at the surrounding area
    updateVisibleTiles();
    visibleEnemyObjects = getVisibleEnemyObjects();
    reachableEnemyObjects = getReachableAttackableObjects(visibleEnemyObjects,
            NULL, NULL);
    enemyObjectsInRange = getEnemyObjectsInRange(visibleEnemyObjects);
    livingEnemyObjectsInRange = AttackableObject::removeDeadObjects(
            enemyObjectsInRange);
    visibleAlliedObjects = getVisibleAlliedObjects();
    reachableAlliedObjects = getReachableAttackableObjects(
            visibleAlliedObjects, NULL, NULL);
    if (isWorker())
        markedTiles = getVisibleMarkedTiles();

    // If the creature can see enemies that are reachable.
    if (reachableEnemyObjects.size() > 0)
    {
        // Check to see if there is any combat actions (maneuvering/attacking) in our action queue.
        bool alreadyFighting = false;
        sem_wait(&actionQueueLockSemaphore);
        for (unsigned int i = 0; i < actionQueue.size(); ++i)
        {
            if (actionQueue[i].type == CreatureAction::attackObject
                    || actionQueue[i].type == CreatureAction::maneuver)
            {
                alreadyFighting = true;
                break;
            }
        }
        sem_post(&actionQueueLockSemaphore);

        // If we are not already fighting with a creature or maneuvering then start doing so.
        if (!alreadyFighting)
        {
            if (isWorker())
                tempDouble = 0.05;
            else
                tempDouble = 0.8;

            if (randomDouble(0.0, 1.0) < tempDouble)
            {
                tempAction.type = CreatureAction::maneuver;
                battleFieldAgeCounter = 0;
                pushAction(tempAction);
                // Jump immediately to the action processor since we don't want to decide to train or something if there are enemies around.
                goto creatureActionDoWhileLoop;
            }
        }
    }

    // Check to see if we have found a "home" tile where we can sleep yet.
    if (!isWorker() && randomDouble(0.0, 1.0) < 0.03 && homeTile == NULL
            && peekAction().type != CreatureAction::findHome)
    {
        // Check to see if there are any quarters owned by our color that we can reach.
        std::vector<Room*> tempRooms = gameMap.getRoomsByTypeAndColor(
                Room::quarters, color);
        tempRooms = gameMap.getReachableRooms(tempRooms, positionTile(),
                tilePassability);
        if (tempRooms.size() > 0)
        {
            tempAction.type = CreatureAction::findHome;
            pushAction(tempAction);
            goto creatureActionDoWhileLoop;
        }
    }

    // If we have found a home tile to sleep on, see if we are tired enough to want to go to sleep.
    if (!isWorker() && homeTile != NULL && 100.0 * powl(randomDouble(0.0, 0.8),
            2) > awakeness && peekAction().type != CreatureAction::sleep)
    {
        tempAction.type = CreatureAction::sleep;
        pushAction(tempAction);
        goto creatureActionDoWhileLoop;
    }

    // Check to see if there is a Dojo we can train at.
    if (!isWorker() && randomDouble(0.0, 1.0) < 0.1 && randomDouble(0.5, 1.0)
            < awakeness / 100.0 && peekAction().type != CreatureAction::train)
    {
        //TODO: Check here to see if the controlling seat has any dojo's to train at, if not then don't try to train.
        tempAction.type = CreatureAction::train;
        pushAction(tempAction);
        trainWait = 0;
        goto creatureActionDoWhileLoop;
    }

    if (battleFieldAgeCounter > 0)
        --battleFieldAgeCounter;

    // The loopback variable allows creatures to begin processing a new
    // action immediately after some other action happens.
    creatureActionDoWhileLoop: do
    {
        loopBack = false;

        // Carry out the current task
        double diceRoll;
        double tempDouble;
        Tile *neighborTile;
        std::vector<Tile*> neighbors, neighbors2, creatureNeighbors,
                claimableTiles;
        bool wasANeighbor = false;
        Player *tempPlayer;
        Tile *tempTile, *tempTile2, *myTile;
        Room *tempRoom;
        std::list<Tile*> tempPath, tempPath2;
        pair<LocationType, double> minimumFieldValue;
        std::vector<Room*> treasuriesOwned;
        std::vector<Room*> tempRooms;

        sem_wait(&actionQueueLockSemaphore);
        if (actionQueue.size() > 0)
        {
            CreatureAction topActionItem = actionQueue.front();
            sem_post(&actionQueueLockSemaphore);

            diceRoll = randomDouble(0.0, 1.0);
            switch (topActionItem.type)
            {
                case CreatureAction::idle:
                    //cout << "idle ";
                    setAnimationState("Idle");
                    //FIXME: make this into a while loop over a vector of <action, probability> pairs

                    // Workers only.
                    if (isWorker())
                    {
                        // Decide to check for diggable tiles
                        if (diceRoll < 0.5)
                        {
                            loopBack = true;
                            pushAction(CreatureAction::digTile);
                        }

                        // Decide to check for clamiable tiles
                        else if (diceRoll < 0.9)
                        {
                            loopBack = true;
                            pushAction(CreatureAction::claimTile);
                        }

                        // Decide to deposit the gold we are carrying into a treasury.
                        else if (diceRoll < 0.7 + 0.6 * (gold
                                / (double) maxGoldCarriedByWorkers))
                        {
                            loopBack = true;
                            pushAction(CreatureAction::depositGold);
                        }
                    }
                    // Non-workers only
                    else
                    {
                    }

                    // Any creature.

                    // Decide to "wander" a short distance
                    if (diceRoll < 0.6)
                    {
                        loopBack = true;
                        pushAction(CreatureAction::walkToTile);

                        // Workers should move around randomly at large jumps.  Non-workers either wander short distances or follow workers.
                        int tempX, tempY;
                        bool workerFound = false;
                        if (!isWorker())
                        {
                            // Non-workers only.

                            // Check to see if we want to try to follow a worker around or if we want to try to explore.
                            double r = randomDouble(0.0, 1.0);
                            //if(creatureJob == weakFighter) r -= 0.2;
                            if (r < 0.7)
                            {
                                // Try to find a worker to follow around.
                                for (unsigned int i = 0; !workerFound && i
                                        < reachableAlliedObjects.size(); ++i)
                                {
                                    // Check to see if we found a worker.
                                    if (reachableAlliedObjects[i]->getAttackableObjectType()
                                            == AttackableObject::creature
                                            && ((Creature*) reachableAlliedObjects[i])->isWorker())
                                    {
                                        // We found a worker so find a tile near the worker to walk to.  See if the worker is digging.
                                        tempTile
                                                = reachableAlliedObjects[i]->getCoveredTiles()[0];
                                        if (((Creature*) reachableAlliedObjects[i])->peekAction().type
                                                == CreatureAction::digTile)
                                        {
                                            // Worker is digging, get near it since it could expose enemies.
                                            tempX = tempTile->x + 3.0
                                                    * gaussianRandomDouble();
                                            tempY = tempTile->y + 3.0
                                                    * gaussianRandomDouble();
                                        }
                                        else
                                        {
                                            // Worker is not digging, wander a bit farther around the worker.
                                            tempX = tempTile->x + 8.0
                                                    * gaussianRandomDouble();
                                            tempY = tempTile->y + 8.0
                                                    * gaussianRandomDouble();
                                        }
                                        workerFound = true;
                                    }

                                    // If there are no workers around, choose tiles far away to "roam" the dungeon.
                                    if (!workerFound)
                                    {
                                        if (visibleTiles.size() > 0)
                                        {
                                            tempTile
                                                    = visibleTiles[randomDouble(
                                                            0.6, 0.8)
                                                            * (visibleTiles.size()
                                                                    - 1)];
                                            tempX = tempTile->x;
                                            tempY = tempTile->y;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // Randomly choose a tile near where we are standing to walk to.
                                if (visibleTiles.size() > 0)
                                {
                                    unsigned int tileIndex =
                                            visibleTiles.size() * randomDouble(
                                                    0.1, 0.3);
                                    myTile = positionTile();
                                    tempPath = gameMap.path(myTile,
                                            visibleTiles[tileIndex],
                                            tilePassability);
                                    if (setWalkPath(tempPath, 2, false))
                                    {
                                        setAnimationState("Walk");
                                        pushAction(CreatureAction::walkToTile);
                                        //loopBack = true;
                                        break;
                                    }
                                }

                            }
                        }
                        else
                        {
                            // Workers only.

                            // Choose a tile far away from our current position to wander to.
                            tempTile = visibleTiles[randomUint(
                                    visibleTiles.size() / 2,
                                    visibleTiles.size() - 1)];
                            tempX = tempTile->x;
                            tempY = tempTile->y;
                        }

                        Tile *tempPositionTile = positionTile();
                        std::list<Tile*> result;
                        if (tempPositionTile != NULL)
                        {
                            result = gameMap.path(tempPositionTile->x,
                                    tempPositionTile->y, tempX, tempY,
                                    tilePassability);
                        }

                        gameMap.cutCorners(result, tilePassability);
                        if (setWalkPath(result, 2, false))
                        {
                            //loopBack = true;
                            setAnimationState("Walk");
                            pushAction(CreatureAction::walkToTile);
                            break;
                        }
                    }
                    break;

                case CreatureAction::walkToTile:
                    if (randomDouble(0.0, 1.0) < 0.6
                            && enemyObjectsInRange.size() > 0)
                    {
                        popAction();
                        tempAction.type = CreatureAction::attackObject;
                        pushAction(tempAction);
                        clearDestinations();
                        loopBack = true;
                        break;
                    }

                    //TODO: Peek at the item that caused us to walk
                    // If we are walking toward a tile we are trying to dig out, check to see if it is still marked for digging.
                    sem_wait(&actionQueueLockSemaphore);
                    tempBool = (actionQueue[1].type == CreatureAction::digTile);
                    sem_post(&actionQueueLockSemaphore);
                    if (tempBool)
                    {
                        tempPlayer = getControllingPlayer();

                        // Check to see if the tile is still marked for digging
                        sem_wait(&walkQueueLockSemaphore);
                        unsigned int index = walkQueue.size();
                        Tile *currentTile = NULL;
                        if (index > 0)
                            currentTile = gameMap.getTile((int) walkQueue[index
                                    - 1].x, (int) walkQueue[index - 1].y);

                        sem_post(&walkQueueLockSemaphore);

                        if (currentTile != NULL)
                        {
                            // If it is not marked
                            if (tempPlayer != NULL
                                    && !currentTile->getMarkedForDigging(
                                            tempPlayer))
                            {
                                // Clear the walk queue
                                clearDestinations();
                            }
                        }
                    }

                    //cout << "walkToTile ";
                    sem_wait(&walkQueueLockSemaphore);
                    if (walkQueue.size() == 0)
                    {
                        popAction();
                        loopBack = true;

                        // This extra post is included here because if the break statement happens
                        // the one at the end of the 'if' block will not happen.
                        sem_post(&walkQueueLockSemaphore);
                        break;
                    }
                    sem_post(&walkQueueLockSemaphore); // If this is removed remove the one in the 'if' block as well.
                    break;

                case CreatureAction::claimTile:
                    myTile = positionTile();
                    //NOTE:  This is a workaround for the problem with the positionTile() function,
                    // it can be removed when that issue is resolved.
                    if (myTile == NULL)
                    {
                        popAction();
                        goto claimTileBreakStatement;
                    }

                    // Randomly decide to stop claiming with a small probability
                    if (randomDouble(0.0, 1.0) < 0.1 + 0.2 * markedTiles.size())
                    {
                        // If there are any visible tiles marked for digging start working on that.
                        if (markedTiles.size() > 0)
                        {
                            loopBack = true;
                            popAction();
                            pushAction(CreatureAction::digTile);
                            break;
                        }
                    }

                    // See if the tile we are standing on can be claimed
                    if ((myTile->getColor() != color || myTile->colorDouble
                            < 1.0) && myTile->isClaimable())
                    {
                        //cout << "\nTrying to claim the tile I am standing on.";
                        // Check to see if one of the tile's neighbors is claimed for our color
                        neighbors = gameMap.neighborTiles(myTile);
                        for (unsigned int j = 0; j < neighbors.size(); ++j)
                        {
                            // Check to see if the current neighbor is already claimed
                            tempTile = neighbors[j];
                            if (tempTile->getColor() == color
                                    && tempTile->colorDouble >= 1.0)
                            {
                                //cout << "\t\tFound a neighbor that is claimed.";
                                // If we found a neighbor that is claimed for our side than we can start
                                // dancing on this tile.  If there is "left over" claiming that can be done
                                // it will spill over into neighboring tiles until it is gone.
                                setAnimationState("Claim");
                                myTile->claimForColor(color, danceRate);
                                recieveExp(1.5 * (danceRate / (0.35 + 0.05
                                        * level)));

                                // Since we danced on a tile we are done for this turn
                                goto claimTileBreakStatement;
                            }
                        }
                    }

                    // The tile we are standing on is already claimed or is not currently
                    // claimable, find candidates for claiming.
                    // Start by checking the neighbor tiles of the one we are already in
                    neighbors = gameMap.neighborTiles(myTile);
                    while (neighbors.size() > 0)
                    {
                        // If the current neighbor is claimable, walk into it and skip to the end of this turn
                        tempInt = randomUint(0, neighbors.size() - 1);
                        tempTile = neighbors[tempInt];
                        //NOTE:  I don't think the "colorDouble" check should happen here.
                        if (tempTile != NULL && tempTile->getTilePassability()
                                == Tile::walkableTile && (tempTile->getColor()
                                != color || tempTile->colorDouble < 1.0)
                                && tempTile->isClaimable())
                        {
                            // The neighbor tile is a potential candidate for claiming, to be an actual candidate
                            // though it must have a neighbor of its own that is already claimed for our side.
                            neighbors2 = gameMap.neighborTiles(tempTile);
                            for (unsigned int i = 0; i < neighbors2.size(); ++i)
                            {
                                tempTile2 = neighbors2[i];
                                if (tempTile2->getColor() == color
                                        && tempTile2->colorDouble >= 1.0)
                                {
                                    clearDestinations();
                                    addDestination(tempTile->x, tempTile->y);
                                    setAnimationState("Walk");
                                    goto claimTileBreakStatement;
                                }
                            }
                        }

                        neighbors.erase(neighbors.begin() + tempInt);
                    }

                    //cout << "\nLooking at the visible tiles to see if I can claim a tile.";
                    // If we still haven't found a tile to claim, check the rest of the visible tiles
                    for (unsigned int i = 0; i < visibleTiles.size(); ++i)
                    {
                        // if this tile is not fully claimed yet or the tile is of another player's color
                        tempTile = visibleTiles[i];
                        if (tempTile != NULL && tempTile->getTilePassability()
                                == Tile::walkableTile && (tempTile->colorDouble
                                < 1.0 || tempTile->getColor() != color)
                                && tempTile->isClaimable())
                        {
                            // Check to see if one of the tile's neighbors is claimed for our color
                            neighbors = gameMap.neighborTiles(visibleTiles[i]);
                            for (unsigned int j = 0; j < neighbors.size(); ++j)
                            {
                                tempTile = neighbors[j];
                                if (tempTile->getColor() == color
                                        && tempTile->colorDouble >= 1.0)
                                {
                                    claimableTiles.push_back(tempTile);
                                }
                            }
                        }
                    }

                    //cout << "  I see " << claimableTiles.size() << " tiles I can claim.";
                    // Randomly pick a claimable tile, plot a path to it and walk to it
                    while (claimableTiles.size() > 0)
                    {
                        // Randomly find a "good" tile to claim.  A good tile is one that has many neighbors
                        // already claimed, this makes the claimed are more "round" and less jagged.
                        tempUnsigned = 0;
                        do
                        {
                            int numNeighborsClaimed;

                            // Start by randomly picking a candidate tile.
                            tempTile = claimableTiles[randomUint(0,
                                    claimableTiles.size() - 1)];

                            // Count how many of the candidate tile's neighbors are already claimed.
                            neighbors = gameMap.neighborTiles(tempTile);
                            numNeighborsClaimed = 0;
                            for (unsigned int i = 0; i < neighbors.size(); ++i)
                            {
                                if (neighbors[i]->getColor() == color
                                        && neighbors[i]->colorDouble >= 1.0)
                                    ++numNeighborsClaimed;
                            }

                            // Pick a random number in [0:1], if this number is high enough, than use this tile to claim.  The
                            // bar for success approaches 0 as numTiles approaches N so this will be guaranteed to succeed at,
                            // or before the time we get to the last unclaimed tile.  The bar for success is also lowered
                            // according to how many neighbors are already claimed.
                            //NOTE: The bar can be negative, when this happens we are guarenteed to use this candidate tile.
                            double bar;
                            bar = 1.0 - (numNeighborsClaimed / 4.0)
                                    - (tempUnsigned
                                            / (double) (claimableTiles.size()
                                                    - 1));
                            if (randomDouble(0.0, 1.0) >= bar)
                                break;

                            // Safety catch to prevent infinite loop in case the bar for success is too high and is never met.
                            if (tempUnsigned >= claimableTiles.size() - 1)
                                break;

                            // Increment the counter indicating how many candidate tiles we have rejected so far.
                            ++tempUnsigned;
                        } while (true);

                        if (tempTile != NULL)
                        {
                            // If we find a valid path to the tile start walking to it and break
                            tempPath = gameMap.path(myTile, tempTile,
                                    tilePassability);
                            gameMap.cutCorners(tempPath, tilePassability);
                            if (setWalkPath(tempPath, 2, false))
                            {
                                //loopBack = true;
                                setAnimationState("Walk");
                                pushAction(CreatureAction::walkToTile);
                                goto claimTileBreakStatement;
                            }
                        }

                        // If we got to this point, the tile we randomly picked cannot be gotten to via a
                        // valid path.  Delete it from the claimable tiles vector and repeat the outer
                        // loop to try to find another valid tile.
                        for (unsigned int i = 0; i < claimableTiles.size(); ++i)
                        {
                            if (claimableTiles[i] == tempTile)
                            {
                                claimableTiles.erase(claimableTiles.begin() + i);
                                break; // Break out of this for loop.
                            }
                        }
                    }

                    // We couldn't find a tile to try to claim so we stop trying
                    popAction();
                    claimTileBreakStatement: break;

                case CreatureAction::digTile:
                    myTile = positionTile();
                    if (myTile == NULL)
                        break;

                    //cout << "dig ";

                    // Randomly decide to stop digging with a small probability
                    if (randomDouble(0.0, 1.0) < 0.35 - 0.2
                            * markedTiles.size())
                    {
                        loopBack = true;
                        popAction();
                        goto claimTileBreakStatement;
                    }

                    // See if any of the tiles is one of our neighbors
                    wasANeighbor = false;
                    creatureNeighbors = gameMap.neighborTiles(myTile);
                    tempPlayer = getControllingPlayer();
                    for (unsigned int i = 0; i < creatureNeighbors.size()
                            && !wasANeighbor; ++i)
                    {
                        tempTile = creatureNeighbors[i];

                        if (tempPlayer != NULL
                                && tempTile->getMarkedForDigging(tempPlayer))
                        {
                            // We found a tile marked by our controlling seat, dig out the tile.

                            // If the tile is a gold tile accumulate gold for this creature.
                            if (tempTile->getType() == Tile::gold)
                            {
                                tempDouble = 5 * min(digRate,
                                        tempTile->getFullness());
                                gold += tempDouble;
                                gameMap.getSeatByColor(color)->goldMined
                                        += tempDouble;
                                recieveExp(5.0 * digRate / 20.0);
                            }

                            // Turn so that we are facing toward the tile we are going to dig out.
                            faceToward(tempTile->x, tempTile->y);

                            // Dig out the tile by decreasing the tile's fullness.
                            setAnimationState("Dig");
                            tempTile->digOut(digRate, true);
                            recieveExp(1.5 * digRate / 20.0);

                            // If the tile has been dug out, move into that tile and try to continue digging.
                            if (tempTile->getFullness() < 1)
                            {
                                recieveExp(2.5);
                                setAnimationState("Walk");

                                // Remove the dig action and replace it with
                                // walking to the newly dug out tile.
                                //popAction();
                                addDestination(tempTile->x, tempTile->y);
                                pushAction(CreatureAction::walkToTile);
                            }

                            wasANeighbor = true;

                            //Set sound position and play dig sound.
                            sound->setPosition(getPosition());
                            sound->play(CreatureSound::DIG);
                            break;
                        }
                    }

                    // Check to see if we are carrying the maximum amount of gold we can carry, and if so, try to take it to a treasury.
                    if (gold >= maxGoldCarriedByWorkers)
                    {
                        // Remove the dig action and replace it with a depositGold action.
                        //popAction();
                        pushAction(CreatureAction::depositGold);
                    }

                    // If we successfully dug a tile then we are done for this turn.
                    if (wasANeighbor)
                        break;

                    /*
                     // Randomly decide to stop digging with a larger probability
                     if(randomDouble(0.0, 1.0) < 0.1)
                     {
                     loopBack = true;
                     popAction();
                     goto claimTileBreakStatement;
                     }
                     */

                    // Find paths to all of the neighbor tiles for all of the marked visible tiles.
                    possiblePaths.clear();
                    for (unsigned int i = 0; i < markedTiles.size(); ++i)
                    {
                        neighbors = gameMap.neighborTiles(markedTiles[i]);
                        for (unsigned int j = 0; j < neighbors.size(); ++j)
                        {
                            neighborTile = neighbors[j];
                            if (neighborTile != NULL
                                    && neighborTile->getFullness() < 1)
                                possiblePaths.push_back(gameMap.path(
                                        positionTile(), neighborTile,
                                        tilePassability));
                        }
                    }

                    // Find the shortest path and start walking toward the tile to be dug out
                    if (possiblePaths.size() > 0)
                    {
                        // Find the N shortest valid paths, see if there are any valid paths shorter than this first guess
                        shortPaths.clear();
                        for (unsigned int i = 0; i < possiblePaths.size(); ++i)
                        {
                            // If the current path is long enough to be valid
                            unsigned int currentLength =
                                    possiblePaths[i].size();
                            if (currentLength >= 2)
                            {
                                shortPaths.push_back(possiblePaths[i]);

                                // If we already have enough short paths
                                if (shortPaths.size() > 5)
                                {
                                    unsigned int longestLength, longestIndex;

                                    // Kick out the longest
                                    longestLength = shortPaths[0].size();
                                    longestIndex = 0;
                                    for (unsigned int j = 1; j
                                            < shortPaths.size(); ++j)
                                    {
                                        if (shortPaths[j].size()
                                                > longestLength)
                                        {
                                            longestLength = shortPaths.size();
                                            longestIndex = j;
                                        }
                                    }

                                    shortPaths.erase(shortPaths.begin()
                                            + longestIndex);
                                }
                            }
                        }

                        // Randomly pick a short path to take
                        unsigned int numShortPaths = shortPaths.size();
                        if (numShortPaths > 0)
                        {
                            unsigned int shortestIndex;
                            shortestIndex = randomUint(0, numShortPaths - 1);
                            walkPath = shortPaths[shortestIndex];

                            // If the path is a legitimate path, walk down it to the tile to be dug out
                            gameMap.cutCorners(walkPath, tilePassability);
                            if (setWalkPath(walkPath, 2, false))
                            {
                                //loopBack = true;
                                setAnimationState("Walk");
                                pushAction(CreatureAction::walkToTile);
                                break;
                            }
                        }
                    }

                    // If none of our neighbors are marked for digging we got here too late.
                    // Finish digging
                    sem_wait(&actionQueueLockSemaphore);
                    tempBool = (actionQueue.front().type
                            == CreatureAction::digTile);
                    sem_post(&actionQueueLockSemaphore);
                    if (tempBool)
                    {
                        popAction();
                        loopBack = true;
                    }
                    break;

                case CreatureAction::depositGold:
                    // Check to see if we are standing in a treasury.
                    myTile = positionTile();
                    if (myTile != NULL)
                    {
                        tempRoom = myTile->getCoveringRoom();
                        if (tempRoom != NULL && tempRoom->getType()
                                == Room::treasury)
                        {
                            // Deposit as much of the gold we are carrying as we can into this treasury.
                            gold -= ((RoomTreasury*) tempRoom)->depositGold(
                                    gold, myTile);

                            // Depending on how much gold we have left (what did not fit in this treasury) we may want to continue
                            // looking for another treasury to put the gold into.  Roll a dice to see if we want to quit looking not.
                            if (randomDouble(1.0, maxGoldCarriedByWorkers)
                                    > gold)
                            {
                                popAction();
                                break;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }

                    // We were not standing in a treasury that has enough room for the gold we are carrying, so try to find one to walk to.
                    // Check to see if our seat controls any treasuries.
                    treasuriesOwned = gameMap.getRoomsByTypeAndColor(
                            Room::treasury, color);
                    if (treasuriesOwned.size() > 0)
                    {
                        Tile *nearestTreasuryTile;
                        nearestTreasuryTile = NULL;
                        unsigned int nearestTreasuryDistance;
                        bool validPathFound;
                        validPathFound = false;
                        tempPath.clear();
                        tempPath2.clear();
                        // Loop over the treasuries to find the closest one.
                        for (unsigned int i = 0; i < treasuriesOwned.size(); ++i)
                        {
                            if (!validPathFound)
                            {
                                // We have not yet found a valid path to a treasury, check to see if we can get to this treasury.
                                tempUnsigned = randomUint(0,
                                        treasuriesOwned[i]->numCoveredTiles()
                                                - 1);
                                nearestTreasuryTile
                                        = treasuriesOwned[i]->getCoveredTile(
                                                tempUnsigned);
                                tempPath = gameMap.path(myTile,
                                        nearestTreasuryTile, tilePassability);
                                if (tempPath.size() >= 2
                                        && ((RoomTreasury*) treasuriesOwned[i])->emptyStorageSpace()
                                                > 0)
                                {
                                    validPathFound = true;
                                    nearestTreasuryDistance = tempPath.size();
                                }
                            }
                            else
                            {
                                // We have already found at least one valid path to a treasury, see if this one is closer.
                                tempUnsigned = randomUint(0,
                                        treasuriesOwned[i]->numCoveredTiles()
                                                - 1);
                                tempTile = treasuriesOwned[i]->getCoveredTile(
                                        tempUnsigned);
                                tempPath2 = gameMap.path(myTile, tempTile,
                                        tilePassability);
                                if (tempPath2.size() >= 2 && tempPath2.size()
                                        < nearestTreasuryDistance
                                        && ((RoomTreasury*) treasuriesOwned[i])->emptyStorageSpace()
                                                > 0)
                                {
                                    tempPath = tempPath2;
                                    nearestTreasuryDistance = tempPath.size();
                                }
                            }
                        }

                        if (validPathFound)
                        {
                            // Begin walking to this treasury.
                            gameMap.cutCorners(tempPath, tilePassability);
                            if (setWalkPath(tempPath, 2, false))
                            {
                                setAnimationState("Walk");
                                pushAction(CreatureAction::walkToTile);
                                //loopBack = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        // There are no treasuries available so just go back to what we were doing.
                        popAction();
                        loopBack = true;
                        break;
                    }

                    // If we get to here, there is either no treasuries controlled by us, or they are all
                    // unreachable, or they are all full, so quit trying to deposit gold.
                    popAction();
                    loopBack = true;
                    break;

                case CreatureAction::findHome:
                    // Check to see if we are standing in an open quarters tile that we can claim as our home.
                    myTile = positionTile();
                    if (myTile != NULL)
                    {
                        tempRoom = myTile->getCoveringRoom();
                        if (tempRoom != NULL && tempRoom->getType()
                                == Room::quarters)
                        {
                            if (((RoomQuarters*) tempRoom)->claimTileForSleeping(
                                    myTile, this))
                                homeTile = myTile;
                        }
                    }
                    else
                    {
                        break;
                    }

                    // If we found a tile to claim as our home in the above block.
                    if (homeTile != NULL)
                    {
                        popAction();
                        loopBack = true;
                        break;
                    }

                    // Check to see if we can walk to a quarters that does have an open tile.
                    tempRooms = gameMap.getRoomsByTypeAndColor(Room::quarters,
                            color);
                    std::random_shuffle(tempRooms.begin(), tempRooms.end());
                    unsigned int nearestQuartersDistance;
                    bool validPathFound;
                    validPathFound = false;
                    tempPath.clear();
                    tempPath2.clear();
                    for (unsigned int i = 0; i < tempRooms.size(); ++i)
                    {
                        // Get the list of open rooms at the current quarters and check to see if
                        // there is a place where we could put a bed big enough to sleep in.
                        tempTile
                                = ((RoomQuarters*) tempRooms[i])->getLocationForBed(
                                        bedDim1, bedDim2);

                        // If the previous attempt to place the bed in this quarters failed, try again with the bed the other way.
                        if (tempTile == NULL)
                            tempTile
                                    = ((RoomQuarters*) tempRooms[i])->getLocationForBed(
                                            bedDim2, bedDim1);

                        // Check to see if either of the two possible bed orientations tried above resulted in a successful placement.
                        if (tempTile != NULL)
                        {
                            tempPath2 = gameMap.path(myTile, tempTile,
                                    tilePassability);

                            // Find out the minimum valid path length of the paths determined in the above block.
                            if (!validPathFound)
                            {
                                // If the current path is long enough to be valid then record the path and the distance.
                                if (tempPath2.size() >= 2)
                                {
                                    tempPath = tempPath2;
                                    nearestQuartersDistance = tempPath.size();
                                    validPathFound = true;
                                }
                            }
                            else
                            {
                                // If the current path is long enough to be valid but shorter than the
                                // shortest path seen so far, then record the path and the distance.
                                if (tempPath2.size() >= 2 && tempPath2.size()
                                        < nearestQuartersDistance)
                                {
                                    tempPath = tempPath2;
                                    nearestQuartersDistance = tempPath.size();
                                }
                            }
                        }
                    }

                    // If we found a valid path to an open room in a quarters, then start walking along it.
                    if (validPathFound)
                    {
                        gameMap.cutCorners(tempPath, tilePassability);
                        if (setWalkPath(tempPath, 2, false))
                        {
                            setAnimationState("Walk");
                            pushAction(CreatureAction::walkToTile);
                            //loopBack = true;
                            break;
                        }
                    }

                    // If we got here there are no reachable quarters that are unclaimed so we quit trying to find one.
                    popAction();
                    loopBack = true;
                    break;

                case CreatureAction::sleep:
                    myTile = positionTile();
                    if (homeTile == NULL)
                        break;

                    if (myTile != homeTile)
                    {
                        // Walk to the the home tile.
                        tempPath = gameMap.path(myTile, homeTile,
                                tilePassability);
                        gameMap.cutCorners(tempPath, tilePassability);
                        if (setWalkPath(tempPath, 2, false))
                        {
                            setAnimationState("Walk");
                            pushAction(CreatureAction::walkToTile);
                            //loopBack = true;
                            break;
                        }
                    }
                    else
                    {
                        // We are at the home tile so sleep.
                        setAnimationState("Sleep");
                        awakeness += 4.0;
                        if (awakeness > 100.0)
                        {
                            awakeness = 100.0;
                            popAction();
                        }
                    }
                    break;

                case CreatureAction::train:
                    // Creatures can only train to level 10 at a dojo.
                    //TODO: Check to see if the dojo has been upgraded to allow training to a higher level.
                    stopUsingDojo = false;
                    if (level > 10)
                    {
                        popAction();
                        loopBack = true;
                        trainWait = 0;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    // Randomly decide to stop training, we are more likely to stop when we are tired.
                    if (100.0 * powl(randomDouble(0.0, 1.0), 2) > awakeness)
                    {
                        popAction();
                        trainWait = 0;
                        loopBack = true;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    // Decrement a counter each turn until it reaches 0, if it reaches 0 we try to train this turn.
                    if (trainWait > 0)
                    {
                        setAnimationState("Idle");
                        --trainWait;
                        goto trainBreakStatement;
                    }

                    // Make sure we are on the map.
                    myTile = positionTile();
                    if (myTile != NULL)
                    {
                        // See if we are in a dojo now.
                        tempRoom = myTile->getCoveringRoom();
                        if (tempRoom != NULL && tempRoom->getType()
                                == Room::dojo
                                && tempRoom->numOpenCreatureSlots() > 0)
                        {
                            // Train at this dojo.
                            trainingDojo = (RoomDojo*) tempRoom;
                            trainingDojo->addCreatureUsingRoom(this);
                            tempTile = tempRoom->getCentralTile();
                            faceToward(tempTile->x, tempTile->y);
                            setAnimationState("Attack1");
                            recieveExp(5.0);
                            awakeness -= 5.0;
                            trainWait = randomUint(3, 8);
                            goto trainBreakStatement;
                        }
                    }
                    else
                    {
                        // We are not on the map, don't do anything.
                        popAction();
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    // Get the list of dojos controlled by our seat and make sure there is at least one.
                    tempRooms = gameMap.getRoomsByTypeAndColor(Room::dojo,
                            color);
                    if (tempRooms.size() == 0)
                    {
                        popAction();
                        loopBack = true;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    // Pick a dojo to train at and try to walk to it.
                    //TODO: Pick a close dojo, not necessarily the closest just a somewhat closer than average one.
                    tempInt = 0;
                    double maxTrainDistance;
                    maxTrainDistance = 40.0;
                    do
                    {
                        tempInt = randomUint(0, tempRooms.size() - 1);
                        tempRoom = tempRooms[tempInt];
                        tempRooms.erase(tempRooms.begin() + tempInt);
                        tempDouble = 1.0 / (maxTrainDistance
                                - gameMap.crowDistance(myTile,
                                        tempRoom->getCoveredTile(0)));
                        if (randomDouble(0.0, 1.0) < tempDouble)
                            break;
                        ++tempInt;
                    } while (tempInt < 5 && tempRoom->numOpenCreatureSlots()
                            == 0 && tempRooms.size() > 0);

                    if (tempRoom->numOpenCreatureSlots() == 0)
                    {
                        // The room is already being used, stop trying to train.
                        popAction();
                        loopBack = true;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    tempTile = tempRoom->getCoveredTile(randomUint(0,
                            tempRoom->numCoveredTiles() - 1));
                    tempPath = gameMap.path(myTile, tempTile, tilePassability);
                    if (tempPath.size() < maxTrainDistance && setWalkPath(
                            tempPath, 2, false))
                    {
                        setAnimationState("Walk");
                        //loopBack = true;
                        pushAction(CreatureAction::walkToTile);
                        goto trainBreakStatement;
                    }
                    else
                    {
                        // We could not find a dojo to train at so stop trying to find one.
                        popAction();
                        loopBack = true;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    trainBreakStatement: if (stopUsingDojo && trainingDojo
                            != NULL)
                    {

                        trainingDojo->removeCreatureUsingRoom(this);
                        trainingDojo = NULL;
                    }
                    break;

                case CreatureAction::attackObject:
                    // If there are no more enemies which are reachable, stop attacking
                    if (reachableEnemyObjects.size() == 0)
                    {
                        popAction();
                        loopBack = true;
                        break;
                    }

                    myTile = positionTile();

                    // Find the first enemy close enough to hit and attack it
                    if (livingEnemyObjectsInRange.size() > 0)
                    {
                        tempAttackableObject = livingEnemyObjectsInRange[0];

                        // Turn to face the creature we are attacking and set the animation state to Attack.
                        //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
                        tempTile = tempAttackableObject->getCoveredTiles()[0];
                        clearDestinations();
                        faceToward(tempTile->x, tempTile->y);
                        setAnimationState("Attack1");

                        //Play attack sound
                        //TODO - syncronise with animation
                        sound->setPosition(getPosition());
                        sound->play(CreatureSound::ATTACK);

                        // Calculate how much damage we do.
                        double damageDone = getHitroll(gameMap.crowDistance(
                                myTile, tempTile));
                        damageDone *= randomDouble(0.0, 1.0);
                        damageDone -= powl(randomDouble(0.0, 0.4), 2.0)
                                * tempAttackableObject->getDefense();

                        // Make sure the damage is positive.
                        if (damageDone < 0.0)
                            damageDone = 0.0;

                        // Do the damage and award experience points to both creatures.
                        tempAttackableObject->takeDamage(damageDone, tempTile);
                        double expGained;
                        expGained = 1.0 + 0.2 * powl(damageDone, 1.3);
                        awakeness -= 0.5;

                        // Give a small amount of experince to the creature we hit.
                        tempAttackableObject->recieveExp(0.15 * expGained);

                        // Add a bonus modifier based on the level of the creature we hit
                        // to expGained and give ourselves that much experience.
                        if (tempAttackableObject->getLevel() >= level)
                            expGained
                                    *= 1.0 + (tempAttackableObject->getLevel()
                                            - level) / 10.0;
                        else
                            expGained /= 1.0 + (level
                                    - tempAttackableObject->getLevel()) / 10.0;

                        recieveExp(expGained);

                        cout << "\n" << name << " did " << damageDone
                                << " damage to "
                                << tempAttackableObject->getName();
                        cout << " who now has " << tempAttackableObject->getHP(
                                tempTile) << "hp";

                        // Randomly decide to start maneuvering again so we don't just stand still and fight.
                        if (randomDouble(0.0, 1.0) <= 0.6)
                            popAction();

                        break;
                    }

                    // There is not an enemy within range, begin maneuvering to try to get near an enemy, or out of the combat situation.
                    popAction();
                    pushAction(CreatureAction::maneuver);
                    loopBack = true;
                    break;

                case CreatureAction::maneuver:
                    myTile = positionTile();

                    // If there is an enemy within range, stop maneuvering and attack it.
                    if (livingEnemyObjectsInRange.size() > 0)
                    {
                        popAction();
                        loopBack = true;

                        // If the next action down the stack is not an attackObject action, add it.
                        sem_wait(&actionQueueLockSemaphore);
                        tempBool = (actionQueue.front().type
                                != CreatureAction::attackObject);
                        sem_post(&actionQueueLockSemaphore);
                        if (tempBool)
                            pushAction(CreatureAction::attackObject);

                        break;
                    }

                    // If there are no more enemies which are reachable, stop maneuvering.
                    if (reachableEnemyObjects.size() == 0)
                    {
                        popAction();
                        loopBack = true;
                        break;
                    }

                    /*
                     // Check to see if we should try to strafe the enemy
                     if(randomDouble(0.0, 1.0) < 0.3)
                     {
                     //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
                     tempTile = nearestEnemyObject->getCoveredTiles()[0];
                     tempVector = Ogre::Vector3(tempTile->x, tempTile->y, 0.0);
                     sem_wait(&positionLockSemaphore);
                     tempVector -= position;
                     sem_post(&positionLockSemaphore);
                     tempVector.normalise();
                     tempVector *= randomDouble(0.0, 3.0);
                     tempQuat.FromAngleAxis(Ogre::Degree((randomDouble(0.0, 1.0) < 0.5 ? 90 : 270)), Ogre::Vector3::UNIT_Z);
                     tempTile = gameMap.getTile(positionTile()->x + tempVector.x, positionTile()->y + tempVector.y);
                     if(tempTile != NULL)
                     {
                     tempPath = gameMap.path(positionTile(), tempTile, tilePassability);

                     if(setWalkPath(tempPath, 2, false))
                     setAnimationState("Walk");
                     }
                     }
                     */

                    // There are no enemy creatures in range so we will have to maneuver towards one.
                    // Prepare the battlefield so we can decide where to move.
                    if (battleFieldAgeCounter == 0)
                    {
                        computeBattlefield();
                        battleFieldAgeCounter = randomUint(2, 6);
                    }

                    // Find a location on the battlefield to move to, we try to find a minumum if we are
                    // trying to "attack" and a maximum if we are trying to "retreat".
                    if (battleField->get(myTile->x, myTile->y).first > 0.0)
                    {
                        minimumFieldValue = battleField->min(); // Attack
                        tempBool = true;
                        //TODO: Set this to some sort of Attack-move animation.
                    }
                    else
                    {
                        minimumFieldValue = battleField->max(); // Retreat
                        tempBool = false;
                    }

                    // Pick a destination tile near the tile we got from the battlefield.
                    clearDestinations();
                    tempDouble = max(weaponL->range, weaponR->range); // Pick a true destination randomly within the max range of our weapons.
                    tempDouble = sqrt(tempDouble);
                    //FIXME:  This should find a path to a tile we can walk to, it does not always do this the way it is right now.
                    tempPath = gameMap.path(positionTile()->x,
                            positionTile()->y, minimumFieldValue.first.first
                                    + randomDouble(-1.0 * tempDouble,
                                            tempDouble),
                            minimumFieldValue.first.second + randomDouble(-1.0
                                    * tempDouble, tempDouble), tilePassability);

                    // Walk a maximum of N tiles before recomputing the destination since we are in combat.
                    tempUnsigned = 5;
                    if (tempPath.size() >= tempUnsigned)
                        tempPath.resize(tempUnsigned);

                    gameMap.cutCorners(tempPath, tilePassability);
                    if (setWalkPath(tempPath, 2, false))
                    {
                        if (tempBool)
                            setAnimationState("Walk");
                        else
                            setAnimationState("Flee");
                    }

                    // Push a walkToTile action into the creature's action queue to make them walk the path they have
                    // decided on without recomputing, this helps prevent them from getting stuck in local minima.
                    pushAction(CreatureAction::walkToTile);

                    // This is a debugging statement, it produces a visual display of the battlefield as seen by the first creature.
                    if (battleField->name.compare("field_1") == 0)
                    {
                        //battleField->refreshMeshes(1.0);
                    }
                    break;

                default:
                    cerr
                            << "\n\nERROR:  Unhandled action type in Creature::doTurn().\n\n";
                    exit(1);
                    break;
            }
        }
        else
        {
            sem_post(&actionQueueLockSemaphore);
            cerr
                    << "\n\nERROR:  Creature has empty action queue in doTurn(), this should not happen.\n\n";
            exit(1);
        }

    } while (loopBack);

    // Update the visual debugging entities
    if (hasVisualDebuggingEntities)
    {
        // if we are standing in a different tile than we were last turn
        Tile *currentPositionTile = positionTile();
        if (currentPositionTile != previousPositionTile)
        {
            //TODO:  This destroy and re-create is kind of a hack as its likely only a few tiles will actually change.
            destroyVisualDebugEntities();
            createVisualDebugEntities();
        }
    }
}

double Creature::getHitroll(double range)
{
    double tempHitroll = 1.0;

    if (weaponL != NULL && weaponL->range >= range)
        tempHitroll += weaponL->damage;
    if (weaponR != NULL && weaponR->range >= range)
        tempHitroll += weaponR->damage;
    tempHitroll *= log((double) log((double) level + 1) + 1);

    return tempHitroll;
}

double Creature::getDefense()
{
    double returnValue = 3.0;
    if (weaponL != NULL)
        returnValue += weaponL->defense;
    if (weaponR != NULL)
        returnValue += weaponR->defense;

    return returnValue;
}

/** \brief Increases the creature's level, adds bonuses to stat points, changes the mesh, etc.
 *
 */
void Creature::doLevelUp()
{
    if (level >= 100)
        return;

    ++level;
    cout << "\n\n" << name << " has reached level " << level << "\n";

    if (isWorker())
    {
        digRate += 4.0 * level / (level + 5.0);
        danceRate += 0.12 * level / (level + 5.0);
    }
    cout << "New dig rate: " << digRate << "\tnew dance rate: " << danceRate
            << "\n";

    moveSpeed += 0.4 / (level + 2.0);
    //if(digRate > 60)  digRate = 60;

    maxHP += hpPerLevel;
    maxMana += manaPerLevel;

    // Scale up the mesh.
    if (meshesExist && ((level <= 30 && level % 2 == 0) || (level > 30 && level
            % 3 == 0)))
    {
        double scaleFactor = 1.0 + level / 250.0;
        if (scaleFactor > 1.03)
            scaleFactor = 1.04;
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::scaleSceneNode;
        request->p = sceneNode;
        request->vec = Ogre::Vector3(scaleFactor, scaleFactor, scaleFactor);

        // Add the request to the queue of rendering operations to be performed before the next frame.
        queueRenderRequest(request);
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
    //double effectiveRadius = min(5.0, sightRadius) + sightRadius*powl(randomDouble(0.0, 1.0), 3.0);
    double effectiveRadius = sightRadius;
    visibleTiles = gameMap.visibleTiles(positionTile(), effectiveRadius);
}

/*! \brief Loops over the visibleTiles and adds all enemy creatures in each tile to a list which it returns.
 *
 */
std::vector<AttackableObject*> Creature::getVisibleEnemyObjects()
{
    return getVisibleForce(color, true);
}

/*! \brief Loops over objectsToCheck and returns a vector containing all the ones which can be reached via a valid path.
 *
 */
std::vector<AttackableObject*> Creature::getReachableAttackableObjects(
        const std::vector<AttackableObject*> &objectsToCheck,
        unsigned int *minRange, AttackableObject **nearestObject)
{
    std::vector<AttackableObject*> tempVector;
    Tile *myTile = positionTile(), *objectTile;
    std::list<Tile*> tempPath;
    bool minRangeSet = false;

    // Loop over the vector of objects we are supposed to check.
    for (unsigned int i = 0; i < objectsToCheck.size(); ++i)
    {
        // Try to find a valid path from the tile this creature is in to the nearest tile where the current target object is.
        //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
        objectTile = objectsToCheck[i]->getCoveredTiles()[0];
        if (gameMap.pathExists(myTile->x, myTile->y, objectTile->x,
                objectTile->y, tilePassability))
        {
            tempVector.push_back(objectsToCheck[i]);

            if (minRange != NULL)
            {
                //TODO:  If this could be computed without the path call that would be better.
                tempPath = gameMap.path(myTile, objectTile, tilePassability);

                if (!minRangeSet)
                {
                    *nearestObject = objectsToCheck[i];
                    *minRange = tempPath.size();
                    minRangeSet = true;
                }
                else
                {
                    if (tempPath.size() < *minRange)
                    {
                        *minRange = tempPath.size();
                        *nearestObject = objectsToCheck[i];
                    }
                }
            }
        }
    }

    //TODO: Maybe think of a better canary value for this.
    if (minRange != NULL && !minRangeSet)
        *minRange = 999999;

    return tempVector;
}

/*! \brief Loops over the enemyObjectsToCheck vector and adds all enemy creatures within weapons range to a list which it returns.
 *
 */
std::vector<AttackableObject*> Creature::getEnemyObjectsInRange(
        const std::vector<AttackableObject*> &enemyObjectsToCheck)
{
    std::vector<AttackableObject*> tempVector;

    // If there are no enemies to check we are done.
    if (enemyObjectsToCheck.size() == 0)
        return tempVector;

    // Find our location and calculate the square of the max weapon range we have.
    Tile *myTile = positionTile();
    double weaponRangeSquared = max(weaponL->range, weaponR->range);
    weaponRangeSquared *= weaponRangeSquared;

    // Loop over the enemyObjectsToCheck and add any within range to the tempVector.
    for (unsigned int i = 0; i < enemyObjectsToCheck.size(); ++i)
    {
        //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
        Tile *tempTile = enemyObjectsToCheck[i]->getCoveredTiles()[0];
        if (tempTile != NULL)
        {
            double rSquared = powl(myTile->x - tempTile->x, 2.0) + powl(
                    myTile->y - tempTile->y, 2.0);

            if (rSquared < weaponRangeSquared)
                tempVector.push_back(enemyObjectsToCheck[i]);
        }
    }

    return tempVector;
}

/*! \brief Loops over the visibleTiles and adds all allied creatures in each tile to a list which it returns.
 *
 */
std::vector<AttackableObject*> Creature::getVisibleAlliedObjects()
{
    return getVisibleForce(color, false);
}

/*! \brief Loops over the visibleTiles and adds any which are marked for digging to a vector which it returns.
 *
 */
std::vector<Tile*> Creature::getVisibleMarkedTiles()
{
    std::vector<Tile*> tempVector;
    Player *tempPlayer = getControllingPlayer();

    // Loop over all the visible tiles.
    for (unsigned int i = 0; i < visibleTiles.size(); ++i)
    {
        // Check to see if the tile is marked for digging.
        if (tempPlayer != NULL && visibleTiles[i]->getMarkedForDigging(
                tempPlayer))
            tempVector.push_back(visibleTiles[i]);
    }

    return tempVector;
}

/*! \brief Loops over the visibleTiles and returns any creatures in those tiles whose color matches (or if invert is true, does not match) the given color parameter.
 *
 */
std::vector<AttackableObject*> Creature::getVisibleForce(int color, bool invert)
{
    return gameMap.getVisibleForce(visibleTiles, color, invert);
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
    for (unsigned int i = 0; i < visibleTiles.size(); ++i)
    {
        currentTile = visibleTiles[i];

        if (currentTile != NULL)
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
    std::list<Tile*>::iterator itr;
    for (itr = visualDebugEntityTiles.begin(); itr
            != visualDebugEntityTiles.end(); ++itr)
    {
        currentTile = *itr;

        if (currentTile != NULL)
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
    sem_wait(&positionLockSemaphore);
    Ogre::Vector3 tempPosition(position);
    sem_post(&positionLockSemaphore);

    return gameMap.getTile((int) (tempPosition.x), (int) (tempPosition.y));
}

/** \brief Conform: AttackableObject - Returns a vector containing the tile the creature is in.
 *
 */
std::vector<Tile*> Creature::getCoveredTiles()
{
    std::vector<Tile*> tempVector;
    tempVector.push_back(positionTile());
    return tempVector;
}

/*! \brief Completely destroy this creature, including its OGRE entities, scene nodes, etc.
 *
 */
void Creature::deleteYourself()
{
    // Make sure the weapons are deleted as well.
    weaponL->deleteYourself();
    weaponR->deleteYourself();

    // If we are standing in a valid tile, we need to notify that tile we are no longer there.
    if (positionTile() != NULL)
        positionTile()->removeCreature(this);

    if (meshesExist)
        destroyMesh();

    // Create a render request asking the render queue to actually do the deletion of this creature.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::deleteCreature;
    request->p = this;

    // Add the requests to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

/*! \brief Creates a string with a unique number embedded into it so the creature's name will not be the same as any other OGRE entity name.
 *
 */
std::string Creature::getUniqueCreatureName()
{
    static int uniqueNumber = 1;
    std::string tempString = className + Ogre::StringConverter::toString(
            uniqueNumber);
    ++uniqueNumber;
    return tempString;
}

void Creature::createStatsWindow()
{
    sem_wait(&statsWindowLockSemaphore);

    if (statsWindow != NULL)
    {
        sem_post(&statsWindowLockSemaphore);
        return;
    }

    CEGUI::WindowManager *wmgr = CEGUI::WindowManager::getSingletonPtr();
    CEGUI::Window *rootWindow = CEGUI::System::getSingleton().getGUISheet();

    statsWindow = wmgr->createWindow("OD/FrameWindow",
            std::string("Root/CreatureStatsWindows/") + getName());
    statsWindow->setPosition(UVector2(UDim(0.7, 0), UDim(0.65, 0)));
    statsWindow->setSize(UVector2(UDim(0.25, 0), UDim(0.3, 0)));

    CEGUI::Window *textWindow = wmgr->createWindow("OD/StaticText",
            statsWindow->getName() + "TextDisplay");
    textWindow->setPosition(UVector2(UDim(0.05, 0), UDim(0.15, 0)));
    textWindow->setSize(UVector2(UDim(0.9, 0), UDim(0.8, 0)));
    statsWindow->addChildWindow(textWindow);
    rootWindow->addChildWindow(statsWindow);
    statsWindow->show();
    sem_post(&statsWindowLockSemaphore);

    updateStatsWindow();
}

void Creature::destroyStatsWindow()
{
    sem_wait(&statsWindowLockSemaphore);
    if (statsWindow != NULL)
    {
        statsWindow->destroy();
        statsWindow = NULL;
    }
    sem_post(&statsWindowLockSemaphore);
}

void Creature::updateStatsWindow()
{
    sem_wait(&statsWindowLockSemaphore);

    if (statsWindow != NULL)
        statsWindow->getChild(statsWindow->getName() + "TextDisplay")->setText(
                getStatsText());

    sem_post(&statsWindowLockSemaphore);
}

std::string Creature::getStatsText()
{
    std::stringstream tempSS;
    tempSS << "Creature name: " << name << "\n";
    tempSS << "HP: " << getHP(NULL) << " / " << maxHP << "\n";
    tempSS << "Mana: " << getMana() << " / " << maxMana << "\n";
    sem_wait(&actionQueueLockSemaphore);
    tempSS << "AI State: " << actionQueue.front().toString() << "\n";
    sem_post(&actionQueueLockSemaphore);
    return tempSS.str();
}

/** \brief Conform: AttackableObject - Returns whether or not this creature is capable of moving.
 *
 */
bool Creature::isMobile()
{
    return true;
}

/** \brief Conform: AttackableObject - Returns the creature's level.
 *
 */
int Creature::getLevel()
{
    return level;
}

/** \brief Conform: AttackableObject - Returns the creature's color.
 *
 */
int Creature::getColor()
{
    return color;
}

/** \brief Sets the creature's color.
 *
 */
void Creature::setColor(int nColor)
{
    color = nColor;
}

/** \brief Conform: AttackableObject - Returns the type of AttackableObject that this is (Creature, Room, etc).
 *
 */
AttackableObject::AttackableObjectType Creature::getAttackableObjectType()
{
    return AttackableObject::creature;
}

/** \brief Conform: AttackableObject - Returns the name of this creature.
 *
 */
string Creature::getName()
{
    return name;
}

/** \brief Conform: AttackableObject - Deducts a given amount of HP from this creature.
 *
 */
void Creature::takeDamage(double damage, Tile *tileTakingDamage)
{
    sem_wait(&hpLockSemaphore);
    hp -= damage;
    sem_post(&hpLockSemaphore);
}

/** \brief Conform: AttackableObject - Adds experience to this creature.
 *
 */
void Creature::recieveExp(double experience)
{
    if (experience < 0.0)
        return;

    exp += experience;
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
//FIXME: This should be made into getControllingSeat(), when this is done it can simply be a call to GameMap::getSeatByColor().
Player* Creature::getControllingPlayer()
{
    Player *tempPlayer;

    if (gameMap.me->seat->color == color)
    {
        return gameMap.me;
    }

    // Try to find and return a player with color equal to this creature's
    for (unsigned int i = 0; i < gameMap.numPlayers(); ++i)
    {
        tempPlayer = gameMap.getPlayer(i);
        if (tempPlayer->seat->color == color)
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
    sem_wait(&actionQueueLockSemaphore);
    actionQueue.clear();
    actionQueue.push_front(CreatureAction::idle);
    sem_post(&actionQueueLockSemaphore);
}

void Creature::pushAction(CreatureAction action)
{
    sem_wait(&actionQueueLockSemaphore);
    actionQueue.push_front(action);
    sem_post(&actionQueueLockSemaphore);

    updateStatsWindow();
}

void Creature::popAction()
{
    sem_wait(&actionQueueLockSemaphore);
    actionQueue.pop_front();
    sem_post(&actionQueueLockSemaphore);

    updateStatsWindow();
}

CreatureAction Creature::peekAction()
{
    sem_wait(&actionQueueLockSemaphore);
    CreatureAction tempAction = actionQueue.front();
    sem_post(&actionQueueLockSemaphore);

    return tempAction;
}

/** \brief This function loops over the visible tiles and computes a score for each one indicating how
 * frindly or hostile that tile is and stores it in the battleField variable.
 *
 */
void Creature::computeBattlefield()
{
    Tile *myTile, *tempTile;
    int xDist, yDist;
    AttackableObject* tempObject;

    // Loop over the tiles in this creature's battleField and compute their value.
    // The creature will then walk towards the tile with the minimum value to
    // attack or towards the maximum value to retreat.
    myTile = positionTile();
    battleField->clear();
    for (unsigned int i = 0; i < visibleTiles.size(); ++i)
    {
        tempTile = visibleTiles[i];
        double tileValue = 0.0;// - sqrt(rSquared)/sightRadius;

        // Enemies
        for (unsigned int j = 0; j < reachableEnemyObjects.size(); ++j)
        {
            // Skip over objects which will not attack us (they either do not attack at all, or they are dead).
            tempObject = reachableEnemyObjects[j];
            if (!(tempObject->getAttackableObjectType()
                    == AttackableObject::creature
                    || tempObject->getAttackableObjectType()
                            == AttackableObject::trap) || tempObject->getHP(
                    NULL) <= 0.0)
            {
                continue;
            }

            //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
            Tile *tempTile2 = tempObject->getCoveredTiles()[0];

            // Compensate for how close the creature is to me
            //rSquared = powl(myTile->x - tempTile2->x, 2.0) + powl(myTile->y - tempTile2->y, 2.0);
            //double factor = 1.0 / (sqrt(rSquared) + 1.0);

            // Subtract for the distance from the enemy creature to r
            xDist = tempTile->x - tempTile2->x;
            yDist = tempTile->y - tempTile2->y;
            tileValue -= 1.0 / sqrt(
                    (double) (xDist * xDist + yDist * yDist + 1));
        }

        // Allies
        for (unsigned int j = 0; j < reachableAlliedObjects.size(); ++j)
        {
            //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
            Tile *tempTile2 = visibleAlliedObjects[j]->getCoveredTiles()[0];

            // Compensate for how close the creature is to me
            //rSquared = powl(myTile->x - tempTile2->x, 2.0) + powl(myTile->y - tempTile2->y, 2.0);
            //double factor = 1.0 / (sqrt(rSquared) + 1.0);

            xDist = tempTile->x - tempTile2->x;
            yDist = tempTile->y - tempTile2->y;
            tileValue += 1.2 / (sqrt((double) (xDist * xDist + yDist * yDist
                    + 1)));
        }

        const double jitter = 0.00;
        const double tileScaleFactor = 0.5;
        battleField->set(tempTile->x, tempTile->y, (tileValue + randomDouble(
                -1.0 * jitter, jitter)) * tileScaleFactor);
    }
}

