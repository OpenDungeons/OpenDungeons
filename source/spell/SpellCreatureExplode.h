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

#ifndef SPELLCREATUREEXPLODE_H
#define SPELLCREATUREEXPLODE_H

#include "spell/Spell.h"
#include "spell/SpellType.h"

class GameMap;

class SpellCreatureExplode : public Spell
{
public:
    static int getSpellCost(std::vector<EntityBase*>& targets, GameMap* gameMap, SpellType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    static void castSpell(GameMap* gameMap, const std::vector<EntityBase*>& targets, Player* player);

    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);
};

#endif // SPELLCREATUREEXPLODE_H
