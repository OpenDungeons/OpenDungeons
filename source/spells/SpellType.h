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

#ifndef SPELLTYPE_H
#define SPELLTYPE_H

#include <iosfwd>
#include <string>

class ODPacket;

enum class SpellType
{
    nullSpellType = 0,
    summonWorker,
    callToWar,
    creatureHeal,
    creatureExplosion,
    creatureHaste,
    creatureDefense,
    creatureSlow,
    creatureStrength,
    creatureWeak,
    nbSpells     // Must be the last in this enum
};

std::istream& operator>>(std::istream& is, SpellType& tt);
std::ostream& operator<<(std::ostream& os, const SpellType& tt);

ODPacket& operator>>(ODPacket& is, SpellType& tt);
ODPacket& operator<<(ODPacket& os, const SpellType& tt);


#endif // SPELLTYPE_H
