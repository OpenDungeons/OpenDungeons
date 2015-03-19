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
#include "utils/Helper.h"
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

std::string Trap::getTrapNameFromTrapType(TrapType t)
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
            return "UnknownTrapType enum=" + Helper::toString(static_cast<int>(t));
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

    for(Seat* seat : getGameMap()->getSeats())
    {
        for(Tile* tile : mCoveredTiles)
            seat->notifyBuildingRemovedFromGameMap(this, tile);
        for(Tile* tile : mCoveredTilesDestroyed)
            seat->notifyBuildingRemovedFromGameMap(this, tile);
    }

    removeAllBuildingObjects();
    getGameMap()->removeActiveObject(this);
}

void Trap::doUpkeep()
{
    Building::doUpkeep();

    // We remove trap entities if we can
    for(auto it = mTrapEntitiesWaitingRemove.begin(); it != mTrapEntitiesWaitingRemove.end();)
    {
        RenderedMovableEntity* trapEntity = *it;
        if(!trapEntity->notifyRemoveAsked())
        {
            ++it;
            continue;
        }

        removeBuildingObject(trapEntity);
        it = mTrapEntitiesWaitingRemove.erase(it);
    }

    if (numCoveredTiles() <= 0)
        return;

    for(Tile* tile : mCoveredTiles)
    {
        // If the trap is deactivated, it cannot shoot
        TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData[tile]);
        if (!trapTileData->isActivated())
            continue;

        if(trapTileData->decreaseReloadTime())
            continue;

        if(shoot(tile))
        {
            trapTileData->setReloadTime(mReloadTime);
            if(!trapTileData->decreaseShoot())
                deactivate(tile);

            const std::vector<Seat*>& seats = tile->getSeatsWithVision();
            TrapEntity* trapEntity = trapTileData->getTrapEntity();
            trapEntity->seatsSawTriggering(seats);

            for(Seat* seat : trapEntity->getSeatsNotHidden())
                seat->setVisibleBuildingOnTile(this, tile);

            // Warn the player the trap has triggered
            GameMap* gameMap = getGameMap();
            if (gameMap->isServerGameMap())
                gameMap->playerIsFighting(gameMap->getPlayerBySeat(getSeat()), tile);
        }
    }
}

int32_t Trap::getNbNeededCraftedTrap() const
{
    int32_t nbNeededCraftedTrap = 0;
    for(Tile* tile : mCoveredTiles)
    {
        if(mTileData.count(tile) <= 0)
            continue;

        TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
        if (trapTileData->isActivated())
            continue;

        if(trapTileData->getCarriedCraftedTrap() != nullptr)
            continue;

        ++nbNeededCraftedTrap;
    }

    return nbNeededCraftedTrap;
}

bool Trap::removeCoveredTile(Tile* t)
{
    if(!Building::removeCoveredTile(t))
        return false;

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(t));
    trapTileData->setRemoveTrap(true);

    return true;
}

void Trap::updateActiveSpots()
{
    // Active spots are handled by the server only
    if(!getGameMap()->isServerGameMap())
        return;

    // For a trap, by default, every tile is an active spot
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        TrapTileData* trapTileData = static_cast<TrapTileData*>(p.second);
        if(trapTileData->getTrapEntity() == nullptr)
        {
            RenderedMovableEntity* obj = notifyActiveSpotCreated(p.first);
            if(obj == nullptr)
                continue;

            addBuildingObject(p.first, obj);
            continue;
        }

        if(trapTileData->getRemoveTrap())
        {
            trapTileData->setRemoveTrap(false);
            if(mBuildingObjects.count(p.first) <= 0)
                continue;

            RenderedMovableEntity* trapEntity = mBuildingObjects.at(p.first);
            if(trapEntity->notifyRemoveAsked())
                removeBuildingObject(p.first);
            else
                mTrapEntitiesWaitingRemove.push_back(trapEntity);

            continue;
        }
    }

    // We restore the vision if we need to
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        TrapTileData* trapTileData = static_cast<TrapTileData*>(p.second);
        if(trapTileData->mSeatsVision.empty())
            continue;

        TrapEntity* trapEntity = trapTileData->getTrapEntity();
        if(trapEntity == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(p.first));
            continue;
        }

        trapEntity->seatsSawTriggering(trapTileData->mSeatsVision);
        trapEntity->notifySeatsWithVision(trapTileData->mSeatsVision);
        for(Seat* seat : trapEntity->getSeatsNotHidden())
            seat->setVisibleBuildingOnTile(this, p.first);

        // We empty the list of seats with vision to make sure we don't do it again
        // if there is an active spots update
        trapTileData->mSeatsVision.clear();

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

    for(Seat* seat : trapEntity->getSeatsNotHidden())
        seat->setVisibleBuildingOnTile(this, tile);

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData[tile]);
    trapTileData->setTrapEntity(trapEntity);
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

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData[tile]);
    trapTileData->setActivated(true);
    trapTileData->setNbShootsBeforeDeactivation(mNbShootsBeforeDeactivation);
    trapTileData->setReloadTime(0);

    RenderedMovableEntity* entity = getBuildingObjectFromTile(tile);
    if (entity == nullptr)
        return;

    entity->setMeshOpacity(1.0f);
}

void Trap::deactivate(Tile* tile)
{
    if (tile == nullptr)
        return;

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData[tile]);
    trapTileData->setActivated(false);

    RenderedMovableEntity* entity = getBuildingObjectFromTile(tile);
    if (entity == nullptr)
        return;

    entity->setMeshOpacity(0.5f);
}

bool Trap::isActivated(Tile* tile) const
{
    std::map<Tile*, TileData*>::const_iterator it = mTileData.find(tile);
    if (it == mTileData.end())
        return false;

    TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
    return trapTileData->isActivated();
}

void Trap::setupTrap(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    setName(name);
    setSeat(seat);
    for(Tile* tile : tiles)
    {
        mCoveredTiles.push_back(tile);
        TrapTileData* trapTileData = createTileData(tile);
        mTileData[tile] = trapTileData;
        trapTileData->mHP = DEFAULT_TILE_HP;
        trapTileData->setReloadTime(mReloadTime);

        tile->setCoveringBuilding(this);
    }
}

int32_t Trap::getNeededWorkshopPointsPerTrap(TrapType trapType)
{
    switch(trapType)
    {
        case TrapType::nullTrapType:
            return 0;
        case TrapType::cannon:
            return ConfigManager::getSingleton().getTrapConfigInt32("CannonWorkshopPointsPerTile");
        case TrapType::spike:
            return ConfigManager::getSingleton().getTrapConfigInt32("SpikeWorkshopPointsPerTile");
        case TrapType::boulder:
            return ConfigManager::getSingleton().getTrapConfigInt32("BoulderWorkshopPointsPerTile");
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

    if(carriedEntity->getObjectType() != GameEntityType::craftedTrap)
        return false;

    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(carriedEntity);
    if(craftedTrap->getTrapType() != getType())
        return false;

    return true;
}

Tile* Trap::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    OD_ASSERT_TRUE_MSG(carriedEntity->getObjectType() == GameEntityType::craftedTrap,
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(carriedEntity->getObjectType() != GameEntityType::craftedTrap)
        return nullptr;

    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(carriedEntity);
    OD_ASSERT_TRUE_MSG(craftedTrap->getTrapType() == getType(),
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(craftedTrap->getTrapType() != getType())
        return nullptr;

    for(Tile* tile : mCoveredTiles)
    {
        if(mTileData.count(tile) <= 0)
            continue;

        TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
        if (trapTileData->isActivated())
            continue;

        if(trapTileData->getCarriedCraftedTrap() != nullptr)
            continue;

        // We can accept the craftedTrap on this tile
        trapTileData->setCarriedCraftedTrap(craftedTrap);
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
        if(mTileData.count(tile) <= 0)
            continue;

        TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
        if(trapTileData->isActivated())
            continue;
        if(trapTileData->getCarriedCraftedTrap() != carriedEntity)
            continue;

        // We check if the carrier is at the expected destination
        Tile* carrierTile = carrier->getPositionTile();
        OD_ASSERT_TRUE_MSG(carrierTile != nullptr, "carrier=" + carrier->getName());
        if(carrierTile == nullptr)
        {
            trapTileData->setCarriedCraftedTrap(nullptr);
            return;
        }

        Tile* tileExpected = getGameMap()->getTile(tile->getX(), tile->getY());
        if(tileExpected != carrierTile)
        {
            trapTileData->setCarriedCraftedTrap(nullptr);
            return;
        }

        // The carrier has brought carried trap
        CraftedTrap* craftedTrap = trapTileData->getCarriedCraftedTrap();
        OD_ASSERT_TRUE_MSG(tile->removeEntity(craftedTrap), "trap=" + getName()
            + ", craftedTrap=" + craftedTrap->getName()
            + ", tile=" + Tile::displayAsString(tile));
        craftedTrap->removeFromGameMap();
        craftedTrap->deleteYourself();
        activate(tile);
        trapTileData->setCarriedCraftedTrap(nullptr);
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
    if(mTileData.count(tile) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "name=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return false;
    }

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
    const std::vector<Seat*>& seatsNotHidden = trapTileData->getTrapEntity()->getSeatsNotHidden();
    if(std::find(seatsNotHidden.begin(), seatsNotHidden.end(), seat) == seatsNotHidden.end())
        return false;

    return true;
}

std::string Trap::getTrapStreamFormat()
{
    return "typeTrap\tname\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY\tisActivated(0/1)\t\tSubsequent Lines: optional specific data";
}

void Trap::exportHeadersToStream(std::ostream& os) const
{
    os << getType() << "\t";
}

void Trap::exportTileDataToStream(std::ostream& os, Tile* tile, TileData* tileData) const
{
    TrapTileData* trapTileData = static_cast<TrapTileData*>(tileData);
    os << "\t" << (trapTileData->isActivated() ? 1 : 0);
    if(getGameMap()->isInEditorMode())
        return;

    os << "\t" << trapTileData->mHP;
    os << "\t" << trapTileData->getReloadTime();
    os << "\t" << trapTileData->getNbShootsBeforeDeactivation();

    // We only save enemy seats that have vision on the building
    std::vector<Seat*> seatsToSave;
    for(Seat* seat : getGameMap()->getSeats())
    {
        if(getSeat()->isAlliedSeat(seat))
            continue;

        if(!seat->haveVisionOnBuilding(this, tile))
            continue;

        seatsToSave.push_back(seat);
    }
    uint32_t nbSeatsVision = seatsToSave.size();
    os << "\t" << nbSeatsVision;
    for(Seat* seat : seatsToSave)
        os << "\t" << seat->getId();
}

void Trap::importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData)
{
    TrapTileData* trapTileData = static_cast<TrapTileData*>(tileData);
    int isTrapActiv;
    OD_ASSERT_TRUE(is >> isTrapActiv);
    if(is.eof())
    {
        // Default initialization
        trapTileData->mHP = DEFAULT_TILE_HP;
        mCoveredTiles.push_back(tile);
        tile->setCoveringBuilding(this);
        if(isTrapActiv != 0)
            activate(tile);

        return;
    }

    // We read saved trap state
    double tileHealth;
    uint32_t reloadTime;
    int32_t nbShootsBeforeDeactivation;
    int32_t nbSeatsVision;
    OD_ASSERT_TRUE(is >> tileHealth);
    OD_ASSERT_TRUE(is >> reloadTime);
    OD_ASSERT_TRUE(is >> nbShootsBeforeDeactivation);
    OD_ASSERT_TRUE(is >> nbSeatsVision);

    if(isTrapActiv != 0)
        activate(tile);

    trapTileData->mHP = tileHealth;
    if(trapTileData->mHP > 0.0)
    {
        mCoveredTiles.push_back(tile);
        tile->setCoveringBuilding(this);
    }
    else
    {
        mCoveredTilesDestroyed.push_back(tile);
    }
    trapTileData->setNbShootsBeforeDeactivation(nbShootsBeforeDeactivation);
    trapTileData->setReloadTime(reloadTime);
    trapTileData->setIsWorking(tileHealth > 0.0);

    GameMap* gameMap = getGameMap();
    while(nbSeatsVision > 0)
    {
        --nbSeatsVision;
        int seatId;
        OD_ASSERT_TRUE(is >> seatId);
        Seat* seat = gameMap->getSeatById(seatId);
        if(seat == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "trap=" + getName() + ", seatId=" + Helper::toString(seatId));
            continue;
        }
        trapTileData->mSeatsVision.push_back(seat);
    }
}

TrapTileData* Trap::createTileData(Tile* tile)
{
    return new TrapTileData();
}

bool Trap::isTileVisibleForSeat(Tile* tile, Seat* seat) const
{
    if(mTileData.count(tile) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return false;
    }

    const TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
    if(trapTileData->getTrapEntity() == nullptr)
        return false;

    const std::vector<Seat*>& seatsNotHidden = trapTileData->getTrapEntity()->getSeatsNotHidden();
    if(std::find(seatsNotHidden.begin(), seatsNotHidden.end(), seat) == seatsNotHidden.end())
        return false;

    return true;
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
