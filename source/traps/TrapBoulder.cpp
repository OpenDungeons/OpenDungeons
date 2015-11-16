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

#include "traps/TrapBoulder.h"

#include "entities/Tile.h"
#include "entities/MissileBoulder.h"
#include "entities/TrapEntity.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

const std::string TrapBoulderName = "Boulder";
const std::string TrapBoulderNameDisplay = "Boulder trap";
const TrapType TrapBoulder::mTrapType = TrapType::boulder;

namespace
{
class TrapBoulderFactory : public TrapFactory
{
    TrapType getTrapType() const override
    { return TrapBoulder::mTrapType; }

    const std::string& getName() const override
    { return TrapBoulderName; }

    const std::string& getNameReadable() const override
    { return TrapBoulderNameDisplay; }

    void checkBuildTrap(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildTrapDefault(gameMap, TrapType::boulder, inputManager, inputCommand);
    }

    bool buildTrap(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getTrapTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = TrapManager::costPerTile(TrapType::boulder);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        TrapBoulder* trap = new TrapBoulder(gameMap);
        return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
    }

    void checkBuildTrapEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildTrapDefaultEditor(gameMap, TrapType::boulder, inputManager, inputCommand);
    }

    bool buildTrapEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        TrapBoulder* trap = new TrapBoulder(gameMap);
        return buildTrapDefaultEditor(gameMap, trap, packet);
    }

    Trap* getTrapFromStream(GameMap* gameMap, std::istream& is) const override
    {
        TrapBoulder* trap = new TrapBoulder(gameMap);
        if(!Trap::importTrapFromStream(*trap, is))
        {
            OD_LOG_ERR("Error while building a trap from the stream");
        }
        return trap;
    }

    bool buildTrapOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = TrapManager::costPerTile(TrapType::boulder);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        TrapBoulder* trap = new TrapBoulder(gameMap);
        return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
    }
};

// Register the factory
static TrapRegister reg(new TrapBoulderFactory);
}

const std::string TrapBoulder::MESH_BOULDER = "Boulder";

TrapBoulder::TrapBoulder(GameMap* gameMap) :
    Trap(gameMap)
{
    mReloadTime = ConfigManager::getSingleton().getTrapConfigUInt32("BoulderReloadTurns");
    mMinDamage = ConfigManager::getSingleton().getTrapConfigDouble("BoulderDamagePerHitMin");
    mMaxDamage = ConfigManager::getSingleton().getTrapConfigDouble("BoulderDamagePerHitMax");
    mNbShootsBeforeDeactivation = ConfigManager::getSingleton().getTrapConfigUInt32("BoulderNbShootsBeforeDeactivation");
    setMeshName("");
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
    Tile* tileChosen = tiles[Random::Uint(0, tiles.size() - 1)];
    // We launch the boulder
    Ogre::Vector3 direction(static_cast<Ogre::Real>(tileChosen->getX() - tile->getX()),
                            static_cast<Ogre::Real>(tileChosen->getY() - tile->getY()),
                            0);
    Ogre::Vector3 position;
    position.x = static_cast<Ogre::Real>(tile->getX());
    position.y = static_cast<Ogre::Real>(tile->getY());
    position.z = 0;
    direction.normalise();
    MissileBoulder* missile = new MissileBoulder(getGameMap(), true, getSeat(), getName(), "Boulder",
        direction, ConfigManager::getSingleton().getTrapConfigDouble("BoulderSpeed"),
        Random::Double(mMinDamage, mMaxDamage), nullptr);
    missile->addToGameMap();
    missile->createMesh();
    missile->setPosition(position);
    // We don't want the missile to stay idle for 1 turn. Because we are in a doUpkeep context,
    // we can safely call the missile doUpkeep as we know the engine will not call it the turn
    // it has been added
    missile->doUpkeep();
    missile->setAnimationState("Triggered", true);

    return true;
}

TrapEntity* TrapBoulder::getTrapEntity(Tile* tile)
{
    return new TrapEntity(getGameMap(), true, getName(), MESH_BOULDER, tile, 0.0, false, isActivated(tile) ? 1.0f : 0.5f);
}
