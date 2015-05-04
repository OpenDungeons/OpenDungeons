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

#include "spell/SpellCreatureHeal.h"

#include "creatureeffect/CreatureEffectHeal.h"

#include "entities/Creature.h"
#include "entities/Tile.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "spell/SpellType.h"
#include "spell/SpellManager.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static SpellManagerRegister<SpellCreatureHeal> reg(SpellType::creatureHeal, "creatureHeal");

int SpellCreatureHeal::getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    int32_t priceTotal = 0;
    int32_t pricePerTile = ConfigManager::getSingleton().getSpellConfigInt32("CreatureHealPrice");
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    if(playerMana < pricePerTile)
        return pricePerTile;

    std::vector<EntityBase*> creatures;
    gameMap->playerSelects(creatures, tileX1, tileY1, tileX2, tileY2, SelectionTileAllowed::groundClaimedAllied,
        SelectionEntityWanted::creatureAliveOwned, player);

    if(creatures.empty())
        return 0;

    std::random_shuffle(creatures.begin(), creatures.end());
    for(EntityBase* target : creatures)
    {
        if(playerMana < pricePerTile)
            break;

        if(target->getObjectType() != GameEntityType::creature)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_ASSERT_TRUE_MSG(false, "Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        // Only hurt creatures can be healed
        Creature* creature = static_cast<Creature*>(target);
        if(!creature->isHurt())
            continue;

        targets.push_back(target);

        priceTotal += pricePerTile;
        playerMana -= pricePerTile;
    }

    return priceTotal;
}

void SpellCreatureHeal::castSpell(GameMap* gameMap, const std::vector<EntityBase*>& targets, Player* player)
{
    player->setSpellCooldownTurns(SpellType::creatureHeal, ConfigManager::getSingleton().getSpellConfigUInt32("CreatureHealCooldown"));
    for(EntityBase* target : targets)
    {
        if(target->getObjectType() != GameEntityType::creature)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_ASSERT_TRUE_MSG(false, "Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        Creature* creature = static_cast<Creature*>(target);
        if(creature->getHP() <= 0)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_ASSERT_TRUE_MSG(false, "Dead creature target name=" + creature->getName());
            }
            continue;
        }

        CreatureEffectHeal* effect = new CreatureEffectHeal(ConfigManager::getSingleton().getSpellConfigUInt32("CreatureHealDuration"),
            ConfigManager::getSingleton().getSpellConfigDouble("CreatureHealValue"),
            "SpellCreatureHeal");
        creature->addCreatureEffect(effect);
    }
}

Spell* SpellCreatureHeal::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    OD_ASSERT_TRUE_MSG(false, "SpellCreatureHeal cannot be read from stream");
    return nullptr;
}

Spell* SpellCreatureHeal::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    OD_ASSERT_TRUE_MSG(false, "SpellCreatureHeal cannot be read from packet");
    return nullptr;
}
