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

#include "entities/DoorEntity.h"

#include "entities/GameEntityType.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "traps/Trap.h"
#include "traps/TrapDoor.h"
#include "traps/TrapManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <iostream>

DoorEntity::DoorEntity(GameMap* gameMap, Building& building, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity,
        const std::string& initialAnimationState, bool initialAnimationLoop) :
    TrapEntity(
        gameMap,
        building,
        meshName,
        tile,
        rotationAngle,
        hideCoveredTile,
        opacity),
    mBuilding(&building)
{
    setSeat(building.getSeat());
    mPrevAnimationState = initialAnimationState;
    mPrevAnimationStateLoop = initialAnimationLoop;
    mBuilding->addGameEntityListener(this);
}

DoorEntity::DoorEntity(GameMap* gameMap) :
    TrapEntity(gameMap)
{
}

DoorEntity::~DoorEntity()
{
    if(mBuilding != nullptr)
        mBuilding->removeGameEntityListener(this);
}

void DoorEntity::exportToPacket(ODPacket& os, const Seat* seat) const
{
    TrapEntity::exportToPacket(os, seat);
    int seatId = getSeat()->getId();
    os << seatId;
}

void DoorEntity::importFromPacket(ODPacket& is)
{
    TrapEntity::importFromPacket(is);
    int seatId;
    OD_ASSERT_TRUE(is >> seatId);
    Seat* seat = getGameMap()->getSeatById(seatId);
    if(seat == nullptr)
    {
        OD_LOG_ERR("Couldn't get seat id=" + Helper::toString(seatId) + ", name=" + getName());
        return;
    }
    setSeat(seat);
}

bool DoorEntity::canSlap(Seat* seat)
{
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return false;
    }

    if(getGameMap()->isInEditorMode())
        return true;

    // Only the owning player can slap a door
    if(getSeat() != seat)
        return false;

    return true;
}

void DoorEntity::slap()
{
    if(!getIsOnServerMap())
        return;

    if(mBuilding == nullptr)
    {
        OD_LOG_ERR("name=" + getName());
        return;
    }

    if(mBuilding->getObjectType() != GameEntityType::trap)
    {
        OD_LOG_ERR("name=" + getName() + ", type=" + Helper::toString(static_cast<uint32_t>(mBuilding->getObjectType())));
        return;
    }

    Trap* trap = static_cast<Trap*>(mBuilding);
    if(!trap->isDoor())
    {
        OD_LOG_ERR("name=" + getName() + ", wrong type=" + TrapManager::getTrapNameFromTrapType(trap->getType()));
        return;
    }

    Tile* posTile = getPositionTile();
    if(posTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName());
        return;
    }

    // We notify the trap that it should change the floodfill for owning seat. It should be
    // handled in the trap because we want to remind if a door is open/closed when saving a game. And
    // saving it in the trap is the easiest
    TrapDoor* trapDoor = static_cast<TrapDoor*>(trap);
    trapDoor->notifyDoorSlapped(this, posTile);
}

DoorEntity* DoorEntity::getDoorEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    DoorEntity* obj = new DoorEntity(gameMap);
    return obj;
}

std::string DoorEntity::getListenerName() const
{
    return getName();
}

bool DoorEntity::notifyDead(GameEntity* entity)
{
    if(entity == mBuilding)
    {
        mBuilding = nullptr;
        return false;
    }
    return true;
}

bool DoorEntity::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mBuilding)
    {
        mBuilding = nullptr;
        return false;
    }
    return true;
}

bool DoorEntity::notifyPickedUp(GameEntity* entity)
{
    OD_LOG_ERR(getName() + ", entity=" + entity->getName());
    return true;
}

bool DoorEntity::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR(getName() + ", entity=" + entity->getName());
    return true;
}
