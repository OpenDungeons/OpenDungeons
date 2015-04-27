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

#include "spell/SpellSummonWorker.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"

#include "game/Player.h"

#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "spell/SpellType.h"

#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

#include <OgreStringConverter.h>

int SpellSummonWorker::getSpellSummonWorkerCost(GameMap* gameMap, const std::vector<Tile*>& tiles, Player* player)
{
    int32_t nbFreeWorkers = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerNbFree");
    int32_t nbWorkers = gameMap->getNbWorkersForSeat(player->getSeat());
    int32_t priceTotal = 0;
    int32_t pricePerWorker = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerBasePrice");
    int32_t maxMana = static_cast<int32_t>(ConfigManager::getSingleton().getMaxManaPerSeat());
    int32_t nbWorkersSummoned = 0;
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    if(nbWorkers > nbFreeWorkers)
    {
        pricePerWorker *= std::pow(2, nbWorkers - nbFreeWorkers);
    }
    for(Tile* tile : tiles)
    {
        if(tile->isFullTile())
            continue;

        if(!tile->isClaimedForSeat(player->getSeat()))
            continue;

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
            if(nbWorkersSummoned == 0)
                return newPrice;
            else
                return priceTotal;
        }
        ++nbWorkersSummoned;
        priceTotal = newPrice;
        pricePerWorker *= 2;

        // To avoid having a too big price (its exponential), we break if we are over the max mana
        if(priceTotal > maxMana)
            return priceTotal;
    }

    return priceTotal;
}

void SpellSummonWorker::castSpellSummonWorker(GameMap* gameMap, const std::vector<Tile*>& tiles, Player* player, int manaSpent)
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

    std::vector<Tile*> tilesCastable;
    for(Tile* tile : tiles)
    {
        if(tile->getFullness() > 0)
            continue;

        if(!tile->isClaimedForSeat(player->getSeat()))
            continue;

        tilesCastable.push_back(tile);
    }

    if(tilesCastable.empty())
        return;

    std::random_shuffle(tilesCastable.begin(), tilesCastable.end());
    int32_t nbFreeWorkers = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerNbFree");
    int32_t nbWorkers = gameMap->getNbWorkersForSeat(player->getSeat());
    int32_t priceTotal = 0;
    int32_t pricePerWorker = ConfigManager::getSingleton().getSpellConfigInt32("SummonWorkerBasePrice");
    if(nbWorkers > nbFreeWorkers)
    {
        pricePerWorker *= std::pow(2, nbWorkers - nbFreeWorkers);
    }
    for(Tile* tile : tilesCastable)
    {
        ++nbWorkers;
        if(nbWorkers > nbFreeWorkers)
        {
            int32_t newPrice = priceTotal + pricePerWorker;
            if(newPrice > manaSpent)
                return;

            priceTotal = newPrice;
            pricePerWorker *= 2;
        }

         // Create a new creature and copy over the class-based creature parameters.
        Creature* newCreature = new Creature(gameMap, classToSpawn, player->getSeat());
        LogManager::getSingleton().logMessage("Spawning a creature class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Ogre::StringConverter::toString(player->getSeat()->getId()));

        newCreature->addToGameMap();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                    static_cast<Ogre::Real>(tile->getY()),
                                    static_cast<Ogre::Real>(0.0));
        newCreature->createMesh();
        newCreature->setPosition(spawnPosition, false);
   }
}
