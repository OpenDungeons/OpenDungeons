/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "game/Spell.h"

#include "entities/Creature.h"

#include <algorithm>
#include <cmath>

double Spell::heal(int spellLevel, Creature *targetCreature, double hp, double maxMana)
{
    const double manaPerHPHealed = 0.1;
    double manaCost = 5;
    double maxHPHealed = (maxMana - manaCost) / manaPerHPHealed;
    maxHPHealed = std::min(maxHPHealed, 10.0 * spellLevel);
    maxHPHealed = std::min(maxHPHealed, hp);

    //TODO: Should lock the HP to prevent race conditions.
    double currentHP = targetCreature->getHP();
    double maxHP = targetCreature->getMaxHp();

    if (currentHP <= 0.0)
        return manaCost;

    double newHP = std::min(currentHP + maxHPHealed, maxHP);
    double amountHealed = newHP - currentHP;

    // Prevent lowering the HP if the creature has more than full HP due to some other effect.
    if (amountHealed > 0.0)
    {
        manaCost += 0.1 * amountHealed;
        targetCreature->setHP(newHP);
    }

    return manaCost;
}
