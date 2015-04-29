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

#include "spell/SpellCallToWar.h"

#include "entities/Creature.h"
#include "entities/Tile.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

// TODO : use the correct mesh when available
SpellCallToWar::SpellCallToWar(GameMap* gameMap) :
    Spell(gameMap, getSpellNameFromSpellType(getSpellType()), "TrainingDummy1", 0.0,
        ConfigManager::getSingleton().getSpellConfigInt32("CallToWarNbTurnsMax"),
        "Triggered", true)
{
}

SpellCallToWar::~SpellCallToWar()
{
}

bool SpellCallToWar::canSlap(Seat* seat)
{
    // Only the spell caster can slap the spell
    if(getSeat() != seat)
        return false;

    if(!getIsOnMap())
        return false;

    return true;
}

void SpellCallToWar::slap()
{
    if(!getGameMap()->isServerGameMap())
        return;

    removeFromGameMap();
    deleteYourself();
}

int SpellCallToWar::getSpellCallToWarCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    // Call to war can be cast on every tile where fullness = 0 (no matter type or vision)
    std::vector<EntityBase*> tiles;
    gameMap->playerSelects(tiles, tileX1, tileY1, tileX2, tileY2, SelectionTileAllowed::groundTiles,
        SelectionEntityWanted::tiles, player);

    if(tiles.empty())
        return 0;

    int32_t priceTotal = 0;
    int32_t pricePerTile = ConfigManager::getSingleton().getSpellConfigInt32("CallToWarPrice");
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

        priceTotal += pricePerTile;
        playerMana -= pricePerTile;
        if(playerMana < pricePerTile)
            return priceTotal;
    }

    return priceTotal;
}

void SpellCallToWar::castSpellCallToWar(GameMap* gameMap, const std::vector<EntityBase*>& targets, Player* player)
{
    player->setSpellCooldownTurns(SpellType::callToWar, ConfigManager::getSingleton().getSpellConfigUInt32("CallToWarCooldown"));
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
        SpellCallToWar* spell = new SpellCallToWar(gameMap);
        spell->setSeat(player->getSeat());
        spell->addToGameMap();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                    static_cast<Ogre::Real>(tile->getY()),
                                    static_cast<Ogre::Real>(0.0));
        spell->createMesh();
        spell->setPosition(spawnPosition, false);
    }
}

