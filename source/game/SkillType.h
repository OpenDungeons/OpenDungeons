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

#ifndef SKILLTYPE_H
#define SKILLTYPE_H

#include <cstdint>
#include <vector>
#include <string>

class ODPacket;

enum class SkillType
{
    nullSkillType,

    // Rooms
    roomArena,
    roomBridgeStone,
    roomBridgeWooden,
    roomCrypt,
    roomDormitory,
    roomHatchery,
    roomLibrary,
    roomPrison,
    roomTrainingHall,
    roomTreasury,
    roomWorkshop,

    // Traps
    trapBoulder,
    trapCannon,
    trapDoorWooden,
    trapSpike,

    // Spells
    spellCallToWar,
    spellCreatureDefense,
    spellCreatureExplosion,
    spellCreatureHaste,
    spellCreatureHeal,
    spellCreatureSlow,
    spellCreatureStrength,
    spellCreatureWeak,
    spellSummonWorker,

    // This should be the last
    countSkill
};

ODPacket& operator<<(ODPacket& os, const SkillType& type);
ODPacket& operator>>(ODPacket& is, SkillType& type);
std::ostream& operator<<(std::ostream& os, const SkillType& type);
std::istream& operator>>(std::istream& is, SkillType& type);

namespace Skills
{
    //! \brief The skill name as used in level files.
    std::string toString(SkillType type);

    //! \brief The skill type from its string representation.
    SkillType fromString(const std::string& type);

    //! \brief The skill name as seen in game events.
    std::string skillTypeToPlayerVisibleString(SkillType type);
}

#endif // SKILLTYPE_H
