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
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/MapLight.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"
#include "game/Player.h"
#include "game/Research.h"
#include "game/Seat.h"
#include "gamemap/MapLoader.h"
#include "gamemap/Pathfinding.h"
#include "gamemap/TileSet.h"
#include "goals/Goal.h"
#include "modes/ModeManager.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/ODFrameListener.h"
#include "rooms/Room.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomPortal.h"
#include "rooms/RoomTreasury.h"
#include "rooms/RoomType.h"
#include "spells/Spell.h"
#include "sound/SoundEffectsManager.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/ResourceManager.h"

#include <OgreTimer.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

const std::string DEFAULT_NICK = "You";

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
}

std::string GameMap::serverStr()
{
    if (mIsServerGameMap)
        return std::string("SERVER - ");

    return std::string("CLIENT (" + getLocalPlayerNick() + ") - ");
}

bool GameMap::isInEditorMode() const
{
    if (isServerGameMap())
        return (ODServer::getSingleton().getServerMode() == ServerMode::ModeEditor);

    return (ODFrameListener::getSingleton().getModeManager()->getCurrentModeType() == ModeManager::EDITOR);
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
        OD_LOG_ERR("Couldn't add rogue seat");
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
    if (!allocateMapMemory(sizeX, sizeY))
        return false;

    for (int jj = 0; jj < mMapSizeY; ++jj)
    {
        for (int ii = 0; ii < mMapSizeX; ++ii)
        {
            Tile* tile = new Tile(this, isServerGameMap(), ii, jj);
            tile->setName(Tile::buildName(ii, jj));
            tile->setType(TileType::dirt);
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
    clearWeapons();
    clearTraps();

    clearMapLights();
    clearRooms();
    // NOTE : clearRenderedMovableEntities should be called after clearRooms because clearRooms will try to remove the objects from the room
    clearRenderedMovableEntities();
    clearSpells();
    clearTiles();
    clearCreatureMoodModifiers();

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

    processActiveObjectsChanges();
    processDeletionQueues();

    // We check if the different vectors are empty
    if(!mActiveObjects.empty())
    {
        OD_LOG_ERR("mActiveObjects not empty size=" + Helper::toString(static_cast<uint32_t>(mActiveObjects.size())));
        for(GameEntity* entity : mActiveObjects)
        {
            OD_LOG_ERR("entity not removed=" + entity->getName());
        }
        mActiveObjects.clear();
    }
    if(!mActiveObjectsToAdd.empty())
    {
        OD_LOG_ERR("mActiveObjectsToAdd not empty size=" + Helper::toString(static_cast<uint32_t>(mActiveObjectsToAdd.size())));
        for(GameEntity* entity : mActiveObjectsToAdd)
        {
            OD_LOG_ERR("entity not removed=" + entity->getName());
        }
        mActiveObjectsToAdd.clear();
    }
    if(!mActiveObjectsToRemove.empty())
    {
        OD_LOG_ERR("mActiveObjectsToRemove not empty size=" + Helper::toString(static_cast<uint32_t>(mActiveObjectsToRemove.size())));
        for(GameEntity* entity : mActiveObjectsToRemove)
        {
            OD_LOG_ERR("entity not removed=" + entity->getName());
        }
        mActiveObjectsToRemove.clear();
    }
    if(!mAnimatedObjects.empty())
    {
        OD_LOG_ERR("mAnimatedObjects not empty size=" + Helper::toString(static_cast<uint32_t>(mAnimatedObjects.size())));
        for(GameEntity* entity : mAnimatedObjects)
        {
            OD_LOG_ERR("entity not removed=" + entity->getName());
        }
        mAnimatedObjects.clear();
    }
    if(!mEntitiesToDelete.empty())
    {
        OD_LOG_ERR("mEntitiesToDelete not empty size=" + Helper::toString(static_cast<uint32_t>(mEntitiesToDelete.size())));
        for(GameEntity* entity : mEntitiesToDelete)
        {
            OD_LOG_ERR("entity not removed=" + entity->getName());
        }
        mEntitiesToDelete.clear();
    }
    if(!mGameEntityClientUpkeep.empty())
    {
        OD_LOG_ERR("mGameEntityClientUpkeep not empty size=" + Helper::toString(static_cast<uint32_t>(mGameEntityClientUpkeep.size())));
        for(GameEntity* entity : mGameEntityClientUpkeep)
        {
            OD_LOG_ERR("entity not removed=" + entity->getName());
        }
        mGameEntityClientUpkeep.clear();
    }
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
    mUniqueFloodFillValue = 0;
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
    if(index >= static_cast<int>(mWeapons.size()))
    {
        OD_LOG_ERR("index=" + Helper::toString(index));
        return nullptr;
    }

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
    OD_LOG_INF(serverStr() + "Adding Creature " + cc->getName()
        + ", seatId=" + (cc->getSeat() != nullptr ? Helper::toString(cc->getSeat()->getId()) : std::string("null")));

    mCreatures.push_back(cc);
}

void GameMap::removeCreature(Creature *c)
{
    OD_LOG_INF(serverStr() + "Removing Creature " + c->getName());

    std::vector<Creature*>::iterator it = std::find(mCreatures.begin(), mCreatures.end(), c);
    if(it == mCreatures.end())
    {
        OD_LOG_ERR("creature name=" + c->getName());
        return;
    }

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

std::vector<Creature*> GameMap::getCreaturesByAlliedSeat(const Seat* seat) const
{
    std::vector<Creature*> tempVector;

    // Loop over all the creatures in the GameMap and add them to the temp vector if their seat matches the one in parameter.
    for (Creature* creature : mCreatures)
    {
        if (seat->isAlliedSeat(creature->getSeat()) && creature->isAlive())
            tempVector.push_back(creature);
    }

    return tempVector;
}

std::vector<Creature*> GameMap::getCreaturesBySeat(const Seat* seat) const
{
    std::vector<Creature*> tempVector;

    // Loop over all the creatures in the GameMap and add them to the temp vector if their seat matches the one in parameter.
    for (Creature* creature : mCreatures)
    {
        if (creature->getSeat() == seat && creature->isAlive())
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

        for(const CreatureAction& action : creature->getActionQueue())
        {
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

        for(const CreatureAction& action : creature->getActionQueue())
        {
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
    OD_LOG_INF(serverStr() + "Adding rendered object " + obj->getName()
        + ",MeshName=" + obj->getMeshName());
    mRenderedMovableEntities.push_back(obj);
}

void GameMap::removeRenderedMovableEntity(RenderedMovableEntity *obj)
{
    OD_LOG_INF(serverStr() + "Removing rendered object " + obj->getName()
        + ",MeshName=" + obj->getMeshName());
    std::vector<RenderedMovableEntity*>::iterator it = std::find(mRenderedMovableEntities.begin(), mRenderedMovableEntities.end(), obj);
    if(it == mRenderedMovableEntities.end())
    {
        OD_LOG_ERR("obj name=" + obj->getName());
        return;
    }

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

    if(isServerGameMap())
    {
        // Set positions and update active spots
        for (RenderedMovableEntity* rendered : mRenderedMovableEntities)
        {
            rendered->setPosition(rendered->getPosition());
        }

        for (Room* room : mRooms)
        {
            room->updateActiveSpots();
        }

        for (Spell* spell : mSpells)
        {
            spell->setPosition(spell->getPosition());
        }

        for (Trap* trap : mTraps)
        {
            trap->updateActiveSpots();
        }

        for (Creature* creature : mCreatures)
        {
            //Set up definition for creature. This was previously done in createMesh for some reason.
            creature->setupDefinition(*this, *ConfigManager::getSingleton().getCreatureDefinitionDefaultWorker());
            //Set position to update info on what tile the creature is in.
            creature->setPosition(creature->getPosition());
            //Doesn't do anything currently.
            //creature->restoreInitialEntityState();
        }
    }
    else
    {
        // On client we create meshes
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
            rendered->setPosition(rendered->getPosition());
        }

        // Create OGRE entities for the creatures
        for (Creature* creature : mCreatures)
        {
            creature->setupDefinition(*this, *ConfigManager::getSingleton().getCreatureDefinitionDefaultWorker());
            creature->createMesh();
            creature->setPosition(creature->getPosition());
        }

        // Create OGRE entities for the map lights.
        for (MapLight* mapLight: mMapLights)
        {
            mapLight->createMesh();
            mapLight->setPosition(mapLight->getPosition());
        }

        // Create OGRE entities for the rooms
        for (Room* room : mRooms)
        {
            room->createMesh();
        }

        // Create OGRE entities for the rooms
        for (Trap* trap : mTraps)
        {
            trap->createMesh();
        }

        // Create OGRE entities for spells
        for (Spell* spell : mSpells)
        {
            spell->createMesh();
            spell->setPosition(spell->getPosition());
        }
    }

    for (Room* room : mRooms)
    {
        room->restoreInitialEntityState();
    }

    for (Trap* trap : mTraps)
    {
        trap->restoreInitialEntityState();
    }

    //Doesn't do anything currently
    /*
    for (Spell* spell : mSpells)
    {
        spell->restoreInitialEntityState();
    }

    for (RenderedMovableEntity* rendered : mRenderedMovableEntities)
    {
        rendered->restoreInitialEntityState();
    }*/

    OD_LOG_INF("entities created");
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

void GameMap::doTurn(double timeSinceLastTurn)
{
    OD_LOG_INF("Computing turn " + Helper::toString(mTurnNumber) + ", timeSinceLastTurn=" + Helper::toString(timeSinceLastTurn));
    unsigned int numCallsTo_path_atStart = mNumCallsTo_path;

    uint32_t miscUpkeepTime = doMiscUpkeep(timeSinceLastTurn);

    // Count how many creatures the player controls
    for(Creature* creature : mCreatures)
    {
        // Check to see if the creature has died.
        if (!creature->isAlive())
            continue;

        // We only count fighters
        if (creature->getDefinition()->isWorker())
            continue;

        Seat *tempSeat = creature->getSeat();
        if(tempSeat == nullptr)
            continue;

        ++(tempSeat->mNumCreaturesFighters);
    }

    OD_LOG_INF("During this turn there were " + Helper::toString(mNumCallsTo_path - numCallsTo_path_atStart)
        + " calls to GameMap::path(), miscUpkeepTime=" + Helper::toString(miscUpkeepTime));
}

void GameMap::doPlayerAITurn(double timeSinceLastTurn)
{
    mAiManager.doTurn(timeSinceLastTurn);
}

unsigned long int GameMap::doMiscUpkeep(double timeSinceLastTurn)
{
    Tile *tempTile;
    Ogre::Timer stopwatch;
    unsigned long int timeTaken;

    // We check if it is pay day
    mTimePayDay += timeSinceLastTurn;
    if((mTimePayDay >= ConfigManager::getSingleton().getTimePayDay()))
    {
        mTimePayDay = 0;
        // We only notify players with a dungeon temple
        for(Player* player : getPlayers())
        {
            if(!player->getIsHuman())
                continue;

            // We notify the player if he owns a fighter only
            bool isCreatureSeat = false;
            for(Creature* creature : mCreatures)
            {
                if(creature->getSeat() != player->getSeat())
                    continue;

                if(creature->getDefinition()->isWorker())
                    continue;

                isCreatureSeat = true;
                break;
            }

            if(!isCreatureSeat)
                continue;

            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, player);
            serverNotification->mPacket << "It's pay day !" << EventShortNoticeType::majorGameEvent;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }

        for(Creature* creature : mCreatures)
        {
            creature->itsPayDay();
        }
    }

    // Loop over all the filled seats in the game and check all the unfinished goals for each seat.
    // Add any seats with no remaining goals to the winningSeats vector.
    for (Seat* seat : mSeats)
    {
        if(seat->getPlayer() == nullptr)
            continue;

        seat->getPlayer()->upkeepPlayer(timeSinceLastTurn);

        // Check the previously completed goals to make sure they are still met.
        seat->checkAllCompletedGoals();

        // Check the goals and move completed ones to the completedGoals list for the seat.
        //NOTE: Once seats are placed on this list, they stay there even if goals are unmet.  We may want to change this.
        if (seat->checkAllGoals() == 0 && seat->numFailedGoals() == 0)
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

        seat->computeSeatBeginTurn();

        // Add the amount of mana this seat accrued this turn if the player has a dungeon temple
        if(seat->getNbRooms(RoomType::dungeonTemple) == 0)
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

        if(!creature->isAlive())
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

        if(!creature->isAlive())
            continue;

        if (creature->getDefinition()->isWorker())
            continue;

        ++ret;
    }

    return ret;
}

void GameMap::updateAnimations(Ogre::Real timeSinceLastFrame)
{
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
              int d1 = Pathfinding::squaredDistanceTile(*tile1, *tileStart);
              int d2 = Pathfinding::squaredDistanceTile(*tile2, *tileStart);
              return d1 < d2;
        });

    double magic = 2.0;
    for(Tile* tile : possibleDestsTmp)
    {
        if(!pathExists(creature, tileStart, tile))
            continue;

        // The first reachable tile is the best by default
        Ogre::Real dist = std::hypotf(tile->getX() - tileStart->getX(), tile->getY() - tileStart->getY());
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

    // We check if the tile we are heading to is walkable. We don't do the same for the start tile because it might
    //not be the case if a creature is on a door tile while it is closed
    if(creature == nullptr)
        return false;

    if((creature->getMoveSpeedGround() > 0.0) &&
        (creature->getMoveSpeedWater() > 0.0) &&
        (creature->getMoveSpeedLava() > 0.0))
    {
        return tileStart->isSameFloodFill(creature->getSeat(), FloodFillType::groundWaterLava, tileEnd);
    }
    if((creature->getMoveSpeedGround() > 0.0) &&
        (creature->getMoveSpeedWater() > 0.0))
    {
        return tileStart->isSameFloodFill(creature->getSeat(), FloodFillType::groundWater, tileEnd);
    }
    if((creature->getMoveSpeedGround() > 0.0) &&
        (creature->getMoveSpeedLava() > 0.0))
    {
        return tileStart->isSameFloodFill(creature->getSeat(), FloodFillType::groundLava, tileEnd);
    }

    return tileStart->isSameFloodFill(creature->getSeat(), FloodFillType::ground, tileEnd);
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

    AstarEntry *currentEntry = new AstarEntry(start, x1, y1, x2, y2);
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
            // We process the tile if the creature can go through. But if it is the first tile that is
            // not passable, we also process it. That happens if a door is closed
            if((creature->canGoThroughTile(neighbor.getTile())) ||
               (neighbor.getTile() == start))
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
    OD_LOG_INF(serverStr() + "Added player: " + player->getNick());
    return true;
}

bool GameMap::assignAI(Player& player, const std::string& aiType, const std::string& parameters)
{
    if (mAiManager.assignAI(player, aiType, parameters))
    {
        OD_LOG_INF("Assign AI: " + aiType + ", to player: " + player.getNick());
        return true;
    }

    OD_LOG_INF("Couldn't assign AI: " + aiType + ", to player: " + player.getNick());
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
        if(tile == nullptr)
        {
            OD_LOG_ERR("unexpected null tile");
            continue;
        }

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
        if(tile == nullptr)
        {
            OD_LOG_ERR("unexpected null tile");
            continue;
        }

        tile->fillWithAttackableCreatures(returnList, seat, enemyCreatures);
    }

    return returnList;
}

std::vector<GameEntity*> GameMap::getVisibleCarryableEntities(Creature* carrier, const std::vector<Tile*>& visibleTiles)
{
    std::vector<GameEntity*> returnList;

    // Loop over the visible tiles
    for (Tile* tile : visibleTiles)
    {
        if(tile == nullptr)
        {
            OD_LOG_ERR("unexpected null tile");
            continue;
        }

        tile->fillWithCarryableEntities(carrier, returnList);
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
    OD_LOG_INF(serverStr() + "Adding room " + r->getName() + ", nbTiles="
        + Helper::toString(nbTiles) + ", seatId=" + Helper::toString(r->getSeat()->getId()));
    for(Tile* tile : r->getCoveredTiles())
    {
        OD_LOG_INF(serverStr() + "Adding room " + r->getName() + ", tile=" + Tile::displayAsString(tile));
    }

    mRooms.push_back(r);
}

void GameMap::removeRoom(Room *r)
{
    OD_LOG_INF(serverStr() + "Removing room " + r->getName());
    // Rooms are removed when absorbed by another room or when they have no more tile
    // In both cases, the client have enough information to do that alone so no need to notify him
    std::vector<Room*>::iterator it = std::find(mRooms.begin(), mRooms.end(), r);
    if(it == mRooms.end())
    {
        OD_LOG_ERR("Room name=" + r->getName());
        return;
    }

    mRooms.erase(it);
}

std::vector<Room*> GameMap::getRoomsByType(RoomType type) const
{
    std::vector<Room*> returnList;
    for (Room* room : mRooms)
    {
        if (room->getType() == type  && room->getHP(nullptr) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

std::vector<Room*> GameMap::getRoomsByTypeAndSeat(RoomType type, const Seat* seat)
{
    std::vector<Room*> returnList;
    for (Room* room : mRooms)
    {
        if (room->getType() == type && room->getSeat() == seat && room->getHP(nullptr) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

std::vector<const Room*> GameMap::getRoomsByTypeAndSeat(RoomType type, const Seat* seat) const
{
    std::vector<const Room*> returnList;
    for (const Room* room : mRooms)
    {
        if (room->getType() == type && room->getSeat() == seat && room->getHP(nullptr) > 0.0)
            returnList.push_back(room);
    }

    return returnList;
}

unsigned int GameMap::numRoomsByTypeAndSeat(RoomType type, const Seat* seat) const
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
    OD_LOG_INF(serverStr() + "Adding trap " + trap->getName() + ", nbTiles="
        + Helper::toString(nbTiles) + ", seatId=" + Helper::toString(trap->getSeat()->getId()));

    mTraps.push_back(trap);
}

void GameMap::removeTrap(Trap *t)
{
    OD_LOG_INF(serverStr() + "Removing trap " + t->getName());
    std::vector<Trap*>::iterator it = std::find(mTraps.begin(), mTraps.end(), t);
    if(it == mTraps.end())
    {
        OD_LOG_ERR("Trap name=" + t->getName());
        return;
    }

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
    OD_LOG_INF(serverStr() + "Adding MapLight " + m->getName());
    mMapLights.push_back(m);
}

void GameMap::removeMapLight(MapLight *m)
{
    OD_LOG_INF(serverStr() + "Removing MapLight " + m->getName());

    std::vector<MapLight*>::iterator it = std::find(mMapLights.begin(), mMapLights.end(), m);
    if(it == mMapLights.end())
    {
        OD_LOG_ERR("MapLight name=" + m->getName());
        return;
    }

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
    if(s == nullptr)
    {
        OD_LOG_ERR("unexpected null seat");
        return false;
    }

    for(Seat* seat : mSeats)
    {
        if(seat->getId() == s->getId())
        {
            OD_LOG_ERR("Duplicated seat id=" + Helper::toString(seat->getId()));
            return false;
        }
    }
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
        serverNotification->mPacket << "You Won" << EventShortNoticeType::majorGameEvent;
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

bool GameMap::doFloodFill(Seat* seat, Tile* tile)
{
    if (!mFloodFillEnabled)
        return false;

    if(tile->isFloodFillFilled(seat))
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
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::ground, neigh);
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundWater, neigh);
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundLava, neigh);
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundWaterLava, neigh);
                break;
            }
            case TileType::water:
            {
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundWater, neigh);
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundWaterLava, neigh);
                break;
            }
            case TileType::lava:
            {
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundLava, neigh);
                hasChanged |= tile->updateFloodFillFromTile(seat, FloodFillType::groundWaterLava, neigh);
                break;
            }
            default:
                continue;
        }

        // If the tile is fully filled, no need to continue
        if(tile->isFloodFillFilled(seat))
            return true;
    }

    return hasChanged;
}

void GameMap::replaceFloodFill(Seat* seat, FloodFillType floodFillType, uint32_t colorOld, uint32_t colorNew)
{
    for (int jj = 0; jj < getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < getMapSizeX(); ++ii)
        {
            Tile* tile = getTile(ii,jj);
            if(tile->getFloodFillValue(seat, floodFillType) != colorOld)
                continue;

            tile->replaceFloodFill(seat, floodFillType, colorNew);
        }
    }
}

void GameMap::refreshFloodFill(Seat* seat, Tile* tile)
{
    std::vector<uint32_t> colors(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);

    // If the tile has opened a new place, we use the same floodfillcolor for all the areas
    for(Tile* neigh : tile->getAllNeighbors())
    {
        for(uint32_t i = 0; i < colors.size(); ++i)
        {
            FloodFillType type = static_cast<FloodFillType>(i);
            if(colors[i] == Tile::NO_FLOODFILL)
            {
                colors[i] = neigh->getFloodFillValue(seat, type);
                tile->updateFloodFillFromTile(seat, type, neigh);
            }
            else if((colors[i] != Tile::NO_FLOODFILL) &&
               (neigh->getFloodFillValue(seat, type) != Tile::NO_FLOODFILL) &&
               (neigh->getFloodFillValue(seat, type) != colors[i]))
            {
                replaceFloodFill(seat, type, neigh->getFloodFillValue(seat, type), colors[i]);
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
    // We do the floodfill for the rogue seat. Then, once it is done, we copy for the other seats.
    // If there are locked doors, floodfill will be refreshed when they are added
    Seat* rogueSeat = getSeatRogue();
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
                       (tile->getFloodFillValue(rogueSeat, FloodFillType::ground) == Tile::NO_FLOODFILL))
                    {
                        isTileFound = true;
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::ground) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::ground, nextUniqueFloodFillValue());
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundWater) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundWater, nextUniqueFloodFillValue());
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundLava) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundLava, nextUniqueFloodFillValue());
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundWaterLava) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundWaterLava, nextUniqueFloodFillValue());
                        break;
                    }
                }
                else if(currentType == FloodFillType::groundWater)
                {
                    if((tile->getType() == TileType::water) &&
                       (tile->getFloodFillValue(rogueSeat, FloodFillType::groundWater) == Tile::NO_FLOODFILL))
                    {
                        isTileFound = true;
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundWater) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundWater, nextUniqueFloodFillValue());
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundWaterLava) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundWaterLava, nextUniqueFloodFillValue());
                        break;
                    }
                }
                else if(currentType == FloodFillType::groundLava)
                {
                    if((tile->getType() == TileType::lava) &&
                       (tile->getFloodFillValue(rogueSeat, FloodFillType::groundLava) == Tile::NO_FLOODFILL))
                    {
                        isTileFound = true;
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundLava) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundLava, nextUniqueFloodFillValue());
                        if(tile->getFloodFillValue(rogueSeat, FloodFillType::groundWaterLava) == Tile::NO_FLOODFILL)
                            tile->replaceFloodFill(rogueSeat, FloodFillType::groundWaterLava, nextUniqueFloodFillValue());
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
                    OD_LOG_ERR("Unexpected enum value=" + Tile::toString(currentType));
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
                if(doFloodFill(rogueSeat, tile))
                    ++nbTiles;
            }

            // For optimization purposes, if a tile has changed, we go on the other side
            if(nbTiles > 0)
            {
                for(int xx = getMapSizeX() - 1; xx >= 0; --xx)
                {
                    Tile* tile = getTile(xx, yy);
                    if(doFloodFill(rogueSeat, tile))
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

    // We copy floodfill for all seats
    for(int xx = 0; xx < getMapSizeX(); ++xx)
    {
        for(int yy = 0; yy < getMapSizeY(); ++yy)
        {
            Tile* tile = getTile(xx, yy);
            if(tile == nullptr)
                continue;

            tile->copyFloodFillToOtherSeats(rogueSeat);
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
    if(!isServerGameMap())
        return;

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
        if(it == mActiveObjects.end())
        {
            OD_LOG_ERR("name=" + ge->getName());
            continue;
        }
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
            tempSS << tempGoal->getFailedMessage(*seat) << "\n";
        }
    }

    if (seat->numUncompleteGoals() > 0)
    {
        // Loop over the list of unmet goals for the seat we are sitting in an print them.
        tempSS << formatTitleOn << "Unfinished Goals:" << formatTitleOff << "\n\n";
        for (unsigned int i = 0; i < seat->numUncompleteGoals(); ++i)
        {
            Goal *tempGoal = seat->getUncompleteGoal(i);
            tempSS << tempGoal->getDescription(*seat) << "\n";
        }
    }

    if (seat->numCompletedGoals() > 0)
    {
        // Loop over the list of completed goals for the seat we are sitting in an print them.
        tempSS << "\n" << formatTitleOn << "Completed Goals:" << formatTitleOff << "\n\n";
        for (unsigned int i = 0; i < seat->numCompletedGoals(); ++i)
        {
            Goal *tempGoal = seat->getCompletedGoal(i);
            tempSS << tempGoal->getSuccessMessage(*seat) << "\n";
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
        ret = className + Helper::toString(mUniqueNumberCreature);
    } while(getCreature(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameRoom(RoomType type)
{
    std::string ret;
    do
    {
        ++mUniqueNumberRoom;
        ret = RoomManager::getRoomNameFromRoomType(type) + "_" + Helper::toString(mUniqueNumberRoom);
    } while(getRoomByName(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameRenderedMovableEntity(const std::string& baseName)
{
    std::string ret;
    do
    {
        ++mUniqueNumberRenderedMovableEntity;
        ret = RenderedMovableEntity::RENDEREDMOVABLEENTITY_PREFIX + baseName + "_" + Helper::toString(mUniqueNumberRenderedMovableEntity);
    } while(getRenderedMovableEntity(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameTrap(TrapType type)
{
    std::string ret;
    do
    {
        ++mUniqueNumberTrap;
        ret = TrapManager::getTrapNameFromTrapType(type) + "_" + Helper::toString(mUniqueNumberTrap);
    } while(getTrapByName(ret) != nullptr);
    return ret;
}

std::string GameMap::nextUniqueNameMapLight()
{
    std::string ret;
    do
    {
        ++mUniqueNumberMapLight;
        ret = MapLight::MAPLIGHT_NAME_PREFIX + Helper::toString(mUniqueNumberMapLight);
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
        case GameEntityType::giftBoxEntity:
            return getRenderedMovableEntity(entityName);

        case GameEntityType::spell:
            return getSpell(entityName);

        case GameEntityType::mapLight:
            return getMapLight(entityName);

        case GameEntityType::room:
            return getRoomByName(entityName);

        case GameEntityType::trap:
            return getTrapByName(entityName);

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
    creature->setDestination(tile);
}

void GameMap::consoleToggleCreatureVisualDebug(const std::string& creatureName)
{
    Creature* creature = getCreature(creatureName);
    if(creature == nullptr)
        return;

    bool enable = !creature->getHasVisualDebuggingEntities();
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

void GameMap::consoleAskUnlockResearches()
{
    for(Seat* seat : getSeats())
    {
        for(uint32_t i = 0; i < static_cast<uint32_t>(ResearchType::countResearch); ++i)
        {
            ResearchType research = static_cast<ResearchType>(i);
            if(research == ResearchType::nullResearchType)
                continue;

            if(seat->isResearchDone(research))
                continue;

            seat->addResearch(research);
        }
    }
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
    if(nb > 0)
    {
        OD_LOG_ERR("Duplicate mood modifier=" + name);
        return false;
    }

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
    OD_LOG_INF(serverStr() + "Adding spell " + spell->getName()
        + ",MeshName=" + spell->getMeshName());
    mSpells.push_back(spell);
}

void GameMap::removeSpell(Spell *spell)
{
    OD_LOG_INF(serverStr() + "Removing spell " + spell->getName()
        + ",MeshName=" + spell->getMeshName());
    std::vector<Spell*>::iterator it = std::find(mSpells.begin(), mSpells.end(), spell);
    if(it == mSpells.end())
    {
        OD_LOG_ERR("spell name=" + spell->getName());
        return;
    }

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

const std::string& GameMap::getMeshForDefaultTile() const
{
    // 0 means tile not linked to any neighboor
    return mTileSet->getTileValues(TileVisual::dirtFull).at(0).getMeshName();
}

const TileSetValue& GameMap::getMeshForTile(const Tile* tile) const
{
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

const Ogre::Vector3& GameMap::getTileSetScale() const
{
    return mTileSet->getScale();
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

void GameMap::playerSelects(std::vector<EntityBase*>& entities, int tileX1, int tileY1, int tileX2,
    int tileY2, SelectionTileAllowed tileAllowed, SelectionEntityWanted entityWanted, Player* player)
{
    std::vector<Tile*> tiles = rectangularRegion(tileX1, tileY1, tileX2, tileY2);
    for(Tile* tile : tiles)
    {
        switch(tileAllowed)
        {
            case SelectionTileAllowed::groundClaimedOwned:
            {
                if(tile->isFullTile())
                    continue;

                if(tile->getSeat() == nullptr)
                    continue;

                if(!tile->isClaimed())
                    continue;

                if(player->getSeat() != tile->getSeat())
                    continue;

                break;
            }
            case SelectionTileAllowed::groundClaimedAllied:
            {
                if(tile->isFullTile())
                    continue;

                if(tile->getSeat() == nullptr)
                    continue;

                if(!tile->isClaimed())
                    continue;

                if(!player->getSeat()->isAlliedSeat(tile->getSeat()))
                    continue;

                break;
            }
            case SelectionTileAllowed::groundClaimedNotEnemy:
            {
                if(tile->isFullTile())
                    continue;

                if(tile->getSeat() == nullptr)
                    continue;

                if(tile->isClaimed() && !player->getSeat()->isAlliedSeat(tile->getSeat()))
                    continue;

                break;
            }
            case SelectionTileAllowed::groundTiles:
            {
                if(tile->isFullTile())
                    continue;

                break;
            }
            default:
            {
                static bool logMsg = false;
                if(!logMsg)
                {
                    logMsg = true;
                    OD_LOG_ERR("Wrong SelectionTileAllowed int=" + Helper::toString(static_cast<uint32_t>(tileAllowed)));
                }
                continue;
            }
        }

        if(entityWanted == SelectionEntityWanted::tiles)
        {
            entities.push_back(tile);
            continue;
        }

        tile->fillWithEntities(entities, entityWanted, player);
    }
}

void GameMap::addClientUpkeepEntity(GameEntity* entity)
{
    // GameEntityClientUpkeep objects are only used on client side
    if(isServerGameMap())
        return;

    mGameEntityClientUpkeep.push_back(entity);
}

void GameMap::removeClientUpkeepEntity(GameEntity* entity)
{
    auto it = std::find(mGameEntityClientUpkeep.begin(), mGameEntityClientUpkeep.end(), entity);
    if(it == mGameEntityClientUpkeep.end())
        return;

    mGameEntityClientUpkeep.erase(it);
}

void GameMap::clientUpKeep(int64_t turnNumber)
{
    mTurnNumber = turnNumber;
    mLocalPlayer->decreaseSpellCooldowns();

    for(GameEntity* entity : mGameEntityClientUpkeep)
    {
        entity->clientUpkeep();
    }
}

void GameMap::doorLock(Tile* tileDoor, Seat* seat, bool locked)
{
    if(!locked)
    {
        // When a door is unlocked, we check all its neighboors to find a floodfill value for each possible
        // tile type and set the same for all neighboor tiles
        std::vector<uint32_t> colors(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);

        for(uint32_t i = 0; i < colors.size(); ++i)
        {
            FloodFillType type = static_cast<FloodFillType>(i);
            colors[i] = tileDoor->getFloodFillValue(seat, type);
        }

        // Now, we check all neighboors and replace floodfill values if different
        for(uint32_t i = 0; i < colors.size(); ++i)
        {
            if(colors[i] == Tile::NO_FLOODFILL)
                continue;

            FloodFillType type = static_cast<FloodFillType>(i);
            for(Tile* neigh : tileDoor->getAllNeighbors())
            {
                uint32_t neighColor = neigh->getFloodFillValue(seat, type);
                if(neighColor == Tile::NO_FLOODFILL)
                    continue;

                if(neighColor == colors[i])
                    continue;

                replaceFloodFill(seat, type, neighColor, colors[i]);
            }
        }

        return;
    }

    // We save the list of the creatures that are on the same floodfill as the door tile. Then, we will check if the path
    // is still valid
    std::vector<Creature*> creatures;
    for(Seat* alliedSeat : getSeats())
    {
        if((alliedSeat != seat) &&
           (!alliedSeat->isAlliedSeat(seat)))
        {
            continue;
        }

        std::vector<Creature*> alliedCreatures = getCreaturesBySeat(seat);
        for(Creature* creature : alliedCreatures)
        {
            if(!pathExists(creature, tileDoor, creature->getPositionTile()))
                continue;

            creatures.push_back(creature);
        }
    }

    std::vector<uint32_t> colors(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);
    Tile* tileChange = nullptr;
    uint32_t nbTilesNotFull = 0;
    for(Tile* neigh : tileDoor->getAllNeighbors())
    {
        // We look for the second not full tile and replace its floodfillvalues (the second is better than
        // the first in case a door is placed next to the gamemap border)
        if(neigh->isFullTile())
            continue;

        ++nbTilesNotFull;
        if(nbTilesNotFull >= 2)
        {
            tileChange = neigh;
            break;
        }
    }

    // If there is no tile to change, leaving
    if(tileChange == nullptr)
        return;

    // We only change tiles floodfilled like tileChange. That will avoid changing floodfill on tiles closed by
    // another closed door or something
    std::vector<uint32_t> colorsToChange(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);
    for(uint32_t i = 0; i < colors.size(); ++i)
    {
        FloodFillType type = static_cast<FloodFillType>(i);
        uint32_t tileChangeColor = tileChange->getFloodFillValue(seat, type);
        if(tileChangeColor == Tile::NO_FLOODFILL)
            continue;

        colorsToChange[i] = tileChangeColor;
        colors[i] = nextUniqueFloodFillValue();
    }

    changeFloodFillConnectedTiles(tileChange, seat, colorsToChange, colors, tileDoor);

    // We check if a creature from the given seat has a path through the door and stop it if there is
    for(Creature* creature : creatures)
        creature->checkWalkPathValid();
}

void GameMap::changeFloodFillConnectedTiles(Tile* startTile, Seat* seat, const std::vector<uint32_t>& oldColors,
    const std::vector<uint32_t>& newColors, Tile* tileIgnored)
{
    std::vector<Tile*> tiles;
    tiles.push_back(startTile);
    while(!tiles.empty())
    {
        Tile* tile = tiles.back();
        tiles.pop_back();

        // We add the neighboor tiles if they are floodfilled as startTile
        for(Tile* neigh : tile->getAllNeighbors())
        {
            // We check if the tile should not be processed
            if(neigh == tileIgnored)
                continue;

            for(uint32_t i = 0; i < newColors.size(); ++i)
            {
                if(newColors[i] == Tile::NO_FLOODFILL)
                    continue;

                FloodFillType type = static_cast<FloodFillType>(i);
                uint32_t neighColor = neigh->getFloodFillValue(seat, type);
                if(neighColor == Tile::NO_FLOODFILL)
                    continue;

                if((neighColor == oldColors[i]) &&
                   (std::find(tiles.begin(), tiles.end(), tile) == tiles.end()))
                {
                    tiles.push_back(neigh);
                    break;
                }
            }
        }

        // We replace floodillcolor for the current tile
        for(uint32_t i = 0; i < newColors.size(); ++i)
        {
            FloodFillType type = static_cast<FloodFillType>(i);
            if(newColors[i] == Tile::NO_FLOODFILL)
                continue;

            if(tile->getFloodFillValue(seat, type) == oldColors[i])
                tile->replaceFloodFill(seat, type, newColors[i]);
        }
    }
}

void GameMap::notifySeatsConfigured()
{
    mTeamIds.clear();
    // We always add the rogue team id
    mTeamIds.push_back(0);

    for(Seat* seat : mSeats)
    {
        if(seat->isRogueSeat())
            continue;

        uint32_t teamIndex = 0;
        for(int teamId : mTeamIds)
        {
            if(teamId == seat->getTeamId())
                break;

            ++teamIndex;
        }

        // If the team id was not in the list, save it
        if(teamIndex == mTeamIds.size())
            mTeamIds.push_back(seat->getTeamId());

        seat->setTeamIndex(teamIndex);
    }

    uint32_t nbTeams = mTeamIds.size();
    for(int xxx = 0; xxx < getMapSizeX(); ++xxx)
    {
        for(int yyy = 0; yyy < getMapSizeY(); ++yyy)
        {
            Tile* tile = getTile(xxx, yyy);
            if(tile == nullptr)
                continue;

            tile->setTeamsNumber(nbTeams);
        }
    }
    // Now that team ids are set and tiles are configured, we can compute floodfill
    enableFloodFill();
}

void GameMap::fireSpatialSound(const std::vector<Seat*>& seats, SpatialSoundType soundType,
        const std::string& soundFamily, Tile* tile)
{
    for(Seat* seat : seats)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::playSpatialSound, seat->getPlayer());
        serverNotification->mPacket << soundType << soundFamily << tile->getX() << tile->getY();
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}
