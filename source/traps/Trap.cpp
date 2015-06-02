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

#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/RenderedMovableEntity.h"
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

void Trap::addToGameMap()
{
    getGameMap()->addTrap(this);
    setIsOnMap(true);
    getGameMap()->addActiveObject(this);
}

void Trap::removeFromGameMap()
{
    getGameMap()->removeTrap(this);
    setIsOnMap(false);
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
}

RenderedMovableEntity* Trap::notifyActiveSpotCreated(Tile* tile)
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
    OD_ASSERT_TRUE_MSG(carriedEntity->getObjectType() == GameEntityType::craftedTrap,
        "Trap=" + getName() + ", entity=" + carriedEntity->getName());
    if(carriedEntity->getObjectType() != GameEntityType::craftedTrap)
        return nullptr;

    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(carriedEntity);
    OD_ASSERT_TRUE_MSG(craftedTrap->getTrapType() == getType(),
        "Trap=" + getName() + ", entity=" + carriedEntity->getName());
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
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(p.first));
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
    OD_ASSERT_TRUE(is >> trapTileData->mClaimedValue);
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
    if(std::find(trapTileData->mSeatsVision.begin(), trapTileData->mSeatsVision.end(), seat) == trapTileData->mSeatsVision.end())
        return false;

    return true;
}

bool Trap::isClaimable(Seat* seat) const
{
    if(getSeat()->canBuildingBeDestroyedBy(seat))
        return false;

    return true;
}

void Trap::claimForSeat(Seat* seat, Tile* tile, double danceRate)
{
    if(mTileData.count(tile) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return;
    }

    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
    if(danceRate < trapTileData->mClaimedValue)
    {
        trapTileData->mClaimedValue -= danceRate;
        return;
    }

    trapTileData->mHP = 0.0;
    tile->claimTile(seat);
}
std::string Trap::formatTrapPrice(TrapType type, uint32_t price)
{
    return TrapManager::getTrapNameFromTrapType(type) + " [" + Helper::toString(price)+ " gold]";
}

void Trap::checkBuildTrapDefault(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    int32_t pricePerTarget = TrapManager::costPerTile(type);
    int32_t playerGold = static_cast<int32_t>(player->getSeat()->getGold());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerGold < pricePerTarget)
        {
            std::string txt = formatTrapPrice(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatTrapPrice(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    std::vector<Tile*> buildableTiles = gameMap->getBuildableTilesForPlayerInArea(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY, player);

    if(inputManager.mCommandState == InputCommandState::building)
        inputCommand.selectTiles(buildableTiles);

    if(buildableTiles.empty())
    {
        std::string txt = formatTrapPrice(type, 0);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    int32_t priceTotal = static_cast<int32_t>(buildableTiles.size()) * pricePerTarget;
    if(playerGold < priceTotal)
    {
        std::string txt = formatTrapPrice(type, priceTotal);
        inputCommand.displayText(Ogre::ColourValue::Red, txt);
        return;
    }

    std::string txt = formatTrapPrice(type, priceTotal);
    inputCommand.displayText(Ogre::ColourValue::White, txt);

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    ClientNotification *clientNotification = TrapManager::createTrapClientNotification(type);
    uint32_t nbTiles = buildableTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool Trap::getTrapTilesDefault(std::vector<Tile*>& tiles, GameMap* gameMap, Player* player, ODPacket& packet)
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);

    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
        {
            OD_ASSERT_TRUE(false);
            return false;
        }

        if(!tile->isBuildableUpon(player->getSeat()))
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(player->getSeat()->getId()));
            continue;
        }

        tiles.push_back(tile);
    }

    return true;
}

bool Trap::buildTrapDefault(GameMap* gameMap, Trap* trap, Seat* seat, const std::vector<Tile*>& tiles)
{
    if(tiles.empty())
        return false;

    trap->setupTrap(gameMap->nextUniqueNameTrap(trap->getMeshName()), seat, tiles);
    trap->addToGameMap();
    trap->createMesh();

    if((seat->getPlayer() != nullptr) &&
       (seat->getPlayer()->getIsHuman()))
    {
        // We notify the clients with vision of the changed tiles. Note that we need
        // to calculate per seat since they could have vision on different parts of the building
        std::map<Seat*,std::vector<Tile*>> tilesPerSeat;
        const std::vector<Seat*>& seats = gameMap->getSeats();
        for(Seat* tmpSeat : seats)
        {
            if(tmpSeat->getPlayer() == nullptr)
                continue;
            if(!tmpSeat->getPlayer()->getIsHuman())
                continue;

            for(Tile* tile : tiles)
            {
                if(!tmpSeat->hasVisionOnTile(tile))
                    continue;

                tile->changeNotifiedForSeat(tmpSeat);
                tilesPerSeat[tmpSeat].push_back(tile);
            }
        }

        for(const std::pair<Seat* const,std::vector<Tile*>>& p : tilesPerSeat)
        {
            uint32_t nbTiles = p.second.size();
            ServerNotification serverNotification(
                ServerNotificationType::refreshTiles, p.first->getPlayer());
            serverNotification.mPacket << nbTiles;
            for(Tile* tile : p.second)
            {
                gameMap->tileToPacket(serverNotification.mPacket, tile);
                p.first->updateTileStateForSeat(tile);
                p.first->exportTileToPacket(serverNotification.mPacket, tile);
            }
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
    }

    trap->updateActiveSpots();

    return true;
}

void Trap::checkBuildTrapDefaultEditor(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    std::string txt = TrapManager::getTrapNameFromTrapType(type);
    inputCommand.displayText(Ogre::ColourValue::White, txt);
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    std::vector<Tile*> tiles = gameMap->rectangularRegion(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY);

    std::vector<Tile*> buildableTiles;
    for(Tile* tile : tiles)
    {
        // We accept any tile if there is no building
        if(tile->getIsBuilding())
            continue;

        buildableTiles.push_back(tile);
    }
    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectTiles(buildableTiles);
        return;
    }

    ClientNotification *clientNotification = TrapManager::createTrapClientNotificationEditor(type);
    uint32_t nbTiles = buildableTiles.size();
    int32_t seatId = inputManager.mSeatIdSelected;
    clientNotification->mPacket << seatId;
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool Trap::buildTrapDefaultEditor(GameMap* gameMap, Trap* trap, ODPacket& packet)
{
    int32_t seatId;
    OD_ASSERT_TRUE(packet >> seatId);
    Seat* seatTrap = gameMap->getSeatById(seatId);
    if(seatTrap == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "seatId=" + Helper::toString(seatId));
        return false;
    }

    std::vector<Tile*> tiles;
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);

    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
        {
            OD_ASSERT_TRUE(false);
            return false;
        }

        // If the tile is not buildable, we change it
        if(tile->getCoveringBuilding() != nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(seatId));
            continue;
        }

        tiles.push_back(tile);
        if((tile->getType() != TileType::gold) &&
           (tile->getType() != TileType::dirt))
        {
            tile->setType(TileType::dirt);
        }
        tile->setFullness(0.0);
        tile->claimTile(seatTrap);
        tile->computeTileVisual();
    }


    return buildTrapDefault(gameMap, trap, seatTrap, tiles);
}
