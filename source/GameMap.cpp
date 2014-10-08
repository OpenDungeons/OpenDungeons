/*!
 * \file   GameMap.cpp
 * \brief  The central object holding everything that is on the map
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef _MSC_VER
#define snprintf_is_banned_in_OD_code _snprintf
#endif

#include "GameMap.h"

#include "ODServer.h"
#include "ODFrameListener.h"
#include "ServerNotification.h"
#include "RadialVector2.h"
#include "Tile.h"
#include "Creature.h"
#include "Player.h"
#include "ResourceManager.h"
#include "Trap.h"
#include "Seat.h"
#include "MapLight.h"
#include "TileCoordinateMap.h"
#include "MissileObject.h"
#include "Weapon.h"
#include "MapLoader.h"
#include "LogManager.h"
#include "MortuaryQuad.h"
#include "CullingManager.h"
#include "RoomDungeonTemple.h"
#include "RoomTreasury.h"
#include "RoomObject.h"
#include "Goal.h"

#include <OgreTimer.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include <cmath>
#include <cstdlib>

//! \brief The number of seconds the local player must stay out of danger to trigger the calm music again.
const float BATTLE_TIME_COUNT = 10.0f;

using namespace std;

/*! \brief A helper class for the A* search in the GameMap::path function.
*
* This class stores the requisite information about a tile which is placed in
* the search queue for the A-star, or A*, algorithm which is used to
* calculate paths in the path function.
*
* The A* description can be found here:
* http://en.wikipedia.org/wiki/A*_search_algorithm
*/
class AstarEntry
{
public:
    AstarEntry() :
        tile    (NULL),
        parent  (0),
        g       (0),
        h       (0)
    {}

    AstarEntry(Tile* tile, int x1, int y1, int x2, int y2) :
        tile    (tile),
        parent  (0),
        g       (0),
        h       (0)
    {
        h = computeHeuristic(x1, y1, x2, y2);
    }

    static double computeHeuristic(const int& x1, const int& y1, const int& x2, const int& y2)
    {
        return fabs(static_cast<double>(x2 - x1)) + fabs(static_cast<double>(y2 - y1));
    }

    void setHeuristic(const int& x1, const int& y1, const int& x2, const int& y2)
    {
        h = fabs(static_cast<double>(x2 - x1)) + fabs(static_cast<double>(y2 - y1));
    }

    inline double fCost() const
    { return g + h; }

    inline Tile* getTile() const
    { return tile; }

    inline void setTile(Tile* newTile)
    { tile = newTile; }

    inline AstarEntry* getParent() const
    { return parent; }

    inline void setParent(AstarEntry* newParent)
    { parent = newParent; }

    inline const double& getG() const
    { return g; }

    inline void setG(const double& newG)
    { g = newG; }

private:
    Tile*       tile;
    AstarEntry* parent;
    double      g;
    double      h;
};

GameMap::GameMap(bool isServerGameMap) :
        culm(NULL),
        mIsServerGameMap(isServerGameMap),
        mLocalPlayer(NULL),
        mTurnNumber(-1),
        mIsPaused(false),
        creatureDefinitionFilename("levels/creatures.def"), // default name
        floodFillEnabled(false),
        numCallsTo_path(0),
        tileCoordinateMap(new TileCoordinateMap(100)),
        aiManager(*this)
{
    // Init the player
    mLocalPlayer = new Player();
    mLocalPlayer->setNick("defaultNickName");
    mLocalPlayer->setGameMap(this);
    resetUniqueNumbers();
}

GameMap::~GameMap()
{
    clearAll();
    delete tileCoordinateMap;
    delete mLocalPlayer;
}

bool GameMap::loadLevel(const std::string& levelFilepath)
{
    // Read in the game map filepath
    std::string levelPath = ResourceManager::getSingletonPtr()->getResourcePath()
                            + levelFilepath;

    // TODO The map loader class should be merged back to GameMap.
    if (MapLoader::readGameMapFromFile(levelPath, *this))
        setLevelFileName(levelFilepath);
    else
        return false;

    return true;
}

bool GameMap::createNewMap(int sizeX, int sizeY)
{
    stringstream ss;

    if (!allocateMapMemory(sizeX, sizeY))
        return false;

    for (int jj = 0; jj < mMapSizeY; ++jj)
    {
        for (int ii = 0; ii < mMapSizeX; ++ii)
        {
            Tile* tile = new Tile(this, ii, jj);
            tile->setName(Tile::buildName(ii, jj));
            tile->setFullness(tile->getFullness());
            tile->setType(Tile::dirt);
            addTile(tile);
        }
    }

    mTurnNumber = -1;

    return true;
}

void GameMap::setAllFullnessAndNeighbors()
{
    for (int ii = 0; ii < mMapSizeX; ++ii)
    {
        for (int jj = 0; jj < mMapSizeY; ++jj)
        {
            Tile* tile = getTile(ii, jj);
            tile->setFullness(tile->getFullness());
            setTileNeighbors(tile);
        }
    }
}

void GameMap::clearAll()
{
    clearCreatures();
    clearClasses();
    clearTraps();
    clearMissileObjects();

    clearMapLights();
    clearRooms();
    // NOTE : clearRoomObjects should be called after clearRooms because clearRooms will try to remove the objects from the room
    clearRoomObjects();
    clearTiles();

    clearActiveObjects();

    clearGoalsForAllSeats();
    clearEmptySeats();
    getLocalPlayer()->setSeat(NULL);
    clearPlayers();
    clearFilledSeats();

    clearAiManager();

    mTurnNumber = -1;
    resetUniqueNumbers();
}

void GameMap::clearCreatures()
{
    for (unsigned int ii = 0; ii < creatures.size(); ++ii)
    {
        removeAnimatedObject(creatures[ii]);
        creatures[ii]->deleteYourself();
    }

    creatures.clear();
}

void GameMap::clearAiManager()
{
   aiManager.clearAIList();
}

void GameMap::clearClasses()
{
    classDescriptions.clear();
}

void GameMap::clearRoomObjects()
{
    for (std::vector<RoomObject*>::iterator it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
    {
        RoomObject* obj = *it;
        removeActiveObject(obj);
        removeAnimatedObject(obj);
        obj->deleteYourself();
    }

    mRoomObjects.clear();
}

void GameMap::clearActiveObjects()
{
    mActiveObjects.clear();
    mActiveObjectsToAdd.clear();
    mActiveObjectsToRemove.clear();
}

void GameMap::clearPlayers()
{

    for (unsigned int ii = 0; ii < numPlayers(); ++ii)
    {
        delete players[ii];
    }
    getLocalPlayer()->clearCreatureInHand();

    players.clear();
}

void GameMap::resetUniqueNumbers()
{
    mUniqueNumberCreature = 0;
    mUniqueNumberFloodFilling = 0;
    mUniqueNumberMissileObj = 0;
    mUniqueNumberRoom = 0;
    mUniqueNumberRoomObj = 0;
    mUniqueNumberTrap = 0;
    mUniqueNumberMapLight = 0;
}

void GameMap::addClassDescription(CreatureDefinition *c)
{
    boost::shared_ptr<CreatureDefinition> ptr(c);
    classDescriptions.push_back(ptr);
}

void GameMap::addCreature(Creature *cc)
{
    creatures.push_back(cc);

    cc->getPositionTile()->addCreature(cc);
    if(!mIsServerGameMap)
        culm->mMyCullingQuad.insert(cc);

    addAnimatedObject(cc);
    addActiveObject(cc);
    cc->setIsOnMap(true);
}

void GameMap::removeCreature(Creature *c)
{
    // Loop over the creatures looking for creature c
    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        if (c == creatures[i])
        {
            // Creature found
            // Remove the creature from the tile it's in
            c->getPositionTile()->removeCreature(c);
            creatures.erase(creatures.begin() + i);
            break;
        }
    }

    removeAnimatedObject(c);
    removeActiveObject(c);
    c->setIsOnMap(false);
}

void GameMap::queueEntityForDeletion(GameEntity *ge)
{
    entitiesToDelete.push_back(ge);
}

void GameMap::queueMapLightForDeletion(MapLight *ml)
{
    mapLightsToDelete.push_back(ml);
}

CreatureDefinition* GameMap::getClassDescription(const std::string& className)
{
    for (unsigned int i = 0; i < classDescriptions.size(); ++i)
    {
        if (classDescriptions[i]->getClassName().compare(className) == 0)
            return classDescriptions[i].get();
    }

    return NULL;
}

unsigned int GameMap::numCreatures() const
{
    return creatures.size();
}

std::vector<Creature*> GameMap::getCreaturesBySeat(Seat* seat)
{
    std::vector<Creature*> tempVector;

    // Loop over all the creatures in the GameMap and add them to the temp vector if their seat matches the one in parameter.
    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        Creature* creature = creatures[i];
        if (creature->getSeat() == seat && creature->getHP() > 0.0)
            tempVector.push_back(creatures[i]);
    }

    return tempVector;
}

void GameMap::clearAnimatedObjects()
{
    animatedObjects.clear();
}

void GameMap::addAnimatedObject(MovableGameEntity *a)
{
    animatedObjects.push_back(a);
}

void GameMap::removeAnimatedObject(MovableGameEntity *a)
{
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
}

MovableGameEntity* GameMap::getAnimatedObject(int index)
{
    MovableGameEntity* tempAnimatedObject = animatedObjects[index];

    return tempAnimatedObject;
}

MovableGameEntity* GameMap::getAnimatedObject(const std::string& name)
{
    for (unsigned int i = 0; i < animatedObjects.size(); ++i)
    {
        MovableGameEntity* mge = animatedObjects[i];
        if (mge->getName().compare(name) == 0)
        {
            return mge;
        }
    }

    return NULL;
}

void GameMap::addRoomObject(RoomObject *obj)
{
    if(isServerGameMap())
    {
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::addRoomObject, NULL);
            serverNotification->mPacket << obj;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            OD_ASSERT_TRUE(false);
            exit(1);
        }
    }
    mRoomObjects.push_back(obj);
    addActiveObject(obj);
    addAnimatedObject(obj);
}

void GameMap::removeRoomObject(RoomObject *obj)
{
    std::vector<RoomObject*>::iterator it = std::find(mRoomObjects.begin(), mRoomObjects.end(), obj);
    OD_ASSERT_TRUE_MSG(it != mRoomObjects.end(), "obj name=" + obj->getName());
    if(it == mRoomObjects.end())
        return;

    mRoomObjects.erase(it);

    if(isServerGameMap())
    {
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::removeRoomObject, NULL);
            const std::string& name = obj->getName();
            serverNotification->mPacket << name;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in Room::removeRoomObject", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
    removeAnimatedObject(obj);
    removeActiveObject(obj);
}

RoomObject* GameMap::getRoomObject(const std::string& name)
{
    for(std::vector<RoomObject*>::iterator it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
    {
        RoomObject* obj = *it;
        if(name.compare(obj->getName()) == 0)
            return obj;
    }
    return NULL;
}


unsigned int GameMap::numAnimatedObjects()
{
    return animatedObjects.size();
}

void GameMap::addActiveObject(GameEntity *a)
{
    // Active objects are only used on server side
    if(!isServerGameMap())
        return;

    mActiveObjectsToAdd.push_back(a);
}

void GameMap::removeActiveObject(GameEntity *a)
{
    // Active objects are only used on server side
    if(!isServerGameMap())
        return;

    // Loop over the activeObjects looking for activeObject a
    for (std::vector<GameEntity*>::iterator it = mActiveObjects.begin(); it != mActiveObjects.end(); ++it)
    {
        GameEntity* ge = *it;
        if (a == ge)
        {
            // ActiveObject found
            mActiveObjectsToRemove.push_back(a);
            return;
        }
    }

    for (std::deque<GameEntity*>::iterator it = mActiveObjectsToAdd.begin(); it != mActiveObjectsToAdd.end(); ++it)
    {
        GameEntity* ge = *it;
        if (a == ge)
        {
            // ActiveObject found
            mActiveObjectsToRemove.push_back(a);
            return;
        }
    }
}

unsigned int GameMap::numClassDescriptions()
{
    return classDescriptions.size();
}

Creature* GameMap::getCreature(int index)
{
    Creature *tempCreature = creatures[index];
    return tempCreature;
}

const Creature* GameMap::getCreature(int index) const
{
    const Creature *tempCreature = creatures[index];
    return tempCreature;
}

CreatureDefinition* GameMap::getClassDescription(int index)
{
    return classDescriptions[index].get();
}

void GameMap::createAllEntities()
{
    // Create OGRE entities for map tiles
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            getTile(ii,jj)->createMesh();
        }
    }

    // Create OGRE entities for the creatures
    for (unsigned int i = 0, num = numCreatures(); i < num; ++i)
    {
        Creature *currentCreature = getCreature(i);
        currentCreature->createMesh();
        currentCreature->getWeaponL()->createMesh();
        currentCreature->getWeaponR()->createMesh();
    }

    // Create OGRE entities for the map lights.
    for (unsigned int i = 0, num = numMapLights(); i < num; ++i)
    {
        getMapLight(i)->createOgreEntity();
    }

    // Create OGRE entities for the rooms
    for (unsigned int i = 0, num = numRooms(); i < num; ++i)
    {
        Room* room = getRoom(i);
        room->createMesh();
        room->updateActiveSpots();
    }

    // Create OGRE entities for the rooms
    for (unsigned int i = 0, num = numTraps(); i < num; ++i)
    {
        Trap* trap = getTrap(i);
        trap->createMesh();
        trap->updateActiveSpots();
    }
    LogManager::getSingleton().logMessage("entities created");
}

void GameMap::destroyAllEntities()
{
    // Destroy OGRE entities for map tiles
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            Tile* tile = getTile(ii,jj);
            tile->destroyMesh();
        }
    }

    // Destroy OGRE entities for the creatures
    for (unsigned int i = 0; i < numCreatures(); ++i)
    {
        Creature *currentCreature = getCreature(i);
        currentCreature->getWeaponL()->destroyMesh();
        currentCreature->getWeaponR()->destroyMesh();
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
        currentRoom->destroyMesh();
    }

    // Destroy OGRE entities for the traps
    for (unsigned int i = 0; i < numTraps(); ++i)
    {
        Trap* trap = getTrap(i);
        trap->destroyMesh();
    }
}

Creature* GameMap::getCreature(const std::string& cName)
{
    //TODO: This function should look the name up in a map of creature names onto pointers, care should also be taken to minimize calls to this function.
    Creature *returnValue = NULL;

    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        if (creatures[i]->getName().compare(cName) == 0)
        {
            returnValue = creatures[i];
            break;
        }
    }

    return returnValue;
}

const Creature* GameMap::getCreature(const std::string& cName) const
{
    //TODO: This function should look the name up in a map of creature names onto pointers, care should also be taken to minimize calls to this function.
    Creature *returnValue = NULL;

    for (unsigned int i = 0; i < creatures.size(); ++i)
    {
        if (creatures[i]->getName().compare(cName) == 0)
        {
            returnValue = creatures[i];
            break;
        }
    }

    return returnValue;
}

void GameMap::doTurn()
{
    std::cout << "\nComputing turn " << mTurnNumber;
    unsigned int numCallsTo_path_atStart = numCallsTo_path;

    uint32_t miscUpkeepTime = doMiscUpkeep();

    // Count how many creatures the player controls
    unsigned int cptCreature = 0;
    while (cptCreature < numCreatures())
    {
        // Check to see if the creature has died.
        Creature *tempCreature = creatures[cptCreature];
        if (tempCreature->getHP() > 0.0)
        {
            Seat *tempSeat = tempCreature->getSeat();
            if(tempSeat != NULL)
                ++(tempSeat->mNumCreaturesControlled);
        }

        ++cptCreature;
    }

    std::cout << "\nDuring this turn there were " << numCallsTo_path
              - numCallsTo_path_atStart << " calls to GameMap::path()."
              << "miscUpkeepTime=" << miscUpkeepTime << std::endl;
}

void GameMap::doPlayerAITurn(double frameTime)
{
    aiManager.doTurn(frameTime);
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

        // Set the creatures count to 0. It will be reset by the next count in doTurn()
        filledSeats[i]->mNumCreaturesControlled = 0;
    }

    // Count how many of each kobold there are per seat.
    std::map<Seat*, int> koboldCounts;
    for (unsigned int i = 0; i < numCreatures(); ++i)
    {
        Creature *tempCreature = creatures[i];

        if (tempCreature->getDefinition()->isWorker())
        {
            Seat* seat = tempCreature->getSeat();
            ++koboldCounts[seat];
        }
    }

    // Count how many dungeon temples each seat controls.
    std::vector<Room*> dungeonTemples = getRoomsByType(Room::dungeonTemple);
    std::map<Seat*, int> dungeonTempleSeatCounts;
    for (unsigned int i = 0, size = dungeonTemples.size(); i < size; ++i)
    {
        ++dungeonTempleSeatCounts[dungeonTemples[i]->getSeat()];
    }

    // Compute how many kobolds each seat should have as determined by the number of dungeon temples they control.
    std::map<Seat*, int>::iterator itr = dungeonTempleSeatCounts.begin();
    std::map<Seat*, int> koboldsNeededPerSeat;
    while (itr != dungeonTempleSeatCounts.end())
    {
        Seat* seat = itr->first;
        int numDungeonTemples = itr->second;
        int numKobolds = koboldCounts[seat];
        int numKoboldsNeeded = std::max(4 * numDungeonTemples - numKobolds, 0);
        numKoboldsNeeded = std::min(numKoboldsNeeded, numDungeonTemples);
        koboldsNeededPerSeat[seat] = numKoboldsNeeded;

        ++itr;
    }

    // Loop back over all the dungeon temples and for each one decide if it should try to produce a kobold.
    for (unsigned int i = 0; i < dungeonTemples.size(); ++i)
    {
        RoomDungeonTemple *dungeonTemple = static_cast<RoomDungeonTemple*>(dungeonTemples[i]);
        Seat* seat = dungeonTemple->getSeat();
        if (koboldsNeededPerSeat[seat] > 0)
        {
            --koboldsNeededPerSeat[seat];
            dungeonTemple->produceKobold();
        }
    }

    // Carry out the upkeep round of all the active objects in the game.
    unsigned int activeObjectCount = 0;
    unsigned int nbActiveObjectCount = mActiveObjects.size();
    while (activeObjectCount < nbActiveObjectCount)
    {
        GameEntity* ge = mActiveObjects[activeObjectCount];
        ge->doUpkeep();

        ++activeObjectCount;
    }

    // We add the queued active objects
    while (!mActiveObjectsToAdd.empty())
    {
        GameEntity* ge = mActiveObjectsToAdd.front();
        mActiveObjectsToAdd.pop_front();
        mActiveObjects.push_back(ge);
    }

    // We remove the queued active objects
    while (!mActiveObjectsToRemove.empty())
    {
        GameEntity* ge = mActiveObjectsToRemove.front();
        mActiveObjectsToRemove.pop_front();
        std::vector<GameEntity*>::iterator it = std::find(mActiveObjects.begin(), mActiveObjects.end(), ge);
        OD_ASSERT_TRUE_MSG(it != mActiveObjects.end(), "name=" + ge->getName());
        if(it != mActiveObjects.end())
            mActiveObjects.erase(it);
    }

    // Carry out the upkeep round for each seat.  This means recomputing how much gold is
    // available in their treasuries, how much mana they gain/lose during this turn, etc.
    for (unsigned int i = 0; i < filledSeats.size(); ++i)
    {
        tempSeat = filledSeats[i];

        // Add the amount of mana this seat accrued this turn.
        //cout << "\nSeat " << i << " has " << tempSeat->numClaimedTiles << " claimed tiles.";
        tempSeat->mManaDelta = 50 + tempSeat->getNumClaimedTiles();
        tempSeat->mMana += tempSeat->mManaDelta;
        if (tempSeat->mMana > 250000)
            tempSeat->mMana = 250000;

        // Update the count on how much gold is available in all of the treasuries claimed by the given seat.
        tempSeat->mGold = getTotalGoldForSeat(tempSeat);
    }

    // Determine the number of tiles claimed by each seat.
    // Begin by setting the number of claimed tiles for each seat to 0.
    for (unsigned int i = 0; i < filledSeats.size(); ++i)
        filledSeats[i]->setNumClaimedTiles(0);

    for (unsigned int i = 0; i < emptySeats.size(); ++i)
        emptySeats[i]->setNumClaimedTiles(0);

    // Now loop over all of the tiles, if the tile is claimed increment the given seats count.
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            tempTile = getTile(ii,jj);

            // Check to see if the current tile is claimed by anyone.
            if (tempTile->getType() == Tile::claimed)
            {
                // Increment the count of the seat who owns the tile.
                tempSeat = tempTile->getSeat();
                if (tempSeat != NULL)
                {
                    tempSeat->incrementNumClaimedTiles();
                }
            }


        }
    }

    timeTaken = stopwatch.getMicroseconds();
    return timeTaken;
}

void GameMap::updateAnimations(Ogre::Real timeSinceLastFrame)
{
    // During the first turn, we setup everything
    if(!isServerGameMap() && getTurnNumber() == 0)
    {
        LogManager::getSingleton().logMessage("Starting game map");
        setGamePaused(false);

        // Create ogre entities for the tiles, rooms, and creatures
        createAllEntities();
    }

    if(mIsPaused)
        return;

    // Update the animations on any AnimatedObjects which have them
    unsigned int entities_number = numAnimatedObjects();
    for (unsigned int i = 0; i < entities_number; ++i)
    {
        MovableGameEntity* currentAnimatedObject = getAnimatedObject(i);

        if (!currentAnimatedObject)
            continue;

        currentAnimatedObject->update(timeSinceLastFrame);
    }

    if(isServerGameMap())
    {
        updatePlayerFightingTime(timeSinceLastFrame);
        return;
    }

    // Advance the "flickering" of the lights by the amount of time that has passed since the last frame.
    entities_number = numMapLights();
    for (unsigned int i = 0; i < entities_number; ++i)
    {
        MapLight* tempMapLight = getMapLight(i);

        if (!tempMapLight)
            continue;

        tempMapLight->advanceFlicker(timeSinceLastFrame);
    }
}

void GameMap::updatePlayerFightingTime(Ogre::Real timeSinceLastFrame)
{
    // Updates fighting time for server players
    for (unsigned int i = 0; i < players.size(); ++i)
    {
        Player* player = players[i];
        if (player == NULL)
            continue;

        float fightingTime = player->getFightingTime();
        if (fightingTime == 0.0f)
            continue;

        fightingTime -= timeSinceLastFrame;
        // We can trigger the calm music again
        if (fightingTime <= 0.0f)
        {
            fightingTime = 0.0f;
            try
            {
                // Notify the player he is no longer under attack.
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::playerNoMoreFighting, player);
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                OD_ASSERT_TRUE(false);
                exit(1);
            }
        }
        player->setFightingTime(fightingTime);
    }
}

void GameMap::playerIsFighting(Player* player)
{
    if (player == NULL)
        return;

    // No need to notify AI players
    if(player->getHasAI())
        return;

    if (player->getFightingTime() == 0.0f)
    {
        try
        {
            // Notify the player he is now under attack.
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::playerFighting, player);
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            OD_ASSERT_TRUE(false);
            exit(1);
        }
    }

    player->setFightingTime(BATTLE_TIME_COUNT);
}

bool GameMap::pathExists(Tile* tileStart, Tile* tileEnd, const CreatureDefinition* creatureDef)
{
    // If floodfill is not enabled, we cannot check if the path exists so we return true
    if(!floodFillEnabled)
        return true;

    if(!tileStart->canCreatureGoThroughTile(creatureDef) || !tileEnd->canCreatureGoThroughTile(creatureDef))
        return false;

    if((creatureDef->getMoveSpeedGround() > 0.0) &&
        (creatureDef->getMoveSpeedWater() > 0.0) &&
        (creatureDef->getMoveSpeedLava() > 0.0))
    {
        return (tileStart->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] == tileEnd->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava]);
    }
    if((creatureDef->getMoveSpeedGround() > 0.0) &&
        (creatureDef->getMoveSpeedWater() > 0.0))
    {
        return (tileStart->mFloodFillColor[Tile::FloodFillTypeGroundWater] == tileEnd->mFloodFillColor[Tile::FloodFillTypeGroundWater]);
    }
    if((creatureDef->getMoveSpeedGround() > 0.0) &&
        (creatureDef->getMoveSpeedLava() > 0.0))
    {
        return (tileStart->mFloodFillColor[Tile::FloodFillTypeGroundLava] == tileEnd->mFloodFillColor[Tile::FloodFillTypeGroundLava]);
    }

    return (tileStart->mFloodFillColor[Tile::FloodFillTypeGround] == tileEnd->mFloodFillColor[Tile::FloodFillTypeGround]);
}

std::list<Tile*> GameMap::path(int x1, int y1, int x2, int y2, const CreatureDefinition* creatureDef, Seat* seat, bool throughDiggableTiles)
{
    ++numCallsTo_path;
    std::list<Tile*> returnList;

    // If the start tile was not found return an empty path
    Tile* start = getTile(x1, y1);
    if (start == NULL)
        return returnList;

    // If the end tile was not found return an empty path
    Tile* destination = getTile(x2, y2);
    if (destination == NULL)
        return returnList;

    // If flood filling is enabled, we can possibly eliminate this path by checking to see if they two tiles are floodfilled differently.
    if (!throughDiggableTiles && !pathExists(start, destination, creatureDef))
        return returnList;

    AstarEntry *currentEntry = new AstarEntry(getTile(x1, y1), x1, y1, x2, y2);
    AstarEntry neighbor;

    std::vector<AstarEntry*> openList;
    openList.push_back(currentEntry);

    std::vector<AstarEntry*> closedList;
    AstarEntry* destinationEntry = NULL;
    while (true)
    {
        // if the openList is empty we failed to find a path
        if (openList.empty())
            break;

        // Get the lowest fScore from the openList and move it to the closed list
        std::vector<AstarEntry*>::iterator smallestAstar = openList.begin();
        for (std::vector<AstarEntry*>::iterator itr = openList.begin(); itr != openList.end(); ++itr)
        {
            if ((*itr)->fCost() < (*smallestAstar)->fCost())
                smallestAstar = itr;
        }

        currentEntry = *smallestAstar;
        openList.erase(smallestAstar);
        closedList.push_back(currentEntry);

        // We found the path, break out of the search loop
        if (currentEntry->getTile() == destination)
        {
            destinationEntry = currentEntry;
            break;
        }

        // Check the tiles surrounding the current square
        bool areTilesPassable[4] = {false, false, false, false};
        // Note : to disable diagonals, process tiles from 0 to 3. To allow them, process tiles from 0 to 7
        for (unsigned int i = 0; i < 8; ++i)
        {
            Tile* neighborTile = NULL;
            switch(i)
            {
                // We process the 4 adjacent tiles
                case 0:
                    neighborTile = getTile(currentEntry->getTile()->getX() - 1, currentEntry->getTile()->getY());
                    break;
                case 1:
                    neighborTile = getTile(currentEntry->getTile()->getX() + 1, currentEntry->getTile()->getY());
                    break;
                case 2:
                    neighborTile = getTile(currentEntry->getTile()->getX(), currentEntry->getTile()->getY() - 1);
                    break;
                case 3:
                    neighborTile = getTile(currentEntry->getTile()->getX(), currentEntry->getTile()->getY() + 1);
                    break;
                // We process the 4 diagonal tiles. We only process a diagonal tile if the 2 tiles adjacent to the original one are
                // passable.
                case 4:
                    if(areTilesPassable[0] && areTilesPassable[2])
                        neighborTile = getTile(currentEntry->getTile()->getX() - 1, currentEntry->getTile()->getY() - 1);
                    break;
                case 5:
                    if(areTilesPassable[0] && areTilesPassable[3])
                        neighborTile = getTile(currentEntry->getTile()->getX() - 1, currentEntry->getTile()->getY() + 1);
                    break;
                case 6:
                    if(areTilesPassable[1] && areTilesPassable[2])
                        neighborTile = getTile(currentEntry->getTile()->getX() + 1, currentEntry->getTile()->getY() - 1);
                    break;
                case 7:
                    if(areTilesPassable[1] && areTilesPassable[3])
                        neighborTile = getTile(currentEntry->getTile()->getX() + 1, currentEntry->getTile()->getY() + 1);
                    break;
            }
            if(neighborTile == NULL)
                continue;

            neighbor.setTile(neighborTile);

            bool processNeighbor = false;
            if(neighbor.getTile()->canCreatureGoThroughTile(creatureDef))
            {
                processNeighbor = true;
                // We set passability for the 4 adjacent tiles only
                if(i < 4)
                    areTilesPassable[i] = true;
             }
            else if(throughDiggableTiles && neighbor.getTile()->isDiggable(seat))
                processNeighbor = true;

            if (!processNeighbor)
                continue;

            // See if the neighbor is in the closed list
            AstarEntry* neighborEntry = NULL;
            for(std::vector<AstarEntry*>::iterator itr = closedList.begin(); itr != closedList.end(); ++itr)
            {
                if (neighbor.getTile() == (*itr)->getTile())
                {
                    neighborEntry = *itr;
                    break;
                }
            }

            // Ignore the neighbor if it is on the closed list
            if (neighborEntry != NULL)
                continue;

            // See if the neighbor is in the open list
            for(std::vector<AstarEntry*>::iterator itr = openList.begin(); itr != openList.end(); ++itr)
            {
                if (neighbor.getTile() == (*itr)->getTile())
                {
                    neighborEntry = *itr;
                    break;
                }
            }

            // If the neighbor is not in the open list
            if (neighborEntry == NULL)
            {
                double weightToParent = AstarEntry::computeHeuristic(neighbor.getTile()->getX(), neighbor.getTile()->getY(),
                    currentEntry->getTile()->getX(), currentEntry->getTile()->getY());
                weightToParent /= currentEntry->getTile()->getCreatureSpeedOnTile(creatureDef);
                neighbor.setG(currentEntry->getG() + weightToParent);

                // Use the manhattan distance for the heuristic
                neighbor.setHeuristic(neighbor.getTile()->getX(), neighbor.getTile()->getY(), x2, y2);
                neighbor.setParent(currentEntry);

                openList.push_back(new AstarEntry(neighbor));
            }
            else
            {
                // If this path to the given neighbor tile is a shorter path than the
                // one already given, make this the new parent.
                double weightToParent = AstarEntry::computeHeuristic(neighbor.getTile()->getX(), neighbor.getTile()->getY(),
                    currentEntry->getTile()->getX(), currentEntry->getTile()->getY());
                weightToParent /= currentEntry->getTile()->getCreatureSpeedOnTile(creatureDef);

                if (currentEntry->getG() + weightToParent < neighborEntry->getG())
                {
                    neighborEntry->setG(currentEntry->getG() + weightToParent);
                    neighborEntry->setParent(currentEntry);
                }
            }
        }
    }

    if (destinationEntry != NULL)
    {
        // Follow the parent chain back the the starting tile
        AstarEntry* curEntry = destinationEntry;
        do
        {
            if (curEntry->getTile() != NULL)
            {
                returnList.push_front(curEntry->getTile());
                curEntry = curEntry->getParent();
            }

        } while (curEntry != NULL);
    }

    // Clean up the memory we allocated by deleting the astarEntries in the open and closed lists
    for (std::vector<AstarEntry*>::iterator itr = openList.begin(); itr != openList.end(); ++itr)
        delete *itr;

    for (std::vector<AstarEntry*>::iterator itr = closedList.begin(); itr != closedList.end(); ++itr)
        delete *itr;

    return returnList;
}

void GameMap::addPlayer(Player* player, Seat* seat)
{
    player->setSeat(seat);
    player->setGameMap(this);
    players.push_back(player);
    LogManager::getSingleton().logMessage("Added player: " + player->getNick());
}

bool GameMap::assignAI(Player& player, const std::string& aiType, const std::string& parameters)
{
    if (aiManager.assignAI(player, aiType, parameters))
    {
        player.setHasAI(true);
        LogManager::getSingleton().logMessage("Assign AI: " + aiType + ", to player: " + player.getNick());
        return true;
    }

    LogManager::getSingleton().logMessage("Couldn't assign AI: " + aiType + ", to player: " + player.getNick());
    return false;
}

Player* GameMap::getPlayer(unsigned int index)
{
    if (index < players.size())
        return players[index];
    return NULL;
}

const Player* GameMap::getPlayer(unsigned int index) const
{
    if (index < players.size())
        return players[index];
    return NULL;
}

Player* GameMap::getPlayer(const std::string& pName)
{
    for (unsigned int i = 0; i < numPlayers(); ++i)
    {
        if (players[i]->getNick().compare(pName) == 0)
        {
            return players[i];
        }
    }

    return NULL;
}

const Player* GameMap::getPlayer(const std::string& pName) const
{
    for (unsigned int i = 0; i < numPlayers(); ++i)
    {
        if (players[i]->getNick().compare(pName) == 0)
        {
            return players[i];
        }
    }

    return NULL;
}

unsigned int GameMap::numPlayers() const
{
    return players.size();
}

Player* GameMap::getPlayerBySeatId(int seatId)
{
    if(!mIsServerGameMap && getLocalPlayer()->getSeat()->getId() == seatId)
        return getLocalPlayer();

    for (std::vector<Player*>::iterator it = players.begin(); it != players.end(); ++it)
    {
        Player* player = *it;
        if(player->getSeat()->getId() == seatId)
            return player;
    }
    return NULL;
}

Player* GameMap::getPlayerBySeat(Seat* seat)
{
    if(!mIsServerGameMap && getLocalPlayer()->getSeat() == seat)
        return getLocalPlayer();

    for (std::vector<Player*>::iterator it = players.begin(); it != players.end(); ++it)
    {
        Player* player = *it;
        if(player->getSeat() == seat)
            return player;
    }
    return NULL;
}

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
        std::swap(x0, y0);
        std::swap(x1, y1);
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

std::vector<Tile*> GameMap::visibleTiles(Tile *startTile, double sightRadius)
{
    std::vector<Tile*> tempVector;

    if (!startTile->permitsVision())
        return tempVector;

    int startX = startTile->x;
    int startY = startTile->y;
    int sightRadiusSquared = (int)(sightRadius * sightRadius);
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
    while (!tileQueue.empty())
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
            smallAngle.setTheta(tempAngle.getTheta() - deltaTheta);
            largeAngle.setTheta(tempAngle.getTheta() + deltaTheta);

            // Now that we have identified the boundary lines of the region obscured by this tile, loop through until the end of
            // the tileQueue and remove any tiles which fall inside this obscured region since they are not visible either.
            std::list<std::pair<Tile*, double> >::iterator tileQueueIterator =
                tileQueue.begin();
            while (tileQueueIterator != tileQueue.end())
            {
                tempAngle.setTheta((*tileQueueIterator).second);

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

std::vector<GameEntity*> GameMap::getVisibleForce(std::vector<Tile*> visibleTiles, Seat* seat, bool invert)
{
    std::vector<GameEntity*> returnList;

    // Loop over the visible tiles
    for (std::vector<Tile*>::iterator itr = visibleTiles.begin(); itr != visibleTiles.end(); ++itr)
    {
        Tile* tile = *itr;
        OD_ASSERT_TRUE(tile != NULL);
        if(tile == NULL)
            continue;
        tile->fillAttackableObjects(returnList, seat, invert);
    }

    return returnList;
}

void GameMap::clearRooms()
{
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        Room *tempRoom = getRoom(i);
        removeActiveObject(tempRoom);
        tempRoom->removeAllRoomObject();
        tempRoom->deleteYourself();
    }

    rooms.clear();
}

void GameMap::addRoom(Room *r)
{
    rooms.push_back(r);
    addActiveObject(r);
}

void GameMap::removeRoom(Room *r)
{
    // Rooms are removed when absorbed by another room or when they have no more tile
    // In both cases, the client have enough information to do that alone so no need to notify him
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        if (r == rooms[i])
        {
            //TODO:  Loop over the tiles and make any whose coveringRoom variable points to this room point to NULL.
            r->removeAllRoomObject();
            rooms.erase(rooms.begin() + i);
            break;
        }
    }
    removeActiveObject(r);
}

Room* GameMap::getRoom(int index)
{
    return rooms[index];
}

unsigned int GameMap::numRooms()
{
    return rooms.size();
}

std::vector<Room*> GameMap::getRoomsByType(Room::RoomType type)
{
    std::vector<Room*> returnList;
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        Room* room = rooms[i];
        if (room->getType() == type  && room->getHP(NULL) > 0.0)
            returnList.push_back(rooms[i]);
    }

    return returnList;
}

std::vector<Room*> GameMap::getRoomsByTypeAndSeat(Room::RoomType type, Seat* seat)
{
    std::vector<Room*> returnList;
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        Room* room = rooms[i];
        if (room->getType() == type && room->getSeat() == seat && room->getHP(NULL) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

std::vector<const Room*> GameMap::getRoomsByTypeAndSeat(Room::RoomType type, Seat* seat) const
{
    std::vector<const Room*> returnList;
    for (unsigned int i = 0; i < rooms.size(); ++i)
    {
        const Room* room = rooms[i];
        if (room->getType() == type && room->getSeat() == seat && room->getHP(NULL) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

unsigned int GameMap::numRoomsByTypeAndSeat(Room::RoomType type, Seat* seat) const
{
    unsigned int count = 0;;
    std::vector<Room*>::const_iterator it;
    for (it = rooms.begin(); it != rooms.end(); ++it)
    {
        Room* room = *it;
        if (room->getType() == type && room->getSeat() == seat && room->getHP(NULL) > 0.0)
            ++count;
    }
    return count;
}

std::vector<Room*> GameMap::getReachableRooms(const std::vector<Room*>& vec,
                                              Tile* startTile,
                                              const CreatureDefinition* creatureDef)
{
    std::vector<Room*> returnVector;

    for (unsigned int i = 0; i < vec.size(); ++i)
    {
        Room* room = vec[i];
        Tile* coveredTile = room->getCoveredTile(0);
        if (pathExists(startTile, coveredTile, creatureDef))
        {
            returnVector.push_back(room);
        }
    }

    return returnVector;
}

Room* GameMap::getRoomByName(const std::string& name)
{
    for (std::vector<Room*>::const_iterator it = rooms.begin(); it != rooms.end(); ++it)
    {
        Room* room = *it;
        if(room->getName().compare(name) == 0)
            return room;
    }

    return NULL;
}

Trap* GameMap::getTrapByName(const std::string& name)
{
    for (std::vector<Trap*>::const_iterator it = traps.begin(); it != traps.end(); ++it)
    {
        Trap* trap = *it;
        if(trap->getName().compare(name) == 0)
            return trap;
    }

    return NULL;
}

void GameMap::clearTraps()
{
    for (unsigned int i = 0; i < traps.size(); ++i)
    {
        Trap* trap = traps[i];
        removeActiveObject(trap);
        trap->deleteYourself();
    }

    traps.clear();
}

void GameMap::addTrap(Trap *t)
{
    traps.push_back(t);
    addActiveObject(t);
}

void GameMap::removeTrap(Trap *t)
{
    for (std::vector<Trap*>::iterator it = traps.begin(); it != traps.end(); ++it)
    {
        Trap* trap = *it;
        if (trap == t)
        {
            traps.erase(it);
            break;
        }
    }
    removeActiveObject(t);
}

Trap* GameMap::getTrap(int index)
{
    return traps[index];
}

unsigned int GameMap::numTraps()
{
    return traps.size();
}

int GameMap::getTotalGoldForSeat(Seat* seat)
{
    int tempInt = 0;
    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndSeat(Room::treasury, seat);
    for (unsigned int i = 0; i < treasuriesOwned.size(); ++i)
    {
        tempInt += static_cast<RoomTreasury*>(treasuriesOwned[i])->getTotalGold();
    }

    return tempInt;
}

bool GameMap::withdrawFromTreasuries(int gold, Seat* seat)
{
    // Check to see if there is enough gold available in all of the treasuries owned by the given seat.
    if (seat->getGold() < gold)
        return false;

    // Loop over the treasuries withdrawing gold until the full amount has been withdrawn.
    int goldStillNeeded = gold;
    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndSeat(Room::treasury, seat);
    for (unsigned int i = 0; i < treasuriesOwned.size() && goldStillNeeded > 0; ++i)
    {
        goldStillNeeded -= static_cast<RoomTreasury*>(treasuriesOwned[i])->withdrawGold(goldStillNeeded);
    }

    return true;
}

void GameMap::clearMapLights()
{
    for (unsigned int i = 0; i < mapLights.size(); ++i)
    {
        mapLights[i]->deleteYourself();
    }

    mapLights.clear();
}

void GameMap::addMapLight(MapLight *m)
{
    mapLights.push_back(m);

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
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::removeMapLight);
            serverNotification->mPacket << m;
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

MapLight* GameMap::getMapLight(const std::string& name)
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

void GameMap::clearEmptySeats()
{
    for (unsigned int i = 0; i < numEmptySeats(); ++i)
        delete emptySeats[i];

    emptySeats.clear();
}

void GameMap::addEmptySeat(Seat *s)
{
    if (s == NULL)
        return;

    emptySeats.push_back(s);

    // Add the goals for all seats to this seat.
    for (unsigned int i = 0; i < numGoalsForAllSeats(); ++i)
        s->addGoal(getGoalForAllSeats(i));
}

Seat* GameMap::getEmptySeat(int index)
{
    return emptySeats[index];
}

const Seat* GameMap::getEmptySeat(int index) const
{
    return emptySeats[index];
}

Seat* GameMap::getEmptySeat(const std::string& faction)
{
    Seat* seat = NULL;
    for (std::vector<Seat*>::iterator it = emptySeats.begin(); it != emptySeats.end(); ++it)
    {
        if((*it)->mFaction == faction)
        {
            seat = *it;
            break;
        }
    }

    return seat;
}

Seat* GameMap::popEmptySeat(int id)
{
    Seat* seat = NULL;
    for (std::vector<Seat*>::iterator it = emptySeats.begin(); it != emptySeats.end(); ++it)
    {
        if((*it)->getId() == id)
        {
            seat = *it;
            emptySeats.erase(it);
            filledSeats.push_back(seat);
            return seat;
        }
    }

    return NULL;
}

unsigned int GameMap::numEmptySeats() const
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
    if (s == NULL)
        return;

    filledSeats.push_back(s);

    // Add the goals for all seats to this seat.
    for (unsigned int i = 0; i < numGoalsForAllSeats(); ++i)
        s->addGoal(getGoalForAllSeats(i));
}

Seat* GameMap::getFilledSeat(int index)
{
    return filledSeats[index];
}

const Seat* GameMap::getFilledSeat(int index) const
{
    return filledSeats[index];
}

unsigned int GameMap::numFilledSeats() const
{
    return filledSeats.size();
}

Seat* GameMap::getSeatById(int id)
{
    for (unsigned int i = 0; i < filledSeats.size(); ++i)
    {
        if (filledSeats[i]->getId() == id)
            return filledSeats[i];
    }

    for (unsigned int i = 0; i < emptySeats.size(); ++i)
    {
        if (emptySeats[i]->getId() == id)
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

    Player* player = getPlayerBySeat(s);
    if (player && player->getHasAI() == false)
    {
        ServerNotification* serverNotification = new ServerNotification(ServerNotification::chatServer, player);
        serverNotification->mPacket << "You have won!";
        ODServer::getSingleton().queueServerNotification(serverNotification);
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

const Goal* GameMap::getGoalForAllSeats(unsigned int i) const
{
    return goalsForAllSeats[i];
}

unsigned int GameMap::numGoalsForAllSeats() const
{
    return goalsForAllSeats.size();
}

void GameMap::clearGoalsForAllSeats()
{
    goalsForAllSeats.clear();

    // Add the goal to each of the empty seats currently in the game.
    for (unsigned int i = 0; i < numEmptySeats(); ++i)
    {
        emptySeats[i]->clearUncompleteGoals();
        emptySeats[i]->clearCompletedGoals();
    }

    // Add the goal to each of the filled seats currently in the game.
    for (unsigned int i = 0; i < numFilledSeats(); ++i)
    {
        filledSeats[i]->clearUncompleteGoals();
        filledSeats[i]->clearCompletedGoals();
    }
}

void GameMap::clearMissileObjects()
{
    for (unsigned int i = 0; i < missileObjects.size(); ++i)
    {
        MissileObject* mo = missileObjects[i];
        removeActiveObject(mo);

        for (std::vector<MovableGameEntity*>::iterator it = animatedObjects.begin(); it != animatedObjects.end(); ++it)
        {
            MovableGameEntity* ao = *it;
            if (mo == ao)
            {
                animatedObjects.erase(it);
                break;
            }
        }
        mo->deleteYourself();
    }

    missileObjects.clear();
}

void GameMap::addMissileObject(MissileObject *m)
{
    if(isServerGameMap())
    {
        LogManager::getSingleton().logMessage("Adding MissileObject " + m->getName());
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::addMissileObject, NULL);
            serverNotification->mPacket << m;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in GameMap::addMissileObject", Ogre::LML_CRITICAL);
            exit(1);
        }
    }

    missileObjects.push_back(m);
    addActiveObject(m);
    addAnimatedObject(m);
}

void GameMap::removeMissileObject(MissileObject *m)
{
    if(isServerGameMap())
    {
        LogManager::getSingleton().logMessage("Removing MissileObject " + m->getName());
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::removeMissileObject, NULL);
            std::string name = m->getName();
            serverNotification->mPacket << name;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in GameMap::removeMissileObject", Ogre::LML_CRITICAL);
            exit(1);
        }
    }

    for (unsigned int i = 0; i < missileObjects.size(); ++i)
    {
        if (m == missileObjects[i])
        {
            missileObjects.erase(missileObjects.begin() + i);
            break;
        }
    }

    removeActiveObject(m);
    removeAnimatedObject(m);
}

MissileObject* GameMap::getMissileObject(int index)
{
    return missileObjects[index];
}

MissileObject* GameMap::getMissileObject(const std::string& name)
{
    for(std::vector<MissileObject*>::iterator it = missileObjects.begin(); it != missileObjects.end(); ++it)
    {
        MissileObject* mo = *it;
        if(mo->getName().compare(name) == 0)
            return mo;
    }
    return NULL;
}

unsigned int GameMap::numMissileObjects()
{
    return missileObjects.size();
}

Ogre::Real GameMap::crowDistance(Tile *t1, Tile *t2)
{
    if (t1 != NULL && t2 != NULL)
        return crowDistance(t1->x, t2->x, t1->y, t2->y);
    else
        return -1.0f;
}

Ogre::Real GameMap::crowDistance(int x1, int x2, int y1, int y2)
{
    return sqrt(pow(static_cast<Ogre::Real>(x2 - x1), 2.0f)
                + pow(static_cast<Ogre::Real>(y2 - y1), 2.0f));
}

bool GameMap::doFloodFill(Tile* tile)
{
    if (!floodFillEnabled)
        return false;

    if(tile->isFloodFillFilled())
        return false;

    bool hasChanged = false;
    // If a neigboor is colored with the same colors, we color the tile
    std::vector<Tile*> tiles = tile->getAllNeighbors();
    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* neigh = *it;
        switch(neigh->getType())
        {
            case Tile::dirt:
            case Tile::gold:
            case Tile::claimed:
            {
                if((tile->mFloodFillColor[Tile::FloodFillTypeGround] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGround] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGround] = neigh->mFloodFillColor[Tile::FloodFillTypeGround];
                    hasChanged = true;
                }

                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundWater] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundWater] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundWater] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundWater];
                    hasChanged = true;
                }

                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundLava] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundLava] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundLava] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundLava];
                    hasChanged = true;
                }

                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava];
                    hasChanged = true;
                }

                // If the tile is fully filled, no need to continue
                if(tile->isFloodFillFilled())
                    return true;

                break;
            }
            case Tile::water:
            {
                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundWater] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundWater] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundWater] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundWater];
                    hasChanged = true;
                }

                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava];
                    hasChanged = true;
                }

                // If the tile is fully filled, no need to continue
                if(tile->isFloodFillFilled())
                    return true;

                break;
            }
            case Tile::lava:
            {
                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundLava] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundLava] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundLava] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundLava];
                    hasChanged = true;
                }

                if((tile->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] == -1) &&
                   (neigh->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] != -1))
                {
                    tile->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] = neigh->mFloodFillColor[Tile::FloodFillTypeGroundWaterLava];
                    hasChanged = true;
                }

                // If the tile is fully filled, no need to continue
                if(tile->isFloodFillFilled())
                    return true;

                break;
            }
            default:
                return false;
        }
    }

    return hasChanged;
}

void GameMap::replaceFloodFill(Tile::FloodFillType floodFillType, int colorOld, int colorNew)
{
    OD_ASSERT_TRUE(floodFillType < Tile::FloodFillType::FloodFillTypeMax);
    if(floodFillType >= Tile::FloodFillType::FloodFillTypeMax)
        return;

    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            Tile* tile = getTile(ii,jj);
            if(tile->mFloodFillColor[floodFillType] == colorOld)
                tile->mFloodFillColor[floodFillType] = colorNew;
        }
    }
}

void GameMap::refreshFloodFill(Tile* tile)
{
    int colors[Tile::FloodFillTypeMax];
    for(int i = 0; i < Tile::FloodFillTypeMax; ++i)
        colors[i] = -1;

    // If the tile has opened a new place, we use the same floodfillcolor for all the areas
    std::vector<Tile*> neighTiles = tile->getAllNeighbors();
    for(std::vector<Tile*>::iterator it = neighTiles.begin(); it != neighTiles.end(); ++it)
    {
        Tile* neigh = *it;
        for(int i = 0; i < Tile::FloodFillTypeMax; ++i)
        {
            if(colors[i] == -1)
            {
                colors[i] = neigh->mFloodFillColor[i];
                tile->mFloodFillColor[i] = neigh->mFloodFillColor[i];
            }
            else if((colors[i] != -1) &&
               (neigh->mFloodFillColor[i] != -1) &&
               (neigh->mFloodFillColor[i] != colors[i]))
            {
                replaceFloodFill(static_cast<Tile::FloodFillType>(i), neigh->mFloodFillColor[i], colors[i]);
            }
        }
    }
}

void GameMap::enableFloodFill()
{
    // Carry out a flood fill of the whole level to make sure everything is good.
    // Start by setting the flood fill color for every tile on the map to -1.
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            for(int kk = 0; kk < Tile::FloodFillTypeMax; ++kk)
            {
                getTile(ii,jj)->mFloodFillColor[kk] = -1;
            }
        }

    }

    // The algorithm used to find a path is efficient when the path exists but not if it doesn't.
    // To improve path finding, we tag the contiguous tiles to know if a path exists between 2 tiles or not.
    // Because creatures can go through ground, water or lava, we process all of theses.
    // Note : when a tile is digged, floodfill will have to be refreshed.
    floodFillEnabled = true;

    // To optimize floodfilling, we start by tagging the dirt tiles with fullness = 0
    // because they are walkable for most creatures. When we will have tagged all
    // thoses, we will deal with water/lava remaining (there can be some left if
    // surrounded by not passable tiles).
    while(true)
    {
        int yy = 0;

        bool isTileFound;
        isTileFound = false;
        while(!isTileFound && (yy < getMapSizeY()))
        {
            for(int xx = 0; xx < getMapSizeX(); ++xx)
            {
                Tile* tile = getTile(xx, yy);
                if((tile->getFullness() == 0.0) &&
                   (tile->mFloodFillColor[Tile::FloodFillTypeGround] == -1) &&
                   ((tile->getType() == Tile::dirt) ||
                    (tile->getType() == Tile::gold) ||
                    (tile->getType() == Tile::claimed)))
                {
                    isTileFound = true;
                    for(int i = 0; i < Tile::FloodFillTypeMax; ++i)
                    {
                        if(tile->mFloodFillColor[i] == -1)
                            tile->mFloodFillColor[i] = nextUniqueNumberFloodFilling();
                    }
                    break;
                }
            }

            if(!isTileFound)
                ++yy;
        }

        // If there are no more walkable tiles, we stop. The only remaining tiles should be
        // surrounded lava/water tiles. We ignore them as it would be time consuming to floodfill
        // them for a case that will almost never happen. If it do happen, we will apply the standard
        // algorithm without floodfill optimization.
        if(!isTileFound)
            break;

        while(yy < getMapSizeY())
        {
            int nbTiles = 0;
            for(int xx = 0; xx < getMapSizeX(); ++xx)
            {
                Tile* tile = getTile(xx, yy);
                if(doFloodFill(tile))
                    ++nbTiles;
            }

            // For optimization purposes, if a tile has changed, we go on the other side
            if(nbTiles > 0)
            {
                for(int xx = getMapSizeX(); xx > 0; --xx)
                {
                    Tile* tile = getTile(xx - 1, yy);
                    if(doFloodFill(tile))
                        ++nbTiles;
                }
            }

            // If at least one tile as been changed, we go back to the previous line
            if((nbTiles > 0) && (yy > 0))
                --yy;
            else
                ++yy;
        }
    }
}

std::list<Tile*> GameMap::path(Creature *c1, Creature *c2, const CreatureDefinition* creatureDef, Seat* seat, bool throughDiggableTiles)
{
    return path(c1->getPositionTile()->x, c1->getPositionTile()->y,
                c2->getPositionTile()->x, c2->getPositionTile()->y, creatureDef, seat, throughDiggableTiles);
}

std::list<Tile*> GameMap::path(Tile *t1, Tile *t2, const CreatureDefinition* creatureDef, Seat* seat, bool throughDiggableTiles)
{
    return path(t1->x, t1->y, t2->x, t2->y, creatureDef, seat, throughDiggableTiles);
}

Ogre::Real GameMap::crowDistance(Creature *c1, Creature *c2)
{
    //TODO:  This is sub-optimal, improve it.
    Tile* tempTile1 = c1->getPositionTile();
    Tile* tempTile2 = c2->getPositionTile();
    return crowDistance(tempTile1->x, tempTile1->y, tempTile2->x, tempTile2->y);
}

void GameMap::processDeletionQueues()
{
    while (entitiesToDelete.size() > 0)
    {
        GameEntity* entity = *entitiesToDelete.begin();
        entitiesToDelete.erase(entitiesToDelete.begin());
        delete entity;
    }

    while (mapLightsToDelete.size() > 0)
    {
        MapLight* ml = *mapLightsToDelete.begin();
        mapLightsToDelete.erase(mapLightsToDelete.begin());
        delete ml;
    }
}

void GameMap::refreshBorderingTilesOf(const std::vector<Tile*>& affectedTiles)
{
    // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
    std::vector<Tile*> borderTiles = tilesBorderedByRegion(affectedTiles);

    borderTiles.insert(borderTiles.end(), affectedTiles.begin(), affectedTiles.end());

    // Loop over all the affected tiles and force them to examine their neighbors.  This allows
    // them to switch to a mesh with fewer polygons if some are hidden by the neighbors, etc.
    for (std::vector<Tile*>::iterator itr = borderTiles.begin(); itr != borderTiles.end() ; ++itr)
        (*itr)->refreshMesh();
}

std::vector<Tile*> GameMap::getDiggableTilesForPlayerInArea(int x1, int y1, int x2, int y2,
    Player* player)
{
    std::vector<Tile*> tiles = rectangularRegion(x1, y1, x2, y2);
    std::vector<Tile*>::iterator it = tiles.begin();
    while (it != tiles.end())
    {
        Tile* tile = *it;
        if (!tile->isDiggable(player->getSeat()))
        {
            it = tiles.erase(it);
        }
        else
            ++it;
    }
    return tiles;
}

std::vector<Tile*> GameMap::getBuildableTilesForPlayerInArea(int x1, int y1, int x2, int y2,
    Player* player)
{
    std::vector<Tile*> tiles = rectangularRegion(x1, y1, x2, y2);
    std::vector<Tile*>::iterator it = tiles.begin();
    while (it != tiles.end())
    {
        Tile* tile = *it;
        if (!tile->isBuildableUpon())
        {
            it = tiles.erase(it);
        }
        else if (!(tile->getFullness() == 0.0
                    && tile->getType() == Tile::claimed
                    && tile->getClaimedPercentage() >= 1.0
                    && tile->isClaimedForSeat(player->getSeat())))
        {
            it = tiles.erase(it);
        }
        else
            ++it;
    }
    return tiles;
}

void GameMap::markTilesForPlayer(std::vector<Tile*>& tiles, bool isDigSet, Player* player)
{
    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        tile->setMarkedForDigging(isDigSet, player);
    }
    refreshBorderingTilesOf(tiles);
}

Room* GameMap::buildRoomForPlayer(std::vector<Tile*>& tiles, Room::RoomType roomType, Player* player, bool forceName, const std::string& name)
{
    Room* newRoom = Room::createRoom(this, roomType, tiles, player->getSeat(), forceName, name);
    Room::setupRoom(this, newRoom);
    refreshBorderingTilesOf(tiles);
    return newRoom;
}

Trap* GameMap::buildTrapForPlayer(std::vector<Tile*>& tiles, Trap::TrapType typeTrap, Player* player, bool forceName, const std::string& name)
{
    Trap* newTrap = Trap::createTrap(this, typeTrap, tiles, player->getSeat(), forceName, name);
    Trap::setupTrap(this, newTrap);
    refreshBorderingTilesOf(tiles);
    return newTrap;
}

std::string GameMap::getGoalsStringForPlayer(Player* player)
{
    bool playerIsAWinner = seatIsAWinner(player->getSeat());
    std::stringstream tempSS("");
    Seat* seat = player->getSeat();
    seat->resetGoalsChanged();

    if (playerIsAWinner)
    {
        tempSS << "\nCongratulations, you have completed this level.";
    }
    else if (seat->numFailedGoals() > 0)
    {
        // Loop over the list of completed goals for the seat we are sitting in an print them.
        tempSS << "\nFailed Goals: (You cannot complete this level!)\n---------------------\n";
        for (unsigned int i = 0; i < seat->numFailedGoals(); ++i)
        {
            Goal *tempGoal = seat->getFailedGoal(i);
            tempSS << tempGoal->getFailedMessage(seat) << "\n";
        }
    }

    if (seat->numUncompleteGoals() > 0)
    {
        // Loop over the list of unmet goals for the seat we are sitting in an print them.
        tempSS << "Unfinished Goals:\n---------------------\n";
        for (unsigned int i = 0; i < seat->numUncompleteGoals(); ++i)
        {
            Goal *tempGoal = seat->getUncompleteGoal(i);
            tempSS << tempGoal->getDescription(seat) << "\n";
        }
    }

    if (seat->numCompletedGoals() > 0)
    {
        // Loop over the list of completed goals for the seat we are sitting in an print them.
        tempSS << "\nCompleted Goals:\n---------------------\n";
        for (unsigned int i = 0; i < seat->numCompletedGoals(); ++i)
        {
            Goal *tempGoal = seat->getCompletedGoal(i);
            tempSS << tempGoal->getSuccessMessage(seat) << "\n";
        }
    }

    return tempSS.str();
}

int GameMap::addGoldToSeat(int gold, int seatId)
{
    Seat* seat = getSeatById(seatId);
    if(seat == NULL)
        return gold;

    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndSeat(Room::treasury, seat);
    for (std::vector<Room*>::iterator it = treasuriesOwned.begin(); it != treasuriesOwned.end(); ++it)
    {
        RoomTreasury* treasury = static_cast<RoomTreasury*>(*it);
        if(treasury->numCoveredTiles() == 0)
            continue;

        gold -= treasury->depositGold(gold, treasury->getCoveredTile(0));
        if(gold <= 0)
            break;
    }

    return gold;
}

int GameMap::nextSeatId(int SeatId)
{
    int firstSeatId = -1;
    bool useNext = false;
    for(std::vector<Seat*>::iterator it = emptySeats.begin(); it != emptySeats.end(); ++it)
    {
        Seat* seat = *it;
        if(useNext)
            return seat->getId();

        if(firstSeatId == -1)
            firstSeatId = seat->getId();

        if(seat->getId() == SeatId)
            useNext = true;
    }

    for(std::vector<Seat*>::iterator it = filledSeats.begin(); it != filledSeats.end(); ++it)
    {
        Seat* seat = *it;
        if(useNext)
            return seat->getId();

        if(firstSeatId == -1)
            firstSeatId = seat->getId();

        if(seat->getId() == SeatId)
            useNext = true;
    }

    // If we reach here, that means that we have to last seat id. We return the first one we could find
    return firstSeatId;
}

std::string GameMap::nextUniqueNameCreature(const std::string& className)
{
    std::string ret;
    do
    {
        ++mUniqueNumberCreature;
        ret = className + Ogre::StringConverter::toString(mUniqueNumberCreature);
    } while(getAnimatedObject(ret) != NULL);
    return ret;
}

int GameMap::nextUniqueNumberFloodFilling()
{
    return ++mUniqueNumberFloodFilling;
}

std::string GameMap::nextUniqueNameMissileObj()
{
    std::string ret;
    do
    {
        ++mUniqueNumberMissileObj;
        ret = MissileObject::MISSILEOBJECT_NAME_PREFIX + Ogre::StringConverter::toString(mUniqueNumberMissileObj);
    } while(getAnimatedObject(ret) != NULL);
    return ret;
}

std::string GameMap::nextUniqueNameRoom(const std::string& meshName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberRoom;
        ret = meshName + Ogre::StringConverter::toString(mUniqueNumberRoom);
    } while(getAnimatedObject(ret) != NULL);
    return ret;
}

std::string GameMap::nextUniqueNameRoomObj(const std::string& baseName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberRoomObj;
        ret = RoomObject::ROOMOBJECT_PREFIX + baseName + "_" + Ogre::StringConverter::toString(mUniqueNumberRoomObj);
    } while(getAnimatedObject(ret) != NULL);
    return ret;
}

std::string GameMap::nextUniqueNameTrap(const std::string& meshName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberTrap;
        ret = meshName + "_" + Ogre::StringConverter::toString(mUniqueNumberTrap);
    } while(getAnimatedObject(ret) != NULL);
    return ret;
}

std::string GameMap::nextUniqueNameMapLight()
{
    std::string ret;
    do
    {
        ++mUniqueNumberMapLight;
        ret = MapLight::MAPLIGHT_NAME_PREFIX + Ogre::StringConverter::toString(mUniqueNumberMapLight);
    } while(getAnimatedObject(ret) != NULL);
    return ret;
}

void GameMap::logFloodFileTiles()
{
    for(int yy = 0; yy < getMapSizeY(); ++yy)
    {
        for(int xx = 0; xx < getMapSizeX(); ++xx)
        {
            Tile* tile = getTile(xx, yy);
            std::string str = "Tile floodfill : " + Tile::displayAsString(tile) + " - fullness=" + Ogre::StringConverter::toString(tile->getFullness());
            for(int i = 0; i < Tile::FloodFillTypeMax; ++i)
            {
                str += ", [" + Ogre::StringConverter::toString(i) + "]=" +
                    Ogre::StringConverter::toString(tile->getFloodFill(static_cast<Tile::FloodFillType>(i)));
            }
            LogManager::getSingleton().logMessage(str);
        }
    }
}

void GameMap::consoleSetCreatureDestination(const std::string& creatureName, int x, int y)
{
    Creature* creature = getCreature(creatureName);
    if(creature == NULL)
        return;
    Tile* tile = getTile(x, y);
    if(tile == NULL)
        return;
    if(creature->getPositionTile() == NULL)
        return;
    creature->clearActionQueue();
    creature->clearDestinations();
    creature->setDestination(tile);
}

bool GameMap::pathToBestFightingPosition(std::list<Tile*>& pathToTarget, Creature* attackingCreature,
    Tile* attackedTile)
{
    // First, we search the tiles from where we can attack as far as possible
    Tile* tileCreature = attackingCreature->getPositionTile();
    if((tileCreature == NULL) || (attackedTile == NULL))
        return false;

    double rangeL = attackingCreature->getWeaponL()->getRange();
    double rangeR = attackingCreature->getWeaponR()->getRange();
    double range = std::max(rangeL, rangeR);

    std::vector<Tile*> possibleTiles;
    while(possibleTiles.empty() && range >= 0)
    {
        for(int i = -range; i <= range; ++i)
        {
            int diffY = range - std::abs(i);
            Tile* tile;
            tile = getTile(attackedTile->getX() + i, attackedTile->getY() + diffY);
            if(tile != NULL && pathExists(tileCreature, tile, attackingCreature->getDefinition()))
                possibleTiles.push_back(tile);

            if(diffY == 0)
                continue;

            tile = getTile(attackedTile->getX() + i, attackedTile->getY() - diffY);
            if(tile != NULL && pathExists(tileCreature, tile, attackingCreature->getDefinition()))
                possibleTiles.push_back(tile);
        }

        // If we could find no tile within range, we decrease range and search again
        if(possibleTiles.empty())
            --range;
    }

    // If we found no tile, return empty list
    if(possibleTiles.empty())
        return false;

    // To find the closest tile, we only consider distance to avoid too complex
    Tile* closestTile = *possibleTiles.begin();
    double shortestDist = std::pow(static_cast<double>(std::abs(closestTile->getX() - tileCreature->getX())), 2);
    shortestDist += std::pow(static_cast<double>(std::abs(closestTile->getY() - tileCreature->getY())), 2);
    for(std::vector<Tile*>::iterator it = (possibleTiles.begin() + 1); it != possibleTiles.end(); ++it)
    {
        Tile* tile = *it;
        double dist = std::pow(static_cast<double>(std::abs(tile->getX() - tileCreature->getX())), 2);
        dist += std::pow(static_cast<double>(std::abs(tile->getY() - tileCreature->getY())), 2);
        if(dist < shortestDist)
        {
            shortestDist = dist;
            closestTile = tile;
        }
    }

    if(tileCreature == closestTile)
        return true;

    pathToTarget = path(tileCreature, closestTile, attackingCreature->getDefinition(), attackingCreature->getSeat());
    return true;
}

GameEntity* GameMap::getClosestTileWhereGameEntityFromList(std::vector<GameEntity*> listObjects, Tile* origin, Tile*& attackedTile)
{
    if(listObjects.empty())
        return NULL;

    GameEntity* closestGameEntity = NULL;
    double shortestDist = 0.0;
    for(std::vector<GameEntity*>::iterator itObj = listObjects.begin(); itObj != listObjects.end(); ++itObj)
    {
        GameEntity* gameEntity = *itObj;
        std::vector<Tile*> tiles = gameEntity->getCoveredTiles();
        for(std::vector<Tile*>::iterator itTile = tiles.begin(); itTile != tiles.end(); ++itTile)
        {
            Tile* tile = *itTile;
            OD_ASSERT_TRUE(tile != NULL);
            double dist = std::pow(static_cast<double>(std::abs(tile->getX() - origin->getX())), 2);
            dist += std::pow(static_cast<double>(std::abs(tile->getY() - origin->getY())), 2);
            if(closestGameEntity == NULL || dist < shortestDist)
            {
                shortestDist = dist;
                closestGameEntity = gameEntity;
                attackedTile = tile;
            }
        }
    }

    return closestGameEntity;
}
