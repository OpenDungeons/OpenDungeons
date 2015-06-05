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

#include "entities/Building.h"
#include "entities/Creature.h"
#include "entities/GameEntity.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "traps/Trap.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

//TODO: THis is currently duplicated here and in tile
const std::string TILE_PREFIX = "Tile_";
const std::string TILE_SCANF = TILE_PREFIX + "%i_%i";

void Tile::setFullness(double f)
{
    double oldFullness = getFullness();

    mFullness = f;

    // If the tile was marked for digging and has been dug out, unmark it and set its fullness to 0.
    if (mFullness == 0.0 && isMarkedForDiggingByAnySeat())
    {
        setMarkedForDiggingForAllPlayersExcept(false, nullptr);
    }

    if ((oldFullness > 0.0) && (mFullness == 0.0))
    {
        // Do a flood fill to update the contiguous region touching the tile.
        for(Seat* seat : mGameMap->getSeats())
            getGameMap()->refreshFloodFill(seat, this);
    }
}

void Tile::createMeshLocal()
{
    EntityBase::createMeshLocal();

    if(mIsOnServerMap)
        return;

    RenderManager::getSingleton().rrCreateTile(*this, *getGameMap(), *getGameMap()->getLocalPlayer());
}

void Tile::destroyMeshLocal()
{
    EntityBase::destroyMeshLocal();

    if(mIsOnServerMap)
        return;

    RenderManager::getSingleton().rrDestroyTile(*this);
}

bool Tile::isBuildableUpon(Seat* seat) const
{
    if(isFullTile())
        return false;
    if(getIsBuilding())
        return false;
    if(!isClaimedForSeat(seat))
        return false;

    return true;
}

void Tile::setCoveringBuilding(Building *building)
{
    if(mCoveringBuilding == building)
        return;

    std::vector<Seat*> seatsNotif;
    // We set the tile as dirty for all seats if needed (we have to check because we
    // don't want to refresh tiles for traps for enemy players)
    if(mCoveringBuilding != nullptr)
    {
        for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        {
            if(!mCoveringBuilding->shouldSetCoveringTileDirty(seatChanged.first, this))
                continue;

            seatChanged.second = true;
        }
    }
    mCoveringBuilding = building;
    mIsRoom = (getCoveringRoom() != nullptr);
    mIsTrap = (getCoveringTrap() != nullptr);
    if(mCoveringBuilding != nullptr)
    {
        for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        {
            if(!mCoveringBuilding->shouldSetCoveringTileDirty(seatChanged.first, this))
                continue;

            seatChanged.second = true;
        }

        // Set the tile as claimed and of the team color of the building
        setSeat(mCoveringBuilding->getSeat());
        mClaimedPercentage = 1.0;
    }
}

bool Tile::isGroundClaimable(Seat* seat) const
{
    if(getFullness() > 0.0)
        return false;

    if(mType != TileType::dirt && mType != TileType::gold)
        return false;

    if((getCoveringBuilding() != nullptr) &&
        (!getCoveringBuilding()->isClaimable(seat)))
    {
        return false;
    }

    if(isClaimedForSeat(seat))
        return false;

    return true;
}


void Tile::updateFromPacket(ODPacket& is)
{
    int seatId;
    std::string meshName;
    std::stringstream ss;

    // We set the seat if there is one
    OD_ASSERT_TRUE(is >> mIsRoom);
    OD_ASSERT_TRUE(is >> mIsTrap);
    OD_ASSERT_TRUE(is >> mRefundPriceRoom);
    OD_ASSERT_TRUE(is >> mRefundPriceTrap);

    OD_ASSERT_TRUE(is >> seatId);

    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);

    OD_ASSERT_TRUE(is >> mScale);

    ss.str(std::string());
    ss << TILE_PREFIX;
    ss << getX();
    ss << "_";
    ss << getY();

    setName(ss.str());

    OD_ASSERT_TRUE(is >> mTileVisual);

    if(seatId == -1)
    {
        setSeat(nullptr);
    }
    else
    {
        Seat* seat = getGameMap()->getSeatById(seatId);
        if(seat != nullptr)
            setSeat(seat);

    }

    // We need to check if the tile is unmarked after reading the needed information.
    if(getMarkedForDigging(getGameMap()->getLocalPlayer()) &&
        !isDiggable(getGameMap()->getLocalPlayer()->getSeat()))
    {
        removePlayerMarkingTile(getGameMap()->getLocalPlayer());
    }
}

void Tile::loadFromLine(const std::string& line, Tile *t)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    int xLocation = Helper::toInt(elems[0]);
    int yLocation = Helper::toInt(elems[1]);

    std::stringstream tileName("");
    tileName << TILE_PREFIX;
    tileName << xLocation;
    tileName << "_";
    tileName << yLocation;

    t->setName(tileName.str());
    t->mX = xLocation;
    t->mY = yLocation;
    t->mPosition = Ogre::Vector3(static_cast<Ogre::Real>(t->mX), static_cast<Ogre::Real>(t->mY), 0.0f);

    TileType tileType = static_cast<TileType>(Helper::toInt(elems[2]));
    t->setType(tileType);

    // If the tile type is lava or water, we ignore fullness
    double fullness;
    switch(tileType)
    {
        case TileType::water:
        case TileType::lava:
            fullness = 0.0;
            break;

        default:
            fullness = Helper::toDouble(elems[3]);
            break;
    }
    t->setFullnessValue(fullness);

    bool shouldSetSeat = false;
    // We allow to set seat if the tile is dirt (full or not) or if it is gold (ground only)
    if(elems.size() >= 5)
    {
        if(tileType == TileType::dirt)
        {
            shouldSetSeat = true;
        }
        else if((tileType == TileType::gold) &&
            (fullness == 0.0))
        {
            shouldSetSeat = true;
        }
    }

    if(!shouldSetSeat)
    {
        t->setSeat(nullptr);
        return;
    }

    int seatId = Helper::toInt(elems[4]);
    Seat* seat = t->getGameMap()->getSeatById(seatId);
    if(seat == nullptr)
        return;
    t->setSeat(seat);
    t->mClaimedPercentage = 1.0;
}

void Tile::refreshMesh()
{
    if (!isMeshExisting())
        return;

    if(mIsOnServerMap)
        return;

    RenderManager::getSingleton().rrRefreshTile(*this, *getGameMap(), *getGameMap()->getLocalPlayer());
}

void Tile::setSelected(bool ss, const Player* pp)
{
    if (mSelected != ss)
    {
        mSelected = ss;

        RenderManager::getSingleton().rrTemporalMarkTile(this);
    }
}

void Tile::setMarkedForDiggingForAllPlayersExcept(bool s, Seat* exceptSeat)
{
    for (Player* player : getGameMap()->getPlayers())
    {
        if(exceptSeat == nullptr || (player->getSeat() != nullptr && !exceptSeat->isAlliedSeat(player->getSeat())))
            setMarkedForDigging(s, player);
    }
}


bool Tile::addEntity(GameEntity *entity)
{
    if(std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), entity) != mEntitiesInTile.end())
        return false;

    mEntitiesInTile.push_back(entity);
    return true;
}

bool Tile::removeEntity(GameEntity *entity)
{
    std::vector<GameEntity*>::iterator it = std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), entity);
    if(it == mEntitiesInTile.end())
        return false;

    mEntitiesInTile.erase(it);
    return true;
}


void Tile::claimForSeat(Seat* seat, double nDanceRate)
{
    // If there is a claimable building, we claim it
    if((getCoveringBuilding() != nullptr) &&
        (getCoveringBuilding()->isClaimable(seat)))
    {
        getCoveringBuilding()->claimForSeat(seat, this, nDanceRate);
        return;
    }

    // Claiming walls is less efficient than claiming ground
    if(getFullness() > 0)
        nDanceRate *= ConfigManager::getSingleton().getClaimingWallPenalty();

    // If the seat is allied, we add to it. If it is an enemy seat, we subtract from it.
    if (getSeat() != nullptr && getSeat()->isAlliedSeat(seat))
    {
        mClaimedPercentage += nDanceRate;
    }
    else
    {
        mClaimedPercentage -= nDanceRate;
        if (mClaimedPercentage <= 0.0)
        {
            // We notify the old seat that the tile is lost
            if(getSeat() != nullptr)
                getSeat()->notifyTileClaimedByEnemy(this);

            // The tile is not yet claimed, but it is now an allied seat.
            mClaimedPercentage *= -1.0;
            setSeat(seat);
            computeTileVisual();
            setDirtyForAllSeats();
        }
    }

    if ((getSeat() != nullptr) && (mClaimedPercentage >= 1.0) &&
        (getSeat()->isAlliedSeat(seat)))
    {
        claimTile(seat);
    }
}

void Tile::claimTile(Seat* seat)
{
    // Claim the tile.
    // We need this because if we are a client, the tile may be from a non allied seat
    setSeat(seat);
    mClaimedPercentage = 1.0;

    // If an enemy player had marked this tile to dig, we disable it
    setMarkedForDiggingForAllPlayersExcept(false, seat);

    computeTileVisual();
    setDirtyForAllSeats();

    // Force all the neighbors to recheck their meshes as we have updated this tile.
    for (Tile* tile : mNeighbors)
    {
        // Update potential active spots.
        Building* building = tile->getCoveringBuilding();
        if (building != nullptr)
        {
            building->updateActiveSpots();
            building->createMesh();
        }
    }
}

void Tile::unclaimTile()
{
    // Unclaim the tile.
    setSeat(nullptr);
    mClaimedPercentage = 0.0;

    computeTileVisual();
    setDirtyForAllSeats();

    // Force all the neighbors to recheck their meshes as we have updated this tile.
    for (Tile* tile : mNeighbors)
    {
        // Update potential active spots.
        Building* building = tile->getCoveringBuilding();
        if (building != nullptr)
        {
            building->updateActiveSpots();
            building->createMesh();
        }
    }
}

double Tile::digOut(double digRate, bool doScaleDigRate)
{
    if (doScaleDigRate)
        digRate = scaleDigRate(digRate);

    double amountDug = 0.0;

    if (getFullness() == 0.0 || mType == TileType::lava || mType == TileType::water || mType == TileType::rock)
        return 0.0;

    if (digRate >= mFullness)
    {
        amountDug = mFullness;
        setFullness(0.0);

        computeTileVisual();
        setDirtyForAllSeats();

        for (Tile* tile : mNeighbors)
        {
            // Update potential active spots.
            Building* building = tile->getCoveringBuilding();
            if (building != nullptr)
            {
                building->updateActiveSpots();
                building->createMesh();
            }
        }
    }
    else
    {
        amountDug = digRate;
        setFullness(mFullness - digRate);
    }

    return amountDug;
}

void Tile::fillWithAttackableCreatures(std::vector<GameEntity*>& entities, Seat* seat, bool invert)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        if(!entity->isAttackable(this, seat))
            continue;

        // The invert flag is used to determine whether we want to return a list of the creatures
        // allied with supplied seat or the contrary.
        if ((invert && !entity->getSeat()->isAlliedSeat(seat)) || (!invert
            && entity->getSeat()->isAlliedSeat(seat)))
        {
            // Add the current creature
            if (std::find(entities.begin(), entities.end(), entity) == entities.end())
                entities.push_back(entity);
        }
    }
}

void Tile::fillWithAttackableRoom(std::vector<GameEntity*>& entities, Seat* seat, bool invert)
{
    Room* room = getCoveringRoom();
    if((room != nullptr) &&
        room->isAttackable(this, seat))
    {
        if ((invert && !room->getSeat()->isAlliedSeat(seat)) || (!invert
            && room->getSeat()->isAlliedSeat(seat)))
        {
            // If the room is not in the list already then add it.
            if (std::find(entities.begin(), entities.end(), room) == entities.end())
                entities.push_back(room);
        }
    }
}

void Tile::fillWithAttackableTrap(std::vector<GameEntity*>& entities, Seat* seat, bool invert)
{
    Trap* trap = getCoveringTrap();
    if((trap != nullptr) &&
        trap->isAttackable(this, seat))
    {
        if ((invert && !trap->getSeat()->isAlliedSeat(seat)) || (!invert
            && trap->getSeat()->isAlliedSeat(seat)))
        {
            // If the trap is not in the list already then add it.
            if (std::find(entities.begin(), entities.end(), trap) == entities.end())
                entities.push_back(trap);
        }
    }
}

void Tile::fillWithCarryableEntities(std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        if(entity->getEntityCarryType() == EntityCarryType::notCarryable)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

void Tile::fillWithChickenEntities(std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        if(entity->getObjectType() != GameEntityType::chickenEntity)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

void Tile::fillWithCraftedTraps(std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        if(entity->getObjectType() != GameEntityType::craftedTrap)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

void Tile::fillWithEntities(std::vector<EntityBase*>& entities, SelectionEntityWanted entityWanted, Player* player) const
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        switch(entityWanted)
        {
            case SelectionEntityWanted::creatureAliveOwned:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(player->getSeat() != entity->getSeat())
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(creature->getHP() <= 0)
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveAllied:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(entity->getSeat() == nullptr)
                    continue;

                if(!player->getSeat()->isAlliedSeat(entity->getSeat()))
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(creature->getHP() <= 0)
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveEnemy:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(entity->getSeat() == nullptr)
                    continue;

                if(player->getSeat()->isAlliedSeat(entity->getSeat()))
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(creature->getHP() <= 0)
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAlive:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(creature->getHP() <= 0)
                    continue;

                break;
            }
            default:
            {
                static bool logMsg = false;
                if(!logMsg)
                {
                    logMsg = true;
                    OD_LOG_ERR("Wrong SelectionEntityWanted int=" + Helper::toString(static_cast<uint32_t>(entityWanted)));
                }
                continue;
            }
        }

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

bool Tile::addTreasuryObject(TreasuryObject* obj)
{
    if(!mIsOnServerMap)
        return true;

    if (std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), obj) != mEntitiesInTile.end())
        return false;

    // If there is already a treasury object, we merge it
    bool isMerged = false;
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        if(entity->getObjectType() != GameEntityType::treasuryObject)
            continue;

        TreasuryObject* treasury = static_cast<TreasuryObject*>(entity);
        treasury->mergeGold(obj);
        isMerged = true;
        break;
    }

    if(!isMerged)
        mEntitiesInTile.push_back(obj);

    return true;
}

Room* Tile::getCoveringRoom() const
{
    if(mCoveringBuilding == nullptr)
        return nullptr;

    if(mCoveringBuilding->getObjectType() != GameEntityType::room)
        return nullptr;

    return static_cast<Room*>(mCoveringBuilding);
}

Trap* Tile::getCoveringTrap() const
{
    if(mCoveringBuilding == nullptr)
        return nullptr;

    if(mCoveringBuilding->getObjectType() != GameEntityType::trap)
        return nullptr;

    return static_cast<Trap*>(mCoveringBuilding);
}

void Tile::computeVisibleTiles()
{
    if(!getGameMap()->getIsFOWActivated())
    {
        // If the FOW is deactivated, we allow vision for every seat
        for(Seat* seat : getGameMap()->getSeats())
            notifyVision(seat);

        return;
    }

    if(!isClaimed())
        return;

    // A claimed tile can see it self and its neighboors
    notifyVision(getSeat());
    for(Tile* tile : mNeighbors)
    {
        tile->notifyVision(getSeat());
    }
}

void Tile::setDirtyForAllSeats()
{
    if(!mIsOnServerMap)
        return;

    for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        seatChanged.second = true;
}

void Tile::notifyEntitiesSeatsWithVision()
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        entity->notifySeatsWithVision(mSeatsWithVision);
    }
}


bool Tile::isFullTile() const
{
    if(mIsOnServerMap)
    {
        return getFullness() > 0.0;
    }
    else
    {
        switch(mTileVisual)
        {
            case TileVisual::claimedFull:
            case TileVisual::dirtFull:
            case TileVisual::goldFull:
            case TileVisual::rockFull:
                return true;
            default:
                return false;
        }
    }
}

bool Tile::permitsVision()
{
    if(isFullTile())
        return false;

    if((getCoveringBuilding() != nullptr) &&
       (!getCoveringBuilding()->permitsVision(this)))
    {
        return false;
    }

    return true;
}
