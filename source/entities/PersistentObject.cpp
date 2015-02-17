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

#include "entities/Tile.h"

#include "network/ODPacket.h"

#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "utils/LogManager.h"

#include <iostream>

PersistentObject::PersistentObject(GameMap* gameMap, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity) :
    RenderedMovableEntity(gameMap, buildingName, meshName, rotationAngle, hideCoveredTile, opacity),
    mTile(tile)
{
    mPosition.x = static_cast<Ogre::Real>(mTile->getX());
    mPosition.y = static_cast<Ogre::Real>(mTile->getY());
    mPosition.z = 0;
}

PersistentObject::PersistentObject(GameMap* gameMap) :
    RenderedMovableEntity(gameMap)
{
}

PersistentObject* PersistentObject::getPersistentObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    PersistentObject* obj = new PersistentObject(gameMap);
    return obj;
}

void PersistentObject::addToGameMap()
{
    // We register on both client and server gamemaps
    RenderedMovableEntity::addToGameMap();
    // We register on the tile we are on
    mTile->registerPersistentObject(this);
}

void PersistentObject::removeFromGameMap()
{
    RenderedMovableEntity::removeFromGameMap();
    // Removes the PersistentObject. On client map, the persistent object will be removed when the associated tile is refreshed
    mTile->removePersistentObject(this);
}

void PersistentObject::notifySeatsWithVision(const std::vector<Seat*>& seats)
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

        // If the object has already been notified once, we remove it and re-create it
        if(std::find(mSeatsAlreadyNotifiedOnce.begin(), mSeatsAlreadyNotifiedOnce.end(), seat) == mSeatsAlreadyNotifiedOnce.end())
            mSeatsAlreadyNotifiedOnce.push_back(seat);
        else
            fireRemoveEntity(seat);

        fireAddEntity(seat, false);
    }
}

void PersistentObject::exportToPacket(ODPacket& os) const
{
    RenderedMovableEntity::exportToPacket(os);

    getGameMap()->tileToPacket(os, mTile);
}

void PersistentObject::importFromPacket(ODPacket& is)
{
    RenderedMovableEntity::importFromPacket(is);

    mTile = getGameMap()->tileFromPacket(is);
    OD_ASSERT_TRUE_MSG(mTile != nullptr, "name=" + getName());
}
