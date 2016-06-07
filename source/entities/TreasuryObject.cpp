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

#include "entities/TreasuryObject.h"

#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "rooms/Room.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>
#include <ostream>

TreasuryObject::TreasuryObject(GameMap* gameMap, int goldValue) :
    RenderedMovableEntity(gameMap, "Treasury_", getMeshNameForGold(goldValue), 0.0f, false),
    mGoldValue(goldValue),
    mHasGoldValueChanged(false)
{
}

TreasuryObject::TreasuryObject(GameMap* gameMap) :
    RenderedMovableEntity(gameMap),
    mGoldValue(0),
    mHasGoldValueChanged(false)
{
}

GameEntityType TreasuryObject::getObjectType() const
{
    return GameEntityType::treasuryObject;
}


void TreasuryObject::mergeGold(TreasuryObject* obj)
{
    OD_LOG_INF("Merging treasury=" + obj->getName() + " into " + getName());
    mGoldValue += obj->mGoldValue;
    mHasGoldValueChanged = true;
    obj->mGoldValue = 0;
    obj->mHasGoldValueChanged = true;
    obj->setIsOnMap(false);
}

void TreasuryObject::doUpkeep()
{
    if(mGoldValue <= 0)
    {
        removeFromGameMap();
        deleteYourself();
        return;
    }

    if(!getIsOnMap())
        return;

    // We check if we are on a tile where there is a treasury room. If so, we add gold there
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return;
    }

    if((mGoldValue > 0) &&
       (tile->getCoveringRoom() != nullptr))
    {
        int goldDeposited = tile->getCoveringRoom()->depositGold(mGoldValue, tile);
        if(goldDeposited > 0)
        {
            // We withdraw the amount we could deposit
            mGoldValue -= goldDeposited;
            if(mGoldValue == 0)
            {
                removeFromGameMap();
                deleteYourself();
                return;
            }
            mHasGoldValueChanged = true;
        }
    }

    if(mHasGoldValueChanged)
    {
        mHasGoldValueChanged = false;
        // If we are empty, we can remove safely
        if(mGoldValue == 0)
        {
            removeFromGameMap();
            deleteYourself();
            return;
        }

        // If the gold value changed the mesh, we remove the old mesh and create a new one
        if(getMeshName().compare(getMeshNameForGold(mGoldValue)) != 0)
        {
            // The treasury fullnes changed. We remove the object and create a new one
            removeFromGameMap();
            deleteYourself();

            TreasuryObject* obj = new TreasuryObject(getGameMap(), mGoldValue);
            obj->addToGameMap();
            Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                        static_cast<Ogre::Real>(tile->getY()), 0.0f);
            obj->createMesh();
            obj->setPosition(spawnPosition);
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
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return false;
    }

    if(!tile->isClaimedForSeat(seat) && !getGameMap()->isInEditorMode())
        return false;

    return true;
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

void TreasuryObject::addEntityToPositionTile()
{
    if(getIsOnMap())
        return;

    setIsOnMap(true);
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR(getGameMap()->serverStr() + "entityName=" + getName() + ", pos=" + Helper::toString(getPosition()));
        return;
    }
    OD_ASSERT_TRUE_MSG(tile->addTreasuryObject(this), getGameMap()->serverStr() + "entity=" + getName() + ", pos=" + Helper::toString(getPosition()) + ", tile=" + Tile::displayAsString(tile));
}

EntityCarryType TreasuryObject::getEntityCarryType(Creature* carrier)
{
    if(!getIsOnMap())
        return EntityCarryType::notCarryable;

    // We do not let it be carried as it will be removed during next upkeep
    if(mGoldValue <= 0)
        return EntityCarryType::notCarryable;

    // If we are on a treasury not full, we doesn't allow to be carried
    Tile* myTile = getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + getName());
        return EntityCarryType::gold;
    }

    if(myTile->getCoveringRoom() == nullptr)
        return EntityCarryType::gold;

    if(myTile->getCoveringRoom()->getTotalGoldStorage() <= 0)
        return EntityCarryType::gold;

    // If the treasury is on a room where it can be absorbed, it should not
    // be carried
    if(myTile->getCoveringRoom()->getTotalGoldStored() >= myTile->getCoveringRoom()->getTotalGoldStorage())
        return EntityCarryType::gold;

    return EntityCarryType::notCarryable;
}

void TreasuryObject::notifyEntityCarryOn(Creature* carrier)
{
    removeEntityFromPositionTile();
}

void TreasuryObject::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    addEntityToPositionTile();
}

int TreasuryObject::stealGold(Creature& creature, int value)
{
    if(value > mGoldValue)
        value = mGoldValue;

    if(value > 0)
    {
        // If the gold value is turned to 0, the treasury will be removed during its upkeep
        mGoldValue -= value;
        mHasGoldValueChanged = true;
    }

    return value;
}

const char* TreasuryObject::getMeshNameForGold(int gold)
{
    if (gold <= 0)
    {
        OD_LOG_ERR("Asking mesh for empty TreasuryObject");
        return "";
    }

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
    TreasuryObject* obj = new TreasuryObject(gameMap);
    obj->importFromStream(is);
    return obj;
}

TreasuryObject* TreasuryObject::getTreasuryObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    TreasuryObject* obj = new TreasuryObject(gameMap);
    obj->importFromPacket(is);
    return obj;
}

void TreasuryObject::exportToPacket(ODPacket& os, const Seat* seat) const
{
    RenderedMovableEntity::exportToPacket(os, seat);
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

bool TreasuryObject::importFromStream(std::istream& is)
{
    if(!RenderedMovableEntity::importFromStream(is))
        return false;
    if(!(is >> mGoldValue))
        return false;

    return true;
}

std::string TreasuryObject::getTreasuryObjectStreamFormat()
{
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "value";

    return format;
}
