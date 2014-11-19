/*!
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

#ifndef AIWRAPPER_H
#define AIWRAPPER_H

#include "rooms/Room.h"

#include <vector>

class Player;
class Seat;
class GameMap;
class Creature;

class AIWrapper
{

public:
    AIWrapper(GameMap& gameMap, Player& player);
    virtual ~AIWrapper();

    int getGoldInTreasury() const;

    //Do we need more than a true/false here?
    bool buildRoom(Room* room, const std::vector<Tile*>& tiles);
    bool dropHand(int x, int y, int index);
    bool pickUpCreature(Creature* creature);
    const std::vector<GameEntity*>& getObjectsInHand();
    std::vector<const Room*> getOwnedRoomsByType(Room::RoomType type);
    Room* getDungeonTemple();
    void markTileForDigging(Tile* tile);

    GameMap& getGameMap() const
    { return gameMap; }

    Player& getPlayer() const
    { return player; }

    //! \brief Searches for the best place where to place a room around the given tile. It will take
    //! into account any constructible tile (even if not digged yet). On success, it returns true and bestX
    //! and bestY will be set accordingly. It will return false if no constructible square of wantedSize
    //! is found
    bool findBestPlaceForRoom(Tile* tile, Seat* playerSeat, int32_t wantedSize, bool useWalls,
        int32_t& bestX, int32_t& bestY);

    bool digWayToTile(Tile* tileStart, Tile* tileEnd);
    bool computePointsForRoom(Tile* tile, Seat* playerSeat, int32_t wantedSize,
        bool bottomLeft2TopRight, bool useWalls, int32_t& points);

private:
    bool shouldGroundTileBeConsideredForBestPlaceForRoom(Tile* tile, Seat* playerSeat);
    bool shouldWallTileBeConsideredForBestPlaceForRoom(Tile* tile, Seat* playerSeat);

    std::vector<Tile*> getAffectedTiles(int x1, int y1, int x2, int y2);

    GameMap& gameMap;
    Player& player;
    Seat& seat;
};

#endif // AIWRAPPER_H
