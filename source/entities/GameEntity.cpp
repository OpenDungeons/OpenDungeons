/*!
 * \file   GameEntity.cpp
 * \date:  28 March 2012
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
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

#include "entities/GameEntity.h"

#include "entities/BuildingObject.h"
#include "entities/ChickenEntity.h"
#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/MapLight.h"
#include "entities/MissileObject.h"
#include "entities/PersistentObject.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/ResearchEntity.h"
#include "entities/SmallSpiderEntity.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "entities/TreasuryObject.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "spell/Spell.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

void GameEntity::createMesh()
{
    if (mMeshExists)
        return;

    mMeshExists = true;
    createMeshLocal();
}

void GameEntity::destroyMesh()
{
    if(!mMeshExists)
        return;

    mMeshExists = false;

    destroyMeshLocal();
}

void GameEntity::deleteYourself()
{
    destroyMesh();
    if(mIsDeleteRequested)
        return;

    mIsDeleteRequested = true;

    getGameMap()->queueEntityForDeletion(this);
}

Tile* GameEntity::getPositionTile() const
{
    const Ogre::Vector3& tempPosition = getPosition();

    return getGameMap()->getTile(Helper::round(tempPosition.x),
                                 Helper::round(tempPosition.y));
}

bool GameEntity::addEntityToTile(Tile* tile)
{
    return tile->addEntity(this);
}

bool GameEntity::removeEntityFromTile(Tile* tile)
{
    return tile->removeEntity(this);
}

void GameEntity::firePickupEntity(Player* playerPicking)
{
    int seatId = playerPicking->getSeat()->getId();
    GameEntityType entityType = getObjectType();
    const std::string& entityName = getName();
    for(std::vector<Seat*>::iterator it = mSeatsWithVisionNotified.begin(); it != mSeatsWithVisionNotified.end();)
    {
        Seat* seat = *it;
        if(seat->getPlayer() == nullptr)
        {
            ++it;
            continue;
        }
        if(!seat->getPlayer()->getIsHuman())
        {
            ++it;
            continue;
        }

        // For other players than the one picking up the entity, we send a remove message
        if(seat->getPlayer() != playerPicking)
        {
            fireRemoveEntity(seat);
            it = mSeatsWithVisionNotified.erase(it);
            continue;
        }

        ++it;

        // If the creature was picked up by a human, we send an async message
        if(playerPicking->getIsHuman())
        {
            ServerNotification serverNotification(
                ServerNotificationType::entityPickedUp, seat->getPlayer());
            serverNotification.mPacket << seatId << entityType << entityName;
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotificationType::entityPickedUp, seat->getPlayer());
            serverNotification->mPacket << seatId << entityType << entityName;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void GameEntity::fireDropEntity(Player* playerPicking, Tile* tile)
{
    // If the player is a human, we send an asynchronous message to be as reactive as
    // possible. If it is an AI, we queue the message because it might have been created
    // during this turn (and, thus, not exist on client side)
    int seatId = playerPicking->getSeat()->getId();
    for(Seat* seat : getGameMap()->getSeats())
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;
        if(!seat->hasVisionOnTile(tile))
            continue;

        // For players with vision on the tile where the entity is dropped, we send an add message
        if(seat->getPlayer() != playerPicking)
        {
            // Because the entity is dropped, it is not on the map for the other players so no need
            // to check
            mSeatsWithVisionNotified.push_back(seat);
            fireAddEntity(seat, false);
            continue;
        }

        // If the creature was dropped by a human, we send an async message
        if(playerPicking->getIsHuman())
        {
            ServerNotification serverNotification(
                ServerNotificationType::entityDropped, seat->getPlayer());
            serverNotification.mPacket << seatId;
            getGameMap()->tileToPacket(serverNotification.mPacket, tile);
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotificationType::entityDropped, seat->getPlayer());
            serverNotification->mPacket << seatId;
            getGameMap()->tileToPacket(serverNotification->mPacket, tile);
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void GameEntity::notifySeatsWithVision(const std::vector<Seat*>& seats)
{
    // We notify seats that lost vision
    for(std::vector<Seat*>::iterator it = mSeatsWithVisionNotified.begin(); it != mSeatsWithVisionNotified.end();)
    {
        Seat* seat = *it;
        // If the seat is still in the list, nothing to do
        if(std::find(seats.begin(), seats.end(), seat) != seats.end())
        {
            ++it;
            continue;
        }

        it = mSeatsWithVisionNotified.erase(it);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireRemoveEntity(seat);
    }

    // We notify seats that gain vision
    for(Seat* seat : seats)
    {
        // If the seat was already in the list, nothing to do
        if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
            continue;

        mSeatsWithVisionNotified.push_back(seat);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireAddEntity(seat, false);
    }
}

void GameEntity::addSeatWithVision(Seat* seat, bool async)
{
    if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
        return;

    mSeatsWithVisionNotified.push_back(seat);
    fireAddEntity(seat, async);
}

void GameEntity::removeSeatWithVision(Seat* seat)
{
    std::vector<Seat*>::iterator it = std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat);
    if(it == mSeatsWithVisionNotified.end())
        return;

    mSeatsWithVisionNotified.erase(it);
    fireRemoveEntity(seat);
}

void GameEntity::fireRemoveEntityToSeatsWithVision()
{
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireRemoveEntity(seat);
    }

    mSeatsWithVisionNotified.clear();
}

void GameEntity::exportHeadersToStream(std::ostream& os) const
{
    // GameEntity are saved in the level file per type. For this reason, there is no
    // need to save the type
}

void GameEntity::exportHeadersToPacket(ODPacket& os) const
{
    os << getObjectType();
}

void GameEntity::exportToPacket(ODPacket& os) const
{
    int seatId = -1;
    if(mSeat != nullptr)
        seatId = mSeat->getId();

    os << seatId;
    os << mName;
    os << mMeshName;
    os << mPosition;
}

void GameEntity::importFromPacket(ODPacket& is)
{
    int seatId;
    OD_ASSERT_TRUE(is >> seatId);
    if(seatId != -1)
        mSeat = mGameMap->getSeatById(seatId);

    OD_ASSERT_TRUE(is >> mName);
    OD_ASSERT_TRUE(is >> mMeshName);
    OD_ASSERT_TRUE(is >> mPosition);
}

void GameEntity::exportToStream(std::ostream& os) const
{
    int seatId = -1;
    if(mSeat != nullptr)
        seatId = mSeat->getId();

    os << seatId << "\t";
    os << mName << "\t";
    os << mMeshName << "\t";
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
}

void GameEntity::importFromStream(std::istream& is)
{
    int seatId;
    OD_ASSERT_TRUE(is >> seatId);
    if(seatId != -1)
        mSeat = mGameMap->getSeatById(seatId);

    OD_ASSERT_TRUE(is >> mName);
    OD_ASSERT_TRUE(is >> mMeshName);
    OD_ASSERT_TRUE(is >> mPosition.x >> mPosition.y >> mPosition.z);
}

std::string GameEntity::getGameEntityStreamFormat()
{
    return "SeatId\tName\tMeshName\tPosX\tPosY\tPosZ";
}

GameEntity* GameEntity::getGameEntityeEntityFromStream(GameMap* gameMap, GameEntityType type, std::istream& is)
{
    GameEntity* entity = nullptr;
    switch(type)
    {
        case GameEntityType::buildingObject:
        {
            // Building objects are not stored in level files, we should not be here
            OD_ASSERT_TRUE(false);
            break;
        }
        case GameEntityType::chickenEntity:
        {
            entity = ChickenEntity::getChickenEntityFromStream(gameMap, is);
            break;
        }
        case GameEntityType::craftedTrap:
        {
            entity = CraftedTrap::getCraftedTrapFromStream(gameMap, is);
            break;
        }
        case GameEntityType::creature:
        {
            entity = Creature::getCreatureFromStream(gameMap, is);
            break;
        }
        case GameEntityType::mapLight:
        {
            entity = MapLight::getMapLightFromStream(gameMap, is);
            break;
        }
        case GameEntityType::missileObject:
        {
            entity = MissileObject::getMissileObjectFromStream(gameMap, is);
            break;
        }
        case GameEntityType::persistentObject:
        {
            // Persistent objects are not stored in level files, we should not be here
            OD_ASSERT_TRUE(false);
            break;
        }
        case GameEntityType::smallSpiderEntity:
        {
            // Small spiders are not stored in level files, we should not be here
            OD_ASSERT_TRUE(false);
            break;
        }
        case GameEntityType::spell:
        {
            entity = Spell::getSpellFromStream(gameMap, is);
            break;
        }
        case GameEntityType::trapEntity:
        {
            // Trap entities are handled by the trap
            OD_ASSERT_TRUE(false);
            break;
        }
        case GameEntityType::treasuryObject:
        {
            entity = TreasuryObject::getTreasuryObjectFromStream(gameMap, is);
            break;
        }
        case GameEntityType::researchEntity:
        {
            entity = ResearchEntity::getResearchEntityFromStream(gameMap, is);
            break;
        }
        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Helper::toString(
                static_cast<int>(type)));
            break;
        }
    }

    OD_ASSERT_TRUE(entity != nullptr);
    if(entity == nullptr)
        return nullptr;

    entity->importFromStream(is);
    return entity;
}

GameEntity* GameEntity::getGameEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    GameEntity* entity = nullptr;
    GameEntityType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case GameEntityType::buildingObject:
        {
            entity = BuildingObject::getBuildingObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::chickenEntity:
        {
            entity = ChickenEntity::getChickenEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::craftedTrap:
        {
            entity = CraftedTrap::getCraftedTrapFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::creature:
        {
            entity = Creature::getCreatureFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::mapLight:
        {
            entity = MapLight::getMapLightFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::missileObject:
        {
            entity = MissileObject::getMissileObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::persistentObject:
        {
            entity = PersistentObject::getPersistentObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::smallSpiderEntity:
        {
            entity = SmallSpiderEntity::getSmallSpiderEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::spell:
        {
            entity = Spell::getSpellFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::trapEntity:
        {
            entity = TrapEntity::getTrapEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::treasuryObject:
        {
            entity = TreasuryObject::getTreasuryObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::researchEntity:
        {
            entity = ResearchEntity::getResearchEntityFromPacket(gameMap, is);
            break;
        }
        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Helper::toString(
                static_cast<int>(type)));
            break;
        }
    }

    OD_ASSERT_TRUE(entity != nullptr);
    if(entity == nullptr)
        return nullptr;

    entity->importFromPacket(is);
    return entity;
}

std::string GameEntity::getOgreNamePrefix() const
{
    return Helper::toString(static_cast<int32_t>(getObjectType())) + "-";
}

ODPacket& operator<<(ODPacket& os, const GameEntityType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, GameEntityType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<GameEntityType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const GameEntityType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

std::istream& operator>>(std::istream& is, GameEntityType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<GameEntityType>(tmp);
    return is;
}
