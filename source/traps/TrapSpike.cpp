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

#include "traps/TrapSpike.h"

#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "entities/RenderedMovableEntity.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

TrapSpike::TrapSpike(GameMap* gameMap) :
    ProximityTrap(gameMap)
{
    mReloadTime = ConfigManager::getSingleton().getTrapConfigUInt32("SpikeReloadTurns");
    mMinDamage = ConfigManager::getSingleton().getTrapConfigDouble("SpikeDamagePerHitMin");
    mMaxDamage = ConfigManager::getSingleton().getTrapConfigDouble("SpikeDamagePerHitMax");
    setMeshName("Spike");
}

bool TrapSpike::shoot(Tile* tile)
{
    std::vector<Tile*> visibleTiles;
    visibleTiles.push_back(tile);
    std::vector<GameEntity*> enemyCreatures = getGameMap()->getVisibleCreatures(visibleTiles, getSeat(), true);
    if(enemyCreatures.empty())
        return false;

    RenderedMovableEntity* spike = getBuildingObjectFromTile(tile);
    spike->setAnimationState("Triggered", false);

    // We damage every creature standing on the trap
    for(std::vector<GameEntity*>::iterator it = enemyCreatures.begin(); it != enemyCreatures.end(); ++it)
    {
        GameEntity* target = *it;
        target->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), 0.0, target->getCoveredTiles()[0]);
    }
    std::vector<GameEntity*> alliedCreatures = getGameMap()->getVisibleCreatures(visibleTiles, getSeat(), false);
    for(std::vector<GameEntity*>::iterator it = alliedCreatures.begin(); it != alliedCreatures.end(); ++it)
    {
        GameEntity* target = *it;
        target->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), 0.0, target->getCoveredTiles()[0]);
    }
    return true;
}

RenderedMovableEntity* TrapSpike::notifyActiveSpotCreated(Tile* tile)
{
    return loadBuildingObject(getGameMap(), "Spiketrap", tile, 0.0, true);
}

TrapSpike* TrapSpike::getTrapSpikeFromStream(GameMap* gameMap, std::istream &is)
{
    TrapSpike* trap = new TrapSpike(gameMap);
    return trap;
}

TrapSpike* TrapSpike::getTrapSpikeFromPacket(GameMap* gameMap, ODPacket &is)
{
    TrapSpike* trap = new TrapSpike(gameMap);
    return trap;
}
