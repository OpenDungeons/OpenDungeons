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

#include "entities/SmallSpiderEntity.h"

#include "entities/Tile.h"
#include "network/ODPacket.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <istream>
#include <ostream>

const int32_t NB_TURNS_DIE_BEFORE_REMOVE = 0;

SmallSpiderEntity::SmallSpiderEntity(GameMap* gameMap, bool isOnServerMap, const std::string& cryptName, int32_t nbTurnLife) :
    RenderedMovableEntity(gameMap, isOnServerMap, cryptName, "SmallSpider", 0.0f, false),
    mNbTurnLife(nbTurnLife),
    mIsSlapped(false)
{
}

SmallSpiderEntity::SmallSpiderEntity(GameMap* gameMap, bool isOnServerMap) :
    RenderedMovableEntity(gameMap, isOnServerMap),
    mNbTurnLife(0),
    mIsSlapped(false)
{
    setMeshName("SmallSpider");
}

void SmallSpiderEntity::doUpkeep()
{
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        removeFromGameMap();
        deleteYourself();
        return;
    }

    --mNbTurnLife;

    // If the spider is outside the crypt or too old, it dies
    Room* currentCrypt = nullptr;
    if((tile->getCoveringRoom() != nullptr) &&
       (tile->getCoveringRoom()->getType() == RoomType::crypt))
    {
       currentCrypt = tile->getCoveringRoom();
    }

    if(mIsSlapped || (mNbTurnLife <= 0) || (currentCrypt == nullptr))
    {
        removeFromGameMap();
        deleteYourself();
        return;
    }

    // Handle normal behaviour : move or pick (if not already moving)
    if(isMoving())
        return;

    std::vector<Ogre::Vector3> moves;
    // We randomly choose some tiles to walk
    int posX = tile->getX();
    int posY = tile->getY();
    while((Random::Int(1,3) > 1) && (moves.size() < 3))
    {
        std::vector<Tile*> possibleTileMove;
        addTileToListIfPossible(posX - 1, posY, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX + 1, posY, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX, posY - 1, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX, posY + 1, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX - 1, posY - 1, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX + 1, posY + 1, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX - 1, posY - 1, currentCrypt, possibleTileMove);
        addTileToListIfPossible(posX + 1, posY + 1, currentCrypt, possibleTileMove);

        // We cannot move. That can happen if all the nearby tiles have been destroyed
        if(possibleTileMove.empty())
            break;

        Tile* tileDest = possibleTileMove[Random::Uint(0, possibleTileMove.size() - 1)];
        Ogre::Vector3 dest(static_cast<Ogre::Real>(tileDest->getX()), static_cast<Ogre::Real>(tileDest->getY()), 0.0);
        moves.push_back(dest);
        posX = tileDest->getX();
        posY = tileDest->getY();
    }

    if(moves.empty())
    {
        setAnimationState(EntityAnimation::idle_anim);
        return;
    }

    setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, moves);
}

bool SmallSpiderEntity::canSlap(Seat* seat)
{
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        return false;
    }

    Room* currentCrypt = tile->getCoveringRoom();
    if(currentCrypt == nullptr)
        return false;
    if(currentCrypt->getType() != RoomType::crypt)
        return false;

    if(currentCrypt->getSeat() != seat)
        return false;

    return !mIsSlapped;
}

void SmallSpiderEntity::addTileToListIfPossible(int x, int y, Room* currentCrypt, std::vector<Tile*>& possibleTileMove)
{
    Tile* tile = getGameMap()->getTile(x, y);
    if(tile == nullptr)
        return;

    if(currentCrypt != tile->getCoveringBuilding())
        return;

    // We can move on this tile
    possibleTileMove.push_back(tile);
}

SmallSpiderEntity* SmallSpiderEntity::getSmallSpiderEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    SmallSpiderEntity* obj = new SmallSpiderEntity(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}
