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

#include "traps/Trap.h"

#include "entities/BuildingObject.h"
#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "traps/TrapManager.h"
#include "traps/TrapType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <istream>
#include <ostream>

void TrapTileData::fireSeatsSawTriggering()
{
    if(mTrapEntity == nullptr)
        return;

    for(Seat* seat : mSeatsVision)
        mTrapEntity->seatSawTriggering(seat);
}

void TrapTileData::seatSawTriggering(Seat* seat)
{
    if(std::find(mSeatsVision.begin(), mSeatsVision.end(), seat) != mSeatsVision.end())
        return;

    mSeatsVision.push_back(seat);
    if(mTrapEntity == nullptr)
        return;

    mTrapEntity->seatSawTriggering(seat);
}

void TrapTileData::seatsSawTriggering(const std::vector<Seat*>& seats)
{
    for(Seat* seat : seats)
        seatSawTriggering(seat);
}

Trap::Trap(GameMap* gameMap) :
    Building(gameMap),
    mNbShootsBeforeDeactivation(0),
    mReloadTime(0),
    mMinDamage(0.0),
    mMaxDamage(0.0)
{
}

GameEntityType Trap::getObjectType() const
{
    return GameEntityType::trap;
}

void Trap::addToGameMap()
{
    getGameMap()->addTrap(this);
    getGameMap()->addActiveObject(this);
}

void Trap::removeFromGameMap()
{
    fireEntityRemoveFromGameMap();
    setIsOnMap(false);
    getGameMap()->removeTrap(this);
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
        BuildingObject* trapEntity = *it;
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
            trapTileData->seatsSawTriggering(seats);

            for(Seat* seat : trapTileData->mSeatsVision)
                seat->setVisibleBuildingOnTile(this, tile);
        }
    }
}

int32_t Trap::getNbNeededCraftedTrap() const
{
    int32_t nbNeededCraftedTrap = 0;
    for(Tile* tile : mCoveredTiles)
    {
        auto it = mTileData.find(tile);
        if(it == mTileData.end())
            continue;

        TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
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
    // For a trap, by default, every tile is an active spot
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        TrapTileData* trapTileData = static_cast<TrapTileData*>(p.second);
        if(trapTileData->getTrapEntity() == nullptr)
        {
            BuildingObject* obj = notifyActiveSpotCreated(p.first);
            if(obj == nullptr)
                continue;

            addBuildingObject(p.first, obj);
            continue;
        }

        if(trapTileData->getRemoveTrap())
        {
            trapTileData->setRemoveTrap(false);
            auto it = mBuildingObjects.find(p.first);
            if(it == mBuildingObjects.end())
                continue;

            BuildingObject* trapEntity = it->second;
            if(trapEntity->notifyRemoveAsked())
                removeBuildingObject(p.first);
            else
                mTrapEntitiesWaitingRemove.push_back(trapEntity);

            continue;
        }
    }
}

BuildingObject* Trap::notifyActiveSpotCreated(Tile* tile)
{
    TrapEntity* trapEntity = getTrapEntity(tile);
    if(trapEntity == nullptr)
        return nullptr;

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData[tile]);
    trapTileData->setTrapEntity(trapEntity);
    trapTileData->fireSeatsSawTriggering();

    for(Seat* seat : trapTileData->mSeatsVision)
        seat->setVisibleBuildingOnTile(this, tile);

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

    BuildingObject* entity = getBuildingObjectFromTile(tile);
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

    BuildingObject* entity = getBuildingObjectFromTile(tile);
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
    setIsOnMap(true);
    setName(name);
    setSeat(seat);
    std::vector<Seat*> alliedSeats = seat->getAlliedSeats();
    alliedSeats.push_back(seat);
    for(Tile* tile : tiles)
    {
        mCoveredTiles.push_back(tile);
        TrapTileData* trapTileData = createTileData(tile);
        mTileData[tile] = trapTileData;
        trapTileData->mHP = DEFAULT_TILE_HP;
        trapTileData->setReloadTime(mReloadTime);
        // Allied seats with the creator do see the trap from the start
        trapTileData->seatsSawTriggering(alliedSeats);
        tile->setCoveringBuilding(this);

        // In the editor, activate each trap tile by default
        if(getGameMap()->isInEditorMode())
            activate(tile);
    }
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
    if(carriedEntity->getObjectType() != GameEntityType::craftedTrap)
    {
        OD_LOG_ERR("Trap=" + getName() + ", entity=" + carriedEntity->getName());
        return nullptr;
    }

    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(carriedEntity);
    if(craftedTrap->getTrapType() != getType())
    {
        OD_LOG_ERR("Trap=" + getName() + ", entity=" + carriedEntity->getName());
        return nullptr;
    }

    for(Tile* tile : mCoveredTiles)
    {
        auto it = mTileData.find(tile);
        if(it == mTileData.end())
            continue;

        TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
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
        auto it = mTileData.find(tile);
        if(it == mTileData.end())
            continue;

        TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
        if(trapTileData->isActivated())
            continue;
        if(trapTileData->getCarriedCraftedTrap() != carriedEntity)
            continue;

        // We check if the carrier is at the expected destination
        Tile* carrierTile = carrier->getPositionTile();
        if(carrierTile == nullptr)
        {
            OD_LOG_ERR("carrier=" + carrier->getName());
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
        craftedTrap->removeEntityFromPositionTile();
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
    auto it = mTileData.find(tile);
    if(it == mTileData.end())
    {
        OD_LOG_ERR("name=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return false;
    }

    TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
    if(std::find(trapTileData->mSeatsVision.begin(), trapTileData->mSeatsVision.end(), seat) == trapTileData->mSeatsVision.end())
        return false;

    return true;
}

void Trap::restoreInitialEntityState()
{
    // We restore the vision if we need to
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        TrapTileData* trapTileData = static_cast<TrapTileData*>(p.second);
        if(trapTileData->mSeatsVision.empty())
            continue;

        for(Seat* seat : p.second->mSeatsVision)
            seat->setVisibleBuildingOnTile(this, p.first);

        trapTileData->seatsSawTriggering(trapTileData->mSeatsVision);
        TrapEntity* trapEntity = trapTileData->getTrapEntity();
        if(trapEntity == nullptr)
        {
            OD_LOG_ERR("tile=" + Tile::displayAsString(p.first));
            continue;
        }

        trapEntity->notifySeatsWithVision(trapTileData->mSeatsVision);
        for(Seat* seat : trapTileData->mSeatsVision)
            seat->setVisibleBuildingOnTile(this, p.first);
    }
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
    os << "\t" << trapTileData->mClaimedValue;

    // We only save enemy seats that have vision on the building
    std::vector<Seat*> seatsToSave;
    for(Seat* seat : trapTileData->mSeatsVision)
    {
        if(getSeat()->isAlliedSeat(seat))
            continue;

        seatsToSave.push_back(seat);
    }
    uint32_t nbSeatsVision = seatsToSave.size();
    os << "\t" << nbSeatsVision;
    for(Seat* seat : seatsToSave)
        os << "\t" << seat->getId();
}

bool Trap::importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData)
{
    TrapTileData* trapTileData = static_cast<TrapTileData*>(tileData);
    int isTrapActiv;
    if(!(is >> isTrapActiv))
        return false;

    if(is.eof())
    {
        // Default initialization
        trapTileData->mHP = DEFAULT_TILE_HP;
        mCoveredTiles.push_back(tile);
        tile->setCoveringBuilding(this);
        if(isTrapActiv != 0)
            activate(tile);

        return true;
    }

    // We read saved trap state
    double tileHealth;
    uint32_t reloadTime;
    int32_t nbShootsBeforeDeactivation;
    int32_t nbSeatsVision;
    if(!(is >> tileHealth))
        return false;
    if(!(is >> reloadTime))
        return false;
    if(!(is >> nbShootsBeforeDeactivation))
        return false;
    if(!(is >> trapTileData->mClaimedValue))
        return false;
    if(!(is >> nbSeatsVision))
        return false;

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
        if(!(is >> seatId))
            return false;

        Seat* seat = gameMap->getSeatById(seatId);
        if(seat == nullptr)
        {
            OD_LOG_ERR("trap=" + getName() + ", seatId=" + Helper::toString(seatId));
            continue;
        }
        trapTileData->seatSawTriggering(seat);
    }

    return true;
}

TrapTileData* Trap::createTileData(Tile* tile)
{
    return new TrapTileData();
}

bool Trap::isTileVisibleForSeat(Tile* tile, Seat* seat) const
{
    if(getGameMap()->isInEditorMode())
        return true;

    auto it = mTileData.find(tile);
    if(it == mTileData.end())
    {
        OD_LOG_ERR("trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return false;
    }

    const TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
    if(std::find(trapTileData->mSeatsVision.begin(), trapTileData->mSeatsVision.end(), seat) == trapTileData->mSeatsVision.end())
        return false;

    return true;
}

bool Trap::isClaimable(Seat* seat) const
{
    return !getSeat()->isAlliedSeat(seat);
}

void Trap::claimForSeat(Seat* seat, Tile* tile, double danceRate)
{
    auto it = mTileData.find(tile);
    if(it == mTileData.end())
    {
        OD_LOG_ERR("trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return;
    }

    TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
    if(danceRate < trapTileData->mClaimedValue)
    {
        trapTileData->mClaimedValue -= danceRate;
        return;
    }

    trapTileData->mHP = 0.0;
    tile->claimTile(seat);
}

bool Trap::sortForMapSave(Trap* t1, Trap* t2)
{
    // We sort room by seat id then meshName
    int seatId1 = t1->getSeat()->getId();
    int seatId2 = t2->getSeat()->getId();
    if(seatId1 != seatId2)
        return seatId1 < seatId2;

    if(t1->getType() != t2->getType())
        return static_cast<uint32_t>(t1->getType()) < static_cast<uint32_t>(t2->getType());

    return t1->getName().compare(t2->getName()) < 0;
}

bool Trap::shouldSetCoveringTileDirty(Seat* seat, Tile* tile)
{
    auto it = mTileData.find(tile);
    if(it == mTileData.end())
    {
        OD_LOG_ERR("trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return true;
    }

    TrapTileData* trapTileData = static_cast<TrapTileData*>(it->second);
    if(std::find(trapTileData->mSeatsVision.begin(), trapTileData->mSeatsVision.end(), seat) == trapTileData->mSeatsVision.end())
        return false;

    return true;
}

void Trap::fireTrapSound(Tile& tile, const std::string& soundFamily)
{
    std::string sound = "Traps/" + soundFamily;
    for(Seat* seat : tile.getSeatsWithVision())
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::playSpatialSound, seat->getPlayer());
        serverNotification->mPacket << sound << tile.getX() << tile.getY();
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

bool Trap::importTrapFromStream(Trap& trap, std::istream& is)
{
    return trap.importFromStream(is);
}
