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

#include "traps/TrapSpike.h"

#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

static TrapManagerRegister<TrapSpike> reg(TrapType::spike, "Spike", "Spike trap");

const std::string TrapSpike::MESH_SPIKE = "Spiketrap";

TrapSpike::TrapSpike(GameMap* gameMap) :
    Trap(gameMap)
{
    mReloadTime = ConfigManager::getSingleton().getTrapConfigUInt32("SpikeReloadTurns");
    mMinDamage = ConfigManager::getSingleton().getTrapConfigDouble("SpikeDamagePerHitMin");
    mMaxDamage = ConfigManager::getSingleton().getTrapConfigDouble("SpikeDamagePerHitMax");
    mNbShootsBeforeDeactivation = ConfigManager::getSingleton().getTrapConfigUInt32("SpikeNbShootsBeforeDeactivation");
    setMeshName("");
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
    for(GameEntity* target : enemyCreatures)
    {
        target->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), 0.0, target->getCoveredTile(0), false, false);
    }
    std::vector<GameEntity*> alliedCreatures = getGameMap()->getVisibleCreatures(visibleTiles, getSeat(), false);
    for(GameEntity* target : alliedCreatures)
    {
        target->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), 0.0, target->getCoveredTile(0), false, false);
    }
    return true;
}

TrapEntity* TrapSpike::getTrapEntity(Tile* tile)
{
    return new TrapEntity(getGameMap(), true, getName(), MESH_SPIKE, tile, 0.0, true, isActivated(tile) ? 1.0f : 0.7f);
}

void TrapSpike::checkBuildTrap(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildTrapDefault(gameMap, TrapType::spike, inputManager, inputCommand);
}

bool TrapSpike::buildTrap(GameMap* gameMap, Player* player, ODPacket& packet)
{
    std::vector<Tile*> tiles;
    if(!getTrapTilesDefault(tiles, gameMap, player, packet))
        return false;

    int32_t pricePerTarget = TrapManager::costPerTile(TrapType::spike);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    TrapSpike* trap = new TrapSpike(gameMap);
    return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
}

void TrapSpike::checkBuildTrapEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildTrapDefaultEditor(gameMap, TrapType::spike, inputManager, inputCommand);
}

bool TrapSpike::buildTrapEditor(GameMap* gameMap, ODPacket& packet)
{
    TrapSpike* trap = new TrapSpike(gameMap);
    return buildTrapDefaultEditor(gameMap, trap, packet);
}

bool TrapSpike::buildTrapOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles)
{
    int32_t pricePerTarget = TrapManager::costPerTile(TrapType::spike);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    TrapSpike* trap = new TrapSpike(gameMap);
    return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
}

Trap* TrapSpike::getTrapFromStream(GameMap* gameMap, std::istream& is)
{
    TrapSpike* trap = new TrapSpike(gameMap);
    trap->importFromStream(is);
    return trap;
}
