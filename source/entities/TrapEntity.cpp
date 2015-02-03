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

        // We don't notify clients so that the objects stays visible
    }

    // We notify seats that gain vision
    bool isEditorMode = getGameMap()->isInEditorMode();
    for(Seat* seat : seats)
    {
        // If the seat was already in the list, nothing to do
        if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
            continue;

        // If we are hidden for current seat, we do not notify our state
        // In editor mode, everybody can see traps
        if(!isEditorMode && std::find(mSeatsNotHidden.begin(), mSeatsNotHidden.end(), seat) == mSeatsNotHidden.end())
            continue;

        mSeatsWithVisionNotified.push_back(seat);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        // If the object has already been notified once, we remove it and re-create it
        if(std::find(mSeatsAlreadyNotifiedOnce.begin(), mSeatsAlreadyNotifiedOnce.end(), seat) == mSeatsAlreadyNotifiedOnce.end())
            mSeatsAlreadyNotifiedOnce.push_back(seat);
        else
            fireRemoveEntity(seat);

        fireAddEntity(seat, false);
    }
}
