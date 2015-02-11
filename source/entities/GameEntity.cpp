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
#include "entities/MissileObject.h"
#include "entities/PersistentObject.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/SmallSpiderEntity.h"
#include "spell/Spell.h"
#include "entities/TrapEntity.h"
#include "entities/TreasuryObject.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/RenderManager.h"
#include "rooms/Room.h"
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

std::string GameEntity::getNodeNameWithoutPostfix()
{
    return getOgreNamePrefix() + getName();
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

GameEntity* GameEntity::getGameEntityeEntityFromStream(GameMap* gameMap, std::istream& is)
{
    // Not used yet
    return nullptr;
}

GameEntity* GameEntity::getGameEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    // For now, only RenderedMovableEntity are used this way. But this should change soon
    RenderedMovableEntity* entity = nullptr;
    GameEntityType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case GameEntityType::buildingObject:
        {
            entity = BuildingObject::getBuildingObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::treasuryObject:
        {
            entity = TreasuryObject::getTreasuryObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::chickenEntity:
        {
            entity = ChickenEntity::getChickenEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::missileObject:
        {
            entity = MissileObject::getMissileObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::smallSpiderEntity:
        {
            entity = SmallSpiderEntity::getSmallSpiderEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::craftedTrap:
        {
            entity = CraftedTrap::getCraftedTrapFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::persistentObject:
        {
            entity = PersistentObject::getPersistentObjectFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::trapEntity:
        {
            entity = TrapEntity::getTrapEntityFromPacket(gameMap, is);
            break;
        }
        case GameEntityType::spell:
        {
            entity = Spell::getSpellFromPacket(gameMap, is);
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
