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

#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "network/ODPacket.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <iostream>

const int32_t NB_TURNS_OUTSIDE_HATCHERY_BEFORE_DIE = 30;
const int32_t NB_TURNS_DIE_BEFORE_REMOVE = 5;

ChickenEntity::ChickenEntity(GameMap* gameMap, bool isOnServerMap, const std::string& hatcheryName) :
    RenderedMovableEntity(gameMap, isOnServerMap, hatcheryName, "Chicken", 0.0f, false),
    mChickenState(ChickenState::free),
    mNbTurnOutsideHatchery(0),
    mNbTurnDie(0),
    mIsSlapped(false),
    mLockedEat(false)
{
}

ChickenEntity::ChickenEntity(GameMap* gameMap, bool isOnServerMap) :
    RenderedMovableEntity(gameMap, isOnServerMap),
    mChickenState(ChickenState::free),
    mNbTurnOutsideHatchery(0),
    mNbTurnDie(0),
    mIsSlapped(false),
    mLockedEat(false)
{
    setMeshName("Chicken");
}

void ChickenEntity::doUpkeep()
{
    // If we are dead, we remove the chicken
    if(mChickenState == ChickenState::eaten)
    {
        // No need to remove the chicken from its tile as it has already been in eatChicken
        // or when dying
        removeFromGameMap();
        deleteYourself();
        return;
    }

    if(!getIsOnMap())
        return;

    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return;
    }

    if(mChickenState == ChickenState::dying)
    {
        if(mNbTurnDie < NB_TURNS_DIE_BEFORE_REMOVE)
        {
            ++mNbTurnDie;
            return;
        }
        removeFromGameMap();
        deleteYourself();
        return;
    }

    Room* currentHatchery = nullptr;
    if(tile->getCoveringRoom() != nullptr)
    {
        Room* room = tile->getCoveringRoom();
        if(room->getType() == RoomType::hatchery)
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
        clearDestinations(EntityAnimation::die_anim, false, false);
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
    Ogre::Vector3 v (static_cast<Ogre::Real>(tileDest->getX()), static_cast<Ogre::Real>(tileDest->getY()), 0.0);
    std::vector<Ogre::Vector3> path;
    path.push_back(v);
    setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, path);
}

void ChickenEntity::addTileToListIfPossible(int x, int y, Room* currentHatchery, std::vector<Tile*>& possibleTileMove)
{
    Tile* tile = getGameMap()->getTile(x, y);
    if(tile == nullptr)
        return;

    if(tile->getFullness() > 0.0)
        return;

    TileType tileType = tile->getType();
    switch(tileType)
    {
        case TileType::dirt:
        case TileType::gold:
        case TileType::rock:
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

GameEntityType ChickenEntity::getObjectType() const
{
    return GameEntityType::chickenEntity;
}

bool ChickenEntity::tryPickup(Seat* seat)
{
    if(!getIsOnMap())
        return false;

    // We do not let it be picked up as it will be removed during next upkeep. However, this is
    // true only on server side. On client side, if a chicken is available, it can be picked up (it will
    // be up to the server to validate or not) because the client do not know the chicken state.
    if(getIsOnServerMap() && (mChickenState != ChickenState::free))
        return false;

    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return false;
    }

    if(getGameMap()->isInEditorMode())
        return true;

    if(!tile->isClaimedForSeat(seat))
        return false;

    if(tile->getSeat() != seat)
        return false;

    return true;
}

void ChickenEntity::pickup()
{
    removeEntityFromPositionTile();
    RenderedMovableEntity::pickup();
}

bool ChickenEntity::tryDrop(Seat* seat, Tile* tile)
{
    if (tile->isFullTile())
        return false;

    // In editor mode, we allow to drop an object in dirt, claimed or gold tiles
    if(getGameMap()->isInEditorMode() &&
       (tile->getTileVisual() == TileVisual::dirtGround || tile->getTileVisual() == TileVisual::goldGround || tile->getTileVisual() == TileVisual::rockGround))
    {
        return true;
    }

    // we cannot drop a chicken on a tile we don't see
    if(!seat->hasVisionOnTile(tile))
        return false;

    // Otherwise, we allow to drop an object only on allied claimed tiles
    if(tile->isClaimedForSeat(seat))
        return true;

    return false;
}

void ChickenEntity::correctEntityMovePosition(Ogre::Vector3& position)
{
    static const double offset = 0.3;
    if(position.x > 0)
        position.x += Random::Double(-offset, offset);

    if(position.y > 0)
        position.y += Random::Double(-offset, offset);

    if(position.z > 0)
        position.z += Random::Double(-offset, offset);
}

bool ChickenEntity::eatChicken(Creature* creature)
{
    if(mChickenState != ChickenState::free)
        return false;

    OD_LOG_INF("chicken=" + getName() + " eaten by " + creature->getName());

    removeEntityFromPositionTile();
    mChickenState = ChickenState::eaten;
    clearDestinations(EntityAnimation::idle_anim, true, true);
    return true;
}

bool ChickenEntity::canSlap(Seat* seat)
{
    if(!getIsOnMap())
        return false;

    // We do not let it be picked up as it will be removed during next upkeep. However, this is
    // true only on server side. On client side, if a chicken is available, it can be picked up (it will
    // be up to the server to validate or not) because the client do not know the chicken state.
    if(getIsOnServerMap() && (mChickenState != ChickenState::free))
        return false;

    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return false;
    }

    if(getGameMap()->isInEditorMode())
        return !mIsSlapped;

    if(!tile->isClaimedForSeat(seat))
        return false;

    if(tile->getSeat() != seat)
        return false;

    return !mIsSlapped;
}

ChickenEntity* ChickenEntity::getChickenEntityFromStream(GameMap* gameMap, std::istream& is)
{
    ChickenEntity* obj = new ChickenEntity(gameMap, true);
    obj->importFromStream(is);
    return obj;
}

ChickenEntity* ChickenEntity::getChickenEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    ChickenEntity* obj = new ChickenEntity(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}

void ChickenEntity::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
}

bool ChickenEntity::importFromStream(std::istream& is)
{
    if(!RenderedMovableEntity::importFromStream(is))
        return false;
    if(!(is >> mPosition.x >> mPosition.y >> mPosition.z))
        return false;

    return true;
}

std::string ChickenEntity::getChickenEntityStreamFormat()
{
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "PosX\tPosY\tPosZ";

    return format;
}
