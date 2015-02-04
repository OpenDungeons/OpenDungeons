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

#include "entities/ChickenEntity.h"

#include "network/ODPacket.h"
#include "gamemap/GameMap.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <iostream>

const int32_t NB_TURNS_OUTSIDE_HATCHERY_BEFORE_DIE = 30;
const int32_t NB_TURNS_DIE_BEFORE_REMOVE = 5;

ChickenEntity::ChickenEntity(GameMap* gameMap, const std::string& hatcheryName) :
    RenderedMovableEntity(gameMap, hatcheryName, "Chicken", 0.0f, false),
    mChickenState(ChickenState::free),
    mNbTurnOutsideHatchery(0),
    mNbTurnDie(0),
    mIsSlapped(false)
{
}

ChickenEntity::ChickenEntity(GameMap* gameMap) :
    RenderedMovableEntity(gameMap),
    mChickenState(ChickenState::free),
    mNbTurnOutsideHatchery(0),
    mNbTurnDie(0),
    mIsSlapped(false)
{
    setMeshName("Chicken");
}

void ChickenEntity::doUpkeep()
{
    if(!getIsOnMap())
        return;
    if(mChickenState == ChickenState::dead)
       return;

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;

    if(mChickenState == ChickenState::eaten)
        mChickenState = ChickenState::dead;

    if(mChickenState == ChickenState::dying)
    {
        if(mNbTurnDie < NB_TURNS_DIE_BEFORE_REMOVE)
        {
            ++mNbTurnDie;
            return;
        }
        mChickenState = ChickenState::dead;
    }

    // If we are dead, we remove the chicken
    if(mChickenState == ChickenState::dead)
    {
        // No need to remove the chicken from its tile as it has already been in eatChicken
        // or when dying
        removeFromGameMap();
        deleteYourself();
        return;
    }


    Room* currentHatchery = nullptr;
    if(tile->getCoveringRoom() != nullptr)
    {
        Room* room = tile->getCoveringRoom();
        if(room->getType() == Room::RoomType::hatchery)
        {
            currentHatchery = room;
        }
    }

    if(currentHatchery != nullptr)
        mNbTurnOutsideHatchery = 0;
    else
        ++mNbTurnOutsideHatchery;

    // If we are outside a hatchery for too long, we die
    if(mIsSlapped || (mNbTurnOutsideHatchery >= NB_TURNS_OUTSIDE_HATCHERY_BEFORE_DIE))
    {
        mChickenState = ChickenState::dying;
        clearDestinations();
        setAnimationState("Die", false);
        return;
    }

    // Handle normal behaviour : move or pick (if not already moving)
    if(isMoving())
        return;

    // We might not move
    if(Random::Int(1,2) == 1)
    {
        setAnimationState("Pick");
        return;
    }

    int posChickenX = tile->getX();
    int posChickenY = tile->getY();
    std::vector<Tile*> possibleTileMove;
    // We move chickens from 1 tile only to avoid slow creatures from running
    // for ages when the hatchery is big
    addTileToListIfPossible(posChickenX - 1, posChickenY, currentHatchery, possibleTileMove);
    addTileToListIfPossible(posChickenX + 1, posChickenY, currentHatchery, possibleTileMove);
    addTileToListIfPossible(posChickenX, posChickenY - 1, currentHatchery, possibleTileMove);
    addTileToListIfPossible(posChickenX, posChickenY + 1, currentHatchery, possibleTileMove);

    // We cannot move. That can happen if all the nearby tiles have been destroyed
    if(possibleTileMove.empty())
        return;

    uint32_t indexTile = Random::Uint(0, possibleTileMove.size() - 1);
    Tile* tileDest = possibleTileMove[indexTile];
    Ogre::Real x = static_cast<Ogre::Real>(tileDest->getX());
    Ogre::Real y = static_cast<Ogre::Real>(tileDest->getY());

    addDestination(x, y);
    setAnimationState("Walk");
}

void ChickenEntity::addTileToListIfPossible(int x, int y, Room* currentHatchery, std::vector<Tile*>& possibleTileMove)
{
    Tile* tile = getGameMap()->getTile(x, y);
    if(tile == nullptr)
        return;

    if(tile->getFullness() > 0.0)
        return;

    Tile::TileType tileType = tile->getType();
    switch(tileType)
    {
        case Tile::TileType::dirt:
        case Tile::TileType::gold:
        case Tile::TileType::claimed:
        {
            break;
        }
        default:
            return;

    }

    if((currentHatchery != nullptr) && (currentHatchery != tile->getCoveringBuilding()))
        return;

    // We can move on this tile
    possibleTileMove.push_back(tile);
}

bool ChickenEntity::tryPickup(Seat* seat, bool isEditorMode)
{
    if(!getIsOnMap())
        return false;

    // We do not let it be picked up as it will be removed during next upkeep. However, this is
    // true only on server side. On client side, if a chicken is available, it can be picked up (it will
    // be up to the server to validate or not) because the client do not know the chicken state.
    if(getGameMap()->isServerGameMap() && (mChickenState != ChickenState::free))
        return false;

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return false;

    if(isEditorMode)
        return true;

    if(!tile->isClaimedForSeat(seat))
        return false;

    if(tile->getSeat() != seat)
        return false;

    return true;
}

void ChickenEntity::pickup()
{
    Tile* tile = getPositionTile();
    RenderedMovableEntity::pickup();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;
    OD_ASSERT_TRUE(tile->removeEntity(this));
}

bool ChickenEntity::tryDrop(Seat* seat, Tile* tile, bool isEditorMode)
{
    if (tile->getFullness() > 0.0)
        return false;

    // In editor mode, we allow to drop an object in dirt, claimed or gold tiles
    if(isEditorMode && (tile->getType() == Tile::dirt || tile->getType() == Tile::gold || tile->getType() == Tile::claimed))
        return true;

    // we cannot drop a chicken on a tile we don't see
    if(!seat->hasVisionOnTile(tile))
        return false;

    // Otherwise, we allow to drop an object only on allied claimed tiles
    if(tile->getType() == Tile::claimed && tile->getSeat() != nullptr && tile->getSeat()->isAlliedSeat(seat))
        return true;

    return false;
}

bool ChickenEntity::eatChicken(Creature* creature)
{
    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return false;

    if(mChickenState != ChickenState::free)
        return false;

    OD_ASSERT_TRUE(tile->removeEntity(this));
    mChickenState = ChickenState::eaten;
    clearDestinations();
    return true;
}

bool ChickenEntity::canSlap(Seat* seat, bool isEditorMode)
{
    if(!getIsOnMap())
        return false;

    // We do not let it be picked up as it will be removed during next upkeep. However, this is
    // true only on server side. On client side, if a chicken is available, it can be picked up (it will
    // be up to the server to validate or not) because the client do not know the chicken state.
    if(getGameMap()->isServerGameMap() && (mChickenState != ChickenState::free))
        return false;

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return false;

    if(isEditorMode)
        return !mIsSlapped;

    if(!tile->isClaimedForSeat(seat))
        return false;

    if(tile->getSeat() != seat)
        return false;

    return !mIsSlapped;
}

ChickenEntity* ChickenEntity::getChickenEntityFromStream(GameMap* gameMap, std::istream& is)
{
    ChickenEntity* obj = new ChickenEntity(gameMap);
    return obj;
}

ChickenEntity* ChickenEntity::getChickenEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    ChickenEntity* obj = new ChickenEntity(gameMap);
    return obj;
}

const char* ChickenEntity::getFormat()
{
    // TODO : implement saving/loading in the level file
    return "position";
}
