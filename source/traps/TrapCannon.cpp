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

#include "traps/TrapCannon.h"

#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "entities/MissileOneHit.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

static TrapManagerRegister<TrapCannon> reg(TrapType::cannon, "Cannon");

const std::string TrapCannon::MESH_CANON = "Cannon";
const Ogre::Real CANNON_MISSILE_HEIGHT = 0.3;

TrapCannon::TrapCannon(GameMap* gameMap) :
    Trap(gameMap),
    mRange(0)
{
    mReloadTime = ConfigManager::getSingleton().getTrapConfigUInt32("CannonReloadTurns");
    mRange = ConfigManager::getSingleton().getTrapConfigUInt32("CannonRange");
    mMinDamage = ConfigManager::getSingleton().getTrapConfigDouble("CannonDamagePerHitMin");
    mMaxDamage = ConfigManager::getSingleton().getTrapConfigDouble("CannonDamagePerHitMax");
    mNbShootsBeforeDeactivation = ConfigManager::getSingleton().getTrapConfigUInt32("CannonNbShootsBeforeDeactivation");
    setMeshName("Cannon");
}

bool TrapCannon::shoot(Tile* tile)
{
    std::vector<Tile*> visibleTiles = getGameMap()->visibleTiles(tile->getX(), tile->getY(), mRange);
    std::vector<GameEntity*> enemyObjects = getGameMap()->getVisibleCreatures(visibleTiles, getSeat(), true);

    if(enemyObjects.empty())
        return false;

    // Select an enemy to shoot at.
    GameEntity* targetEnemy = enemyObjects[Random::Uint(0, enemyObjects.size()-1)];

    // Create the cannonball to move toward the enemy creature.
    Ogre::Vector3 direction(static_cast<Ogre::Real>(targetEnemy->getCoveredTile(0)->getX()),
                            static_cast<Ogre::Real>(targetEnemy->getCoveredTile(0)->getY()),
                            CANNON_MISSILE_HEIGHT);

    Ogre::Vector3 position;
    position.x = static_cast<Ogre::Real>(tile->getX());
    position.y = static_cast<Ogre::Real>(tile->getY());
    position.z = CANNON_MISSILE_HEIGHT;
    direction = direction - position;
    direction.normalise();
    MissileOneHit* missile = new MissileOneHit(getGameMap(), true, getSeat(), getName(), "Cannonball",
        "", direction, Random::Double(mMinDamage, mMaxDamage), 0.0, nullptr, false);
    missile->addToGameMap();
    missile->createMesh();
    missile->setPosition(position, false);
    missile->setMoveSpeed(ConfigManager::getSingleton().getTrapConfigDouble("CannonSpeed"));
    // We don't want the missile to stay idle for 1 turn. Because we are in a doUpkeep context,
    // we can safely call the missile doUpkeep as we know the engine will not call it the turn
    // it has been added
    missile->doUpkeep();
    return true;
}

TrapEntity* TrapCannon::getTrapEntity(Tile* tile)
{
    return new TrapEntity(getGameMap(), true, getName(), MESH_CANON, tile, 90.0, false, isActivated(tile) ? 1.0f : 0.5f);
}

int TrapCannon::getTrapCost(std::vector<Tile*>& tiles, GameMap* gameMap, TrapType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    return getTrapCostDefault(tiles, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void TrapCannon::buildTrap(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat)
{
    TrapCannon* room = new TrapCannon(gameMap);
    buildTrapDefault(gameMap, room, tiles, seat);
}

Trap* TrapCannon::getTrapFromStream(GameMap* gameMap, std::istream& is)
{
    return new TrapCannon(gameMap);
}
