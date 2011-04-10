//#if defined(WIN32) || defined(_WIN32)
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <iostream>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>

#include <pthread.h>

#include "Globals.h"
#include "Functions.h"
#include "GameMap.h"
#include "RadialVector2.h"
#include "Tile.h"
#include "Creature.h"
#include "Player.h"
#include "Trap.h"
#include "Seat.h"
#include "MapLight.h"
#include "ProtectedObject.h"
#include "TileCoordinateMap.h"
#include "MissileObject.h"
#include "Weapon.h"

GameMap::GameMap() :
        nextUniqueFloodFillColor(1),
        loadNextLevel(false),
        floodFillEnabled(false),
        numCallsTo_path(0),
        averageAILeftoverTime(0.0),
        tileCoordinateMap(new TileCoordinateMap(100)),
        maxAIThreads(2),
        me(NULL),
        miscUpkeepTime(0),
        creatureTurnsTime(0),
        length(0),
        width(0)
{
    sem_init(&threadReferenceCountLockSemaphore, 0, 1);
    sem_init(&creaturesLockSemaphore, 0, 1);
    sem_init(&animatedObjectsLockSemaphore, 0, 1);
    sem_init(&activeObjectsLockSemaphore, 0, 1);
    sem_init(&newActiveObjectsLockSemaphore, 0, 1);
    sem_init(&tilesLockSemaphore, 0, 1);
}

/*! \brief Erase all creatures, tiles, etc. from the map and make a new rectangular one.
 *
 * The new map consists entirely of the same kind of tile, with no creature
 * classes loaded, no players, and no creatures.
 */
void GameMap::createNewMap(int xSize, int ySize)
{
    Tile *tempTile;
    char array[255];

    clearAll();

    for (int j = 0; j < ySize; ++j)
    {
        for (int i = 0; i < xSize; ++i)
        {
            tempTile = new Tile;
            tempTile->setType(Tile::dirt);
            tempTile->setFullness(100.0);
            tempTile->x = i;
            tempTile->y = j;

            snprintf(array, sizeof(array), "Level_%3i_%3i", i, j);
            tempTile->name = array;
            tempTile->createMesh();
            sem_wait(&tilesLockSemaphore);
            tiles.insert(std::pair<std::pair<int, int> , Tile*> (std::pair<int, int> (i, j),
                    tempTile));
            sem_post(&tilesLockSemaphore);
        }
    }

    // Loop over all the tiles and force them to examine their
    // neighbors.  This allows them to switch to a mesh with fewer
    // polygons if some are hidden by the neighbors.
    for(TileMap_t::iterator itr = gameMap.firstTile(), last = gameMap.lastTile();
            itr != last; ++itr)
    {
        itr->second->setFullness(itr->second->getFullness());
    }
    length = ySize;
    width = xSize;
}

/*! \brief Returns a pointer to the tile at location (x, y).
 *
 * The tile pointers are stored internally in a map so calls to this function
 * have a complexity O(log(N)) where N is the number of tiles in the map.
 */
Tile* GameMap::getTile(int x, int y)
{
    Tile *returnValue = NULL;
    std::pair<int, int> location(x, y);

    sem_wait(&tilesLockSemaphore);
    TileMap_t::iterator itr = tiles.find(location);
    returnValue = (itr != tiles.end()) ? itr->second : NULL;
    sem_post(&tilesLockSemaphore);

    return returnValue;
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles, creatures, classes, and players in the GameMap.
 *
 */
void GameMap::clearAll()
{
    clearCreatures();
    clearClasses();
    clearTraps();

    clearMapLights();
    clearRooms();
    clearTiles();

    clearGoalsForAllSeats();
    clearEmptySeats();
    clearPlayers();
    clearFilledSeats();
}

/*! \brief Clears the mesh and deletes the data structure for all the tiles in the GameMap.
 *
 */
void GameMap::clearTiles()
{
    sem_wait(&tilesLockSemaphore);
    for(TileMap_t::iterator itr = tiles.begin(), end = tiles.end();
            itr != end; ++itr)
    {
        itr->second->deleteYourself();
    }

    tiles.clear();
    sem_post(&tilesLockSemaphore);
}

/*! \brief Clears the mesh and deletes the data structure for all the creatures in the GameMap.
 *
 */
void GameMap::clearCreatures()
{
    sem_wait(&creaturesLockSemaphore);
    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        removeAnimatedObject(creatures[i]);
        creatures[i]->deleteYourself();
    }

    creatures.clear();
    sem_post(&creaturesLockSemaphore);
}

/*! \brief Deletes the data structure for all the creature classes in the GameMap.
 *
 */
void GameMap::clearClasses()
{
    for (unsigned int i = 0; i < numClassDescriptions(); ++i)
    {
        delete classDescriptions[i];
    }

    classDescriptions.clear();
}

/*! \brief Deletes the data structure for all the players in the GameMap.
 *
 */
void GameMap::clearPlayers()
{
    for (unsigned int i = 0; i < numPlayers(); ++i)
    {
        delete players[i];
    }

    players.clear();
}

/*! \brief Returns the number of tile pointers currently stored in this GameMap.
 *
 */
unsigned int GameMap::numTiles()
{
    sem_wait(&tilesLockSemaphore);
    unsigned int tempUnsigned = tiles.size();
    sem_post(&tilesLockSemaphore);

    return tempUnsigned;
}

/*! \brief Adds the address of a new tile to be stored in this GameMap.
 *
 */
void GameMap::addTile(Tile *t)
{
    // Notify the neighbor tiles already existing on the GameMap of our existance.
    for (unsigned int i = 0; i < 4; ++i)
    {
        int tempX = t->x, tempY = t->y;
        switch (i)
        {
            break;
        case 0:
            ++tempX;
            break;
        case 1:
            ++tempY;
            break;
        case 2:
            --tempX;
            break;
        case 3:
            --tempY;
            break;

        default:
            std::cerr << "\n\n\nERROR:  Unknown neighbor index.\n\n\n";
            exit(1);
        }

        // If the current neigbor tile exists, add the current tile as one of its
        // neighbors and add it as one of the current tile's neighbors.
        Tile *tempTile = getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            tempTile->addNeighbor(t);
            t->addNeighbor(tempTile);
        }
    }

    sem_wait(&tilesLockSemaphore);
    tiles.insert(std::pair<std::pair<int, int> , Tile*> (std::pair<int, int> (t->x, t->y), t));
    sem_post(&tilesLockSemaphore);
}

/** \brief Returns all the valid tiles in the rectangular region specified by the two corner points given.
 *
 */
std::vector<Tile*> GameMap::rectangularRegion(int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> returnList;
    Tile *tempTile;

    if (x1 > x2)
        swap(x1, x2);
    if (y1 > y2)
        swap(y1, y2);

    for (int i = x1; i <= x2; ++i)
    {
        for (int j = y1; j <= y2; ++j)
        {
            //TODO:  This routine could be sped up by using the neighborTiles function.
            tempTile = getTile(i, j);

            if (tempTile != NULL)
                returnList.push_back(tempTile);
        }
    }

    return returnList;
}

/** \brief Returns all the valid tiles in the curcular region surrounding the given point and extending outward to the specified radius.
 *
 */
std::vector<Tile*> GameMap::circularRegion(int x, int y, double radius)
{
    std::vector<Tile*> returnList;
    Tile *tempTile;
    int xDist, yDist, distSquared;
    double radiusSquared = radius * radius;

    if (radius < 0.0)
        radius = 0.0;

    for (int i = x - radius; i <= x + radius; ++i)
    {
        for (int j = y - radius; j <= y + radius; ++j)
        {
            //TODO:  This routine could be sped up by using the neighborTiles function.
            xDist = i - x;
            yDist = j - y;
            distSquared = xDist * xDist + yDist * yDist;
            if (distSquared < radiusSquared)
            {
                tempTile = getTile(i, j);
                if (tempTile != NULL)
                    returnList.push_back(tempTile);
            }
        }
    }
    return returnList;
}

/** \brief Returns a vector of all the valid tiles which are a neighbor to one or more tiles in the specified region, i.e. the "perimeter" of the region extended out one tile.
 *
 */
std::vector<Tile*> GameMap::tilesBorderedByRegion(
        const std::vector<Tile*> &region)
{
    std::vector<Tile*> neighbors, returnList;

    // Loop over all the tiles in the specified region.
    for (unsigned int i = 0; i < region.size(); ++i)
    {
        // Get the tiles bordering the current tile and loop over them.
        neighbors = neighborTiles(region[i]);
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            bool neighborFound = false;

            // Check to see if the current neighbor is one of the tiles in the region.
            for (unsigned int k = 0; k < region.size(); ++k)
            {
                if (region[k] == neighbors[j])
                {
                    neighborFound = true;
                    break;
                }
            }

            if (!neighborFound)
            {
                // Check to see if the current neighbor is already in the returnList.
                for (unsigned int k = 0; k < returnList.size(); ++k)
                {
                    if (returnList[k] == neighbors[j])
                    {
                        neighborFound = true;
                        break;
                    }
                }
            }

            // If the given neighbor was not already in the returnList, then add it.
            if (!neighborFound)
                returnList.push_back(neighbors[j]);
        }
    }

    return returnList;
}

/*! \brief Adds the address of a new class description to be stored in this GameMap.
 *
 * The class descriptions take the form of a creature data structure with most
 * of the data members filled in.  This class structure is then copied into the
 * data structure of new creatures that are added who are of this class.  The
 * copied members include things like HP, mana, etc, that are the same for all
 * members of that class.  Creature specific things like location, etc. are
 * then filled out for the individual creature.
 */
void GameMap::addClassDescription(CreatureClass *c)
{
    classDescriptions.push_back(c);
}

/*! \brief Copies the creature class structure into a newly created structure and stores the address of the new structure in this GameMap.
 *
 */
void GameMap::addClassDescription(CreatureClass c)
{
    classDescriptions.push_back(new CreatureClass(c));
}

/*! \brief Adds the address of a new creature to be stored in this GameMap.
 *
 */
void GameMap::addCreature(Creature *c)
{
    sem_wait(&creaturesLockSemaphore);
    creatures.push_back(c);
    sem_post(&creaturesLockSemaphore);

    c->positionTile()->addCreature(c);

    addAnimatedObject(c);

    sem_wait(&c->isOnMapLockSemaphore);
    c->isOnMap = true;
    sem_post(&c->isOnMapLockSemaphore);
}

/*! \brief Removes the creature from the game map but does not delete its data structure.
 *
 */
void GameMap::removeCreature(Creature *c)
{
    sem_wait(&creaturesLockSemaphore);

    // Loop over the creatures looking for creature c
    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        if (c == creatures[i])
        {
            // Creature found
            // Remove the creature from the tile it's in
            c->positionTile()->removeCreature(c);
            creatures.erase(creatures.begin() + i);
            break;
        }
    }

    sem_post(&creaturesLockSemaphore);

    removeAnimatedObject(c);

    sem_wait(&c->isOnMapLockSemaphore);
    c->isOnMap = false;
    sem_post(&c->isOnMapLockSemaphore);
}

/** \brief Adds the given creature to the queue of creatures to be deleted in a future turn
 * when it is safe to do so after all references to the creature have been cleared.
 *
 */
void GameMap::queueCreatureForDeletion(Creature *c)
{
    // If the creature has a homeTile where they sleep, their bed needs to be destroyed.
    if (c->homeTile != NULL)
        ((RoomQuarters*) c->homeTile->getCoveringRoom())->releaseTileForSleeping(
                c->homeTile, c);

    // Remove the creature from the GameMap in case the caller forgot to do so.
    removeCreature(c);

    //TODO: This needs to include the turn number that the creature was pushed so proper multithreaded locks can be by the threads to retire the creatures.
    creaturesToDelete[turnNumber.get()].push_back(c);
}

/*! \brief Returns a pointer to the first class description whose 'name' parameter matches the query string.
 *
 */
CreatureClass* GameMap::getClassDescription(std::string query)
{
    for (unsigned int i = 0; i < classDescriptions.size(); ++i)
    {
        if (classDescriptions[i]->className.compare(query) == 0)
            return classDescriptions[i];
    }

    return NULL;
}

/*! \brief Returns the total number of creatures stored in this game map.
 *
 */
unsigned int GameMap::numCreatures()
{
    sem_wait(&creaturesLockSemaphore);
    unsigned int tempUnsigned = creatures.size();
    sem_post(&creaturesLockSemaphore);

    return tempUnsigned;
}

/*! \brief Returns a vector containing all the creatures controlled by the given seat.
 *
 */
std::vector<Creature*> GameMap::getCreaturesByColor(int color)
{
    std::vector<Creature*> tempVector;

    // Loop over all the creatures in the GameMap and add them to the temp vector if their color matches that of the desired seat.
    sem_wait(&creaturesLockSemaphore);
    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        if (creatures[i]->color == color)
            tempVector.push_back(creatures[i]);
    }
    sem_post(&creaturesLockSemaphore);

    return tempVector;
}

void GameMap::clearAnimatedObjects()
{
    sem_wait(&animatedObjectsLockSemaphore);
    animatedObjects.clear();
    sem_post(&animatedObjectsLockSemaphore);
}

void GameMap::addAnimatedObject(AnimatedObject *a)
{
    sem_wait(&animatedObjectsLockSemaphore);
    animatedObjects.push_back(a);
    sem_post(&animatedObjectsLockSemaphore);
}

void GameMap::removeAnimatedObject(AnimatedObject *a)
{
    sem_wait(&animatedObjectsLockSemaphore);

    // Loop over the animatedObjects looking for animatedObject a
    for (unsigned int i = 0; i < animatedObjects.size(); ++i)
    {
        if (a == animatedObjects[i])
        {
            // AnimatedObject found
            animatedObjects.erase(animatedObjects.begin() + i);
            break;
        }
    }

    sem_post(&animatedObjectsLockSemaphore);
}

AnimatedObject* GameMap::getAnimatedObject(int index)
{
    sem_wait(&animatedObjectsLockSemaphore);
    AnimatedObject* tempAnimatedObject = animatedObjects[index];
    sem_post(&animatedObjectsLockSemaphore);

    return tempAnimatedObject;
}

AnimatedObject* GameMap::getAnimatedObject(std::string name)
{
    AnimatedObject* tempAnimatedObject = NULL;

    sem_wait(&animatedObjectsLockSemaphore);
    for (unsigned int i = 0; i < animatedObjects.size(); ++i)
    {
        if (animatedObjects[i]->getName().compare(name) == 0)
        {
            tempAnimatedObject = animatedObjects[i];
            break;
        }
    }
    sem_post(&animatedObjectsLockSemaphore);

    return tempAnimatedObject;
}

unsigned int GameMap::numAnimatedObjects()
{
    sem_wait(&animatedObjectsLockSemaphore);
    unsigned int tempUnsigned = animatedObjects.size();
    sem_post(&animatedObjectsLockSemaphore);

    return tempUnsigned;
}

//void GameMap::clearActiveObjects()

void GameMap::addActiveObject(ActiveObject *a)
{
    sem_wait(&activeObjectsLockSemaphore);
    activeObjects.push_back(a);
    sem_post(&activeObjectsLockSemaphore);
}

void GameMap::removeActiveObject(ActiveObject *a)
{
    sem_wait(&activeObjectsLockSemaphore);

    // Loop over the activeObjects looking for activeObject a
    for (unsigned int i = 0; i < activeObjects.size(); ++i)
    {
        if (a == activeObjects[i])
        {
            // ActiveObject found
            activeObjects.erase(activeObjects.begin() + i);
            break;
        }
    }

    sem_post(&activeObjectsLockSemaphore);
}

//ActiveObject* GameMap::getActiveObject(int index)

//unsigned int GameMap::numActiveObjects()

/*! \brief Returns the total number of class descriptions stored in this game map.
 *
 */
unsigned int GameMap::numClassDescriptions()
{
    return classDescriptions.size();
}

/*! \brief Gets the i'th creature in this GameMap.
 *
 */
Creature* GameMap::getCreature(int index)
{
    sem_wait(&creaturesLockSemaphore);
    Creature *tempCreature = creatures[index];
    sem_post(&creaturesLockSemaphore);

    return tempCreature;
}

/*! \brief Gets the i'th class description in this GameMap.
 *
 */
CreatureClass* GameMap::getClassDescription(int index)
{
    return classDescriptions[index];
}

/*! \brief Creates meshes for all the tiles and creatures stored in this GameMap.
 *
 */
void GameMap::createAllEntities()
{
    // Create OGRE entities for map tiles
    sem_wait(&tilesLockSemaphore);
    for(TileMap_t::iterator itr = tiles.begin(), end = tiles.end();
            itr != end; ++itr)
    {
        itr->second->createMesh();
    }
    sem_post(&tilesLockSemaphore);

    // Create OGRE entities for the creatures
    for (unsigned int i = 0, num = numCreatures(); i < num; ++i)
    {
        Creature *currentCreature = getCreature(i);
        currentCreature->createMesh();
        currentCreature->weaponL->createMesh();
        currentCreature->weaponR->createMesh();
    }

    // Create OGRE entities for the map lights.
    for (unsigned int i = 0, num = numMapLights(); i < num; ++i)
    {
        getMapLight(i)->createOgreEntity();
    }

    // Create OGRE entities for the rooms
    for (unsigned int i = 0, num = numRooms(); i < num; ++i)
    {
        getRoom(i)->createMeshes();
    }
}

void GameMap::destroyAllEntities()
{
    // Destroy OGRE entities for map tiles
    sem_wait(&tilesLockSemaphore);
    TileMap_t::iterator itr = tiles.begin();
    while (itr != tiles.end())
    {
        itr->second->destroyMesh();
        ++itr;
    }
    sem_post(&tilesLockSemaphore);

    // Destroy OGRE entities for the creatures
    for (unsigned int i = 0; i < numCreatures(); ++i)
    {
        Creature *currentCreature = getCreature(i);
        currentCreature->weaponL->destroyMesh();
        currentCreature->weaponR->destroyMesh();
        currentCreature->destroyMesh();
    }

    // Destroy OGRE entities for the map lights.
    for (unsigned int i = 0; i < numMapLights(); ++i)
    {
        MapLight *currentMapLight = getMapLight(i);
        currentMapLight->destroyOgreEntity();
    }

    // Destroy OGRE entities for the rooms
    for (unsigned int i = 0; i < numRooms(); ++i)
    {
        Room *currentRoom = getRoom(i);
        currentRoom->destroyMeshes();
    }
}

/*! \brief Returns a pointer to the creature whose name matches cName.
 *
 */
Creature* GameMap::getCreature(std::string cName)
{
    //TODO: This function should look the name up in a map of creature names onto pointers, care should also be taken to minimize calls to this function.
    Creature *returnValue = NULL;

    sem_wait(&creaturesLockSemaphore);
    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        if (creatures[i]->name.compare(cName) == 0)
        {
            returnValue = creatures[i];
            break;
        }
    }
    sem_post(&creaturesLockSemaphore);

    return returnValue;
}

/*! \brief Loops over all the creatures and calls their individual doTurn methods, also check goals and do the upkeep.
 *
 */
void GameMap::doTurn()
{
    // Local variables
    unsigned int tempUnsigned;

    // Compute the moving window average of how much extra time was left over after the previous doTurn() calls finished.
    averageAILeftoverTime = 0.0;
    for (tempUnsigned = 0; tempUnsigned < previousLeftoverTimes.size(); ++tempUnsigned)
        averageAILeftoverTime += previousLeftoverTimes[tempUnsigned];

    if (previousLeftoverTimes.size() > 0)
        averageAILeftoverTime /= (double) previousLeftoverTimes.size();

    if (loadNextLevel)
    {
        if (numCreatures() > 0)
        {
            while (numCreatures() > 0)
            {
                sem_wait(&creaturesLockSemaphore);
                Creature *tempCreature = creatures[0];
                sem_post(&creaturesLockSemaphore);

                queueCreatureForDeletion(tempCreature);
            }
        }
        else
        {
            loadNextLevel = false;
            //TODO: The return value from the level load should be checked to make sure it loaded properly.
            readGameMapFromFile(nextLevel);
            createAllEntities();
            me->seat = popEmptySeat();
        }
    }

    sem_wait(&creatureAISemaphore);

    std::cout << "\nStarting creature AI for turn " << turnNumber.get();
    unsigned int numCallsTo_path_atStart = numCallsTo_path;

    processDeletionQueues();

    //TODO: Run a stopwatch during each of these threads to see how long they take to help with the load balancing.
    pthread_t thread1, thread3;
    pthread_create(&thread1, NULL, GameMap::creatureDoTurnThread, NULL);
    //pthread_create(&thread2, NULL, GameMap::tileUpkeepThread, NULL);
    pthread_create(&thread3, NULL, GameMap::miscUpkeepThread, NULL);

    pthread_join(thread3, NULL);
    //pthread_join(thread2, NULL);
    pthread_join(thread1, NULL);

    // Remove dead creatures from the map and put them into the deletion queue.
    unsigned int count = 0;
    while (count < numCreatures())
    {
        // Check to see if the creature has died.
        sem_wait(&creaturesLockSemaphore);
        Creature *tempCreature = creatures[count];
        sem_post(&creaturesLockSemaphore);
        if (tempCreature->getHP(NULL) <= 0.0)
        {
            // Let the creature lay dead on the ground for a few turns before removing it from the GameMap.
            tempCreature->clearDestinations();
            tempCreature->setAnimationState("Die", false);
            if (tempCreature->deathCounter <= 0)
            {
                // Remove the creature from the game map and into the deletion queue, it will be deleted
                // when it is safe, i.e. all other pointers to it have been wiped from the program.
                queueCreatureForDeletion(tempCreature);
            }
            else
            {
                tempCreature->deathCounter--;
                ++count;
            }
        }
        else
        {
            // Since the creature is still alive we should add its alignment and faction to
            // its controlling seat to be used in the RoomPortal::spawnCreature routine.
            Player *tempPlayer = tempCreature->getControllingPlayer();
            if (tempPlayer != NULL)
            {
                Seat *tempSeat = tempPlayer->seat;

                tempSeat->numCreaturesControlled++;

                tempSeat->factionHumans += tempCreature->coefficientHumans;
                tempSeat->factionCorpars += tempCreature->coefficientCorpars;
                tempSeat->factionUndead += tempCreature->coefficientUndead;
                tempSeat->factionConstructs
                        += tempCreature->coefficientConstructs;
                tempSeat->factionDenizens += tempCreature->coefficientDenizens;

                tempSeat->alignmentAltruism
                        += tempCreature->coefficientAltruism;
                tempSeat->alignmentOrder += tempCreature->coefficientOrder;
                tempSeat->alignmentPeace += tempCreature->coefficientPeace;
            }

            ++count;
        }
    }

    std::cout << "\nDuring this turn there were " << numCallsTo_path
            - numCallsTo_path_atStart << " calls to GameMap::path().";

    sem_post(&creatureAISemaphore);
}

void *GameMap::miscUpkeepThread(void *p)
{
    gameMap.miscUpkeepTime = gameMap.doMiscUpkeep();
    return NULL;
}

void *GameMap::creatureDoTurnThread(void *p)
{
    gameMap.creatureTurnsTime = gameMap.doCreatureTurns();
    return NULL;
}

unsigned long int GameMap::doMiscUpkeep()
{
    Tile *tempTile;
    Seat *tempSeat;
    Ogre::Timer stopwatch;
    unsigned long int timeTaken;

    // Loop over all the filled seats in the game and check all the unfinished goals for each seat.
    // Add any seats with no remaining goals to the winningSeats vector.
    for (unsigned int i = 0; i < numFilledSeats(); ++i)
    {
        // Check the previously completed goals to make sure they are still met.
        filledSeats[i]->checkAllCompletedGoals();

        // Check the goals and move completed ones to the completedGoals list for the seat.
        //NOTE: Once seats are placed on this list, they stay there even if goals are unmet.  We may want to change this.
        if (filledSeats[i]->checkAllGoals() == 0
                && filledSeats[i]->numFailedGoals() == 0)
            addWinningSeat(filledSeats[i]);

        // Set all the alignment and faction coefficients for this seat to 0, they will be
        // filled up in the loop below which removes the dead creatures from the map.
        filledSeats[i]->numCreaturesControlled = 0;
        filledSeats[i]->factionHumans = 0.0;
        filledSeats[i]->factionCorpars = 0.0;
        filledSeats[i]->factionUndead = 0.0;
        filledSeats[i]->factionConstructs = 0.0;
        filledSeats[i]->factionDenizens = 0.0;
        filledSeats[i]->alignmentAltruism = 0.0;
        filledSeats[i]->alignmentOrder = 0.0;
        filledSeats[i]->alignmentPeace = 0.0;
    }

    // Count how many of each color kobold there are.
    std::map<int, int> koboldColorCounts;
    for (unsigned int i = 0; i < numCreatures(); ++i)
    {
        sem_wait(&creaturesLockSemaphore);
        Creature *tempCreature = creatures[i];
        sem_post(&creaturesLockSemaphore);

        if (tempCreature->isWorker())
        {
            int color = tempCreature->color;
            ++koboldColorCounts[color];
        }
    }

    // Count how many dungeon temples each color controls.
    std::vector<Room*> dungeonTemples = getRoomsByType(Room::dungeonTemple);
    std::map<int, int> dungeonTempleColorCounts;
    for(unsigned int i = 0, size = dungeonTemples.size();
            i < size; ++i)
    {
        ++dungeonTempleColorCounts[dungeonTemples[i]->color];
    }

    // Compute how many kobolds each color should have as determined by the number of dungeon temples they control.
    std::map<int, int>::iterator colorItr = dungeonTempleColorCounts.begin();
    std::map<int, int> koboldsNeededPerColor;
    while (colorItr != dungeonTempleColorCounts.end())
    {
        int color = colorItr->first;
        int numDungeonTemples = colorItr->second;
        int numKobolds = koboldColorCounts[color];
        int numKoboldsNeeded = std::max(4 * numDungeonTemples - numKobolds, 0);
        numKoboldsNeeded = std::min(numKoboldsNeeded, numDungeonTemples);
        koboldsNeededPerColor[color] = numKoboldsNeeded;

        ++colorItr;
    }

    // Loop back over all the dungeon temples and for each one decide if it should try to produce a kobold.
    for (unsigned int i = 0; i < dungeonTemples.size(); ++i)
    {
        RoomDungeonTemple *dungeonTemple =
                (RoomDungeonTemple*) dungeonTemples[i];
        int color = dungeonTemple->color;
        if (koboldsNeededPerColor[color] > 0)
        {
            koboldsNeededPerColor[color]--;
            dungeonTemple->produceKobold();
        }
    }

    // Carry out the upkeep round of all the active objects in the game.
    sem_wait(&activeObjectsLockSemaphore);
    unsigned int activeObjectCount = 0;
    while (activeObjectCount < activeObjects.size())
    {
        if (!activeObjects[activeObjectCount]->doUpkeep())
        {
            activeObjects.erase(activeObjects.begin() + activeObjectCount);
        }
        else
        {
            ++activeObjectCount;
        }
    }
    sem_wait(&newActiveObjectsLockSemaphore);
    while (!newActiveObjects.empty()) // we create new active objects queued by active objects, such as cannon balls
	{
		activeObjects.push_back(newActiveObjects.front());
		newActiveObjects.pop();
	}
    sem_post(&newActiveObjectsLockSemaphore);
    sem_post(&activeObjectsLockSemaphore);

    // Remove empty rooms from the GameMap.
    //NOTE:  The auto-increment on this loop is canceled by a decrement in the if statement, changes to the loop structure will need to keep this consistent.
    for (unsigned int i = 0; i < numRooms(); ++i)
    {
        Room *tempRoom = getRoom(i);
        //tempRoom->doUpkeep(tempRoom);

        // Check to see if the room now has 0 covered tiles, if it does we can remove it from the map.
        if (tempRoom->numCoveredTiles() == 0)
        {
            removeRoom(tempRoom);
            tempRoom->deleteYourself();
            --i; //NOTE:  This decrement is to cancel out the increment that will happen on the next loop iteration.
        }
    }

    // Carry out the upkeep round for each seat.  This means recomputing how much gold is
    // available in their treasuries, how much mana they gain/lose during this turn, etc.
    for (unsigned int i = 0; i < filledSeats.size(); ++i)
    {
        tempSeat = filledSeats[i];

        // Add the amount of mana this seat accrued this turn.
        //cout << "\nSeat " << i << " has " << tempSeat->numClaimedTiles << " claimed tiles.";
        tempSeat->manaDelta = 50 + tempSeat->getNumClaimedTiles();
        tempSeat->mana += tempSeat->manaDelta;
        if (tempSeat->mana > 250000)
            tempSeat->mana = 250000;

        // Update the count on how much gold is available in all of the treasuries claimed by the given seat.
        tempSeat->gold = getTotalGoldForColor(tempSeat->color);
    }

    // Determine the number of tiles claimed by each seat.
    // Begin by setting the number of claimed tiles for each seat to 0.
    for (unsigned int i = 0; i < filledSeats.size(); ++i)
        filledSeats[i]->setNumClaimedTiles(0);

    for (unsigned int i = 0; i < emptySeats.size(); ++i)
        emptySeats[i]->setNumClaimedTiles(0);

    // Now loop over all of the tiles, if the tile is claimed increment the given seats count.
    sem_wait(&tilesLockSemaphore);
    std::map<std::pair<int, int> , Tile*>::iterator currentTile = tiles.begin();
    while (currentTile != tiles.end())
    {
        tempTile = currentTile->second;

        // Check to see if the current tile is claimed by anyone.
        if (tempTile->getType() == Tile::claimed)
        {
            // Increment the count of the seat who owns the tile.
            tempSeat = getSeatByColor(tempTile->getColor());
            if (tempSeat != NULL)
            {
                sem_wait(&tempSeat->numClaimedTilesLockSemaphore);
                unsigned int tempUInt = tempSeat->rawGetNumClaimedTiles();
                ++tempUInt;
                tempSeat->rawSetNumClaimedTiles(tempUInt);
                sem_post(&tempSeat->numClaimedTilesLockSemaphore);

                // Add a small increment of this player's color to the tiles to allow the claimed area to grow on its own.
                std::vector<Tile*> neighbors = neighborTiles(
                        currentTile->second);
                for (unsigned int i = 0; i < neighbors.size(); ++i)
                {
                    if (neighbors[i]->getType() == Tile::dirt
                            && neighbors[i]->getFullness() < 1)// && neighbors[i]->colorDouble < 0.8)
                        neighbors[i]->claimForColor(tempSeat->color, 0.04);
                }
            }
        }

        ++currentTile;
    }
    sem_post(&tilesLockSemaphore);

    timeTaken = stopwatch.getMicroseconds();
    return timeTaken;
}

unsigned long int GameMap::doCreatureTurns()
{
    Ogre::Timer stopwatch;

    // Prepare the arrays of creature pointers and parameters for the threads.
    sem_wait(&creaturesLockSemaphore);
    unsigned int arraySize = creatures.size();
    Creature **creatureArray = new Creature*[arraySize];
    for (unsigned int i = 0; i < creatures.size() && i < arraySize; ++i)
        creatureArray[i] = creatures[i];
    sem_post(&creaturesLockSemaphore);

    //FIXME: Currently this just spawns a single thread as spawning more than one causes a segfault, probably due to a race condition.
    unsigned int numThreads = std::min(maxAIThreads, arraySize);
    CDTHTStruct *threadParams = new CDTHTStruct[numThreads];
    pthread_t *threads = new pthread_t[numThreads];
    for (unsigned int i = 0; i < numThreads; ++i)
    {
        int startCreature = i * (arraySize / numThreads);
        int endCreature;

        endCreature = (i + 1 == numThreads)
                ? arraySize - 1
                : (i + 1) * (arraySize / numThreads) - 1;

        threadParams[i].numCreatures = endCreature - startCreature + 1;
        threadParams[i].creatures = &creatureArray[startCreature];

        pthread_create(&threads[i], NULL, GameMap::creatureDoTurnHelperThread,
                &threadParams[i]);
    }

    for (unsigned int i = 0; i < numThreads; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    delete[] creatureArray;
    delete[] threadParams;
    delete[] threads;

    return stopwatch.getMicroseconds();
}

void *GameMap::creatureDoTurnHelperThread(void *p)
{
    // Call the individual creature AI for each creature in this game map.
    CDTHTStruct *params = (CDTHTStruct*) p;

    //cout << *params->creatures;
    for (int i = 0; i < params->numCreatures; ++i)
    {
        if (params->creatures[i]->getHP(NULL) > 0.0)
            params->creatures[i]->doTurn();
    }

    return NULL;
}

/*! \brief Returns whether or not a Creature with a given passability would be able to move between the two specified tiles.
 *
 */
bool GameMap::pathExists(int x1, int y1, int x2, int y2,
        Tile::TileClearType passability)
{
    return (passability == Tile::walkableTile)
            ? walkablePathExists(x1, y1, x2, y2)
            : path(x1, y1, x2, y2, passability).size() >= 2;
}

/*! \brief Calculates the walkable path between tiles (x1, y1) and (x2, y2).
 *
 * The search is carried out using the A-star search algorithm.
 * The path returned contains both the starting and ending tiles, and consists
 * entirely of tiles which satify the 'passability' criterion specified in the
 * search.  The returned tiles are also a "manhattan path" meaning that every
 * successive tile is one of the 4 nearest neighbors of the previous tile in
 * the path.  In most cases you will want to call GameMap::cutCorners on the
 * returned path to shorten the number of steps on the path, as well as the
 * actual walking distance along the path.
 */
std::list<Tile*> GameMap::path(int x1, int y1, int x2, int y2,
        Tile::TileClearType passability)
{
    ++numCallsTo_path;

    //TODO:  Make the openList a priority queue sorted by the cost to improve lookup times on retrieving the next open item.
    std::list<Tile*> returnList;
    AstarEntry *currentEntry;
    Tile *destination;
    std::list<AstarEntry*> openList;
    std::list<AstarEntry*> closedList;
    std::list<AstarEntry*>::iterator itr;

    // If the start tile was not found return an empty path
    if (getTile(x1, y1) == NULL)
        return returnList;

    // If the end tile was not found return an empty path
    destination = getTile(x2, y2);
    if (destination == NULL)
        return returnList;

    // If flood filling is enabled, we can possibly eliminate this path by checking to see if they two tiles are colored differently.
    if (floodFillEnabled && passability == Tile::walkableTile
            && !walkablePathExists(x1, y1, x2, y2))
        return returnList;

    //TODO:  make this a local variable, don't forget to remove the delete statement at the end of this function.
    AstarEntry *neighbor = new AstarEntry;

    currentEntry = new AstarEntry;
    currentEntry->tile = getTile(x1, y1);
    currentEntry->parent = NULL;
    currentEntry->g = 0.0;
    currentEntry->setHeuristic(x1, y1, x2, y2);
    openList.push_back(currentEntry);

    bool pathFound = false;
    while (true)
    {
        // if the openList is empty we failed to find a path
        if (openList.size() <= 0)
            break;

        // Get the lowest fScore from the openList and move it to the closed list
        std::list<AstarEntry*>::iterator itr = openList.begin(), smallestAstar =
                openList.begin();
        while (itr != openList.end())
        {
            if ((*itr)->fCost() < (*smallestAstar)->fCost())
                smallestAstar = itr;
            ++itr;
        }

        currentEntry = *smallestAstar;
        openList.erase(smallestAstar);
        closedList.push_back(currentEntry);

        // We found the path, break out of the search loop
        if (currentEntry->tile == destination)
        {
            pathFound = true;
            break;
        }

        // Check the tiles surrounding the current square
        std::vector<Tile*> neighbors = neighborTiles(currentEntry->tile);
        bool processNeighbor;
        for (unsigned int i = 0; i < neighbors.size(); ++i)
        {
            neighbor->tile = neighbors[i];

            processNeighbor = true;
            if (neighbor->tile != NULL)
            {
                //TODO:  This code is duplicated in GameMap::pathIsClear, it should be moved into a function.
                // See if the neighbor tile in question is passable
                switch (passability)
                {
                    case Tile::walkableTile:
                        if (!(neighbor->tile->getTilePassability()
                                == Tile::walkableTile))
                        {
                            processNeighbor = false; // skip this tile and go on to the next neighbor tile
                        }
                        break;

                    case Tile::flyableTile:
                        if (!(neighbor->tile->getTilePassability()
                                == Tile::walkableTile
                                || neighbor->tile->getTilePassability()
                                        == Tile::flyableTile))
                        {
                            processNeighbor = false; // skip this tile and go on to the next neighbor tile
                        }
                        break;

                    case Tile::impassableTile:
                        std::cerr
                                << "\n\nERROR:  Trying to find a path through impassable tiles in GameMap::path()\n\n";
                        exit(1);
                        break;

                    default:
                        std::cerr
                                << "\n\nERROR:  Unhandled tile type in GameMap::path()\n\n";
                        exit(1);
                        break;
                }

                if (processNeighbor)
                {
                    // See if the neighbor is in the closed list
                    bool neighborFound = false;
                    std::list<AstarEntry*>::iterator itr = closedList.begin();
                    while (itr != closedList.end())
                    {
                        if (neighbor->tile == (*itr)->tile)
                        {
                            neighborFound = true;
                            break;
                        }
                        else
                        {
                            ++itr;
                        }
                    }

                    // Ignore the neighbor if it is on the closed list
                    if (!neighborFound)
                    {
                        // See if the neighbor is in the open list
                        neighborFound = false;
                        std::list<AstarEntry*>::iterator itr = openList.begin();
                        while (itr != openList.end())
                        {
                            if (neighbor->tile == (*itr)->tile)
                            {
                                neighborFound = true;
                                break;
                            }
                            else
                            {
                                ++itr;
                            }
                        }

                        // If the neighbor is not in the open list
                        if (!neighborFound)
                        {
                            // NOTE: This +1 weights all steps the same, diagonal steps
                            // should get a greater wieght iis they are included in the future
                            neighbor->g = currentEntry->g + 1;

                            // Use the manhattan distance for the heuristic
                            currentEntry->setHeuristic(x1, y1,
                                    neighbor->tile->x, neighbor->tile->y);
                            neighbor->parent = currentEntry;

                            openList.push_back(new AstarEntry(*neighbor));
                        }
                        else
                        {
                            // If this path to the given neighbor tile is a shorter path than the
                            // one already given, make this the new parent.
                            // NOTE: This +1 weights all steps the same, diagonal steps
                            // should get a greater wieght iis they are included in the future
                            if (currentEntry->g + 1 < (*itr)->g)
                            {
                                // NOTE: This +1 weights all steps the same, diagonal steps
                                // should get a greater wieght iis they are included in the future
                                (*itr)->g = currentEntry->g + 1;
                                (*itr)->parent = currentEntry;
                            }
                        }
                    }
                }
            }
        }
    }

    if (pathFound)
    {
        //Find the destination tile in the closed list
        //TODO:  Optimize this by remembering this from above so this loop does not need to be carried out.
        itr = closedList.begin();
        while (itr != closedList.end())
        {
            if ((*itr)->tile == destination)
                break;
            else
                ++itr;
        }

        // Follow the parent chain back the the starting tile
        currentEntry = (*itr);
        do
        {
            if (currentEntry->tile != NULL)
            {
                returnList.push_front(currentEntry->tile);
                currentEntry = currentEntry->parent;
            }

        } while (currentEntry != NULL);
    }

    // Clean up the memory we allocated by deleting the astarEntries in the open and closed lists
    itr = openList.begin();
    while (itr != openList.end())
    {
        delete *itr;
        ++itr;
    }

    itr = closedList.begin();
    while (itr != closedList.end())
    {
        delete *itr;
        ++itr;
    }

    delete neighbor;

    return returnList;
}

/*! \brief Returns an iterator to be used for the purposes of looping over the tiles stored in this GameMap.
 *
 */
TileMap_t::iterator GameMap::firstTile()
{
    sem_wait(&tilesLockSemaphore);
    TileMap_t::iterator tempItr = tiles.begin();
    sem_post(&tilesLockSemaphore);

    return tempItr;
}

/*! \brief Returns an iterator to be used for the purposes of looping over the tiles stored in this GameMap.
 *
 */
TileMap_t::iterator GameMap::lastTile()
{
    sem_wait(&tilesLockSemaphore);
    TileMap_t::iterator tempItr = tiles.end();
    sem_post(&tilesLockSemaphore);

    return tempItr;
}

/*! \brief Returns the (up to) 4 nearest neighbor tiles of the tile located at (x, y).
 *
 */
std::vector<Tile*> GameMap::neighborTiles(int x, int y)
{
    std::vector<Tile*> tempVector;

    Tile *tempTile = getTile(x, y);
    if (tempTile != NULL)
        tempVector = neighborTiles(tempTile);

    return tempVector;
}

std::vector<Tile*> GameMap::neighborTiles(Tile *t)
{
    return t->getAllNeighbors();
}

/*! \brief Adds a pointer to a player structure to the players stored by this GameMap.
 *
 */
bool GameMap::addPlayer(Player *p)
{
    if (emptySeats.size() > 0)
    {
        p->seat = popEmptySeat();
        players.push_back(p);
        return true;
    }
    else
    {
        return false;
    }
}

/*! \brief Returns a pointer to the i'th player structure stored by this GameMap.
 *
 */
Player* GameMap::getPlayer(int index)
{
    return players[index];
}

/*! \brief Returns a pointer to the player structure stored by this GameMap whose name matches pName.
 *
 */
Player* GameMap::getPlayer(std::string pName)
{
    for (unsigned int i = 0; i < numPlayers(); ++i)
    {
        if (players[i]->nick.compare(pName) == 0)
        {
            return players[i];
        }
    }

    return NULL;
}

/*! \brief Returns the number of player structures stored in this GameMap.
 *
 */
unsigned int GameMap::numPlayers()
{
    return players.size();
}

bool GameMap::walkablePathExists(int x1, int y1, int x2, int y2)
{
    Tile *tempTile1, *tempTile2;
    tempTile1 = getTile(x1, y1);
    if (tempTile1)
    {
        tempTile2 = getTile(x2, y2);
        if (tempTile2)
        {
            return (tempTile1->floodFillColor == tempTile2->floodFillColor);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

/*! \brief Returns a list of valid tiles along a straight line from (x1, y1) to (x2, y2), NOTE: in spite of
 * the name, you do not need to be able to see through the tiles returned by this method.
 *
 * This algorithm is from
 * http://en.wikipedia.org/w/index.php?title=Bresenham%27s_line_algorithm&oldid=295047020
 * A more detailed description of how it works can be found there.
 */
std::list<Tile*> GameMap::lineOfSight(int x0, int y0, int x1, int y1)
{
    std::list<Tile*> path;

    // Calculate the components of the 'manhattan distance'
    int Dx = x1 - x0;
    int Dy = y1 - y0;

    // Determine if the slope of the line is greater than 1
    int steep = (abs(Dy) >= abs(Dx));
    if (steep)
    {
        swap(x0, y0);
        swap(x1, y1);
        // recompute Dx, Dy after swap
        Dx = x1 - x0;
        Dy = y1 - y0;
    }

    // Determine whether the x component is increasing or decreasing
    int xstep = 1;
    if (Dx < 0)
    {
        xstep = -1;
        Dx = -Dx;
    }

    // Determine whether the y component is increasing or decreasing
    int ystep = 1;
    if (Dy < 0)
    {
        ystep = -1;
        Dy = -Dy;
    }

    // Loop over the pixels on the line and add them to the return list
    int TwoDy = 2 * Dy;
    int TwoDyTwoDx = TwoDy - 2 * Dx; // 2*Dy - 2*Dx
    int E = TwoDy - Dx; //2*Dy - Dx
    int y = y0;
    int xDraw, yDraw;
    for (int x = x0; x != x1; x += xstep)
    {
        // Treat a steep line as if it were actually its inverse
        if (steep)
        {
            xDraw = y;
            yDraw = x;
        }
        else
        {
            xDraw = x;
            yDraw = y;
        }

        // If the tile exists, add it to the path.
        Tile *currentTile = getTile(xDraw, yDraw);
        if (currentTile != NULL)
        {
            path.push_back(currentTile);
        }
        else
        {
            // This should fix a bug where creatures "cut across" null sections of the map if they can see the other side.
            path.clear();
            return path;
        }

        // If the error has accumulated to the next tile, "increment" the y coordinate
        if (E > 0)
        {
            // Also add the tile for this y-value for the next row over so that the line of sight consists of a 4-connected
            // path (i.e. you can traverse the path without ever having to move "diagonal" on the square grid).
            currentTile = getTile(xDraw + 1, y);
            if (currentTile != NULL)
            {
                path.push_back(currentTile);
            }
            else
            {
                // This should fix a bug where creatures "cut across" null sections of the map if they can see the other side.
                path.clear();
                return path;
            }

            // Now increment y to the value it will be for the next x-value.
            E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
            y = y + ystep;

        }
        else
        {
            E += TwoDy; //E += 2*Dy;
        }
    }

    return path;
}

/*! \brief Returns the tiles visible from the given start tile out to the specified sight radius.
 *
 */
std::vector<Tile*> GameMap::visibleTiles(Tile *startTile, double sightRadius)
{
    std::vector<Tile*> tempVector;

    if (!startTile->permitsVision())
        return tempVector;

    int startX = startTile->x;
    int startY = startTile->y;
    int sightRadiusSquared = sightRadius * sightRadius;
    std::list<std::pair<Tile*, double> > tileQueue;

    int tileCounter = 0;
    while (true)
    {
        int rSquared = tileCoordinateMap->getRadiusSquared(tileCounter);
        if (rSquared > sightRadiusSquared)
            break;

        std::pair<int, int> coord = tileCoordinateMap->getCoordinate(tileCounter);

        Tile *tempTile = getTile(startX + coord.first, startY + coord.second);
        double tempTheta = tileCoordinateMap->getCentralTheta(tileCounter);
        if (tempTile != NULL)
            tileQueue.push_back(std::pair<Tile*, double> (tempTile, tempTheta));

        ++tileCounter;
    }

    //TODO: Loop backwards and remove any non-see through tiles until we get to one which permits vision (this cuts down the cost of walks toward the end when an opaque block is found).

    // Now loop over the queue, determining which tiles are visible and push them onto the tempVector which will be returned as the output of the function.
    while (tileQueue.size() > 0)
    {
        // If the tile lets light though it it is visible and we can remove it from the queue and put it in the return list.
        if ((*tileQueue.begin()).first->permitsVision())
        {
            // The tile is visible.
            tempVector.push_back((*tileQueue.begin()).first);
            tileQueue.erase(tileQueue.begin());
            continue;
        }
        else
        {
            // The tile is does not allow vision to it.  Remove it from the queue and remove any tiles obscured by this one.
            // We add it to the return list as well since this tile is as far as we can see in this direction.  Calculate
            // the radial vectors to the corners of this tile.
            Tile *obstructingTile = (*tileQueue.begin()).first;
            tempVector.push_back(obstructingTile);
            tileQueue.erase(tileQueue.begin());
            RadialVector2 smallAngle, largeAngle, tempAngle;

            // Calculate the obstructing tile's angular size and the direction to it.  We want to check if other tiles
            // are within deltaTheta of the calculated direction.
            double dx = obstructingTile->x - startTile->x;
            double dy = obstructingTile->y - startTile->y;
            double rsq = dx * dx + dy * dy;
            double deltaTheta = 1.5 / sqrt(rsq);
            tempAngle.fromCartesian(dx, dy);
            smallAngle.theta = tempAngle.theta - deltaTheta;
            largeAngle.theta = tempAngle.theta + deltaTheta;

            // Now that we have identified the boundary lines of the region obscured by this tile, loop through until the end of
            // the tileQueue and remove any tiles which fall inside this obscured region since they are not visible either.
            std::list<std::pair<Tile*, double> >::iterator tileQueueIterator =
                    tileQueue.begin();
            while (tileQueueIterator != tileQueue.end())
            {
                tempAngle.theta = (*tileQueueIterator).second;

                // If the current tile is in the obscured region.
                if (tempAngle.directionIsBetween(smallAngle, largeAngle))
                {
                    // The tile is in the obscured region so remove it from the queue of possibly visible tiles.
                    tileQueueIterator = tileQueue.erase(tileQueueIterator);
                }
                else
                {
                    // The tile is not obscured by the current obscuring tile so leave it in the queue for now.
                    ++tileQueueIterator;
                }
            }
        }
    }

    //TODO:  Add the sector shaped region of the visible region

    return tempVector;
}

/*! \brief Loops over the visibleTiles and returns any creatures in those tiles whose color matches (or if invert is true, does not match) the given color parameter.
 *
 */
std::vector<AttackableObject*> GameMap::getVisibleForce(
        std::vector<Tile*> visibleTiles, int color, bool invert)
{
    //TODO:  This function also needs to list Rooms, Traps, Doors, etc (maybe add GameMap::getAttackableObjectsInCell to do this).
    std::vector<AttackableObject*> returnList;

    // Loop over the visible tiles
    for (std::vector<Tile*>::iterator itr = visibleTiles.begin(), end = visibleTiles.end();
            itr != end; ++itr)
    {
        //TODO: Implement Tile::getAttackableObject() to let you list all attackableObjects in the tile in a single list.
        // Loop over the creatures in the given tile
        for (unsigned int i = 0; i < (*itr)->numCreaturesInCell(); ++i)
        {
            Creature *tempCreature = (*itr)->getCreature(i);
            // If it is an enemy
            if (tempCreature != NULL)
            {
                // The invert flag is used to determine whether we want to return a list of those creatures
                // whose color matches the one supplied or is any color but the one supplied.
                if ((invert && tempCreature->getColor() != color) || (!invert
                        && tempCreature->getColor() == color))
                {
                    // Add the current creature
                    returnList.push_back(tempCreature);
                }
            }
        }

        // Check to see if the tile is covered by a Room, if it is then check to see if it should be added to the returnList.
        Room *tempRoom = (*itr)->getCoveringRoom();
        if (tempRoom != NULL)
        {
            // Check to see if the color is appropriate based on the condition of the invert flag.
            if ((invert && tempRoom->getColor() != color) || (!invert
                    && tempRoom->getColor() != color))
            {
                // Check to see if the given room is already in the returnList.
                bool roomFound = false;
                for (unsigned int i = 0; i < returnList.size(); ++i)
                {
                    if (returnList[i] == tempRoom)
                    {
                        roomFound = true;
                        break;
                    }
                }

                // If the room is not in the return list already then add it.
                if (!roomFound)
                    returnList.push_back(tempRoom);
            }
        }
    }

    return returnList;
}

/*! \brief Determines whether or not you can travel along a path.
 *
 */
bool GameMap::pathIsClear(std::list<Tile*> path,
        Tile::TileClearType passability)
{
    std::list<Tile*>::iterator itr;
    //std::list<Tile*>::iterator last;

    if (path.size() == 0)
        return false;

    //last = --path.end();

    // Loop over tile in the path and check to see if it is clear
    bool isClear = true;
    for (itr = path.begin(); itr != path.end() && isClear; ++itr)
    {
        //TODO:  This code is duplicated in GameMap::path, it should be moved into a function.
        // See if the path tile in question is passable
        switch (passability)
        {
            // Walking creatures can only move through walkableTile's.
            case Tile::walkableTile:
                isClear = (isClear && ((*itr)->getTilePassability()
                        == Tile::walkableTile));
                break;

                // Flying creatures can move through walkableTile's or flyableTile's.
            case Tile::flyableTile:
                isClear = (isClear && ((*itr)->getTilePassability()
                        == Tile::walkableTile || (*itr)->getTilePassability()
                        == Tile::flyableTile));
                break;

                // No creatures can walk through impassableTile's
            case Tile::impassableTile:
                isClear = false;
                break;

            default:
                std::cerr
                        << "\n\nERROR:  Unhandled tile type in GameMap::pathIsClear()\n\n";
                exit(1);
                break;
        }
    }

    return isClear;
}

/*! \brief Loops over a path an replaces 'manhattan' paths with 'as the crow flies' paths.
 *
 */
void GameMap::cutCorners(std::list<Tile*> &path,
        Tile::TileClearType passability)
{
    // Size must be >= 3 or else t3 and t4 can end up pointing at the same value
    if (path.size() <= 3)
        return;

    std::list<Tile*>::iterator t1 = path.begin();
    std::list<Tile*>::iterator t2 = t1;
    ++t2;
    std::list<Tile*>::iterator t3;
    std::list<Tile*>::iterator t4;
    std::list<Tile*>::iterator secondLast = path.end();
    --secondLast;

    // Loop t1 over all but the last tile in the path
    while (t1 != path.end())
    {
        // Loop t2 from t1 until the end of the path
        t2 = t1;
        ++t2;

        while (t2 != path.end())
        {
            // If we have a clear line of sight to t2, advance to
            // the next tile else break out of the inner loop
            std::list<Tile*> lineOfSightPath = lineOfSight((*t1)->x, (*t1)->y,
                    (*t2)->x, (*t2)->y);

            if (pathIsClear(lineOfSightPath, passability))
                ++t2;
            else
                break;
        }

        // Delete the tiles 'strictly between' t1 and t2
        t3 = t1;
        ++t3;
        if (t3 != t2)
        {
            t4 = t2;
            --t4;
            if (t3 != t4)
            {
                path.erase(t3, t4);
            }
        }

        t1 = t2;

        secondLast = path.end();
        --secondLast;
    }
}

/** \brief Calls the deleteYourself() method on each of the rooms in the game map as well as clearing the vector of stored rooms.
 *
 */
void GameMap::clearRooms()
{
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        Room *tempRoom = getRoom(i);
        removeActiveObject(tempRoom);
        tempRoom->deleteYourself();
    }

    rooms.clear();
}

/** \brief A simple mutator method to add the given Room to the GameMap.
 *
 */
void GameMap::addRoom(Room *r)
{
    rooms.push_back(r);
    addActiveObject(r);
}

void GameMap::removeRoom(Room *r)
{
    removeActiveObject(r);

    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        if (r == rooms[i])
        {
            //TODO:  Loop over the tiles and make any whose coveringRoom variable points to this room point to NULL.
            rooms.erase(rooms.begin() + i);
            break;
        }
    }
}

/** \brief A simple accessor method to return the given Room.
 *
 */
Room* GameMap::getRoom(int index)
{
    return rooms[index];
}

/** \brief A simple accessor method to return the number of Rooms stored in the GameMap.
 *
 */
unsigned int GameMap::numRooms()
{
    return rooms.size();
}

std::vector<Room*> GameMap::getRoomsByType(Room::RoomType type)
{
    std::vector<Room*> returnList;
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        if (rooms[i]->getType() == type)
            returnList.push_back(rooms[i]);
    }

    return returnList;
}

std::vector<Room*> GameMap::getRoomsByTypeAndColor(Room::RoomType type,
        int color)
{
    std::vector<Room*> returnList;
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        if (rooms[i]->getType() == type && rooms[i]->color == color)
            returnList.push_back(rooms[i]);
    }

    return returnList;
}

std::vector<Room*> GameMap::getReachableRooms(const std::vector<Room*> &vec,
        Tile *startTile, Tile::TileClearType passability)
{
    std::vector<Room*> returnVector;

    for (unsigned int i = 0; i < vec.size(); ++i)
    {
        if (pathExists(startTile->x, startTile->y,
                vec[i]->getCoveredTile(0)->x, vec[i]->getCoveredTile(0)->y,
                passability))
            returnVector.push_back(vec[i]);
    }

    return returnVector;
}

void GameMap::clearTraps()
{
    for (unsigned int i = 0; i < traps.size(); ++i)
    {
        removeActiveObject(traps[i]);
    }

    /*
     for(unsigned int i = 0; i < numTraps(); ++i)
     {
     getTrap(i)->deleteYourself();
     }
     */

    traps.clear();
}

void GameMap::addTrap(Trap *t)
{
    traps.push_back(t);
    addActiveObject(t);
}

void GameMap::removeTrap(Trap *t)
{
    removeActiveObject(t);

    for (unsigned int i = 0; i < traps.size(); ++i)
    {
        if (t == traps[i])
        {
            traps.erase(traps.begin() + i);
            break;
        }
    }
}

Trap* GameMap::getTrap(int index)
{
    return traps[index];
}

unsigned int GameMap::numTraps()
{
    return traps.size();
}

int GameMap::getTotalGoldForColor(int color)
{
    int tempInt = 0;
    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndColor(Room::treasury,
            color);
    for (unsigned int i = 0; i < treasuriesOwned.size(); ++i)
        tempInt += ((RoomTreasury*) treasuriesOwned[i])->getTotalGold();

    return tempInt;
}

int GameMap::withdrawFromTreasuries(int gold, int color)
{
    // Check to see if there is enough gold available in all of the treasuries owned by the given color.
    int totalGold = getTotalGoldForColor(color);
    if (totalGold < gold)
        return 0;

    // Loop over the treasuries withdrawing gold until the full amount has been withdrawn.
    int goldStillNeeded = gold;
    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndColor(Room::treasury,
            color);
    for (unsigned int i = 0; i < treasuriesOwned.size() && goldStillNeeded > 0; ++i)
        goldStillNeeded -= ((RoomTreasury*) treasuriesOwned[i])->withdrawGold(
                goldStillNeeded);

    return gold;
}

void GameMap::clearMapLights()
{
    for (unsigned int i = 0; i < mapLights.size(); ++i)
    {
        mapLights[i]->deleteYourself();
    }

    mapLights.clear();
}

void GameMap::clearMapLightIndicators()
{
    for (unsigned int i = 0; i < mapLights.size(); ++i)
        mapLights[i]->destroyOgreEntityVisualIndicator();
}

void GameMap::addMapLight(MapLight *m)
{
    mapLights.push_back(m);

    if (!m->isPermanent())
    {
        addActiveObject((TemporaryMapLight*) m);
    }

    /*
     // Place a message in the queue to inform the clients about the destruction of this MapLight.
     ServerNotification *serverNotification = new ServerNotification;
     serverNotification->type = ServerNotification::addMapLight;
     serverNotification->p = m;

     queueServerNotification(serverNotification);
     */
}

void GameMap::removeMapLight(MapLight *m)
{
    for (unsigned int i = 0; i < mapLights.size(); ++i)
    {
        if (mapLights[i] == m)
        {
            /*
             // Place a message in the queue to inform the clients about the destruction of this MapLight.
             ServerNotification *serverNotification = new ServerNotification;
             serverNotification->type = ServerNotification::removeMapLight;
             serverNotification->p = m;

             queueServerNotification(serverNotification);
             */

            mapLights.erase(mapLights.begin() + i);
            break;

        }
    }
}

MapLight* GameMap::getMapLight(int index)
{
    return mapLights[index];
}

MapLight* GameMap::getMapLight(std::string name)
{
    for (unsigned int i = 0; i < mapLights.size(); ++i)
    {
        if (mapLights[i]->getName() == name)
            return mapLights[i];
    }

    return NULL;
}

unsigned int GameMap::numMapLights()
{
    return mapLights.size();
}

/** \brief A simple mutator method to clear the vector of empty Seats stored in the GameMap.
 *
 */
void GameMap::clearEmptySeats()
{
    for (unsigned int i = 0; i < numEmptySeats(); ++i)
        delete emptySeats[i];

    emptySeats.clear();
}

/** \brief A simple mutator method to add another empty Seat to the GameMap.
 *
 */
void GameMap::addEmptySeat(Seat *s)
{
    emptySeats.push_back(s);

    // Add the goals for all seats to this seat.
    for (unsigned int i = 0; i < numGoalsForAllSeats(); ++i)
        s->addGoal(getGoalForAllSeats(i));
}

/** \brief A simple accessor method to return the given Seat.
 *
 */
Seat* GameMap::getEmptySeat(int index)
{
    return emptySeats[index];
}

/** \brief Removes the first empty Seat from the GameMap and returns a pointer to it, this is used when a Player "sits down" at the GameMap.
 *
 */
Seat* GameMap::popEmptySeat()
{
    Seat *s = NULL;
    if (emptySeats.size() > 0)
    {
        s = emptySeats[0];
        emptySeats.erase(emptySeats.begin());
        filledSeats.push_back(s);
    }

    return s;
}

/** \brief A simple accessor method to return the number of empty Seats on the GameMap.
 *
 */
unsigned int GameMap::numEmptySeats()
{
    return emptySeats.size();
}

void GameMap::clearFilledSeats()
{
    for (unsigned int i = 0; i < numFilledSeats(); ++i)
        delete filledSeats[i];

    filledSeats.clear();
}

void GameMap::addFilledSeat(Seat *s)
{
    filledSeats.push_back(s);

    // Add the goals for all seats to this seat.
    for (unsigned int i = 0; i < numGoalsForAllSeats(); ++i)
        s->addGoal(getGoalForAllSeats(i));
}

Seat* GameMap::getFilledSeat(int index)
{
    return filledSeats[index];
}

Seat* GameMap::popFilledSeat()
{
    Seat *s = NULL;
    if (filledSeats.size() > 0)
    {
        s = filledSeats[0];
        filledSeats.erase(filledSeats.begin());
        emptySeats.push_back(s);
    }

    return s;
}

unsigned int GameMap::numFilledSeats()
{
    return filledSeats.size();
}

Seat* GameMap::getSeatByColor(int color)
{
    for (unsigned int i = 0; i < filledSeats.size(); ++i)
    {
        if (filledSeats[i]->color == color)
            return filledSeats[i];
    }

    for (unsigned int i = 0; i < emptySeats.size(); ++i)
    {
        if (emptySeats[i]->color == color)
            return emptySeats[i];
    }

    return NULL;
}

void GameMap::addWinningSeat(Seat *s)
{
    // Make sure the seat has not already been added.
    for (unsigned int i = 0; i < winningSeats.size(); ++i)
    {
        if (winningSeats[i] == s)
            return;
    }

    winningSeats.push_back(s);
}

Seat* GameMap::getWinningSeat(unsigned int index)
{
    return winningSeats[index];
}

unsigned int GameMap::getNumWinningSeats()
{
    return winningSeats.size();
}

bool GameMap::seatIsAWinner(Seat *s)
{
    bool isAWinner = false;
    for (unsigned int i = 0; i < getNumWinningSeats(); ++i)
    {
        if (getWinningSeat(i) == s)
        {
            isAWinner = true;
            break;
        }
    }

    return isAWinner;
}

void GameMap::addGoalForAllSeats(Goal *g)
{
    goalsForAllSeats.push_back(g);

    // Add the goal to each of the empty seats currently in the game.
    for (unsigned int i = 0, num = numEmptySeats(); i < num; ++i)
        emptySeats[i]->addGoal(g);

    // Add the goal to each of the filled seats currently in the game.
    for (unsigned int i = 0, num = numFilledSeats(); i < num; ++i)
        filledSeats[i]->addGoal(g);
}

Goal* GameMap::getGoalForAllSeats(unsigned int i)
{
    return goalsForAllSeats[i];
}

unsigned int GameMap::numGoalsForAllSeats()
{
    return goalsForAllSeats.size();
}

void GameMap::clearGoalsForAllSeats()
{
    goalsForAllSeats.clear();

    // Add the goal to each of the empty seats currently in the game.
    for (unsigned int i = 0; i < numEmptySeats(); ++i)
    {
        emptySeats[i]->clearGoals();
        emptySeats[i]->clearCompletedGoals();
    }

    // Add the goal to each of the filled seats currently in the game.
    for (unsigned int i = 0; i < numFilledSeats(); ++i)
    {
        filledSeats[i]->clearGoals();
        filledSeats[i]->clearCompletedGoals();
    }
}

void GameMap::clearMissileObjects()
{
    for (unsigned int i = 0; i < missileObjects.size(); ++i)
    {
        removeActiveObject(missileObjects[i]);

        for (unsigned int j = 0; j < animatedObjects.size(); ++j)
        {
            if (missileObjects[i] == animatedObjects[j])
            {
                animatedObjects.erase(animatedObjects.begin() + j);
                break;
            }
        }
    }

    missileObjects.clear();
}

void GameMap::addMissileObject(MissileObject *m)
{
    missileObjects.push_back(m);
    sem_wait(&newActiveObjectsLockSemaphore);
    newActiveObjects.push(m);
    sem_post(&newActiveObjectsLockSemaphore);
    sem_wait(&animatedObjectsLockSemaphore);
    animatedObjects.push_back(m);
    sem_post(&animatedObjectsLockSemaphore);
}

void GameMap::removeMissileObject(MissileObject *m)
{
    removeActiveObject(m);

    for (unsigned int i = 0; i < missileObjects.size(); ++i)
    {
        if (m == missileObjects[i])
        {
            //TODO:  Loop over the tiles and make any whose coveringRoom variable points to this room point to NULL.
            missileObjects.erase(missileObjects.begin() + i);
            break;
        }
    }

    removeAnimatedObject(m);
}

MissileObject* GameMap::getMissileObject(int index)
{
    return missileObjects[index];
}

unsigned int GameMap::numMissileObjects()
{
    return missileObjects.size();
}

/** \brief Returns the as the crow flies distance between tiles located at the two coordinates
 * given.  If tiles do not exist at these locations the function returns -1.0.
 */
double GameMap::crowDistance(Tile *t1, Tile *t2)
{
    if (t1 != NULL && t2 != NULL)
        return crowDistance(t1->x, t2->x, t1->y, t2->y);
    else
        return -1.0;
}

double GameMap::crowDistance(int x1, int x2, int y1, int y2)
{
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

/** \brief Returns an auto-incremented number for use in the flood fill algorithm used to determine walkability.
 *
 */
int GameMap::uniqueFloodFillColor()
{
    return ++nextUniqueFloodFillColor;
}

/** \brief Starts at the tile at the given coordinates and paints outward over all the tiles whose passability matches the passability of the seed tile.
 *
 */
unsigned int GameMap::doFloodFill(int startX, int startY,
        Tile::TileClearType passability, int color)
{
    unsigned int tilesFlooded = 1;

    if (!floodFillEnabled)
        return 0;

    if (color < 0)
        color = uniqueFloodFillColor();

    // Check to see if we should color the current tile.
    Tile *tempTile = gameMap.getTile(startX, startY);
    if (tempTile != NULL)
    {
        // If the tile is walkable, color it.
        //FIXME:  This should be improved to use the "passability" parameter.
        if (tempTile->getTilePassability() == Tile::walkableTile)
            tempTile->floodFillColor = color;
        else
            return 0;
    }

    // Get the current tile's neighbors, loop over each of them.
    std::vector<Tile*> neighbors = gameMap.neighborTiles(startX, startY);
    for (unsigned int i = 0; i < neighbors.size(); ++i)
    {
        if (neighbors[i]->floodFillColor != color)
        {
            tilesFlooded += doFloodFill(neighbors[i]->x, neighbors[i]->y,
                    passability, color);
        }
    }

    return tilesFlooded;
}

/** \brief Temporarily disables the flood fill computations on this game map.
 *
 */
void GameMap::disableFloodFill()
{
    floodFillEnabled = false;
}

/** \brief Re-enables the flood filling on the game map, also recomputes the painting on the
 * whole map since the passabilities may have changed since the flood filling was disabled.
 */
void GameMap::enableFloodFill()
{
    Tile *tempTile;

    // Carry out a flood fill of the whole level to make sure everything is good.
    // Start by setting the flood fill color for every tile on the map to -1.
    sem_wait(&tilesLockSemaphore);
    for(std::map<std::pair<int, int>, Tile*>::iterator currentTile = tiles.begin(),
            end = tiles.end(); currentTile != end; ++currentTile)
    {
        tempTile = currentTile->second;
        tempTile->floodFillColor = -1;
    }
    sem_post(&tilesLockSemaphore);

    // Loop over the tiles again, this time flood filling when the flood fill color is -1.  This will flood the map enough times to cover the whole map.

    //TODO:  The looping construct here has a potential race condition in that the endTile could change between the time when it is initialized and the end of this loop.  If this happens the loop could continue infinitely.
    floodFillEnabled = true;
    sem_wait(&tilesLockSemaphore);
    std::map<std::pair<int, int> , Tile*>::iterator endTile = tiles.end();
    sem_post(&tilesLockSemaphore);
    for(std::map<std::pair<int, int>, Tile*>::iterator currentTile = tiles.begin();
                currentTile != endTile; ++currentTile)
    {
        tempTile = currentTile->second;
        if (tempTile->floodFillColor == -1)
            doFloodFill(tempTile->x, tempTile->y);
    }
}

/** \brief <i>Convenience function, calls: path(int, int, int, int, TileClearType)</i>
 *
 */
std::list<Tile*> GameMap::path(Creature *c1, Creature *c2,
        Tile::TileClearType passability)
{
    return path(c1->positionTile()->x, c1->positionTile()->y,
            c2->positionTile()->x, c2->positionTile()->y, passability);
}

/** \brief <i>Convenience function, calls: path(int, int, int, int, TileClearType)</i>
 *
 */
std::list<Tile*> GameMap::path(Tile *t1, Tile *t2,
        Tile::TileClearType passability)
{
    return path(t1->x, t1->y, t2->x, t2->y, passability);
}

/** \brief <i>Convenience function, calls: crowDistance(Tile*, Tile*)</i>
 *
 */
double GameMap::crowDistance(Creature *c1, Creature *c2)
{
    //TODO:  This is sub-optimal, improve it.
    Tile *tempTile1 = c1->positionTile(), *tempTile2 = c2->positionTile();
    return crowDistance(tempTile1->x, tempTile1->y, tempTile2->x, tempTile2->y);
}

/** \brief Increments a semaphore for the given turn indicating how many outstanding references to game asssets have been copied by other functions.
 *
 */
void GameMap::threadLockForTurn(long int turn)
{
    // Lock the thread reference count map to prevent race conditions.
    sem_wait(&threadReferenceCountLockSemaphore);

    std::map<long int, ProtectedObject<unsigned int> >::iterator result =
            threadReferenceCount.find(turn);
    if (result != threadReferenceCount.end())
    {
        (*result).second.lock();
        (*result).second.rawSet((*result).second.rawGet() + 1);
        (*result).second.unlock();
    }
    else
    {
        threadReferenceCount[turn].rawSet(1);
    }

    // Unlock the thread reference count map.
    sem_post(&threadReferenceCountLockSemaphore);
}

/** \brief Decrements a semaphore for the given turn indicating how many outstanding references to game asssets there are,
 * when this reaches 0 the turn can be safely retired and assets queued for deletion then can be safely deleted.
 *
 */
void GameMap::threadUnlockForTurn(long int turn)
{
    // Lock the thread reference count map to prevent race conditions.
    sem_wait(&threadReferenceCountLockSemaphore);

    std::map<long int, ProtectedObject<unsigned int> >::iterator result =
            threadReferenceCount.find(turn);
    if (result != threadReferenceCount.end())
    {
        (*result).second.lock();
        (*result).second.rawSet((*result).second.rawGet() - 1);
        (*result).second.unlock();
    }
    else
    {
        std::cout
                << "\n\n\nERROR:  Calling threadUnlockForTurn on a turn number which does not have any current locks, bailing out.\n\n\n";
        exit(1);
    }

    // Unlock the thread reference count map.
    sem_post(&threadReferenceCountLockSemaphore);
}

void GameMap::processDeletionQueues()
{
    long int turn = turnNumber.get();

    std::cout << "\nProcessing deletion queues on turn " << turn << ":  ";
    long int latestTurnToBeRetired = -1;

    // Lock the thread reference count map to prevent race conditions.
    sem_wait(&threadReferenceCountLockSemaphore);

    // Loop over the thread reference count and find the first turn number which has 0 outstanding threads holding references for that turn.
    std::map<long int, ProtectedObject<unsigned int> >::iterator
            currentThreadReferenceCount = threadReferenceCount.begin();
    while (currentThreadReferenceCount != threadReferenceCount.end())
    {
        std::cout << "(" << (*currentThreadReferenceCount).first << ", "
                << (*currentThreadReferenceCount).second.rawGet() << ")   ";
        if ((*currentThreadReferenceCount).second.get() == 0)
        {
            // There are no threads which could be holding references to objects from the current turn so it is safe to retire.
            latestTurnToBeRetired = (*currentThreadReferenceCount).first;
            std::map<long int, ProtectedObject<unsigned int> >::iterator
                    tempIterator = currentThreadReferenceCount++;
            threadReferenceCount.erase(tempIterator);
        }
        else
        {
            // There is one or more threads which could still be holding references to objects from the current turn so we cannot retire it.
            break;
        }
    }

    // Unlock the thread reference count map.
    sem_post(&threadReferenceCountLockSemaphore);

    // If we did not find any turns which have no threads locking them we are safe to retire this turn.
    if (latestTurnToBeRetired < 0)
        return;

    // Loop over the creaturesToDeleteMap and delete all the creatures in any mapped vector whose
    // key value (the turn those creatures were added) is less than the latestTurnToBeRetired.
    std::map<long int, std::vector<Creature*> >::iterator
            currentTurnForCreatureRetirement = creaturesToDelete.begin();
    while (currentTurnForCreatureRetirement != creaturesToDelete.end()
            && (*currentTurnForCreatureRetirement).first
                    <= latestTurnToBeRetired)
    {
        long int currentTurnToRetire =
                (*currentTurnForCreatureRetirement).first;

        // Check to see if any creatures can be deleted.
        while (creaturesToDelete[currentTurnToRetire].size() > 0)
        {
            std::cout << "\nSending message to delete creature "
                    << (*creaturesToDelete[currentTurnToRetire].begin())->name;
            std::cout.flush();

            (*creaturesToDelete[currentTurnToRetire].begin())->deleteYourself();
            creaturesToDelete[currentTurnToRetire].erase(
                    creaturesToDelete[currentTurnToRetire].begin());
        }

        ++currentTurnForCreatureRetirement;
    }
}

