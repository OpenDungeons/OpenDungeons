/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "creatureaction/CreatureAction.h"
#include "creatureaction/CreatureActionFightFriendly.h"
#include "entities/BuildingObject.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
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

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("ArenaCostPerTile"); }

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

static const Ogre::Real OFFSET_DUMMY = 0.3;
static const Ogre::Vector3 SCALE(0.7,0.7,0.7);

RoomArena::RoomArena(GameMap* gameMap) :
    Room(gameMap)
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

    Room::absorbRoom(r);

    RoomArena* rc = static_cast<RoomArena*>(r);
    OD_ASSERT_TRUE_MSG(rc->mCreaturesFighting.empty(), "room=" + getName() + ", roomAbs=" + rc->getName() + ", creature=" + Helper::toString(rc->mCreaturesFighting.size()));
}

bool RoomArena::hasOpenCreatureSpot(Creature* c)
{
    // We allow up to number central active spots creatures fighting
    if(mCreaturesFighting.size() >= mCentralActiveSpotTiles.size())
        return false;

    // We allow using arena only if level is not too high
    if (c->getLevel() >= ConfigManager::getSingleton().getRoomConfigUInt32("ArenaMaxTrainingLevel"))
        return false;

    return true;
}

bool RoomArena::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    mCreaturesFighting.push_back(creature);
    creature->addGameEntityListener(this);
    return true;
}

void RoomArena::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    auto it = std::find(mCreaturesFighting.begin(), mCreaturesFighting.end(), c);
    if(it == mCreaturesFighting.end())
    {
        OD_LOG_ERR("room=" + getName() + ", trying to remove " + c->getName());
        return;
    }

    c->removeGameEntityListener(this);
    mCreaturesFighting.erase(it);
}

void RoomArena::doUpkeep()
{
    Room::doUpkeep();

    if(mCoveredTiles.empty())
        return;

    // Check if a creature is ko. We fill a vector with creatures to stop using
    // the arena because we don't want to make them stop while iterating mCreaturesFighting
    // as it would invalidate the iterator
    std::vector<Creature*> creatures;
    for(Creature* creature : mCreaturesFighting)
    {
        if(!creature->isKo())
            continue;

        creatures.push_back(creature);
    }

    for(Creature* creature : creatures)
        creature->clearActionQueue();

    // If less than 2 creatures, nothing to do
    if(mCreaturesFighting.size() < 2)
        return;

    // Each creature not already fighting should look for the closest one and fight it
    for(Creature* creature : mCreaturesFighting)
    {
        if(creature->isActionInList(CreatureActionType::fightFriendly))
            continue;

        Tile* tileCreature = creature->getPositionTile();
        if(tileCreature == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", creatureName=" + creature->getName() + ", position=" + Helper::toString(creature->getPosition()));
            continue;
        }

        double closestDist = 0;
        Creature* closestOpponent = nullptr;
        for(Creature* opponent : mCreaturesFighting)
        {
            if(opponent == creature)
                continue;

            Tile* tileOpponent = opponent->getPositionTile();
            if(tileOpponent == nullptr)
            {
                OD_LOG_ERR("room=" + getName() + ", creatureName=" + opponent->getName() + ", position=" + Helper::toString(opponent->getPosition()));
                continue;
            }

            double dist = Pathfinding::squaredDistanceTile(*tileCreature, *tileOpponent);
            if((closestOpponent != nullptr) && (dist >= closestDist))
                continue;

            closestDist = dist;
            closestOpponent = opponent;
        }

        if(closestOpponent == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
            continue;
        }

        // We don't notify player fight when in the arena
        creature->pushAction(Utils::make_unique<CreatureActionFightFriendly>(*creature, closestOpponent, true, getCoveredTiles(), false));
    }
}

bool RoomArena::useRoom(Creature& creature, bool forced)
{
    // If the creature is alone, it should wander in the room. If it is not, it should wait until there is one ready
    if(mCreaturesFighting.size() < 2)
    {
        std::vector<Tile*> tiles = getCoveredTiles();
        if(tiles.empty())
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName());
            return false;
        }

        uint32_t index = Random::Uint(0, tiles.size() - 1);
        Tile* tile = tiles[index];
        if(!creature.setDestination(tile))
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName() + ", tile=" + Tile::displayAsString(tile));
            return false;
        }
        return false;
    }

    // We do nothing while waiting
    return false;
}

BuildingObject* RoomArena::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
    Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
    Ogre::Real z = 0;
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            return nullptr;
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            x -= OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 90.0, SCALE, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            x += OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 270.0, SCALE, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            y += OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 0.0, SCALE, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            y -= OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 180.0, SCALE, false);
        }
        default:
            break;
    }
    return nullptr;
}

bool RoomArena::shouldStopUseIfHungrySleepy(Creature& creature, bool forced)
{
    // If fighting, the creatures in the arena should not stop because hungry/tired
    // That means it should stop only if alone
    if(mCreaturesFighting.size() < 2)
        return true;

    return false;
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
    for(auto it = mCreaturesFighting.begin(); it != mCreaturesFighting.end(); ++it)
    {
        Creature* creature = *it;
        if(creature != entity)
            continue;

        mCreaturesFighting.erase(it);
        return false;
    }
    return true;
}

bool RoomArena::notifyRemovedFromGameMap(GameEntity* entity)
{
    for(auto it = mCreaturesFighting.begin(); it != mCreaturesFighting.end(); ++it)
    {
        Creature* creature = *it;
        if(creature != entity)
            continue;

        mCreaturesFighting.erase(it);
        return false;
    }
    return true;
}

bool RoomArena::notifyPickedUp(GameEntity* entity)
{
    for(auto it = mCreaturesFighting.begin(); it != mCreaturesFighting.end(); ++it)
    {
        Creature* creature = *it;
        if(creature != entity)
            continue;

        mCreaturesFighting.erase(it);
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
