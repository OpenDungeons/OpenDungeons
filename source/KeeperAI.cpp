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

#include "KeeperAI.h"

#include "GameMap.h"
#include "Tile.h"
#include "LogManager.h"

#include <vector>

AIFactoryRegister<KeeperAI> KeeperAI::reg("KeeperAI");

KeeperAI::KeeperAI(GameMap& gameMap, Player& player, const std::string& parameters):
    BaseAI(gameMap, player, parameters)
{
    mSomePlaceMade = false;
    mSleepRoomMade = false;
    mLastTimeLookingForGold = 30.0; // debug: 30s, otherwise: 3 minutes
}

bool KeeperAI::doTurn(double frameTime)
{
    if (!mSomePlaceMade)
    {
        MakeSomePlace();
        return true;
    }

    if (!mSleepRoomMade && mAiWrapper.getGoldInTreasury() > 3000)
    {
        buildSleepRoom();
        return true;
    }

    mLastTimeLookingForGold += frameTime * 20.0;
std::cout << mLastTimeLookingForGold << std::endl;
    // Each three minutes.
    if(mLastTimeLookingForGold >= 30.0)
    {
        mLastTimeLookingForGold = 0.0;
        lookForGold();
    }
    return true;
}

void KeeperAI::MakeSomePlace()
{
    Tile* central = mAiWrapper.getDungeonTemple()->getCentralTile();
    std::vector<Tile*> tiles = mAiWrapper.getGameMap().circularRegion(central->getX(), central->getY(), 8);
    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        if(tile && tile->getType() == Tile::dirt && tile->getFullness() > 1.0)
        {
            // Make place for some rooms
            mAiWrapper.markTileForDigging(tile);
        }
    }
    mSomePlaceMade = true;
}

void KeeperAI::buildSleepRoom()
{
    Tile* central = mAiWrapper.getDungeonTemple()->getCentralTile();
    // Check whether at least enough tiles are claimed.
    std::vector<Tile*> tiles = mAiWrapper.getGameMap().rectangularRegion(central->getX() - 2, central->getY() + 5,
                                                                         central->getX() + 2, central->getY() + 3);
    unsigned int numClaimedTiles = 0;
    int team_color_id = mAiWrapper.getPlayer().getSeat()->color;
    for (std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        if (tile && tile->getType() == Tile::claimed && tile->getFullness() < 1.0
                && tile->isBuildableUpon() && tile->getColor() == team_color_id
                && tile->colorDouble > 0.99)
            ++numClaimedTiles;
    }
    if (numClaimedTiles < 12)
        return;

    mAiWrapper.buildRoom(Room::quarters, central->getX() - 2, central->getY() + 5,
                                         central->getX() + 2, central->getY() + 3);
    mSleepRoomMade = true;
}

void KeeperAI::lookForGold()
{
    std::cout << "Look for gold" << std::endl;

    Tile* central = mAiWrapper.getDungeonTemple()->getCentralTile();

    std::vector<Tile*> tiles = mAiWrapper.getGameMap().circularRegion(central->getX(), central->getY(), 200);
    Tile* firstGoldTile = NULL;
    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        if(tile && tile->getType() == Tile::gold && tile->getFullness() > 1.0)
        {
            // Get the closest gold tile.
            if (firstGoldTile == NULL)
            {
                firstGoldTile = tile;
            }
            else
            {
                Ogre::Real currentDistance = mAiWrapper.getGameMap().crowDistance(central, firstGoldTile);
                Ogre::Real newDistance = mAiWrapper.getGameMap().crowDistance(central, tile);
                if (newDistance < currentDistance)
                    firstGoldTile = tile;
            }

            // Set it up to dig anyway.
            mAiWrapper.markTileForDigging(tile);
        }
    }

    // No more gold
    if (firstGoldTile == NULL)
        return;

    // Set a diggable path up to the first gold spot for the given team color
    int team_color_id = mAiWrapper.getPlayer().getSeat()->color;
    std::list<Tile*> goldPath = mAiWrapper.getGameMap().path(central, firstGoldTile, Tile::diggableTile, team_color_id);

    std::cout << "Found path to gold: " << goldPath.size() << std::endl;
    for(std::list<Tile*>::iterator it = goldPath.begin(); it != goldPath.end(); ++it)
    {
        // Make a three tile wide path when possible
        Tile* tile = *it;
        if (tile && tile->isDiggable(team_color_id))
        {
            mAiWrapper.markTileForDigging(tile);
            std::cout << "Added tile to dig" << std::endl;
        }

        // Set neighbors too so the path is wide enough
        std::vector<Tile*> neighborTiles = tile->getAllNeighbors();
        for(std::vector<Tile*>::iterator it2 = neighborTiles.begin(); it2 != neighborTiles.end(); ++it2)
        {
            if ((*it2) && (*it2)->isDiggable(team_color_id))
                mAiWrapper.markTileForDigging(*it2);
        }
    }
}
