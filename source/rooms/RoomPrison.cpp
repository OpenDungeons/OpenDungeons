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

#include "rooms/RoomPrison.h"

#include "entities/Creature.h"
#include "entities/SmallSpiderEntity.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const std::string RoomPrisonName = "Prison";
const std::string RoomPrisonNameDisplay = "Prison room";
const RoomType RoomPrison::mRoomType = RoomType::prison;

namespace
{
class RoomPrisonFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomPrison::mRoomType; }

    const std::string& getName() const override
    { return RoomPrisonName; }

    const std::string& getNameReadable() const override
    { return RoomPrisonNameDisplay; }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomPrison::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomPrison::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomPrison* room = new RoomPrison(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomPrison::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomPrison* room = new RoomPrison(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomPrison* room = new RoomPrison(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomPrison::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomPrison* room = new RoomPrison(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomPrisonFactory);
}

const int32_t OFFSET_TILE_X = 0;
const int32_t OFFSET_TILE_Y = -1;

RoomPrison::RoomPrison(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("PrisonGround");
}

RenderedMovableEntity* RoomPrison::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            // Prison do not have central active spot
            return nullptr;
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Skull", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Skull", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Skull", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Skull", tile, 180.0, false);
        }
        default:
            break;
    }
    return nullptr;
}

void RoomPrison::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }
    RoomPrison* rc = static_cast<RoomPrison*>(r);
    mPendingPrisoners.insert(mPendingPrisoners.end(), rc->mPendingPrisoners.begin(), rc->mPendingPrisoners.end());
    rc->mPendingPrisoners.clear();

    Room::absorbRoom(r);
}

void RoomPrison::doUpkeep()
{
    Room::doUpkeep();

    if(mCoveredTiles.empty())
        return;

    // We check if we have enough room for all prisoners
    uint32_t nbCreatures = 0;
    for(Tile* tile : mCoveredTiles)
    {
        for(GameEntity* entity : tile->getEntitiesInTile())
        {
            if(entity->getObjectType() != GameEntityType::creature)
                continue;

            Creature* creature = static_cast<Creature*>(entity);
            if(creature->getSeatPrison() != getSeat())
                continue;

            Tile* creatureTile = creature->getPositionTile();
            if(creatureTile == nullptr)
            {
                OD_LOG_ERR("creatureName=" + creature->getName() + ", position=" + Helper::toString(creature->getPosition()));
                continue;
            }

            if(nbCreatures >= mCentralActiveSpotTiles.size())
            {
                // We have more prisoner than room. We free them
                creature->setSeatPrison(nullptr);
                continue;
            }

            ++nbCreatures;
            // We slightly damage the prisoner
            double damage = ConfigManager::getSingleton().getRoomConfigDouble("PrisonDamagePerTurn");
            creature->takeDamage(this, damage, 0.0, 0.0, 0.0, creatureTile, false);

            if(creature->isAlive())
                actionPrisoner(creature);
            else
            {
                // The creature is dead. We can release it
                OD_LOG_INF("creature=" + creature->getName() + " died in prison=" + getName());

                if((getSeat()->getPlayer() != nullptr) &&
                   (getSeat()->getPlayer()->getIsHuman()))
                {
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotificationType::chatServer, getSeat()->getPlayer());
                    std::string msg = "A creature died starving in your prison";
                    serverNotification->mPacket << msg << EventShortNoticeType::aboutCreatures;
                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }

                creature->setSeatPrison(nullptr);
                creature->removeFromGameMap();
                creature->deleteYourself();

                const std::string& className = ConfigManager::getSingleton().getRoomConfigString("PrisonSpawnClass");
                const CreatureDefinition* classToSpawn = getGameMap()->getClassDescription(className);
                if(classToSpawn == nullptr)
                {
                    OD_LOG_ERR("className=" + className);
                    continue;
                }
                // Create a new creature and copy over the class-based creature parameters.
                Creature* newCreature = new Creature(getGameMap(), true, classToSpawn, getSeat());

                // Add the creature to the gameMap and create meshes so it is visible.
                newCreature->addToGameMap();
                newCreature->setPosition(Ogre::Vector3(creatureTile->getX(), creatureTile->getY(), 0.0f));
                newCreature->createMesh();
            }
        }
    }
}

bool RoomPrison::hasCarryEntitySpot(GameEntity* carriedEntity)
{
    if(carriedEntity->getObjectType() != GameEntityType::creature)
        return false;

    Creature* creature = static_cast<Creature*>(carriedEntity);
    if(!creature->canBeCarriedToBuilding(this))
        return false;

    // We count current prisoners + prisoners on their way
    uint32_t nbCreatures = countPrisoners();
    nbCreatures += mPendingPrisoners.size();
    if(nbCreatures >= mCentralActiveSpotTiles.size())
        return false;

    return true;
}

Tile* RoomPrison::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    if(carriedEntity->getObjectType() != GameEntityType::creature)
    {
        OD_LOG_ERR("room=" + getName() + ", entity=" + carriedEntity->getName());
        return nullptr;
    }

    Creature* creature = static_cast<Creature*>(carriedEntity);
    mPendingPrisoners.push_back(creature);
    return getCoveredTile(0);
}

void RoomPrison::notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
{
    // The carrier has brought the enemy creature
    if(carriedEntity->getObjectType() != GameEntityType::creature)
    {
        OD_LOG_ERR("room=" + getName() + ", entity=" + carriedEntity->getName());
        return;
    }

    // We add the creature to the prison. Note that we do not check if there is enough room because we
    // want to call setSeatPrison so that everything is correctly set in the creature and that will avoid
    // to do it twice (because a prisoner can also be dropped and in this case, notifyCarryingStateChanged
    // will not be called). We will handle if too many prisoners during the upkeep
    Creature* prisonerCreature = static_cast<Creature*>(carriedEntity);
    prisonerCreature->setSeatPrison(getSeat());

    // We check if we were waiting for this creature
    auto it = std::find(mPendingPrisoners.begin(), mPendingPrisoners.end(), prisonerCreature);
    if(it != mPendingPrisoners.end())
        mPendingPrisoners.erase(it);
}

uint32_t RoomPrison::countPrisoners()
{
    uint32_t nbCreatures = 0;
    for(Tile* tile : mCoveredTiles)
    {
        for(GameEntity* entity : tile->getEntitiesInTile())
        {
            if(entity->getObjectType() != GameEntityType::creature)
                continue;

            Creature* creature = static_cast<Creature*>(entity);
            if(creature->getSeatPrison() != getSeat())
                continue;

            ++nbCreatures;
        }
    }

    return nbCreatures;
}

void RoomPrison::actionPrisoner(Creature* creature)
{
    // We move the prisoner if it is not already moving
    if(creature->isMoving())
        return;

    if(creature->isKo())
        return;

    if(Random::Uint(1, 4) > 1)
        return;

    Tile* creatureTile = creature->getPositionTile();
    if(creatureTile == nullptr)
    {
        OD_LOG_ERR("creatureName=" + creature->getName() + ", position=" + Helper::toString(creature->getPosition()));
        return;
    }

    std::vector<Tile*> availableTiles;
    for(Tile* tile : creatureTile->getAllNeighbors())
    {
        if(tile->getCoveringBuilding() != this)
            continue;

        availableTiles.push_back(tile);
    }

    if(availableTiles.empty())
        return;

    uint32_t index = Random::Uint(0, availableTiles.size() - 1);
    Tile* tileDest = availableTiles[index];
    Ogre::Vector3 v (static_cast<Ogre::Real>(tileDest->getX()), static_cast<Ogre::Real>(tileDest->getY()), 0.0);
    std::vector<Ogre::Vector3> path;
    path.push_back(v);
    creature->setWalkPath(EntityAnimation::flee_anim, EntityAnimation::idle_anim, true, path);
}
