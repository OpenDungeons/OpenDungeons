#include <cmath>
#include <algorithm>

#include <CEGUI.h>
#include <CEGUIWindow.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>
#include <OgreVector2.h>
#include "CreatureAction.h"
#include "Field.h"
#include "Weapon.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "SoundEffectsHelper.h"
#include "CreatureSound.h"
#include "Player.h"
#include "Seat.h"
#include "RenderManager.h"
#include "Random.h"
#include "LogManager.h"
#include "Quadtree.h"
#include "Creature.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

//TODO: make this read from file
static const int MAX_LEVEL = 100;

Creature::Creature( 
                    GameMap*            gameMap,
                    const std::string&  name

                    ) :
        weaponL                 (0),
        weaponR                 (0),
        homeTile                (0),
        definition              (0),
        hasVisualDebuggingEntities  (false),
        awakeness               (100.0),
        maxHP                   (100.0),
        maxMana                 (100.0),
        level                   (1),
        exp                     (0.0),
        digRate                 (1.0),
        danceRate               (1.0),
        deathCounter            (10),
        gold                    (0),
        battleFieldAgeCounter   (0),
        trainWait               (0),
        previousPositionTile    (0),
        battleField             (new Field("autoname")),
        trainingDojo            (0),	
	index_point             (position.x,position.y),
        sound                   (SoundEffectsHelper::getSingleton().createCreatureSound(getName()))
{
    setGameMap(gameMap);
    sem_init(&hpLockSemaphore, 0, 1);
    sem_init(&manaLockSemaphore, 0, 1);
    sem_init(&isOnMapLockSemaphore, 0, 1);
    sem_init(&actionQueueLockSemaphore, 0, 1);
    sem_init(&statsWindowLockSemaphore, 0, 1);


    setName(name);
    gameMap->myCullingQuad.insert(this);


    sem_wait(&statsWindowLockSemaphore);
    statsWindow = 0;
    sem_post(&statsWindowLockSemaphore);

    setIsOnMap(false);

    setHP(10.0);
    setMana(10.0);

    setObjectType(GameEntity::creature);

    pushAction(CreatureAction::idle);
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
    return "className\tname\tposX\tposY\tposZ\tcolor\tweaponL"
        + Weapon::getFormat() + "\tweaponR" + Weapon::getFormat() + "\tHP\tmana";
}

/*! \brief A matched function to transport creatures between files and over the network.
 *
 */
std::ostream& operator<<(std::ostream& os, Creature *c)
{
    os << c->definition->getClassName() << "\t" << c->getName() << "\t";

    os << c->getPosition() << "\t";
    os << c->getColor() << "\t";
    os << c->weaponL << "\t" << c->weaponR << "\t";
    os << c->getHP() << "\t";
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
    std::string className;
    std::string tempString;

    is >> className;
    is >> tempString;

    if (tempString.compare("autoname") == 0)
        tempString = c->getUniqueCreatureName();

    c->setName(tempString);

    is >> xLocation >> yLocation >> zLocation;
    xLocation+=GameMap::mapSizeX/2;
    yLocation+=GameMap::mapSizeY/2;
    c->setPosition(Ogre::Vector3(xLocation, yLocation, zLocation));

    int color = 0;
    is >> color;
    c->setColor(color);

    c->weaponL = new Weapon;
    is >> c->weaponL;
    c->weaponL->setParentCreature(c);
    c->weaponL->setHandString("L");

    c->weaponR = new Weapon;
    is >> c->weaponR;
    c->weaponR->setParentCreature(c);
    c->weaponR->setHandString("R");

    // Copy the class based items
    CreatureDefinition *creatureClass = c->getGameMap()->getClassDescription(className);
    if (creatureClass != 0)
    {
        //*c = *creatureClass;
        c->definition = creatureClass;
    }
    assert(c->definition);

    is >> tempDouble;
    c->setHP(tempDouble);
    is >> tempDouble;
    c->setMana(tempDouble);

    return is;
}

Creature& Creature::operator=(const CreatureDefinition* c2)
{
    setCreatureDefinition(c2);
    return *this;
}

/*! \brief Changes the creature's position to a new position.
 *
 *  This is an overloaded function which just calls Creature::setPosition(double x, double y, double z).
 */
void Creature::setPosition(const Ogre::Vector3& v)
{
    // If we are on the gameMap we may need to update the tile we are in
    if (getIsOnMap())
    {
        // We are on the map
        // Move the creature relative to its parent scene node.  We record the
        // tile the creature is in before and after the move to properly
        // maintain the results returned by the positionTile() function.
        Tile *oldPositionTile = positionTile();
        MovableGameEntity::setPosition(v);
        Tile *newPositionTile = positionTile();

        if (oldPositionTile != newPositionTile)
        {
            if (oldPositionTile != 0)
                oldPositionTile->removeCreature(this);

            if (positionTile() != 0)
                positionTile()->addCreature(this);

	    
        }

	tracingCullingQuad->moveEntryDelta(this,get2dPosition());

    }
    else
    {
        // We are not on the map
        


        MovableGameEntity::setPosition(v);
    }

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getName() + "_node";
    request->vec = v;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void Creature::setHP(double nHP)
{
    sem_wait(&hpLockSemaphore);
    hp = nHP;
    sem_post(&hpLockSemaphore);

    updateStatsWindow();
}

double Creature::getHP() const
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

double Creature::getMana() const
{
    sem_wait(&manaLockSemaphore);
    double tempDouble = mana;
    sem_post(&manaLockSemaphore);

    return tempDouble;
}

void Creature::setIsOnMap(bool nIsOnMap)
{
    sem_wait(&isOnMapLockSemaphore);
    isOnMap = nIsOnMap;
    sem_post(&isOnMapLockSemaphore);
}

bool Creature::getIsOnMap() const
{
    sem_wait(&isOnMapLockSemaphore);
    bool tempBool = isOnMap;
    sem_post(&isOnMapLockSemaphore);

    return tempBool;
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
    //TODO: get rid of all these goto label stuff in here

    // If we are not standing somewhere on the map, do nothing.
    if (positionTile() == 0)
    {
        return;
    }

    // Check to see if we have earned enough experience to level up.
    while (exp >= 5 * (getLevel() + powl(getLevel() / 3.0, 2)) && getLevel() < 100)
    {
        doLevelUp();
    }

    // Heal.
    sem_wait(&hpLockSemaphore);
    hp += 0.1;
    if (hp > definition->getMaxHp())
        hp = definition->getMaxHp();
    sem_post(&hpLockSemaphore);

    // Regenrate mana.
    sem_wait(&manaLockSemaphore);
    mana += 0.45;
    if (mana > definition->getMaxMana())
        mana = definition->getMaxMana();
    sem_post(&manaLockSemaphore);

    awakeness -= 0.15;

    // Look at the surrounding area
    updateVisibleTiles();
    visibleEnemyObjects         = getVisibleEnemyObjects();
    reachableEnemyObjects       = getReachableAttackableObjects(visibleEnemyObjects, 0, 0);
    enemyObjectsInRange         = getEnemyObjectsInRange(visibleEnemyObjects);
    livingEnemyObjectsInRange   = GameEntity::removeDeadObjects(enemyObjectsInRange);
    visibleAlliedObjects        = getVisibleAlliedObjects();
    reachableAlliedObjects      = getReachableAttackableObjects(visibleAlliedObjects, 0, 0);

    std::vector<Tile*> markedTiles;

    CreatureAction tempAction;
    // If the creature can see enemies that are reachable.
    if (!reachableEnemyObjects.empty())
    {
        // Check to see if there is any combat actions (maneuvering/attacking) in our action queue.
        bool alreadyFighting = false;
        sem_wait(&actionQueueLockSemaphore);
        for (unsigned int i = 0, size = actionQueue.size(); i < size; ++i)
        {
            if (actionQueue[i].getType() == CreatureAction::attackObject
                    || actionQueue[i].getType() == CreatureAction::maneuver)
            {
                alreadyFighting = true;
                break;
            }
        }
        sem_post(&actionQueueLockSemaphore);

        // If we are not already fighting with a creature or maneuvering then start doing so.
        if (!alreadyFighting)
        {
            if (Random::Double(0.0, 1.0) < (definition->isWorker() ? 0.05 : 0.8))
            {
                tempAction.setType(CreatureAction::maneuver);
                battleFieldAgeCounter = 0;
                pushAction(tempAction);
                // Jump immediately to the action processor since we don't want to decide to
                //train or something if there are enemies around.
                goto creatureActionDoWhileLoop;
            }
        }
    }

    if (battleFieldAgeCounter > 0)
    {
        --battleFieldAgeCounter;
    }

    if (definition->isWorker())
    {
        markedTiles = getVisibleMarkedTiles();
    }
    else
    {
        // Check to see if we have found a "home" tile where we can sleep yet.
        if (Random::Double(0.0, 1.0) < 0.03 && homeTile == 0 && peekAction().getType() != CreatureAction::findHome)
        {
            // Check to see if there are any quarters owned by our color that we can reach.
            std::vector<Room*> tempRooms = getGameMap()->getRoomsByTypeAndColor(Room::quarters, getColor());
            tempRooms = getGameMap()->getReachableRooms(tempRooms, positionTile(), definition->getTilePassability());
            if (!tempRooms.empty())
            {
                tempAction.setType(CreatureAction::findHome);
                pushAction(tempAction);
                goto creatureActionDoWhileLoop;
            }
        }

        // If we have found a home tile to sleep on, see if we are tired enough to want to go to sleep.
        if (homeTile != 0 && 100.0 * powl(Random::Double(0.0, 0.8), 2) > awakeness && peekAction().getType() != CreatureAction::sleep)
        {
            tempAction.setType(CreatureAction::sleep);
            pushAction(tempAction);
        }
        else if (Random::Double(0.0, 1.0) < 0.1 && Random::Double(0.5, 1.0) < awakeness / 100.0 && peekAction().getType() != CreatureAction::train)
        {
            // Check to see if there is a Dojo we can train at.
            //TODO: Check here to see if the controlling seat has any dojo's to train at, if not then don't try to train.
            tempAction.setType(CreatureAction::train);
            pushAction(tempAction);
            trainWait = 0;
        }
    }

    creatureActionDoWhileLoop:

    // The loopback variable allows creatures to begin processing a new
    // action immediately after some other action happens.
    bool            loopBack        = false;
    bool            tempBool        = false;
    bool            stopUsingDojo   = false;
    int             tempInt         = 0;
    unsigned int    tempUnsigned    = 0;
    unsigned int    loops           = 0;
    double          tempDouble      = 0.0;

    GameEntity* tempAttackableObject;

    std::list<Tile*>    tempPath;
    std::list<Tile*>    tempPath2;
    std::vector<Room*>  tempRooms;
    std::vector<Tile*>  neighbors;
    std::vector<Tile*>  claimableTiles;

    //FIXME: This is never initialised with some values (see the other comment addressing this)
    std::pair<LocationType, double> minimumFieldValue;

    do
    {
        ++loops;
        
        loopBack = false;

        // Carry out the current task
        Player* tempPlayer;
        Tile*   tempTile;
        Tile*   myTile;
        Room*   tempRoom;

        tempPath.clear();
        tempPath2.clear();
        tempRooms.clear();
        neighbors.clear();
        claimableTiles.clear();

        sem_wait(&actionQueueLockSemaphore);
        if (!actionQueue.empty())
        {
            CreatureAction topActionItem = actionQueue.front();
            sem_post(&actionQueueLockSemaphore);

            double diceRoll = Random::Double(0.0, 1.0);
            switch (topActionItem.getType())
            {
                case CreatureAction::idle:
                    //cout << "idle ";
                    setAnimationState("Idle");
                    //FIXME: make this into a while loop over a vector of <action, probability> pairs
                    //TODO: This should mainly be decided based on whether there are any
                    //marked and/or unclaimed tiles nearby, not a random value.
                    // Workers only.
                    if (definition->isWorker())
                    {
                        // Decide to check for diggable tiles
                        if (diceRoll < 0.7)
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
                        /*else if (diceRoll < 0.7 + 0.6 * (gold
                                / (double) maxGoldCarriedByWorkers))*/
                        else if(gold != 0)
                        {
                            //TODO: We need a flag to see if we have tried to do this
                            // so the creature won't get confused if we are out of space.
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
                        int tempX = 0, tempY = 0;

                        if (!definition->isWorker())
                        {
                            // Non-workers only.

                            // Check to see if we want to try to follow a worker around or if we want to try to explore.
                            double r = Random::Double(0.0, 1.0);
                            //if(creatureJob == weakFighter) r -= 0.2;
                            if (r < 0.7)
                            {
                                bool workerFound = false;
                                // Try to find a worker to follow around.
                                for (unsigned int i = 0; !workerFound && i
                                        < reachableAlliedObjects.size(); ++i)
                                {
                                    // Check to see if we found a worker.
                                    if (reachableAlliedObjects[i]->getObjectType() == GameEntity::creature
                                            && static_cast<Creature*>(reachableAlliedObjects[i])->definition->isWorker())
                                    {
                                        // We found a worker so find a tile near the worker to walk to.  See if the worker is digging.
                                        tempTile
                                                = reachableAlliedObjects[i]->getCoveredTiles()[0];
                                        if (static_cast<Creature*>(reachableAlliedObjects[i])->peekAction().getType()
                                                == CreatureAction::digTile)
                                        {
                                            // Worker is digging, get near it since it could expose enemies.
                                            tempX = static_cast<double>(tempTile->x) + 3.0
                                                    * Random::gaussianRandomDouble();
                                            tempY = static_cast<double>(tempTile->y) + 3.0
                                                    * Random::gaussianRandomDouble();
                                        }
                                        else
                                        {
                                            // Worker is not digging, wander a bit farther around the worker.
                                            tempX = static_cast<double>(tempTile->x) + 8.0
                                                    * Random::gaussianRandomDouble();
                                            tempY = static_cast<double>(tempTile->y) + 8.0
                                                    * Random::gaussianRandomDouble();
                                        }
                                        workerFound = true;
                                    }

                                    // If there are no workers around, choose tiles far away to "roam" the dungeon.
                                    if (!workerFound)
                                    {
                                        if (!visibleTiles.empty())
                                        {
                                            tempTile
                                                    = visibleTiles[static_cast<unsigned int>(
                                                            Random::Double(0.6, 0.8)
                                                        * (visibleTiles.size()
                                                        - 1)
														)];
                                            tempX = tempTile->x;
                                            tempY = tempTile->y;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // Randomly choose a tile near where we are standing to walk to.
                                if (!visibleTiles.empty())
                                {
                                    unsigned int tileIndex = static_cast<unsigned int>(
                                            visibleTiles.size() * Random::Double(
                                                    0.1, 0.3)
													);
                                    myTile = positionTile();
                                    tempPath = getGameMap()->path(myTile,
                                            visibleTiles[tileIndex],
                                            definition->getTilePassability());
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
                            tempTile = visibleTiles[Random::Uint(
                                    visibleTiles.size() / 2,
                                    visibleTiles.size() - 1)];
                            tempX = tempTile->x;
                            tempY = tempTile->y;
                        }

                        Tile *tempPositionTile = positionTile();
                        std::list<Tile*> result;
                        if (tempPositionTile != NULL)
                        {
                            result = getGameMap()->path(tempPositionTile->x,
                                    tempPositionTile->y, tempX, tempY, definition->getTilePassability());
                        }

                        getGameMap()->cutCorners(result, definition->getTilePassability());
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
                    //TODO: This should be decided based on some aggressiveness parameter.
                    if (Random::Double(0.0, 1.0) < 0.6 && !enemyObjectsInRange.empty())
                    {
                        popAction();
                        tempAction.setType(CreatureAction::attackObject);
                        pushAction(tempAction);
                        clearDestinations();
                        loopBack = true;
                        break;
                    }

                    //TODO: Peek at the item that caused us to walk
                    // If we are walking toward a tile we are trying to dig out, check to see if it is still marked for digging.
                    sem_wait(&actionQueueLockSemaphore);
                    tempBool = (actionQueue[1].getType() == CreatureAction::digTile);
                    sem_post(&actionQueueLockSemaphore);
                    if (tempBool)
                    {
                        tempPlayer = getControllingPlayer();

                        // Check to see if the tile is still marked for digging
                        sem_wait(&walkQueueLockSemaphore);
                        unsigned int index = walkQueue.size();
                        Tile *currentTile = NULL;
                        if (index > 0)
                            currentTile = getGameMap()->getTile((int) walkQueue[index - 1].x,
                                    (int) walkQueue[index - 1].y);

                        sem_post(&walkQueueLockSemaphore);

                        if (currentTile != NULL)
                        {
                            // If it is not marked
                            if (tempPlayer != 0 && !currentTile->getMarkedForDigging(tempPlayer))
                            {
                                // Clear the walk queue
                                clearDestinations();
                            }
                        }
                    }

                    //cout << "walkToTile ";
                    sem_wait(&walkQueueLockSemaphore);
                    if (walkQueue.empty())
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
                    if (myTile == 0)
                    {
                        popAction();
                        break;
                    }

                    // Randomly decide to stop claiming with a small probability
                    if (Random::Double(0.0, 1.0) < 0.1 + 0.2 * markedTiles.size())
                    {
                        // If there are any visible tiles marked for digging start working on that.
                        if (!markedTiles.empty())
                        {
                            loopBack = true;
                            popAction();
                            pushAction(CreatureAction::digTile);
                            break;
                        }
                    }

                    // See if the tile we are standing on can be claimed
                    if ((myTile->getColor() != getColor() || myTile->colorDouble
                            < 1.0) && myTile->isClaimable())
                    {
                        //cout << "\nTrying to claim the tile I am standing on.";
                        // Check to see if one of the tile's neighbors is claimed for our color
                        neighbors = getGameMap()->neighborTiles(myTile);
                        for (unsigned int j = 0; j < neighbors.size(); ++j)
                        {
                            // Check to see if the current neighbor is already claimed
                            tempTile = neighbors[j];
                            if (tempTile->getColor() == getColor()
                                    && tempTile->colorDouble >= 1.0)
                            {
                                //cout << "\t\tFound a neighbor that is claimed.";
                                // If we found a neighbor that is claimed for our side than we can start
                                // dancing on this tile.  If there is "left over" claiming that can be done
                                // it will spill over into neighboring tiles until it is gone.
                                setAnimationState("Claim");
                                myTile->claimForColor(getColor(), definition->getDanceRate());
                                recieveExp(1.5 * (definition->getDanceRate() / (0.35 + 0.05
                                        * getLevel())));

                                // Since we danced on a tile we are done for this turn
                                goto claimTileBreakStatement;
                            }
                        }
                    }

                    // The tile we are standing on is already claimed or is not currently
                    // claimable, find candidates for claiming.
                    // Start by checking the neighbor tiles of the one we are already in
                    neighbors = getGameMap()->neighborTiles(myTile);
                    while (!neighbors.empty())
                    {
                        // If the current neighbor is claimable, walk into it and skip to the end of this turn
                        tempInt = Random::Uint(0, neighbors.size() - 1);
                        tempTile = neighbors[tempInt];
                        //NOTE:  I don't think the "colorDouble" check should happen here.
                        if (tempTile != NULL && tempTile->getTilePassability()
                                == Tile::walkableTile && (tempTile->getColor()
                                != getColor() || tempTile->colorDouble < 1.0)
                                && tempTile->isClaimable())
                        {
                            // The neighbor tile is a potential candidate for claiming, to be an actual candidate
                            // though it must have a neighbor of its own that is already claimed for our side.
                            Tile* tempTile2;
                            std::vector<Tile*> neighbors2 = getGameMap()->neighborTiles(tempTile);
                            for (unsigned int i = 0; i < neighbors2.size(); ++i)
                            {
                                tempTile2 = neighbors2[i];
                                if (tempTile2->getColor() == getColor()
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
                                < 1.0 || tempTile->getColor() != getColor())
                                && tempTile->isClaimable())
                        {
                            // Check to see if one of the tile's neighbors is claimed for our color
                            neighbors = getGameMap()->neighborTiles(visibleTiles[i]);
                            for (unsigned int j = 0; j < neighbors.size(); ++j)
                            {
                                tempTile = neighbors[j];
                                if (tempTile->getColor() == getColor()
                                        && tempTile->colorDouble >= 1.0)
                                {
                                    claimableTiles.push_back(tempTile);
                                }
                            }
                        }
                    }

                    //cout << "  I see " << claimableTiles.size() << " tiles I can claim.";
                    // Randomly pick a claimable tile, plot a path to it and walk to it
                    while (!claimableTiles.empty())
                    {
                        // Randomly find a "good" tile to claim.  A good tile is one that has many neighbors
                        // already claimed, this makes the claimed are more "round" and less jagged.
                        do
                        {
                            int numNeighborsClaimed;

                            // Start by randomly picking a candidate tile.
                            tempTile = claimableTiles[Random::Uint(0,
                                    claimableTiles.size() - 1)];

                            // Count how many of the candidate tile's neighbors are already claimed.
                            neighbors = getGameMap()->neighborTiles(tempTile);
                            numNeighborsClaimed = 0;
                            for (unsigned int i = 0; i < neighbors.size(); ++i)
                            {
                                if (neighbors[i]->getColor() == getColor()
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
                            if (Random::Double(0.0, 1.0) >= bar)
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
                            tempPath = getGameMap()->path(myTile, tempTile, definition->getTilePassability());
                            getGameMap()->cutCorners(tempPath, definition->getTilePassability());
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
                    claimTileBreakStatement:
                    break;

                case CreatureAction::digTile:
                {
                    myTile = positionTile();
                    if (myTile == NULL)
                        break;

                    //cout << "dig ";

                    // Randomly decide to stop digging with a small probability
                    /*
                    if (Random::Double(0.0, 1.0) < 0.35 - 0.2
                            * markedTiles.size())
                    {
                        loopBack = true;
                        popAction();
                        goto claimTileBreakStatement;
                    }*/

                    // See if any of the tiles is one of our neighbors
                    bool wasANeighbor = false;
                    std::vector<Tile*> creatureNeighbors = getGameMap()->neighborTiles(myTile);
                    tempPlayer = getControllingPlayer();
                    for (unsigned int i = 0; i < creatureNeighbors.size() && !wasANeighbor; ++i)
                    {
                        tempTile = creatureNeighbors[i];

                        if (tempPlayer != NULL
                                && tempTile->getMarkedForDigging(tempPlayer))
                        {
                            // We found a tile marked by our controlling seat, dig out the tile.

                            // If the tile is a gold tile accumulate gold for this creature.
                            if (tempTile->getType() == Tile::gold)
                            {
                                //FIXME: Make sure we can't dig gold if the creature has max gold.
                                tempDouble = 5 * std::min(definition->getDigRate(),
                                        tempTile->getFullness());
                                gold += tempDouble;
                                getGameMap()->getSeatByColor(getColor())->goldMined
                                        += tempDouble;
                                recieveExp(5.0 * definition->getDigRate() / 20.0);
                            }

                            // Turn so that we are facing toward the tile we are going to dig out.
                            faceToward(tempTile->x, tempTile->y);

                            // Dig out the tile by decreasing the tile's fullness.
                            setAnimationState("Dig");
                            double amountDug = tempTile->digOut(definition->getDigRate(), true);
                            if(amountDug > 0.0)
                            {
                                recieveExp(1.5 * definition->getDigRate() / 20.0);

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
                                sound->setPosition(getPosition());
                                sound->play(CreatureSound::DIG);
                            }
                            else
                            {
                                //We tried to dig a tile we are not able to
                                //Completely bail out if this happens.
                                clearActionQueue();
                            }

                            wasANeighbor = true;

                            //Set sound position and play dig sound.
                            
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
                    std::vector<std::list<Tile*> > possiblePaths;
                    Tile* neighborTile;
                    for (unsigned int i = 0; i < markedTiles.size(); ++i)
                    {
                        neighbors = getGameMap()->neighborTiles(markedTiles[i]);
                        for (unsigned int j = 0; j < neighbors.size(); ++j)
                        {
                            neighborTile = neighbors[j];
                            if (neighborTile != 0 && neighborTile->getFullness() < 1)
                                possiblePaths.push_back(getGameMap()->path(
                                        positionTile(), neighborTile, definition->getTilePassability()));
                        }
                    }

                    // Find the shortest path and start walking toward the tile to be dug out
                    if (!possiblePaths.empty())
                    {
                        // Find the N shortest valid paths, see if there are any valid paths shorter than this first guess
                        std::vector<std::list<Tile*> > shortPaths;
                        //shortPaths.clear();
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
                            shortestIndex = Random::Uint(0, numShortPaths - 1);
                            std::list<Tile*> walkPath = shortPaths[shortestIndex];

                            // If the path is a legitimate path, walk down it to the tile to be dug out
                            getGameMap()->cutCorners(walkPath, definition->getTilePassability());
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
                    tempBool = (actionQueue.front().getType() == CreatureAction::digTile);
                    sem_post(&actionQueueLockSemaphore);
                    if (tempBool)
                    {
                        popAction();
                        loopBack = true;
                    }
                    break;
                }

                case CreatureAction::depositGold:
                {
                    // Check to see if we are standing in a treasury.
                    myTile = positionTile();
                    if (myTile != 0)
                    {
                        tempRoom = myTile->getCoveringRoom();
                        if (tempRoom != NULL && tempRoom->getType() == Room::treasury)
                        {
                            // Deposit as much of the gold we are carrying as we can into this treasury.
                            gold -= static_cast<RoomTreasury*>(tempRoom)->depositGold(gold, myTile);

                            // Depending on how much gold we have left (what did not fit in this treasury) we may want to continue
                            // looking for another treasury to put the gold into.  Roll a dice to see if we want to quit looking not.
                            if (Random::Double(1.0, maxGoldCarriedByWorkers) > gold)
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
                    std::vector<Room*> treasuriesOwned = getGameMap()->getRoomsByTypeAndColor(
                            Room::treasury, getColor());
                    if (!treasuriesOwned.empty())
                    {
                        Tile *nearestTreasuryTile;
                        nearestTreasuryTile = NULL;
                        unsigned int nearestTreasuryDistance;
                        nearestTreasuryDistance = 0; // to avoid a compilation warning
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
                                tempUnsigned = Random::Uint(0,
                                        treasuriesOwned[i]->numCoveredTiles()
                                                - 1);
                                nearestTreasuryTile
                                        = treasuriesOwned[i]->getCoveredTile(
                                                tempUnsigned);
                                tempPath = getGameMap()->path(myTile,
                                        nearestTreasuryTile, definition->getTilePassability());
                                if (tempPath.size() >= 2
                                        && static_cast<RoomTreasury*>(treasuriesOwned[i])->emptyStorageSpace() > 0)
                                {
                                    validPathFound = true;
                                    nearestTreasuryDistance = tempPath.size();
                                }
                            }
                            else
                            {
                                // We have already found at least one valid path to a treasury, see if this one is closer.
                                tempUnsigned = Random::Uint(0,
                                        treasuriesOwned[i]->numCoveredTiles()
                                                - 1);
                                tempTile = treasuriesOwned[i]->getCoveredTile(
                                        tempUnsigned);
                                tempPath2 = getGameMap()->path(myTile, tempTile,
                                        definition->getTilePassability());
                                if (tempPath2.size() >= 2 && tempPath2.size()
                                        < nearestTreasuryDistance
                                        && static_cast<RoomTreasury*>(treasuriesOwned[i])->emptyStorageSpace() > 0)
                                {
                                    tempPath = tempPath2;
                                    nearestTreasuryDistance = tempPath.size();
                                }
                            }
                        }

                        if (validPathFound)
                        {
                            // Begin walking to this treasury.
                            getGameMap()->cutCorners(tempPath, definition->getTilePassability());
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
                        LogManager::getSingleton().logMessage("No space to put gold for creature for player "
                            + Ogre::StringConverter::toString(getColor()));
                        break;
                    }

                    // If we get to here, there is either no treasuries controlled by us, or they are all
                    // unreachable, or they are all full, so quit trying to deposit gold.
                    popAction();
                    loopBack = true;
                    LogManager::getSingleton().logMessage("No space to put gold for creature for player "
                        + Ogre::StringConverter::toString(getColor()));
                    break;
                }

                case CreatureAction::findHome:
                    // Check to see if we are standing in an open quarters tile that we can claim as our home.
                    myTile = positionTile();
                    if (myTile != NULL)
                    {
                        tempRoom = myTile->getCoveringRoom();
                        if (tempRoom != NULL && tempRoom->getType() == Room::quarters)
                        {
                            if (static_cast<RoomQuarters*>(tempRoom)->claimTileForSleeping(myTile, this))
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
                    tempRooms = getGameMap()->getRoomsByTypeAndColor(Room::quarters, getColor());
                    std::random_shuffle(tempRooms.begin(), tempRooms.end());
                    unsigned int nearestQuartersDistance;
                    nearestQuartersDistance = 0; // to avoid a compilation warning
                    bool validPathFound;
                    validPathFound = false;
                    tempPath.clear();
                    tempPath2.clear();
                    for (unsigned int i = 0; i < tempRooms.size(); ++i)
                    {
                        // Get the list of open rooms at the current quarters and check to see if
                        // there is a place where we could put a bed big enough to sleep in.
                        tempTile = static_cast<RoomQuarters*>(tempRooms[i])->getLocationForBed(
                                        definition->getBedDim1(), definition->getBedDim2());

                        // If the previous attempt to place the bed in this quarters failed, try again with the bed the other way.
                        if (tempTile == NULL)
                            tempTile
                                    = static_cast<RoomQuarters*>(tempRooms[i])->getLocationForBed(
                                            definition->getBedDim2(), definition->getBedDim1());

                        // Check to see if either of the two possible bed orientations tried above resulted in a successful placement.
                        if (tempTile != NULL)
                        {
                            tempPath2 = getGameMap()->path(myTile, tempTile,
                                    definition->getTilePassability());

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
                        getGameMap()->cutCorners(tempPath, definition->getTilePassability());
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
                        tempPath = getGameMap()->path(myTile, homeTile,
                                definition->getTilePassability());
                        getGameMap()->cutCorners(tempPath, definition->getTilePassability());
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
                {
                    // Creatures can only train to level 10 at a dojo.
                    //TODO: Check to see if the dojo has been upgraded to allow training to a higher level.
                    stopUsingDojo = false;
                    if (getLevel() > 10)
                    {
                        popAction();
                        loopBack = true;
                        trainWait = 0;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    // Randomly decide to stop training, we are more likely to stop when we are tired.
                    if (100.0 * powl(Random::Double(0.0, 1.0), 2) > awakeness)
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
                        if (tempRoom != 0 && tempRoom->getType() == Room::dojo
                                && tempRoom->numOpenCreatureSlots() > 0)
                        {
                            // Train at this dojo.
                            trainingDojo = static_cast<RoomDojo*>(tempRoom);
                            trainingDojo->addCreatureUsingRoom(this);
                            tempTile = tempRoom->getCentralTile();
                            faceToward(tempTile->x, tempTile->y);
                            setAnimationState("Attack1");
                            recieveExp(5.0);
                            awakeness -= 5.0;
                            trainWait = Random::Uint(3, 8);
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
                    tempRooms = getGameMap()->getRoomsByTypeAndColor(Room::dojo, getColor());

                    if (tempRooms.empty())
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
                        tempInt = Random::Uint(0, tempRooms.size() - 1);
                        tempRoom = tempRooms[tempInt];
                        tempRooms.erase(tempRooms.begin() + tempInt);
                        tempDouble = 1.0 / (maxTrainDistance
                                - getGameMap()->crowDistance(myTile,
                                        tempRoom->getCoveredTile(0)));
                        if (Random::Double(0.0, 1.0) < tempDouble)
                            break;
                        ++tempInt;
                    } while (tempInt < 5 && tempRoom->numOpenCreatureSlots()
                            == 0 && !tempRooms.empty());

                    if (tempRoom->numOpenCreatureSlots() == 0)
                    {
                        // The room is already being used, stop trying to train.
                        popAction();
                        loopBack = true;
                        stopUsingDojo = true;
                        goto trainBreakStatement;
                    }

                    tempTile = tempRoom->getCoveredTile(Random::Uint(0,
                            tempRoom->numCoveredTiles() - 1));
                    tempPath = getGameMap()->path(myTile, tempTile, definition->getTilePassability());
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
                }

                case CreatureAction::attackObject:
                    // If there are no more enemies which are reachable, stop attacking
                    if (reachableEnemyObjects.empty())
                    {
                        popAction();
                        loopBack = true;
                        break;
                    }

                    myTile = positionTile();

                    // Find the first enemy close enough to hit and attack it
                    if (!livingEnemyObjectsInRange.empty())
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
                        double damageDone = getHitroll(getGameMap()->crowDistance(
                                myTile, tempTile));
                        damageDone *= Random::Double(0.0, 1.0);
                        damageDone -= powl(Random::Double(0.0, 0.4), 2.0)
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
                        if(tempAttackableObject->getObjectType() == GameEntity::creature)
                        {
                            Creature* tempCreature = static_cast<Creature*>(tempAttackableObject);
                            tempCreature->recieveExp(0.15 * expGained);

                            // Add a bonus modifier based on the level of the creature we hit
                            // to expGained and give ourselves that much experience.
                            if (tempCreature->getLevel() >= getLevel())
                                expGained *= 1.0 + (tempCreature->getLevel() - getLevel()) / 10.0;
                            else
                                expGained /= 1.0 + (getLevel() - tempCreature->getLevel()) / 10.0;
                        }
                        recieveExp(expGained);

                        std::cout << "\n" << getName() << " did " << damageDone
                                << " damage to "
                                //FIXME: Attackabe object needs a name...
                                << "";
                                //<< tempAttackableObject->getName();
                        std::cout << " who now has " << tempAttackableObject->getHP(
                                tempTile) << "hp";

                        // Randomly decide to start maneuvering again so we don't just stand still and fight.
                        if (Random::Double(0.0, 1.0) <= 0.6)
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
                    if (!livingEnemyObjectsInRange.empty())
                    {
                        popAction();
                        loopBack = true;

                        // If the next action down the stack is not an attackObject action, add it.
                        sem_wait(&actionQueueLockSemaphore);
                        tempBool = (actionQueue.front().getType() != CreatureAction::attackObject);
                        sem_post(&actionQueueLockSemaphore);
                        if (tempBool)
                            pushAction(CreatureAction::attackObject);

                        break;
                    }

                    // If there are no more enemies which are reachable, stop maneuvering.
                    if (reachableEnemyObjects.empty())
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
                     tempTile = getGameMap()->getTile(positionTile()->x + tempVector.x, positionTile()->y + tempVector.y);
                     if(tempTile != NULL)
                     {
                     tempPath = getGameMap()->path(positionTile(), tempTile, tilePassability);

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
                        battleFieldAgeCounter = Random::Uint(2, 6);
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
                    tempDouble = std::max(weaponL->getRange(), weaponR->getRange()); // Pick a true destination randomly within the max range of our weapons.
                    tempDouble = sqrt(tempDouble);
                    //FIXME:  This should find a path to a tile we can walk to, it does not always
                    //do this the way it is right now. Because minimumFieldValue is never initialisesed...
                    tempPath = getGameMap()->path(positionTile()->x,
                            positionTile()->y, minimumFieldValue.first.first
                                    + Random::Double(-1.0 * tempDouble,
                                            tempDouble),
                            minimumFieldValue.first.second + Random::Double(-1.0
                                    * tempDouble, tempDouble), definition->getTilePassability());

                    // Walk a maximum of N tiles before recomputing the destination since we are in combat.
                    tempUnsigned = 5;
                    if (tempPath.size() >= tempUnsigned)
                        tempPath.resize(tempUnsigned);

                    getGameMap()->cutCorners(tempPath, definition->getTilePassability());
                    if (setWalkPath(tempPath, 2, false))
                    {
                        setAnimationState(tempBool ? "Walk" : "Flee");
                    }

                    // Push a walkToTile action into the creature's action queue to make them walk the path they have
                    // decided on without recomputing, this helps prevent them from getting stuck in local minima.
                    pushAction(CreatureAction::walkToTile);

                    // This is a debugging statement, it produces a visual display of the battlefield as seen by the first creature.
                    /*
                    if (battleField->name.compare("field_1") == 0)
                    {
                        battleField->refreshMeshes(1.0);
                    }*/
                    break;

                default:
                    std::cerr << "\n\nERROR:  Unhandled action type in Creature::doTurn().\n\n";
                    exit(1);
                    break;
            }
        }
        else
        {
            sem_post(&actionQueueLockSemaphore);
            std::cerr << "\n\nERROR:  Creature has empty action queue in doTurn(), this should not happen.\n\n";
            exit(1);
        }
    } while (loopBack && loops < 20);

    if(loops >= 20)
    {
        LogManager::getSingleton().logMessage("> 20 loops in Creature::doTurn name:" + getName() +
                " colour: " + Ogre::StringConverter::toString(getColor()) + ". Breaking out..");
    }

    // Update the visual debugging entities
    //if we are standing in a different tile than we were last turn
    if (hasVisualDebuggingEntities && positionTile() != previousPositionTile)
    {
        //TODO: This destroy and re-create is kind of a hack as its likely only a few
        //tiles will actually change.
        destroyVisualDebugEntities();
        createVisualDebugEntities();
    }
}

double Creature::getHitroll(double range)
{
    double tempHitroll = 1.0;

    if (weaponL != 0 && weaponL->getRange() >= range)
        tempHitroll += weaponL->getDamage();
    if (weaponR != 0 && weaponR->getRange() >= range)
        tempHitroll += weaponR->getDamage();
    tempHitroll *= log((double) log((double) getLevel() + 1) + 1);

    return tempHitroll;
}

double Creature::getDefense() const
{
    double returnValue = 3.0;
    if (weaponL != NULL)
        returnValue += weaponL->getDefense();
    if (weaponR != NULL)
        returnValue += weaponR->getDefense();

    return returnValue;
}

/** \brief Increases the creature's level, adds bonuses to stat points, changes the mesh, etc.
 *
 */
void Creature::doLevelUp()
{
    if (getLevel() >= MAX_LEVEL)
        return;

    setLevel(getLevel() + 1);
    std::cout << "\n\n" << getName() << " has reached level " << getLevel() << "\n";

    if (definition->isWorker())
    {
        digRate += 4.0 * getLevel() / (getLevel() + 5.0);
        danceRate += 0.12 * getLevel() / (getLevel() + 5.0);
    }
    std::cout << "New dig rate: " << digRate << "\tnew dance rate: " << danceRate << "\n";

    moveSpeed += 0.4 / (getLevel() + 2.0);
    //if(digRate > 60)  digRate = 60;

    maxHP += definition->getHpPerLevel();
    maxMana += definition->getManaPerLevel();

    // Scale up the mesh.
    if (isMeshExisting() && ((getLevel() <= 30 && getLevel() % 2 == 0) || (getLevel() > 30 && getLevel()
            % 3 == 0)))
    {
		Ogre::Real scaleFactor = 1.0 + static_cast<double>(getLevel()) / 250.0;
        if (scaleFactor > 1.03)
            scaleFactor = 1.04;
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::scaleSceneNode;
        request->p = sceneNode;
        request->vec = Ogre::Vector3(scaleFactor, scaleFactor, scaleFactor);

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
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
    //double effectiveRadius = sightRadius;
    //visibleTiles = getGameMap()->visibleTiles(positionTile(), effectiveRadius);
    visibleTiles = getGameMap()->visibleTiles(positionTile(), definition->getSightRadius());
}

/*! \brief Loops over the visibleTiles and adds all enemy creatures in each tile to a list which it returns.
 *
 */
std::vector<GameEntity*> Creature::getVisibleEnemyObjects()
{
    return getVisibleForce(getColor(), true);
}

/*! \brief Loops over objectsToCheck and returns a vector containing all the ones which can be reached via a valid path.
 *
 */
std::vector<GameEntity*> Creature::getReachableAttackableObjects(
        const std::vector<GameEntity*> &objectsToCheck,
        unsigned int *minRange, GameEntity **nearestObject)
{
    std::vector<GameEntity*> tempVector;
    Tile *myTile = positionTile(), *objectTile;
    std::list<Tile*> tempPath;
    bool minRangeSet = false;

    // Loop over the vector of objects we are supposed to check.
    for (unsigned int i = 0; i < objectsToCheck.size(); ++i)
    {
        // Try to find a valid path from the tile this creature is in to the nearest tile where the current target object is.
        //TODO:  This should be improved so it picks the closest tile rather than just the [0] tile.
        objectTile = objectsToCheck[i]->getCoveredTiles()[0];
        if (getGameMap()->pathExists(myTile->x, myTile->y, objectTile->x,
                objectTile->y, definition->getTilePassability()))
        {
            tempVector.push_back(objectsToCheck[i]);

            if (minRange != NULL)
            {
                //TODO:  If this could be computed without the path call that would be better.
                tempPath = getGameMap()->path(myTile, objectTile, definition->getTilePassability());

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
std::vector<GameEntity*> Creature::getEnemyObjectsInRange(
        const std::vector<GameEntity*> &enemyObjectsToCheck)
{
    std::vector<GameEntity*> tempVector;

    // If there are no enemies to check we are done.
    if (enemyObjectsToCheck.empty())
        return tempVector;

    // Find our location and calculate the square of the max weapon range we have.
    Tile *myTile = positionTile();
    double weaponRangeSquared = std::max(weaponL->getRange(), weaponR->getRange());
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
std::vector<GameEntity*> Creature::getVisibleAlliedObjects()
{
    return getVisibleForce(getColor(), false);
}

/*! \brief Loops over the visibleTiles and adds any which are marked for digging to a vector which it returns.
 *
 */
std::vector<Tile*> Creature::getVisibleMarkedTiles()
{
    std::vector<Tile*> tempVector;
    Player *tempPlayer = getControllingPlayer();

    // Loop over all the visible tiles.
    for (unsigned int i = 0, size = visibleTiles.size(); i < size; ++i)
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
std::vector<GameEntity*> Creature::getVisibleForce(int color, bool invert)
{
    return getGameMap()->getVisibleForce(visibleTiles, color, invert);
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
            request->p2 = static_cast<void*>(this);

            // Add the request to the queue of rendering operations to be performed before the next frame.
            RenderManager::queueRenderRequest(request);

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
            request->p2 = static_cast<void*>(this);

            // Add the request to the queue of rendering operations to be performed before the next frame.
            RenderManager::queueRenderRequest(request);
        }
    }

}

/*! \brief Returns a pointer to the tile the creature is currently standing in.
 *
 */
Tile* Creature::positionTile()
{
    Ogre::Vector3 tempPosition = getPosition();

    return getGameMap()->getTile((int) (tempPosition.x), (int) (tempPosition.y));
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

/*! \brief Creates a string with a unique number embedded into it so the creature's name will not be the same as any other OGRE entity name.
 *
 */
std::string Creature::getUniqueCreatureName()
{
    static int uniqueNumber = 1;
    std::string tempString = definition->getClassName() + Ogre::StringConverter::toString(
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

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
    CEGUI::Window* rootWindow = CEGUI::System::getSingleton().getGUISheet();

    statsWindow = wmgr->createWindow("OD/FrameWindow",
            std::string("Root/CreatureStatsWindows/") + getName());
    statsWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.7, 0), CEGUI::UDim(0.65, 0)));
    statsWindow->setSize(CEGUI::UVector2(CEGUI::UDim(0.25, 0), CEGUI::UDim(0.3, 0)));

    CEGUI::Window *textWindow = wmgr->createWindow("OD/StaticText",
            statsWindow->getName() + "TextDisplay");
    textWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.05, 0), CEGUI::UDim(0.15, 0)));
    textWindow->setSize(CEGUI::UVector2(CEGUI::UDim(0.9, 0), CEGUI::UDim(0.8, 0)));
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
    tempSS << "Creature name: " << getName() << "\n";
    tempSS << "HP: " << getHP() << " / " << maxHP << "\n";
    tempSS << "Mana: " << getMana() << " / " << maxMana << "\n";
    sem_wait(&actionQueueLockSemaphore);
    tempSS << "AI State: " << actionQueue.front().toString() << "\n";
    sem_post(&actionQueueLockSemaphore);
    return tempSS.str();
}

/** \brief Sets the creature definition for this creature
 * 
 */
void Creature::setCreatureDefinition(const CreatureDefinition* def)
{
    definition = def;
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
    if (experience < 0)
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

    if (getGameMap()->getLocalPlayer()->getSeat()->getColor() == getColor())
    {
        return getGameMap()->getLocalPlayer();
    }

    // Try to find and return a player with color equal to this creature's
    for (unsigned int i = 0, numPlayers = getGameMap()->numPlayers();
            i < numPlayers; ++i)
    {
        tempPlayer = getGameMap()->getPlayer(i);
        if (tempPlayer->getSeat()->getColor() == getColor())
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
    Tile *tempTile;
    int xDist, yDist;
    GameEntity* tempObject;

    // Loop over the tiles in this creature's battleField and compute their value.
    // The creature will then walk towards the tile with the minimum value to
    // attack or towards the maximum value to retreat.
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
            if ( ! (    tempObject->getObjectType() == GameEntity::creature
                     || tempObject->getObjectType() == GameEntity::trap)
                     || tempObject->getHP(0) <= 0.0)
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
        battleField->set(tempTile->x, tempTile->y, (tileValue + Random::Double(
                -1.0 * jitter, jitter)) * tileScaleFactor);
    }
}
