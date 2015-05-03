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

#include "spell/SpellManager.h"


int SpellManager::getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    std::map<SpellType, SpellFunctions>::iterator it = getMap().find(type);
    if(it == getMap().end())
        return 0;

    return ((*it).second.mGetSpellCostFunc)(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void SpellManager::castSpell(GameMap* gameMap, SpellType type, const std::vector<EntityBase*>& targets,
    Player* player)
{
    std::map<SpellType, SpellFunctions>::iterator it = getMap().find(type);
    if(it != getMap().end())
        ((*it).second.mCastSpellFunc)(gameMap, targets, player);
}

std::map<SpellType, SpellFunctions>& SpellManager::getMap()
{
    static std::map<SpellType, SpellFunctions> spellList;
    return spellList;
}
