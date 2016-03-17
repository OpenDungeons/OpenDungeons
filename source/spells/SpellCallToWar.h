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

#ifndef SPELLCALLTOWAR_H
#define SPELLCALLTOWAR_H

#include "spells/Spell.h"
#include "spells/SpellType.h"

class GameMap;
class InputCommand;
class InputManager;

class SpellCallToWar : public Spell
{
public:
    SpellCallToWar(GameMap* gameMap);
    virtual ~SpellCallToWar();

    SpellType getSpellType() const override
    { return SpellType::callToWar; }

    bool canSlap(Seat* seat) override;

    void slap() override;

    static void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet);

    static Spell* getSpellFromStream(GameMap* gameMap, std::istream &is);
    static Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is);

    static const SpellType mSpellType;
};

#endif // SPELLCALLTOWAR_H
