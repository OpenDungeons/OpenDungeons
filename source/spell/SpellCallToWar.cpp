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

#include "gamemap/GameMap.h"

#include "utils/ConfigManager.h"
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

int SpellCallToWar::getSpellCallToWarCost(GameMap* gameMap, const std::vector<Tile*>& tiles, Player* player)
{
    // Call to war can be cast on every tile where fullness = 0 (no matter type or vision)
    int32_t priceTotal = 0;
    int32_t pricePerTile = ConfigManager::getSingleton().getSpellConfigInt32("CallToWarPrice");
    for(Tile* tile : tiles)
    {
        if(tile->getFullness() > 0)
            continue;

        priceTotal += pricePerTile;
    }

    return priceTotal;
}

void SpellCallToWar::castSpellCallToWar(GameMap* gameMap, const std::vector<Tile*>& tiles, Player* player)
{
    for(Tile* tile : tiles)
    {
        if(tile->getFullness() > 0)
            continue;

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

