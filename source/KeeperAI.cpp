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

#include <vector>

AIFactoryRegister<KeeperAI> KeeperAI::reg("KeeperAI");

KeeperAI::KeeperAI(GameMap& gameMap, Player& player, const std::string& parameters)
    : BaseAI(gameMap, player, parameters)
{

}

bool KeeperAI::doTurn(double frameTime)
{
    frameTime = 0;
    static bool first = true;
    if(first)
    {
        lookForGold();
        first = false;
    }
    return true;
}

void KeeperAI::lookForGold()
{
    const Tile* central = mAiWrapper.getDungeonTemple()->getCentralTile();

    std::vector<Tile*> tiles = mAiWrapper.getGameMap().circularRegion(central->getX(), central->getY(), 30);
    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        if(tile->getType() == Tile::gold && tile->getFullness() > 1.0)
        {
            mAiWrapper.markTileForDigging(tile);
        }
    }
}
