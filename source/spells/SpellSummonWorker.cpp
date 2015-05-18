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

#include "spells/SpellType.h"
#include "spells/SpellManager.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static SpellManagerRegister<SpellSummonWorker> reg(SpellType::summonWorker, "summonWorker");

int SpellSummonWorker::getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    std::vector<EntityBase*> tiles;
    gameMap->playerSelects(tiles, tileX1, tileY1, tileX2, tileY2, SelectionTileAllowed::groundClaimedAllied,
        SelectionEntityWanted::tiles, player);

    int32_t nbFreeWorkers = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerNbFree");
    int32_t nbWorkers = gameMap->getNbWorkersForSeat(player->getSeat());
    int32_t pricePerWorker = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerBasePrice");
    if(nbWorkers > nbFreeWorkers)
    {
        pricePerWorker *= std::pow(2, nbWorkers - nbFreeWorkers);
    }

    if(tiles.empty())
        return pricePerWorker;

    int32_t priceTotal = 0;
    int32_t nbWorkersSummoned = 0;
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());

    for(EntityBase* target : tiles)
    {
        if(target->getObjectType() != GameEntityType::tile)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_ASSERT_TRUE_MSG(false, "Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        ++nbWorkers;
        if(nbWorkers <= nbFreeWorkers)
        {
            ++nbWorkersSummoned;
            continue;
        }

        int32_t newPrice = priceTotal + pricePerWorker;
        if(newPrice > playerMana)
        {
            // If the spell is more expensive than the mana we have, we return the last maximum we can afford.
            if(nbWorkersSummoned > 0)
                break;

            return newPrice;
        }
        ++nbWorkersSummoned;
        priceTotal = newPrice;
        pricePerWorker *= 2;
    }

    if(nbWorkersSummoned <= 0)
        return priceTotal;

    std::random_shuffle(tiles.begin(), tiles.end());
    for(EntityBase* tile : tiles)
    {
        if(nbWorkersSummoned <= 0)
            break;

        --nbWorkersSummoned;
        targets.push_back(tile);
    }

    return priceTotal;
}

void SpellSummonWorker::castSpell(GameMap* gameMap, const std::vector<EntityBase*>& targets, Player* player)
{
    player->setSpellCooldownTurns(SpellType::summonWorker, ConfigManager::getSingleton().getSpellConfigUInt32("SummonWorkerCooldown"));
    // Creates a creature from the first worker class found for the given faction.
    const CreatureDefinition* classToSpawn = player->getSeat()->getWorkerClassToSpawn();

    if (classToSpawn == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "No worker creature definition, class=nullptr, player="
            + player->getNick());
        return;
    }

    for(EntityBase* target : targets)
    {
        if(target->getObjectType() != GameEntityType::tile)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_ASSERT_TRUE_MSG(false, "Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        Tile* tile = static_cast<Tile*>(target);
         // Create a new creature and copy over the class-based creature parameters.
        Creature* newCreature = new Creature(gameMap, classToSpawn, player->getSeat());
        LogManager::getSingleton().logMessage("Spawning a creature class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(player->getSeat()->getId()));

        newCreature->addToGameMap();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                    static_cast<Ogre::Real>(tile->getY()),
                                    static_cast<Ogre::Real>(0.0));
        newCreature->createMesh();
        newCreature->setPosition(spawnPosition, false);
   }
}

Spell* SpellSummonWorker::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    OD_ASSERT_TRUE_MSG(false, "SpellSummonWorker cannot be read from stream");
    return nullptr;
}

Spell* SpellSummonWorker::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    OD_ASSERT_TRUE_MSG(false, "SpellSummonWorker cannot be read from packet");
    return nullptr;
}
