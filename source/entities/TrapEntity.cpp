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

#include "entities/TrapEntity.h"
#include "entities/DoorEntity.h"

#include "network/ODPacket.h"

#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <iostream>

TrapEntity::TrapEntity(GameMap* gameMap, bool isOnServerMap, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity) :
    PersistentObject(gameMap, isOnServerMap, buildingName, meshName, tile, rotationAngle, hideCoveredTile, opacity)
{
}

TrapEntity::TrapEntity(GameMap* gameMap, bool isOnServerMap) :
    PersistentObject(gameMap, isOnServerMap)
{
}

TrapEntity* TrapEntity::getTrapEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    TrapEntityType trapEntityType;
    is >> trapEntityType;

    TrapEntity* trapEntity = nullptr;
    switch(trapEntityType)
    {
        case TrapEntityType::trapEntity:
            trapEntity = new TrapEntity(gameMap, false);
            break;
        case TrapEntityType::doorEntity:
            trapEntity = new DoorEntity(gameMap, false);
            break;
        default:
            OD_LOG_ERR("Unknown TrapEntityType=" + Helper::toString(static_cast<uint32_t>(trapEntityType)));
            break;
    }
    return trapEntity;
}

bool TrapEntity::isVisibleForSeat(Seat* seat)
{
    // In editor mode, traps are visible for everybody
    if(getGameMap()->isInEditorMode())
        return true;

    if(std::find(mSeatsNotHidden.begin(), mSeatsNotHidden.end(), seat) == mSeatsNotHidden.end())
        return false;

    return true;
}

void TrapEntity::seatSawTriggering(Seat* seat)
{
    if(std::find(mSeatsNotHidden.begin(), mSeatsNotHidden.end(), seat) != mSeatsNotHidden.end())
        return;

    mSeatsNotHidden.push_back(seat);
}

void TrapEntity::notifySeatsWithVision(const std::vector<Seat*>& seats)
{
    // If we are in the editor, everyseat has vision
    if(getGameMap()->isInEditorMode())
    {
        PersistentObject::notifySeatsWithVision(seats);
    }
    else
    {
        // We only notify seats that have seen the trap trigger
        std::vector<Seat*> seatsToNotify;
        for(Seat* seat : seats)
        {
            if(std::find(mSeatsNotHidden.begin(), mSeatsNotHidden.end(), seat) == mSeatsNotHidden.end())
                continue;

            seatsToNotify.push_back(seat);
        }
        PersistentObject::notifySeatsWithVision(seatsToNotify);
    }
}

void TrapEntity::exportHeadersToPacket(ODPacket& os) const
{
    PersistentObject::exportHeadersToPacket(os);
    os << getTrapEntityType();
}

ODPacket& operator<<(ODPacket& os, const TrapEntityType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, TrapEntityType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<TrapEntityType>(tmp);
    return is;
}
