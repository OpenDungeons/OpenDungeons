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

#include "traps/TrapBoulder.h"
#include "network/ODPacket.h"
#include "gamemap/GameMap.h"
#include "entities/MissileBoulder.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

TrapBoulder::TrapBoulder(GameMap* gameMap) :
    Trap(gameMap)
{
    mReloadTime = ConfigManager::getSingleton().getTrapConfigUInt32("BoulderReloadTurns");
    mMinDamage = ConfigManager::getSingleton().getTrapConfigDouble("BoulderDamagePerHitMin");
    mMaxDamage = ConfigManager::getSingleton().getTrapConfigDouble("BoulderDamagePerHitMax");
    setMeshName("Boulder");
}

TrapBoulder* TrapBoulder::getTrapBoulderFromStream(GameMap* gameMap, std::istream &is)
{
    TrapBoulder* trap = new TrapBoulder(gameMap);
    return trap;
}

TrapBoulder* TrapBoulder::getTrapBoulderFromPacket(GameMap* gameMap, ODPacket &is)
{
    TrapBoulder* trap = new TrapBoulder(gameMap);
    return trap;
}

bool TrapBoulder::shoot(Tile* tile)
{
    std::vector<Tile*> tiles = tile->getAllNeighbors();
    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end();)
    {
        Tile* tmpTile = *it;
        std::vector<Tile*> vecTile;
        vecTile.push_back(tmpTile);

        if(getGameMap()->getVisibleCreatures(vecTile, getSeat(), true).empty())
            it = tiles.erase(it);
        else
            ++it;
    }
    if(tiles.empty())
        return false;

    // We take a random tile and launch boulder it
    Tile* tileChoosen = tiles[Random::Uint(0, tiles.size() - 1)];
    // We launch the boulder
    Ogre::Vector3 direction(static_cast<Ogre::Real>(tileChoosen->getX() - tile->getX()),
                            static_cast<Ogre::Real>(tileChoosen->getY() - tile->getY()),
                            0);
    Ogre::Vector3 position;
    position.x = static_cast<Ogre::Real>(tile->getX());
    position.y = static_cast<Ogre::Real>(tile->getY());
    position.z = 0;
    direction.normalise();
    MissileBoulder* missile = new MissileBoulder(getGameMap(), getSeat(), getName(), "Boulder",
        direction, Random::Double(mMinDamage, mMaxDamage));
    missile->setPosition(position);
    getGameMap()->addRenderedMovableEntity(missile);
    missile->setMoveSpeed(1.0);
    missile->createMesh();
    // We don't want the missile to stay idle for 1 turn. Because we are in a doUpkeep context,
    // we can safely call the missile doUpkeep as we know the engine will not call it the turn
    // it has been added
    missile->doUpkeep();

    return true;
}

RenderedMovableEntity* TrapBoulder::notifyActiveSpotCreated(Tile* tile)
{
    return loadBuildingObject(getGameMap(), "Boulder", tile, 0.0);
}
