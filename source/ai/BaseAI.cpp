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

#include "ai/BaseAI.h"

#include "entities/Creature.h"

#include "game/Player.h"

#include "gamemap/GameMap.h"

#include "network/ODServer.h"

const int32_t pointsPerWallSpot = 50;
const int32_t handicapPerTileOffset = 20;

BaseAI::BaseAI(GameMap& gameMap, Player& player, const std::string& parameters):
    mGameMap(gameMap),
    mPlayer(player)
{
    initialize(parameters);
}

bool BaseAI::initialize(const std::string& parameters)
{
    return true;
}

Room* BaseAI::getDungeonTemple()
{
    std::vector<Room*> dt = mGameMap.getRoomsByTypeAndSeat(Room::dungeonTemple, mPlayer.getSeat());
    if(!dt.empty())
        return dt.front();
    else
        return NULL;
}

bool BaseAI::buildRoom(Room* room, const std::vector<Tile*>& tiles)
{
    room->setupRoom(mGameMap.nextUniqueNameRoom(room->getMeshName()), mPlayer.getSeat(), tiles);
    mGameMap.addRoom(room);
    room->createMesh();
    room->checkForRoomAbsorbtion();
    room->updateActiveSpots();
    mGameMap.refreshBorderingTilesOf(tiles);

    return true;
}

bool BaseAI::shouldGroundTileBeConsideredForBestPlaceForRoom(Tile* tile, Seat* mPlayerSeat)
{
    switch(tile->getType())
    {
        case Tile::TileType::dirt:
        case Tile::TileType::gold:
        {
            // Dirt and gold can always be built (even if digging may be needed depending on fullness)
            return true;
        }
        case Tile::TileType::claimed:
        {
            // We check if we can build on that tile and if there is no building currently
            if(!tile->isClaimedForSeat(mPlayerSeat))
                return false;
            if(tile->getCoveringBuilding() != nullptr)
                return false;

            // We don't want to break a wall where there are activespots from another one
            for(Tile* t : tile->getAllNeighbors())
            {
                if(t->isClaimedForSeat(mPlayerSeat) &&
                    (t->getCoveringRoom() != nullptr))
                {
                    return false;
                }
            }
            return true;
        }
        default:
            return false;
    }

    return false;
}

bool BaseAI::shouldWallTileBeConsideredForBestPlaceForRoom(Tile* tile, Seat* mPlayerSeat)
{
    // We only consider wall claimed for the correct seat or dirt (that can be claimed)
    if(tile->getFullness() <= 0.0)
        return false;

    if(tile->getType() == Tile::TileType::dirt)
        return true;

    if(tile->isWallClaimedForSeat(mPlayerSeat))
        return true;

    return false;
}

//! To find the position, we try every square of the wantedSize width around the given tile for each possible distance
bool BaseAI::findBestPlaceForRoom(Tile* tile, Seat* mPlayerSeat, int32_t wantedSize, bool useWalls,
    int32_t& bestX, int32_t& bestY)
{
    // We use a point system to find the best position. Once we find a valid position, we will set a handicap
    // that will increase as we go away from the given tile. Once the handicap is > to the max points we can get minus
    // the points the room we found got, we can stop searching.
    // With this logic, we can tune easily what the AI should prefer between distance and active spots.

    // We search for the maximum points a room can get
    int32_t maxPointsPossible = 0;
    if(wantedSize >= 3)
    {
        // Maximum central active spots
        int32_t nbCentralActiveSpots = ((wantedSize - 3) / 2) + 1;
        // Wall active spots
        if(useWalls)
            maxPointsPossible += nbCentralActiveSpots * 4 * pointsPerWallSpot;
    }

    bool isFound = false;
    int32_t handicap = 0;
    int32_t bestPoints = 0;
    int32_t bestDistance = 0;
    int32_t maxOffset = std::max(mGameMap.getMapSizeX(), mGameMap.getMapSizeY());

    for(int32_t offset = 1; offset < maxOffset; ++offset)
    {
        int32_t points = 0;
        int32_t nbTiles = offset * 2 + wantedSize - 1;
        for(int32_t k = 0; k < nbTiles; ++k)
        {
            Tile* t;
            // North
            t  = mGameMap.getTile(tile->getX() - offset - wantedSize + 2 + k, tile->getY() + offset);
            if((t != nullptr) &&
               computePointsForRoom(t, mPlayerSeat, wantedSize, true, useWalls, points))
            {
                points -= handicap;
                int32_t centerX = t->getX() + (wantedSize / 2);
                int32_t centerY = t->getY() + (wantedSize / 2);
                int32_t distance = (tile->getX() - centerX) * (tile->getX() - centerX);
                distance += (tile->getY() - centerY) * (tile->getY() - centerY);
                if((points > bestPoints) ||
                   (points == bestPoints && distance < bestDistance))
                {
                    bestDistance = distance;
                    bestX = t->getX();
                    bestY = t->getY();
                    bestPoints = points;
                    isFound = true;
                }
            }
            // East
            t  = mGameMap.getTile(tile->getX() + offset, tile->getY() - k + offset);
            if((t != nullptr) &&
               computePointsForRoom(t, mPlayerSeat, wantedSize, true, useWalls, points))
            {
                points -= handicap;
                int32_t centerX = t->getX() + (wantedSize / 2);
                int32_t centerY = t->getY() + (wantedSize / 2);
                int32_t distance = (tile->getX() - centerX) * (tile->getX() - centerX);
                distance += (tile->getY() - centerY) * (tile->getY() - centerY);
                if((points > bestPoints) ||
                   (points == bestPoints && distance < bestDistance))
                {
                    bestDistance = distance;
                    bestX = t->getX();
                    bestY = t->getY();
                    bestPoints = points;
                    isFound = true;
                }
            }
            // South
            t  = mGameMap.getTile(tile->getX() + offset + wantedSize - 2 - k, tile->getY() - offset);
            if((t != nullptr) &&
               computePointsForRoom(t, mPlayerSeat, wantedSize, false, useWalls, points))
            {
                points -= handicap;
                int32_t centerX = t->getX() - (wantedSize / 2);
                int32_t centerY = t->getY() - (wantedSize / 2);
                int32_t distance = (tile->getX() - centerX) * (tile->getX() - centerX);
                distance += (tile->getY() - centerY) * (tile->getY() - centerY);
                if((points > bestPoints) ||
                   (points == bestPoints && distance < bestDistance))
                {
                    bestDistance = distance;
                    bestX = t->getX() - wantedSize + 1;
                    bestY = t->getY() - wantedSize + 1;
                    bestPoints = points;
                    isFound = true;
                }
            }
            // West
            t  = mGameMap.getTile(tile->getX() - offset, tile->getY() - offset + k);
            if((t != nullptr) &&
               computePointsForRoom(t, mPlayerSeat, wantedSize, false, useWalls, points))
            {
                points -= handicap;
                int32_t centerX = t->getX() - (wantedSize / 2);
                int32_t centerY = t->getY() - (wantedSize / 2);
                int32_t distance = (tile->getX() - centerX) * (tile->getX() - centerX);
                distance += (tile->getY() - centerY) * (tile->getY() - centerY);
                if((points > bestPoints) ||
                   (points == bestPoints && distance < bestDistance))
                {
                    bestDistance = distance;
                    bestX = t->getX() - wantedSize + 1;
                    bestY = t->getY() - wantedSize + 1;
                    bestPoints = points;
                    isFound = true;
                }
            }
        }

        if(isFound)
        {
            handicap += handicapPerTileOffset;
            // If we already found the best place, stop searching
            if(handicap > (maxPointsPossible - bestPoints))
                break;
        }
    }
    return isFound;
}

bool BaseAI::computePointsForRoom(Tile* tile, Seat* mPlayerSeat, int32_t wantedSize,
    bool bottomLeft2TopRight, bool useWalls, int32_t& points)
{
    int tileX = tile->getX();
    int tileY = tile->getY();
    bool isOk = true;

    // We check if the tile is reachable. No need to compute if the tile is behind rocks
    points = 0;
    // We start by searching for a place fitting the min size. If we find it,
    // we increase tile by tile until the max size. If not, no need to continue
    for(int32_t xx = 0; xx < wantedSize; ++xx)
    {
        for(int32_t yy = 0; yy < wantedSize; ++yy)
        {
            Tile* t;
            if(bottomLeft2TopRight)
                t  = mGameMap.getTile(tileX + xx, tileY + yy);
            else
                t  = mGameMap.getTile(tileX - xx, tileY - yy);

            if(t == nullptr)
            {
                isOk = false;
                break;
            }

            if(!shouldGroundTileBeConsideredForBestPlaceForRoom(t, mPlayerSeat))
            {
                isOk = false;
                break;
            }
        }

        if(!isOk)
            break;
    }

    if(!isOk)
        return false;

    // If we don't want to consider walls, we stop here (for example for rooms that do not have bonus
    // with wall active spots like treasury or dormitory)
    if(!useWalls)
        return true;

    // We search points for first wall. That's not exactly how the activespots will be computed but it will be enough (especially
    // when the room size is even)
    int nbConsecutiveTiles;
    int nbActiveWallSpots;
    // We search points for first wall
    nbConsecutiveTiles = 0;
    nbActiveWallSpots = 0;
    for(int32_t kk = 0; kk < wantedSize; ++kk)
    {
        Tile* t;
        if(bottomLeft2TopRight)
            t  = mGameMap.getTile(tileX - 1, tileY + kk);
        else
            t  = mGameMap.getTile(tileX + 1, tileY - kk);
        if(t == nullptr)
            continue;

        if(shouldWallTileBeConsideredForBestPlaceForRoom(t, mPlayerSeat))
            ++nbConsecutiveTiles;
        else
            nbConsecutiveTiles = 0;

        if(nbActiveWallSpots == 0)
        {
            if(nbConsecutiveTiles >= 3)
            {
                nbConsecutiveTiles = 0;
                ++nbActiveWallSpots;
            }
        }
        else if(nbConsecutiveTiles >= 2)
        {
            nbConsecutiveTiles = 0;
            ++nbActiveWallSpots;
        }
    }
    points += nbActiveWallSpots * pointsPerWallSpot;
    // We search points for second wall
    nbConsecutiveTiles = 0;
    nbActiveWallSpots = 0;
    for(int32_t kk = 0; kk < wantedSize; ++kk)
    {
        Tile* t;
        if(bottomLeft2TopRight)
            t = mGameMap.getTile(tileX + wantedSize, tileY + kk);
        else
            t = mGameMap.getTile(tileX - wantedSize, tileY - kk);
        if(t == nullptr)
            continue;

        if(shouldWallTileBeConsideredForBestPlaceForRoom(t, mPlayerSeat))
            ++nbConsecutiveTiles;
        else
            nbConsecutiveTiles = 0;

        if(nbActiveWallSpots == 0)
        {
            if(nbConsecutiveTiles >= 3)
            {
                nbConsecutiveTiles = 0;
                ++nbActiveWallSpots;
            }
        }
        else if(nbConsecutiveTiles >= 2)
        {
            nbConsecutiveTiles = 0;
            ++nbActiveWallSpots;
        }
    }
    points += nbActiveWallSpots * pointsPerWallSpot;
    // We search points for first wall
    nbConsecutiveTiles = 0;
    nbActiveWallSpots = 0;
    for(int32_t kk = 0; kk < wantedSize; ++kk)
    {
        Tile* t;
        if(bottomLeft2TopRight)
            t = mGameMap.getTile(tileX + kk, tileY - 1);
        else
            t = mGameMap.getTile(tileX - kk, tileY + 1);
        if(t == nullptr)
            continue;

        if(shouldWallTileBeConsideredForBestPlaceForRoom(t, mPlayerSeat))
            ++nbConsecutiveTiles;
        else
            nbConsecutiveTiles = 0;

        if(nbActiveWallSpots == 0)
        {
            if(nbConsecutiveTiles >= 3)
            {
                nbConsecutiveTiles = 0;
                ++nbActiveWallSpots;
            }
        }
        else if(nbConsecutiveTiles >= 2)
        {
            nbConsecutiveTiles = 0;
            ++nbActiveWallSpots;
        }
    }
    points += nbActiveWallSpots * pointsPerWallSpot;
    // We search points for first wall
    nbConsecutiveTiles = 0;
    nbActiveWallSpots = 0;
    for(int32_t kk = 0; kk < wantedSize; ++kk)
    {
        Tile* t;
        if(bottomLeft2TopRight)
            t = mGameMap.getTile(tileX + kk, tileY + wantedSize);
        else
            t = mGameMap.getTile(tileX - kk, tileY - wantedSize);
        if(t == nullptr)
            continue;

        if(shouldWallTileBeConsideredForBestPlaceForRoom(t, mPlayerSeat))
            ++nbConsecutiveTiles;
        else
            nbConsecutiveTiles = 0;

        if(nbActiveWallSpots == 0)
        {
            if(nbConsecutiveTiles >= 3)
            {
                nbConsecutiveTiles = 0;
                ++nbActiveWallSpots;
            }
        }
        else if(nbConsecutiveTiles >= 2)
        {
            nbConsecutiveTiles = 0;
            ++nbActiveWallSpots;
        }
    }
    points += nbActiveWallSpots * pointsPerWallSpot;

    return true;
}

bool BaseAI::digWayToTile(Tile* tileStart, Tile* tileEnd)
{
    // We find a way to tileEnd. We search in reverse order to stop when we reach the first
    // accessible tile

    // Set a diggable path up to the first gold spot for the given team color, by the first available kobold
    Seat* seat = mPlayer.getSeat();
    std::vector<Creature*> creatures = mGameMap.getCreaturesBySeat(seat);

    if (creatures.empty())
        return false;

    Creature* kobold = nullptr;
    for (Creature* creature : creatures)
    {
        if (creature->getDefinition()->getClassName() == "Kobold")
        {
            kobold = creature;
            break;
        }
    }

    if (kobold == nullptr)
        return false;

    std::list<Tile*> pathToDig = mGameMap.path(tileEnd, tileStart, kobold,
        seat, true);
    if (pathToDig.empty())
        return false;

    // We search for the first reachable tile in the list
    bool isPathFound = false;
    for(std::list<Tile*>::iterator it = pathToDig.begin(); it != pathToDig.end();)
    {
        Tile* tile = *it;
        if(!isPathFound &&
           (tile->getFullness() == 0.0) &&
           (mGameMap.pathExists(kobold, tileStart, tile)))
        {
            isPathFound = true;
        }

        if(isPathFound)
        {
            it = pathToDig.erase(it);
            continue;
        }

        // If the tile should be dug, we check if one of its neighboors can be reached.
        // If yes, we will stop after digging it to avoid digging through a wall as much as
        // possible
        for(Tile* t : tile->getAllNeighbors())
        {
            if(!isPathFound &&
               (t->getFullness() == 0.0) &&
               (mGameMap.pathExists(kobold, tileStart, t)))
            {
                // we let the iterator increment because we want to dig the currently
                // tested tile
                isPathFound = true;
                break;
            }
        }

        ++it;
    }

    for(Tile* tile : pathToDig)
    {
        if (tile && tile->isDiggable(seat))
            tile->setMarkedForDigging(true, &mPlayer);
    }

    return true;
}

