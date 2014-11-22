/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "network/ODPacket.h"
#include "gamemap/GameMap.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <iostream>

const int32_t NB_TURNS_DIE_BEFORE_REMOVE = 0;

SmallSpiderEntity::SmallSpiderEntity(GameMap* gameMap, const std::string& cryptName, int32_t nbTurnLife) :
    RenderedMovableEntity(gameMap, cryptName, "SmallSpider", 0.0f, false),
    mNbTurnLife(nbTurnLife)
{
}

SmallSpiderEntity::SmallSpiderEntity(GameMap* gameMap) :
    RenderedMovableEntity(gameMap),
    mNbTurnLife(0)
{
    setMeshName("SmallSpider");
}

void SmallSpiderEntity::doUpkeep()
{
    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
    {
        getGameMap()->removeRenderedMovableEntity(this);
        deleteYourself();
        return;
    }

    --mNbTurnLife;

    // If the spider is outside the crypt or too old, it dies
    Room* currentCrypt = tile->getCoveringRoom();
    if((mNbTurnLife <= 0) || (currentCrypt == nullptr) || (currentCrypt->getType() != Room::RoomType::crypt))
    {
        getGameMap()->removeRenderedMovableEntity(this);
        deleteYourself();
        return;
    }

    // Handle normal behaviour : move or pick (if not already moving)
    if(isMoving())
        return;

    int posSpiderX = tile->getX();
    int posSpiderY = tile->getY();
    std::vector<Tile*> possibleTileMove;
    // We move spiders from 1 tile only
    addTileToListIfPossible(posSpiderX - 1, posSpiderY, currentCrypt, possibleTileMove);
    addTileToListIfPossible(posSpiderX + 1, posSpiderY, currentCrypt, possibleTileMove);
    addTileToListIfPossible(posSpiderX, posSpiderY - 1, currentCrypt, possibleTileMove);
    addTileToListIfPossible(posSpiderX, posSpiderY + 1, currentCrypt, possibleTileMove);

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

void SmallSpiderEntity::addTileToListIfPossible(int x, int y, Room* currentCrypt, std::vector<Tile*>& possibleTileMove)
{
    Tile* tile = getGameMap()->getTile(x, y);
    if(tile == nullptr)
        return;

    if(currentCrypt != tile->getCoveringRoom())
        return;

    // We can move on this tile
    possibleTileMove.push_back(tile);
}

SmallSpiderEntity* SmallSpiderEntity::getSmallSpiderEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    SmallSpiderEntity* obj = new SmallSpiderEntity(gameMap);
    return obj;
}

const char* SmallSpiderEntity::getFormat()
{
    // Small spider are not to be saved in file
    return "Unused";
}
