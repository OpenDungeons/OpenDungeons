/*
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

#include "rooms/RoomArena.h"

#include "entities/Creature.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const std::string RoomArenaName = "Arena";
const std::string RoomArenaNameDisplay = "Arena room";
const RoomType RoomArena::mRoomType = RoomType::arena;

namespace
{
class RoomArenaFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomArena::mRoomType; }

    const std::string& getName() const override
    { return RoomArenaName; }

    const std::string& getNameReadable() const override
    { return RoomArenaNameDisplay; }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomArena::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomArena::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomArena* room = new RoomArena(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomArena::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomArena* room = new RoomArena(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomArena* room = new RoomArena(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomArena::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomArena* room = new RoomArena(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomArenaFactory);
}

RoomArena::RoomArena(GameMap* gameMap) :
    Room(gameMap),
    mCreatureFighting1(nullptr),
    mCreatureFighting2(nullptr)
{
    setMeshName("Arena");
}

void RoomArena::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }

    RoomArena* rc = static_cast<RoomArena*>(r);
    std::vector<Creature*> creaturesFighting;
    if(rc->mCreatureFighting1 != nullptr)
        creaturesFighting.push_back(rc->mCreatureFighting1);
    if(rc->mCreatureFighting2 != nullptr)
        creaturesFighting.push_back(rc->mCreatureFighting2);

    for(Creature* creature : creaturesFighting)
    {
        if(mCreatureFighting1 == nullptr)
        {
            creature->removeGameEntityListener(rc);
            mCreatureFighting1 = creature;
            mCreatureFighting1->addGameEntityListener(this);
            // We remove the creature from its old arena
            if(rc->mCreatureFighting1 == creature)
                rc->mCreatureFighting1 = nullptr;
            if(rc->mCreatureFighting2 == creature)
                rc->mCreatureFighting2 = nullptr;
            continue;
        }
        if(mCreatureFighting2 == nullptr)
        {
            creature->removeGameEntityListener(rc);
            mCreatureFighting2 = creature;
            mCreatureFighting2->addGameEntityListener(this);
            // We remove the creature from its old arena
            if(rc->mCreatureFighting1 == creature)
                rc->mCreatureFighting1 = nullptr;
            if(rc->mCreatureFighting2 == creature)
                rc->mCreatureFighting2 = nullptr;
            continue;
        }

        // We have no free space for the creature. It should stop working
        creature->stopJob();
    }

    Room::absorbRoom(r);
}

bool RoomArena::hasOpenCreatureSpot(Creature* c)
{
    // We do not allow any creature to fight unless we have at least 1 active spot
    if(mCreatureFighting1 == nullptr)
        return true;

    if(mCreatureFighting2 == nullptr)
        return true;

    // We allow using arena only if level is not too high
    if (c->getLevel() >= ConfigManager::getSingleton().getRoomConfigUInt32("ArenaMaxTrainingLevel"))
        return false;

    return false;
}

bool RoomArena::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    if(mCreatureFighting1 == nullptr)
    {
        mCreatureFighting1 = creature;
        mCreatureFighting1->addGameEntityListener(this);
        return true;
    }

    if(mCreatureFighting2 == nullptr)
    {
        mCreatureFighting2 = creature;
        mCreatureFighting2->addGameEntityListener(this);
        return true;
    }

    OD_LOG_ERR("room=" + getName() + ", trying to add creature in unavailable arena creature=" + creature->getName());
    return false;
}

void RoomArena::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    if(mCreatureFighting1 == c)
    {
        mCreatureFighting1->removeGameEntityListener(this);
        mCreatureFighting1 = nullptr;
        return;
    }
    if(mCreatureFighting2 == c)
    {
        mCreatureFighting2->removeGameEntityListener(this);
        mCreatureFighting2 = nullptr;
        return;
    }

    OD_LOG_ERR("room=" + getName() + ", removing unexpected creature=" + c->getName());
}

void RoomArena::doUpkeep()
{
    Room::doUpkeep();

    if(mCoveredTiles.empty())
        return;

    // If a creature is ko, it should be removed from the arena
    if((mCreatureFighting1 != nullptr) && (mCreatureFighting1->isKo()))
    {
        mCreatureFighting1->stopJob();
        OD_ASSERT_TRUE_MSG(mCreatureFighting1 == nullptr, "room=" + getName() + ", mCreatureFighting1=" + mCreatureFighting1->getName());
    }
    if((mCreatureFighting2 != nullptr) && (mCreatureFighting2->isKo()))
    {
        mCreatureFighting2->stopJob();
        OD_ASSERT_TRUE_MSG(mCreatureFighting2 == nullptr, "room=" + getName() + ", mCreatureFighting2=" + mCreatureFighting2->getName());
    }

    // If no creature, nothing to do
    if((mCreatureFighting1 == nullptr) && (mCreatureFighting2 == nullptr))
        return;

    // If there is only one creature, we move it in the arena
    if((mCreatureFighting1 == nullptr) || (mCreatureFighting2 == nullptr))
    {
        Creature& creature = (mCreatureFighting1 == nullptr ? *mCreatureFighting2 : *mCreatureFighting1);
        if(creature.isMoving())
            return;
        if(creature.isWarmup())
            return;

        // We randomly take a tile from the room and let the creature go
        uint32_t index = Random::Uint(0, mCoveredTiles.size() - 1);
        creature.setDestination(mCoveredTiles[index]);
        return;
    }

    // Both creatures are set. They should fight if not already
    if(!mCreatureFighting1->isActionInList(CreatureActionType::fightArena))
    {
        mCreatureFighting1->fightInArena(*mCreatureFighting2);
    }
    if(!mCreatureFighting2->isActionInList(CreatureActionType::fightArena))
    {
        mCreatureFighting2->fightInArena(*mCreatureFighting1);
    }
}

void RoomArena::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);
    // TODO: save fighting creatures
}

bool RoomArena::importFromStream(std::istream& is)
{
    if(!Room::importFromStream(is))
        return false;
    // TODO: load fighting creatures

    return true;
}

std::string RoomArena::getListenerName() const
{
    return getName();
}

bool RoomArena::notifyDead(GameEntity* entity)
{
    if(entity == mCreatureFighting1)
    {
        mCreatureFighting1 = nullptr;
        return false;
    }
    if(entity == mCreatureFighting2)
    {
        mCreatureFighting2 = nullptr;
        return false;
    }
    return true;
}

bool RoomArena::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mCreatureFighting1)
    {
        mCreatureFighting1 = nullptr;
        return false;
    }
    if(entity == mCreatureFighting2)
    {
        mCreatureFighting2 = nullptr;
        return false;
    }
    return true;
}

bool RoomArena::notifyPickedUp(GameEntity* entity)
{
    if(entity == mCreatureFighting1)
    {
        mCreatureFighting1 = nullptr;
        return false;
    }
    if(entity == mCreatureFighting2)
    {
        mCreatureFighting2 = nullptr;
        return false;
    }
    return true;
}

bool RoomArena::notifyDropped(GameEntity* entity)
{
    // That should not happen. If the creature has been picked up, we should have unsubscribed
    OD_LOG_ERR("name=" + getName() + ", entity=" + entity->getName());
    return true;
}
