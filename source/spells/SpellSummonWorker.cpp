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

#include "spells/SpellSummonWorker.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "spells/SpellType.h"
#include "spells/SpellManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static SpellManagerRegister<SpellSummonWorker> reg(SpellType::summonWorker, "summonWorker", "Summon worker", "SummonWorkerCooldown");

void SpellSummonWorker::checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());

    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        int32_t price = getNextWorkerPriceForPlayer(gameMap, player);
        if(playerMana < price)
        {
            std::string txt = formatSpellPrice(SpellType::summonWorker, price);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatSpellPrice(SpellType::summonWorker, price);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos, inputManager.mYPos);
        return;
    }

    std::vector<EntityBase*> targets;
    gameMap->playerSelects(targets, inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX,
        inputManager.mLStartDragY, SelectionTileAllowed::groundClaimedAllied, SelectionEntityWanted::tiles, player);

    int32_t nbFreeWorkers = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerNbFree");
    int32_t nbWorkers = gameMap->getNbWorkersForSeat(player->getSeat());
    int32_t pricePerWorker = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerBasePrice");
    if(nbWorkers > nbFreeWorkers)
        pricePerWorker *= std::pow(2, nbWorkers - nbFreeWorkers);

    int32_t priceTotal = 0;

    std::vector<Tile*> tiles;
    for(EntityBase* target : targets)
    {
        if(target->getObjectType() != GameEntityType::tile)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_LOG_ERR("Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        Tile* tile = static_cast<Tile*>(target);
        ++nbWorkers;
        if(nbWorkers <= nbFreeWorkers)
        {
            tiles.push_back(tile);
            continue;
        }

        int32_t newPrice = priceTotal + pricePerWorker;
        if(newPrice > playerMana)
                break;

        tiles.push_back(tile);
        priceTotal = newPrice;
        pricePerWorker *= 2;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        std::string txt = formatSpellPrice(SpellType::summonWorker, priceTotal);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        inputCommand.selectTiles(tiles);
        return;
    }

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = SpellManager::createSpellClientNotification(SpellType::summonWorker);
    uint32_t nbTiles = tiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : tiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool SpellSummonWorker::castSpell(GameMap* gameMap, Player* player, ODPacket& packet)
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);
    std::vector<Tile*> tiles;
    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
            return false;

        tiles.push_back(tile);
    }

    if(tiles.empty())
        return false;

    return summonWorkersOnTiles(gameMap, player, tiles);
}

bool SpellSummonWorker::summonWorkersOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles)
{
    // Creates a creature from the first worker class found for the given faction.
    const CreatureDefinition* classToSpawn = player->getSeat()->getWorkerClassToSpawn();

    if (classToSpawn == nullptr)
    {
        OD_LOG_ERR("No worker creature definition, class=nullptr, player=" + player->getNick());
        return false;
    }

    int32_t nbFreeWorkers = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerNbFree");
    int32_t nbWorkers = gameMap->getNbWorkersForSeat(player->getSeat());
    int32_t pricePerWorker = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerBasePrice");
    if(nbWorkers > nbFreeWorkers)
        pricePerWorker *= std::pow(2, nbWorkers - nbFreeWorkers);

    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    int32_t priceTotal = 0;

    std::vector<Tile*> tilesSummon;
    for(Tile* tile : tiles)
    {
        if(nbWorkers < nbFreeWorkers)
        {
            tilesSummon.push_back(tile);
            ++nbWorkers;
            continue;
        }

        int32_t newPrice = priceTotal + pricePerWorker;
        if(newPrice > playerMana)
                break;

        tilesSummon.push_back(tile);
        priceTotal = newPrice;
        pricePerWorker *= 2;
    }

    if(!player->getSeat()->takeMana(priceTotal))
        return false;

    for(Tile* tile : tilesSummon)
    {
        Creature* newCreature = new Creature(gameMap, true, classToSpawn, player->getSeat());
        OD_LOG_INF("Spawning a creature class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(player->getSeat()->getId()));

        newCreature->addToGameMap();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                    static_cast<Ogre::Real>(tile->getY()),
                                    static_cast<Ogre::Real>(0.0));
        newCreature->createMesh();
        newCreature->setPosition(spawnPosition);
    }

    return true;
}

int32_t SpellSummonWorker::getNextWorkerPriceForPlayer(GameMap* gameMap, Player* player)
{
    int32_t nbWorkers = gameMap->getNbWorkersForSeat(player->getSeat());
    int32_t nbFreeWorkers = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerNbFree");
    if(nbWorkers < nbFreeWorkers)
        return 0;

    int32_t price = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerBasePrice");
    price *= std::pow(2, nbWorkers - nbFreeWorkers);

    return price;
}

Spell* SpellSummonWorker::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    OD_LOG_ERR("SpellSummonWorker cannot be read from stream");
    return nullptr;
}

Spell* SpellSummonWorker::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    OD_LOG_ERR("SpellSummonWorker cannot be read from packet");
    return nullptr;
}
