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

#ifndef BASEAI_H
#define BASEAI_H

#include <string>
#include <vector>

class GameMap;
class Player;
class Room;
class Tile;
class Seat;

class BaseAI
{
public:
    BaseAI(GameMap& gameMap, Player& player, const std::string& parameters = std::string());
    virtual ~BaseAI()
    {}

     /** \brief This is the function that will be called each turn for the ai.
     *  This is the function that will be called each turn for the ai.
     *  For custom AI's this should be overridden and return true on a
     *  successful call.
     *  \param frameTime Time elapsed since last call in seconds.
     */
    virtual bool doTurn(double frameTime) = 0;

protected:
    virtual bool initialize(const std::string& parameters);
    Room* getDungeonTemple();
    bool buildRoom(Room* room, const std::vector<Tile*>& tiles);

    //! \brief Searches for the best place where to place a room around the given tile. It will take
    //! into account any constructible tile (even if not digged yet). On success, it returns true and bestX
    //! and bestY will be set accordingly. It will return false if no constructible square of wantedSize
    //! is found
    bool findBestPlaceForRoom(Tile* tile, Seat* playerSeat, int32_t wantedSize, bool useWalls,
        int32_t& bestX, int32_t& bestY);

    bool digWayToTile(Tile* tileStart, Tile* tileEnd);
    bool computePointsForRoom(Tile* tile, Seat* playerSeat, int32_t wantedSize,
        bool bottomLeft2TopRight, bool useWalls, int32_t& points);

    GameMap& mGameMap;
    Player& mPlayer;

private:
    bool shouldGroundTileBeConsideredForBestPlaceForRoom(Tile* tile, Seat* playerSeat);
    bool shouldWallTileBeConsideredForBestPlaceForRoom(Tile* tile, Seat* playerSeat);
};

#endif // BASEAI_H
