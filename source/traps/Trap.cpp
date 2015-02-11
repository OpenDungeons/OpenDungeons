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

#include "traps/Trap.h"

#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "entities/RenderedMovableEntity.h"
#include "traps/TrapCannon.h"
#include "traps/TrapSpike.h"
#include "traps/TrapBoulder.h"
#include "utils/Random.h"
#include "game/Player.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

Trap::Trap(GameMap* gameMap) :
    Building(gameMap),
    mNbShootsBeforeDeactivation(0),
    mReloadTime(0),
    mMinDamage(0.0),
    mMaxDamage(0.0)
{
}

Trap* Trap::getTrapFromStream(GameMap* gameMap, std::istream &is)
{
    Trap* tempTrap = nullptr;
    TrapType nType;
    is >> nType;

    switch (nType)
    {
        case TrapType::nullTrapType:
            tempTrap = nullptr;
            break;
        case TrapType::cannon:
            tempTrap = TrapCannon::getTrapCannonFromStream(gameMap, is);
            break;
        case TrapType::spike:
            tempTrap = TrapSpike::getTrapSpikeFromStream(gameMap, is);
            break;
        case TrapType::boulder:
            tempTrap = TrapBoulder::getTrapBoulderFromStream(gameMap, is);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    if(tempTrap == nullptr)
        return nullptr;

    tempTrap->importFromStream(is);

    return tempTrap;
}

Trap* Trap::getTrapFromPacket(GameMap* gameMap, ODPacket &is)
{
    Trap* tempTrap = nullptr;
    TrapType nType;
    is >> nType;

    switch (nType)
    {
        case TrapType::nullTrapType:
            tempTrap = nullptr;
            break;
        case TrapType::cannon:
            tempTrap = TrapCannon::getTrapCannonFromPacket(gameMap, is);
            break;
        case TrapType::spike:
            tempTrap = TrapSpike::getTrapSpikeFromPacket(gameMap, is);
            break;
        case TrapType::boulder:
            tempTrap = TrapBoulder::getTrapBoulderFromPacket(gameMap, is);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    if(tempTrap == nullptr)
        return nullptr;

    tempTrap->importFromPacket(is);

    return tempTrap;
}

const char* Trap::getTrapNameFromTrapType(TrapType t)
{
    switch (t)
    {
        case TrapType::nullTrapType:
            return "NullTrapType";

        case TrapType::cannon:
            return "Cannon";

        case TrapType::spike:
            return "Spike";

        case TrapType::boulder:
            return "Boulder";

        default:
            return "UnknownTrapType";
    }
}

int Trap::costPerTile(TrapType t)
{
    switch (t)
    {
        case TrapType::nullTrapType:
            return 0;

        case TrapType::cannon:
            return ConfigManager::getSingleton().getTrapConfigInt32("CannonCostPerTile");

        case TrapType::spike:
            return ConfigManager::getSingleton().getTrapConfigInt32("SpikeCostPerTile");

        case TrapType::boulder:
            return ConfigManager::getSingleton().getTrapConfigInt32("BoulderCostPerTile");

        default:
            return 0;
    }
}

void Trap::addToGameMap()
{
    getGameMap()->addTrap(this);
    setIsOnMap(true);
    if(!getGameMap()->isServerGameMap())
        return;

    getGameMap()->addActiveObject(this);
}

void Trap::removeFromGameMap()
{
    getGameMap()->removeTrap(this);
    setIsOnMap(false);
    if(!getGameMap()->isServerGameMap())
        return;

    removeAllBuildingObjects();
    getGameMap()->removeActiveObject(this);
}

void Trap::doUpkeep()
{
    std::vector<Tile*> tilesToRemove;
    for (Tile* tile : mCoveredTiles)
    {
        if(tile->getClaimedPercentage() >= 1.0 && !getSeat()->isAlliedSeat(tile->getSeat()))
        {
            tilesToRemove.push_back(tile);
            continue;
        }

        if (mTileHP[tile] <= 0.0)
        {
            tilesToRemove.push_back(tile);
            continue;
        }
    }

    if (!tilesToRemove.empty())
    {
        for(Tile* tile : tilesToRemove)
            removeCoveredTile(tile);

        updateActiveSpots();
        createMesh();
    }

    // If no more tiles, the trap is removed
    if (numCoveredTiles() <= 0)
    {
        removeFromGameMap();
        deleteYourself();
        return;
    }

    for(Tile* tile : mCoveredTiles)
    {
        // If the trap is deactivated, it cannot shoot
        TrapTileInfo& tileInfo = mTrapTiles[tile];
        if (!tileInfo.isActivated())
            continue;

        if(tileInfo.decreaseReloadTime())
            continue;

        if(shoot(tile))
        {
            tileInfo.setReloadTime(mReloadTime);
            if(!tileInfo.decreaseShoot())
                deactivate(tile);

            const std::vector<Seat*>& seats = tile->getSeatsWithVision();
            TrapEntity* trapEntity = tileInfo.getTrapEntity();
            trapEntity->seatsSawTriggering(seats);

            // Warn the player the trap has triggered
            GameMap* gameMap = getGameMap();
            if (gameMap->isServerGameMap())
                gameMap->playerIsFighting(gameMap->getPlayerBySeat(getSeat()));
        }
    }
}

int32_t Trap::getNbNeededCraftedTrap() const
{
    int32_t nbNeededCraftedTrap = 0;
    for(Tile* tile : mCoveredTiles)
    {
        if(mTrapTiles.count(tile) <= 0)
            continue;

        const TrapTileInfo& trapTileInfo = mTrapTiles.at(tile);
        if (trapTileInfo.isActivated())
            continue;

        if(trapTileInfo.getCarriedCraftedTrap() != nullptr)
            continue;

        ++nbNeededCraftedTrap;
    }

    return nbNeededCraftedTrap;
}

void Trap::addCoveredTile(Tile* t, double nHP)
{
    Building::addCoveredTile(t, nHP);

    // The trap starts deactivated.
    mTrapTiles[t] = TrapTileInfo(mReloadTime, false);
}

bool Trap::removeCoveredTile(Tile* t)
{
    if(!Building::removeCoveredTile(t))
        return false;

    mTrapTiles.erase(t);

    return true;
}

void Trap::updateActiveSpots()
{
    // Active spots are handled by the server only
    if(!getGameMap()->isServerGameMap())
        return;

    // For a trap, by default, every tile is an active spot
    if(mCoveredTiles.size() > mBuildingObjects.size())
    {
        // More tiles than RenderedMovableEntity. This will happen when the trap is created
        for(Tile* tile : mCoveredTiles)
        {
            RenderedMovableEntity* obj = notifyActiveSpotCreated(tile);
            if(obj == nullptr)
                continue;

            addBuildingObject(tile, obj);
        }
    }
    else if(mCoveredTiles.size() < mBuildingObjects.size())
    {
        // Less tiles than RenderedMovableEntity. This will happen when a tile from this trap is destroyed
        std::vector<Tile*> tilesToRemove;
        for(std::pair<Tile* const, RenderedMovableEntity*> p : mBuildingObjects)
        {
            Tile* tile = p.first;
            // We store removed tiles
            if(std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tile) == mCoveredTiles.end())
                tilesToRemove.push_back(tile);
        }

        // Then, we process removing (that will remove tiles from mBuildingObjects)
        OD_ASSERT_TRUE(!tilesToRemove.empty());
        for(std::vector<Tile*>::iterator it = tilesToRemove.begin(); it != tilesToRemove.end(); ++it)
        {
            Tile* tile = *it;
            if(mBuildingObjects.count(tile) > 0)
                notifyActiveSpotRemoved(tile);
        }
    }
}

RenderedMovableEntity* Trap::notifyActiveSpotCreated(Tile* tile)
{
    TrapEntity* trapEntity = getTrapEntity(tile);
    if(trapEntity == nullptr)
        return nullptr;

    // Allied seats with the creator do see the trap from the start
    trapEntity->seatSawTriggering(getSeat());
    trapEntity->seatsSawTriggering(getSeat()->getAlliedSeats());
    mTrapTiles[tile].setTrapEntity(trapEntity);
    return trapEntity;
}

void Trap::notifyActiveSpotRemoved(Tile* tile)
{
    removeBuildingObject(tile);
}

void Trap::activate(Tile* tile)
{
    if (tile == nullptr)
        return;

    mTrapTiles[tile].setActivated(true);
    mTrapTiles[tile].setNbShootsBeforeDeactivation(mNbShootsBeforeDeactivation);
    mTrapTiles[tile].setReloadTime(0);

    RenderedMovableEntity* entity = getBuildingObjectFromTile(tile);
    if (entity == nullptr)
        return;

    entity->setMeshOpacity(1.0f);
}

void Trap::deactivate(Tile* tile)
{
    if (tile == nullptr)
        return;

    mTrapTiles[tile].setActivated(false);

    RenderedMovableEntity* entity = getBuildingObjectFromTile(tile);
    if (entity == nullptr)
        return;

    entity->setMeshOpacity(0.5f);
}

bool Trap::isActivated(Tile* tile) const
{
    std::map<Tile*, TrapTileInfo>::const_iterator it = mTrapTiles.find(tile);
    if (it == mTrapTiles.end())
        return false;

    return it->second.isActivated();
}

void Trap::setupTrap(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    setName(name);
    setSeat(seat);
    for(Tile* tile : tiles)
        addCoveredTile(tile, Trap::DEFAULT_TILE_HP);
}

int32_t Trap::getNeededForgePointsPerTrap(TrapType trapType)
{
    // TODO : use configuration file (same as trap price)
    switch(trapType)
    {
        case TrapType::nullTrapType:
            return 0;
        case TrapType::cannon:
            return 50;
        case TrapType::spike:
            return 60;
        case TrapType::boulder:
            return 70;
        default:
            OD_ASSERT_TRUE_MSG(false, "Asked for wrong trap type=" + Ogre::StringConverter::toString(static_cast<int32_t>(trapType)));
            break;
    }
    // We shouldn't go here
    return 0;
}

bool Trap::hasCarryEntitySpot(GameEntity* carriedEntity)
{
    if(getNbNeededCraftedTrap() <= 0)
        return false;

    if(carriedEntity->getObjectType() != GameEntityType::renderedMovableEntity)
        return false;

    RenderedMovableEntity* rme = static_cast<RenderedMovableEntity*>(carriedEntity);
    if(rme->getRenderedMovableEntityType() != RenderedMovableEntityType::craftedTrap)
        return false;

    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(rme);
    if(craftedTrap->getTrapType() != getType())
        return false;

    return true;
}

Tile* Trap::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    OD_ASSERT_TRUE_MSG(carriedEntity->getObjectType() == GameEntityType::renderedMovableEntity,
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(carriedEntity->getObjectType() != GameEntityType::renderedMovableEntity)
        return nullptr;

    RenderedMovableEntity* rme = static_cast<RenderedMovableEntity*>(carriedEntity);
    OD_ASSERT_TRUE_MSG(rme->getRenderedMovableEntityType() == RenderedMovableEntityType::craftedTrap,
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(rme->getRenderedMovableEntityType() != RenderedMovableEntityType::craftedTrap)
        return nullptr;

    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(rme);
    OD_ASSERT_TRUE_MSG(craftedTrap->getTrapType() == getType(),
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(craftedTrap->getTrapType() != getType())
        return nullptr;

    for(Tile* tile : mCoveredTiles)
    {
        if(mTrapTiles.count(tile) <= 0)
            continue;

        TrapTileInfo& trapTileInfo = mTrapTiles.at(tile);
        if (trapTileInfo.isActivated())
            continue;

        if(trapTileInfo.getCarriedCraftedTrap() != nullptr)
            continue;

        // We can accept the craftedTrap on this tile
        trapTileInfo.setCarriedCraftedTrap(craftedTrap);
        return tile;
    }

    return nullptr;
}

void Trap::notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
{
    if(carriedEntity == nullptr)
        return;

    for(Tile* tile : mCoveredTiles)
    {
        if(mTrapTiles.count(tile) <= 0)
            continue;

        TrapTileInfo& trapTileInfo = mTrapTiles.at(tile);
        if(trapTileInfo.isActivated())
            continue;
        if(trapTileInfo.getCarriedCraftedTrap() != carriedEntity)
            continue;

        // We check if the carrier is at the expected destination
        Tile* carrierTile = carrier->getPositionTile();
        OD_ASSERT_TRUE_MSG(carrierTile != nullptr, "carrier=" + carrier->getName());
        if(carrierTile == nullptr)
        {
            trapTileInfo.setCarriedCraftedTrap(nullptr);
            return;
        }

        Tile* tileExpected = getGameMap()->getTile(tile->getX(), tile->getY());
        if(tileExpected != carrierTile)
        {
            trapTileInfo.setCarriedCraftedTrap(nullptr);
            return;
        }

        // The carrier has brought carried trap
        CraftedTrap* craftedTrap = trapTileInfo.getCarriedCraftedTrap();
        OD_ASSERT_TRUE_MSG(tile->removeEntity(craftedTrap), "trap=" + getName()
            + ", craftedTrap=" + craftedTrap->getName()
            + ", tile=" + Tile::displayAsString(tile));
        craftedTrap->removeFromGameMap();
        craftedTrap->deleteYourself();
        activate(tile);
        trapTileInfo.setCarriedCraftedTrap(nullptr);
    }
    // We couldn't find the entity in the list. That may happen if the active spot has
    // been erased between the time the carrier tried to come and the time it arrived.
    // In any case, nothing to do
}

bool Trap::isAttackable(Tile* tile, Seat* seat) const
{
    if(!Building::isAttackable(tile, seat))
        return false;

    // We check if the trap is hidden for this seat
    OD_ASSERT_TRUE_MSG(mTrapTiles.count(tile) > 0, "name=" + getName() + ", tile=" + Tile::displayAsString(tile));
    if(mTrapTiles.count(tile) <= 0)
        return false;

    const TrapTileInfo& trapTileInfo = mTrapTiles.at(tile);
    const std::vector<Seat*>& seatsNotHidden = trapTileInfo.getTrapEntity()->getSeatsNotHidden();
    if(std::find(seatsNotHidden.begin(), seatsNotHidden.end(), seat) == seatsNotHidden.end())
        return false;

    return true;
}

std::string Trap::getFormat()
{
    return "typeTrap\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY\tisActivated(0/1)\t\tSubsequent Lines: optional specific data";
}

void Trap::exportHeadersToStream(std::ostream& os)
{
    os << getType() << "\t";
}

void Trap::exportHeadersToPacket(ODPacket& os)
{
    os << getType();
}

void Trap::exportToPacket(ODPacket& os) const
{
    int nbTiles = mCoveredTiles.size();
    const std::string& name = getName();
    int seatId = getSeat()->getId();
    os << name << seatId;
    os << nbTiles;
    for(Tile* tempTile : mCoveredTiles)
    {
        os << tempTile->getX() << tempTile->getY();
    }
}

void Trap::importFromPacket(ODPacket& is)
{
    int tilesToLoad, tempX, tempY, tempInt;
    std::string name;
    OD_ASSERT_TRUE(is >> name);
    setName(name);

    OD_ASSERT_TRUE(is >> tempInt);
    setSeat(getGameMap()->getSeatById(tempInt));

    OD_ASSERT_TRUE(is >> tilesToLoad);
    for (int i = 0; i < tilesToLoad; ++i)
    {
        OD_ASSERT_TRUE(is >> tempX >> tempY);
        Tile *tempTile = getGameMap()->getTile(tempX, tempY);
        OD_ASSERT_TRUE_MSG(tempTile != nullptr, "tile=" + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        if (tempTile != nullptr)
        {
            addCoveredTile(tempTile, Trap::DEFAULT_TILE_HP);
            tempTile->setSeat(getSeat());
        }
    }
}

void Trap::exportToStream(std::ostream& os) const
{
    int32_t nbTiles = mCoveredTiles.size();
    int seatId = getSeat()->getId();
    os << seatId << "\t" << nbTiles << "\n";
    for(Tile* tempTile : mCoveredTiles)
    {
        os << tempTile->getX() << "\t" << tempTile->getY() << "\t";
        os << (isActivated(tempTile) ? 1 : 0) << "\n";
    }
}

void Trap::importFromStream(std::istream& is)
{
    int tilesToLoad, tempX, tempY, tempInt, tempActiv;

    OD_ASSERT_TRUE(is >> tempInt);
    setSeat(getGameMap()->getSeatById(tempInt));

    OD_ASSERT_TRUE(is >> tilesToLoad);
    for (int i = 0; i < tilesToLoad; ++i)
    {
        OD_ASSERT_TRUE(is >> tempX >> tempY >> tempActiv);
        Tile *tempTile = getGameMap()->getTile(tempX, tempY);
        OD_ASSERT_TRUE_MSG(tempTile != nullptr, "tile=" + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        if (tempTile != nullptr)
        {
            addCoveredTile(tempTile, Trap::DEFAULT_TILE_HP);
            tempTile->setSeat(getSeat());
            if(tempActiv != 0)
                activate(tempTile);
        }
    }
}

std::istream& operator>>(std::istream& is, TrapType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<TrapType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const TrapType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}

ODPacket& operator>>(ODPacket& is, TrapType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<TrapType>(tmp);
    return is;
}

ODPacket& operator<<(ODPacket& os, const TrapType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}
