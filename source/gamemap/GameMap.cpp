/*!
 * \file   GameMap.cpp
 * \brief  The central object holding everything that is on the map
 *
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "gamemap/GameMap.h"

#include "creaturemood/CreatureMood.h"
#include "creaturemood/CreatureMoodCreature.h"

#include "gamemap/MapLoader.h"

#include "goals/Goal.h"

#include "entities/Tile.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/MapLight.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Weapon.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "modes/ModeManager.h"

#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/ODFrameListener.h"

#include "rooms/Room.h"
#include "rooms/RoomDungeonTemple.h"
#include "rooms/RoomPortal.h"
#include "rooms/RoomTreasury.h"
#include "rooms/RoomType.h"

#include "spell/Spell.h"

#include "traps/Trap.h"

#include "utils/LogManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/ResourceManager.h"

#include <OgreTimer.h>
#include <OgreStringConverter.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include <cmath>
#include <cstdlib>
#include <cassert>

const std::string DEFAULT_NICK = "Player";

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
        tile    (nullptr),
        parent  (nullptr),
        g       (0.0),
        h       (0.0)
    {}

    AstarEntry(Tile* tile, int x1, int y1, int x2, int y2) :
        tile    (tile),
        parent  (nullptr),
        g       (0.0),
        h       (0.0)
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
        TileContainer(isServerGameMap ? 15 : 0),
        mIsServerGameMap(isServerGameMap),
        mLocalPlayer(nullptr),
        mLocalPlayerNick(DEFAULT_NICK),
        mTurnNumber(-1),
        mIsPaused(false),
        mTimePayDay(0),
        mFloodFillEnabled(false),
        mIsFOWActivated(true),
        mNumCallsTo_path(0),
        mAiManager(*this),
        mTileSet(nullptr)
{
    resetUniqueNumbers();
}

GameMap::~GameMap()
{
    clearAll();
    processDeletionQueues();
}

bool GameMap::isInEditorMode() const
{
    if (isServerGameMap())
        return (ODServer::getSingleton().getServerMode() == ServerMode::ModeEditor);

    return (ODFrameListener::getSingleton().getModeManager()->getCurrentModeTypeExceptConsole() == ModeManager::EDITOR);
}

bool GameMap::loadLevel(const std::string& levelFilepath)
{
    // We reset the creature definitions
    clearClasses();
    for(auto it : ConfigManager::getSingleton().getCreatureDefinitions())
    {
        addClassDescription(it.second);
    }

    // We reset the weapons definitions
    clearWeapons();
    const std::vector<const Weapon*>& weapons = ConfigManager::getSingleton().getWeapons();
    for(const Weapon* def : weapons)
    {
        addWeapon(def);
    }

    // We reset the mood modifiers
    clearCreatureMoodModifiers();
    const std::map<const std::string, std::vector<const CreatureMood*>>& moodModifiers = ConfigManager::getSingleton().getCreatureMoodModifiers();
    for(std::pair<const std::string, std::vector<const CreatureMood*>> p : moodModifiers)
    {
        std::vector<CreatureMood*> moodModifiersClone;
        for(const CreatureMood* mood : p.second)
        {
            moodModifiersClone.push_back(mood->clone());
        }
        addCreatureMoodModifiers(p.first, moodModifiersClone);
    }

    // We add the rogue default seat (seatId = 0 and teamId = 0)
    Seat* rogueSeat = Seat::createRogueSeat(this);
    if(!addSeat(rogueSeat))
    {
        OD_ASSERT_TRUE(false);
        delete rogueSeat;
    }

    if (MapLoader::readGameMapFromFile(levelFilepath, *this))
        setLevelFileName(levelFilepath);
    else
        return false;

    // We initialize the mood modifiers
    for(std::pair<const std::string, std::vector<CreatureMood*>>& p : mCreatureMoodModifiers)
    {
        for(CreatureMood* creatureMood : p.second)
        {
            creatureMood->init(this);
        }
    }

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
            tile->setType(TileType::dirt);
            tile->addToGameMap();
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
    clearWeapons();
    clearTraps();

    clearMapLights();
    clearRooms();
    // NOTE : clearRenderedMovableEntities should be called after clearRooms because clearRooms will try to remove the objects from the room
    clearRenderedMovableEntities();
    clearSpells();
    clearTiles();
    clearCreatureMoodModifiers();

    clearActiveObjects();

    clearGoalsForAllSeats();
    clearSeats();
    mLocalPlayer = nullptr;
    clearPlayers();

    clearAiManager();

    mLocalPlayerNick = DEFAULT_NICK;
    mTurnNumber = -1;
    resetUniqueNumbers();
    mIsFOWActivated = true;
    mTimePayDay = 0;
}

void GameMap::clearCreatures()
{
    // We need to work on a copy of mCreatures because removeFromGameMap will remove them from this vector
    std::vector<Creature*> creatures = mCreatures;
    for (Creature* creature : creatures)
    {
        creature->removeFromGameMap();
        creature->deleteYourself();
    }

    mCreatures.clear();
}

void GameMap::clearAiManager()
{
   mAiManager.clearAIList();
}

void GameMap::clearClasses()
{
    for (std::pair<const CreatureDefinition*,CreatureDefinition*>& def : mClassDescriptions)
    {
        if(def.second != nullptr)
            delete def.second;

        // On client side, classes are sent by network so they should be deleted
        if(!isServerGameMap())
            delete def.first;
    }
    mClassDescriptions.clear();
}

void GameMap::clearWeapons()
{
    for (std::pair<const Weapon*,Weapon*>& def : mWeapons)
    {
        if(def.second != nullptr)
            delete def.second;

        // On client side, weapons are sent by network so they should be deleted
        if(!isServerGameMap())
            delete def.first;
    }
    mWeapons.clear();
}

void GameMap::clearRenderedMovableEntities()
{
    // We need to work on a copy of mRenderedMovableEntities because removeFromGameMap will remove them from this vector
    std::vector<RenderedMovableEntity*> renderedMovableEntities = mRenderedMovableEntities;
    for (RenderedMovableEntity* obj : renderedMovableEntities)
    {
        obj->removeFromGameMap();
        obj->deleteYourself();
    }

    mRenderedMovableEntities.clear();
}

void GameMap::clearActiveObjects()
{
    mActiveObjects.clear();
    mActiveObjectsToAdd.clear();
    mActiveObjectsToRemove.clear();
}

void GameMap::clearPlayers()
{
    for (Player* player : mPlayers)
        delete player;

    mPlayers.clear();
}

void GameMap::resetUniqueNumbers()
{
    mUniqueNumberCreature = 0;
    mUniqueNumberMissileObj = 0;
    mUniqueNumberRoom = 0;
    mUniqueNumberRenderedMovableEntity = 0;
    mUniqueNumberTrap = 0;
    mUniqueNumberMapLight = 0;
}

void GameMap::addClassDescription(const CreatureDefinition *c)
{
    mClassDescriptions.push_back(std::pair<const CreatureDefinition*,CreatureDefinition*>(c, nullptr));
}

void GameMap::addWeapon(const Weapon* weapon)
{
    mWeapons.push_back(std::pair<const Weapon*,Weapon*>(weapon, nullptr));
}

const Weapon* GameMap::getWeapon(int index)
{
    OD_ASSERT_TRUE_MSG(index < static_cast<int>(mWeapons.size()), "index=" + Ogre::StringConverter::toString(index));
    if(index >= static_cast<int>(mWeapons.size()))
        return nullptr;

    std::pair<const Weapon*,Weapon*>& def = mWeapons[index];
    if(def.second != nullptr)
        return def.second;

    return def.first;
}

const Weapon* GameMap::getWeapon(const std::string& name)
{
    for (std::pair<const Weapon*,Weapon*>& def : mWeapons)
    {
        if(def.second != nullptr)
        {
            if (def.second->getName().compare(name) == 0)
                return def.second;
        }
        else if(def.first->getName().compare(name) == 0)
            return def.first;
    }

    return nullptr;
}

Weapon* GameMap::getWeaponForTuning(const std::string& name)
{
    for (std::pair<const Weapon*,Weapon*>& def : mWeapons)
    {
        if(def.second != nullptr)
        {
            if (def.second->getName().compare(name) == 0)
                return def.second;
        }
        else if(def.first->getName().compare(name) == 0)
        {
            // If the definition is not a copy, we make one because we want to keep the original so we are able
            // to save the changes if the map is saved
            def.second = new Weapon(*def.first);
            return def.second;
        }
    }

    // It is a new definition
    Weapon* def = new Weapon(name);
    mWeapons.push_back(std::pair<const Weapon*,Weapon*>(nullptr, def));
    return def;
}

uint32_t GameMap::numWeapons()
{
    return mWeapons.size();
}

void GameMap::saveLevelEquipments(std::ofstream& levelFile)
{
    for (std::pair<const Weapon*,Weapon*>& def : mWeapons)
    {
        if(def.second == nullptr)
            continue;

        Weapon::writeWeaponDiff(def.first, def.second, levelFile);
    }
}

void GameMap::addCreature(Creature *cc)
{
    LogManager::getSingleton().logMessage(serverStr() + "Adding Creature " + cc->getName()
        + ", seatId=" + (cc->getSeat() != nullptr ? Ogre::StringConverter::toString(cc->getSeat()->getId()) : std::string("null")));

    mCreatures.push_back(cc);
}

void GameMap::removeCreature(Creature *c)
{
    LogManager::getSingleton().logMessage(serverStr() + "Removing Creature " + c->getName());

    std::vector<Creature*>::iterator it = std::find(mCreatures.begin(), mCreatures.end(), c);
    OD_ASSERT_TRUE_MSG(it != mCreatures.end(), "creature name=" + c->getName());
    if(it == mCreatures.end())
        return;

    mCreatures.erase(it);
}

void GameMap::queueEntityForDeletion(GameEntity *ge)
{
    mEntitiesToDelete.push_back(ge);
}

const CreatureDefinition* GameMap::getClassDescription(const string &className)
{
    for (std::pair<const CreatureDefinition*,CreatureDefinition*>& def : mClassDescriptions)
    {
        if (def.second != nullptr)
        {
            if(def.second->getClassName().compare(className) == 0)
                return def.second;
        }
        else if(def.first->getClassName().compare(className) == 0)
            return def.first;
    }

    return nullptr;
}

CreatureDefinition* GameMap::getClassDescriptionForTuning(const std::string& name)
{
    for (std::pair<const CreatureDefinition*,CreatureDefinition*>& def : mClassDescriptions)
    {
        if(def.second != nullptr)
        {
            if (def.second->getClassName().compare(name) == 0)
                return def.second;
        }
        else if(def.first->getClassName().compare(name) == 0)
        {
            def.second = new CreatureDefinition(*def.first);
            return def.second;
        }
    }

    // It is a new definition
    CreatureDefinition* def = new CreatureDefinition(name);
    mClassDescriptions.push_back(std::pair<const CreatureDefinition*,CreatureDefinition*>(nullptr, def));
    return def;
}

std::vector<Creature*> GameMap::getCreaturesByAlliedSeat(Seat* seat)
{
    std::vector<Creature*> tempVector;

    // Loop over all the creatures in the GameMap and add them to the temp vector if their seat matches the one in parameter.
    for (Creature* creature : mCreatures)
    {
        if (seat->isAlliedSeat(creature->getSeat()) && creature->getHP() > 0.0)
            tempVector.push_back(creature);
    }

    return tempVector;
}

std::vector<Creature*> GameMap::getCreaturesBySeat(Seat* seat)
{
    std::vector<Creature*> tempVector;

    // Loop over all the creatures in the GameMap and add them to the temp vector if their seat matches the one in parameter.
    for (Creature* creature : mCreatures)
    {
        if (creature->getSeat() == seat && creature->getHP() > 0.0)
            tempVector.push_back(creature);
    }

    return tempVector;
}

Creature* GameMap::getWorkerToPickupBySeat(Seat* seat)
{
    // 1 - Take idle worker
    // 2 - Take fighting/fleeing
    // 3 - Take claimers
    // 4 - Take diggers
    // 5 - Take gold digger/depositers or all the rest
    // For each, we pickup the highest leveled available

    uint32_t idleWorkerLevel = 0;
    Creature* idleWorker = nullptr;
    uint32_t fightingWorkerLevel = 0;
    Creature* fightingWorker = nullptr;
    uint32_t claimerWorkerLevel = 0;
    Creature* claimerWorker = nullptr;
    uint32_t diggerWorkerLevel = 0;
    Creature* diggerWorker = nullptr;
    uint32_t otherWorkerLevel = 0;
    Creature* otherWorker = nullptr;
    std::vector<Creature*> creatures = getCreaturesBySeat(seat);
    for(Creature* creature : creatures)
    {
        if(!creature->getDefinition()->isWorker())
            continue;

        if(!creature->tryPickup(seat))
            continue;

        bool isIdle = true;
        bool isFighting = false;
        bool isClaiming = false;
        bool isDigging = false;

        const std::deque<CreatureAction>& actions = creature->getActionQueue();
        for(std::deque<CreatureAction>::const_iterator itAction = actions.begin(); itAction != actions.end(); ++itAction)
        {
            const CreatureAction& action = *itAction;
            switch(action.getType())
            {
                case CreatureActionType::fight:
                case CreatureActionType::attackObject:
                case CreatureActionType::flee:
                    isIdle = false;
                    isFighting = true;
                    break;

                case CreatureActionType::claimTile:
                    isIdle = false;
                    isClaiming = true;
                    break;

                case CreatureActionType::digTile:
                    isIdle = false;
                    isDigging = true;
                    break;

                case CreatureActionType::idle:
                case CreatureActionType::walkToTile:
                    // We do nothing
                    break;

                default:
                    // Other
                    isIdle = false;
                    break;
            }
        }

        if(isIdle)
        {
            if(creature->getLevel() > idleWorkerLevel)
            {
                idleWorker = creature;
                idleWorkerLevel = creature->getLevel();
            }
        }
        else if(isFighting)
        {
            if(creature->getLevel() > fightingWorkerLevel)
            {
                fightingWorker = creature;
                fightingWorkerLevel = creature->getLevel();
            }
        }
        else if(isClaiming)
        {
            if(creature->getLevel() > claimerWorkerLevel)
            {
                claimerWorker = creature;
                claimerWorkerLevel = creature->getLevel();
            }
        }
        else if(isDigging)
        {
            if(creature->getLevel() > diggerWorkerLevel)
            {
                diggerWorker = creature;
                diggerWorkerLevel = creature->getLevel();
            }
        }
        else
        {
            if(creature->getLevel() > otherWorkerLevel)
            {
                otherWorker = creature;
                otherWorkerLevel = creature->getLevel();
            }
        }
    }
    if(idleWorker != nullptr)
        return idleWorker;
    else if(fightingWorker != nullptr)
        return fightingWorker;
    else if(claimerWorker != nullptr)
        return claimerWorker;
    else if(diggerWorker != nullptr)
        return diggerWorker;
    else if(otherWorker != nullptr)
        return otherWorker;

    return nullptr;
}

Creature* GameMap::getFighterToPickupBySeat(Seat* seat)
{
    // 1 - Take fleeing fighters
    // 2 - Take idle fighters
    // 3 - Take busy (working/eating) fighters
    // 4 - Then take fighting fighters or all the rest
    // For each, we pickup the highest leveled available

    uint32_t idleFighterLevel = 0;
    Creature* idleFighter = nullptr;
    uint32_t fleeingFighterLevel = 0;
    Creature* fleeingFighter = nullptr;
    uint32_t busyFighterLevel = 0;
    Creature* busyFighter = nullptr;
    uint32_t otherFighterLevel = 0;
    Creature* otherFighter = nullptr;
    std::vector<Creature*> creatures = getCreaturesBySeat(seat);
    for(Creature* creature : creatures)
    {
        if(creature->getDefinition()->isWorker())
            continue;

        if(!creature->tryPickup(seat))
            continue;

        bool isIdle = true;
        bool isFleeing = false;
        bool isBusy = false;

        const std::deque<CreatureAction>& actions = creature->getActionQueue();
        for(std::deque<CreatureAction>::const_iterator itAction = actions.begin(); itAction != actions.end(); ++itAction)
        {
            const CreatureAction& action = *itAction;
            switch(action.getType())
            {
                case CreatureActionType::flee:
                    isIdle = false;
                    isFleeing = true;
                    break;

                case CreatureActionType::eatdecided:
                case CreatureActionType::eatforced:
                case CreatureActionType::jobdecided:
                case CreatureActionType::jobforced:
                    isIdle = false;
                    isBusy = true;
                    break;

                case CreatureActionType::idle:
                case CreatureActionType::walkToTile:
                    // We do nothing
                    break;

                default:
                    // Other
                    isIdle = false;
                    break;
            }
        }

        if(isFleeing)
        {
            if(creature->getLevel() > fleeingFighterLevel)
            {
                fleeingFighter = creature;
                fleeingFighterLevel = creature->getLevel();
            }
        }
        else if(isIdle)
        {
            if(creature->getLevel() > idleFighterLevel)
            {
                idleFighter = creature;
                idleFighterLevel = creature->getLevel();
            }
        }
        else if(isBusy)
        {
            if(creature->getLevel() > busyFighterLevel)
            {
                busyFighter = creature;
                busyFighterLevel = creature->getLevel();
            }
        }
        else
        {
            if(creature->getLevel() > otherFighterLevel)
            {
                otherFighter = creature;
                otherFighterLevel = creature->getLevel();
            }
        }
    }
    if(fleeingFighter != nullptr)
        return fleeingFighter;
    else if(idleFighter != nullptr)
        return idleFighter;
    else if(busyFighter != nullptr)
        return busyFighter;
    else if(otherFighter != nullptr)
        return otherFighter;

    return nullptr;
}

void GameMap::clearAnimatedObjects()
{
    mAnimatedObjects.clear();
}

void GameMap::addAnimatedObject(MovableGameEntity *a)
{
    mAnimatedObjects.push_back(a);
}

void GameMap::removeAnimatedObject(MovableGameEntity *a)
{
    std::vector<MovableGameEntity*>::iterator it = std::find(mAnimatedObjects.begin(), mAnimatedObjects.end(), a);
    if(it == mAnimatedObjects.end())
        return;

    mAnimatedObjects.erase(it);
}

MovableGameEntity* GameMap::getAnimatedObject(const std::string& name) const
{
    for (MovableGameEntity* mge : mAnimatedObjects)
    {
        if (mge->getName().compare(name) == 0)
            return mge;
    }

    return nullptr;
}

void GameMap::addRenderedMovableEntity(RenderedMovableEntity *obj)
{
    LogManager::getSingleton().logMessage(serverStr() + "Adding rendered object " + obj->getName()
        + ",MeshName=" + obj->getMeshName());
    mRenderedMovableEntities.push_back(obj);
}

void GameMap::removeRenderedMovableEntity(RenderedMovableEntity *obj)
{
    LogManager::getSingleton().logMessage(serverStr() + "Removing rendered object " + obj->getName()
        + ",MeshName=" + obj->getMeshName());
    std::vector<RenderedMovableEntity*>::iterator it = std::find(mRenderedMovableEntities.begin(), mRenderedMovableEntities.end(), obj);
    OD_ASSERT_TRUE_MSG(it != mRenderedMovableEntities.end(), "obj name=" + obj->getName());
    if(it == mRenderedMovableEntities.end())
        return;

    mRenderedMovableEntities.erase(it);
}

RenderedMovableEntity* GameMap::getRenderedMovableEntity(const std::string& name)
{
    for(RenderedMovableEntity* renderedMovableEntity : mRenderedMovableEntities)
    {
        if(name.compare(renderedMovableEntity->getName()) == 0)
            return renderedMovableEntity;
    }
    return nullptr;
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

    if(std::find(mActiveObjects.begin(), mActiveObjects.end(), a) != mActiveObjects.end())
    {
        mActiveObjectsToRemove.push_back(a);
        return;
    }

    if(std::find(mActiveObjectsToAdd.begin(), mActiveObjectsToAdd.end(), a) != mActiveObjectsToAdd.end())
    {
        mActiveObjectsToRemove.push_back(a);
        return;
    }
}

unsigned int GameMap::numClassDescriptions()
{
    return mClassDescriptions.size();
}

void GameMap::saveLevelClassDescriptions(std::ofstream& levelFile)
{
    for (std::pair<const CreatureDefinition*,CreatureDefinition*>& def : mClassDescriptions)
    {
        if(def.second == nullptr)
            continue;

        CreatureDefinition::writeCreatureDefinitionDiff(def.first, def.second, levelFile, ConfigManager::getSingleton().getCreatureDefinitions());
    }
}

const CreatureDefinition* GameMap::getClassDescription(int index)
{
    std::pair<const CreatureDefinition*,CreatureDefinition*>& def = mClassDescriptions[index];
    if(def.second != nullptr)
        return def.second;
    else
        return def.first;
}

void GameMap::createAllEntities()
{
    mTileSet = ConfigManager::getSingleton().getTileSet(mTileSetName);

    std::vector<GameEntity*> entities;
    // Create OGRE entities for map tiles
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            getTile(ii,jj)->createMesh();
        }
    }

    // Create OGRE entities for rendered entities
    for (RenderedMovableEntity* rendered : mRenderedMovableEntities)
    {
        rendered->createMesh();
        rendered->setPosition(rendered->getPosition(), false);
        entities.push_back(rendered);
    }

    // Create OGRE entities for the creatures
    for (Creature* creature : mCreatures)
    {
        creature->createMesh();
        creature->setPosition(creature->getPosition(), false);
        entities.push_back(creature);
    }

    // Create OGRE entities for the map lights.
    for (MapLight* mapLight: mMapLights)
    {
        mapLight->createMesh();
        mapLight->setPosition(mapLight->getPosition(), false);
    }

    // Create OGRE entities for the rooms
    for (Room* room : mRooms)
    {
        room->createMesh();
        room->updateActiveSpots();
        entities.push_back(room);
    }

    // Create OGRE entities for the rooms
    for (Trap* trap : mTraps)
    {
        trap->createMesh();
        trap->updateActiveSpots();
        entities.push_back(trap);
    }

    // Create OGRE entities for spells
    for (Spell* spell : mSpells)
    {
        spell->createMesh();
        spell->setPosition(spell->getPosition(), false);
        entities.push_back(spell);
    }

    for(GameEntity* entity : entities)
    {
        // We restore the initial states
        entity->restoreInitialEntityState();
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
    for (Creature* creature : mCreatures)
    {
        creature->destroyMesh();
    }

    // Destroy OGRE entities for the map lights.
    for (MapLight* mapLight : mMapLights)
    {
        mapLight->destroyMesh();
    }

    // Destroy OGRE entities for the rooms
    for (Room *currentRoom : mRooms)
    {
        currentRoom->destroyMesh();
    }

    // Destroy OGRE entities for the traps
    for (Trap* trap : mTraps)
    {
        trap->destroyMesh();
    }
}

Creature* GameMap::getCreature(const std::string& cName) const
{
    for (Creature* creature : mCreatures)
    {
        if (creature->getName().compare(cName) == 0)
            return creature;
    }
    return nullptr;
}

void GameMap::doTurn()
{
    std::cout << "\nComputing turn " << mTurnNumber << std::endl;
    unsigned int numCallsTo_path_atStart = mNumCallsTo_path;

    uint32_t miscUpkeepTime = doMiscUpkeep();

    // Count how many creatures the player controls
    for(Creature* creature : mCreatures)
    {
        // Check to see if the creature has died.
        if (creature->getHP() <= 0.0)
            continue;

        // We only count fighters
        if (creature->getDefinition()->isWorker())
            continue;

        Seat *tempSeat = creature->getSeat();
        if(tempSeat == nullptr)
            continue;

        ++(tempSeat->mNumCreaturesFighters);
    }

    std::cout << "During this turn there were " << mNumCallsTo_path
              - numCallsTo_path_atStart << " calls to GameMap::path()."
              << "miscUpkeepTime=" << miscUpkeepTime << std::endl;
}

void GameMap::doPlayerAITurn(double frameTime)
{
    mAiManager.doTurn(frameTime);
}

unsigned long int GameMap::doMiscUpkeep()
{
    Tile *tempTile;
    Ogre::Timer stopwatch;
    unsigned long int timeTaken;

    // Loop over all the filled seats in the game and check all the unfinished goals for each seat.
    // Add any seats with no remaining goals to the winningSeats vector.
    for (Seat* seat : mSeats)
    {
        if(seat->getPlayer() == nullptr)
            continue;

        // Check the previously completed goals to make sure they are still met.
        seat->checkAllCompletedGoals();

        // Check the goals and move completed ones to the completedGoals list for the seat.
        //NOTE: Once seats are placed on this list, they stay there even if goals are unmet.  We may want to change this.
        if (seat->checkAllGoals() == 0
                && seat->numFailedGoals() == 0)
            addWinningSeat(seat);

        seat->mNumCreaturesFightersMax = getMaxNumberCreatures(seat);

        // Set the creatures count to 0. It will be reset by the next count in doTurn()
        seat->mNumCreaturesFighters = 0;
    }

    // At each upkeep, we re-compute tiles with vision
    for (Seat* seat : mSeats)
        seat->clearTilesWithVision();

    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            getTile(ii,jj)->clearVision();
        }
    }

    // Compute vision. We need to compute every seats including AI because
    // a human can be allied with an AI and they would share vision
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            getTile(ii,jj)->computeVisibleTiles();
        }
    }

    for (Creature* creature : mCreatures)
    {
        creature->computeVisibleTiles();
    }

    for (Seat* seat : mSeats)
    {
        if(!seat->getIsDebuggingVision())
            continue;

        seat->displaySeatVisualDebug(true);
    }

    // We send to each seat the list of tiles he has vision on
    for (Seat* seat : mSeats)
        seat->sendVisibleTiles();

    // Carry out the upkeep round of all the active objects in the game.
    unsigned int activeObjectCount = 0;
    unsigned int nbActiveObjectCount = mActiveObjects.size();
    while (activeObjectCount < nbActiveObjectCount)
    {
        GameEntity* ge = mActiveObjects[activeObjectCount];
        ge->doUpkeep();

        ++activeObjectCount;
    }

    // Carry out the upkeep round for each seat. This means recomputing how much gold is
    // available in their treasuries, how much mana they gain/lose during this turn, etc.
    for (Seat* seat : mSeats)
    {
        if(seat->getPlayer() == nullptr)
            continue;

        // Add the amount of mana this seat accrued this turn if the player has a dungeon temple
        std::vector<Room*> dungeonTemples = getRoomsByTypeAndSeat(RoomType::dungeonTemple, seat);
        if(dungeonTemples.empty())
        {
            seat->mManaDelta = 0;
            seat->getPlayer()->notifyNoMoreDungeonTemple();
        }
        else
        {
            seat->mManaDelta = 50 + seat->getNumClaimedTiles();
            seat->mMana += seat->mManaDelta;
            double maxMana = ConfigManager::getSingleton().getMaxManaPerSeat();
            if (seat->mMana > maxMana)
                seat->mMana = maxMana;
        }

        // Update the count on how much gold is available in all of the treasuries claimed by the given seat.
        seat->mGold = getTotalGoldForSeat(seat);
    }

    // Determine the number of tiles claimed by each seat.
    // Begin by setting the number of claimed tiles for each seat to 0.
    for (Seat* seat : mSeats)
        seat->setNumClaimedTiles(0);

    // Now loop over all of the tiles, if the tile is claimed increment the given seats count.
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            tempTile = getTile(ii,jj);

            // Check to see if the current tile is claimed by anyone.
            if (tempTile->isClaimed())
            {
                // Increment the count of the seat who owns the tile.
                tempTile->getSeat()->incrementNumClaimedTiles();
            }
        }
    }

    timeTaken = stopwatch.getMicroseconds();
    return timeTaken;
}

int GameMap::getNbWorkersForSeat(Seat* seat) const
{
    int ret = 0;
    for (Creature* creature : mCreatures)
    {
        if(creature->getSeat() != seat)
            continue;

        if(creature->getHP() <= 0)
            continue;

        if(!creature->getDefinition()->isWorker())
            continue;

        ++ret;
    }

    return ret;
}

int GameMap::getNbFightersForSeat(Seat* seat) const
{
    int ret = 0;
    for (Creature* creature : mCreatures)
    {
        if (creature->getSeat() != seat)
            continue;

        if(creature->getHP() <= 0)
            continue;

        if (creature->getDefinition()->isWorker())
            continue;

        ++ret;
    }

    return ret;
}

void GameMap::updateAnimations(Ogre::Real timeSinceLastFrame)
{
    // During the first turn, we setup everything
    if(getTurnNumber() == 0 && !isServerGameMap())
    {
        assert(getTile(0, 0) != nullptr);
        //NOTE: This test is a workaround to prevent this being called more than once.
        //This should probably be fixed in a better way.
        if(!getTile(0, 0)->isMeshExisting())
        {
            LogManager::getSingleton().logMessage("Starting game map");

            setGamePaused(false);

            // Create ogre entities for the tiles, rooms, and creatures
            createAllEntities();
        }
    }

    if(mIsPaused)
        return;

    if(getTurnNumber() > 0)
    {
        // Update the animations on any AnimatedObjects which have them
        for(MovableGameEntity* currentAnimatedObject : mAnimatedObjects)
        {
            if (currentAnimatedObject == nullptr)
                continue;

            currentAnimatedObject->update(timeSinceLastFrame);
        }
    }

    if(isServerGameMap())
    {
        updatePlayerTime(timeSinceLastFrame);

        // We check if it is pay day
        mTimePayDay += timeSinceLastFrame;
        if((mTimePayDay >= ConfigManager::getSingleton().getTimePayDay()))
        {
            mTimePayDay = 0;
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, nullptr);
            serverNotification->mPacket << "It's pay day !";
            ODServer::getSingleton().queueServerNotification(serverNotification);
            for(Creature* creature : mCreatures)
            {
                creature->itsPayDay();
            }
        }
        return;
    }
}

void GameMap::updatePlayerTime(Ogre::Real timeSinceLastFrame)
{
    // Updates fighting time for server players
    for (Player* player : mPlayers)
    {
        if (player == nullptr)
            continue;

        if (!player->getIsHuman())
            continue;

        player->updateTime(timeSinceLastFrame);
    }
}

void GameMap::playerIsFighting(Player* player, Tile* tile)
{
    if (player == nullptr)
        return;

    // No need to check for inactive players
    if (player->getSeat() == nullptr)
        return;

    // Get every player's allies
    for (Player* ally : mPlayers)
    {
        // No need to warn AI about music
        if (!ally || !ally->getIsHuman())
            continue;

        if (ally->getSeat() == nullptr || !ally->getSeat()->isAlliedSeat(player->getSeat()))
            continue;

        // Warn the ally about the battle
        ally->notifyTeamFighting(player, tile);
    }
}

std::list<Tile*> GameMap::findBestPath(const Creature* creature, Tile* tileStart, const std::vector<Tile*> possibleDests,
    Tile*& chosenTile)
{
    chosenTile = nullptr;
    std::list<Tile*> returnList;
    if(possibleDests.empty())
        return returnList;

    // We start by sorting the vector
    std::vector<Tile*> possibleDestsTmp = possibleDests;
    std::sort(possibleDestsTmp.begin(), possibleDestsTmp.end(), [this, &tileStart](Tile* tile1, Tile* tile2){
              Ogre::Real d1 = crowDistance(tile1, tileStart);
              Ogre::Real d2 = crowDistance(tile2, tileStart);
              return d1 < d2;
        });

    double magic = 2.0;
    for(Tile* tile : possibleDestsTmp)
    {
        if(!pathExists(creature, tileStart, tile))
            continue;

        // The first reachable tile is the best by default
        Ogre::Real dist = crowDistance(tile, tileStart);
        if(chosenTile == nullptr)
        {
            chosenTile = tile;
            returnList = path(tileStart, tile, creature, creature->getSeat(), false);
            continue;
        }

        // We compute the path only if walkableDist < (dist * magic).
        double walkableDist = static_cast<double>(returnList.size());
        if(walkableDist < (dist * magic))
            continue;

        std::list<Tile*> pathTmp = path(tileStart, tile, creature, creature->getSeat(), false);
        if(pathTmp.size() < returnList.size())
        {
            // The path is shorter
            chosenTile = tile;
            returnList = pathTmp;
        }
    }
    return returnList;
}

bool GameMap::pathExists(const Creature* creature, Tile* tileStart, Tile* tileEnd)
{
    // If floodfill is not enabled, we cannot check if the path exists so we return true
    if(!mFloodFillEnabled)
        return true;

    if(creature == nullptr || !creature->canGoThroughTile(tileStart) || !creature->canGoThroughTile(tileEnd))
        return false;

    if((creature->getMoveSpeedGround() > 0.0) &&
        (creature->getMoveSpeedWater() > 0.0) &&
        (creature->getMoveSpeedLava() > 0.0))
    {
        return tileStart->isSameFloodFill(FloodFillType::groundWaterLava, tileEnd);
    }
    if((creature->getMoveSpeedGround() > 0.0) &&
        (creature->getMoveSpeedWater() > 0.0))
    {
        return tileStart->isSameFloodFill(FloodFillType::groundWater, tileEnd);
    }
    if((creature->getMoveSpeedGround() > 0.0) &&
        (creature->getMoveSpeedLava() > 0.0))
    {
        return tileStart->isSameFloodFill(FloodFillType::groundLava, tileEnd);
    }

    return tileStart->isSameFloodFill(FloodFillType::ground, tileEnd);
}

std::list<Tile*> GameMap::path(int x1, int y1, int x2, int y2, const Creature* creature, Seat* seat, bool throughDiggableTiles)
{
    ++mNumCallsTo_path;
    std::list<Tile*> returnList;

    // If the start tile was not found return an empty path
    Tile* start = getTile(x1, y1);
    if (start == nullptr)
        return returnList;

    // If the end tile was not found return an empty path
    Tile* destination = getTile(x2, y2);
    if (destination == nullptr)
        return returnList;

    if (creature == nullptr)
        return returnList;

    // If flood filling is enabled, we can possibly eliminate this path by checking to see if they two tiles are floodfilled differently.
    if (!throughDiggableTiles && !pathExists(creature, start, destination))
        return returnList;

    AstarEntry *currentEntry = new AstarEntry(getTile(x1, y1), x1, y1, x2, y2);
    AstarEntry neighbor;

    std::vector<AstarEntry*> openList;
    openList.push_back(currentEntry);

    std::vector<AstarEntry*> closedList;
    AstarEntry* destinationEntry = nullptr;
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
            Tile* neighborTile = nullptr;
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
                default:
                    break;
            }
            if(neighborTile == nullptr)
                continue;

            neighbor.setTile(neighborTile);

            bool processNeighbor = false;
            if(creature->canGoThroughTile(neighbor.getTile()))
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
            AstarEntry* neighborEntry = nullptr;
            for(std::vector<AstarEntry*>::iterator itr = closedList.begin(); itr != closedList.end(); ++itr)
            {
                if (neighbor.getTile() == (*itr)->getTile())
                {
                    neighborEntry = *itr;
                    break;
                }
            }

            // Ignore the neighbor if it is on the closed list
            if (neighborEntry != nullptr)
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
            if (neighborEntry == nullptr)
            {
                double weightToParent = AstarEntry::computeHeuristic(neighbor.getTile()->getX(), neighbor.getTile()->getY(),
                    currentEntry->getTile()->getX(), currentEntry->getTile()->getY());

                if(currentEntry->getTile()->getFullness() == 0)
                    weightToParent /= creature->getMoveSpeed(currentEntry->getTile());
                else
                    weightToParent /= creature->getMoveSpeedGround();
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

                if(currentEntry->getTile()->getFullness() == 0)
                    weightToParent /= creature->getMoveSpeed(currentEntry->getTile());
                else
                    weightToParent /= creature->getMoveSpeedGround();

                if (currentEntry->getG() + weightToParent < neighborEntry->getG())
                {
                    neighborEntry->setG(currentEntry->getG() + weightToParent);
                    neighborEntry->setParent(currentEntry);
                }
            }
        }
    }

    if (destinationEntry != nullptr)
    {
        // Follow the parent chain back the the starting tile
        AstarEntry* curEntry = destinationEntry;
        do
        {
            if (curEntry->getTile() != nullptr)
            {
                returnList.push_front(curEntry->getTile());
                curEntry = curEntry->getParent();
            }

        } while (curEntry != nullptr);
    }

    // Clean up the memory we allocated by deleting the astarEntries in the open and closed lists
    for (std::vector<AstarEntry*>::iterator itr = openList.begin(); itr != openList.end(); ++itr)
        delete *itr;

    for (std::vector<AstarEntry*>::iterator itr = closedList.begin(); itr != closedList.end(); ++itr)
        delete *itr;

    return returnList;
}

bool GameMap::addPlayer(Player* player)
{
    mPlayers.push_back(player);
    LogManager::getSingleton().logMessage(serverStr() + "Added player: " + player->getNick());
    return true;
}

bool GameMap::assignAI(Player& player, const std::string& aiType, const std::string& parameters)
{
    if (mAiManager.assignAI(player, aiType, parameters))
    {
        LogManager::getSingleton().logMessage("Assign AI: " + aiType + ", to player: " + player.getNick());
        return true;
    }

    LogManager::getSingleton().logMessage("Couldn't assign AI: " + aiType + ", to player: " + player.getNick());
    return false;
}

Player* GameMap::getPlayer(const std::string& pName) const
{
    for (Player* player : mPlayers)
    {
        if (player->getNick().compare(pName) == 0)
        {
            return player;
        }
    }

    return nullptr;
}

Player* GameMap::getPlayerBySeatId(int seatId) const
{
    for (Player* player : mPlayers)
    {
        if(player->getSeat()->getId() == seatId)
            return player;
    }
    return nullptr;
}

Player* GameMap::getPlayerBySeat(Seat* seat) const
{
    for (Player* player : mPlayers)
    {
        if(player->getSeat() == seat)
            return player;
    }
    return nullptr;
}

std::vector<GameEntity*> GameMap::getVisibleForce(const std::vector<Tile*>& visibleTiles, Seat* seat, bool enemyForce)
{
    std::vector<GameEntity*> returnList;

    // Loop over the visible tiles
    for (Tile* tile : visibleTiles)
    {
        OD_ASSERT_TRUE(tile != nullptr);
        if(tile == nullptr)
            continue;

        tile->fillWithAttackableCreatures(returnList, seat, enemyForce);
        tile->fillWithAttackableRoom(returnList, seat, enemyForce);
        tile->fillWithAttackableTrap(returnList, seat, enemyForce);
    }

    return returnList;
}

std::vector<GameEntity*> GameMap::getVisibleCreatures(const std::vector<Tile*>& visibleTiles, Seat* seat, bool enemyCreatures)
{
    std::vector<GameEntity*> returnList;

    // Loop over the visible tiles
    for (Tile* tile : visibleTiles)
    {
        OD_ASSERT_TRUE(tile != nullptr);
        if(tile == nullptr)
            continue;

        tile->fillWithAttackableCreatures(returnList, seat, enemyCreatures);
    }

    return returnList;
}

std::vector<GameEntity*> GameMap::getVisibleCarryableEntities(const std::vector<Tile*>& visibleTiles)
{
    std::vector<GameEntity*> returnList;

    // Loop over the visible tiles
    for (Tile* tile : visibleTiles)
    {
        OD_ASSERT_TRUE(tile != nullptr);
        if(tile == nullptr)
            continue;

        tile->fillWithCarryableEntities(returnList);
    }

    return returnList;
}

void GameMap::clearRooms()
{
    // We need to work on a copy of mRooms because removeFromGameMap will remove them from this vector
    std::vector<Room*> rooms = mRooms;
    for (Room *tempRoom : rooms)
    {
        tempRoom->removeFromGameMap();
        tempRoom->deleteYourself();
    }

    mRooms.clear();
}

void GameMap::addRoom(Room *r)
{
    int nbTiles = r->numCoveredTiles();
    LogManager::getSingleton().logMessage(serverStr() + "Adding room " + r->getName() + ", nbTiles="
        + Ogre::StringConverter::toString(nbTiles) + ", seatId=" + Ogre::StringConverter::toString(r->getSeat()->getId()));
    for(Tile* tile : r->getCoveredTiles())
    {
        LogManager::getSingleton().logMessage(serverStr() + "Adding room " + r->getName() + ", tile=" + Tile::displayAsString(tile));
    }

    mRooms.push_back(r);
}

void GameMap::removeRoom(Room *r)
{
    LogManager::getSingleton().logMessage(serverStr() + "Removing room " + r->getName());
    // Rooms are removed when absorbed by another room or when they have no more tile
    // In both cases, the client have enough information to do that alone so no need to notify him
    std::vector<Room*>::iterator it = std::find(mRooms.begin(), mRooms.end(), r);
    OD_ASSERT_TRUE_MSG(it != mRooms.end(), "Room name=" + r->getName());
    if(it == mRooms.end())
        return;

    mRooms.erase(it);
}

std::vector<Room*> GameMap::getRoomsByType(RoomType type)
{
    std::vector<Room*> returnList;
    for (Room* room : mRooms)
    {
        if (room->getType() == type  && room->getHP(nullptr) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

std::vector<Room*> GameMap::getRoomsByTypeAndSeat(RoomType type, Seat* seat)
{
    std::vector<Room*> returnList;
    for (Room* room : mRooms)
    {
        if (room->getType() == type && room->getSeat() == seat && room->getHP(nullptr) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

std::vector<const Room*> GameMap::getRoomsByTypeAndSeat(RoomType type, Seat* seat) const
{
    std::vector<const Room*> returnList;
    for (const Room* room : mRooms)
    {
        if (room->getType() == type && room->getSeat() == seat && room->getHP(nullptr) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

unsigned int GameMap::numRoomsByTypeAndSeat(RoomType type, Seat* seat) const
{
    int cptRooms = 0;
    for (Room* room : mRooms)
    {
        if (room->getType() == type && room->getSeat() == seat && room->getHP(nullptr) > 0.0)
            ++cptRooms;
    }
    return cptRooms;
}

std::vector<Room*> GameMap::getReachableRooms(const std::vector<Room*>& vec,
                                              Tile* startTile,
                                              const Creature* creature)
{
    std::vector<Room*> returnVector;

    for (unsigned int i = 0; i < vec.size(); ++i)
    {
        Room* room = vec[i];
        Tile* coveredTile = room->getCoveredTile(0);
        if (pathExists(creature, startTile, coveredTile))
        {
            returnVector.push_back(room);
        }
    }

    return returnVector;
}

std::vector<Building*> GameMap::getReachableBuildingsPerSeat(Seat* seat,
       Tile *startTile, const Creature* creature)
{
    std::vector<Building*> returnList;
    for (Room* room : mRooms)
    {
        if (room->getSeat() != seat)
            continue;

        if (room->getHP(nullptr) <= 0.0)
            continue;

        if(!pathExists(creature, startTile, room->getCoveredTile(0)))
            continue;

        returnList.push_back(room);
    }

    for (Trap* trap : mTraps)
    {
        if (trap->getSeat() != seat)
            continue;

        if (trap->getHP(nullptr) <= 0.0)
            continue;

        if(!pathExists(creature, startTile, trap->getCoveredTile(0)))
            continue;

        returnList.push_back(trap);
    }

    return returnList;
}

Room* GameMap::getRoomByName(const std::string& name)
{
    for (Room* room : mRooms)
    {
        if(room->getName().compare(name) == 0)
            return room;
    }

    return nullptr;
}

Trap* GameMap::getTrapByName(const std::string& name)
{
    for (Trap* trap : mTraps)
    {
        if(trap->getName().compare(name) == 0)
            return trap;
    }

    return nullptr;
}

void GameMap::clearTraps()
{
    // We need to work on a copy of mTraps because removeFromGameMap will remove them from this vector
    std::vector<Trap*> traps = mTraps;
    for (Trap* trap : traps)
    {
        trap->removeFromGameMap();
        trap->deleteYourself();
    }

    mTraps.clear();
}

void GameMap::addTrap(Trap *trap)
{
    int nbTiles = trap->numCoveredTiles();
    LogManager::getSingleton().logMessage(serverStr() + "Adding trap " + trap->getName() + ", nbTiles="
        + Ogre::StringConverter::toString(nbTiles) + ", seatId=" + Ogre::StringConverter::toString(trap->getSeat()->getId()));

    mTraps.push_back(trap);
}

void GameMap::removeTrap(Trap *t)
{
    LogManager::getSingleton().logMessage(serverStr() + "Removing trap " + t->getName());
    std::vector<Trap*>::iterator it = std::find(mTraps.begin(), mTraps.end(), t);
    OD_ASSERT_TRUE_MSG(it != mTraps.end(), "Trap name=" + t->getName());
    if(it == mTraps.end())
        return;

    mTraps.erase(it);
}

int GameMap::getTotalGoldForSeat(Seat* seat)
{
    int tempInt = 0;
    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndSeat(RoomType::treasury, seat);
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
    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndSeat(RoomType::treasury, seat);
    for (unsigned int i = 0; i < treasuriesOwned.size() && goldStillNeeded > 0; ++i)
    {
        goldStillNeeded -= static_cast<RoomTreasury*>(treasuriesOwned[i])->withdrawGold(goldStillNeeded);
    }

    return true;
}

void GameMap::clearMapLights()
{
    // We need to work on a copy of mMapLights because removeFromGameMap will remove them from this vector
    std::vector<MapLight*> mapLights = mMapLights;
    for (MapLight* mapLight : mapLights)
    {
        mapLight->removeFromGameMap();
        mapLight->deleteYourself();
    }

    mMapLights.clear();
}

void GameMap::addMapLight(MapLight *m)
{
    LogManager::getSingleton().logMessage(serverStr() + "Adding MapLight " + m->getName());
    mMapLights.push_back(m);
}

void GameMap::removeMapLight(MapLight *m)
{
    LogManager::getSingleton().logMessage(serverStr() + "Removing MapLight " + m->getName());

    std::vector<MapLight*>::iterator it = std::find(mMapLights.begin(), mMapLights.end(), m);
    OD_ASSERT_TRUE_MSG(it != mMapLights.end(), "MapLight name=" + m->getName());
    if(it == mMapLights.end())
        return;

    mMapLights.erase(it);
}

MapLight* GameMap::getMapLight(const std::string& name) const
{
    for (MapLight* mapLight : mMapLights)
    {
        if (mapLight->getName() == name)
            return mapLight;
    }

    return nullptr;
}

void GameMap::clearSeats()
{
    for (Seat* seat : mSeats)
    {
        delete seat;
    }
    mSeats.clear();
}

bool GameMap::addSeat(Seat *s)
{
    OD_ASSERT_TRUE(s != nullptr);
    if (s == nullptr)
        return false;

    for(Seat* seat : mSeats)
    {
        if(seat->getId() == s->getId())
        {
            OD_ASSERT_TRUE_MSG(false, "Duplicated seat id=" + Helper::toString(seat->getId()));
            return false;
        }
    }
    s->setIndex(mSeats.size());
    mSeats.push_back(s);

    // Add the goals for all seats to this seat.
    for (auto& goal : mGoalsForAllSeats)
    {
        s->addGoal(goal.get());
    }
    return true;
}

Seat* GameMap::getSeatById(int id) const
{
    for (Seat* seat : mSeats)
    {
        if (seat->getId() == id)
            return seat;
    }

    return nullptr;
}

void GameMap::addWinningSeat(Seat *s)
{
    // Make sure the seat has not already been added.
    if(std::find(mWinningSeats.begin(), mWinningSeats.end(), s) != mWinningSeats.end())
        return;

    Player* player = getPlayerBySeat(s);
    if (player && player->getIsHuman())
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, player);
        serverNotification->mPacket << "You Won";
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }

    mWinningSeats.push_back(s);
}

bool GameMap::seatIsAWinner(Seat *s) const
{
    if(std::find(mWinningSeats.begin(), mWinningSeats.end(), s) != mWinningSeats.end())
        return true;

    return false;
}

void GameMap::addGoalForAllSeats(std::unique_ptr<Goal>&& g)
{
    // Add the goal to each of the empty seats currently in the game.
    for (Seat* seat : mSeats)
        seat->addGoal(g.get());

    mGoalsForAllSeats.emplace_back(std::move(g));
}

void GameMap::clearGoalsForAllSeats()
{
    for (Seat* seat : mSeats)
    {
        seat->clearUncompleteGoals();
        seat->clearCompletedGoals();
    }

    mGoalsForAllSeats.clear();
}

Ogre::Real GameMap::crowDistance(Tile *t1, Tile *t2)
{
    if (t1 != nullptr && t2 != nullptr)
        return crowDistance(t1->getX(), t2->getX(), t1->getY(), t2->getY());
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
    if (!mFloodFillEnabled)
        return false;

    if(tile->isFloodFillFilled())
        return false;

    bool hasChanged = false;
    // If a neigboor is colored with the same colors, we color the tile
    for(Tile* neigh : tile->getAllNeighbors())
    {
        switch(neigh->getType())
        {
            case TileType::dirt:
            case TileType::gold:
            case TileType::rock:
            {
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::ground, neigh);
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundWater, neigh);
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundLava, neigh);
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundWaterLava, neigh);
                break;
            }
            case TileType::water:
            {
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundWater, neigh);
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundWaterLava, neigh);
                break;
            }
            case TileType::lava:
            {
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundLava, neigh);
                hasChanged |= tile->updateFloodFillFromTile(FloodFillType::groundWaterLava, neigh);
                break;
            }
            default:
                continue;
        }

        // If the tile is fully filled, no need to continue
        if(tile->isFloodFillFilled())
            return true;
    }

    return hasChanged;
}

void GameMap::replaceFloodFill(FloodFillType floodFillType, int colorOld, int colorNew)
{
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            Tile* tile = getTile(ii,jj);
            if(tile->floodFillValue(floodFillType) != colorOld)
                continue;

            tile->replaceFloodFill(floodFillType, colorNew);
        }
    }
}

void GameMap::refreshFloodFill(Tile* tile)
{
    std::vector<int> colors(Tile::toUInt32(FloodFillType::nbValues), -1);

    // If the tile has opened a new place, we use the same floodfillcolor for all the areas
    for(Tile* neigh : tile->getAllNeighbors())
    {
        for(uint32_t i = 0; i < colors.size(); ++i)
        {
            FloodFillType type = static_cast<FloodFillType>(i);
            if(colors[i] == -1)
            {
                colors[i] = neigh->floodFillValue(type);
                tile->updateFloodFillFromTile(type, neigh);
            }
            else if((colors[i] != -1) &&
               (neigh->floodFillValue(type) != -1) &&
               (neigh->floodFillValue(type) != colors[i]))
            {
                replaceFloodFill(type, neigh->floodFillValue(type), colors[i]);
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
            getTile(ii,jj)->resetFloodFill();
        }
    }

    // The algorithm used to find a path is efficient when the path exists but not if it doesn't.
    // To improve path finding, we tag the contiguous tiles to know if a path exists between 2 tiles or not.
    // Because creatures can go through ground, water or lava, we process all of theses.
    // Note : when a tile is digged, floodfill will have to be refreshed.
    mFloodFillEnabled = true;

    // To optimize floodfilling, we start by tagging the dirt tiles with fullness = 0
    // because they are walkable for most creatures. When we will have tagged all
    // thoses, we will deal with water/lava remaining (there can be some left if
    // surrounded by not passable tiles).
    FloodFillType currentType = FloodFillType::ground;
    int floodFillValue = 0;
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
                if(tile->getFullness() > 0.0)
                    continue;

                if(currentType == FloodFillType::ground)
                {
                    if(((tile->getType() == TileType::dirt) ||
                        (tile->getType() == TileType::gold) ||
                        (tile->getType() == TileType::rock)) &&
                       (tile->floodFillValue(FloodFillType::ground) == -1))
                    {
                        isTileFound = true;
                        if(tile->floodFillValue(FloodFillType::ground) == -1)
                            tile->replaceFloodFill(FloodFillType::ground, ++floodFillValue);
                        if(tile->floodFillValue(FloodFillType::groundWater) == -1)
                            tile->replaceFloodFill(FloodFillType::groundWater, ++floodFillValue);
                        if(tile->floodFillValue(FloodFillType::groundLava) == -1)
                            tile->replaceFloodFill(FloodFillType::groundLava, ++floodFillValue);
                        if(tile->floodFillValue(FloodFillType::groundWaterLava) == -1)
                            tile->replaceFloodFill(FloodFillType::groundWaterLava, ++floodFillValue);
                        break;
                    }
                }
                else if(currentType == FloodFillType::groundWater)
                {
                    if((tile->getType() == TileType::water) &&
                       (tile->floodFillValue(FloodFillType::groundWater) == -1))
                    {
                        isTileFound = true;
                        if(tile->floodFillValue(FloodFillType::groundWater) == -1)
                            tile->replaceFloodFill(FloodFillType::groundWater, ++floodFillValue);
                        if(tile->floodFillValue(FloodFillType::groundWaterLava) == -1)
                            tile->replaceFloodFill(FloodFillType::groundWaterLava, ++floodFillValue);
                        break;
                    }
                }
                else if(currentType == FloodFillType::groundLava)
                {
                    if((tile->getType() == TileType::lava) &&
                       (tile->floodFillValue(FloodFillType::groundLava) == -1))
                    {
                        isTileFound = true;
                        if(tile->floodFillValue(FloodFillType::groundLava) == -1)
                            tile->replaceFloodFill(FloodFillType::groundLava, ++floodFillValue);
                        if(tile->floodFillValue(FloodFillType::groundWaterLava) == -1)
                            tile->replaceFloodFill(FloodFillType::groundWaterLava, ++floodFillValue);
                        break;
                    }
                }
            }

            if(!isTileFound)
                ++yy;
        }

        // If there are no more walkable tiles, we go for water. Then, for lava. After that, floodfill should be complete
        if(!isTileFound)
        {
            switch(currentType)
            {
                case FloodFillType::ground:
                {
                    // There are no more ground tiles. We go for water tiles
                    currentType = FloodFillType::groundWater;
                    isTileFound = true;
                    break;
                }
                case FloodFillType::groundWater:
                {
                    // There are no more ground tiles. We go for water tiles
                    currentType = FloodFillType::groundLava;
                    isTileFound = true;
                    break;
                }
                case FloodFillType::groundLava:
                {
                    // There are no more tiles. We can stop
                    break;
                }
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unexpected enum value=" + Tile::toString(currentType));
                    break;
            }
        }

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
                for(int xx = getMapSizeX() - 1; xx >= 0; --xx)
                {
                    Tile* tile = getTile(xx, yy);
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

std::list<Tile*> GameMap::path(Creature *c1, Creature *c2, const Creature* creature, Seat* seat, bool throughDiggableTiles)
{
    return path(c1->getPositionTile()->getX(), c1->getPositionTile()->getY(),
                c2->getPositionTile()->getX(), c2->getPositionTile()->getY(), creature, seat, throughDiggableTiles);
}

std::list<Tile*> GameMap::path(Tile *t1, Tile *t2, const Creature* creature, Seat* seat, bool throughDiggableTiles)
{
    return path(t1->getX(), t1->getY(), t2->getX(), t2->getY(), creature, seat, throughDiggableTiles);
}

std::list<Tile*> GameMap::path(const Creature* creature, Tile* destination, bool throughDiggableTiles)
{
    if (destination == nullptr)
        return std::list<Tile*>();

    Tile* positionTile = creature->getPositionTile();
    if (positionTile == nullptr)
        return std::list<Tile*>();

    return path(positionTile->getX(), positionTile->getY(),
                destination->getX(), destination->getY(),
                creature, creature->getSeat(), throughDiggableTiles);
}

Ogre::Real GameMap::crowDistance(Creature *c1, Creature *c2)
{
    //TODO:  This is sub-optimal, improve it.
    Tile* tempTile1 = c1->getPositionTile();
    Tile* tempTile2 = c2->getPositionTile();
    return crowDistance(tempTile1->getX(), tempTile1->getY(), tempTile2->getX(), tempTile2->getY());
}

Ogre::Real GameMap::squaredCrowDistance(Tile *t1, Tile *t2) const
{
    return std::pow(static_cast<Ogre::Real>(t2->getX() - t1->getX()), 2.0f)
        + std::pow(static_cast<Ogre::Real>(t2->getY() - t1->getY()), 2.0f);
}

void GameMap::processDeletionQueues()
{
    while (!mEntitiesToDelete.empty())
    {
        GameEntity* entity = *mEntitiesToDelete.begin();
        mEntitiesToDelete.erase(mEntitiesToDelete.begin());
        delete entity;
    }
}

void GameMap::processActiveObjectsChanges()
{
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
}

void GameMap::refreshBorderingTilesOf(const std::vector<Tile*>& affectedTiles)
{
    // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
    std::vector<Tile*> borderTiles = tilesBorderedByRegion(affectedTiles);

    borderTiles.insert(borderTiles.end(), affectedTiles.begin(), affectedTiles.end());

    // Loop over all the affected tiles and force them to examine their neighbors.  This allows
    // them to switch to a mesh with fewer polygons if some are hidden by the neighbors, etc.
    for (Tile* tile : borderTiles)
        tile->refreshMesh();
}

std::vector<Tile*> GameMap::getBuildableTilesForPlayerInArea(int x1, int y1, int x2, int y2,
    Player* player)
{
    std::vector<Tile*> tiles = rectangularRegion(x1, y1, x2, y2);
    for (std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end();)
    {
        Tile* tile = *it;
        if (!tile->isBuildableUpon(player->getSeat()))
        {
            it = tiles.erase(it);
            continue;
        }

        ++it;
    }
    return tiles;
}

std::string GameMap::getGoalsStringForPlayer(Player* player)
{
    bool playerIsAWinner = seatIsAWinner(player->getSeat());
    std::stringstream tempSS("");
    Seat* seat = player->getSeat();
    seat->resetGoalsChanged();

    const std::string formatTitleOn = "[font='MedievalSharp-12'][colour='CCBBBBFF']";
    const std::string formatTitleOff = "[font='MedievalSharp-10'][colour='FFFFFFFF']";

    if (playerIsAWinner)
    {
        tempSS << "Congratulations, you have completed this level.";
    }
    else if (seat->numFailedGoals() > 0)
    {
        // Loop over the list of completed goals for the seat we are sitting in an print them.
        tempSS << formatTitleOn << "Failed Goals:\n" << formatTitleOff
               << "(You cannot complete this level!)\n\n";
        for (unsigned int i = 0; i < seat->numFailedGoals(); ++i)
        {
            Goal *tempGoal = seat->getFailedGoal(i);
            tempSS << tempGoal->getFailedMessage(seat) << "\n";
        }
    }

    if (seat->numUncompleteGoals() > 0)
    {
        // Loop over the list of unmet goals for the seat we are sitting in an print them.
        tempSS << formatTitleOn << "Unfinished Goals:" << formatTitleOff << "\n\n";
        for (unsigned int i = 0; i < seat->numUncompleteGoals(); ++i)
        {
            Goal *tempGoal = seat->getUncompleteGoal(i);
            tempSS << tempGoal->getDescription(seat) << "\n";
        }
    }

    if (seat->numCompletedGoals() > 0)
    {
        // Loop over the list of completed goals for the seat we are sitting in an print them.
        tempSS << "\n" << formatTitleOn << "Completed Goals:" << formatTitleOff << "\n\n";
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
    if(seat == nullptr)
        return gold;

    std::vector<Room*> treasuriesOwned = getRoomsByTypeAndSeat(RoomType::treasury, seat);
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
    for(Seat* seat : mSeats)
    {
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
    } while(getCreature(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameRoom(const std::string& meshName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberRoom;
        ret = meshName + Ogre::StringConverter::toString(mUniqueNumberRoom);
    } while(getRoomByName(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameRenderedMovableEntity(const std::string& baseName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberRenderedMovableEntity;
        ret = RenderedMovableEntity::RENDEREDMOVABLEENTITY_PREFIX + baseName + "_" + Ogre::StringConverter::toString(mUniqueNumberRenderedMovableEntity);
    } while(getRenderedMovableEntity(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameTrap(const std::string& meshName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberTrap;
        ret = meshName + "_" + Ogre::StringConverter::toString(mUniqueNumberTrap);
    } while(getTrapByName(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameMapLight()
{
    std::string ret;
    do
    {
        ++mUniqueNumberMapLight;
        ret = MapLight::MAPLIGHT_NAME_PREFIX + Ogre::StringConverter::toString(mUniqueNumberMapLight);
    } while(getMapLight(ret) != nullptr);
    return ret;
}

GameEntity* GameMap::getEntityFromTypeAndName(GameEntityType entityType,
    const std::string& entityName)
{
    switch(entityType)
    {
        case GameEntityType::creature:
            return getCreature(entityName);

        case GameEntityType::buildingObject:
        case GameEntityType::chickenEntity:
        case GameEntityType::craftedTrap:
        case GameEntityType::missileObject:
        case GameEntityType::persistentObject:
        case GameEntityType::smallSpiderEntity:
        case GameEntityType::trapEntity:
        case GameEntityType::treasuryObject:
        case GameEntityType::researchEntity:
            return getRenderedMovableEntity(entityName);

        case GameEntityType::spell:
            return getSpell(entityName);

        case GameEntityType::mapLight:
            return getMapLight(entityName);

        case GameEntityType::tile:
        {
            int x, y;
            if(!Tile::checkTileName(entityName, x, y))
                return nullptr;

            return getTile(x, y);
        }

        default:
            break;
    }

    return nullptr;
}

void GameMap::logFloodFileTiles()
{
    for(int yy = 0; yy < getMapSizeY(); ++yy)
    {
        for(int xx = 0; xx < getMapSizeX(); ++xx)
        {
            Tile* tile = getTile(xx, yy);
            tile->logFloodFill();
        }
    }
}

void GameMap::consoleSetCreatureDestination(const std::string& creatureName, int x, int y)
{
    Creature* creature = getCreature(creatureName);
    if(creature == nullptr)
        return;
    Tile* tile = getTile(x, y);
    if(tile == nullptr)
        return;
    if(creature->getPositionTile() == nullptr)
        return;
    creature->clearActionQueue();
    creature->clearDestinations();
    creature->setDestination(tile);
}

void GameMap::consoleDisplayCreatureVisualDebug(const std::string& creatureName, bool enable)
{
    Creature* creature = getCreature(creatureName);
    if(creature == nullptr)
        return;

    if(enable)
        creature->computeVisualDebugEntities();
    else
        creature->stopComputeVisualDebugEntities();
}

void GameMap::consoleDisplaySeatVisualDebug(int seatId, bool enable)
{
    Seat* seat = getSeatById(seatId);
    if(seat == nullptr)
        return;

    seat->displaySeatVisualDebug(enable);
}

void GameMap::consoleSetLevelCreature(const std::string& creatureName, uint32_t level)
{
    Creature* creature = getCreature(creatureName);
    if(creature == nullptr)
        return;

    creature->setLevel(level);
}

void GameMap::consoleAskToggleFOW()
{
    mIsFOWActivated = !mIsFOWActivated;
}

Creature* GameMap::getWorkerForPathFinding(Seat* seat)
{
    for (Creature* creature : mCreatures)
    {
        if(creature->getSeat() != seat)
            continue;

        if (creature->getDefinition()->isWorker())
            return creature;
    }
    return nullptr;
}

bool GameMap::pathToBestFightingPosition(std::list<Tile*>& pathToTarget, Creature* attackingCreature,
    Tile* attackedTile)
{
    // First, we search the tiles from where we can attack as far as possible
    Tile* tileCreature = attackingCreature->getPositionTile();
    if((tileCreature == nullptr) || (attackedTile == nullptr))
        return false;

    double range = attackingCreature->getBestAttackRange();

    std::vector<Tile*> possibleTiles;
    while(possibleTiles.empty() && range >= 0)
    {
        for(int i = -range; i <= range; ++i)
        {
            int diffY = range - std::abs(i);
            Tile* tile;
            tile = getTile(attackedTile->getX() + i, attackedTile->getY() + diffY);
            if(tile != nullptr && pathExists(attackingCreature, tileCreature, tile))
                possibleTiles.push_back(tile);

            if(diffY == 0)
                continue;

            tile = getTile(attackedTile->getX() + i, attackedTile->getY() - diffY);
            if(tile != nullptr && pathExists(attackingCreature, tileCreature, tile))
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

    pathToTarget = path(attackingCreature, closestTile);
    return true;
}

GameEntity* GameMap::getClosestTileWhereGameEntityFromList(std::vector<GameEntity*> listObjects, Tile* origin, Tile*& attackedTile)
{
    if(listObjects.empty())
        return nullptr;

    GameEntity* closestGameEntity = nullptr;
    double shortestDist = 0.0;
    for(std::vector<GameEntity*>::iterator itObj = listObjects.begin(); itObj != listObjects.end(); ++itObj)
    {
        GameEntity* gameEntity = *itObj;
        std::vector<Tile*> tiles = gameEntity->getCoveredTiles();
        for(std::vector<Tile*>::iterator itTile = tiles.begin(); itTile != tiles.end(); ++itTile)
        {
            Tile* tile = *itTile;
            OD_ASSERT_TRUE(tile != nullptr);
            double dist = std::pow(static_cast<double>(std::abs(tile->getX() - origin->getX())), 2);
            dist += std::pow(static_cast<double>(std::abs(tile->getY() - origin->getY())), 2);
            if(closestGameEntity == nullptr || dist < shortestDist)
            {
                shortestDist = dist;
                closestGameEntity = gameEntity;
                attackedTile = tile;
            }
        }
    }

    return closestGameEntity;
}

void GameMap::fillBuildableTilesAndPriceForPlayerInArea(int x1, int y1, int x2, int y2,
    Player* player, RoomType type, std::vector<Tile*>& tiles, int& goldRequired)
{
    goldRequired = 0;
    tiles = getBuildableTilesForPlayerInArea(x1, y1, x2, y2, player);

    if(tiles.empty())
        return;

    int costPerTile = Room::costPerTile(type);
    goldRequired = tiles.size() * costPerTile;

    // The first treasury tile doesn't cost anything to prevent a player from being stuck
    // without any means to get gold.
    // Thus, we check whether it is the current attempt and we remove the cost of one tile.
    if (type == RoomType::treasury
            && numRoomsByTypeAndSeat(RoomType::treasury, player->getSeat()) == 0)
    {
        goldRequired -= costPerTile;
    }
}

void GameMap::updateVisibleEntities()
{
    // Notify what happened to entities on visible tiles
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            Tile* tile = getTile(ii,jj);
            tile->notifyEntitiesSeatsWithVision();
        }
    }

    // Notify changes on visible tiles
    for(Seat* seat : mSeats)
        seat->notifyChangedVisibleTiles();

    for(Creature* creature : mCreatures)
    {
        creature->fireCreatureRefreshIfNeeded();
    }
}

void GameMap::clearCreatureMoodModifiers()
{
    // We delete map specific mood modifiers
    for(std::pair<const std::string, std::vector<CreatureMood*>>& p : mCreatureMoodModifiers)
    {
        for(const CreatureMood* creatureMood : p.second)
            delete creatureMood;
    }

    mCreatureMoodModifiers.clear();
}

bool GameMap::addCreatureMoodModifiers(const std::string& name,
    const std::vector<CreatureMood*>& moodModifiers)
{
    uint32_t nb = mCreatureMoodModifiers.count(name);
    OD_ASSERT_TRUE_MSG(nb <= 0, "Duplicate mood modifier=" + name);
    if(nb > 0)
        return false;

    mCreatureMoodModifiers[name] = moodModifiers;
    return true;
}

int32_t GameMap::computeCreatureMoodModifiers(const Creature* creature) const
{
    const std::string& moodModifierName = creature->getDefinition()->getMoodModifierName();
    if(mCreatureMoodModifiers.count(moodModifierName) <= 0)
        return 0;

    int32_t moodValue = 0;
    const std::vector<CreatureMood*>& moodModifiers = mCreatureMoodModifiers.at(moodModifierName);
    for(const CreatureMood* mood : moodModifiers)
    {
        moodValue += mood->computeMood(creature);
    }

    return moodValue;
}

std::vector<GameEntity*> GameMap::getNaturalEnemiesInList(const Creature* creature, const std::vector<GameEntity*>& reachableAlliedObjects) const
{
    std::vector<GameEntity*> ret;

    const std::string& moodModifierName = creature->getDefinition()->getMoodModifierName();
    if(mCreatureMoodModifiers.count(moodModifierName) <= 0)
        return ret;

    const std::vector<CreatureMood*>& moodModifiers = mCreatureMoodModifiers.at(moodModifierName);
    for(GameEntity* entity : reachableAlliedObjects)
    {
        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        Creature* alliedCreature = static_cast<Creature*>(entity);
        for(const CreatureMood* mood : moodModifiers)
        {
            if(mood->getCreatureMoodType() != CreatureMoodType::creature)
                continue;

            const CreatureMoodCreature* moodCreature = static_cast<const CreatureMoodCreature*>(mood);
            if(alliedCreature->getDefinition() == moodCreature->getCreatureDefinition())
            {
                ret.push_back(entity);
                break;
            }
        }
    }

    return ret;
}

void GameMap::addSpell(Spell *spell)
{
    LogManager::getSingleton().logMessage(serverStr() + "Adding spell " + spell->getName()
        + ",MeshName=" + spell->getMeshName());
    mSpells.push_back(spell);
}

void GameMap::removeSpell(Spell *spell)
{
    LogManager::getSingleton().logMessage(serverStr() + "Removing spell " + spell->getName()
        + ",MeshName=" + spell->getMeshName());
    std::vector<Spell*>::iterator it = std::find(mSpells.begin(), mSpells.end(), spell);
    OD_ASSERT_TRUE_MSG(it != mSpells.end(), "spell name=" + spell->getName());
    if(it == mSpells.end())
        return;

    mSpells.erase(it);
}

Spell* GameMap::getSpell(const std::string& name) const
{
    for(Spell* spell : mSpells)
    {
        if(name.compare(spell->getName()) == 0)
            return spell;
    }
    return nullptr;
}

void GameMap::clearSpells()
{
    // We need to work on a copy of mSpells because removeFromGameMap will remove them from this vector
    std::vector<Spell*> spells = mSpells;
    for (Spell* spell : spells)
    {
        spell->removeFromGameMap();
        spell->deleteYourself();
    }

    mSpells.clear();
}

std::vector<Spell*> GameMap::getSpellsBySeatAndType(Seat* seat, SpellType type) const
{
    std::vector<Spell*> ret;
    for (Spell* spell : mSpells)
    {
        if(spell->getSeat() != seat)
            continue;

        if(spell->getSpellType() != type)
            continue;

        ret.push_back(spell);
    }

    return ret;
}

const TileSetValue& GameMap::getMeshForTile(const Tile* tile) const
{
    OD_ASSERT_TRUE(!isServerGameMap());

    int index = 0;
    for(int i = 0; i < 4; ++i)
    {
        int diffX;
        int diffY;
        switch(i)
        {
            case 0:
                diffX = 0;
                diffY = -1;
                break;
            case 1:
                diffX = 1;
                diffY = 0;
                break;
            case 2:
                diffX = 0;
                diffY = 1;
                break;
            case 3:
            default:
                diffX = -1;
                diffY = 0;
                break;
        }
        const Tile* t = getTile(tile->getX() + diffX, tile->getY() + diffY);
        if(t == nullptr)
            continue;

        if(mTileSet->areLinked(tile, t))
            index |= (1 << i);
    }

    return mTileSet->getTileValues(tile->getTileVisual()).at(index);
}

uint32_t GameMap::getMaxNumberCreatures(Seat* seat) const
{
    uint32_t nbCreatures = ConfigManager::getSingleton().getMaxCreaturesPerSeatDefault();

    std::vector<const Room*> portals = getRoomsByTypeAndSeat(RoomType::portal, seat);
    for(const Room* room : portals)
    {
        const RoomPortal* roomPortal = static_cast<const RoomPortal*>(room);
        nbCreatures += roomPortal->getNbCreatureMaxIncrease();
    }

    return std::min(nbCreatures, ConfigManager::getSingleton().getMaxCreaturesPerSeatAbsolute());
}
