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

#ifndef SPELLSUMMONWORKER_H
#define SPELLSUMMONWORKER_H

#include "spells/Spell.h"

class GameMap;
class InputCommand;
class InputManager;

class SpellSummonWorker : public Spell
{
public:
    static void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet);
    static bool summonWorkersOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles);
    static int32_t getNextWorkerPriceForPlayer(GameMap* gameMap, Player* player);

    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);

    static const SpellType mSpellType;
};

#endif // SPELLSUMMONWORKER_H
