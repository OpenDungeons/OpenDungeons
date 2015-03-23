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

#include "network/ODPacket.h"

#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "utils/LogManager.h"

#include <iostream>

TrapEntity::TrapEntity(GameMap* gameMap, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity) :
    PersistentObject(gameMap, buildingName, meshName, tile, rotationAngle, hideCoveredTile, opacity)
{
}

TrapEntity::TrapEntity(GameMap* gameMap) :
    PersistentObject(gameMap)
{
}

TrapEntity* TrapEntity::getTrapEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    TrapEntity* obj = new TrapEntity(gameMap);
    return obj;
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

void TrapEntity::seatsSawTriggering(const std::vector<Seat*>& seats)
{
    for(Seat* seat : seats)
        seatSawTriggering(seat);
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
