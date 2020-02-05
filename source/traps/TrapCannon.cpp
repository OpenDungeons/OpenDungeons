/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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
#include "entities/GameEntityType.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "sound/SoundEffectsManager.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"
#include "network/ODServer.h"
#include "network/ServerMode.h"
#include "network/ServerNotification.h"

#include <iostream>

const std::string TrapCannonName = "Cannon";
const std::string TrapCannonNameDisplay = "Cannon trap";
const TrapType TrapCannon::mTrapType = TrapType::cannon;

namespace
{
class TrapCannonFactory : public TrapFactory
{
    TrapType getTrapType() const override
    { return TrapCannon::mTrapType; }

    const std::string& getName() const override
    { return TrapCannonName; }

    const std::string& getNameReadable() const override
    { return TrapCannonNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getTrapConfigInt32("CannonCostPerTile"); }

    const std::string& getMeshName() const override
    {
        static const std::string meshName = "Cannon";
        return meshName;
    }

    void checkBuildTrap(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildTrapDefault(gameMap, TrapType::cannon, inputManager, inputCommand);
    }

    bool buildTrap(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getTrapTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = TrapManager::costPerTile(TrapType::cannon);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        TrapCannon* trap = new TrapCannon(gameMap);
        return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
    }

    void checkBuildTrapEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildTrapDefaultEditor(gameMap, TrapType::cannon, inputManager, inputCommand);
    }

    bool buildTrapEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        TrapCannon* trap = new TrapCannon(gameMap);
        return buildTrapDefaultEditor(gameMap, trap, packet);
    }

    Trap* getTrapFromStream(GameMap* gameMap, std::istream& is) const override
    {
        TrapCannon* trap = new TrapCannon(gameMap);
        if(!Trap::importTrapFromStream(*trap, is))
        {
            OD_LOG_ERR("Error while building a trap from the stream");
        }
        return trap;
    }

    bool buildTrapOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = TrapManager::costPerTile(TrapType::cannon);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        TrapCannon* trap = new TrapCannon(gameMap);
        return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
    }
};

// Register the factory
static TrapRegister reg(new TrapCannonFactory);
}

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
    setMeshName("");
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
    MissileOneHit* missile = new MissileOneHit(getGameMap(), getSeat(), getName(), "Cannonball",
        "", direction, ConfigManager::getSingleton().getTrapConfigDouble("CannonSpeed"),
        Random::Double(mMinDamage, mMaxDamage), 0.0, 0.0, nullptr, false, false, true);
    missile->addToGameMap();
    missile->createMesh();
    missile->setPosition(position);
    // We don't want the missile to stay idle for 1 turn. Because we are in a doUpkeep context,
    // we can safely call the missile doUpkeep as we know the engine will not call it the turn
    // it has been added

    
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::orientEntity, nullptr);

    serverNotification->mPacket << savedTrapEntityName;
    serverNotification->mPacket << direction;    
    ODServer::getSingleton().queueServerNotification(serverNotification);


    missile->doUpkeep();

    fireTrapSound(*tile, "Cannon/Fire");

    return true;
}

TrapEntity* TrapCannon::getTrapEntity(Tile* tile)
{
    TrapEntity *te = new TrapEntity(getGameMap(), *this, reg.getTrapFactory()->getMeshName(), tile, 180.0, false, isActivated(tile) ? 1.0f : 0.5f);
    savedTrapEntityName = te->getName();
    return te;
}

double TrapCannon::getPhysicalDefense() const
{
    return ConfigManager::getSingleton().getTrapConfigUInt32("CannonPhyDef");
}

double TrapCannon::getMagicalDefense() const
{
    return ConfigManager::getSingleton().getTrapConfigUInt32("CannonMagDef");
}

double TrapCannon::getElementDefense() const
{
    return ConfigManager::getSingleton().getTrapConfigUInt32("CannonEleDef");
}
