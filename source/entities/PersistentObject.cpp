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

#include "entities/PersistentObject.h"

#include "entities/Building.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "network/ODPacket.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/LogManager.h"

#include <iostream>

PersistentObject::PersistentObject(GameMap* gameMap, bool isOnServerMap, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, const Ogre::Vector3& scale, bool hideCoveredTile, float opacity) :
    BuildingObject(gameMap,
        isOnServerMap,
        buildingName,
        meshName,
        Ogre::Vector3(static_cast<Ogre::Real>(tile->getX()), static_cast<Ogre::Real>(tile->getY()), 0),
        rotationAngle,
        scale,
        hideCoveredTile,
        opacity),
    mTile(tile),
    mIsWorking(true)
{
}

PersistentObject::PersistentObject(GameMap* gameMap, bool isOnServerMap) :
    BuildingObject(gameMap, isOnServerMap)
{
}

GameEntityType PersistentObject::getObjectType() const
{
    return GameEntityType::persistentObject;
}


PersistentObject* PersistentObject::getPersistentObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    PersistentObject* obj = new PersistentObject(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}

void PersistentObject::notifySeatsWithVision(const std::vector<Seat*>& seats)
{
    // We process seats that lost vision
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

    // We process seats that gain vision. If the PersistentObject is working, we notify
    // that it is there. If it is not working, we notify that it has been removed
    for(Seat* seat : seats)
    {
        auto it = std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat);
        if(mIsWorking)
        {
            // If the seat was already in the list, nothing to do
            if(it != mSeatsWithVisionNotified.end())
                continue;

            mSeatsWithVisionNotified.push_back(seat);
        }
        else
        {
            // If the seat is not already in the list, nothing to do
            if(it != mSeatsWithVisionNotified.end())
                mSeatsWithVisionNotified.erase(it);
        }


        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        // If the PersistentObject is working, we notify vision. If not, we notify it has been removed
        if(mIsWorking)
        {
            // If the object has already been notified once, we remove it and re-create it
            if(std::find(mSeatsAlreadyNotifiedOnce.begin(), mSeatsAlreadyNotifiedOnce.end(), seat) == mSeatsAlreadyNotifiedOnce.end())
                mSeatsAlreadyNotifiedOnce.push_back(seat);
            else
                fireRemoveEntity(seat);

            fireAddEntity(seat, false);
        }
        else
        {
            auto it = std::find(mSeatsAlreadyNotifiedOnce.begin(), mSeatsAlreadyNotifiedOnce.end(), seat);
            if(it != mSeatsAlreadyNotifiedOnce.end())
            {
                mSeatsAlreadyNotifiedOnce.erase(it);
                fireRemoveEntity(seat);
            }
        }
    }
}

void PersistentObject::fireRemoveEntityToSeatsWithVision()
{
    for(Seat* seat : mSeatsAlreadyNotifiedOnce)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireRemoveEntity(seat);
    }
}

void PersistentObject::exportToPacket(ODPacket& os, const Seat* seat) const
{
    BuildingObject::exportToPacket(os, seat);

    getGameMap()->tileToPacket(os, mTile);
}

void PersistentObject::importFromPacket(ODPacket& is)
{
    BuildingObject::importFromPacket(is);

    mTile = getGameMap()->tileFromPacket(is);
    if(mTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName());
    }
}

bool PersistentObject::notifyRemoveAsked()
{
    mIsWorking = false;
    // If at least 1 player has vision on this PersistentObject, we cannot remove it
    // from gamemap.
    // We check if there is at least 1 seat that have been notified previously and
    // lost vision
    for(Seat* seat : mSeatsAlreadyNotifiedOnce)
    {
        auto it = std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat);
        if(it != mSeatsWithVisionNotified.end())
            continue;

        // There is at least 1 seat that have seen the PersistentObject but not currently
        // have vision on it. We cannot remove it
        return false;
    }

    return true;
}
