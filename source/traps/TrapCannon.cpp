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

#include "traps/TrapCannon.h"
#include "network/ODPacket.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "entities/MissileOneHit.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

const Ogre::Real CANNON_MISSILE_HEIGHT = 0.3;

TrapCannon::TrapCannon(GameMap* gameMap) :
    ProximityTrap(gameMap)
{
    mReloadTime = ConfigManager::getSingleton().getTrapConfigUInt32("CannonReloadTurns");
    mRange = ConfigManager::getSingleton().getTrapConfigUInt32("CannonRange");
    mMinDamage = ConfigManager::getSingleton().getTrapConfigDouble("CannonDamagePerHitMin");
    mMaxDamage = ConfigManager::getSingleton().getTrapConfigDouble("CannonDamagePerHitMax");
    setMeshName("Cannon");
}

bool TrapCannon::shoot(Tile* tile)
{
    std::vector<Tile*> visibleTiles = getGameMap()->visibleTiles(tile, mRange);
    std::vector<GameEntity*> enemyObjects = getGameMap()->getVisibleCreatures(visibleTiles, getSeat(), true);

    if(enemyObjects.empty())
        return false;

    // Select an enemy to shoot at.
    GameEntity* targetEnemy = enemyObjects[Random::Uint(0, enemyObjects.size()-1)];

    // Create the cannonball to move toward the enemy creature.
    Ogre::Vector3 direction(static_cast<Ogre::Real>(targetEnemy->getCoveredTiles()[0]->x),
                            static_cast<Ogre::Real>(targetEnemy->getCoveredTiles()[0]->y),
                            CANNON_MISSILE_HEIGHT);

    Ogre::Vector3 position;
    position.x = static_cast<Ogre::Real>(tile->x);
    position.y = static_cast<Ogre::Real>(tile->y);
    position.z = CANNON_MISSILE_HEIGHT;
    direction = direction - position;
    direction.normalise();
    MissileOneHit* missile = new MissileOneHit(getGameMap(), getSeat(), getName(), "Cannonball",
        direction, Random::Double(mMinDamage, mMaxDamage), 0.0, false);
    missile->setPosition(position);
    getGameMap()->addRenderedMovableEntity(missile);
    missile->setMoveSpeed(ConfigManager::getSingleton().getTrapConfigDouble("CannonSpeed"));
    missile->createMesh();
    // We don't want the missile to stay idle for 1 turn. Because we are in a doUpkeep context,
    // we can safely call the missile doUpkeep as we know the engine will not call it the turn
    // it has been added
    missile->doUpkeep();
    return true;
}

RenderedMovableEntity* TrapCannon::notifyActiveSpotCreated(Tile* tile)
{
    return loadBuildingObject(getGameMap(), "Cannon", tile, 90.0);
}

TrapCannon* TrapCannon::getTrapCannonFromStream(GameMap* gameMap, std::istream &is)
{
    TrapCannon* trap = new TrapCannon(gameMap);
    return trap;
}

TrapCannon* TrapCannon::getTrapCannonFromPacket(GameMap* gameMap, ODPacket &is)
{
    TrapCannon* trap = new TrapCannon(gameMap);
    return trap;
}
