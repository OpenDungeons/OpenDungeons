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

#include "entities/TreasuryObject.h"

#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "rooms/RoomTreasury.h"
#include "utils/LogManager.h"

#include <istream>
#include <ostream>

TreasuryObject::TreasuryObject(GameMap* gameMap, bool isOnServerMap, int goldValue) :
    RenderedMovableEntity(gameMap, isOnServerMap, "Treasury_", getMeshNameForGold(goldValue), 0.0f, false),
    mGoldValue(goldValue),
    mHasGoldValueChanged(false)
{
}

TreasuryObject::TreasuryObject(GameMap* gameMap, bool isOnServerMap) :
    RenderedMovableEntity(gameMap, isOnServerMap),
    mGoldValue(0),
    mHasGoldValueChanged(false)
{
}

void TreasuryObject::mergeGold(TreasuryObject* obj)
{
    mGoldValue += obj->mGoldValue;
    mHasGoldValueChanged = true;
    obj->mGoldValue = 0;
    obj->mHasGoldValueChanged = true;
}

void TreasuryObject::addGold(int goldValue)
{
    mGoldValue += goldValue;
    mHasGoldValueChanged = true;
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
       (tile->getCoveringRoom() != nullptr) &&
       (tile->getCoveringRoom()->getType() == RoomType::treasury))
    {
        RoomTreasury* roomTreasury = static_cast<RoomTreasury*>(tile->getCoveringRoom());
        int goldDeposited = roomTreasury->depositGold(mGoldValue, tile);
        // We withdraw the amount we could deposit
        addGold(-goldDeposited);
    }

    // If we are empty, we can remove safely
    if(mHasGoldValueChanged)
    {
        mHasGoldValueChanged = false;
        if(mGoldValue <= 0)
        {
            tile->removeEntity(this);
            removeFromGameMap();
            deleteYourself();
            return;
        }

        if(getMeshName().compare(getMeshNameForGold(mGoldValue)) != 0)
        {
            // The treasury fullnes changed. We remove the object and create a new one
            tile->removeEntity(this);
            removeFromGameMap();
            deleteYourself();

            TreasuryObject* obj = new TreasuryObject(getGameMap(), true, mGoldValue);
            obj->addToGameMap();
            Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                        static_cast<Ogre::Real>(tile->getY()), 0.0f);
            obj->createMesh();
            obj->setPosition(spawnPosition, false);
        }
    }
}

bool TreasuryObject::tryPickup(Seat* seat)
{
    if(!getIsOnMap())
        return false;

    // We do not let it be picked up as it will be removed during next upkeep. However, this is
    // true only on server side. On client side, if a treasury is available, it can be picked up.
    if(getIsOnServerMap() && (mGoldValue <= 0))
        return false;

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return false;

    if(!tile->isClaimedForSeat(seat) && !getGameMap()->isInEditorMode())
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

    tile->removeEntity(this);
}

bool TreasuryObject::tryDrop(Seat* seat, Tile* tile)
{
    if (tile->isFullTile())
        return false;

    // In editor mode, we allow to drop an object in dirt, claimed or gold tiles
    if(getGameMap()->isInEditorMode() &&
       (tile->getTileVisual() == TileVisual::dirtGround || tile->getTileVisual() == TileVisual::goldGround || tile->getTileVisual() == TileVisual::rockGround))
    {
        return true;
    }

    // we cannot drop an object on a tile we don't see
    if(!seat->hasVisionOnTile(tile))
        return false;

    // Otherwise, we allow to drop an object only on allied claimed tiles
    if(tile->isClaimedForSeat(seat))
        return true;

    return false;
}

bool TreasuryObject::addEntityToTile(Tile* tile)
{
    return tile->addTreasuryObject(this);
}

bool TreasuryObject::removeEntityFromTile(Tile* tile)
{
    return tile->removeEntity(this);
}

EntityCarryType TreasuryObject::getEntityCarryType()
{
    if(!getIsOnMap())
        return EntityCarryType::notCarryable;

    // We do not let it be carried as it will be removed during next upkeep
    if(mGoldValue <= 0)
        return EntityCarryType::notCarryable;

    // If we are on a treasury not full, we doesn't allow to be carried
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return EntityCarryType::gold;

    if(myTile->getCoveringRoom() == nullptr)
        return EntityCarryType::gold;

    if(myTile->getCoveringRoom()->getType() != RoomType::treasury)
        return EntityCarryType::gold;

    RoomTreasury* treasury = static_cast<RoomTreasury*>(myTile->getCoveringRoom());
    if(treasury->emptyStorageSpace() == 0)
        return EntityCarryType::gold;

    return EntityCarryType::notCarryable;
}

void TreasuryObject::notifyEntityCarryOn(Creature* carrier)
{
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;

    setIsOnMap(false);
    myTile->removeEntity(this);
}

void TreasuryObject::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    setIsOnMap(true);

    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;

    myTile->addTreasuryObject(this);
}

const char* TreasuryObject::getMeshNameForGold(int gold)
{
    if (gold <= 0)
        return "";

    if (gold <= 250)
        return "GoldstackLv1";

    if (gold <= 500)
        return "GoldstackLv2";

    if (gold <= 750)
        return "GoldstackLv3";

    return "GoldstackLv4";
}

TreasuryObject* TreasuryObject::getTreasuryObjectFromStream(GameMap* gameMap, std::istream& is)
{
    TreasuryObject* obj = new TreasuryObject(gameMap, true);
    return obj;
}

TreasuryObject* TreasuryObject::getTreasuryObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    TreasuryObject* obj = new TreasuryObject(gameMap, false);
    return obj;
}

void TreasuryObject::exportToPacket(ODPacket& os) const
{
    RenderedMovableEntity::exportToPacket(os);
    os << mGoldValue;
}

void TreasuryObject::importFromPacket(ODPacket& is)
{
    RenderedMovableEntity::importFromPacket(is);
    OD_ASSERT_TRUE(is >> mGoldValue);
}

void TreasuryObject::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mGoldValue << "\t";
}

void TreasuryObject::importFromStream(std::istream& is)
{
    RenderedMovableEntity::importFromStream(is);
    OD_ASSERT_TRUE(is >> mGoldValue);
}

std::string TreasuryObject::getTreasuryObjectStreamFormat()
{
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "value";

    return format;
}
