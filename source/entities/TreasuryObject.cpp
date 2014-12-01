/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "entities/TreasuryObject.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "rooms/RoomTreasury.h"
#include "utils/LogManager.h"

#include <iostream>

TreasuryObject::TreasuryObject(GameMap* gameMap, int goldValue) :
    RenderedMovableEntity(gameMap, "Treasury_", "GoldstackLv3", 0.0f, false),
    mGoldValue(goldValue)
{
}

TreasuryObject::TreasuryObject(GameMap* gameMap) :
    RenderedMovableEntity(gameMap),
    mGoldValue(0)
{
    setMeshName("GoldstackLv3");
}

void TreasuryObject::mergeGold(TreasuryObject* obj)
{
    mGoldValue += obj->mGoldValue;
    obj->mGoldValue = 0;
}

void TreasuryObject::addGold(int goldValue)
{
    mGoldValue += goldValue;
}

void TreasuryObject::doUpkeep()
{
    if(!getIsOnMap())
        return;

    // We check if we are on a tile where there is a treasury room. If so, we add gold there
    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;

    if((mGoldValue > 0) &&
       (tile->getCoveringBuilding() != nullptr) &&
       (tile->getCoveringBuilding()->toRoom() != nullptr) &&
       (tile->getCoveringBuilding()->toRoom()->getType() == Room::treasury))
    {
        RoomTreasury* roomTreasury = static_cast<RoomTreasury*>(tile->getCoveringBuilding());
        int goldDeposited = roomTreasury->depositGold(mGoldValue, tile);
        // We withdraw the amount we could deposit
        addGold(-goldDeposited);
    }

    // If we are empty, we can remove safely
    if(mGoldValue <= 0)
    {
        tile->removeTreasuryObject(this);
        getGameMap()->removeRenderedMovableEntity(this);
        deleteYourself();
    }
}

bool TreasuryObject::tryPickup(Seat* seat, bool isEditorMode)
{
    if(!getIsOnMap())
        return false;

    // We do not let it be picked up as it will be removed during next upkeep. However, this is
    // true only on server side. On client side, if a treasury is available, it can be picked up.
    if(getGameMap()->isServerGameMap() && (mGoldValue <= 0))
        return false;

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return false;

    if(!tile->isClaimedForSeat(seat) && !isEditorMode)
        return false;

    return true;
}

void TreasuryObject::pickup()
{
    Tile* tile = getPositionTile();
    RenderedMovableEntity::pickup();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;

    tile->removeTreasuryObject(this);
}

bool TreasuryObject::tryDrop(Seat* seat, Tile* tile, bool isEditorMode)
{
    if (tile->getFullness() > 0.0)
        return false;

    // In editor mode, we allow to drop an object in dirt, claimed or gold tiles
    if(isEditorMode && (tile->getType() == Tile::dirt || tile->getType() == Tile::gold || tile->getType() == Tile::claimed))
        return true;

    // Otherwise, we allow to drop an object only on allied claimed tiles
    if(tile->getType() == Tile::claimed && tile->getSeat() != NULL && tile->getSeat()->isAlliedSeat(seat))
        return true;

    return false;
}

void TreasuryObject::setPosition(const Ogre::Vector3& v)
{
    RenderedMovableEntity::setPosition(v);
    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;

    tile->addTreasuryObject(this);

}

void TreasuryObject::notifyEntityCarried(bool isCarried)
{
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;
    if(isCarried)
    {
        setIsOnMap(false);
        myTile->removeTreasuryObject(this);
    }
    else
    {
        setIsOnMap(true);
        myTile->addTreasuryObject(this);
    }
}

const char* TreasuryObject::getFormat()
{
    // TODO : implement saving/loading in the level file
    return "position\tvalue";
}

TreasuryObject* TreasuryObject::getTreasuryObjectFromStream(GameMap* gameMap, std::istream& is)
{
    TreasuryObject* obj = new TreasuryObject(gameMap);
    return obj;
}

TreasuryObject* TreasuryObject::getTreasuryObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    TreasuryObject* obj = new TreasuryObject(gameMap);
    return obj;
}

void TreasuryObject::exportToPacket(ODPacket& os)
{
    RenderedMovableEntity::exportToPacket(os);
    os << mGoldValue;
}

void TreasuryObject::importFromPacket(ODPacket& is)
{
    RenderedMovableEntity::importFromPacket(is);
    OD_ASSERT_TRUE(is >> mGoldValue);
}

void TreasuryObject::exportToStream(std::ostream& os)
{
    RenderedMovableEntity::exportToStream(os);
    os << mGoldValue << "\t";
}

void TreasuryObject::importFromStream(std::istream& is)
{
    RenderedMovableEntity::importFromStream(is);
    OD_ASSERT_TRUE(is >> mGoldValue);
}
